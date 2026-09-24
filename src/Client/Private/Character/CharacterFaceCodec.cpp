#include "Character/CharacterFaceCodec.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    struct Limit final
    {
        float minimum = 0.0f;
        float maximum = 0.0f;
        bool active = false;
    };

    struct AxisSpec final
    {
        std::size_t axis = 0;
        float minimum = 0.0f;
        float maximum = 0.0f;
    };

    struct HandleConfig final
    {
        std::string_view bone;
        std::string_view pairedBone;
        std::array<Limit, 3> translation{};
        std::array<Limit, 3> scale{};
    };

    void AddHandle(
        std::vector<HandleConfig>& output,
        const std::string_view bone,
        const std::string_view pairedBone,
        const std::initializer_list<AxisSpec> translations,
        const std::initializer_list<AxisSpec> scales = {})
    {
        HandleConfig value;
        value.bone = bone;
        value.pairedBone = pairedBone;

        for (const AxisSpec& axis : translations)
        {
            value.translation[axis.axis] =
                {axis.minimum, axis.maximum, true};
        }

        for (const AxisSpec& axis : scales)
        {
            value.scale[axis.axis] =
                {axis.minimum, axis.maximum, true};
        }

        output.push_back(value);
    }

    const std::vector<HandleConfig>& Config()
    {
        static const std::vector<HandleConfig> Result = []
        {
            std::vector<HandleConfig> values;
            AddHandle(values, "Jaw", "", {{1, -0.0025f, 0.0025f}, {2, -0.003f, 0.003f}});
            AddHandle(values, "Nose", "", {{1, -0.005f, 0.00359465f}, {2, -0.00778484f, 0.00597017f}});
            AddHandle(values, "Nose_Center", "", {{0, -0.011f, 0.008f}});
            AddHandle(values, "Nose_Left", "Nose_Right", {{0, -0.003f, 0.003f}, {2, -0.004f, 0.0025f}});
            AddHandle(values, "Forehead", "", {{0, 0.0f, 0.004f}, {2, -0.0065f, 0.00734684f}});
            AddHandle(values, "Eye_Base_Left", "Eye_Base_Right", {{1, -0.002f, 0.002f}, {2, 0.0f, 0.002f}});
            AddHandle(values, "Eyelid_Top_Right", "Eyelid_Top_Left", {{0, -0.002f, 0.001f}});
            AddHandle(values, "Eyelid_Bottom_Left", "Eyelid_Bottom_Right", {{0, -0.0015f, 0.00204038f}});
            AddHandle(values, "Brow_Left", "Brow_Right", {{0, 0.0f, 0.0045f}, {1, -0.0035f, 0.0002f}, {2, -0.003f, 0.00521977f}});
            AddHandle(values, "CheekBones_Top_Left", "CheekBones_Top_Right", {{0, -0.002f, 0.006f}, {1, -0.004f, 0.004f}, {2, -0.006f, 0.0f}});
            AddHandle(values, "Cheekbones_Front_Right", "Cheekbones_Front_Left", {{0, -0.00629366f, 0.00214155f}, {1, -0.00253781f, 0.00422892f}, {2, -0.00190786f, 0.00432085f}});
            AddHandle(values, "Lip_Right", "Lip_Left", {{0, -0.003f, 0.003f}, {2, -0.004f, 0.0035f}});
            AddHandle(values, "Cheek_Left", "Cheek_Right", {{0, -0.008f, 0.009f}});
            AddHandle(values, "Lip_Top_Center", "", {}, {{2, 0.5f, 1.7f}});
            AddHandle(values, "Lip_Bottom_Center", "", {}, {{2, 0.5f, 1.5f}});
            AddHandle(values, "Chin", "", {{0, -0.005f, 0.005f}, {2, -0.005f, 0.005f}});
            AddHandle(values, "CheekBones_Bottom_Left", "CheekBones_Bottom_Right", {{0, -0.008f, 0.0055f}, {1, -0.007f, 0.01f}, {2, -0.009f, 0.008f}});
            AddHandle(values, "Lip_Bottom_Right", "Lip_Bottom_Left", {}, {{2, 0.5f, 2.2f}});
            AddHandle(values, "Chin_Bottom", "", {{2, -0.015f, 0.01f}});
            AddHandle(values, "Cheek_Bottom_Left", "Cheek_Bottom_Right", {{0, -0.004f, 0.008f}});
            AddHandle(values, "Eye_Left_OutAngle", "Eye_Right_OutAngle", {{2, -0.003f, 0.003f}});
            AddHandle(values, "Eye_Left_InAngle", "Eye_Right_InAngle", {{2, -0.003f, 0.003f}});
            return values;
        }();

        return Result;
    }

    std::uint8_t Quantize(
        const float value,
        const Limit& limit)
    {
        const float normalized = std::clamp(
            (value - limit.minimum) /
                (limit.maximum - limit.minimum),
            0.0f,
            1.0f);

        return static_cast<std::uint8_t>(
            normalized * 255.0f);
    }

    float Dequantize(
        const std::uint8_t value,
        const Limit& limit)
    {
        return limit.minimum +
            (static_cast<float>(value) / 255.0f) *
                (limit.maximum - limit.minimum);
    }

    std::vector<std::uint64_t> PackBytes(
        const std::vector<std::uint8_t>& bytes)
    {
        std::vector<std::uint64_t> result(
            (bytes.size() + 7u) / 8u,
            0);

        for (std::size_t offset = 0; offset < bytes.size(); ++offset)
        {
            const std::uint8_t value =
                bytes[bytes.size() - 1u - offset];
            result[offset / 8u] |=
                static_cast<std::uint64_t>(value) <<
                ((offset % 8u) * 8u);
        }

        return result;
    }

    bool UnpackBytes(
        const std::span<const std::uint64_t> packed,
        std::vector<std::uint8_t>& output)
    {
        output.clear();

        if (packed.empty() || packed.size() > 256u)
        {
            return false;
        }

        bool started = false;

        for (std::size_t wordOffset = 0;
             wordOffset < packed.size();
             ++wordOffset)
        {
            const std::uint64_t word =
                packed[packed.size() - 1u - wordOffset];

            for (int byte = 7; byte >= 0; --byte)
            {
                const std::uint8_t value =
                    static_cast<std::uint8_t>(
                        (word >> (byte * 8)) & 0xFFu);

                if (!started && value == 0)
                {
                    continue;
                }

                started = true;
                output.push_back(value);
            }
        }

        return !output.empty() && output.front() == 1u;
    }

    std::size_t ValueCount()
    {
        std::size_t result = 0;

        for (const HandleConfig& handle : Config())
        {
            for (const Limit& value : handle.translation)
            {
                result += value.active ? 1u : 0u;
            }

            for (const Limit& value : handle.scale)
            {
                result += value.active ? 1u : 0u;
            }
        }

        return result;
    }

    float RandomValue(
        const Limit& limit,
        std::mt19937& random)
    {
        constexpr float FaceFactor = 0.9f;
        constexpr float Charisma = 0.6f;
        const float middle =
            (limit.minimum + limit.maximum) * 0.5f;
        const float narrowedMinimum =
            middle - (middle - limit.minimum) * FaceFactor;
        const float narrowedMaximum =
            middle + (limit.maximum - middle) * FaceFactor;
        const float half =
            (narrowedMaximum - narrowedMinimum) * 0.5f * Charisma;
        std::uniform_real_distribution<float> distribution(
            middle - half,
            middle + half);

        return (distribution(random) + distribution(random)) * 0.5f;
    }
}

namespace client::character
{
    std::vector<std::uint64_t> GenerateRandomFaceForm(
        std::mt19937& random)
    {
        std::vector<std::uint8_t> bytes;
        bytes.reserve(ValueCount() + 1u);
        bytes.push_back(1u);

        for (const HandleConfig& handle : Config())
        {
            for (const Limit& value : handle.translation)
            {
                if (value.active)
                {
                    bytes.push_back(
                        Quantize(RandomValue(value, random), value));
                }
            }

            for (const Limit& value : handle.scale)
            {
                if (value.active)
                {
                    bytes.push_back(
                        Quantize(RandomValue(value, random), value));
                }
            }
        }

        return PackBytes(bytes);
    }

    bool DecodeFaceForm(
        const std::span<const std::uint64_t> packed,
        std::vector<FaceBoneTransform>& output,
        std::string& error)
    {
        output.clear();
        error.clear();

        std::vector<std::uint8_t> bytes;

        if (!UnpackBytes(packed, bytes) ||
            bytes.size() != ValueCount() + 1u)
        {
            error = "Packed face form has an invalid size or sentinel.";
            return false;
        }

        std::size_t cursor = 1;
        output.reserve(Config().size());

        for (const HandleConfig& handle : Config())
        {
            FaceBoneTransform decoded;
            decoded.bone = handle.bone;
            decoded.pairedBone = handle.pairedBone;

            for (std::size_t axis = 0; axis < 3; ++axis)
            {
                if (handle.translation[axis].active)
                {
                    decoded.translation[axis] =
                        Dequantize(bytes[cursor++], handle.translation[axis]);
                }
            }

            for (std::size_t axis = 0; axis < 3; ++axis)
            {
                if (handle.scale[axis].active)
                {
                    decoded.scale[axis] =
                        Dequantize(bytes[cursor++], handle.scale[axis]);
                }
            }

            output.push_back(std::move(decoded));
        }

        return true;
    }

    bool ValidateFaceForm(
        const std::span<const std::uint64_t> packed) noexcept
    {
        try
        {
            std::vector<FaceBoneTransform> decoded;
            std::string error;
            return DecodeFaceForm(packed, decoded, error);
        }
        catch (...)
        {
            return false;
        }
    }
}

