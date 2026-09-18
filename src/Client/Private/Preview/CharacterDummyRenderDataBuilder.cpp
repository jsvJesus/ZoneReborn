#include "Preview/CharacterDummyRenderDataBuilder.h"

#include "Preview/ModelRenderDataBuilder.h"

#include "Core/Assets/MeshLoader.h"
#include "Core/Assets/ModelBundleLoader.h"
#include "Core/Log.h"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    constexpr std::array<
        std::string_view,
        5>
        DefaultDummyModels
    {{
        "characters/avatars/heads/head_m1.model",
        "characters/avatars/body/body_pilot.model",
        "characters/avatars/hands/hand_koja.model",
        "characters/avatars/legs/legs_pants_1.model",
        "characters/avatars/boots/boots_bertsi.model"
    }};
}

namespace client::preview
{
    bool CharacterDummyRenderDataBuilder::BuildDefault(
        const core::resources::ResourceFileSystem& resources,
        const core::math::Transform3x4& transform,
        graphics::SceneRenderData& scene,
        std::string& error)
    {
        error.clear();

        core::assets::ModelBundleLoader
            bundleLoader;

        core::assets::MeshLoader
            meshLoader;

        ModelRenderDataBuilder
            renderDataBuilder;

        std::size_t totalMeshes =
            0;

        std::size_t totalInstances =
            0;

        std::size_t totalTexturedGroups =
            0;

        for (const std::string_view modelReference :
             DefaultDummyModels)
        {
            core::assets::ModelBundle
                bundle;

            std::string
                modelError;

            if (!bundleLoader.Load(
                    resources,
                    modelReference,
                    bundle,
                    modelError))
            {
                error =
                    "CharacterDummy: unable to load model " +
                    std::string(
                        modelReference) +
                    ": " +
                    modelError;

                return false;
            }

            std::vector<std::size_t>
                meshIndices;

            for (const core::assets::VisualRenderSet& renderSet :
                 bundle.visual.renderSets)
            {
                for (const core::assets::VisualGeometry& geometry :
                     renderSet.geometries)
                {
                    core::assets::MeshData
                        mesh;

                    if (!meshLoader.Load(
                            bundle.primitives,
                            geometry,
                            mesh,
                            modelError))
                    {
                        error =
                            "CharacterDummy: unable to load geometry from " +
                            std::string(
                                modelReference) +
                            ": " +
                            modelError;

                        return false;
                    }

                    graphics::SceneMesh
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
                        //
                        // Geometry is still useful even if one material
                        // cannot be reproduced yet.
                        //
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
                graphics::SceneInstance
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
        }

        if (totalInstances ==
            0)
        {
            error =
                "CharacterDummy contains no render instances.";

            return false;
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