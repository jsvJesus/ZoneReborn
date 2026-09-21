#pragma once

#include "Character/CharacterCatalog.h"
#include "Character/CharacterSlots.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace client::character
{
    struct EquippedItem final
    {
        std::int64_t instanceId =
            0;

        std::int32_t itemType =
            0;

        [[nodiscard]]
        bool Empty() const noexcept
        {
            return
                itemType == 0;
        }
    };

    struct FaceState final
    {
        std::uint8_t hairLength =
            0;

        std::uint8_t beardLength =
            0;

        std::uint8_t moustacheLength =
            0;

        std::uint8_t age =
            0;

        std::uint8_t details =
            0;

        std::uint8_t unshaven =
            0;

        std::uint8_t eyebrowPosition =
            0;

        std::uint8_t eyebrowRotation =
            0;

        std::uint32_t hairColor =
            0;

        std::uint32_t skinColor =
            0;

        std::uint32_t eyeColor =
            0;

        std::uint32_t tattooColor =
            0;

        std::int32_t eyebrowStyle =
            0;

        std::int32_t tattooStyle =
            0;

        std::vector<std::int16_t>
            faceForm;
    };

    class State final
    {
    public:
        void Reset();

        [[nodiscard]]
        bool ResetCreator(
            const Catalog& catalog,
            std::string& error);

        [[nodiscard]]
        bool Equip(
            const Catalog& catalog,
            std::int32_t itemType,
            std::string& error);

        void UnequipInstance(
            std::int64_t instanceId);

        void UnequipItemType(
            std::int32_t itemType);

        [[nodiscard]]
        bool ApplyCreatorSelection(
            const Catalog& catalog,
            std::string_view group,
            std::int32_t itemType,
            std::string& error);

        [[nodiscard]]
        bool ApplyCreatorSet(
            const Catalog& catalog,
            const std::vector<
                std::pair<
                    std::string,
                    std::int32_t>>& values,
            std::string& error);

        void ClearPreviewMask();

        [[nodiscard]]
        const EquippedItem& Get(
            Slot slot) const noexcept;

        [[nodiscard]]
        EquippedItem& Get(
            Slot slot) noexcept;

        [[nodiscard]]
        const std::array<
            EquippedItem,
            SlotCount>&
        Slots() const noexcept;

        [[nodiscard]]
        const std::unordered_set<Slot>&
        HiddenSlots() const noexcept;

        [[nodiscard]]
        std::uint32_t VisualModes() const noexcept;

        void SetVisualMode(
            VisualMode mode,
            bool enabled) noexcept;

        [[nodiscard]]
        FaceState& Face() noexcept;

        [[nodiscard]]
        const FaceState& Face() const noexcept;

    private:
        [[nodiscard]]
        std::int64_t NextInstanceId() noexcept;

        void RemoveConflictingItems(
            const ItemDefinition& definition);

        std::array<
            EquippedItem,
            SlotCount>
            slots_{};

        std::unordered_set<Slot>
            hiddenSlots_;

        FaceState
            face_;

        std::uint32_t visualModes_ =
            0;

        std::int64_t nextInstanceId_ =
            -1;
    };
}