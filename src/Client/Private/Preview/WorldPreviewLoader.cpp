#include "Preview/WorldPreviewLoader.h"

#include "Preview/ModelRenderDataBuilder.h"
#include "Preview/TerrainRenderDataBuilder.h"
#include "Preview/SpeedTreeRenderDataBuilder.h"
#include "Preview/FloraRenderDataBuilder.h"
#include "Preview/WaterRenderDataBuilder.h"

#include "Core/Assets/MeshLoader.h"
#include "Core/Assets/ModelBundleLoader.h"
#include "Core/Assets/SpeedTree/CTreeLoader.h"

#include "Core/Log.h"
#include "Core/Resources/ResourcePath.h"
#include "Core/World/TerrainLoader.h"
#include "Core/World/Flora/FloraConfigLoader.h"
#include "Core/World/Flora/FloraVisualLoader.h"
#include "Core/World/WorldLoader.h"
#include "Core/World/Flora/FloraInstanceBuilder.h"

#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <limits>
#include <iterator>

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

        scene.omniLights.reserve(
            world.omniLights.size());

        for (const core::world::WorldOmniLightInstance& source :
             world.omniLights)
        {
            graphics::SceneOmniLight
                light;

            light.guid =
                source.guid;

            light.position =
            {
                source.position.x,
                source.position.y,
                source.position.z
            };

            light.colour =
            {
                std::clamp(
                    source.colour.x /
                        255.0f,
                    0.0f,
                    1.0f),

                std::clamp(
                    source.colour.y /
                        255.0f,
                    0.0f,
                    1.0f),

                std::clamp(
                    source.colour.z /
                        255.0f,
                    0.0f,
                    1.0f)
            };

            light.innerRadius =
                std::max(
                    source.innerRadius,
                    0.0f);

            light.outerRadius =
                std::max(
                    source.outerRadius,
                    light.innerRadius);

            light.multiplier =
                std::max(
                    source.multiplier,
                    0.0f);

            light.priority =
                source.priority;

            light.lightType =
                source.lightType;

            light.isDynamic =
                source.isDynamic;

            light.isStatic =
                source.isStatic;

            light.specular =
                source.specular;

            scene.omniLights.push_back(
                std::move(light));
        }

        core::Log::Info(
            std::string(
                "OmniLight render sources: ") +
            std::to_string(
                scene.omniLights.size()));

        scene.spotLights.reserve(
            world.spotLights.size());

        for (const core::world::WorldSpotLightInstance& source :
             world.spotLights)
        {
            graphics::SceneSpotLight
                light;

            light.guid =
                source.guid;

            light.position =
            {
                source.position.x,
                source.position.y,
                source.position.z
            };

            light.direction =
            {
                source.direction.x,
                source.direction.y,
                source.direction.z
            };

            light.colour =
            {
                std::clamp(
                    source.colour.x /
                        255.0f,
                    0.0f,
                    1.0f),

                std::clamp(
                    source.colour.y /
                        255.0f,
                    0.0f,
                    1.0f),

                std::clamp(
                    source.colour.z /
                        255.0f,
                    0.0f,
                    1.0f)
            };

            light.innerRadius =
                std::max(
                    source.innerRadius,
                    0.0f);

            light.outerRadius =
                std::max(
                    source.outerRadius,
                    light.innerRadius);

            light.cosConeAngle =
                std::clamp(
                    source.cosConeAngle,
                    -1.0f,
                    1.0f);

            light.multiplier =
                std::max(
                    source.multiplier,
                    0.0f);

            light.priority =
                source.priority;

            light.lightType =
                source.lightType;

            light.isDynamic =
                source.isDynamic;

            light.isStatic =
                source.isStatic;

            light.specular =
                source.specular;

            scene.spotLights.push_back(
                std::move(light));
        }

        core::Log::Info(
            std::string(
                "SpotLight render sources: ") +
            std::to_string(
                scene.spotLights.size()));

        scene.pulseLights.reserve(
            world.pulseLights.size());

        for (const core::world::WorldPulseLightInstance& source :
             world.pulseLights)
        {
            graphics::ScenePulseLight
                light;

            light.guid =
                source.guid;

            light.animation =
                source.animation;

            light.position =
            {
                source.position.x,
                source.position.y,
                source.position.z
            };

            light.colour =
            {
                std::clamp(
                    source.colour.x /
                        255.0f,
                    0.0f,
                    1.0f),

                std::clamp(
                    source.colour.y /
                        255.0f,
                    0.0f,
                    1.0f),

                std::clamp(
                    source.colour.z /
                        255.0f,
                    0.0f,
                    1.0f)
            };

            light.innerRadius =
                std::max(
                    source.innerRadius,
                    0.0f);

            light.outerRadius =
                std::max(
                    source.outerRadius,
                    light.innerRadius);

            light.multiplier =
                std::max(
                    source.multiplier,
                    0.0f);

            light.timeScale =
                std::max(
                    source.timeScale,
                    0.0f);

            light.duration =
                std::max(
                    source.duration,
                    0.0f);

            light.priority =
                source.priority;

            light.frames.reserve(
                source.frames.size());

            for (const core::world::WorldPulseLightFrame& sourceFrame :
                 source.frames)
            {
                graphics::ScenePulseLightFrame
                    frame;

                frame.time =
                    sourceFrame.time;

                frame.value =
                    sourceFrame.value;

                light.frames.push_back(
                    frame);
            }

            scene.pulseLights.push_back(
                std::move(light));
        }

        core::Log::Info(
            std::string(
                "PulseLight render sources: ") +
            std::to_string(
                scene.pulseLights.size()));

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

        core::world::flora::FloraInstanceBuilder
            floraInstanceBuilder;

        std::vector<
            core::world::flora::FloraInstance>
            floraInstances;

        std::size_t floraPlacementFailures =
            0;

        core::assets::ModelBundleLoader
            bundleLoader;

        core::assets::MeshLoader
            meshLoader;

        ModelRenderDataBuilder
            modelRenderBuilder;

        std::size_t skippedInstances = 0;
        std::size_t loadedUniqueModels = 0;
        std::size_t texturedModelGroups = 0;
        std::size_t nonRenderableHelperInstances = 0;

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

        std::size_t speedTreeFixedInstances =
            0;

        std::size_t speedTreeRejectedInstances =
            0;

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
                ++speedTreeRejectedInstances;
                continue;
            }

            const SpeedTreeRenderData& renderData =
                cached->second;

            if (!renderData.usesLodChain)
            {
                if (renderData.fixedMeshIndices.empty())
                {
                    ++speedTreeRejectedInstances;
                    continue;
                }

                for (const std::size_t meshIndex :
                     renderData.fixedMeshIndices)
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
                }

                ++speedTreeFixedInstances;

                continue;
            }

            graphics::SceneLodInstance
                instance;

            instance.transform =
                treeInstance.transform;

            for (std::size_t lodIndex = 0;
                 lodIndex <
                    renderData.lods.size();
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

                graphics::SceneLodLevel& level =
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

            if (instance.levelCount > 0)
            {
                instance.levels[
                    instance.levelCount -
                    1]
                    .maximumDistance =
                        std::numeric_limits<float>::max();
            }

            if (instance.levelCount == 0)
            {
                if (!renderData.fixedMeshIndices.empty())
                {
                    for (const std::size_t meshIndex :
                         renderData.fixedMeshIndices)
                    {
                        graphics::SceneInstance
                            fixedInstance;

                        fixedInstance.meshIndex =
                            meshIndex;

                        fixedInstance.transform =
                            treeInstance.transform;

                        scene.instances.push_back(
                            std::move(
                                fixedInstance));
                    }

                    ++speedTreeFixedInstances;

                    continue;
                }

                ++speedTreeRejectedInstances;

                continue;
            }

            scene.lodInstances.push_back(
                std::move(
                    instance));

            ++speedTreeLodInstances;
        }

        core::Log::Info(
        std::string(
        "SpeedTree fixed instances: ") +
            std::to_string(
        speedTreeFixedInstances));

        core::Log::Info(
            std::string(
                "SpeedTree LOD instances: ") +
            std::to_string(
                speedTreeLodInstances));

        core::Log::Info(
            std::string(
                "SpeedTree rejected instances: ") +
            std::to_string(
                speedTreeRejectedInstances));

        core::Log::Info(
            std::string(
                "SpeedTree accounted instances: ") +
            std::to_string(
                speedTreeFixedInstances +
                speedTreeLodInstances +
                speedTreeRejectedInstances));

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
        "SpeedTree billboard source resources: ") +
            std::to_string(
                loadedSpeedTrees));

        core::Log::Info(
            "SpeedTree billboard rendering: disabled until camera-facing renderer is implemented");

        if (speedTreeRejectedInstances != 0)
        {
            core::Log::Warning(
                std::string(
                    "Rejected SpeedTree instances: ") +
                std::to_string(
                    speedTreeRejectedInstances));
        }

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

            if (normalizedModel.starts_with("helpers/") || normalizedModel.starts_with("res/helpers/"))
            {
                ++nonRenderableHelperInstances;
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
            floraTerrainTextures;

        std::unordered_set<std::string>
            floraMatchedTextures;

        std::unordered_set<std::string>
            floraNoEcotypeTextures;

        std::unordered_set<std::string>
            activeFloraEcotypes;

        const auto registerFloraTexture =
            [&floraConfig,
             &floraTerrainTextures,
             &floraMatchedTextures,
             &floraNoEcotypeTextures,
             &activeFloraEcotypes](
                const std::string_view textureReference)
            {
                const std::string textureKey =
                    core::world::flora::BuildFloraTextureKey(
                        textureReference);

                if (textureKey.empty())
                {
                    return;
                }

                floraTerrainTextures.insert(
                    textureKey);

                const std::vector<
                    const core::world::flora::FloraEcotype*>
                    ecotypes =
                        floraConfig.FindEcotypesByTexture(
                            textureReference);

                if (ecotypes.empty())
                {
                    floraNoEcotypeTextures.insert(
                        textureKey);

                    return;
                }

                floraMatchedTextures.insert(
                    textureKey);

                floraNoEcotypeTextures.erase(
                    textureKey);

                for (const core::world::flora::FloraEcotype* ecotype :
                     ecotypes)
                {
                    if (ecotype == nullptr)
                    {
                        continue;
                    }

                    activeFloraEcotypes.insert(
                        ecotype->name);
                }
            };

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

            std::vector<
                core::world::flora::FloraInstance>
                terrainFloraInstances;

            std::string floraPlacementError;

            if (!floraInstanceBuilder.Build(
                terrainInstance.chunkId,
                terrain.heightData,
                terrain.layers,
                terrain.auxiliary,
                terrainInstance.transform,
                floraConfig,
                terrainFloraInstances,
                floraPlacementError))
            {
                ++floraPlacementFailures;

                core::Log::Warning(
                    std::string(
                        "Flora placement failed for terrain ") +
                    terrainInstance.chunkId +
                    ": " +
                    floraPlacementError);
            }
            else
            {
                floraInstances.insert(
                    floraInstances.end(),
                    std::make_move_iterator(
                        terrainFloraInstances.begin()),
                    std::make_move_iterator(
                        terrainFloraInstances.end()));
            }

            for (const core::world::TerrainLayerData& layer : terrain.layers)
            {
                if (layer.blend.empty())
                {
                    continue;
                }

                const bool layerUsed =
                    std::any_of(
                        layer.blend.begin(),
                        layer.blend.end(),
                        [](
                            const std::uint8_t value)
                        {
                            return
                                value != 0;
                        });

                if (!layerUsed)
                {
                    continue;
                }

                if (!layer.texture.sourceReference.empty())
                {
                    registerFloraTexture(
                        layer.texture.sourceReference);
                }
                else if (!layer.texture.sourceLogicalPath.empty())
                {
                    registerFloraTexture(
                        layer.texture.sourceLogicalPath);
                }
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

                    registerFloraTexture(
                        dominantTextures.textureReferences[
                            textureIndex]);
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
        "Flora terrain textures: ") +
            std::to_string(
        floraTerrainTextures.size()));

        core::Log::Info(
            std::string(
                "Flora matched terrain textures: ") +
            std::to_string(
                floraMatchedTextures.size()));

        core::Log::Info(
            std::string(
                "Flora no-ecotype terrain textures: ") +
            std::to_string(
                floraNoEcotypeTextures.size()));

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

        std::unordered_set<std::string>
            uniqueActiveFloraVisuals;

        for (const core::world::flora::FloraEcotype& ecotype :
             floraConfig.ecotypes)
        {
            if (!activeFloraEcotypes.contains(
                    ecotype.name))
            {
                continue;
            }

            for (const core::world::flora::FloraGeneratorRule& generator :
                 ecotype.generators)
            {
                for (const core::world::flora::FloraVisualRule& visual :
                     generator.visuals)
                {
                    const std::string normalized =
                        core::resources::ResourcePath::Normalize(
                            visual.visualReference);

                    if (normalized.empty())
                    {
                        continue;
                    }

                    uniqueActiveFloraVisuals.insert(
                        normalized);
                }
            }
        }

        std::vector<std::string>
            activeFloraVisuals(
                uniqueActiveFloraVisuals.begin(),
                uniqueActiveFloraVisuals.end());

        std::sort(
            activeFloraVisuals.begin(),
            activeFloraVisuals.end());

        core::world::flora::FloraVisualLoader
            floraVisualLoader;

        FloraRenderDataBuilder
            floraRenderBuilder;

        std::unordered_map<
            std::string,
            FloraRenderData>
            floraRenderCache;

        std::unordered_set<std::string>
            failedFloraVisuals;

        std::size_t floraMeshCount =
            0;

        std::size_t floraTriangleCount =
            0;

        std::size_t floraTexturedGroupCount =
            0;

        const float floraAlphaCutoff =
            static_cast<float>(
                floraConfig.alphaTestReference) /
            255.0f;

        for (const std::string& visualReference :
             activeFloraVisuals)
        {
            core::world::flora::FloraVisualAsset
                floraAsset;

            std::string floraError;

            if (!floraVisualLoader.Load(
                    runtime.Resources(),
                    visualReference,
                    floraAsset,
                    floraError))
            {
                failedFloraVisuals.insert(
                    visualReference);

                core::Log::Warning(
                    std::string(
                        "Flora visual load failed: ") +
                    visualReference +
                    ": " +
                    floraError);

                continue;
            }

            FloraRenderData
                renderData;

            if (!floraRenderBuilder.Build(
                    runtime.Resources(),
                    floraAsset,
                    floraAlphaCutoff,
                    scene,
                    renderData,
                    floraError))
            {
                failedFloraVisuals.insert(
                    visualReference);

                core::Log::Warning(
                    std::string(
                        "Flora render build failed: ") +
                    visualReference +
                    ": " +
                    floraError);

                continue;
            }

            floraMeshCount +=
                renderData.meshIndices.size();

            floraTriangleCount +=
                renderData.triangleCount;

            floraTexturedGroupCount +=
                renderData.texturedPrimitiveGroups;

            core::Log::Info(
                std::string(
                    "Flora visual loaded: ") +
                floraAsset.visualLogicalPath +
                ", meshes=" +
                std::to_string(
                    renderData.meshIndices.size()) +
                ", triangles=" +
                std::to_string(
                    renderData.triangleCount) +
                ", texturedGroups=" +
                std::to_string(
                    renderData.texturedPrimitiveGroups));

            floraRenderCache.emplace(
                visualReference,
                std::move(
                    renderData));
        }

        core::Log::Info(
            std::string(
                "Active flora visual resources: ") +
            std::to_string(
                activeFloraVisuals.size()));

        core::Log::Info(
            std::string(
                "Flora visual resources loaded: ") +
            std::to_string(
                floraRenderCache.size()));

        core::Log::Info(
            std::string(
                "Flora visual resources failed: ") +
            std::to_string(
                failedFloraVisuals.size()));

        core::Log::Info(
            std::string(
                "Flora render meshes: ") +
            std::to_string(
                floraMeshCount));

        core::Log::Info(
            std::string(
                "Flora triangles: ") +
            std::to_string(
                floraTriangleCount));

        core::Log::Info(
            std::string(
                "Flora textured primitive groups: ") +
            std::to_string(
                floraTexturedGroupCount));

        core::Log::Info(
            std::string(
                "Flora alpha cutoff: ") +
            std::to_string(
                floraAlphaCutoff));

        std::size_t floraSceneInstanceCount =
            0;

        std::size_t floraMissingRenderResources =
            0;

        std::unordered_map<
            std::string,
            std::size_t>
            floraInstancesPerEcotype;

        for (const core::world::flora::FloraInstance& floraInstance :
             floraInstances)
        {
            const std::string visualKey =
                core::resources::ResourcePath::Normalize(
                    floraInstance.visualReference);

            const auto cached =
                floraRenderCache.find(
                    visualKey);

            if (cached ==
                floraRenderCache.end())
            {
                ++floraMissingRenderResources;

                continue;
            }

            ++floraInstancesPerEcotype[
                floraInstance.ecotypeName];

            for (const std::size_t meshIndex :
                 cached->second.meshIndices)
            {
                graphics::SceneInstance
                    instance;

                instance.meshIndex =
                    meshIndex;

                instance.transform =
                    floraInstance.transform;

                instance.maximumDistance =
                    floraConfig.alphaTestDistance;

                scene.instances.push_back(
                    std::move(
                        instance));

                ++floraSceneInstanceCount;
            }
        }

        core::Log::Info(
            std::string(
                "Procedural flora instances: ") +
            std::to_string(
                floraInstances.size()));

        core::Log::Info(
            std::string(
                "Flora scene instances: ") +
            std::to_string(
                floraSceneInstanceCount));

        core::Log::Info(
            std::string(
                "Flora placement failures: ") +
            std::to_string(
                floraPlacementFailures));

        core::Log::Info(
            std::string(
                "Flora instances without render resource: ") +
            std::to_string(
                floraMissingRenderResources));

        for (const auto& [ecotype, count] :
             floraInstancesPerEcotype)
        {
            core::Log::Info(
                std::string(
                    "Flora instances [") +
                ecotype +
                "]: " +
                std::to_string(
                    count));
        }

        for (const std::string& texture :
            floraNoEcotypeTextures)
        {
            core::Log::Info(
                std::string(
                    "Flora disabled for terrain texture: ") +
                texture);
        }

        WaterRenderDataBuilder
            waterRenderBuilder;

        std::size_t loadedWaterSurfaces =
            0;

        std::size_t failedWaterSurfaces =
            0;

        std::size_t totalWaterVertices =
            0;

        std::size_t totalWaterTriangles =
            0;

        std::size_t uniqueWaterObjects =
            0;

        std::size_t waterChunkReferences =
            0;

        for (const core::world::WorldLargeObjectReference& object :
             world.largeObjects)
        {
            if (!object.vloLoaded)
            {
                continue;
            }

            if (object.vloResource.type !=
                core::world::vlo::VloType::Water)
            {
                continue;
            }

            ++uniqueWaterObjects;

            waterChunkReferences +=
                object.chunkIds.size();

            if (!object.vloResource.water.has_value())
            {
                ++failedWaterSurfaces;

                core::Log::Warning(
                    std::string(
                        "Water VLO has no water definition: ") +
                    object.uid);

                continue;
            }

            WaterRenderData
                renderData;

            std::string
                waterError;

            if (!waterRenderBuilder.Build(
                runtime.Resources(),
                object,
                scene,
                renderData,
                waterError))
            {
                ++failedWaterSurfaces;

                core::Log::Warning(
                    std::string(
                        "Water render build failed: uid=") +
                    object.uid +
                    ": " +
                    waterError);

                continue;
            }

            ++loadedWaterSurfaces;

            totalWaterVertices +=
                renderData.vertexCount;

            totalWaterTriangles +=
                renderData.triangleCount;

            const core::world::water::WaterDefinition&
                water =
                    *object.vloResource.water;

            core::Log::Info(
                std::string(
                    "SO water surface loaded: uid=") +
                object.uid +
                ", chunks=" +
                std::to_string(
                    object.chunkIds.size()) +
                ", vertices=" +
                std::to_string(
                    renderData.vertexCount) +
                ", triangles=" +
                std::to_string(
                    renderData.triangleCount) +
                ", position=(" +
                std::to_string(
                    water.position.x) +
                ", " +
                std::to_string(
                    water.position.y) +
                ", " +
                std::to_string(
                    water.position.z) +
                "), size=(" +
                std::to_string(
                    water.size.x) +
                ", " +
                std::to_string(
                    water.size.z) +
                "), orientation=" +
                std::to_string(
                    water.orientation));
        }

        core::Log::Info(
            std::string(
                "SO water surfaces loaded: ") +
            std::to_string(
                loadedWaterSurfaces));

        core::Log::Info(
            std::string(
                "SO water surfaces failed: ") +
            std::to_string(
                failedWaterSurfaces));

        core::Log::Info(
            std::string(
                "SO water vertices: ") +
            std::to_string(
                totalWaterVertices));

        core::Log::Info(
            std::string(
                "SO water triangles: ") +
            std::to_string(
                totalWaterTriangles));

        core::Log::Info(
            std::string(
                "Water VLO unique objects: ") +
        std::to_string(
            uniqueWaterObjects));

        core::Log::Info(
            std::string(
                "Water VLO chunk coverage references: ") +
            std::to_string(
                waterChunkReferences));

        if (loadedWaterSurfaces !=
            uniqueWaterObjects)
        {
            core::Log::Warning(
                std::string(
                    "Water VLO placement mismatch: unique=") +
                std::to_string(
                    uniqueWaterObjects) +
                ", renderInstances=" +
                std::to_string(
                    loadedWaterSurfaces));
        }
        else
        {
            core::Log::Info(
                std::string(
                    "Water VLO placement validated: references=") +
                std::to_string(
                    waterChunkReferences) +
                ", unique=" +
                std::to_string(
                    uniqueWaterObjects) +
                ", renderInstances=" +
                std::to_string(
                    loadedWaterSurfaces));
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
        "Non-render helper instances: ") +
            std::to_string(
                nonRenderableHelperInstances));

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