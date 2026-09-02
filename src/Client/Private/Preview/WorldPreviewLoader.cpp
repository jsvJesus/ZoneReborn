#include "Preview/WorldPreviewLoader.h"

#include "Core/Assets/MeshLoader.h"
#include "Core/Assets/ModelBundleLoader.h"
#include "Core/Log.h"
#include "Core/Resources/ResourcePath.h"
#include "Core/World/WorldLoader.h"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace client::preview
{
    bool LoadWorldPreview(
        core::Runtime& runtime,
        graphics::SceneRenderData& output,
        std::string& error)
    {
        output = {};
        error.clear();

        core::world::WorldLoader worldLoader;

        core::world::WorldScene world;

        if (!worldLoader.Load(
                runtime.Resources(),
                "so_origins",
                world,
                error))
        {
            return false;
        }

        graphics::SceneRenderData scene;

        std::unordered_map<
            std::string,
            std::vector<std::size_t>>
            modelCache;

        std::unordered_set<std::string>
            failedModels;

        core::assets::ModelBundleLoader
            bundleLoader;

        core::assets::MeshLoader
            meshLoader;

        std::size_t skippedInstances = 0;
        std::size_t loadedUniqueModels = 0;

        for (const core::world::WorldModelInstance& worldInstance :
             world.modelInstances)
        {
            const std::string normalizedModel =
                core::resources::ResourcePath::Normalize(
                    worldInstance.modelReference);

            if (normalizedModel.empty())
            {
                ++skippedInstances;
                continue;
            }

            auto cached =
                modelCache.find(
                    normalizedModel);

            if (cached ==
                modelCache.end())
            {
                if (failedModels.contains(
                        normalizedModel))
                {
                    ++skippedInstances;
                    continue;
                }

                core::assets::ModelBundle bundle;

                std::string modelError;

                if (!bundleLoader.Load(
                        runtime.Resources(),
                        worldInstance.modelReference,
                        bundle,
                        modelError))
                {
                    failedModels.insert(
                        normalizedModel);

                    core::Log::Warning(
                        std::string("Skipping model ") +
                        normalizedModel +
                        ": " +
                        modelError);

                    ++skippedInstances;

                    continue;
                }

                std::vector<std::size_t>
                    meshIndices;

                bool geometryFailed =
                    false;

                for (const core::assets::VisualRenderSet& renderSet :
                     bundle.visual.renderSets)
                {
                    for (const core::assets::VisualGeometry& geometry :
                         renderSet.geometries)
                    {
                        core::assets::MeshData mesh;

                        if (!meshLoader.Load(
                                bundle.primitives,
                                geometry,
                                mesh,
                                modelError))
                        {
                            geometryFailed =
                                true;

                            core::Log::Warning(
                                std::string(
                                    "Skipping geometry in ") +
                                normalizedModel +
                                ": " +
                                modelError);

                            continue;
                        }

                        const std::size_t meshIndex =
                            scene.meshes.size();

                        scene.meshes.push_back(
                            std::move(mesh));

                        meshIndices.push_back(
                            meshIndex);
                    }
                }

                if (meshIndices.empty())
                {
                    failedModels.insert(
                        normalizedModel);

                    if (!geometryFailed)
                    {
                        core::Log::Warning(
                            std::string(
                                "Model contains no supported geometry: ") +
                            normalizedModel);
                    }

                    ++skippedInstances;

                    continue;
                }

                cached =
                    modelCache.emplace(
                        normalizedModel,
                        std::move(meshIndices)).first;

                ++loadedUniqueModels;
            }

            for (const std::size_t meshIndex :
                 cached->second)
            {
                graphics::SceneInstance instance;

                instance.meshIndex =
                    meshIndex;

                instance.transform =
                    worldInstance.transform;

                scene.instances.push_back(
                    std::move(instance));
            }
        }

        if (scene.meshes.empty() ||
            scene.instances.empty())
        {
            error =
                "World contains no renderable model geometry.";

            return false;
        }

        core::Log::Info(
            std::string(
                "Unique renderable models: ") +
            std::to_string(
                loadedUniqueModels));

        core::Log::Info(
            std::string(
                "Unique GPU meshes: ") +
            std::to_string(
                scene.meshes.size()));

        core::Log::Info(
            std::string(
                "Scene render instances: ") +
            std::to_string(
                scene.instances.size()));

        core::Log::Info(
            std::string(
                "Skipped model instances: ") +
            std::to_string(
                skippedInstances));

        core::Log::Info(
            std::string(
                "Unsupported/missing unique models: ") +
            std::to_string(
                failedModels.size()));

        output =
            std::move(scene);

        return true;
    }
}