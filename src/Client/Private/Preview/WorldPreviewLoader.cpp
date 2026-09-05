#include "Preview/WorldPreviewLoader.h"

#include "Preview/ModelRenderDataBuilder.h"
#include "Preview/TerrainRenderDataBuilder.h"
#include "Preview/SpeedTreeRenderDataBuilder.h"

#include "Core/Assets/MeshLoader.h"
#include "Core/Assets/ModelBundleLoader.h"
#include "Core/Assets/SpeedTree/CTreeLoader.h"

#include "Core/Log.h"
#include "Core/Resources/ResourcePath.h"
#include "Core/World/TerrainLoader.h"
#include "Core/World/WorldLoader.h"

#include <cstddef>
#include <cstdint>
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

        core::world::WorldLoader
            worldLoader;

        core::world::WorldScene
            world;

        if (!worldLoader.Load(
                runtime.Resources(),
                "so_origins",
                world,
                error))
        {
            return false;
        }

        graphics::SceneRenderData
            scene;

        core::assets::speedtree::CTreeLoader
            ctreeLoader;

        SpeedTreeRenderDataBuilder
            speedTreeRenderBuilder;

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

        ModelRenderDataBuilder
            modelRenderBuilder;

        std::size_t skippedInstances = 0;
        std::size_t loadedUniqueModels = 0;
        std::size_t texturedModelGroups = 0;

        std::unordered_map<
            std::string,
            core::assets::speedtree::CTreeAsset>
            speedTreeCache;

        std::unordered_set<std::string>
            failedSpeedTrees;

        std::unordered_set<std::string>
            missingSpeedTreeTextures;

        std::size_t loadedSpeedTrees =
            0;

        std::size_t totalBranchVertices =
            0;

        std::size_t totalFrondVertices =
            0;

        std::size_t totalLeafVertices =
            0;

        std::size_t totalBillboardVertices =
            0;

        const auto validateMaterial =
            [&runtime,
             &missingSpeedTreeTextures](
                const core::assets::speedtree::CTreeMaterial& material)
            {
                if (!material.diffuseLogicalPath.empty() &&
                    !runtime.Resources().Exists(
                        material.diffuseLogicalPath))
                {
                    missingSpeedTreeTextures.insert(
                        material.diffuseLogicalPath);
                }

                if (!material.normalLogicalPath.empty() &&
                    !runtime.Resources().Exists(
                        material.normalLogicalPath))
                {
                    missingSpeedTreeTextures.insert(
                        material.normalLogicalPath);
                }
            };

        for (const core::world::WorldSpeedTreeInstance& treeInstance :
             world.speedTreeInstances)
        {
            if (speedTreeCache.contains(
                    treeInstance.sptLogicalPath) ||
                failedSpeedTrees.contains(
                    treeInstance.sptLogicalPath))
            {
                continue;
            }

            core::assets::speedtree::CTreeAsset
                tree;

            std::string treeError;

            if (!ctreeLoader.Load(
                    runtime.Resources(),
                    treeInstance.sptLogicalPath,
                    tree,
                    treeError))
            {
                failedSpeedTrees.insert(
                    treeInstance.sptLogicalPath);

                core::Log::Warning(
                    std::string(
                        "CTREE load failed: ") +
                    treeInstance.sptLogicalPath +
                    ": " +
                    treeError);

                continue;
            }

            std::size_t billboardVertexCount =
                0;

            for (const core::assets::speedtree::CTreeBillboardGroup& group :
                 tree.billboard.groups)
            {
                billboardVertexCount +=
                    group.vertices.size();
            }

            core::Log::Info(
                std::string(
                    "CTREE loaded: ") +
                treeInstance.sptLogicalPath +
                ", branches=" +
                std::to_string(
                    tree.branches.vertices.size()) +
                ", branchLODs=" +
                std::to_string(
                    tree.branches.lods.size()) +
                ", fronds=" +
                std::to_string(
                    tree.fronds.vertices.size()) +
                ", frondLODs=" +
                std::to_string(
                    tree.fronds.lods.size()) +
                ", leaves=" +
                std::to_string(
                    tree.leaves.vertices.size()) +
                ", leafLODs=" +
                std::to_string(
                    tree.leaves.lods.size()) +
                ", billboard=" +
                std::to_string(
                    billboardVertexCount));

            validateMaterial(
                tree.branches.material);

            validateMaterial(
                tree.fronds.material);

            validateMaterial(
                tree.leaves.material);

            validateMaterial(
                tree.billboard.material);

            totalBranchVertices +=
                tree.branches.vertices.size();

            totalFrondVertices +=
                tree.fronds.vertices.size();

            totalLeafVertices +=
                tree.leaves.vertices.size();

            totalBillboardVertices +=
                billboardVertexCount;

            speedTreeCache.emplace(
                treeInstance.sptLogicalPath,
                std::move(
                    tree));

            ++loadedSpeedTrees;
        }

        core::Log::Info(
            std::string(
                "CTREE resources loaded: ") +
            std::to_string(
                loadedSpeedTrees));

        core::Log::Info(
            std::string(
                "CTREE resources failed: ") +
            std::to_string(
                failedSpeedTrees.size()));

        core::Log::Info(
            std::string(
                "CTREE branch vertices: ") +
            std::to_string(
                totalBranchVertices));

        core::Log::Info(
            std::string(
                "CTREE frond vertices: ") +
            std::to_string(
                totalFrondVertices));

        core::Log::Info(
            std::string(
                "CTREE leaf records: ") +
            std::to_string(
                totalLeafVertices));

        core::Log::Info(
            std::string(
                "CTREE billboard vertices: ") +
            std::to_string(
                totalBillboardVertices));

        core::Log::Info(
            std::string(
                "Missing CTREE textures: ") +
            std::to_string(
                missingSpeedTreeTextures.size()));

        for (const std::string& texture :
             missingSpeedTreeTextures)
        {
            core::Log::Warning(
                std::string(
                    "Missing CTREE texture: ") +
                texture);
        }

        std::unordered_map<
            std::string,
            SpeedTreeRenderData>
            speedTreeRenderCache;

        std::unordered_set<std::string>
            failedSpeedTreeRenderResources;

        std::size_t speedTreeRenderMeshes =
            0;

        std::size_t speedTreeRenderInstances =
            0;

        std::size_t speedTreeBranchTriangles =
            0;

        std::size_t speedTreeFrondTriangles =
            0;

        for (const auto& entry :
             speedTreeCache)
        {
            const std::string& resourcePath =
                entry.first;

            const core::assets::speedtree::CTreeAsset& tree =
                entry.second;

            SpeedTreeRenderData
                renderData;

            std::string renderError;

            if (!speedTreeRenderBuilder.Build(
                    runtime.Resources(),
                    tree,
                    scene,
                    renderData,
                    renderError))
            {
                failedSpeedTreeRenderResources.insert(
                    resourcePath);

                core::Log::Warning(
                    std::string(
                        "SpeedTree render build failed: ") +
                    renderError);

                continue;
            }

            speedTreeRenderMeshes +=
                renderData.meshIndices.size();

            speedTreeBranchTriangles +=
                renderData.branchTriangles;

            speedTreeFrondTriangles +=
                renderData.frondTriangles;

            speedTreeRenderCache.emplace(
                resourcePath,
                std::move(
                    renderData));
        }

        for (const core::world::WorldSpeedTreeInstance& treeInstance :
             world.speedTreeInstances)
        {
            const auto cached =
                speedTreeRenderCache.find(
                    treeInstance.sptLogicalPath);

            if (cached ==
                speedTreeRenderCache.end())
            {
                continue;
            }

            for (const std::size_t meshIndex :
                 cached->second.meshIndices)
            {
                graphics::SceneInstance
                    instance;

                instance.meshIndex =
                    meshIndex;

                instance.transform =
                    treeInstance.transform;

                scene.instances.push_back(
                    std::move(
                        instance));

                ++speedTreeRenderInstances;
            }
        }

        core::Log::Info(
            std::string(
                "SpeedTree render resources: ") +
            std::to_string(
                speedTreeRenderCache.size()));

        core::Log::Info(
            std::string(
                "SpeedTree render resources failed: ") +
            std::to_string(
                failedSpeedTreeRenderResources.size()));

        core::Log::Info(
            std::string(
                "SpeedTree render meshes: ") +
            std::to_string(
                speedTreeRenderMeshes));

        core::Log::Info(
            std::string(
                "SpeedTree render instances: ") +
            std::to_string(
                speedTreeRenderInstances));

        core::Log::Info(
            std::string(
                "SpeedTree branch triangles LOD0: ") +
            std::to_string(
                speedTreeBranchTriangles));

        core::Log::Info(
            std::string(
                "SpeedTree frond triangles LOD0: ") +
            std::to_string(
                speedTreeFrondTriangles));

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

                core::assets::ModelBundle
                    bundle;

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
                        std::string(
                            "Skipping model ") +
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
                        core::assets::MeshData
                            mesh;

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

                        graphics::SceneMesh
                            sceneMesh;

                        sceneMesh.geometry =
                            std::move(mesh);

                        std::size_t texturedGroups =
                            0;

                        std::string materialError;

                        if (!modelRenderBuilder.Build(
                                runtime.Resources(),
                                geometry,
                                scene,
                                sceneMesh,
                                texturedGroups,
                                materialError))
                        {
                            core::Log::Warning(
                                std::string(
                                    "Model material fallback in ") +
                                normalizedModel +
                                ": " +
                                materialError);
                        }

                        texturedModelGroups +=
                            texturedGroups;

                        const std::size_t meshIndex =
                            scene.meshes.size();

                        scene.meshes.push_back(
                            std::move(sceneMesh));

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
                graphics::SceneInstance
                    instance;

                instance.meshIndex =
                    meshIndex;

                instance.transform =
                    worldInstance.transform;

                scene.instances.push_back(
                    std::move(instance));
            }
        }

        core::world::TerrainLoader
            terrainLoader;

        TerrainRenderDataBuilder
            terrainRenderBuilder;

        std::size_t loadedTerrains = 0;
        std::size_t failedTerrains = 0;
        std::size_t texturedTerrains = 0;

        for (const core::world::WorldTerrainInstance& terrainInstance :
             world.terrainInstances)
        {
            core::world::TerrainAsset
                terrain;

            std::string terrainError;

            if (!terrainLoader.Load(
                    runtime.Resources(),
                    terrainInstance.cdataLogicalPath,
                    terrain,
                    terrainError))
            {
                ++failedTerrains;

                core::Log::Warning(
                    std::string(
                        "Skipping terrain ") +
                    terrainInstance.chunkId +
                    ": " +
                    terrainError);

                continue;
            }

            std::int32_t materialIndex =
                -1;

            std::string materialError;

            if (!terrainRenderBuilder.Build(
                    runtime.Resources(),
                    terrain,
                    scene,
                    materialIndex,
                    materialError))
            {
                core::Log::Warning(
                    std::string(
                        "Terrain material fallback for ") +
                    terrainInstance.chunkId +
                    ": " +
                    materialError);

                materialIndex =
                    -1;
            }
            else if (materialIndex >= 0)
            {
                ++texturedTerrains;
            }

            const std::size_t vertexCount =
                terrain.mesh.vertices.size();

            const std::size_t triangleCount =
                terrain.mesh.TriangleCount();

            const std::size_t layerCount =
                terrain.layers.size();

            graphics::SceneMesh
                sceneMesh;

            sceneMesh.geometry =
                std::move(
                    terrain.mesh);

            sceneMesh.terrainMaterialIndex =
                materialIndex;

            const std::size_t meshIndex =
                scene.meshes.size();

            scene.meshes.push_back(
                std::move(sceneMesh));

            graphics::SceneInstance
                instance;

            instance.meshIndex =
                meshIndex;

            instance.transform =
                terrainInstance.transform;

            scene.instances.push_back(
                std::move(instance));

            ++loadedTerrains;

            core::Log::Info(
                std::string(
                    "Terrain loaded: ") +
                terrainInstance.chunkId +
                ", vertices=" +
                std::to_string(
                    vertexCount) +
                ", triangles=" +
                std::to_string(
                    triangleCount) +
                ", layers=" +
                std::to_string(
                    layerCount) +
                ", min=" +
                std::to_string(
                    terrain.heightData.minHeight) +
                ", max=" +
                std::to_string(
                    terrain.heightData.maxHeight));
        }

        if (scene.meshes.empty() ||
            scene.instances.empty())
        {
            error =
                "World contains no renderable geometry.";

            return false;
        }

        core::Log::Info(
            std::string(
                "Unique renderable models: ") +
            std::to_string(
                loadedUniqueModels));

        core::Log::Info(
            std::string(
                "Terrain meshes loaded: ") +
            std::to_string(
                loadedTerrains));

        core::Log::Info(
            std::string(
                "Terrain meshes failed: ") +
            std::to_string(
                failedTerrains));

        core::Log::Info(
            std::string(
                "Textured terrain meshes: ") +
            std::to_string(
                texturedTerrains));

        core::Log::Info(
            std::string(
                "Unique terrain textures: ") +
            std::to_string(
                scene.textures.size()));

        core::Log::Info(
            std::string(
                "Terrain materials: ") +
            std::to_string(
                scene.terrainMaterials.size()));

        core::Log::Info(
            std::string(
                "Total GPU meshes: ") +
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

        core::Log::Info(
            std::string(
                "Textured model primitive groups: ") +
            std::to_string(
                texturedModelGroups));

        output =
            std::move(scene);

        return true;
    }
}