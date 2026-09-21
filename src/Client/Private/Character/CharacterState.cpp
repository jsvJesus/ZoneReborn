#include "Character/CharacterState.h"

#include <unordered_set>

namespace client::character
{
    void State::Reset()
    {
        slots_ =
            {};

        hiddenSlots_.clear();

        face_ =
            {};

        visualModes_ =
            0;

        nextInstanceId_ =
            -1;
    }

    bool State::ResetCreator(
        const Catalog& catalog,
        std::string& error)
    {
        Reset();

        //
        // 2026 defaults from EquipSlotsConfigs:
        //
        // U_SHIRT  -> SHIRT_BELUGA
        // U_PANTS  -> PANTS_BELUGA
        // UW_LEGS  -> UNDERPANTS_1
        //
        constexpr std::int32_t ShirtBeluga =
            24030;

        constexpr std::int32_t PantsBeluga =
            24035;

        constexpr std::int32_t UnderPants =
            20510;

        if (!Equip(
                catalog,
                ShirtBeluga,
                error))
        {
            return false;
        }

        if (!Equip(
                catalog,
                PantsBeluga,
                error))
        {
            return false;
        }

        if (!Equip(
                catalog,
                UnderPants,
                error))
        {
            return false;
        }

        for (const CreatorGroup& group :
             catalog.CreatorGroups())
        {
            if (group.options.empty())
            {
                continue;
            }

            if (!Equip(
                    catalog,
                    group.options.front().
                        itemType,
                    error))
            {
                return false;
            }
        }

        hiddenSlots_.clear();

        return true;
    }

    bool State::Equip(
        const Catalog& catalog,
        const std::int32_t itemType,
        std::string& error)
    {
        error.clear();

        if (itemType == 0)
        {
            return true;
        }

        const ItemDefinition* definition =
            catalog.Find(
                itemType);

        if (definition ==
            nullptr)
        {
            error =
                "Unknown character item type: " +
                std::to_string(
                    itemType);

            return false;
        }

        if (definition->slots.empty())
        {
            error =
                "Character item has no SlotType: " +
                std::to_string(
                    itemType);

            return false;
        }

        RemoveConflictingItems(
            *definition);

        const std::int64_t instanceId =
            NextInstanceId();

        for (const Slot slot :
             definition->slots)
        {
            EquippedItem& equipped =
                Get(
                    slot);

            equipped.instanceId =
                instanceId;

            equipped.itemType =
                itemType;
        }

        return true;
    }

    void State::UnequipInstance(
        const std::int64_t instanceId)
    {
        if (instanceId == 0)
        {
            return;
        }

        for (EquippedItem& equipped :
             slots_)
        {
            if (equipped.instanceId ==
                instanceId)
            {
                equipped =
                    {};
            }
        }
    }

    void State::UnequipItemType(
        const std::int32_t itemType)
    {
        for (EquippedItem& equipped :
             slots_)
        {
            if (equipped.itemType ==
                itemType)
            {
                equipped =
                    {};
            }
        }
    }

    bool State::ApplyCreatorSelection(
        const Catalog& catalog,
        const std::string_view group,
        const std::int32_t itemType,
        std::string& error)
    {
        error.clear();

        const CreatorGroup* creatorGroup =
            catalog.FindCreatorGroup(
                group);

        if (creatorGroup ==
            nullptr)
        {
            error =
                "Unknown character creator group: " +
                std::string(
                    group);

            return false;
        }

        bool allowed =
            false;

        for (const CreatorOption& option :
             creatorGroup->options)
        {
            if (option.itemType ==
                itemType)
            {
                allowed =
                    true;

                break;
            }
        }

        if (!allowed)
        {
            error =
                "Item is not valid for creator group " +
                std::string(group) +
                ": " +
                std::to_string(
                    itemType);

            return false;
        }

        hiddenSlots_.clear();

        if (group.find(
                "under-pants") !=
            std::string_view::npos)
        {
            hiddenSlots_.insert(
                Slot::Pants);
        }
        else if (
            group.find(
                "under-shirt") !=
            std::string_view::npos)
        {
            hiddenSlots_.insert(
                Slot::Jacket);
        }

        return Equip(
            catalog,
            itemType,
            error);
    }

    bool State::ApplyCreatorSet(
        const Catalog& catalog,
        const std::vector<
            std::pair<
                std::string,
                std::int32_t>>& values,
        std::string& error)
    {
        Reset();

        constexpr std::int32_t ShirtBeluga =
            24030;

        constexpr std::int32_t PantsBeluga =
            24035;

        constexpr std::int32_t UnderPants =
            20510;

        if (!Equip(
                catalog,
                ShirtBeluga,
                error) ||
            !Equip(
                catalog,
                PantsBeluga,
                error) ||
            !Equip(
                catalog,
                UnderPants,
                error))
        {
            return false;
        }

        for (const auto& [group, itemType] :
             values)
        {
            const CreatorGroup* creatorGroup =
                catalog.FindCreatorGroup(
                    group);

            if (creatorGroup ==
                nullptr)
            {
                continue;
            }

            bool allowed =
                false;

            for (const CreatorOption& option :
                 creatorGroup->options)
            {
                if (option.itemType ==
                    itemType)
                {
                    allowed =
                        true;

                    break;
                }
            }

            if (!allowed)
            {
                continue;
            }

            if (!Equip(
                    catalog,
                    itemType,
                    error))
            {
                return false;
            }
        }

        hiddenSlots_.clear();

        return true;
    }

    void State::ClearPreviewMask()
    {
        hiddenSlots_.clear();
    }

    const EquippedItem& State::Get(
        const Slot slot) const noexcept
    {
        return
            slots_[
                SlotIndex(
                    slot)];
    }

    EquippedItem& State::Get(
        const Slot slot) noexcept
    {
        return
            slots_[
                SlotIndex(
                    slot)];
    }

    const std::array<
        EquippedItem,
        SlotCount>&
    State::Slots() const noexcept
    {
        return slots_;
    }

    const std::unordered_set<Slot>&
    State::HiddenSlots() const noexcept
    {
        return hiddenSlots_;
    }

    std::uint32_t State::VisualModes() const noexcept
    {
        return visualModes_;
    }

    void State::SetVisualMode(
        const VisualMode mode,
        const bool enabled) noexcept
    {
        const std::uint32_t mask =
            1u <<
            static_cast<std::uint32_t>(
                mode);

        if (enabled)
        {
            visualModes_ |=
                mask;
        }
        else
        {
            visualModes_ &=
                ~mask;
        }
    }

    FaceState& State::Face() noexcept
    {
        return face_;
    }

    const FaceState& State::Face() const noexcept
    {
        return face_;
    }

    std::int64_t State::NextInstanceId() noexcept
    {
        return
            nextInstanceId_--;
    }

    void State::RemoveConflictingItems(
        const ItemDefinition& definition)
    {
        std::unordered_set<std::int64_t>
            remove;

        for (const Slot slot :
             definition.slots)
        {
            const EquippedItem& current =
                Get(
                    slot);

            if (current.instanceId != 0)
            {
                remove.insert(
                    current.instanceId);
            }
        }

        for (const std::int64_t instanceId :
             remove)
        {
            UnequipInstance(
                instanceId);
        }
    }
}