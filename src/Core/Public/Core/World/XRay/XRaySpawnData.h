#pragma once

#include "Core/Math/Vector3.h"
#include "Core/World/XRay/XRayLightData.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace core::world::xray
{
    enum class SpawnStateDecoding : std::uint8_t { Opaque, Partial, Complete };

    struct SpawnObject final
    {
        std::uint32_t chunkId = 0;
        std::string section;
        std::string name;
        math::Vector3 position{};
        math::Vector3 rotation{}; // X-Ray Euler angles, radians.
        std::uint8_t gameId = 0;
        std::uint8_t respawnPoint = 0;
        std::uint16_t respawnTime = 0;
        std::uint16_t id = 0xffffu;
        std::uint16_t parentId = 0xffffu;
        std::uint16_t phantomId = 0xffffu;
        std::uint16_t flags = 0;
        std::uint16_t version = 0;
        std::uint16_t gameTypeMask = 0xffffu;
        std::uint16_t scriptVersion = 0;
        std::uint16_t spawnId = 0xffffu;
        std::size_t clientDataOffset = 0;
        std::size_t clientDataSize = 0;
        std::size_t stateOffset = 0;
        std::size_t stateSize = 0;
        std::size_t decodedStateBytes = 0;
        SpawnStateDecoding stateDecoding = SpawnStateDecoding::Opaque;
        std::string visual;
        std::optional<SpawnLamp> lamp;
        std::string customData; // Embedded ALife ini text, never executed.
        std::vector<std::pair<std::string, std::string>> parameters;
        // Original bytes, including unknown STATE and trailing update bytes.
        std::vector<std::byte> packet;
    };

    struct SpawnData final
    {
        std::uint64_t fileBytes = 0;
        std::size_t sectionCount = 0;
        std::size_t decodedStates = 0;
        std::size_t partialStates = 0;
        std::vector<SpawnObject> objects;
    };
}
