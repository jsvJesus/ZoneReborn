#include "Preview/XRayLevelRenderDataBuilder.h"

#include "Core/Assets/MeshData.h"
#include "Core/Log.h"
#include "Core/World/XRay/XRaySpatialMath.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace
{
    constexpr std::uint8_t VisualTypeStatic = 0u;
    constexpr std::uint8_t VisualTypeHierarchy = 1u;

    constexpr std::uint8_t DeclarationTypeFloat2 = 1u;
    constexpr std::uint8_t DeclarationTypeFloat3 = 2u;
    constexpr std::uint8_t DeclarationTypeColour = 4u;
    constexpr std::uint8_t DeclarationTypeShort2 = 6u;
    constexpr std::uint8_t DeclarationTypeShort4 = 7u;

    constexpr std::uint8_t DeclarationUsagePosition = 0u;
    constexpr std::uint8_t DeclarationUsageNormal = 3u;
    constexpr std::uint8_t DeclarationUsageTexcoord = 5u;
    constexpr std::uint8_t DeclarationUsageTangent = 6u;
    constexpr std::uint8_t DeclarationUsageBinormal = 7u;

    struct VisualReference final
    {
        const core::world::xray::Visual* visual = nullptr;
        std::size_t visualIndex = 0;
    };

    bool CheckedAdd(
        const std::uint64_t left,
        const std::uint64_t right,
        std::uint64_t& result) noexcept
    {
        if (right >
            std::numeric_limits<std::uint64_t>::max() -
                left)
        {
            return false;
        }

        result =
            left +
            right;

        return true;
    }

    bool CheckedMultiply(
        const std::uint64_t left,
        const std::uint64_t right,
        std::uint64_t& result) noexcept
    {
        if (left != 0 &&
            right >
                std::numeric_limits<std::uint64_t>::max() /
                    left)
        {
            return false;
        }

        result =
            left *
            right;

        return true;
    }

    bool ReadAt(
        std::ifstream& stream,
        const std::filesystem::path& path,
        const std::uint64_t offset,
        void* destination,
        const std::size_t size,
        std::string& error)
    {
        if (offset >
            static_cast<std::uint64_t>(
                std::numeric_limits<std::streamoff>::max()) ||
            size >
            static_cast<std::size_t>(
                std::numeric_limits<std::streamsize>::max()))
        {
            error =
                "X-Ray geometry read range is too large.";

            return false;
        }

        stream.clear();
        stream.seekg(
            static_cast<std::streamoff>(
                offset),
            std::ios::beg);

        if (!stream)
        {
            error =
                "Unable to seek in X-Ray geometry file: " +
                path.string();

            return false;
        }

        if (size != 0)
        {
            stream.read(
                static_cast<char*>(
                    destination),
                static_cast<std::streamsize>(
                    size));

            if (!stream)
            {
                error =
                    "Unable to read X-Ray geometry file: " +
                    path.string();

                return false;
            }
        }

        return true;
    }

    const core::world::xray::VertexElement* FindElement(
        const core::world::xray::VertexBuffer& buffer,
        const std::uint8_t usage,
        const std::uint8_t usageIndex = 0u) noexcept
    {
        for (const core::world::xray::VertexElement& element :
             buffer.elements)
        {
            if (element.usage ==
                    usage &&
                element.usageIndex ==
                    usageIndex)
            {
                return
                    &element;
            }
        }

        return nullptr;
    }

    template<typename Value>
    Value ReadVertexValue(
        const std::byte* vertex,
        const std::uint16_t offset) noexcept
    {
        Value result{};

        std::memcpy(
            &result,
            vertex +
                offset,
            sizeof(result));

        return result;
    }

    core::math::Vector3 DecodeColourDirection(
        const std::uint32_t value) noexcept
    {
        constexpr float Scale =
            2.0f /
            255.0f;

        core::math::Vector3 result
        {
            static_cast<float>(
                (value >> 16u) &
                0xFFu) *
                    Scale -
                1.0f,

            static_cast<float>(
                (value >> 8u) &
                0xFFu) *
                    Scale -
                1.0f,

            static_cast<float>(
                value &
                0xFFu) *
                    Scale -
                1.0f
        };

        const float lengthSquared =
            result.x *
                result.x +
            result.y *
                result.y +
            result.z *
                result.z;

        if (lengthSquared >
            0.0000001f)
        {
            const float inverseLength =
                1.0f /
                std::sqrt(
                    lengthSquared);

            result.x *=
                inverseLength;

            result.y *=
                inverseLength;

            result.z *=
                inverseLength;
        }
        else
        {
            result =
            {
                0.0f,
                1.0f,
                0.0f
            };
        }

        return result;
    }

    std::uint32_t PackNormal(
        const core::math::Vector3& normal) noexcept
    {
        const std::int32_t x =
            static_cast<std::int32_t>(
                std::clamp(
                    normal.x,
                    -1.0f,
                    1.0f) *
                1023.0f);

        const std::int32_t y =
            static_cast<std::int32_t>(
                std::clamp(
                    normal.y,
                    -1.0f,
                    1.0f) *
                1023.0f);

        const std::int32_t z =
            static_cast<std::int32_t>(
                std::clamp(
                    normal.z,
                    -1.0f,
                    1.0f) *
                511.0f);

        return
            (
                static_cast<std::uint32_t>(x) &
                0x7FFu
            ) |
            (
                (
                    static_cast<std::uint32_t>(y) &
                    0x7FFu
                ) <<
                11u
            ) |
            (
                (
                    static_cast<std::uint32_t>(z) &
                    0x3FFu
                ) <<
                22u
            );
    }

    std::string Lowercase(
        std::string value)
    {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](const unsigned char character)
            {
                return
                    static_cast<char>(
                        std::tolower(
                            character));
            });

        return value;
    }

    bool NormalizeTexturePath(
        const std::string& source,
        std::filesystem::path& output)
    {
        output.clear();

        if (source.empty() ||
            source.front() ==
                '$')
        {
            return false;
        }

        std::string normalized =
            source;

        std::replace(
            normalized.begin(),
            normalized.end(),
            '\\',
            '/');

        std::filesystem::path path(
            normalized);

        if (path.is_absolute() ||
            path.has_root_path())
        {
            return false;
        }

        path =
            path.lexically_normal();

        for (const std::filesystem::path& component :
             path)
        {
            if (component ==
                    ".." ||
                component ==
                    ".")
            {
                return false;
            }
        }

        if (path.empty())
        {
            return false;
        }

        if (Lowercase(
                path.extension().string()) !=
            ".dds")
        {
            path +=
                ".dds";
        }

        output =
            std::move(
                path);

        return true;
    }

    bool ReadTextureFile(
        const std::filesystem::path& path,
        std::vector<std::byte>& output,
        std::string& error)
    {
        std::ifstream stream(
            path,
            std::ios::binary |
                std::ios::ate);

        if (!stream)
        {
            error =
                "Unable to open X-Ray texture: " +
                path.string();

            return false;
        }

        const std::streampos end =
            stream.tellg();

        if (end <
                0 ||
            static_cast<std::uint64_t>(
                end) >
                std::numeric_limits<std::size_t>::max() ||
            static_cast<std::uint64_t>(
                end) >
                static_cast<std::uint64_t>(
                    std::numeric_limits<std::streamsize>::max()))
        {
            error =
                "Unable to determine X-Ray texture size: " +
                path.string();

            return false;
        }

        output.resize(
            static_cast<std::size_t>(
                end));

        stream.seekg(
            0,
            std::ios::beg);

        if (!output.empty())
        {
            stream.read(
                reinterpret_cast<char*>(
                    output.data()),
                static_cast<std::streamsize>(
                    output.size()));

            if (!stream)
            {
                error =
                    "Unable to read X-Ray texture: " +
                    path.string();

                return false;
            }
        }

        return true;
    }

    bool ResolveTexture(
        const core::world::xray::LevelData& level,
        const std::string& textureName,
        const bool levelLocal,
        client::graphics::Renderer& renderer,
        std::unordered_map<std::string, std::int32_t>& cache,
        client::preview::XRayLevelRenderStatistics& statistics,
        std::int32_t& outputTextureIndex,
        std::string& error)
    {
        outputTextureIndex =
            -1;

        std::filesystem::path relativePath;

        if (!NormalizeTexturePath(
                textureName,
                relativePath))
        {
            return true;
        }

        const std::string cacheKey =
            std::string(
                levelLocal
                    ? "level/"
                    : "textures/") +
            Lowercase(
                relativePath.generic_string());

        const auto existing =
            cache.find(
                cacheKey);

        if (existing !=
            cache.end())
        {
            outputTextureIndex =
                existing->second;

            return true;
        }

        std::filesystem::path textureRoot =
            levelLocal
                ? level.levelDirectory
                : level.levelDirectory.
                      parent_path().
                      parent_path() /
                      "textures";

        std::filesystem::path texturePath =
            textureRoot /
            relativePath;

        std::error_code filesystemError;

        bool resolvedLevelLocal =
            levelLocal;

        bool textureExists =
            std::filesystem::is_regular_file(
                texturePath,
                filesystemError);

        if (!textureExists &&
            !levelLocal)
        {
            filesystemError.clear();

            const std::filesystem::path levelTexturePath =
                level.levelDirectory /
                relativePath;

            if (std::filesystem::is_regular_file(
                    levelTexturePath,
                    filesystemError))
            {
                texturePath =
                    levelTexturePath;

                resolvedLevelLocal =
                    true;

                textureExists =
                    true;
            }
        }

        if (!textureExists)
        {
            cache.emplace(
                cacheKey,
                -1);

            ++statistics.missingTextureCount;

            return true;
        }

        client::graphics::SceneTextureData
            texture;

        texture.logicalPath =
            std::string(
                resolvedLevelLocal
                    ? "gamedata/levels/" +
                          level.levelDirectory.
                              filename().
                              generic_string() +
                          "/"
                    : "gamedata/textures/") +
            relativePath.generic_string();

        if (!ReadTextureFile(
                texturePath,
                texture.encodedDds,
                error))
        {
            return false;
        }

        if (!renderer.AppendStreamedTexture(
                texture,
                outputTextureIndex,
                error))
        {
            return false;
        }

        cache.emplace(
            cacheKey,
            outputTextureIndex);

        ++statistics.loadedTextureCount;

        return true;
    }

    client::graphics::SceneAlphaMode AlphaModeForShader(
        const std::string& renderer)
    {
        const std::string lowered =
            Lowercase(
                renderer);

        if (lowered.find(
                "aref") !=
            std::string::npos)
        {
            return
                client::graphics::SceneAlphaMode::Cutout;
        }

        if (lowered.find(
                "blend") !=
                std::string::npos ||
            lowered.find(
                "trans") !=
                std::string::npos ||
            lowered.find(
                "water") !=
                std::string::npos)
        {
            return
                client::graphics::SceneAlphaMode::Blend;
        }

        return
            client::graphics::SceneAlphaMode::Opaque;
    }

    bool DecodeVertices(
        std::ifstream& stream,
        const core::world::xray::LevelData& level,
        const core::world::xray::VertexBuffer& source,
        const std::uint32_t firstVertex,
        const std::uint32_t vertexCount,
        std::vector<core::assets::MeshVertex>& output,
        std::string& error)
    {
        const core::world::xray::VertexElement* position =
            FindElement(
                source,
                DeclarationUsagePosition);

        const core::world::xray::VertexElement* normal =
            FindElement(
                source,
                DeclarationUsageNormal);

        const core::world::xray::VertexElement* tangent =
            FindElement(
                source,
                DeclarationUsageTangent);

        const core::world::xray::VertexElement* binormal =
            FindElement(
                source,
                DeclarationUsageBinormal);

        const core::world::xray::VertexElement* texcoord =
            FindElement(
                source,
                DeclarationUsageTexcoord,
                0u);

        const core::world::xray::VertexElement* lightmap =
            FindElement(
                source,
                DeclarationUsageTexcoord,
                1u);

        if (position ==
                nullptr ||
            position->type !=
                DeclarationTypeFloat3)
        {
            error =
                "X-Ray static vertex buffer has no FLOAT3 position.";

            return false;
        }

        if (normal ==
                nullptr ||
            normal->type !=
                DeclarationTypeColour)
        {
            error =
                "X-Ray static vertex buffer has no packed colour normal.";

            return false;
        }

        if (texcoord ==
            nullptr)
        {
            error =
                "X-Ray static vertex buffer has no texture coordinates.";

            return false;
        }

        if (texcoord->type !=
                DeclarationTypeFloat2 &&
            texcoord->type !=
                DeclarationTypeShort2 &&
            texcoord->type !=
                DeclarationTypeShort4)
        {
            error =
                "X-Ray static vertex buffer uses unsupported texture-coordinate type " +
                std::to_string(
                    texcoord->type) +
                ".";

            return false;
        }

        std::uint64_t rawSize = 0;

        if (!CheckedMultiply(
                vertexCount,
                source.stride,
                rawSize) ||
            rawSize >
                std::numeric_limits<std::size_t>::max())
        {
            error =
                "X-Ray vertex slice is too large.";

            return false;
        }

        std::uint64_t sourceOffset = 0;

        if (!CheckedMultiply(
                firstVertex,
                source.stride,
                sourceOffset) ||
            !CheckedAdd(
                source.dataOffset,
                sourceOffset,
                sourceOffset))
        {
            error =
                "X-Ray vertex slice offset overflows.";

            return false;
        }

        std::vector<std::byte>
            raw(
                static_cast<std::size_t>(
                    rawSize));

        if (!ReadAt(
                stream,
                level.geometryFile,
                sourceOffset,
                raw.data(),
                raw.size(),
                error))
        {
            return false;
        }

        output.clear();
        output.resize(
            vertexCount);

        constexpr float PackedUvScale =
            32.0f /
            32768.0f;

        constexpr float LightmapUvScale =
            1.0f /
            32768.0f;

        for (std::size_t vertexIndex = 0;
             vertexIndex <
                output.size();
             ++vertexIndex)
        {
            const std::byte* bytes =
                raw.data() +
                vertexIndex *
                    source.stride;

            core::assets::MeshVertex& vertex =
                output[
                    vertexIndex];

            std::array<float, 3>
                sourcePosition{};

            std::memcpy(
                sourcePosition.data(),
                bytes +
                    position->offset,
                sizeof(sourcePosition));

            if (!std::isfinite(
                    sourcePosition[0]) ||
                !std::isfinite(
                    sourcePosition[1]) ||
                !std::isfinite(
                    sourcePosition[2]))
            {
                error =
                    "X-Ray static vertex contains a non-finite position.";

                return false;
            }

            vertex.position =
            {
                sourcePosition[0],
                sourcePosition[1],
                sourcePosition[2]
            };

            const std::uint32_t packedNormal =
                ReadVertexValue<std::uint32_t>(
                    bytes,
                    normal->offset);

            vertex.packedNormal =
                PackNormal(
                    DecodeColourDirection(
                        packedNormal));

            std::uint32_t packedTangent = 0;
            std::uint32_t packedBinormal = 0;

            if (tangent !=
                    nullptr &&
                tangent->type ==
                    DeclarationTypeColour)
            {
                packedTangent =
                    ReadVertexValue<std::uint32_t>(
                        bytes,
                        tangent->offset);

                vertex.packedTangent =
                    PackNormal(
                        DecodeColourDirection(
                            packedTangent));
            }

            if (binormal !=
                    nullptr &&
                binormal->type ==
                    DeclarationTypeColour)
            {
                packedBinormal =
                    ReadVertexValue<std::uint32_t>(
                        bytes,
                        binormal->offset);

                vertex.packedBinormal =
                    PackNormal(
                        DecodeColourDirection(
                            packedBinormal));
            }

            if (texcoord->type ==
                DeclarationTypeFloat2)
            {
                std::array<float, 2>
                    uv{};

                std::memcpy(
                    uv.data(),
                    bytes +
                        texcoord->offset,
                    sizeof(uv));

                vertex.u =
                    uv[0];

                vertex.v =
                    uv[1];
            }
            else if (texcoord->type ==
                     DeclarationTypeShort2)
            {
                std::array<std::int16_t, 2>
                    uv{};

                std::memcpy(
                    uv.data(),
                    bytes +
                        texcoord->offset,
                    sizeof(uv));

                const float tangentFraction =
                    static_cast<float>(
                        (packedTangent >> 24u) &
                        0xFFu) /
                    255.0f;

                const float binormalFraction =
                    static_cast<float>(
                        (packedBinormal >> 24u) &
                        0xFFu) /
                    255.0f;

                // D3DCOLOR tangent/binormal alpha is UNORM in the original
                // unpack_tc_base shader, not a raw byte-sized UV offset.

                vertex.u =
                    (
                        static_cast<float>(
                            uv[0]) +
                        tangentFraction
                    ) *
                    PackedUvScale;

                vertex.v =
                    (
                        static_cast<float>(
                            uv[1]) +
                        binormalFraction
                    ) *
                    PackedUvScale;
            }
            else
            {
                std::array<std::int16_t, 2>
                    uv{};

                std::memcpy(
                    uv.data(),
                    bytes +
                        texcoord->offset,
                    sizeof(uv));

                vertex.u =
                    static_cast<float>(
                        uv[0]) *
                    PackedUvScale;

                vertex.v =
                    static_cast<float>(
                        uv[1]) *
                    PackedUvScale;
            }

            if (lightmap !=
                nullptr)
            {
                if (lightmap->type ==
                    DeclarationTypeFloat2)
                {
                    std::array<float, 2>
                        uv{};

                    std::memcpy(
                        uv.data(),
                        bytes +
                            lightmap->offset,
                        sizeof(uv));

                    vertex.u2 =
                        uv[0];

                    vertex.v2 =
                        uv[1];
                }
                else if (lightmap->type ==
                         DeclarationTypeShort2)
                {
                    std::array<std::int16_t, 2>
                        uv{};

                    std::memcpy(
                        uv.data(),
                        bytes +
                            lightmap->offset,
                        sizeof(uv));

                    vertex.u2 =
                        static_cast<float>(
                            uv[0]) *
                        LightmapUvScale;

                    vertex.v2 =
                        static_cast<float>(
                            uv[1]) *
                        LightmapUvScale;
                }
            }
        }

        return true;
    }
}

namespace client::preview
{
    bool StreamXRayLevelRenderData(
        const core::world::xray::LevelData& level,
        graphics::Renderer& renderer,
        XRayLevelRenderStatistics& statistics,
        std::string& error)
    {
        statistics =
            {};

        error.clear();

        if (!renderer.BeginStreamedScene(
                error))
        {
            return false;
        }

        std::ifstream geometry(
            level.geometryFile,
            std::ios::binary);

        if (!geometry)
        {
            error =
                "Unable to open X-Ray geometry file: " +
                level.geometryFile.string();

            return false;
        }

        std::vector<std::vector<VisualReference>>
            visualsByVertexBuffer(
                level.vertexBuffers.size());

        for (std::size_t visualIndex = 0;
             visualIndex <
                level.visuals.size();
             ++visualIndex)
        {
            const core::world::xray::Visual& visual =
                level.visuals[
                    visualIndex];

            if (visual.type ==
                VisualTypeHierarchy)
            {
                ++statistics.hierarchyVisualCount;

                continue;
            }

            if (visual.type !=
                    VisualTypeStatic ||
                !visual.geometry.valid)
            {
                ++statistics.skippedVisualCount;

                continue;
            }

            const core::world::xray::GeometryReference& reference =
                visual.geometry;

            if (visual.shaderId >=
                level.shaders.size())
            {
                error =
                    "X-Ray static visual " +
                    std::to_string(
                        visualIndex) +
                    " references a missing shader.";

                return false;
            }

            if (reference.vertexBuffer >=
                    level.vertexBuffers.size() ||
                reference.indexBuffer >=
                    level.indexBuffers.size())
            {
                error =
                    "X-Ray static visual " +
                    std::to_string(
                        visualIndex) +
                    " references a missing geometry buffer.";

                return false;
            }

            const core::world::xray::VertexBuffer& vertexBuffer =
                level.vertexBuffers[
                    reference.vertexBuffer];

            const core::world::xray::IndexBuffer& indexBuffer =
                level.indexBuffers[
                    reference.indexBuffer];

            const std::uint64_t vertexEnd =
                static_cast<std::uint64_t>(
                    reference.vertexBase) +
                reference.vertexCount;

            const std::uint64_t indexEnd =
                static_cast<std::uint64_t>(
                    reference.indexBase) +
                reference.indexCount;

            if (reference.vertexCount ==
                    0u ||
                reference.indexCount ==
                    0u ||
                reference.indexCount %
                    3u !=
                    0u ||
                vertexEnd >
                    vertexBuffer.vertexCount ||
                indexEnd >
                    indexBuffer.indexCount)
            {
                error =
                    "X-Ray static visual " +
                    std::to_string(
                        visualIndex) +
                    " has an invalid geometry range.";

                return false;
            }

            visualsByVertexBuffer[
                reference.vertexBuffer].
                push_back(
                {
                    &visual,
                    visualIndex
                });

            ++statistics.staticVisualCount;
        }

        std::unordered_map<std::string, std::int32_t>
            textureCache;

        for (std::size_t vertexBufferIndex = 0;
             vertexBufferIndex <
                visualsByVertexBuffer.size();
             ++vertexBufferIndex)
        {
            std::vector<VisualReference>& visualReferences =
                visualsByVertexBuffer[
                    vertexBufferIndex];

            if (visualReferences.empty())
            {
                continue;
            }

            std::stable_sort(
                visualReferences.begin(),
                visualReferences.end(),
                [](const VisualReference& left,
                   const VisualReference& right)
                {
                    return
                        left.visual->shaderId <
                        right.visual->shaderId;
                });

            std::uint32_t firstVertex =
                std::numeric_limits<std::uint32_t>::max();

            std::uint32_t lastVertex =
                0u;

            for (const VisualReference& item :
                 visualReferences)
            {
                const core::world::xray::GeometryReference& reference =
                    item.visual->geometry;

                firstVertex =
                    std::min(
                        firstVertex,
                        reference.vertexBase);

                lastVertex =
                    std::max(
                        lastVertex,
                        reference.vertexBase +
                            reference.vertexCount);
            }

            core::assets::MeshData
                mesh;

            mesh.vertexFormat =
                "xray_level_v14";

            mesh.indexFormat =
                core::assets::MeshIndexFormat::UInt32;

            if (!DecodeVertices(
                    geometry,
                    level,
                    level.vertexBuffers[
                        vertexBufferIndex],
                    firstVertex,
                    lastVertex -
                        firstVertex,
                    mesh.vertices,
                    error))
            {
                error =
                    "Unable to decode X-Ray vertex buffer " +
                    std::to_string(
                        vertexBufferIndex) +
                    ": " +
                    error;

                return false;
            }

            graphics::SceneMesh
                sceneMesh;

            const auto finishMaterialGroup =
                [&level,
                 &renderer,
                 &statistics,
                 &textureCache,
                 &mesh,
                 &sceneMesh,
                 &error](
                    const std::uint16_t shaderId,
                    const std::size_t startIndex)
                {
                    const std::size_t indexCount =
                        mesh.indices32.size() -
                        startIndex;

                    if (indexCount ==
                        0u)
                    {
                        return true;
                    }

                    if (startIndex >
                            std::numeric_limits<std::uint32_t>::max() ||
                        indexCount / 3u >
                            std::numeric_limits<std::uint32_t>::max())
                    {
                        error =
                            "X-Ray DX11 material group is too large.";

                        return false;
                    }

                    core::assets::MeshPrimitiveGroup
                        group;

                    group.startIndex =
                        static_cast<std::uint32_t>(
                            startIndex);

                    group.primitiveCount =
                        static_cast<std::uint32_t>(
                            indexCount /
                            3u);

                    group.startVertex =
                        0u;

                    group.vertexCount =
                        static_cast<std::uint32_t>(
                            mesh.vertices.size());

                    mesh.primitiveGroups.push_back(
                        group);

                    const core::world::xray::ShaderReference& shader =
                        level.shaders[
                            shaderId];

                    graphics::SceneModelMaterial
                        material;

                    material.useXRayLighting =
                        true;

                    material.alphaMode =
                        AlphaModeForShader(
                            shader.renderer);

                    if (!shader.textures.empty())
                    {
                        if (!ResolveTexture(
                                level,
                                shader.textures.front(),
                                false,
                                renderer,
                                textureCache,
                                statistics,
                                material.diffuseTextureIndex,
                                error))
                        {
                            return false;
                        }

                        if (material.diffuseTextureIndex >=
                            0)
                        {
                            ++statistics.texturedMaterialCount;
                        }
                    }

                    // X-Ray terrain stores hemi in diffuse alpha, with its
                    // sun lightmap sampled using base UVs (deffer_impl_flat).
                    std::string diffuseName =
                        shader.textures.empty()
                            ? std::string{}
                            : Lowercase(shader.textures.front());

                    std::replace(
                        diffuseName.begin(),
                        diffuseName.end(),
                        '\\',
                        '/');

                    material.useXRayTerrainLightmap =
                        diffuseName.rfind("terrain/", 0u) == 0u &&
                        shader.textures.size() == 2u &&
                        Lowercase(shader.textures[1]).find("_lm") !=
                            std::string::npos;

                    if (material.useXRayTerrainLightmap ||
                        shader.textures.size() >= 3u)
                    {
                        if (!ResolveTexture(
                                level,
                                shader.textures[
                                    material.useXRayTerrainLightmap ? 1u : 2u],
                                true,
                                renderer,
                                textureCache,
                                statistics,
                                material.lightmapTextureIndex,
                                error))
                        {
                            return false;
                        }

                        if (material.lightmapTextureIndex >=
                            0)
                        {
                            ++statistics.lightmappedMaterialCount;
                        }
                    }

                    sceneMesh.modelMaterials.push_back(
                        material);

                    ++statistics.materialGroupCount;

                    return true;
                };

            bool hasActiveShader =
                false;

            std::uint16_t activeShader =
                0u;

            std::size_t materialGroupStart =
                0u;

            for (const VisualReference& item :
                 visualReferences)
            {
                if (!hasActiveShader)
                {
                    activeShader =
                        item.visual->shaderId;

                    materialGroupStart =
                        mesh.indices32.size();

                    hasActiveShader =
                        true;
                }
                else if (activeShader !=
                         item.visual->shaderId)
                {
                    if (!finishMaterialGroup(
                            activeShader,
                            materialGroupStart))
                    {
                        return false;
                    }

                    activeShader =
                        item.visual->shaderId;

                    materialGroupStart =
                        mesh.indices32.size();
                }

                const core::world::xray::GeometryReference& reference =
                    item.visual->geometry;

                const core::world::xray::IndexBuffer& sourceIndices =
                    level.indexBuffers[
                        reference.indexBuffer];

                std::uint64_t indexOffset = 0;

                if (!CheckedMultiply(
                        reference.indexBase,
                        sizeof(std::uint16_t),
                        indexOffset) ||
                    !CheckedAdd(
                        sourceIndices.dataOffset,
                        indexOffset,
                        indexOffset))
                {
                    error =
                        "X-Ray visual index range offset overflows.";

                    return false;
                }

                std::vector<std::uint16_t>
                    indices(
                        reference.indexCount);

                if (!ReadAt(
                        geometry,
                        level.geometryFile,
                        indexOffset,
                        indices.data(),
                        indices.size() *
                            sizeof(std::uint16_t),
                        error))
                {
                    return false;
                }

                if (mesh.indices32.size() >
                    std::numeric_limits<std::uint32_t>::max() -
                        static_cast<std::size_t>(
                            reference.indexCount))
                {
                    error =
                        "X-Ray DX11 mesh index buffer is too large.";

                    return false;
                }

                const std::uint32_t visualStartIndex =
                    static_cast<std::uint32_t>(mesh.indices32.size());
                core::world::xray::spatial::Bounds visualBounds;

                for (const std::uint16_t index :
                     indices)
                {
                    if (index >=
                        reference.vertexCount)
                    {
                        error =
                            "X-Ray static visual " +
                            std::to_string(
                                item.visualIndex) +
                            " contains an out-of-range local index.";

                        return false;
                    }

                    const std::uint32_t rebasedIndex =
                        reference.vertexBase -
                            firstVertex +
                        index;

                    if (rebasedIndex >=
                            mesh.vertices.size())
                    {
                        error =
                            "X-Ray static visual " +
                            std::to_string(
                                item.visualIndex) +
                            " cannot be rebased into its DX11 vertex buffer.";

                        return false;
                    }

                    mesh.indices32.push_back(
                        rebasedIndex);
                    if (!level.homTriangles.empty())
                        visualBounds.Include(mesh.vertices[rebasedIndex].position);
                }

                if (!level.homTriangles.empty())
                {
                    sceneMesh.occlusionRanges.push_back({
                        visualBounds.minimum, visualBounds.maximum,
                        visualStartIndex, reference.indexCount
                    });
                }

                statistics.triangleCount +=
                    reference.indexCount /
                    3u;
            }

            if (hasActiveShader &&
                !finishMaterialGroup(
                    activeShader,
                    materialGroupStart))
            {
                return false;
            }

            if (mesh.indices32.size() >
                std::numeric_limits<std::uint32_t>::max())
            {
                error =
                    "X-Ray DX11 mesh index buffer is too large.";

                return false;
            }

            statistics.vertexCount +=
                mesh.vertices.size();

            sceneMesh.geometry =
                std::move(
                    mesh);

            if (!renderer.AppendStreamedMesh(
                    sceneMesh,
                    error))
            {
                error =
                    "Unable to upload X-Ray vertex buffer " +
                    std::to_string(
                        vertexBufferIndex) +
                    ": " +
                    error;

                return false;
            }

            ++statistics.meshCount;

            if (statistics.meshCount %
                    32u ==
                0u)
            {
                core::Log::Info(
                    std::string(
                        "X-Ray streamed DX11 meshes: ") +
                    std::to_string(
                        statistics.meshCount));
            }
        }

        if (statistics.meshCount ==
            0u)
        {
            error =
                "X-Ray level contains no supported static geometry.";

            return false;
        }

        if (!renderer.FinishStreamedScene(error))
            return false;

        renderer.SetHomOccluders(level.homTriangles);
        return true;
    }
}
