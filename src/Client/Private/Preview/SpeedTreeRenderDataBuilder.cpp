#include "Preview/SpeedTreeRenderDataBuilder.h"

#include "Core/Images/DdsDecoder.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace
{
    core::math::Vector3 Normalize(
        const core::math::Vector3& value) noexcept
    {
        const float lengthSquared =
            value.x * value.x +
            value.y * value.y +
            value.z * value.z;

        if (lengthSquared <=
            0.000001f)
        {
            return
            {
                0.0f,
                1.0f,
                0.0f
            };
        }

        const float inverseLength =
            1.0f /
            std::sqrt(
                lengthSquared);

        return
        {
            value.x *
                inverseLength,

            value.y *
                inverseLength,

            value.z *
                inverseLength
        };
    }

    core::math::Vector3 Add(
        const core::math::Vector3& a,
        const core::math::Vector3& b) noexcept
    {
        return
        {
            a.x + b.x,
            a.y + b.y,
            a.z + b.z
        };
    }

    core::math::Vector3 Multiply(
        const core::math::Vector3& value,
        const float scalar) noexcept
    {
        return
        {
            value.x * scalar,
            value.y * scalar,
            value.z * scalar
        };
    }

    std::uint32_t PackNormal(
        const core::math::Vector3& value) noexcept
    {
        const core::math::Vector3 normal =
            Normalize(
                value);

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
                static_cast<std::uint32_t>(
                    x) &
                0x7FFu
            ) |
            (
                (
                    static_cast<std::uint32_t>(
                        y) &
                    0x7FFu
                )
                << 11u
            ) |
            (
                (
                    static_cast<std::uint32_t>(
                        z) &
                    0x3FFu
                )
                << 22u
            );
    }

    void AnalyzeAlpha(
        client::graphics::SceneTextureData& texture)
    {
        texture.hasTransparentPixels =
            false;

        texture.hasZeroAlphaPixels =
            false;

        texture.hasPartialAlphaPixels =
            false;

        if (texture.image.pixels.empty())
        {
            return;
        }

        for (std::size_t offset = 3;
             offset <
                texture.image.pixels.size();
             offset += 4)
        {
            const std::uint8_t alpha =
                std::to_integer<std::uint8_t>(
                    texture.image.pixels[
                        offset]);

            if (alpha ==
                255)
            {
                continue;
            }

            texture.hasTransparentPixels =
                true;

            if (alpha ==
                0)
            {
                texture.hasZeroAlphaPixels =
                    true;
            }
            else
            {
                texture.hasPartialAlphaPixels =
                    true;
            }

            if (texture.hasZeroAlphaPixels &&
                texture.hasPartialAlphaPixels)
            {
                return;
            }
        }
    }

    bool ConvertTriangleStrip(
        const std::vector<std::uint32_t>& source,
        const std::size_t vertexCount,
        std::vector<std::uint16_t>& output,
        std::string& error)
    {
        output.clear();

        if (source.size() <
            3)
        {
            return true;
        }

        if (vertexCount >
            static_cast<std::size_t>(
                std::numeric_limits<std::uint16_t>::max()))
        {
            error =
                "CTREE geometry requires 32-bit indices.";

            return false;
        }

        output.reserve(
            source.size() *
            3);

        for (std::size_t index = 2;
             index <
                source.size();
             ++index)
        {
            std::uint32_t i0 =
                source[
                    index -
                    2];

            std::uint32_t i1 =
                source[
                    index -
                    1];

            const std::uint32_t i2 =
                source[
                    index];

            if (i0 >=
                    vertexCount ||
                i1 >=
                    vertexCount ||
                i2 >=
                    vertexCount)
            {
                error =
                    "CTREE triangle strip references invalid vertex.";

                return false;
            }

            if (i0 ==
                    i1 ||
                i1 ==
                    i2 ||
                i0 ==
                    i2)
            {
                continue;
            }

            const std::size_t triangleIndex =
                index -
                2;

            if ((triangleIndex &
                 1u) !=
                0u)
            {
                std::swap(
                    i0,
                    i1);
            }

            output.push_back(
                static_cast<std::uint16_t>(
                    i0));

            output.push_back(
                static_cast<std::uint16_t>(
                    i1));

            output.push_back(
                static_cast<std::uint16_t>(
                    i2));
        }

        return true;
    }

    float ReadLeafFloat(
        const core::assets::speedtree::CTreeLeafVertex& vertex,
        const std::size_t index) noexcept
    {
        constexpr std::size_t FloatSize =
            sizeof(float);

        const std::size_t offset =
            index *
            FloatSize;

        if (offset >
                vertex.extra.size() ||
            FloatSize >
                vertex.extra.size() -
                    offset)
        {
            return 0.0f;
        }

        float value =
            0.0f;

        std::memcpy(
            &value,
            vertex.extra.data() +
                offset,
            sizeof(value));

        return value;
    }

    core::math::Vector3 ReadLeafVector(
        const core::assets::speedtree::CTreeLeafVertex& vertex,
        const std::size_t firstIndex) noexcept
    {
        return
        {
            ReadLeafFloat(
                vertex,
                firstIndex),

            ReadLeafFloat(
                vertex,
                firstIndex +
                    1),

            ReadLeafFloat(
                vertex,
                firstIndex +
                    2)
        };
    }

    std::uint32_t ReadLeafCorner(
        const core::assets::speedtree::CTreeLeafVertex& vertex,
        const std::size_t fallback) noexcept
    {
        const float value =
            ReadLeafFloat(
                vertex,
                7);

        if (value >=
                0.0f &&
            value <=
                3.0f)
        {
            return
                static_cast<std::uint32_t>(
                    value +
                    0.5f);
        }

        return
            static_cast<std::uint32_t>(
                fallback &
                3u);
    }

    bool CopyTriangleList(
        const std::vector<std::uint32_t>& source,
        const std::size_t vertexCount,
        std::vector<std::uint16_t>& output,
        std::string& error)
    {
        output.clear();

        if ((source.size() %
             3u) !=
            0u)
        {
            error =
                "CTREE leaf LOD is not a triangle list.";

            return false;
        }

        if (vertexCount >
            static_cast<std::size_t>(
                std::numeric_limits<std::uint16_t>::max()))
        {
            error =
                "CTREE leaves require 32-bit indices.";

            return false;
        }

        output.reserve(
            source.size());

        for (const std::uint32_t index :
             source)
        {
            if (index >=
                vertexCount)
            {
                error =
                    "CTREE leaf triangle references invalid vertex.";

                return false;
            }

            output.push_back(
                static_cast<std::uint16_t>(
                    index));
        }

        return true;
    }
}

namespace client::preview
{
    bool SpeedTreeRenderDataBuilder::ResolveTexture(
        const core::resources::ResourceFileSystem& resources,
        const std::string& logicalPath,
        graphics::SceneRenderData& scene,
        std::size_t& outputTextureIndex,
        std::string& error)
    {
        error.clear();

        if (logicalPath.empty())
        {
            error =
                "CTREE diffuse texture path is empty.";

            return false;
        }

        const auto cached =
            textureCache_.find(
                logicalPath);

        if (cached !=
            textureCache_.end())
        {
            outputTextureIndex =
                cached->second;

            return true;
        }

        if (!resources.Exists(
                logicalPath))
        {
            error =
                "CTREE diffuse texture was not found: " +
                logicalPath;

            return false;
        }

        std::vector<std::byte>
            encoded;

        if (!resources.ReadBinary(
                logicalPath,
                encoded))
        {
            error =
                "Unable to read CTREE diffuse texture: " +
                logicalPath;

            return false;
        }

        core::images::DdsDecoder
            decoder;

        core::images::RgbaImage
            image;

        if (!decoder.Decode(
                std::span<const std::byte>(
                    encoded.data(),
                    encoded.size()),
                image,
                error))
        {
            error =
                logicalPath +
                ": " +
                error;

            return false;
        }

        graphics::SceneTextureData
            texture;

        texture.logicalPath =
            logicalPath;

        texture.image =
            std::move(
                image);

        AnalyzeAlpha(
            texture);

        outputTextureIndex =
            scene.textures.size();

        scene.textures.push_back(
            std::move(
                texture));

        textureCache_.emplace(
            logicalPath,
            outputTextureIndex);

        return true;
    }

    bool SpeedTreeRenderDataBuilder::BuildIndexedGeometry(
        const core::resources::ResourceFileSystem& resources,
        const core::assets::speedtree::CTreeIndexedGeometry& source,
        const bool cutout,
        graphics::SceneRenderData& scene,
        std::size_t& outputMeshIndex,
        std::size_t& outputTriangleCount,
        std::string& error)
    {
        outputMeshIndex =
            0;

        outputTriangleCount =
            0;

        error.clear();

        if (source.vertices.empty())
        {
            return true;
        }

        if (source.lods.empty())
        {
            error =
                "CTREE geometry contains vertices but has no LOD.";

            return false;
        }

        const core::assets::speedtree::CTreeLod& lod =
            source.lods.front();

        if (lod.indices.empty())
        {
            return true;
        }

        core::assets::MeshData
            mesh;

        mesh.vertexFormat =
            "ctree-indexed";

        mesh.vertices.reserve(
            source.vertices.size());

        for (const core::assets::speedtree::CTreeIndexedVertex& sourceVertex :
             source.vertices)
        {
            core::assets::MeshVertex
                vertex;

            vertex.position =
                sourceVertex.position;

            vertex.packedNormal =
                PackNormal(
                    sourceVertex.normal);

            vertex.u =
                sourceVertex.u;

            vertex.v =
                sourceVertex.v;

            vertex.colour =
                0xFFFFFFFFu;

            mesh.vertices.push_back(
                vertex);
        }

        if (!ConvertTriangleStrip(
                lod.indices,
                mesh.vertices.size(),
                mesh.indices,
                error))
        {
            return false;
        }

        if (mesh.indices.empty())
        {
            return true;
        }

        core::assets::MeshPrimitiveGroup
            group;

        group.startIndex =
            0;

        group.primitiveCount =
            static_cast<std::uint32_t>(
                mesh.indices.size() /
                3);

        group.startVertex =
            0;

        group.vertexCount =
            static_cast<std::uint32_t>(
                mesh.vertices.size());

        mesh.primitiveGroups.push_back(
            group);

        graphics::SceneMesh
            sceneMesh;

        sceneMesh.geometry =
            std::move(
                mesh);

        sceneMesh.modelMaterials.resize(
            1);

        graphics::SceneModelMaterial&
            material =
                sceneMesh.modelMaterials[
                    0];

        std::size_t textureIndex =
            0;

        if (!ResolveTexture(
                resources,
                source.material.diffuseLogicalPath,
                scene,
                textureIndex,
                error))
        {
            return false;
        }

        material.diffuseTextureIndex =
            static_cast<std::int32_t>(
                textureIndex);

        material.alphaMode =
            cutout
                ? graphics::SceneAlphaMode::Cutout
                : graphics::SceneAlphaMode::Opaque;

        material.alphaCutoff =
            0.35f;

        outputTriangleCount =
            sceneMesh.geometry.TriangleCount();

        outputMeshIndex =
            scene.meshes.size();

        scene.meshes.push_back(
            std::move(
                sceneMesh));

        return true;
    }

    bool SpeedTreeRenderDataBuilder::BuildLeaves(
        const core::resources::ResourceFileSystem& resources,
        const core::assets::speedtree::CTreeLeafGeometry& source,
        graphics::SceneRenderData& scene,
        std::size_t& outputMeshIndex,
        std::size_t& outputTriangleCount,
        std::string& error)
    {
        outputMeshIndex =
            0;

        outputTriangleCount =
            0;

        error.clear();

        if (source.vertices.empty())
        {
            return true;
        }

        if ((source.vertices.size() %
             4u) !=
            0u)
        {
            error =
                "CTREE leaf vertex count is not divisible by four.";

            return false;
        }

        if (source.lods.empty())
        {
            error =
                "CTREE leaves contain vertices but no LOD.";

            return false;
        }

        const core::assets::speedtree::CTreeLod& lod =
            source.lods.front();

        if (lod.indices.empty())
        {
            return true;
        }

        core::assets::MeshData
            mesh;

        mesh.vertexFormat =
            "ctree-leaves";

        mesh.vertices.resize(
            source.vertices.size());

        for (std::size_t index = 0;
             index <
                source.vertices.size();
             ++index)
        {
            const core::assets::speedtree::CTreeLeafVertex&
                sourceVertex =
                    source.vertices[
                        index];

            const std::size_t cardStart =
                (
                    index /
                    4u
                ) *
                4u;

            const core::assets::speedtree::CTreeLeafVertex&
                cardVertex =
                    source.vertices[
                        cardStart];

            const core::math::Vector3 tangent =
                Normalize(
                    ReadLeafVector(
                        cardVertex,
                        13));

            const core::math::Vector3 bitangent =
                Normalize(
                    ReadLeafVector(
                        cardVertex,
                        16));

            float width =
                std::abs(
                    ReadLeafFloat(
                        sourceVertex,
                        9));

            float height =
                std::abs(
                    ReadLeafFloat(
                        sourceVertex,
                        10));

            if (width <
                0.001f)
            {
                width =
                    1.0f;
            }

            if (height <
                0.001f)
            {
                height =
                    width;
            }

            const std::uint32_t corner =
                ReadLeafCorner(
                    sourceVertex,
                    index -
                        cardStart);

            float localX =
                0.0f;

            float localY =
                0.0f;

            switch (corner)
            {
                case 0:
                    localX =
                        -0.5f;

                    localY =
                        0.5f;
                    break;

                case 1:
                    localX =
                        0.5f;

                    localY =
                        0.5f;
                    break;

                case 2:
                    localX =
                        0.5f;

                    localY =
                        -0.5f;
                    break;

                default:
                    localX =
                        -0.5f;

                    localY =
                        -0.5f;
                    break;
            }

            core::math::Vector3 position =
                sourceVertex.position;

            position =
                Add(
                    position,
                    Multiply(
                        tangent,
                        localX *
                            width));

            position =
                Add(
                    position,
                    Multiply(
                        bitangent,
                        localY *
                            height));

            core::assets::MeshVertex
                vertex;

            vertex.position =
                position;

            vertex.packedNormal =
                PackNormal(
                    sourceVertex.normal);

            vertex.u =
                ReadLeafFloat(
                    sourceVertex,
                    0);

            vertex.v =
                ReadLeafFloat(
                    sourceVertex,
                    1);

            vertex.colour =
                0xFFFFFFFFu;

            mesh.vertices[
                index] =
                vertex;
        }

        if (!CopyTriangleList(
                lod.indices,
                mesh.vertices.size(),
                mesh.indices,
                error))
        {
            return false;
        }

        if (mesh.indices.empty())
        {
            return true;
        }

        core::assets::MeshPrimitiveGroup
            group;

        group.startIndex =
            0;

        group.primitiveCount =
            static_cast<std::uint32_t>(
                mesh.indices.size() /
                3u);

        group.startVertex =
            0;

        group.vertexCount =
            static_cast<std::uint32_t>(
                mesh.vertices.size());

        mesh.primitiveGroups.push_back(
            group);

        graphics::SceneMesh
            sceneMesh;

        sceneMesh.geometry =
            std::move(
                mesh);

        sceneMesh.modelMaterials.resize(
            1);

        graphics::SceneModelMaterial&
            material =
                sceneMesh.modelMaterials[
                    0];

        std::size_t textureIndex =
            0;

        if (!ResolveTexture(
                resources,
                source.material.diffuseLogicalPath,
                scene,
                textureIndex,
                error))
        {
            return false;
        }

        material.diffuseTextureIndex =
            static_cast<std::int32_t>(
                textureIndex);

        material.alphaMode =
            graphics::SceneAlphaMode::Cutout;

        material.alphaCutoff =
            0.35f;

        outputTriangleCount =
            sceneMesh.geometry.TriangleCount();

        outputMeshIndex =
            scene.meshes.size();

        scene.meshes.push_back(
            std::move(
                sceneMesh));

        return true;
    }

    bool SpeedTreeRenderDataBuilder::Build(
        const core::resources::ResourceFileSystem& resources,
        const core::assets::speedtree::CTreeAsset& tree,
        graphics::SceneRenderData& scene,
        SpeedTreeRenderData& output,
        std::string& error)
    {
        output = {};
        error.clear();

        if (!tree.branches.vertices.empty() &&
            !tree.branches.lods.empty() &&
            !tree.branches.lods.front().indices.empty())
        {
            std::size_t meshIndex =
                0;

            std::size_t triangleCount =
                0;

            if (!BuildIndexedGeometry(
                    resources,
                    tree.branches,
                    false,
                    scene,
                    meshIndex,
                    triangleCount,
                    error))
            {
                error =
                    tree.sptLogicalPath +
                    " branches: " +
                    error;

                return false;
            }

            if (triangleCount >
                0)
            {
                output.meshIndices.push_back(
                    meshIndex);

                output.branchTriangles +=
                    triangleCount;
            }
        }

        if (!tree.fronds.vertices.empty() &&
            !tree.fronds.lods.empty() &&
            !tree.fronds.lods.front().indices.empty())
        {
            std::size_t meshIndex =
                0;

            std::size_t triangleCount =
                0;

            if (!BuildIndexedGeometry(
                    resources,
                    tree.fronds,
                    true,
                    scene,
                    meshIndex,
                    triangleCount,
                    error))
            {
                error =
                    tree.sptLogicalPath +
                    " fronds: " +
                    error;

                return false;
            }

            if (triangleCount >
                0)
            {
                output.meshIndices.push_back(
                    meshIndex);

                output.frondTriangles +=
                    triangleCount;
            }
        }

        if (!tree.leaves.vertices.empty() &&
            !tree.leaves.lods.empty() &&
            !tree.leaves.lods.front().indices.empty())
        {
            std::size_t meshIndex =
                0;

            std::size_t triangleCount =
                0;

            if (!BuildLeaves(
                    resources,
                    tree.leaves,
                    scene,
                    meshIndex,
                    triangleCount,
                    error))
            {
                error =
                    tree.sptLogicalPath +
                    " leaves: " +
                    error;

                return false;
            }

            if (triangleCount >
                0)
            {
                output.meshIndices.push_back(
                    meshIndex);

                output.leafTriangles +=
                    triangleCount;
            }
        }

        if (output.meshIndices.empty())
        {
            error =
                "CTREE contains no renderable LOD0 geometry: " +
                tree.sptLogicalPath;

            return false;
        }

        return true;
    }
}