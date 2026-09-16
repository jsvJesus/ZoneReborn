#include "Core/World/WorldLoader.h"

#include "Core/Log.h"
#include "Core/Resources/ResourcePath.h"
#include "Core/Resources/ResourceType.h"
#include "Core/World/ChunkLoader.h"
#include "Core/World/SpaceLoader.h"
#include "Core/World/Vlo/VloLoader.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
    constexpr float OutdoorChunkSize =
        100.0f;

    int HexValue(
        const char character) noexcept
    {
        if (character >= '0' &&
            character <= '9')
        {
            return character - '0';
        }

        if (character >= 'a' &&
            character <= 'f')
        {
            return
                character - 'a' + 10;
        }

        if (character >= 'A' &&
            character <= 'F')
        {
            return
                character - 'A' + 10;
        }

        return -1;
    }

    bool ParseHex16(
        const std::string_view value,
        std::uint16_t& output) noexcept
    {
        if (value.size() != 4)
        {
            return false;
        }

        std::uint16_t result = 0;

        for (const char character : value)
        {
            const int digit =
                HexValue(character);

            if (digit < 0)
            {
                return false;
            }

            result =
                static_cast<std::uint16_t>(
                    (result << 4u) |
                    static_cast<std::uint16_t>(
                        digit));
        }

        output = result;

        return true;
    }

    std::int32_t SignedGridCoordinate(
        const std::uint16_t value) noexcept
    {
        if (value < 0x8000u)
        {
            return
                static_cast<std::int32_t>(
                    value);
        }

        return
            static_cast<std::int32_t>(
                value) -
            0x10000;
    }

    bool BuildOutdoorChunkTransform(
        const std::string_view chunkId,
        core::math::Transform3x4& output) noexcept
    {
        if (chunkId.size() != 9)
        {
            return false;
        }

        if (chunkId[8] != 'o' &&
            chunkId[8] != 'O')
        {
            return false;
        }

        std::uint16_t rawX = 0;
        std::uint16_t rawZ = 0;

        if (!ParseHex16(
                chunkId.substr(0, 4),
                rawX) ||
            !ParseHex16(
                chunkId.substr(4, 4),
                rawZ))
        {
            return false;
        }

        const std::int32_t gridX =
            SignedGridCoordinate(rawX);

        const std::int32_t gridZ =
            SignedGridCoordinate(rawZ);

        output =
            core::math::Transform3x4::Translation(
                static_cast<float>(gridX) *
                    OutdoorChunkSize,
                0.0f,
                static_cast<float>(gridZ) *
                    OutdoorChunkSize);

        return true;
    }

    bool BuildSpeedTreePath(
        const std::string_view resourceReference,
        std::string& output)
    {
        output.clear();

        std::string resource =
            core::resources::ResourcePath::Normalize(
                resourceReference);

        if (resource.empty())
        {
            return false;
        }

        if (!resource.ends_with(
                ".spt"))
        {
            resource +=
                ".spt";
        }

        if (resource.starts_with(
                "res/"))
        {
            output =
                resource;

            return true;
        }

        output =
            "res/" +
            resource;

        return true;
    }

    bool BuildTerrainCDataPath(
        const std::string_view spaceName,
        const std::string_view resourceReference,
        std::string& output)
    {
        output.clear();

        std::string resource =
            core::resources::ResourcePath::Normalize(
                resourceReference);

        if (resource.empty())
        {
            return false;
        }

        constexpr std::string_view TerrainSuffix =
            "/terrain2";

        if (resource.ends_with(
                TerrainSuffix))
        {
            resource.resize(
                resource.size() -
                TerrainSuffix.size());
        }

        if (!resource.ends_with(
                ".cdata"))
        {
            return false;
        }

        if (resource.starts_with(
                "res/"))
        {
            output =
                resource;

            return true;
        }

        if (resource.starts_with(
                "spaces/"))
        {
            output =
                "res/" +
                resource;

            return true;
        }

        const std::string normalizedSpace =
            core::resources::ResourcePath::Normalize(
                spaceName);

        if (normalizedSpace.empty())
        {
            return false;
        }

        output =
            "res/spaces/" +
            normalizedSpace +
            "/" +
            resource;

        return true;
    }

    bool BuildLargeObjectPaths(
        const std::string_view spaceName,
        const std::string_view uidReference,
        std::string& vloPath,
        std::string& odataPath)
    {
        vloPath.clear();
        odataPath.clear();

        std::string space =
            core::resources::ResourcePath::Normalize(
                spaceName);

        std::string uid =
            core::resources::ResourcePath::Normalize(
                uidReference);

        if (space.empty() ||
            uid.empty())
        {
            return false;
        }

        if (space.find('/') !=
            std::string::npos)
        {
            return false;
        }

        if (uid.find('/') !=
            std::string::npos)
        {
            return false;
        }

        while (!uid.empty() &&
               uid.front() ==
                   '_')
        {
            uid.erase(
                uid.begin());
        }

        constexpr std::string_view VloExtension =
            ".vlo";

        constexpr std::string_view ODataExtension =
            ".odata";

        if (uid.ends_with(
                VloExtension))
        {
            uid.resize(
                uid.size() -
                VloExtension.size());
        }
        else if (uid.ends_with(
                     ODataExtension))
        {
            uid.resize(
                uid.size() -
                ODataExtension.size());
        }

        if (uid.empty())
        {
            return false;
        }

        const std::string prefix =
            "res/spaces/" +
            space +
            "/";

        vloPath =
            prefix +
            "_" +
            uid +
            ".vlo";

        odataPath =
            prefix +
            uid +
            ".odata";

        return true;
    }

    std::string BuildLargeObjectKey(
        const std::string_view uid,
        const std::string_view type)
    {
        std::string normalizedUid =
            core::resources::ResourcePath::Normalize(
                uid);

        std::string normalizedType =
            core::resources::ResourcePath::Normalize(
                type);

        while (!normalizedUid.empty() &&
               normalizedUid.front() ==
                   '_')
        {
            normalizedUid.erase(
                normalizedUid.begin());
        }

        return
            normalizedUid +
            "|" +
            normalizedType;
    }

    bool AddLargeObjectReference(
        const core::resources::ResourceFileSystem& resources,
        const std::string_view spaceName,
        const std::string& chunkId,
        const core::world::ChunkLargeObjectReference& source,
        std::unordered_map<
            std::string,
            std::size_t>& lookup,
        core::world::WorldScene& scene,
        std::string& error)
    {
        error.clear();

        const std::string key =
            BuildLargeObjectKey(
                source.uid,
                source.type);

        if (key.empty())
        {
            error =
                "Large object UID/type is invalid.";

            return false;
        }

        const auto existing =
            lookup.find(
                key);

        if (existing !=
            lookup.end())
        {
            core::world::WorldLargeObjectReference&
                object =
                    scene.largeObjects[
                        existing->second];

            if (std::find(
                    object.chunkIds.begin(),
                    object.chunkIds.end(),
                    chunkId) ==
                object.chunkIds.end())
            {
                object.chunkIds.push_back(
                    chunkId);
            }

            return true;
        }

        core::world::WorldLargeObjectReference
            object;

        object.uid =
            source.uid;

        object.type =
            source.type;

        if (!BuildLargeObjectPaths(
                spaceName,
                source.uid,
                object.vloLogicalPath,
                object.odataLogicalPath))
        {
            error =
                "Unable to build VLO paths for UID: " +
                source.uid;

            return false;
        }

        object.vloExists =
            resources.Exists(
                object.vloLogicalPath);

        object.odataExists =
            resources.Exists(
                object.odataLogicalPath);

        object.chunkIds.push_back(
            chunkId);

        const std::size_t index =
            scene.largeObjects.size();

        scene.largeObjects.push_back(
            std::move(
                object));

        lookup.emplace(
            key,
            index);

        return true;
    }

    core::math::Vector3 TransformPoint(
        const core::math::Vector3& point,
        const core::math::Transform3x4& transform) noexcept
    {
        return
        {
            point.x * transform.values[0] +
                point.y * transform.values[3] +
                point.z * transform.values[6] +
                transform.values[9],

            point.x * transform.values[1] +
                point.y * transform.values[4] +
                point.z * transform.values[7] +
                transform.values[10],

            point.x * transform.values[2] +
                point.y * transform.values[5] +
                point.z * transform.values[8] +
                transform.values[11]
        };
    }

    core::math::Vector3 TransformDirection(
        const core::math::Vector3& direction,
        const core::math::Transform3x4& transform) noexcept
    {
        return
        {
            direction.x * transform.values[0] +
                direction.y * transform.values[3] +
                direction.z * transform.values[6],

            direction.x * transform.values[1] +
                direction.y * transform.values[4] +
                direction.z * transform.values[7],

            direction.x * transform.values[2] +
                direction.y * transform.values[5] +
                direction.z * transform.values[8]
        };
    }

    void AddOmniLightInstance(
        const std::string& chunkId,
        const core::world::ChunkOmniLight& source,
        const core::math::Transform3x4& chunkTransform,
        core::world::WorldScene& scene)
    {
        core::world::WorldOmniLightInstance
            instance;

        instance.chunkId =
            chunkId;

        instance.guid =
            source.guid;

        instance.position =
            TransformPoint(
                source.position,
                chunkTransform);

        instance.colour =
            source.colour;

        instance.innerRadius =
            source.innerRadius;

        instance.outerRadius =
            source.outerRadius;

        instance.multiplier =
            source.multiplier;

        instance.priority =
            source.priority;

        instance.lightType =
            source.lightType;

        instance.isDynamic =
            source.isDynamic;

        instance.isStatic =
            source.isStatic;

        instance.specular =
            source.specular;

        scene.omniLights.push_back(
            std::move(instance));
    }

    void AddSpotLightInstance(
        const std::string& chunkId,
        const core::world::ChunkSpotLight& source,
        const core::math::Transform3x4& chunkTransform,
        core::world::WorldScene& scene)
    {
        core::world::WorldSpotLightInstance
            instance;

        instance.chunkId =
            chunkId;

        instance.guid =
            source.guid;

        instance.position =
            TransformPoint(
                source.position,
                chunkTransform);

        instance.direction =
            TransformDirection(
                source.direction,
                chunkTransform);

        instance.colour =
            source.colour;

        instance.innerRadius =
            source.innerRadius;

        instance.outerRadius =
            source.outerRadius;

        instance.cosConeAngle =
            source.cosConeAngle;

        instance.multiplier =
            source.multiplier;

        instance.priority =
            source.priority;

        instance.lightType =
            source.lightType;

        instance.isDynamic =
            source.isDynamic;

        instance.isStatic =
            source.isStatic;

        instance.specular =
            source.specular;

        scene.spotLights.push_back(
            std::move(instance));
    }

    void AddPulseLightInstance(
        const std::string& chunkId,
        const core::world::ChunkPulseLight& source,
        const core::math::Transform3x4& chunkTransform,
        core::world::WorldScene& scene)
    {
        core::world::WorldPulseLightInstance
            instance;

        instance.chunkId =
            chunkId;

        instance.guid =
            source.guid;

        instance.animation =
            source.animation;

        instance.position =
            TransformPoint(
                source.position,
                chunkTransform);

        instance.colour =
            source.colour;

        instance.innerRadius =
            source.innerRadius;

        instance.outerRadius =
            source.outerRadius;

        instance.multiplier =
            source.multiplier;

        instance.timeScale =
            source.timeScale;

        instance.duration =
            source.duration;

        instance.priority =
            source.priority;

        instance.frames.reserve(
            source.frames.size());

        for (const core::world::ChunkPulseLightFrame& sourceFrame :
             source.frames)
        {
            core::world::WorldPulseLightFrame
                frame;

            frame.time =
                sourceFrame.time;

            frame.value =
                sourceFrame.value;

            instance.frames.push_back(
                frame);
        }

        scene.pulseLights.push_back(
            std::move(instance));
    }

    void AddModelInstance(
        const std::string& chunkId,
        const core::world::ChunkModelInstance& source,
        const core::math::Transform3x4& chunkTransform,
        const bool shell,
        core::world::WorldScene& scene)
    {
        core::world::WorldModelInstance instance;

        instance.chunkId =
            chunkId;

        instance.modelReference =
            source.resource;

        instance.transform =
            core::math::Transform3x4::Multiply(
                source.transform,
                chunkTransform);

        instance.shell =
            shell;

        scene.modelInstances.push_back(
            std::move(instance));
    }

    bool AddSpeedTreeInstance(
        const core::resources::ResourceFileSystem& resources,
        const std::string& chunkId,
        const core::world::ChunkSpeedTreeInstance& source,
        const core::math::Transform3x4& chunkTransform,
        core::world::WorldScene& scene,
        std::string& missingResource)
    {
        missingResource.clear();

        core::world::WorldSpeedTreeInstance
            instance;

        instance.chunkId =
            chunkId;

        instance.resourceReference =
            source.resource;

        if (!BuildSpeedTreePath(
                source.resource,
                instance.sptLogicalPath))
        {
            missingResource =
                source.resource;

            return false;
        }

        if (!resources.Exists(
                instance.sptLogicalPath))
        {
            missingResource =
                instance.sptLogicalPath;

            return false;
        }

        instance.seed =
            source.seed;

        instance.transform =
            core::math::Transform3x4::Multiply(
                source.transform,
                chunkTransform);

        instance.reflectionVisible =
            source.reflectionVisible;

        scene.speedTreeInstances.push_back(
            std::move(instance));

        return true;
    }
}

namespace core::world
{
    bool WorldLoader::Load(
        const resources::ResourceFileSystem& resources,
        const std::string_view spaceName,
        WorldScene& output,
        std::string& error) const
    {
        output = {};
        error.clear();

        if (!resources.IsInitialized())
        {
            error =
                "Resource filesystem is not initialized.";

            return false;
        }

        SpaceLoader spaceLoader;

        SpaceSettings settings;

        if (!spaceLoader.Load(
                resources,
                spaceName,
                settings,
                error))
        {
            return false;
        }

        WorldScene scene;

        scene.spaceName =
            std::string(spaceName);

        scene.settings =
            std::move(settings);

        std::string normalizedSpace =
            resources::ResourcePath::Normalize(
                spaceName);

        if (normalizedSpace.empty())
        {
            error =
                "Unable to normalize space name.";

            return false;
        }

        const std::string prefix =
            "res/spaces/" +
            normalizedSpace +
            "/";

        std::vector<
            const resources::ResourceEntry*>
            chunks =
                resources.FindByType(
                    resources::ResourceType::Chunk);

        std::sort(
            chunks.begin(),
            chunks.end(),
            [](
                const resources::ResourceEntry* left,
                const resources::ResourceEntry* right)
            {
                return
                    left->logicalPath <
                    right->logicalPath;
            });

        ChunkLoader chunkLoader;

        std::size_t failedChunks = 0;

        std::size_t missingSpeedTrees =
            0;

        std::unordered_set<std::string>
            uniqueSpeedTreeResources;

        std::unordered_set<std::string>
            missingSpeedTreeResources;

        std::unordered_map<
            std::string,
            std::size_t>
            largeObjectLookup;

        std::size_t invalidLargeObjectReferences =
            0;

        for (const resources::ResourceEntry* entry :
             chunks)
        {
            if (entry == nullptr)
            {
                continue;
            }

            const std::string normalizedPath =
                resources::ResourcePath::Normalize(
                    entry->logicalPath);

            if (!normalizedPath.starts_with(
                    prefix))
            {
                continue;
            }

            const std::filesystem::path path(
                normalizedPath);

            const std::string chunkId =
                path.stem().string();

            if (chunkId.empty())
            {
                continue;
            }

            Chunk chunk;
            std::string chunkError;

            if (!chunkLoader.Load(
                    resources,
                    spaceName,
                    chunkId,
                    chunk,
                    chunkError))
            {
                ++failedChunks;

                core::Log::Warning(
                    std::string(
                        "Unable to load chunk ") +
                    chunkId +
                    ": " +
                    chunkError);

                continue;
            }

            ++scene.chunkCount;

            math::Transform3x4 chunkTransform =
                math::Transform3x4::Identity();

            math::Transform3x4 outdoorTransform;

            if (chunk.transform.has_value())
            {
                chunkTransform =
                    *chunk.transform;

                ++scene.indoorChunkCount;
            }
            else if (BuildOutdoorChunkTransform(
                         chunkId,
                         outdoorTransform))
            {
                chunkTransform =
                    outdoorTransform;

                ++scene.outdoorChunkCount;
            }
            else
            {
                ++scene.indoorChunkCount;
            }

            scene.modelInstances.reserve(
                scene.modelInstances.size() +
                chunk.models.size() +
                chunk.shells.size());

            for (const ChunkModelInstance& model :
                 chunk.models)
            {
                AddModelInstance(
                    chunkId,
                    model,
                    chunkTransform,
                    false,
                    scene);
            }

            for (const ChunkModelInstance& shell :
                 chunk.shells)
            {
                AddModelInstance(
                    chunkId,
                    shell,
                    chunkTransform,
                    true,
                    scene);
            }

            scene.speedTreeInstances.reserve(
                scene.speedTreeInstances.size() +
                chunk.speedTrees.size());

            for (const ChunkSpeedTreeInstance& tree :
                 chunk.speedTrees)
            {
                std::string missingResource;

                if (!AddSpeedTreeInstance(
                        resources,
                        chunkId,
                        tree,
                        chunkTransform,
                        scene,
                        missingResource))
                {
                    ++missingSpeedTrees;

                    if (!missingResource.empty())
                    {
                        missingSpeedTreeResources.insert(
                            missingResource);
                    }

                    continue;
                }

                const WorldSpeedTreeInstance& instance =
                    scene.speedTreeInstances.back();

                uniqueSpeedTreeResources.insert(
                    instance.sptLogicalPath);
            }

            scene.omniLights.reserve(
                scene.omniLights.size() +
                chunk.omniLights.size());

            for (const ChunkOmniLight& light :
                 chunk.omniLights)
            {
                AddOmniLightInstance(
                    chunkId,
                    light,
                    chunkTransform,
                    scene);
            }

            scene.spotLights.reserve(
                scene.spotLights.size() +
                chunk.spotLights.size());

            for (const ChunkSpotLight& light :
                 chunk.spotLights)
            {
                AddSpotLightInstance(
                    chunkId,
                    light,
                    chunkTransform,
                    scene);
            }

            scene.pulseLights.reserve(
                scene.pulseLights.size() +
                chunk.pulseLights.size());

            for (const ChunkPulseLight& light :
                 chunk.pulseLights)
            {
                AddPulseLightInstance(
                    chunkId,
                    light,
                    chunkTransform,
                    scene);
            }

            for (const ChunkTerrainReference& terrain :
                 chunk.terrains)
            {
                WorldTerrainInstance instance;

                instance.chunkId =
                    chunkId;

                instance.resourceReference =
                    terrain.resource;

                if (!BuildTerrainCDataPath(
                        spaceName,
                        terrain.resource,
                        instance.cdataLogicalPath))
                {
                    core::Log::Warning(
                        std::string(
                            "Invalid terrain reference in chunk ") +
                        chunkId +
                        ": " +
                        terrain.resource);

                    continue;
                }

                if (!resources.Exists(
                        instance.cdataLogicalPath))
                {
                    core::Log::Warning(
                        std::string(
                            "Terrain cdata not found: ") +
                        instance.cdataLogicalPath);

                    continue;
                }

                instance.transform =
                    chunkTransform;

                scene.terrainInstances.push_back(
                    std::move(instance));

                ++scene.terrainReferenceCount;
            }

            for (const ChunkLargeObjectReference& largeObject :
                 chunk.largeObjects)
            {
                ++scene.largeObjectReferenceCount;

                std::string largeObjectError;

                if (!AddLargeObjectReference(
                        resources,
                        spaceName,
                        chunkId,
                        largeObject,
                        largeObjectLookup,
                        scene,
                        largeObjectError))
                {
                    ++invalidLargeObjectReferences;

                    core::Log::Warning(
                        std::string(
                            "Invalid VLO reference in chunk ") +
                        chunkId +
                        ": " +
                        largeObjectError);

                    continue;
                }
            }
        }

        scene.speedTreeInstanceCount =
            scene.speedTreeInstances.size();

        scene.omniLightCount =
            scene.omniLights.size();

        scene.spotLightCount =
            scene.spotLights.size();

        scene.pulseLightCount =
            scene.pulseLights.size();

        scene.missingLargeObjectCount =
            0;

        scene.loadedLargeObjectCount =
            0;

        scene.failedLargeObjectLoadCount =
            0;

        std::unordered_map<
            std::string,
            std::size_t>
            largeObjectTypes;

        vlo::VloLoader
            vloLoader;

        for (WorldLargeObjectReference& object :
             scene.largeObjects)
        {
            const std::string normalizedType =
                resources::ResourcePath::Normalize(
                    object.type);

            ++largeObjectTypes[
                normalizedType];

            if (!object.vloExists)
            {
                ++scene.missingLargeObjectCount;

                continue;
            }

            vlo::VloResource
                resource;

            std::string
                vloError;

            if (!vloLoader.Load(
                    resources,
                    object.vloLogicalPath,
                    object.type,
                    resource,
                    vloError))
            {
                ++scene.failedLargeObjectLoadCount;

                core::Log::Warning(
                    std::string(
                        "Unable to load VLO ") +
                    object.uid +
                    ": " +
                    vloError);

                continue;
            }

            object.vloResource =
                std::move(
                    resource);

            object.vloLoaded =
                true;

            ++scene.loadedLargeObjectCount;

            if (object.vloResource.type ==
                    vlo::VloType::Water &&
                object.vloResource.water.has_value())
            {
                const water::WaterDefinition& water =
                    *object.vloResource.water;

                core::Log::Info(
                    std::string(
                        "Water VLO loaded: uid=") +
                    object.uid +
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
                        water.size.y) +
                    ", " +
                    std::to_string(
                        water.size.z) +
                    "), wave=" +
                    water.waveTexture.logicalPath +
                    ", foam=" +
                    water.foamTexture.logicalPath +
                    ", reflection=" +
                    water.reflectionTexture.logicalPath);

                if (!water.waveTexture.exists)
                {
                    core::Log::Warning(
                        std::string(
                            "Water wave texture not found: ") +
                        water.waveTexture.logicalPath);
                }

                if (!water.foamTexture.exists)
                {
                    core::Log::Warning(
                        std::string(
                            "Water foam texture not found: ") +
                        water.foamTexture.logicalPath);
                }

                if (!water.reflectionTexture.exists)
                {
                    core::Log::Warning(
                        std::string(
                            "Water reflection texture not found: ") +
                        water.reflectionTexture.logicalPath);
                }
            }
        }

        if (scene.chunkCount == 0)
        {
            error =
                "No chunks were loaded for space: " +
                std::string(spaceName);

            return false;
        }

        core::Log::Info(
            std::string("World chunks loaded: ") +
            std::to_string(
                scene.chunkCount));

        core::Log::Info(
            std::string("Outdoor chunks: ") +
            std::to_string(
                scene.outdoorChunkCount));

        core::Log::Info(
            std::string("Indoor chunks: ") +
            std::to_string(
                scene.indoorChunkCount));

        core::Log::Info(
            std::string("World model instances: ") +
            std::to_string(
                scene.modelInstances.size()));

        core::Log::Info(
            std::string(
                "SpeedTree instances: ") +
            std::to_string(
                scene.speedTreeInstances.size()));

        core::Log::Info(
            std::string(
                "OmniLight instances: ") +
            std::to_string(
                scene.omniLights.size()));

        core::Log::Info(
            std::string(
                "SpotLight instances: ") +
            std::to_string(
                scene.spotLights.size()));

        core::Log::Info(
            std::string(
                "PulseLight instances: ") +
            std::to_string(
                scene.pulseLights.size()));

        std::size_t pulseFrameCount =
            0;

        for (const WorldPulseLightInstance& light :
             scene.pulseLights)
        {
            pulseFrameCount +=
                light.frames.size();
        }

        core::Log::Info(
            std::string(
                "PulseLight animation frames: ") +
            std::to_string(
                pulseFrameCount));

        core::Log::Info(
            std::string(
                "Unique SpeedTree resources: ") +
            std::to_string(
                uniqueSpeedTreeResources.size()));

        core::Log::Info(
            std::string(
                "Missing SpeedTree instances: ") +
            std::to_string(
                missingSpeedTrees));

        core::Log::Info(
            std::string(
                "Missing unique SpeedTree resources: ") +
            std::to_string(
                missingSpeedTreeResources.size()));

        core::Log::Info(
            std::string("Terrain instances: ") +
            std::to_string(
                scene.terrainInstances.size()));

        core::Log::Info(
            std::string(
                "VLO chunk references: ") +
            std::to_string(
                scene.largeObjectReferenceCount));

        core::Log::Info(
            std::string(
                "Unique VLO objects: ") +
            std::to_string(
                scene.largeObjects.size()));

        core::Log::Info(
            std::string(
                "Missing VLO resources: ") +
            std::to_string(
                scene.missingLargeObjectCount));

        core::Log::Info(
            std::string(
                "Loaded VLO resources: ") +
            std::to_string(
                scene.loadedLargeObjectCount));

        core::Log::Info(
            std::string(
                "Failed VLO loads: ") +
            std::to_string(
                scene.failedLargeObjectLoadCount));

        core::Log::Info(
            std::string(
                "Invalid VLO references: ") +
            std::to_string(
                invalidLargeObjectReferences));

        for (const auto& [type, count] :
             largeObjectTypes)
        {
            core::Log::Info(
                std::string(
                    "VLO type [") +
                type +
                "]: " +
                std::to_string(
                    count));
        }

        for (const WorldLargeObjectReference& object :
             scene.largeObjects)
        {
            core::Log::Info(
                std::string(
                    "VLO resource: uid=") +
                object.uid +
                ", type=" +
                object.type +
                ", chunks=" +
                std::to_string(
                    object.chunkIds.size()) +
                ", vlo=" +
                object.vloLogicalPath +
                ", odata=" +
                object.odataLogicalPath);

            if (!object.vloExists)
            {
                core::Log::Warning(
                    std::string(
                        "Missing VLO file: ") +
                    object.vloLogicalPath);
            }

            if (!object.odataExists)
            {
                core::Log::Warning(
                    std::string(
                        "Missing VLO odata: ") +
                    object.odataLogicalPath);
            }
        }

        for (const std::string& resource :
             uniqueSpeedTreeResources)
        {
            core::Log::Info(
                std::string(
                    "SpeedTree resource: ") +
                resource);
        }

        for (const std::string& resource :
             missingSpeedTreeResources)
        {
            core::Log::Warning(
                std::string(
                    "Missing SpeedTree resource: ") +
                resource);
        }

        if (failedChunks != 0)
        {
            core::Log::Warning(
                std::string("Failed chunks: ") +
                std::to_string(
                    failedChunks));
        }

        output =
            std::move(scene);

        return true;
    }
}