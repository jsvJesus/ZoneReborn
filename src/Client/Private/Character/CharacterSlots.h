#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace client::character
{
    enum class Slot : std::int32_t
    {
        Head = 101,
        Body = 102,
        Legs = 103,
        Hands = 104,

        Helmet = 105,
        Eyes = 106,
        Mouth = 107,
        Ears = 108,

        UnderwearLegs = 109,
        UnderwearBody = 110,

        UnderPants = 111,
        UnderShirt = 112,
        UnderFoot = 113,
        MiddleShirt = 114,

        Hat = 115,
        CombinedHat = 116,

        Jacket = 117,
        CombinedBody = 118,

        Pants = 119,
        CombinedPants = 120,

        Gloves = 121,
        CombinedGloves = 122,

        Shoes = 123,
        CombinedShoes = 124,

        Vest = 125,
        Armor = 126,
        Backpack = 127,

        Feet = 128,
        Arms = 129,

        Hair = 130,
        Moustache = 131,
        Beard = 132,

        Belt = 133
    };

    enum class VisualMode : std::uint8_t
    {
        Hood = 0,
        Visor = 1,
        Headlight = 2,
        Tuck = 3,
        GasmaskFilterOff = 4
    };

    constexpr std::int32_t FirstSlotValue = 101;
    constexpr std::int32_t LastSlotValue = 133;
    constexpr std::size_t SlotCount =
        static_cast<std::size_t>(
            LastSlotValue -
            FirstSlotValue +
            1);

    [[nodiscard]]
    constexpr bool IsValidSlot(
        const std::int32_t value) noexcept
    {
        return
            value >= FirstSlotValue &&
            value <= LastSlotValue;
    }

    [[nodiscard]]
    constexpr std::size_t SlotIndex(
        const Slot slot) noexcept
    {
        return static_cast<std::size_t>(
            static_cast<std::int32_t>(slot) -
            FirstSlotValue);
    }

    [[nodiscard]]
    constexpr std::optional<Slot> ToSlot(
        const std::int32_t value) noexcept
    {
        if (!IsValidSlot(value))
        {
            return std::nullopt;
        }

        return static_cast<Slot>(value);
    }

    [[nodiscard]]
    constexpr std::string_view SlotName(
        const Slot slot) noexcept
    {
        switch (slot)
        {
            case Slot::Head:
                return "head";

            case Slot::Body:
                return "body";

            case Slot::Legs:
                return "legs";

            case Slot::Hands:
                return "hands";

            case Slot::Helmet:
                return "helmet";

            case Slot::Eyes:
                return "eyes";

            case Slot::Mouth:
                return "mouth";

            case Slot::Ears:
                return "ears";

            case Slot::UnderwearLegs:
                return "uwlegs";

            case Slot::UnderwearBody:
                return "uwbody";

            case Slot::UnderPants:
                return "upants";

            case Slot::UnderShirt:
                return "ushirt";

            case Slot::UnderFoot:
                return "uwfoot";

            case Slot::MiddleShirt:
                return "mshirt";

            case Slot::Hat:
                return "hat";

            case Slot::CombinedHat:
                return "chat";

            case Slot::Jacket:
                return "jacket";

            case Slot::CombinedBody:
                return "cbody";

            case Slot::Pants:
                return "pants";

            case Slot::CombinedPants:
                return "cpants";

            case Slot::Gloves:
                return "gloves";

            case Slot::CombinedGloves:
                return "cgloves";

            case Slot::Shoes:
                return "shoes";

            case Slot::CombinedShoes:
                return "cshoes";

            case Slot::Vest:
                return "vestmed";

            case Slot::Armor:
                return "armor";

            case Slot::Backpack:
                return "backpackmed";

            case Slot::Feet:
                return "feet";

            case Slot::Arms:
                return "arms";

            case Slot::Hair:
                return "hairs";

            case Slot::Moustache:
                return "moustache";

            case Slot::Beard:
                return "beard";

            case Slot::Belt:
                return "belt";
        }

        return {};
    }
}