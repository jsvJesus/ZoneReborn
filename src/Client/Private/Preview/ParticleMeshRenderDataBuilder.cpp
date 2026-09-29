#include "Preview/ParticleMeshRenderDataBuilder.h"

#include "Core/Assets/MeshLoader.h"
#include "Core/Assets/PrimitivesLoader.h"
#include "Core/Assets/VisualLoader.h"
#include "Core/Resources/ResourcePath.h"

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace client::preview
{
    bool ParticleMeshRenderDataBuilder::Build(
        const core::resources::ResourceFileSystem& resources,
        const core::world::particles::ParticleRendererDefinition& renderer,
        graphics::SceneRenderData& scene,
        std::vector<std::size_t>& outputMeshIndices,
        std::string& error)
    {
        outputMeshIndices.clear();
        error.clear();

        std::string visualPath =
            core::resources::ResourcePath::ToResPath(
                renderer.visualName);

        if (visualPath.empty())
        {
            error =
                "Particle renderer contains an invalid visual path.";

            return false;
        }

        std::filesystem::path visualFile(
            visualPath);

        if (!visualFile.has_extension())
        {
            visualFile.replace_extension(
                ".visual");

            visualPath =
                core::resources::ResourcePath::Normalize(
                    visualFile.generic_string());
        }

        if (visualFile.has_extension() &&
            visualFile.extension() !=
                ".visual")
        {
            error =
                "Particle renderer visual does not use the .visual extension: " +
                visualPath;

            return false;
        }

        const core::resources::ResourceEntry* visualEntry =
            resources.Find(
                visualPath);

        if (visualEntry == nullptr)
        {
            error =
                "Particle visual does not exist: " +
                visualPath;

            return false;
        }

        const std::string visualKey =
            core::resources::ResourcePath::Normalize(
                visualEntry->logicalPath);

        const auto cached =
            visualCache_.find(
                visualKey);

        if (cached !=
            visualCache_.end())
        {
            outputMeshIndices =
                cached->second;

            return true;
        }

        core::assets::VisualAsset
            visual;

        core::assets::VisualLoader
            visualLoader;

        if (!visualLoader.Load(
                resources,
                visualEntry->logicalPath,
                visual,
                error))
        {
            return false;
        }

        std::filesystem::path primitivesFile(
            visualEntry->logicalPath);

        primitivesFile.replace_extension(
            ".primitives");

        const std::string primitivesPath =
            core::resources::ResourcePath::Normalize(
                primitivesFile.generic_string());

        const core::resources::ResourceEntry* primitivesEntry =
            resources.Find(
                primitivesPath);

        if (primitivesEntry == nullptr)
        {
            error =
                "Particle primitives do not exist: " +
                primitivesPath;

            return false;
        }

        core::assets::PrimitivesContainer
            primitives;

        core::assets::PrimitivesLoader
            primitivesLoader;

        if (!primitivesLoader.Load(
                resources,
                primitivesEntry->logicalPath,
                primitives,
                error))
        {
            return false;
        }

        core::assets::MeshLoader
            meshLoader;

        std::vector<graphics::SceneMesh>
            preparedMeshes;

        for (const core::assets::VisualRenderSet& renderSet :
             visual.renderSets)
        {
            for (const core::assets::VisualGeometry& geometry :
                 renderSet.geometries)
            {
                graphics::SceneMesh
                    sceneMesh;

                if (!meshLoader.Load(
                        primitives,
                        geometry,
                        sceneMesh.geometry,
                        error))
                {
                    error =
                        visualEntry->logicalPath +
                        ": " +
                        error;

                    return false;
                }

                std::size_t texturedGroups =
                    0;

                if (!materialBuilder_.Build(
                        resources,
                        geometry,
                        scene,
                        sceneMesh,
                        texturedGroups,
                        error))
                {
                    error =
                        visualEntry->logicalPath +
                        ": " +
                        error;

                    return false;
                }

                preparedMeshes.push_back(
                    std::move(
                        sceneMesh));
            }
        }

        if (preparedMeshes.empty())
        {
            error =
                "Particle visual contains no renderable geometry: " +
                visualEntry->logicalPath;

            return false;
        }

        outputMeshIndices.reserve(
            preparedMeshes.size());

        for (graphics::SceneMesh& sceneMesh :
             preparedMeshes)
        {
            outputMeshIndices.push_back(
                scene.meshes.size());

            scene.meshes.push_back(
                std::move(
                    sceneMesh));
        }

        visualCache_.emplace(
            visualKey,
            outputMeshIndices);

        return true;
    }
}
