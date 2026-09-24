#pragma once

#include <cstdint>
#include <vector>

namespace client::character
{
    struct FaceState final
    {
        std::int32_t hairStyle = 0;
        std::int32_t moustacheStyle = 0;
        std::int32_t beardStyle = 0;
        std::uint8_t hairLength = 0;
        std::uint8_t beardLength = 0;
        std::uint8_t moustacheLength = 0;
        std::uint8_t age = 0;
        std::uint8_t details = 0;
        std::uint8_t unshaven = 0;
        std::uint8_t eyebrowPosition = 0;
        std::uint8_t eyebrowRotation = 0;
        std::uint32_t hairColor = 0;
        std::uint32_t skinColor = 0;
        std::uint32_t eyeColor = 0;
        std::uint32_t tattooColor = 0;
        std::int32_t eyebrowStyle = 0;
        std::int32_t tattooStyle = 0;
        std::vector<std::uint64_t> faceForm;
    };
}
