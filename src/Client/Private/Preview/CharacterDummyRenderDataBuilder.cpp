#include "Preview/CharacterDummyRenderDataBuilder.h"

#include "Preview/ModelRenderDataBuilder.h"

#include "Core/Assets/MeshLoader.h"
#include "Core/Assets/ModelBundleLoader.h"
#include "Core/Log.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
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
            value.x *
                transform.values[0] +
            value.y *
                transform.values[3] +
            value.z *
                transform.values[6] +
            transform.values[9],

            value.x *
                transform.values[1] +
            value.y *
                transform.values[4] +
            value.z *
                transform.values[7] +
            transform.values[10],

            value.x *
                transform.values[2] +
            value.y *
                transform.values[5] +
            value.z *
                transform.values[8] +
            transform.values[11]
        };
    }
    
    Vector3 TransformVector(
        const Vector3 value,
        const Transform& transform) noexcept
    {
        return
        {
            value.x *
                transform.values[0] +
            value.y *
                transform.values[3] +
            value.z *
                transform.values[6],

            value.x *
                transform.values[1] +
            value.y *
                transform.values[4] +
            value.z *
                transform.values[7],

            value.x *
                transform.values[2] +
            value.y *
                transform.values[5] +
            value.z *
                transform.values[8]
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
                (packed >> 11u) &
                0x7FFu);

        std::int32_t z =
            static_cast<std::int32_t>(
                (packed >> 22u) &
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

        return Normalize({
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
                static_cast<std::uint32_t>(x) &
                0x7FFu
            ) |
            (
                (
                    static_cast<std::uint32_t>(y) &
                    0x7FFu
                )
                << 11u
            ) |
            (
                (
                    static_cast<std::uint32_t>(z) &
                    0x3FFu
                )
                << 22u
            );
    }

    std::string_view ResolveHead(const std::int32_t id)
    {
        switch (id)
        {
            case 1:
                return "characters/avatars/heads/head_m5_01.model";

            case 2:
                return "characters/avatars/heads/head_m5_02.model";

            case 3:
                return "characters/avatars/heads/head_m5_03.model";

            case 4:
                return "characters/avatars/heads/head_m5_04.model";

            case 5:
                return "characters/avatars/heads/head_m5_05.model";

            case 6:
                return "characters/avatars/heads/head_m5_06_bandit.model";

            case 7:
                return "characters/avatars/heads/head_m5_07.model";

            case 8:
                return "characters/avatars/heads/head_m5_08.model";

            case 9:
                return "characters/avatars/heads/head_m5_09.model";

            case 10:
                return "characters/avatars/heads/head_m5_10.model";

            case 11:
                return "characters/avatars/heads/head_m5_11.model";

            case 12:
                return "characters/avatars/heads/head_m5_12.model";

            case 13:
                return "characters/avatars/heads/head_m5_13.model";

            case 14:
                return "characters/avatars/heads/head_m5_14.model";

            case 15:
                return "characters/avatars/heads/head_m5_15.model";

            case 16:
                return "characters/avatars/heads/head_m5_16.model";

            case 17:
                return "characters/avatars/heads/head_m5_17.model";

            case 18:
                return "characters/avatars/heads/head_m5_18.model";

            case 19:
                return "characters/avatars/heads/head_m5_19.model";

            case 20:
                return "characters/avatars/heads/head_m5_20.model";

            case 21:
                return "characters/avatars/heads/head_m5_21.model";

            case 22:
                return "characters/avatars/heads/head_m5_22_bandit.model";

            case 23:
                return "characters/avatars/heads/head_m5_26.model";

            case 24:
                return "characters/avatars/heads/head_m5_27.model";

            case 25:
                return "characters/avatars/heads/head_m5_28.model";

            case 26:
                return "characters/avatars/heads/head_m5_29.model";

            case 27:
                return "characters/avatars/heads/head_m5_30.model";

            default:
                return {};
        }
    }

    std::string_view ResolveHeadAttachment(const std::int32_t id)
    {
        switch (id)
        {
            case 4:
                return
                    "characters/avatars/heads/custom/"
                    "head_m5_04_beard.model";

            case 6:
                return
                    "characters/avatars/heads/custom/"
                    "head_m5_06_bandit_beard.model";

            case 8:
                return
                    "characters/avatars/heads/custom/"
                    "head_m5_08_beard.model";

            case 9:
                return
                    "characters/avatars/heads/custom/"
                    "head_m5_09_hair.model";

            case 12:
                return
                    "characters/avatars/heads/custom/"
                    "head_m5_12_hair.model";

            case 13:
                return
                    "characters/avatars/heads/custom/"
                    "head_m5_13_hair.model";

            case 14:
                return
                    "characters/avatars/heads/custom/"
                    "head_m5_14_hair.model";

            case 15:
                return
                    "characters/avatars/heads/custom/"
                    "head_m5_15_hair.model";

            case 17:
                return
                    "characters/avatars/heads/custom/"
                    "head_m5_17_hair.model";

            case 19:
                return
                    "characters/avatars/heads/custom/"
                    "head_m5_19_hair.model";

            case 20:
                return
                    "characters/avatars/heads/custom/"
                    "head_m5_20_hair.model";

            case 21:
                return
                    "characters/avatars/heads/custom/"
                    "head_m5_21_hair.model";

            case 22:
                return
                    "characters/avatars/heads/custom/"
                    "head_m5_22_hair.model";

            case 23:
                return
                    "characters/avatars/heads/custom/"
                    "head_m5_26_hair.model";

            case 24:
                return
                    "characters/avatars/heads/custom/"
                    "head_m5_27_dreadlocks.model";

            case 25:
                return
                    "characters/avatars/heads/custom/"
                    "head_m5_28_hair.model";

            default:
                return {};
        }
    }

    std::string_view ResolveBody(const std::int32_t id)
    {
        switch (id)
        {
            case 108:
                return
                    "characters/avatars/body/"
                    "body_shtormovka_woh.model";

            case 109:
                return
                    "characters/avatars/body/"
                    "body_shtormovka.model";

            case 120:
                return
                    "characters/avatars/body/"
                    "body_haki_woh.model";

            case 121:
                return
                    "characters/avatars/body/"
                    "body_haki.model";

            case 122:
                return
                    "characters/avatars/body/"
                    "body_kurtka.model";

            case 200005:
            case 200006:
            case 200007:
            case 200008:
            case 200009:
                return
                    "characters/avatars/body/"
                    "body_smock.model";

            case 200010:
            case 200011:
            case 200012:
            case 200013:
            case 200014:
                return
                    "characters/avatars/body/"
                    "body_sweater.model";

            default:
                return {};
        }
    }
    
    std::string_view ResolvePalms(const std::int32_t id)
    {
        switch (id)
        {
            case 110247:
            case 110248:
            case 110249:
            case 110250:
                return
                    "characters/avatars/hands/"
                    "hand_koja.model";

            default:
                return {};
        }
    }
    
    std::string_view ResolveLegs(const std::int32_t id)
    {
        switch (id)
        {
            case 104:
                return
                    "characters/avatars/legs/"
                    "legs_longi.model";

            case 110256:
                return
                    "characters/avatars/legs/"
                    "legs_pants_1.model";

            case 110257:
                return
                    "characters/avatars/legs/"
                    "legs_pants_2.model";

            case 110258:
                return
                    "characters/avatars/legs/"
                    "legs_pants_3.model";

            case 110259:
                return
                    "characters/avatars/legs/"
                    "legs_pants_4.model";

            case 200200:
            case 200201:
                return
                    "characters/avatars/legs/"
                    "legs_bigpants.model";

            case 200205:
            case 200206:
                return
                    "characters/avatars/legs/"
                    "legs_jeans.model";

            case 200210:
            case 200211:
                return
                    "characters/avatars/legs/"
                    "legs_pants.model";

            default:
                return {};
        }
    }
    
    std::string_view ResolveFeet(const std::int32_t id)
    {
        switch (id)
        {
            case 110251:
            case 110252:
            case 110253:
            case 110254:
                return
                    "characters/avatars/boots/"
                    "boots_bertsi.model";

            case 110255:
                return
                    "characters/avatars/boots/"
                    "boots_botinok.model";

            default:
                return {};
        }
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
             index <
                visual.nodes.size();
             ++index)
        {
            const auto& node =
                visual.nodes[
                    index];

            if (node.parentIndex <
                0)
            {
                output[index] =
                    node.transform;

                continue;
            }

            const std::size_t
                parentIndex =
                    static_cast<std::size_t>(
                        node.parentIndex);

            if (parentIndex >=
                    index ||
                parentIndex >=
                    output.size())
            {
                error =
                    "CharacterDummy contains invalid node hierarchy.";

                return false;
            }

            output[index] =
                Transform::Multiply(
                    node.transform,
                    output[
                        parentIndex]);
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
             index <
                visual.nodes.size();
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
                    "CharacterDummy renderSet node was not found: " +
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
                "Skinned CharacterDummy mesh contains empty bone palette.";

            return false;
        }

        for (core::assets::MeshVertex& vertex :
             mesh.vertices)
        {
            for (std::size_t influence = 0;
                 influence < 3;
                 ++influence)
            {
                if (vertex.boneIndices[
                        influence] >=
                    palette.size())
                {
                    error =
                        "CharacterDummy vertex references invalid bone index.";

                    return false;
                }
            }

            const Vector3 sourcePosition =
                vertex.position;

            const Vector3 sourceNormal =
                UnpackNormal(
                    vertex.packedNormal);

            Vector3 finalPosition{};
            Vector3 finalNormal{};

            for (std::size_t influence = 0;
                 influence < 3;
                 ++influence)
            {
                const float weight =
                    vertex.boneWeights[
                        influence];

                if (std::abs(
                        weight) <=
                    0.000001f)
                {
                    continue;
                }

                const Transform& bone =
                    palette[
                        vertex.boneIndices[
                            influence]];

                finalPosition =
                    Add(
                        finalPosition,
                        Multiply(
                            TransformPoint(
                                sourcePosition,
                                bone),
                            weight));

                finalNormal =
                    Add(
                        finalNormal,
                        Multiply(
                            TransformVector(
                                sourceNormal,
                                bone),
                            weight));
            }

            vertex.position =
                finalPosition;

            vertex.packedNormal =
                PackNormal(
                    finalNormal);
        }

        //
        // Теперь это обычный static mesh,
        // уже запечённый в default pose.
        //
        mesh.skinned =
            false;

        return true;
    }
    
    bool AppendModel(
        const core::resources::ResourceFileSystem& resources,
        const std::string_view modelReference,
        const Transform& transform,
        client::preview::ModelRenderDataBuilder& renderDataBuilder,
        client::graphics::SceneRenderData& scene,
        std::size_t& totalMeshes,
        std::size_t& totalInstances,
        std::size_t& totalTexturedGroups,
        std::string& error)
    {
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
                "CharacterDummy: unable to load " +
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

        core::assets::MeshLoader
            meshLoader;

        std::vector<std::size_t>
            meshIndices;

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
                        "CharacterDummy mesh load failed [" +
                        std::string(
                            modelReference) +
                        "]: " +
                        error;

                    return false;
                }

                if (mesh.skinned)
                {
                    if (!renderSet.
                            treatAsWorldSpaceObject)
                    {
                        core::Log::Warning(
                            std::string(
                                "CharacterDummy skinned vertex format without "
                                "treatAsWorldSpaceObject: ") +
                            std::string(
                                modelReference));
                    }

                    if (!BakeSkinning(
                            mesh,
                            palette,
                            error))
                    {
                        error =
                            "CharacterDummy skinning failed [" +
                            std::string(
                                modelReference) +
                            "]: " +
                            error;

                        return false;
                    }
                }

                if (!mesh.vertices.empty())
                {
                    float minX =
                        mesh.vertices.front().
                            position.x;

                    float minY =
                        mesh.vertices.front().
                            position.y;

                    float minZ =
                        mesh.vertices.front().
                            position.z;

                    float maxX =
                        minX;

                    float maxY =
                        minY;

                    float maxZ =
                        minZ;

                    for (const auto& vertex :
                         mesh.vertices)
                    {
                        minX =
                            std::min(
                                minX,
                                vertex.position.x);

                        minY =
                            std::min(
                                minY,
                                vertex.position.y);

                        minZ =
                            std::min(
                                minZ,
                                vertex.position.z);

                        maxX =
                            std::max(
                                maxX,
                                vertex.position.x);

                        maxY =
                            std::max(
                                maxY,
                                vertex.position.y);

                        maxZ =
                            std::max(
                                maxZ,
                                vertex.position.z);
                    }

                    core::Log::Info(
                        std::string(
                            "CharacterDummy mesh bounds [") +
                        std::string(
                            modelReference) +
                        "]: min=(" +
                        std::to_string(
                            minX) +
                        ", " +
                        std::to_string(
                            minY) +
                        ", " +
                        std::to_string(
                            minZ) +
                        "), max=(" +
                        std::to_string(
                            maxX) +
                        ", " +
                        std::to_string(
                            maxY) +
                        ", " +
                        std::to_string(
                            maxZ) +
                        "), size=(" +
                        std::to_string(
                            maxX - minX) +
                        ", " +
                        std::to_string(
                            maxY - minY) +
                        ", " +
                        std::to_string(
                            maxZ - minZ) +
                        ")");
                }

                client::graphics::SceneMesh
                    sceneMesh;

                sceneMesh.geometry =
                    std::move(
                        mesh);

                std::size_t
                    texturedGroups =
                        0;

                std::string
                    materialError;

                if (!renderDataBuilder.Build(
                        resources,
                        geometry,
                        scene,
                        sceneMesh,
                        texturedGroups,
                        materialError))
                {
                    core::Log::Warning(
                        std::string(
                            "CharacterDummy material fallback [") +
                        std::string(
                            modelReference) +
                        "]: " +
                        materialError);
                }

                totalTexturedGroups +=
                    texturedGroups;

                const std::size_t
                    meshIndex =
                        scene.meshes.size();

                scene.meshes.push_back(
                    std::move(
                        sceneMesh));

                meshIndices.push_back(
                    meshIndex);

                ++totalMeshes;
            }
        }

        if (meshIndices.empty())
        {
            error =
                "CharacterDummy model contains no renderable geometry: " +
                std::string(
                    modelReference);

            return false;
        }

        for (const std::size_t meshIndex :
             meshIndices)
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

            ++totalInstances;
        }

        core::Log::Info(
            std::string(
                "CharacterDummy part loaded: ") +
            std::string(
                modelReference) +
            ", meshes=" +
            std::to_string(
                meshIndices.size()));

        return true;
    }
}

namespace client::preview
{
    bool CharacterDummyAppearance::SetPart(
        const std::string_view choiceGroup,
        const std::int32_t value) noexcept
    {
        if (choiceGroup ==
            "01_head")
        {
            head =
                value;

            return true;
        }

        if (choiceGroup ==
            "02_body")
        {
            body =
                value;

            return true;
        }

        if (choiceGroup ==
            "03_palms")
        {
            palms =
                value;

            return true;
        }

        if (choiceGroup ==
            "04_legs")
        {
            legs =
                value;

            return true;
        }

        if (choiceGroup ==
            "05_feet")
        {
            feet =
                value;

            return true;
        }

        return false;
    }
    
    bool CharacterDummyRenderDataBuilder::BuildDefault(
        const core::resources::ResourceFileSystem& resources,
        const core::math::Transform3x4& transform,
        graphics::SceneRenderData& scene,
        std::string& error)
    {
        CharacterDummyAppearance
            appearance;

        return Build(
            resources,
            appearance,
            transform,
            scene,
            error);
    }

    bool CharacterDummyRenderDataBuilder::Build(
        const core::resources::ResourceFileSystem& resources,
        const CharacterDummyAppearance& appearance,
        const core::math::Transform3x4& transform,
        graphics::SceneRenderData& scene,
        std::string& error)
    {
        error.clear();

        std::vector<std::string_view>
    models;

        models.reserve(
            6);

        models.push_back(
            ResolveHead(
                appearance.head));

        const std::string_view headAttachment =
            ResolveHeadAttachment(
                appearance.head);

        if (!headAttachment.empty())
        {
            models.push_back(
                headAttachment);
        }

        models.push_back(
            ResolveBody(
                appearance.body));

        models.push_back(
            ResolvePalms(
                appearance.palms));

        models.push_back(
            ResolveLegs(
                appearance.legs));

        models.push_back(
            ResolveFeet(
                appearance.feet));

        for (const std::string_view model :
             models)
        {
            if (model.empty())
            {
                error =
                    "CharacterDummy contains unsupported appearance ID.";

                return false;
            }
        }

        ModelRenderDataBuilder
            renderDataBuilder;

        std::size_t totalMeshes =
            0;

        std::size_t totalInstances =
            0;

        std::size_t totalTexturedGroups =
            0;

        for (const std::string_view model :
             models)
        {
            if (!AppendModel(
                    resources,
                    model,
                    transform,
                    renderDataBuilder,
                    scene,
                    totalMeshes,
                    totalInstances,
                    totalTexturedGroups,
                    error))
            {
                return false;
            }
        }

        const auto position =
            transform.Translation();

        core::Log::Info(
            std::string(
                "CharacterDummy position: ") +
            std::to_string(
                position.x) +
            ", " +
            std::to_string(
                position.y) +
            ", " +
            std::to_string(
                position.z));

        core::Log::Info(
            std::string(
                "CharacterDummy appearance: head=") +
            std::to_string(
                appearance.head) +
            ", body=" +
            std::to_string(
                appearance.body) +
            ", palms=" +
            std::to_string(
                appearance.palms) +
            ", legs=" +
            std::to_string(
                appearance.legs) +
            ", feet=" +
            std::to_string(
                appearance.feet));

        core::Log::Info(
            std::string(
                "CharacterDummy meshes: ") +
            std::to_string(
                totalMeshes));

        core::Log::Info(
            std::string(
                "CharacterDummy instances: ") +
            std::to_string(
                totalInstances));

        core::Log::Info(
            std::string(
                "CharacterDummy textured groups: ") +
            std::to_string(
                totalTexturedGroups));

        return true;
    }
}
