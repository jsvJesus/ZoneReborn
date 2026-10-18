#include "Engine/Assets/PrimitivesMeshLoader.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace engine::assets {

    namespace {

        constexpr std::uint32_t primitivesMagic =
            0x42A14E65u;

        constexpr std::size_t maxFileSize =
            64u * 1024u * 1024u;

        constexpr std::uint32_t vertexStride = 32u;

        struct Section {
            std::string name;
            std::size_t offset = 0;
            std::size_t size = 0;
        };

        [[nodiscard]] std::vector<std::uint8_t> read_file(
            const std::filesystem::path& path
        ) {
            std::ifstream file(
                path,
                std::ios::binary | std::ios::ate
            );

            if (!file) {
                throw std::runtime_error(
                    "Cannot open primitives: " +
                    path.string()
                );
            }

            const auto end = file.tellg();

            if (end < std::streampos(32) ||
                static_cast<std::uint64_t>(end) >
                    maxFileSize) {
                throw std::runtime_error(
                    "Invalid primitives file size: " +
                    path.string()
                );
            }

            std::vector<std::uint8_t> bytes(
                static_cast<std::size_t>(end)
            );

            file.seekg(0);

            file.read(
                reinterpret_cast<char*>(bytes.data()),
                static_cast<std::streamsize>(
                    bytes.size()
                )
            );

            if (!file) {
                throw std::runtime_error(
                    "Cannot read primitives: " +
                    path.string()
                );
            }

            return bytes;
        }

        [[nodiscard]] std::uint32_t read_u32(
            const std::vector<std::uint8_t>& bytes,
            std::size_t offset
        ) {
            if (offset > bytes.size() ||
                bytes.size() - offset < 4) {
                throw std::runtime_error(
                    "Truncated primitives data"
                );
            }

            std::uint32_t value = 0;

            std::memcpy(
                &value,
                bytes.data() + offset,
                sizeof(value)
            );

            return value;
        }

        [[nodiscard]] std::uint16_t read_u16(
            const std::vector<std::uint8_t>& bytes,
            std::size_t offset
        ) {
            if (offset > bytes.size() ||
                bytes.size() - offset < 2) {
                throw std::runtime_error(
                    "Truncated primitives indices"
                );
            }

            std::uint16_t value = 0;

            std::memcpy(
                &value,
                bytes.data() + offset,
                sizeof(value)
            );

            return value;
        }

        [[nodiscard]] float read_f32(
            const std::vector<std::uint8_t>& bytes,
            std::size_t offset
        ) {
            const std::uint32_t raw =
                read_u32(bytes, offset);

            float value = 0.0f;

            std::memcpy(
                &value,
                &raw,
                sizeof(value)
            );

            return value;
        }

        [[nodiscard]] std::string read_text(
            const std::vector<std::uint8_t>& bytes,
            std::size_t offset,
            std::size_t length
        ) {
            if (offset > bytes.size() ||
                length > bytes.size() - offset) {
                throw std::runtime_error(
                    "Invalid primitives string"
                );
            }

            return std::string(
                reinterpret_cast<const char*>(
                    bytes.data() + offset
                ),
                length
            );
        }

        [[nodiscard]] std::string read_format(
            const std::vector<std::uint8_t>& bytes,
            const Section& section
        ) {
            if (section.size < 64) {
                throw std::runtime_error(
                    "Invalid primitives format header"
                );
            }

            const char* text = reinterpret_cast<const char*>(
                bytes.data() + section.offset
            );

            std::size_t length = 0;

            while (length < 64 && text[length] != '\0') {
                ++length;
            }

            if (length == 64) {
                throw std::runtime_error(
                    "Unterminated primitives format"
                );
            }

            return std::string(text, length);
        }

        [[nodiscard]] std::size_t aligned_size(
            std::size_t size
        ) {
            if (size >
                (std::numeric_limits<std::size_t>::max)() -
                    3) {
                throw std::runtime_error(
                    "Primitives section overflow"
                );
            }

            return (size + 3) & ~std::size_t{3};
        }

        [[nodiscard]] std::vector<Section> read_sections(
            const std::vector<std::uint8_t>& bytes
        ) {
            if (read_u32(bytes, 0) != primitivesMagic) {
                throw std::runtime_error(
                    "Unsupported primitives signature"
                );
            }

            const std::size_t directorySize =
                read_u32(bytes, bytes.size() - 4);

            if (directorySize == 0 ||
                directorySize > bytes.size() - 8) {
                throw std::runtime_error(
                    "Invalid primitives section directory"
                );
            }

            const std::size_t directoryStart =
                bytes.size() - 4 - directorySize;

            std::size_t directoryOffset = directoryStart;
            std::size_t dataOffset = 4;

            std::vector<Section> sections;

            while (directoryOffset < bytes.size() - 4) {
                const std::size_t directoryEnd =
                    bytes.size() - 4;

                if (directoryEnd - directoryOffset < 24) {
                    throw std::runtime_error(
                        "Truncated primitives directory entry"
                    );
                }

                const std::size_t dataSize =
                    read_u32(bytes, directoryOffset);

                const std::size_t nameLength =
                    read_u32(bytes, directoryOffset + 20);

                if (nameLength == 0 ||
                    nameLength > 255) {
                    throw std::runtime_error(
                        "Invalid primitives section name"
                    );
                }

                const std::size_t paddedName =
                    aligned_size(nameLength);

                if (directoryEnd - directoryOffset - 24 <
                    paddedName) {
                    throw std::runtime_error(
                        "Truncated primitives section name"
                    );
                }

                const std::string name = read_text(
                    bytes,
                    directoryOffset + 24,
                    nameLength
                );

                const std::size_t paddedData =
                    aligned_size(dataSize);

                if (dataOffset > directoryStart ||
                    paddedData >
                        directoryStart - dataOffset) {
                    throw std::runtime_error(
                        "Primitives section exceeds file"
                    );
                }

                sections.push_back({
                    name,
                    dataOffset,
                    dataSize
                });

                dataOffset += paddedData;

                directoryOffset +=
                    24 + paddedName;
            }

            if (dataOffset != directoryStart) {
                throw std::runtime_error(
                    "Primitives section offsets do not match"
                );
            }

            return sections;
        }

        [[nodiscard]] const Section& find_section(
            const std::vector<Section>& sections,
            std::string_view name
        ) {
            const auto found = std::find_if(
                sections.begin(),
                sections.end(),
                [name](const Section& section) {
                    return section.name == name;
                }
            );

            if (found == sections.end()) {
                throw std::runtime_error(
                    "Missing primitives section: " +
                    std::string(name)
                );
            }

            return *found;
        }

        void read_vertices(
            const std::vector<std::uint8_t>& bytes,
            const Section& section,
            PrimitivesMesh& mesh
        ) {
            if (section.size < 68 ||
                read_format(bytes, section) != "xyznuvtb") {
                throw std::runtime_error(
                    "Unsupported primitives vertex format"
                );
            }

            const std::uint32_t count =
                read_u32(bytes, section.offset + 64);

            if (count == 0 || count > 1000000 ||
                static_cast<std::uint64_t>(count) *
                    vertexStride + 68 != section.size) {
                throw std::runtime_error(
                    "Invalid primitives vertex buffer"
                );
            }

            mesh.vertices.reserve(count);

            for (std::uint32_t i = 0; i < count; ++i) {
                const std::size_t offset =
                    section.offset + 68 +
                    static_cast<std::size_t>(i) *
                    vertexStride;

                PrimitivesVertex vertex;

                vertex.x = read_f32(bytes, offset);
                vertex.y = read_f32(bytes, offset + 4);
                vertex.z = read_f32(bytes, offset + 8);

                // xyznuvtb:
                // XYZ = 12 bytes
                // packed normal = 4 bytes
                // UV = 8 bytes
                // packed tangent/binormal = 8 bytes.
                vertex.u = read_f32(bytes, offset + 16);
                vertex.v = read_f32(bytes, offset + 20);

                if (!std::isfinite(vertex.x) ||
                    !std::isfinite(vertex.y) ||
                    !std::isfinite(vertex.z) ||
                    !std::isfinite(vertex.u) ||
                    !std::isfinite(vertex.v)) {
                    throw std::runtime_error(
                        "Non-finite primitives vertex"
                    );
                }

                mesh.vertices.push_back(vertex);
            }
        }

        void read_indices(
            const std::vector<std::uint8_t>& bytes,
            const Section& section,
            PrimitivesMesh& mesh
        ) {
            if (section.size < 72) {
                throw std::runtime_error(
                    "Invalid primitives index header"
                );
            }

            const std::string format = read_format(bytes, section);

            if (format != "list" && format != "list32") {
                throw std::runtime_error(
                    "Unsupported primitives index format"
                );
            }

            const std::uint32_t indexStride = format == "list32" ? 4u : 2u;

            const std::uint32_t count =
                read_u32(bytes, section.offset + 64);

            const std::uint32_t groupCount =
                read_u32(bytes, section.offset + 68);

            if (count == 0 ||
                count > 3000000 ||
                count % 3 != 0 ||
                groupCount > 100000) {
                throw std::runtime_error(
                    "Invalid primitives index count"
                );
            }

            const std::uint64_t expectedSize =
                72ull +
                static_cast<std::uint64_t>(count) * indexStride +
                static_cast<std::uint64_t>(groupCount) * 16ull;

            if (expectedSize != section.size) {
                throw std::runtime_error(
                    "Invalid primitives index buffer size"
                );
            }

            const std::size_t indicesOffset =
                section.offset + 72;

            const std::size_t groupsOffset =
                indicesOffset +
                static_cast<std::size_t>(count) * indexStride;

            mesh.indices.reserve(count);
            mesh.groups.reserve(groupCount);

            for (std::uint32_t i = 0; i < count; ++i) {
                const std::size_t offset = indicesOffset +
                    static_cast<std::size_t>(i) * indexStride;

                const std::uint32_t index = indexStride == 4
                    ? read_u32(bytes, offset)
                    : read_u16(bytes, offset);

                if (index >= mesh.vertices.size()) {
                    throw std::runtime_error(
                        "Primitives index exceeds vertex count"
                    );
                }

                mesh.indices.push_back(index);
            }

            // Group layout confirmed against grass and tree
            // samples: startIndex, triangleCount,
            // startVertex, vertexCount.
            std::uint64_t coveredTriangles = 0;

            for (std::uint32_t i = 0; i < groupCount; ++i) {
                const std::size_t offset =
                    groupsOffset +
                    static_cast<std::size_t>(i) * 16;

                const std::uint32_t startIndex =
                    read_u32(bytes, offset);

                const std::uint32_t triangleCount =
                    read_u32(bytes, offset + 4);

                const std::uint32_t startVertex =
                    read_u32(bytes, offset + 8);

                const std::uint32_t vertexCount =
                    read_u32(bytes, offset + 12);

                if (static_cast<std::uint64_t>(startIndex) +
                        static_cast<std::uint64_t>(
                            triangleCount
                        ) * 3ull > count ||
                    static_cast<std::uint64_t>(startVertex) +
                        vertexCount > mesh.vertices.size()) {
                    throw std::runtime_error(
                        "Invalid primitives material group"
                    );
                }

                PrimitivesGroup group;

                group.startIndex = startIndex;
                group.indexCount = triangleCount * 3;
                group.startVertex = startVertex;
                group.vertexCount = vertexCount;

                mesh.groups.push_back(group);

                coveredTriangles += triangleCount;
            }

            if (groupCount == 0 ||
                coveredTriangles != count / 3) {
                throw std::runtime_error(
                    "Primitives index groups are incomplete"
                );
            }
        }

    }

    bool PrimitivesMesh::valid() const noexcept {
        if (vertices.empty() ||
            indices.empty() ||
            indices.size() % 3 != 0) {
            return false;
        }

        for (const std::uint32_t index : indices) {
            if (index >= vertices.size()) {
                return false;
            }
        }

        return true;
    }

    PrimitivesMesh load_primitives_mesh(
        const std::filesystem::path& path
    ) {
        try {
            const auto bytes = read_file(path);
            const auto sections = read_sections(bytes);

            PrimitivesMesh mesh;

            read_vertices(
                bytes,
                find_section(sections, "vertices"),
                mesh
            );

            read_indices(
                bytes,
                find_section(sections, "indices"),
                mesh
            );

            if (!mesh.valid()) {
                throw std::runtime_error(
                    "Invalid imported geometry"
                );
            }

            return mesh;
        } catch (const std::exception& error) {
            throw std::runtime_error(
                path.string() + ": " + error.what()
            );
        }
    }

}
