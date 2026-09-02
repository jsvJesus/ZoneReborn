#include "Core/World/WorldLoader.h"

#include "Core/Log.h"
#include "Core/Resources/ResourcePath.h"
#include "Core/Resources/ResourceType.h"
#include "Core/World/ChunkLoader.h"
#include "Core/World/SpaceLoader.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
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

            scene.speedTreeInstanceCount +=
                chunk.speedTrees.size();

            scene.terrainReferenceCount +=
                chunk.terrains.size();

            scene.largeObjectReferenceCount +=
                chunk.largeObjects.size();
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
            std::string("SpeedTree instances pending: ") +
            std::to_string(
                scene.speedTreeInstanceCount));

        core::Log::Info(
            std::string("Terrain references pending: ") +
            std::to_string(
                scene.terrainReferenceCount));

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