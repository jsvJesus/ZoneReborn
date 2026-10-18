#include "WorldEditor/WorldGenerator/MapLayoutStorage.h"

#include <Windows.h>

#include <bit>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace world_editor::world_generator {

    namespace {

        constexpr std::uint32_t fileMagic = 0x594C525Au;
        constexpr std::size_t maxFileBytes = 16 * 1024 * 1024;
        constexpr std::uint32_t maxRoutePoints = 8192;

        class Writer final {
        public:
            void u32(std::uint32_t value) {
                for (int i = 0; i < 4; ++i) {
                    bytes.push_back(static_cast<std::uint8_t>(
                        (value >> (i * 8)) & 0xFFu
                    ));
                }
            }

            void f32(float value) {
                u32(std::bit_cast<std::uint32_t>(value));
            }

            std::vector<std::uint8_t> bytes;
        };

        class Reader final {
        public:
            Reader(
                const std::vector<std::uint8_t>& source,
                std::size_t limit
            ) : bytes(source), end(limit) {
            }

            [[nodiscard]] std::uint32_t u32() {
                if (offset > end || end - offset < 4) {
                    throw std::runtime_error(
                        "Map Layout file is incomplete"
                    );
                }

                std::uint32_t value = 0;

                for (int i = 0; i < 4; ++i) {
                    value |= static_cast<std::uint32_t>(
                        bytes[offset++]
                    ) << (i * 8);
                }

                return value;
            }

            [[nodiscard]] float f32() {
                return std::bit_cast<float>(u32());
            }

            [[nodiscard]] bool complete() const noexcept {
                return offset == end;
            }

        private:
            const std::vector<std::uint8_t>& bytes;
            std::size_t offset = 0;
            std::size_t end = 0;
        };

        [[nodiscard]] std::uint32_t checksum(
            const std::vector<std::uint8_t>& bytes,
            std::size_t count
        ) noexcept {
            std::uint32_t hash = 2166136261u;

            for (std::size_t i = 0; i < count; ++i) {
                hash = (hash ^ bytes[i]) * 16777619u;
            }

            return hash;
        }

        void write_terrain(
            Writer& output,
            const terrain::Settings& settings
        ) {
            output.u32(settings.samplesX);
            output.u32(settings.samplesZ);
            output.u32(settings.seed);

            output.f32(settings.originX);
            output.f32(settings.originZ);
            output.f32(settings.spacingMeters);
            output.f32(settings.baseHeightMeters);
            output.f32(settings.reliefMeters);
            output.f32(settings.macroScaleMeters);
            output.f32(settings.detailScaleMeters);
        }

        void read_terrain(
            Reader& input,
            const terrain::Settings& expected
        ) {
            terrain::Settings saved;

            saved.samplesX = input.u32();
            saved.samplesZ = input.u32();
            saved.seed = input.u32();

            saved.originX = input.f32();
            saved.originZ = input.f32();
            saved.spacingMeters = input.f32();
            saved.baseHeightMeters = input.f32();
            saved.reliefMeters = input.f32();
            saved.macroScaleMeters = input.f32();
            saved.detailScaleMeters = input.f32();

            if (saved.samplesX != expected.samplesX ||
                saved.samplesZ != expected.samplesZ ||
                saved.seed != expected.seed ||
                saved.originX != expected.originX ||
                saved.originZ != expected.originZ ||
                saved.spacingMeters != expected.spacingMeters ||
                saved.baseHeightMeters != expected.baseHeightMeters ||
                saved.reliefMeters != expected.reliefMeters ||
                saved.macroScaleMeters != expected.macroScaleMeters ||
                saved.detailScaleMeters != expected.detailScaleMeters) {
                throw std::runtime_error(
                    "Map Layout does not match the active Terrain"
                );
            }
        }

        void validate_layout(const MapLayout& layout) {
            if (layout.version != MapLayout::currentVersion ||
                layout.pois.size() != layout.settings.poiCount ||
                layout.exits.size() != layout.settings.exitCount ||
                layout.roads.nodes.size() !=
                    layout.pois.size() + layout.exits.size() ||
                !road::is_connected(layout.roads)) {
                throw std::runtime_error(
                    "Invalid Map Layout structure"
                );
            }

            const std::uint32_t side = map_size_meters(
                layout.settings.mapSize
            );

            if ((side != 512 && side != 1024 &&
                side != 2048 && side != 4096) ||
                layout.pois.empty() ||
                layout.pois.size() > 64 ||
                layout.exits.empty() ||
                layout.exits.size() > 16 ||
                layout.settings.allowedTypes.size() > 5) {
                throw std::runtime_error(
                    "Invalid Map Layout configuration"
                );
            }

            for (std::size_t i = 0; i < layout.pois.size(); ++i) {
                const auto& poi = layout.pois[i];
                const auto& node = layout.roads.nodes[i];

                if (poi.id != i ||
                    !std::isfinite(poi.centerX) ||
                    !std::isfinite(poi.centerY) ||
                    !std::isfinite(poi.centerZ) ||
                    !std::isfinite(poi.widthMeters) ||
                    !std::isfinite(poi.depthMeters) ||
                    poi.widthMeters <= 0.0f ||
                    poi.depthMeters <= 0.0f ||
                    node.kind != road::RoadNodeKind::POI ||
                    node.referenceId != i ||
                    node.x != poi.centerX ||
                    node.z != poi.centerZ) {
                    throw std::runtime_error(
                        "Invalid Map Layout POI"
                    );
                }

                (void)settlement::specification(poi.type);
            }

            for (std::size_t i = 0; i < layout.exits.size(); ++i) {
                const auto& exit = layout.exits[i];

                const auto& node = layout.roads.nodes[
                    layout.pois.size() + i
                ];

                if (exit.id != i ||
                    static_cast<std::uint32_t>(exit.side) > 3 ||
                    !std::isfinite(exit.x) ||
                    !std::isfinite(exit.y) ||
                    !std::isfinite(exit.z) ||
                    node.kind != road::RoadNodeKind::Exit ||
                    node.referenceId != i ||
                    node.x != exit.x ||
                    node.z != exit.z) {
                    throw std::runtime_error(
                        "Invalid Map Layout exit"
                    );
                }
            }

            for (const auto& edge : layout.roads.edges) {
                if (edge.route.size() < 2 ||
                    edge.route.size() > maxRoutePoints) {
                    throw std::runtime_error("Invalid road route point count");
                }

                const auto& from = layout.roads.nodes[edge.from];
                const auto& to = layout.roads.nodes[edge.to];

                if (edge.route.front().x != from.x ||
                    edge.route.front().z != from.z ||
                    edge.route.back().x != to.x ||
                    edge.route.back().z != to.z ||
                    edge.route.front().distanceMeters != 0.0f) {
                    throw std::runtime_error("Road route endpoints do not match graph");
                }

                float previousDistance = -1.0f;

                for (const auto& point : edge.route) {
                    if (!std::isfinite(point.x) ||
                        !std::isfinite(point.y) ||
                        !std::isfinite(point.z) ||
                        !std::isfinite(point.distanceMeters) ||
                        !std::isfinite(point.slopeDegrees) ||
                        point.distanceMeters <= previousDistance ||
                        point.slopeDegrees < 0.0f ||
                        point.slopeDegrees >= 90.0f ||
                        static_cast<std::uint32_t>(point.water) > 3) {
                        throw std::runtime_error("Invalid road height profile");
                    }

                    previousDistance = point.distanceMeters;
                }
            }
        }

        [[nodiscard]] std::filesystem::path pending_path(
            const std::filesystem::path& path
        ) {
            std::filesystem::path pending = path;
            pending += L".pending";
            return pending;
        }

    }

    void save_map_layout(
        const MapLayout& layout,
        const terrain::Settings& terrainSettings,
        const std::filesystem::path& path
    ) {
        validate_layout(layout);

        if (path.empty()) {
            throw std::invalid_argument(
                "Map Layout path is empty"
            );
        }

        Writer output;

        output.u32(fileMagic);
        output.u32(MapLayout::currentVersion);
        output.u32(static_cast<std::uint32_t>(layout.sourceFingerprint));
        output.u32(static_cast<std::uint32_t>(layout.sourceFingerprint >> 32));

        write_terrain(output, terrainSettings);

        output.u32(map_size_meters(layout.settings.mapSize));
        output.u32(layout.settings.poiCount);
        output.u32(layout.settings.exitCount);
        output.f32(layout.settings.poiSeparationMeters);
        output.f32(layout.settings.borderClearanceMeters);

        const auto& seeds = layout.settings.seeds;

        output.u32(seeds.terrain);
        output.u32(seeds.water);
        output.u32(seeds.vegetation);
        output.u32(seeds.location);
        output.u32(seeds.road);
        output.u32(seeds.decoration);
        output.u32(seeds.biome);

        output.u32(static_cast<std::uint32_t>(
            layout.settings.allowedTypes.size()
        ));

        for (const auto type : layout.settings.allowedTypes) {
            output.u32(static_cast<std::uint32_t>(type));
        }

        for (const auto& poi : layout.pois) {
            output.u32(poi.id);
            output.u32(static_cast<std::uint32_t>(poi.type));

            output.f32(poi.centerX);
            output.f32(poi.centerY);
            output.f32(poi.centerZ);
            output.f32(poi.widthMeters);
            output.f32(poi.depthMeters);
        }

        for (const auto& exit : layout.exits) {
            output.u32(exit.id);
            output.u32(static_cast<std::uint32_t>(exit.side));

            output.f32(exit.x);
            output.f32(exit.y);
            output.f32(exit.z);
        }

        output.u32(static_cast<std::uint32_t>(
            layout.roads.nodes.size()
        ));

        for (const auto& node : layout.roads.nodes) {
            output.u32(static_cast<std::uint32_t>(node.kind));
            output.u32(node.referenceId);
            output.f32(node.x);
            output.f32(node.z);
        }

        output.u32(static_cast<std::uint32_t>(
            layout.roads.edges.size()
        ));

        for (const auto& edge : layout.roads.edges) {
            output.u32(edge.from);
            output.u32(edge.to);
            output.u32(static_cast<std::uint32_t>(edge.route.size()));

            for (const auto& point : edge.route) {
                output.f32(point.x);
                output.f32(point.y);
                output.f32(point.z);
                output.f32(point.distanceMeters);
                output.f32(point.slopeDegrees);
                output.u32(static_cast<std::uint32_t>(point.water));
            }
        }

        output.u32(checksum(
            output.bytes,
            output.bytes.size()
        ));

        if (output.bytes.size() > maxFileBytes) {
            throw std::runtime_error(
                "Map Layout exceeds file size limit"
            );
        }

        std::filesystem::create_directories(
            path.parent_path()
        );

        const auto pending = pending_path(path);

        if (std::filesystem::exists(pending)) {
            throw std::runtime_error(
                "Pending Map Layout save already exists"
            );
        }

        try {
            {
                std::ofstream file(
                    pending,
                    std::ios::binary | std::ios::trunc
                );

                if (!file) {
                    throw std::runtime_error(
                        "Cannot create Map Layout file"
                    );
                }

                file.write(
                    reinterpret_cast<const char*>(
                        output.bytes.data()
                    ),
                    static_cast<std::streamsize>(
                        output.bytes.size()
                    )
                );

                file.flush();

                if (!file) {
                    throw std::runtime_error(
                        "Cannot finish Map Layout save"
                    );
                }
            }

            if (!MoveFileExW(
                pending.c_str(),
                path.c_str(),
                MOVEFILE_REPLACE_EXISTING |
                MOVEFILE_WRITE_THROUGH
            )) {
                throw std::runtime_error(
                    "Cannot replace Map Layout file"
                );
            }
        } catch (...) {
            std::error_code error;
            std::filesystem::remove(pending, error);
            throw;
        }
    }

    MapLayout load_map_layout(
        const std::filesystem::path& path,
        const terrain::Settings& terrainSettings,
        std::uint64_t sourceFingerprint
    ) {
        if (!std::filesystem::exists(path) ||
            std::filesystem::file_size(path) < 8 ||
            std::filesystem::file_size(path) > maxFileBytes) {
            throw std::runtime_error(
                "Map Layout file is missing or invalid"
            );
        }

        std::ifstream file(path, std::ios::binary);

        if (!file) {
            throw std::runtime_error(
                "Cannot open Map Layout file"
            );
        }

        const std::vector<std::uint8_t> bytes(
            std::istreambuf_iterator<char>{file},
            std::istreambuf_iterator<char>{}
        );

        if (bytes.size() < 12) {
            throw std::runtime_error(
                "Map Layout header is incomplete"
            );
        }

        Reader checksumReader(bytes, bytes.size());

        // The stored checksum is the final four bytes.
        Reader footer(bytes, bytes.size());

        for (std::size_t i = 0;
             i < bytes.size() - 4;
             i += 4) {
            // No parsing is required before checksum validation.
        }

        const std::size_t checksumOffset = bytes.size() - 4;

        const std::uint32_t savedChecksum =
            static_cast<std::uint32_t>(bytes[checksumOffset]) |
            (static_cast<std::uint32_t>(
                bytes[checksumOffset + 1]
            ) << 8) |
            (static_cast<std::uint32_t>(
                bytes[checksumOffset + 2]
            ) << 16) |
            (static_cast<std::uint32_t>(
                bytes[checksumOffset + 3]
            ) << 24);

        if (checksum(bytes, checksumOffset) != savedChecksum) {
            throw std::runtime_error(
                "Map Layout checksum mismatch"
            );
        }

        Reader input(bytes, checksumOffset);

        const std::uint32_t magic = input.u32();
        const std::uint32_t version = input.u32();

        if (magic != fileMagic ||
            (version != 3 && version != MapLayout::currentVersion)) {
            throw std::runtime_error(
                "Unsupported Map Layout version. Regenerate and save Map Layout."
            );
        }

        const std::uint64_t fingerprintLow = input.u32();
        const std::uint64_t fingerprintHigh = input.u32();
        const std::uint64_t savedFingerprint = fingerprintLow | (fingerprintHigh << 32);

        if (savedFingerprint != sourceFingerprint) {
            throw std::runtime_error("Saved Map Layout does not match Terrain or Hydrology.");
        }

        read_terrain(input, terrainSettings);

        MapLayout layout;
        layout.sourceFingerprint = savedFingerprint;

        layout.settings.mapSize =
            static_cast<MapSize>(input.u32());

        layout.settings.poiCount = input.u32();
        layout.settings.exitCount = input.u32();

        layout.settings.poiSeparationMeters = input.f32();
        layout.settings.borderClearanceMeters = input.f32();

        auto& seeds = layout.settings.seeds;

        seeds.terrain = input.u32();
        seeds.water = input.u32();
        seeds.vegetation = input.u32();
        seeds.location = input.u32();
        seeds.road = input.u32();
        seeds.decoration = input.u32();

        // Version 3 used VegetationSeed for both Biomes and Flora.
        seeds.biome = version >= 4 ? input.u32() : seeds.vegetation;

        const std::uint32_t typeCount = input.u32();

        if (typeCount > 5 ||
            layout.settings.poiCount == 0 ||
            layout.settings.poiCount > 64 ||
            layout.settings.exitCount == 0 ||
            layout.settings.exitCount > 16) {
            throw std::runtime_error(
                "Invalid Map Layout counts"
            );
        }

        for (std::uint32_t i = 0; i < typeCount; ++i) {
            layout.settings.allowedTypes.push_back(
                static_cast<settlement::POIType>(input.u32())
            );
        }

        layout.pois.resize(layout.settings.poiCount);

        for (auto& poi : layout.pois) {
            poi.id = input.u32();
            poi.type = static_cast<settlement::POIType>(
                input.u32()
            );

            poi.centerX = input.f32();
            poi.centerY = input.f32();
            poi.centerZ = input.f32();
            poi.widthMeters = input.f32();
            poi.depthMeters = input.f32();
        }

        layout.exits.resize(layout.settings.exitCount);

        for (auto& exit : layout.exits) {
            exit.id = input.u32();
            exit.side = static_cast<BoundarySide>(
                input.u32()
            );

            exit.x = input.f32();
            exit.y = input.f32();
            exit.z = input.f32();
        }

        const std::uint32_t nodeCount = input.u32();

        if (nodeCount !=
            layout.settings.poiCount + layout.settings.exitCount) {
            throw std::runtime_error(
                "Invalid road node count"
            );
        }

        layout.roads.nodes.resize(nodeCount);

        for (auto& node : layout.roads.nodes) {
            node.kind = static_cast<road::RoadNodeKind>(
                input.u32()
            );

            node.referenceId = input.u32();
            node.x = input.f32();
            node.z = input.f32();
        }

        const std::uint32_t edgeCount = input.u32();

        if (edgeCount > nodeCount * nodeCount) {
            throw std::runtime_error(
                "Invalid road edge count"
            );
        }

        layout.roads.edges.resize(edgeCount);

        for (auto& edge : layout.roads.edges) {
            edge.from = input.u32();
            edge.to = input.u32();

            const std::uint32_t routeCount = input.u32();

            if (routeCount < 2 || routeCount > maxRoutePoints) {
                throw std::runtime_error("Invalid road route point count");
            }

            edge.route.resize(routeCount);

            for (auto& point : edge.route) {
                point.x = input.f32();
                point.y = input.f32();
                point.z = input.f32();
                point.distanceMeters = input.f32();
                point.slopeDegrees = input.f32();
                point.water = static_cast<biome::WaterKind>(input.u32());
            }
        }

        if (!input.complete()) {
            throw std::runtime_error(
                "Unexpected Map Layout file data"
            );
        }

        validate_layout(layout);

        if (map_size_meters(layout.settings.mapSize) !=
            (terrainSettings.samplesX - 1) *
            terrainSettings.spacingMeters) {
            throw std::runtime_error(
                "Map Layout dimensions do not match Terrain"
            );
        }

        return layout;
    }

}
