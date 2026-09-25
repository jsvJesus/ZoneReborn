#include "Character/CharacterRenderDataBuilder.h"

#include "Preview/ModelRenderDataBuilder.h"

#include "Core/Assets/MeshLoader.h"
#include "Core/Assets/ModelBundleLoader.h"
#include "Core/Assets/ModelSourceLoader.h"
#include "Core/Log.h"
#include "Core/Math/Vector3.h"
#include "Core/Resources/DataSection.h"
#include "Core/Resources/PackedSectionReader.h"
#include "Core/Resources/ResourcePath.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cctype>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace
{
    using Transform =
        core::math::Transform3x4;

    using Vector3 =
        core::math::Vector3;

    Vector3 Add(
        const Vector3 a,
        const Vector3 b) noexcept
    {
        return
        {
            a.x + b.x,
            a.y + b.y,
            a.z + b.z
        };
    }

    std::string Lower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](const unsigned char c)
        {
            return static_cast<char>(std::tolower(c));
        });
        return value;
    }

    std::array<float, 4> FaceTint(
        const std::uint32_t colour,
        const float brightness = 1.0f)
    {
        return
        {
            (static_cast<float>((colour >> 16u) & 0xFFu) / 255.0f) * brightness,
            (static_cast<float>((colour >> 8u) & 0xFFu) / 255.0f) * brightness,
            (static_cast<float>(colour & 0xFFu) / 255.0f) * brightness,
            0.78f
        };
    }

    float SmoothStep(
        const float minimum,
        const float maximum,
        const float value) noexcept
    {
        const float normalized =
            std::clamp(
                (value - minimum) /
                    (maximum - minimum),
                0.0f,
                1.0f);

        return
            normalized *
            normalized *
            (3.0f -
             2.0f * normalized);
    }

    float SampleAlpha(
        const core::images::RgbaImage& image,
        const float u,
        const float v) noexcept
    {
        if (image.width == 0 ||
            image.height == 0 ||
            image.pixels.size() <
                static_cast<std::size_t>(
                    image.width) *
                static_cast<std::size_t>(
                    image.height) *
                4u)
        {
            return 0.0f;
        }

        const std::uint32_t x =
            static_cast<std::uint32_t>(
                std::clamp(
                    u,
                    0.0f,
                    1.0f) *
                static_cast<float>(
                    image.width - 1u));

        const std::uint32_t y =
            static_cast<std::uint32_t>(
                std::clamp(
                    v,
                    0.0f,
                    1.0f) *
                static_cast<float>(
                    image.height - 1u));

        const std::size_t offset =
            (
                static_cast<std::size_t>(y) *
                    image.width +
                x
            ) *
            4u +
            3u;

        return
            static_cast<float>(
                std::to_integer<std::uint8_t>(
                    image.pixels[offset])) /
            255.0f;
    }

    struct EyebrowAnchors final
    {
        float side = 0.0f;
        float height = 0.0f;
        float depth = 0.0f;
    };

    float EyebrowAlpha(
        const Vector3 position,
        const core::images::RgbaImage& eyebrow,
        const EyebrowAnchors& anchors) noexcept
    {
        const float side =
            std::abs(
                position.x);

        const float horizontal =
            std::clamp(
                (side -
                 (anchors.side - 0.040f)) /
                    0.080f,
                0.0f,
                1.0f);

        const float vertical =
            std::clamp(
                0.5f -
                    (position.y - anchors.height) /
                        0.035f,
                0.0f,
                1.0f);

        const float textureAlpha =
            SampleAlpha(
                eyebrow,
                horizontal,
                vertical);

        const float mask =
            (1.0f -
             SmoothStep(
                 0.040f,
                 0.046f,
                 std::abs(
                     side - anchors.side))) *
            (1.0f -
             SmoothStep(
                 0.018f,
                 0.024f,
                 std::abs(
                     position.y - anchors.height))) *
            SmoothStep(
                anchors.depth - 0.004f,
                anchors.depth + 0.012f,
                position.z);

        return
            textureAlpha *
            mask;
    }

    bool BuildEyebrowAtlas(
        const core::assets::MeshData& mesh,
        const core::assets::MeshPrimitiveGroup& group,
        const core::images::RgbaImage& diffuse,
        const core::images::RgbaImage& eyebrow,
        const EyebrowAnchors& anchors,
        client::graphics::SceneTextureData& output)
    {
        if (diffuse.width == 0 ||
            diffuse.height == 0 ||
            eyebrow.width == 0 ||
            eyebrow.height == 0)
        {
            return false;
        }

        const std::size_t pixelCount =
            static_cast<std::size_t>(
                diffuse.width) *
            static_cast<std::size_t>(
                diffuse.height);

        output = {};
        output.image.width =
            diffuse.width;
        output.image.height =
            diffuse.height;
        output.image.pixels.assign(
            pixelCount * 4u,
            std::byte{0});
        output.hasTransparentPixels =
            true;
        output.hasZeroAlphaPixels =
            true;
        output.hasPartialAlphaPixels =
            true;
        output.generateMipmaps =
            false;

        const std::size_t firstIndex =
            group.startIndex;

        const std::size_t finalIndex =
            std::min(
                mesh.indices.size(),
                firstIndex +
                    static_cast<std::size_t>(
                        group.primitiveCount) *
                    3u);

        bool hasEyebrowPixels =
            false;

        for (std::size_t index = firstIndex;
             index + 2u < finalIndex;
             index += 3u)
        {
            const std::uint16_t index0 =
                mesh.indices[index];
            const std::uint16_t index1 =
                mesh.indices[index + 1u];
            const std::uint16_t index2 =
                mesh.indices[index + 2u];

            if (index0 >= mesh.vertices.size() ||
                index1 >= mesh.vertices.size() ||
                index2 >= mesh.vertices.size())
            {
                continue;
            }

            const core::assets::MeshVertex& vertex0 =
                mesh.vertices[index0];
            const core::assets::MeshVertex& vertex1 =
                mesh.vertices[index1];
            const core::assets::MeshVertex& vertex2 =
                mesh.vertices[index2];

            const float minimumSide =
                std::min(
                    {
                        std::abs(vertex0.position.x),
                        std::abs(vertex1.position.x),
                        std::abs(vertex2.position.x)
                    });
            const float maximumSide =
                std::max(
                    {
                        std::abs(vertex0.position.x),
                        std::abs(vertex1.position.x),
                        std::abs(vertex2.position.x)
                    });
            const float minimumHeight =
                std::min(
                    {
                        vertex0.position.y,
                        vertex1.position.y,
                        vertex2.position.y
                    });
            const float maximumHeight =
                std::max(
                    {
                        vertex0.position.y,
                        vertex1.position.y,
                        vertex2.position.y
                    });
            const float maximumDepth =
                std::max(
                    {
                        vertex0.position.z,
                        vertex1.position.z,
                        vertex2.position.z
                    });

            if (maximumSide < anchors.side - 0.050f ||
                minimumSide > anchors.side + 0.050f ||
                maximumHeight < anchors.height - 0.030f ||
                minimumHeight > anchors.height + 0.030f ||
                maximumDepth < anchors.depth - 0.004f)
            {
                continue;
            }

            const float x0 =
                vertex0.u *
                static_cast<float>(
                    diffuse.width - 1u);
            const float y0 =
                vertex0.v *
                static_cast<float>(
                    diffuse.height - 1u);
            const float x1 =
                vertex1.u *
                static_cast<float>(
                    diffuse.width - 1u);
            const float y1 =
                vertex1.v *
                static_cast<float>(
                    diffuse.height - 1u);
            const float x2 =
                vertex2.u *
                static_cast<float>(
                    diffuse.width - 1u);
            const float y2 =
                vertex2.v *
                static_cast<float>(
                    diffuse.height - 1u);

            const float denominator =
                (y1 - y2) *
                    (x0 - x2) +
                (x2 - x1) *
                    (y0 - y2);

            if (std::abs(
                    denominator) <
                0.00001f)
            {
                continue;
            }

            const int minimumX =
                std::max(
                    0,
                    static_cast<int>(
                        std::floor(
                            std::min(
                                {x0, x1, x2}))));
            const int maximumX =
                std::min(
                    static_cast<int>(
                        diffuse.width) - 1,
                    static_cast<int>(
                        std::ceil(
                            std::max(
                                {x0, x1, x2}))));
            const int minimumY =
                std::max(
                    0,
                    static_cast<int>(
                        std::floor(
                            std::min(
                                {y0, y1, y2}))));
            const int maximumY =
                std::min(
                    static_cast<int>(
                        diffuse.height) - 1,
                    static_cast<int>(
                        std::ceil(
                            std::max(
                                {y0, y1, y2}))));

            for (int y = minimumY;
                 y <= maximumY;
                 ++y)
            {
                for (int x = minimumX;
                     x <= maximumX;
                     ++x)
                {
                    const float pixelX =
                        static_cast<float>(x) +
                        0.5f;
                    const float pixelY =
                        static_cast<float>(y) +
                        0.5f;

                    const float weight0 =
                        (
                            (y1 - y2) *
                                (pixelX - x2) +
                            (x2 - x1) *
                                (pixelY - y2)
                        ) /
                        denominator;
                    const float weight1 =
                        (
                            (y2 - y0) *
                                (pixelX - x2) +
                            (x0 - x2) *
                                (pixelY - y2)
                        ) /
                        denominator;
                    const float weight2 =
                        1.0f -
                        weight0 -
                        weight1;

                    if (weight0 < -0.0001f ||
                        weight1 < -0.0001f ||
                        weight2 < -0.0001f)
                    {
                        continue;
                    }

                    const Vector3 position
                    {
                        vertex0.position.x * weight0 +
                            vertex1.position.x * weight1 +
                            vertex2.position.x * weight2,
                        vertex0.position.y * weight0 +
                            vertex1.position.y * weight1 +
                            vertex2.position.y * weight2,
                        vertex0.position.z * weight0 +
                            vertex1.position.z * weight1 +
                            vertex2.position.z * weight2
                    };

                    const std::uint8_t alpha =
                        static_cast<std::uint8_t>(
                            std::clamp(
                                EyebrowAlpha(
                                    position,
                                    eyebrow,
                                    anchors),
                                0.0f,
                                1.0f) *
                            255.0f +
                            0.5f);

                    const std::size_t pixel =
                        (
                            static_cast<std::size_t>(y) *
                                diffuse.width +
                            static_cast<std::size_t>(x)
                        ) *
                        4u;

                    if (alpha <=
                        std::to_integer<std::uint8_t>(
                            output.image.pixels[
                                pixel + 3u]))
                    {
                        continue;
                    }

                    output.image.pixels[pixel] =
                        std::byte{255};
                    output.image.pixels[pixel + 1u] =
                        std::byte{255};
                    output.image.pixels[pixel + 2u] =
                        std::byte{255};
                    output.image.pixels[pixel + 3u] =
                        static_cast<std::byte>(
                            alpha);

                    hasEyebrowPixels =
                        hasEyebrowPixels ||
                        alpha != 0u;
                }
            }
        }

        return
            hasEyebrowPixels;
    }

    Vector3 Multiply(
        const Vector3 value,
        const float scalar) noexcept
    {
        return
        {
            value.x * scalar,
            value.y * scalar,
            value.z * scalar
        };
    }

    Vector3 Normalize(
        const Vector3 value) noexcept
    {
        const float lengthSquared =
            value.x * value.x +
            value.y * value.y +
            value.z * value.z;

        if (lengthSquared <=
            0.0000001f)
        {
            return
            {
                0.0f,
                1.0f,
                0.0f
            };
        }

        const float inverse =
            1.0f /
            std::sqrt(
                lengthSquared);

        return
        {
            value.x * inverse,
            value.y * inverse,
            value.z * inverse
        };
    }

    Vector3 TransformPoint(
        const Vector3 value,
        const Transform& transform) noexcept
    {
        return
        {
            value.x * transform.values[0] +
            value.y * transform.values[3] +
            value.z * transform.values[6] +
            transform.values[9],

            value.x * transform.values[1] +
            value.y * transform.values[4] +
            value.z * transform.values[7] +
            transform.values[10],

            value.x * transform.values[2] +
            value.y * transform.values[5] +
            value.z * transform.values[8] +
            transform.values[11]
        };
    }

    Vector3 TransformVector(
        const Vector3 value,
        const Transform& transform) noexcept
    {
        return
        {
            value.x * transform.values[0] +
            value.y * transform.values[3] +
            value.z * transform.values[6],

            value.x * transform.values[1] +
            value.y * transform.values[4] +
            value.z * transform.values[7],

            value.x * transform.values[2] +
            value.y * transform.values[5] +
            value.z * transform.values[8]
        };
    }

    Vector3 UnpackNormal(
        const std::uint32_t packed) noexcept
    {
        std::int32_t x =
            static_cast<std::int32_t>(
                packed &
                0x7FFu);

        std::int32_t y =
            static_cast<std::int32_t>(
                (
                    packed >>
                    11u
                ) &
                0x7FFu);

        std::int32_t z =
            static_cast<std::int32_t>(
                (
                    packed >>
                    22u
                ) &
                0x3FFu);

        if ((x & 0x400) != 0)
        {
            x -=
                0x800;
        }

        if ((y & 0x400) != 0)
        {
            y -=
                0x800;
        }

        if ((z & 0x200) != 0)
        {
            z -=
                0x400;
        }

        return Normalize(
        {
            static_cast<float>(x) /
                1023.0f,

            static_cast<float>(y) /
                1023.0f,

            static_cast<float>(z) /
                511.0f
        });
    }

    std::uint32_t PackNormal(
        const Vector3 value) noexcept
    {
        const Vector3 normal =
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
                ) <<
                11u
            ) |
            (
                (
                    static_cast<std::uint32_t>(
                        z) &
                    0x3FFu
                ) <<
                22u
            );
    }

    bool BuildNodeTransforms(
        const core::assets::VisualAsset& visual,
        std::vector<Transform>& output,
        std::string& error)
    {
        output.clear();

        output.resize(
            visual.nodes.size());

        for (std::size_t index = 0;
             index < visual.nodes.size();
             ++index)
        {
            const auto& node =
                visual.nodes[index];

            if (node.parentIndex < 0)
            {
                output[index] =
                    node.transform;

                continue;
            }

            const std::size_t parentIndex =
                static_cast<std::size_t>(
                    node.parentIndex);

            if (parentIndex >= index ||
                parentIndex >=
                    output.size())
            {
                error =
                    "Character model contains invalid node hierarchy.";

                return false;
            }

            output[index] =
                Transform::Multiply(
                    node.transform,
                    output[parentIndex]);
        }

        return true;
    }

    bool BuildBonePalette(
        const core::assets::VisualAsset& visual,
        const core::assets::VisualRenderSet& renderSet,
        const std::vector<Transform>& nodeTransforms,
        std::vector<Transform>& palette,
        std::string& error)
    {
        palette.clear();

        if (renderSet.nodes.empty())
        {
            return true;
        }

        std::unordered_map<
            std::string,
            std::size_t>
            lookup;

        lookup.reserve(
            visual.nodes.size());

        for (std::size_t index = 0;
             index < visual.nodes.size();
             ++index)
        {
            lookup.emplace(
                visual.nodes[index].
                    identifier,
                index);
        }

        palette.reserve(
            renderSet.nodes.size());

        for (const std::string& name :
             renderSet.nodes)
        {
            const auto found =
                lookup.find(
                    name);

            if (found ==
                lookup.end())
            {
                error =
                    "Character renderSet node not found: " +
                    name;

                return false;
            }

            palette.push_back(
                nodeTransforms[
                    found->second]);
        }

        return true;
    }

    bool BakeSkinning(
        core::assets::MeshData& mesh,
        const std::vector<Transform>& palette,
        std::string& error)
    {
        if (!mesh.skinned)
        {
            return true;
        }

        if (palette.empty())
        {
            error =
                "Character mesh has empty bone palette.";

            return false;
        }

        for (core::assets::MeshVertex& vertex :
             mesh.vertices)
        {
            Vector3 finalPosition{};
            Vector3 finalNormal{};

            const Vector3 sourcePosition =
                vertex.position;

            const Vector3 sourceNormal =
                UnpackNormal(
                    vertex.packedNormal);

            for (std::size_t influence = 0;
                 influence < 3;
                 ++influence)
            {
                const std::size_t boneIndex =
                    vertex.boneIndices[
                        influence];

                if (boneIndex >=
                    palette.size())
                {
                    error =
                        "Character vertex contains invalid bone index.";

                    return false;
                }

                const float weight =
                    vertex.boneWeights[
                        influence];

                if (std::abs(weight) <=
                    0.000001f)
                {
                    continue;
                }

                finalPosition =
                    Add(
                        finalPosition,
                        Multiply(
                            TransformPoint(
                                sourcePosition,
                                palette[
                                    boneIndex]),
                            weight));

                finalNormal =
                    Add(
                        finalNormal,
                        Multiply(
                            TransformVector(
                                sourceNormal,
                                palette[
                                    boneIndex]),
                            weight));
            }

            vertex.position =
                finalPosition;

            vertex.packedNormal =
                PackNormal(
                    finalNormal);
        }

        mesh.skinned =
            false;

        return true;
    }

    bool AppendModel(
        const core::resources::ResourceFileSystem& resources,
        const std::string_view modelReference,
        const Transform& transform,
        const client::character::FaceState& face,
        client::preview::ModelRenderDataBuilder& materialBuilder,
        client::graphics::SceneRenderData& scene,
        std::size_t& instanceCount,
        client::character::Animator& animator,
        std::string& error)
    {
        const std::string modelKey = Lower(std::string(modelReference));

        core::assets::ModelBundleLoader
            bundleLoader;

        core::assets::ModelBundle
            bundle;

        if (!bundleLoader.Load(
                resources,
                modelReference,
                bundle,
                error))
        {
            error =
                "Unable to load character model " +
                std::string(
                    modelReference) +
                ": " +
                error;

            return false;
        }

        std::vector<Transform>
            nodeTransforms;

        if (!BuildNodeTransforms(
                bundle.visual,
                nodeTransforms,
                error))
        {
            return false;
        }

        EyebrowAnchors eyebrowAnchors;
        Vector3 leftEyebrow{};
        Vector3 rightEyebrow{};
        bool hasLeftEyebrow = false;
        bool hasRightEyebrow = false;

        for (std::size_t index = 0;
             index < bundle.visual.nodes.size();
             ++index)
        {
            const std::string& identifier =
                bundle.visual.nodes[index].identifier;

            if (identifier == "Brow_Left")
            {
                leftEyebrow =
                    nodeTransforms[index].Translation();
                hasLeftEyebrow = true;
            }
            else if (identifier == "Brow_Right")
            {
                rightEyebrow =
                    nodeTransforms[index].Translation();
                hasRightEyebrow = true;
            }
        }

        const bool hasEyebrowAnchors =
            hasLeftEyebrow &&
            hasRightEyebrow;

        if (hasEyebrowAnchors)
        {
            eyebrowAnchors.side =
                (
                    std::abs(leftEyebrow.x) +
                    std::abs(rightEyebrow.x)
                ) *
                0.5f;
            eyebrowAnchors.height =
                (leftEyebrow.y + rightEyebrow.y) *
                0.5f;
            eyebrowAnchors.depth =
                (leftEyebrow.z + rightEyebrow.z) *
                0.5f;
        }

        core::assets::MeshLoader
            meshLoader;

        std::vector<std::size_t>
            modelMeshes;

        std::int32_t eyebrowTextureIndex = -1;

        if (modelKey.ends_with("/manhead.model") &&
            face.eyebrowStyle >= 1 && face.eyebrowStyle <= 4)
        {
            std::size_t textureIndex = 0;
            std::string textureError;
            const std::string path =
                "res/characters2/clothing/ManNude/Brow/Brow_" +
                std::to_string(face.eyebrowStyle) + ".dds";
            if (materialBuilder.LoadTexture(
                    resources, path, scene, textureIndex, textureError))
            {
                eyebrowTextureIndex = static_cast<std::int32_t>(textureIndex);
            }
            else
            {
                core::Log::Warning("Unable to load eyebrow texture " + path + ": " + textureError);
            }
        }

        for (const core::assets::VisualRenderSet& renderSet :
             bundle.visual.renderSets)
        {
            std::vector<Transform>
                palette;

            if (!BuildBonePalette(
                    bundle.visual,
                    renderSet,
                    nodeTransforms,
                    palette,
                    error))
            {
                return false;
            }

            for (const core::assets::VisualGeometry& geometry :
                 renderSet.geometries)
            {
                core::assets::MeshData
                    mesh;

                if (!meshLoader.Load(
                        bundle.primitives,
                        geometry,
                        mesh,
                        error))
                {
                    error =
                        "Unable to load character geometry " +
                        std::string(
                            modelReference) +
                        ": " +
                        error;

                    return false;
                }

                core::assets::MeshData
                    animationSource = mesh;

                if (mesh.skinned &&
                    !BakeSkinning(
                        mesh,
                        palette,
                        error))
                {
                    return false;
                }

                client::graphics::SceneMesh
                    sceneMesh;

                sceneMesh.geometry =
                    std::move(
                        mesh);

                std::size_t texturedGroups =
                    0;

                std::string materialError;

                if (!materialBuilder.Build(
                        resources,
                        geometry,
                        scene,
                        sceneMesh,
                        texturedGroups,
                        materialError))
                {
                    core::Log::Warning(
                        std::string(
                            "Character material fallback [") +
                        std::string(
                            modelReference) +
                        "]: " +
                        materialError);
                }

                const bool hairPart = modelKey.find("/hair/") != std::string::npos;
                const bool moustachePart = modelKey.find("/mustache/") != std::string::npos ||
                    modelKey.find("/moustache/") != std::string::npos;
                const bool beardPart = modelKey.find("/beard/") != std::string::npos;
                const bool facialHairPart = hairPart || moustachePart || beardPart;
                const bool exposedBody =
                    modelKey.ends_with("/manhead.model") ||
                    modelKey.ends_with("/mantorso.model") ||
                    modelKey.ends_with("/manhands.model") ||
                    modelKey.ends_with("/manlegs.model") ||
                    modelKey.ends_with("/manfoot.model");
                for (const core::assets::VisualPrimitiveGroup& visualGroup : geometry.primitiveGroups)
                {
                    if (visualGroup.index < 0 ||
                        static_cast<std::size_t>(visualGroup.index) >= sceneMesh.modelMaterials.size())
                    {
                        continue;
                    }
                    const std::size_t groupIndex =
                        static_cast<std::size_t>(
                            visualGroup.index);
                    std::string key = Lower(visualGroup.material.identifier) + " " + Lower(visualGroup.material.effect);
                    for (const core::assets::VisualMaterialProperty& property : visualGroup.material.properties)
                    {
                        key += " " + Lower(property.name);
                        if (property.texture.has_value()) key += " " + Lower(property.texture->logicalPath);
                    }

                    auto& material = sceneMesh.modelMaterials[groupIndex];
                    if (facialHairPart)
                    {
                        material.tintColour = FaceTint(face.hairColor);
                        material.alphaMode = client::graphics::SceneAlphaMode::Cutout;
                        material.alphaCutoff = 0.2f;
                    }
                    else if (exposedBody &&
                             key.find("eye") == std::string::npos &&
                             key.find("teeth") == std::string::npos &&
                             key.find("mouth") == std::string::npos)
                    {
                        material.tintColour = FaceTint(face.skinColor);
                        if (modelKey.ends_with("/manhead.model") &&
                            eyebrowTextureIndex >= 0 &&
                            hasEyebrowAnchors &&
                            material.diffuseTextureIndex >= 0 &&
                            groupIndex < sceneMesh.geometry.primitiveGroups.size())
                        {
                            const std::size_t diffuseIndex =
                                static_cast<std::size_t>(
                                    material.diffuseTextureIndex);
                            const std::size_t eyebrowIndex =
                                static_cast<std::size_t>(
                                    eyebrowTextureIndex);

                            if (diffuseIndex < scene.textures.size() &&
                                eyebrowIndex < scene.textures.size())
                            {
                                client::graphics::SceneTextureData
                                    eyebrowAtlas;

                                if (BuildEyebrowAtlas(
                                        sceneMesh.geometry,
                                        sceneMesh.geometry.primitiveGroups[
                                            groupIndex],
                                        scene.textures[diffuseIndex].image,
                                        scene.textures[eyebrowIndex].image,
                                        eyebrowAnchors,
                                        eyebrowAtlas))
                                {
                                    eyebrowAtlas.logicalPath =
                                        "generated/character/eyebrow_" +
                                        std::to_string(
                                            face.eyebrowStyle) +
                                        "_" +
                                        std::to_string(
                                            groupIndex);

                                    material.overlayTextureIndex =
                                        static_cast<std::int32_t>(
                                            scene.textures.size());
                                    material.overlayColour =
                                        FaceTint(
                                            face.hairColor);
                                    material.overlayColour[3] =
                                        1.0f;

                                    scene.textures.push_back(
                                        std::move(
                                            eyebrowAtlas));
                                }
                            }
                        }
                    }
                }

                const std::size_t meshIndex =
                    scene.meshes.size();

                scene.meshes.push_back(
                    std::move(
                        sceneMesh));

                if (!animator.AddMesh(
                meshIndex,
                bundle.visual,
                renderSet.nodes,
                std::move(
                    animationSource),
                error))
                {
                    return false;
                }

                modelMeshes.push_back(
                    meshIndex);
            }
        }

        if (modelMeshes.empty())
        {
            error =
                "Character model contains no renderable meshes: " +
                std::string(
                    modelReference);

            return false;
        }

        for (const std::size_t meshIndex :
             modelMeshes)
        {
            client::graphics::SceneInstance
                instance;

            instance.meshIndex =
                meshIndex;

            instance.transform =
                transform;

            scene.instances.push_back(
                std::move(
                    instance));

            ++instanceCount;
        }

        core::Log::Info(
            std::string(
                "Character model loaded: ") +
            std::string(
                modelReference));

        return true;
    }
}

namespace client::character
{
    bool RenderDataBuilder::Build(
        const core::resources::ResourceFileSystem& resources,
        const Catalog& catalog,
        const State& state,
        const core::math::Transform3x4& transform,
        graphics::SceneRenderData& scene,
        std::size_t& outputInstanceCount,
        Animator& animator,
        std::string& error)
    {
        error.clear();

        outputInstanceCount =
            0;

        animator.Reset();

        if (!animator.LoadIdle(
            resources,
            error))
        {
            error =
                "Unable to initialize character idle animation: " +
                error;

            return false;
        }

        if (!animator.SetFaceForm(state.Face().faceForm, error))
        {
            error = "Unable to apply character face form: " + error;
            return false;
        }

        ModelComposer composer;
        ModelPlan plan;

        if (!composer.Compose(
                catalog,
                state,
                plan,
                error))
        {
            return false;
        }

        preview::ModelRenderDataBuilder
            materialBuilder;

        for (const std::string& model :
             plan.visibleModels)
        {
            if (!AppendModel(
            resources,
            model,
            transform,
            state.Face(),
            materialBuilder,
            scene,
            outputInstanceCount,
            animator,
            error))
            {
                return false;
            }
        }

        core::Log::Info(
            std::string(
                "Character composition complete: models=") +
            std::to_string(
                plan.visibleModels.size()) +
            ", instances=" +
            std::to_string(
                outputInstanceCount));

        return true;
    }
}
