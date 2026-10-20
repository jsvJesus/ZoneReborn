#include "Engine/Assets/CompiledTreeLoader.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace engine::assets {

    namespace {

        class BinaryReader {
        public:
            explicit BinaryReader(const std::filesystem::path& path) {
                std::ifstream file(path, std::ios::binary | std::ios::ate);

                if (!file) {
                    throw std::runtime_error("Cannot open compiled tree: " + path.string());
                }

                const auto length = file.tellg();

                if (length < 0 || length > 64 * 1024 * 1024) {
                    throw std::runtime_error("Invalid compiled tree size: " + path.string());
                }

                bytes_.resize(static_cast<std::size_t>(length));
                file.seekg(0);

                if (!file.read(bytes_.data(), length)) {
                    throw std::runtime_error("Cannot read compiled tree: " + path.string());
                }
            }

            [[nodiscard]] std::uint32_t u32() {
                require(4);
                std::uint32_t value;
                std::memcpy(&value, bytes_.data() + position_, 4);
                position_ += 4;
                return value;
            }

            [[nodiscard]] float f32() {
                require(4);
                float value;
                std::memcpy(&value, bytes_.data() + position_, 4);
                position_ += 4;
                return value;
            }

            [[nodiscard]] std::string string() {
                const auto length = u32();

                if (length > 1024) {
                    throw std::runtime_error("Compiled tree texture path is too long");
                }

                require(length);
                std::string value(bytes_.data() + position_, length);
                position_ += length;
                return value;
            }

            void skip(std::size_t length) {
                require(length);
                position_ += length;
            }

            [[nodiscard]] std::size_t remaining() const noexcept {
                return bytes_.size() - position_;
            }

        private:
            void require(std::size_t length) const {
                if (length > remaining()) {
                    throw std::runtime_error("Truncated compiled tree");
                }
            }

            std::vector<char> bytes_;
            std::size_t position_ = 0;
        };

        struct SourceVertex {
            std::array<float, 25> values{};
        };

        struct SourceSection {
            std::vector<SourceVertex> vertices;
            std::uint32_t leafCount = 0;
            std::vector<std::uint32_t> firstLod;
            std::string diffuse;
        };

        struct SourceBillboards {
            std::vector<PrimitivesVertex> vertices;
            std::string diffuse;
        };

        [[nodiscard]] SourceSection read_section(
            BinaryReader& reader,
            bool leafSection
        ) {
            SourceSection section;
            const auto count = reader.u32();
            const std::size_t stride = leafSection ? 100 : 64;

            if (count > 100000 || count > reader.remaining() / stride) {
                throw std::runtime_error("Invalid compiled tree vertex count");
            }

            if (leafSection) {
                section.leafCount = count;
            }

            section.vertices.resize(count);

            for (auto& vertex : section.vertices) {
                const std::size_t componentCount = stride / sizeof(float);

                for (std::size_t component = 0;
                     component < componentCount;
                     ++component) {
                    vertex.values[component] = reader.f32();
                }
            }

            const auto lodCount = reader.u32();

            if (lodCount > 32) {
                throw std::runtime_error("Invalid compiled tree LOD count");
            }

            for (std::uint32_t lod = 0; lod < lodCount; ++lod) {
                const auto indexCount = reader.u32();

                if (indexCount > reader.remaining() / 4) {
                    throw std::runtime_error("Invalid compiled tree index count");
                }

                if (section.firstLod.empty() && indexCount > 0) {
                    section.firstLod.resize(indexCount);

                    for (auto& index : section.firstLod) {
                        index = reader.u32();
                    }
                } else {
                    reader.skip(static_cast<std::size_t>(indexCount) * 4);
                }
            }

            section.diffuse = reader.string();
            (void)reader.string();
            return section;
        }

        [[nodiscard]] SourceBillboards read_billboards(BinaryReader& reader) {
            if (reader.u32() != 1) {
                throw std::runtime_error("Unsupported compiled tree billboard layout");
            }

            const auto vertexCount = reader.u32();

            if (vertexCount == 0 || vertexCount > 120 ||
                vertexCount % 6 != 0 ||
                vertexCount > reader.remaining() / 20) {
                throw std::runtime_error("Invalid compiled tree billboard count");
            }

            SourceBillboards billboards;
            billboards.vertices.reserve(vertexCount);

            for (std::uint32_t i = 0; i < vertexCount; ++i) {
                PrimitivesVertex vertex;
                vertex.x = reader.f32();
                vertex.y = reader.f32();
                vertex.z = reader.f32();
                vertex.u = reader.f32();
                vertex.v = -reader.f32();

                if (!std::isfinite(vertex.x) ||
                    !std::isfinite(vertex.y) ||
                    !std::isfinite(vertex.z) ||
                    !std::isfinite(vertex.u) ||
                    !std::isfinite(vertex.v)) {
                    throw std::runtime_error("Non-finite compiled tree billboard");
                }

                billboards.vertices.push_back(vertex);
            }

            if (reader.u32() != 0) {
                throw std::runtime_error("Unsupported compiled tree billboard indices");
            }

            billboards.diffuse = reader.string();
            (void)reader.string();

            if (reader.remaining() != 0) {
                throw std::runtime_error("Unexpected compiled tree trailing data");
            }

            return billboards;
        }

        [[nodiscard]] std::filesystem::path texture_path(
            const std::filesystem::path& packsDirectory,
            const std::string& source
        ) {
            std::string normalized = source;
            std::replace(normalized.begin(), normalized.end(), '\\', '/');

            if (!normalized.starts_with("speedtree/") ||
                normalized.find("..") != std::string::npos) {
                throw std::runtime_error("Invalid compiled tree texture path");
            }

            const std::filesystem::path relative(normalized);

            if (relative.is_absolute() ||
                !std::filesystem::is_regular_file(packsDirectory / relative)) {
                throw std::runtime_error("Compiled tree texture missing: " + normalized);
            }

            return relative;
        }

        void add_material(
            CompiledTreeModel& result,
            const std::filesystem::path& diffuse,
            std::uint32_t startIndex,
            std::uint32_t startVertex,
            bool alphaTest
        ) {
            const auto& mesh = result.mesh;
            const auto indexCount = static_cast<std::uint32_t>(
                mesh.indices.size() - startIndex
            );

            if (indexCount == 0) {
                return;
            }

            const auto groupIndex = static_cast<std::uint32_t>(mesh.groups.size());
            result.mesh.groups.push_back({
                startIndex,
                indexCount,
                startVertex,
                static_cast<std::uint32_t>(mesh.vertices.size() - startVertex)
            });

            VisualMaterial material;
            material.primitiveGroup = groupIndex;
            material.diffusePath = diffuse;
            material.alphaTest = alphaTest;
            material.doubleSided = alphaTest;
            material.alphaReference = alphaTest ? 96 : 0;
            result.materials.materials.push_back(std::move(material));
        }

        void append_strip(
            CompiledTreeModel& result,
            const SourceSection& section,
            const std::filesystem::path& diffuse,
            bool alphaTest
        ) {
            if (section.firstLod.size() < 3 || section.vertices.empty()) {
                return;
            }

            const auto startVertex = static_cast<std::uint32_t>(result.mesh.vertices.size());
            const auto startIndex = static_cast<std::uint32_t>(result.mesh.indices.size());

            for (const auto& source : section.vertices) {
                const auto& v = source.values;

                if (!std::isfinite(v[0]) || !std::isfinite(v[1]) ||
                    !std::isfinite(v[2]) || !std::isfinite(v[6]) ||
                    !std::isfinite(v[7])) {
                    throw std::runtime_error("Non-finite compiled tree vertex");
                }

                result.mesh.vertices.push_back({
                    v[0], v[1], v[2], v[6], -v[7]
                });
            }

            for (std::size_t i = 2; i < section.firstLod.size(); ++i) {
                std::uint32_t a = section.firstLod[i - 2];
                std::uint32_t b = section.firstLod[i - 1];
                const std::uint32_t c = section.firstLod[i];

                if (a >= section.vertices.size() ||
                    b >= section.vertices.size() ||
                    c >= section.vertices.size()) {
                    throw std::runtime_error("Compiled tree index out of range");
                }

                if (a == b || b == c || a == c) {
                    continue;
                }

                if ((i & 1u) != 0) {
                    std::swap(a, b);
                }

                result.mesh.indices.insert(
                    result.mesh.indices.end(),
                    {startVertex + a, startVertex + b, startVertex + c}
                );
            }

            add_material(result, diffuse, startIndex, startVertex, alphaTest);
        }

        void append_billboards(
            CompiledTreeModel& result,
            const SourceBillboards& source,
            const std::filesystem::path& diffuse
        ) {
            const auto startVertex = static_cast<std::uint32_t>(result.mesh.vertices.size());
            const auto startIndex = static_cast<std::uint32_t>(result.mesh.indices.size());
            result.mesh.vertices.insert(
                result.mesh.vertices.end(),
                source.vertices.begin(),
                source.vertices.begin() + 6
            );

            for (std::uint32_t i = 0; i < 6; ++i) {
                result.mesh.indices.push_back(startVertex + i);
            }

            add_material(result, diffuse, startIndex, startVertex, true);
            result.cameraFacing = true;
        }

        void append_leaves(
            CompiledTreeModel& result,
            const SourceSection& section,
            const std::filesystem::path& diffuse
        ) {
            if (section.firstLod.size() < 3 || section.vertices.empty()) {
                return;
            }

            const auto startVertex = static_cast<std::uint32_t>(
                result.mesh.vertices.size()
            );
            const auto startIndex = static_cast<std::uint32_t>(
                result.mesh.indices.size()
            );

            for (const auto& source : section.vertices) {
                const auto& v = source.values;

                if (!std::isfinite(v[0]) || !std::isfinite(v[1]) ||
                    !std::isfinite(v[2]) || !std::isfinite(v[6]) ||
                    !std::isfinite(v[7])) {
                    throw std::runtime_error("Non-finite compiled tree leaf");
                }

                result.mesh.vertices.push_back({
                    v[0], v[1], v[2], v[6], -v[7]
                });
            }

            for (std::size_t i = 0; i + 2 < section.firstLod.size(); i += 3) {
                const auto a = section.firstLod[i];
                const auto b = section.firstLod[i + 1];
                const auto c = section.firstLod[i + 2];

                if (a >= section.vertices.size() ||
                    b >= section.vertices.size() ||
                    c >= section.vertices.size()) {
                    throw std::runtime_error("Compiled tree leaf index out of range");
                }

                const auto& pa = section.vertices[a].values;
                const auto& pb = section.vertices[b].values;
                const auto& pc = section.vertices[c].values;
                const float abx = pb[0] - pa[0];
                const float aby = pb[1] - pa[1];
                const float abz = pb[2] - pa[2];
                const float acx = pc[0] - pa[0];
                const float acy = pc[1] - pa[1];
                const float acz = pc[2] - pa[2];
                const float crossX = aby * acz - abz * acy;
                const float crossY = abz * acx - abx * acz;
                const float crossZ = abx * acy - aby * acx;

                if (crossX * crossX + crossY * crossY +
                    crossZ * crossZ <= 0.00000001f) {
                    continue;
                }

                result.mesh.indices.insert(
                    result.mesh.indices.end(),
                    {startVertex + a, startVertex + b, startVertex + c}
                );
            }

            add_material(result, diffuse, startIndex, startVertex, true);
        }

        void validate_source(const std::filesystem::path& path) {
            std::array<unsigned char, 20> header{};
            std::ifstream file(path, std::ios::binary);

            if (!file.read(
                reinterpret_cast<char*>(header.data()),
                static_cast<std::streamsize>(header.size())
            ) || header[0] != 0xE8 || header[1] != 0x03 ||
                header[2] != 0 || header[3] != 0 ||
                std::string_view(
                    reinterpret_cast<const char*>(header.data() + 8),
                    12
                ) != "__IdvSpt_02_") {
                throw std::runtime_error("Unsupported SpeedTree source: " + path.string());
            }
        }

    }

    CompiledTreeModel load_compiled_tree(
        const std::filesystem::path& packsDirectory,
        const std::filesystem::path& sourceRelativePath
    ) {
        const auto source = sourceRelativePath.generic_string();

        if (!source.starts_with("speedtree/") ||
            source.find("..") != std::string::npos ||
            sourceRelativePath.extension() != ".spt") {
            throw std::runtime_error("Invalid SpeedTree source path");
        }

        const auto fullSource = packsDirectory / sourceRelativePath;
        validate_source(fullSource);

        auto compiledPath = fullSource;
        compiledPath.replace_extension(".ctree");
        BinaryReader reader(compiledPath);

        if (reader.u32() != 105) {
            throw std::runtime_error("Unsupported compiled tree version");
        }

        (void)reader.u32();
        for (int coordinate = 0; coordinate < 6; ++coordinate) {
            (void)reader.f32();
        }

        (void)reader.f32();
        (void)reader.f32();

        const auto branches = read_section(reader, false);
        const auto fronds = read_section(reader, false);
        const auto leaves = read_section(reader, true);
        const auto billboards = read_billboards(reader);

        CompiledTreeModel result;
        result.materials.modelPath = sourceRelativePath;
        result.materials.visualPath = compiledPath;

        if (!branches.firstLod.empty()) {
            append_strip(
                result, branches,
                texture_path(packsDirectory, branches.diffuse), false
            );
        }

        if (!fronds.firstLod.empty()) {
            append_strip(
                result, fronds,
                texture_path(packsDirectory, fronds.diffuse), true
            );
        }

        if (!leaves.firstLod.empty()) {
            append_leaves(
                result, leaves,
                texture_path(packsDirectory, leaves.diffuse)
            );
        }

        // Only assets without detailed triangles use the compiled
        // distant view. Two crossed views create a visible dark seam.
        if (result.mesh.groups.empty() && leaves.leafCount > 0) {
            append_billboards(
                result, billboards,
                texture_path(packsDirectory, billboards.diffuse)
            );
        }

        if (!result.mesh.valid() || result.materials.materials.empty()) {
            throw std::runtime_error("Compiled tree has no renderable geometry");
        }

        return result;
    }

}
