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
#include "Core/World/Flora/FloraConfigLoader.h"
#include "Core/World/WorldLoader.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <limits>

namespace client::preview
{
    bool LoadWorldPreview(
        core::Runtime& runtime,
        const std::string_view spaceName,
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
                spaceName,
                world,
                error))
        {
            return false;
        }

        core::world::flora::FloraConfigLoader
            floraConfigLoader;

        core::world::flora::FloraConfig
            floraConfig;

        if (!floraConfigLoader.Load(
                runtime.Resources(),
                floraConfig,
                error))
        {
            error =
                "Unable to load SO flora system: " +
                error;

            return false;
        }

        std::size_t floraTextureRuleCount =
            0;

        std::size_t floraGeneratorCount =
            0;

        std::size_t floraVisualRuleCount =
            0;

        for (const core::world::flora::FloraEcotype& ecotype :
             floraConfig.ecotypes)
        {
            floraTextureRuleCount +=
                ecotype.textures.size();

            floraGeneratorCount +=
                ecotype.generators.size();

            for (const core::world::flora::FloraGeneratorRule& generator :
                 ecotype.generators)
            {
                floraVisualRuleCount +=
                    generator.visuals.size();
            }
        }

        core::Log::Info(
            std::string(
                "SO flora ecotypes: ") +
            std::to_string(
                floraConfig.ecotypes.size()));

        core::Log::Info(
            std::string(
                "SO flora terrain texture rules: ") +
            std::to_string(
                floraTextureRuleCount));

        core::Log::Info(
            std::string(
                "SO flora generators: ") +
            std::to_string(
                floraGeneratorCount));

        core::Log::Info(
            std::string(
                "SO flora visual rules: ") +
            std::to_string(
                floraVisualRuleCount));

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

        std::size_t speedTreeLodInstances =
            0;

        std::array<std::size_t, 3>
            speedTreeLodTriangles{};

        std::size_t speedTreeBillboardTriangles =
            0;

        std::size_t speedTreeBillboardResources =
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

            for (std::size_t lodIndex = 0;
                 lodIndex < 3;
                 ++lodIndex)
            {
                speedTreeRenderMeshes +=
                    renderData.lods[
                        lodIndex]
                        .meshIndices.size();

                speedTreeLodTriangles[
                    lodIndex] +=
                    renderData.lods[
                        lodIndex]
                        .triangleCount;
            }

            if (renderData.hasBillboard)
            {
                ++speedTreeRenderMeshes;

                ++speedTreeBillboardResources;

                speedTreeBillboardTriangles +=
                    renderData.billboardTriangles;
            }

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

            const SpeedTreeRenderData&
                renderData =
                    cached->second;

            graphics::SceneLodInstance
                instance;

            instance.transform =
                treeInstance.transform;

            for (std::size_t lodIndex = 0;
                 lodIndex < 3;
                 ++lodIndex)
            {
                if (renderData.lods[
                        lodIndex]
                        .meshIndices.empty())
                {
                    continue;
                }

                if (instance.levelCount >=
                    instance.levels.size())
                {
                    break;
                }

                graphics::SceneLodLevel&
                    level =
                        instance.levels[
                            instance.levelCount];

                level.meshIndices =
                    renderData.lods[
                        lodIndex]
                        .meshIndices;

                level.maximumDistance =
                    renderData.maximumDistances[
                        lodIndex];

                ++instance.levelCount;
            }

            if (renderData.hasBillboard &&
                instance.levelCount <
                    instance.levels.size())
            {
                graphics::SceneLodLevel&
                    billboardLevel =
                        instance.levels[
                            instance.levelCount];

                billboardLevel.meshIndices.push_back(
                    renderData.billboardMeshIndex);

                billboardLevel.maximumDistance =
                    std::numeric_limits<float>::max();

                ++instance.levelCount;
            }
            else if (instance.levelCount >
                     0)
            {
                instance.levels[
                    instance.levelCount -
                    1]
                    .maximumDistance =
                        std::numeric_limits<float>::max();
            }

            if (instance.levelCount ==
                0)
            {
                continue;
            }

            scene.lodInstances.push_back(
                std::move(
                    instance));

            ++speedTreeLodInstances;
        }

        core::Log::Info(
            std::string(
                "Scene fixed instances: ") +
            std::to_string(
                scene.instances.size()));

        core::Log::Info(
            std::string(
                "Scene LOD instances: ") +
            std::to_string(
                scene.lodInstances.size()));

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
                "SpeedTree LOD instances: ") +
            std::to_string(
                speedTreeLodInstances));

        core::Log::Info(
            std::string(
                "SpeedTree LOD0 triangles: ") +
            std::to_string(
                speedTreeLodTriangles[0]));

        core::Log::Info(
            std::string(
                "SpeedTree LOD1 triangles: ") +
            std::to_string(
                speedTreeLodTriangles[1]));

        core::Log::Info(
            std::string(
                "SpeedTree LOD2 triangles: ") +
            std::to_string(
                speedTreeLodTriangles[2]));

        core::Log::Info(
            std::string(
                "SpeedTree billboard resources: ") +
            std::to_string(
                speedTreeBillboardResources));

        core::Log::Info(
            std::string(
                "SpeedTree billboard triangles: ") +
            std::to_string(
                speedTreeBillboardTriangles));

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

        std::unordered_set<std::string>
            floraDominantTextures;

        std::unordered_set<std::string>
            floraMatchedTextures;

        std::unordered_set<std::string>
            floraUnmatchedTextures;

        std::unordered_set<std::string>
            activeFloraEcotypes;

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

            const core::world::TerrainDominantTextureData&
                dominantTextures =
                    terrain.auxiliary.dominantTextures;

            if (dominantTextures.present &&
                !dominantTextures.textureReferences.empty())
            {
                std::vector<std::uint8_t>
                    usedTextures(
                        dominantTextures.textureReferences.size(),
                        0);

                for (const std::uint8_t textureIndex :
                     dominantTextures.indices)
                {
                    if (textureIndex <
                        usedTextures.size())
                    {
                        usedTextures[
                            textureIndex] =
                            1;
                    }
                }

                for (std::size_t textureIndex = 0;
                     textureIndex <
                        usedTextures.size();
                     ++textureIndex)
                {
                    if (usedTextures[
                            textureIndex] ==
                        0)
                    {
                        continue;
                    }

                    const std::string& textureReference =
                        dominantTextures.textureReferences[
                            textureIndex];

                    const std::string normalizedTexture =
                        core::resources::ResourcePath::Normalize(
                            textureReference);

                    if (normalizedTexture.empty())
                    {
                        continue;
                    }

                    floraDominantTextures.insert(
                        normalizedTexture);

                    const std::vector<
                        const core::world::flora::FloraEcotype*>
                        ecotypes =
                            floraConfig.FindEcotypesByTexture(
                                textureReference);

                    if (ecotypes.empty())
                    {
                        floraUnmatchedTextures.insert(
                            normalizedTexture);

                        continue;
                    }

                    floraMatchedTextures.insert(
                        normalizedTexture);

                    for (const core::world::flora::FloraEcotype* ecotype :
                         ecotypes)
                    {
                        if (ecotype ==
                            nullptr)
                        {
                            continue;
                        }

                        activeFloraEcotypes.insert(
                            ecotype->name);
                    }
                }
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

        core::Log::Info(
            std::string(
                "Flora dominant terrain textures: ") +
            std::to_string(
                floraDominantTextures.size()));

        core::Log::Info(
            std::string(
                "Flora matched terrain textures: ") +
            std::to_string(
                floraMatchedTextures.size()));

        core::Log::Info(
            std::string(
                "Flora unmatched terrain textures: ") +
            std::to_string(
                floraUnmatchedTextures.size()));

        core::Log::Info(
            std::string(
                "Active flora ecotypes: ") +
            std::to_string(
                activeFloraEcotypes.size()));

        for (const std::string& ecotype :
             activeFloraEcotypes)
        {
            core::Log::Info(
                std::string(
                    "Flora ecotype: ") +
                ecotype);
        }

        for (const std::string& texture :
             floraUnmatchedTextures)
        {
            core::Log::Warning(
                std::string(
                    "No flora ecotype for terrain texture: ") +
                texture);
        }

        if (scene.meshes.empty() ||
            (
                scene.instances.empty() &&
                scene.lodInstances.empty()
            ))
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