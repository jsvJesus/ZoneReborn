#include "Character/CharacterState.h"
#include "Character/CharacterFaceCodec.h"

#include <algorithm>
#include <limits>
#include <unordered_set>

namespace
{
    std::uint8_t PercentageToByte(
        const std::uint64_t value) noexcept
    {
        return static_cast<std::uint8_t>(
            (value * 255u + 50u) /
            100u);
    }
}

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

        if (!ResetFace(
                catalog,
                error))
        {
            return false;
        }

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
        State backup = *this;
        const FaceState preservedFace = face_;
        const bool preserveFace = !preservedFace.faceForm.empty();

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
            *this = std::move(backup);
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
                *this = std::move(backup);
                return false;
            }
        }

        hiddenSlots_.clear();

        if (preserveFace && !ApplyFaceState(catalog, preservedFace, error))
        {
            *this = std::move(backup);
            return false;
        }

        return true;
    }

    bool State::ResetFace(
        const Catalog& catalog,
        std::string& error)
    {
        FaceState defaults;
        defaults.eyebrowStyle = 1;
        defaults.skinColor = 0xE4E4E4u;
        defaults.eyeColor = 0xBAC7C9u;
        defaults.hairColor = 0xE1E0E0u;
        std::mt19937 defaultFace(0x5A17u);
        defaults.faceForm = GenerateRandomFaceForm(defaultFace);

        return ApplyFaceState(
            catalog,
            defaults,
            error);
    }

    bool State::ApplyFaceValue(
        const Catalog& catalog,
        const std::string_view group,
        const std::uint64_t value,
        bool& modelChanged,
        std::string& error)
    {
        error.clear();
        modelChanged = false;

        const auto applyStyle =
            [&](const Slot slot, std::int32_t& target) -> bool
            {
                if (value > static_cast<std::uint64_t>(
                        std::numeric_limits<std::int32_t>::max()))
                {
                    error = "Face style value is out of range.";
                    return false;
                }

                const std::int32_t itemType =
                    static_cast<std::int32_t>(value);

                if (!catalog.Faces().IsStyleAllowed(group, itemType))
                {
                    error = "Face style is not allowed for " +
                        std::string(group) + ": " +
                        std::to_string(itemType);
                    return false;
                }

                if (target == itemType)
                {
                    return true;
                }

                const std::int64_t previous = Get(slot).instanceId;

                if (previous != 0)
                {
                    UnequipInstance(previous);
                }

                if (itemType != 0 && !Equip(catalog, itemType, error))
                {
                    return false;
                }

                target = itemType;
                modelChanged = true;
                return true;
            };

        if (group == "HairStyle")
        {
            return applyStyle(Slot::Hair, face_.hairStyle);
        }
        if (group == "MustacheStyle")
        {
            return applyStyle(Slot::Moustache, face_.moustacheStyle);
        }
        if (group == "BeardStyle")
        {
            return applyStyle(Slot::Beard, face_.beardStyle);
        }

        if (group == "EyebrowsStyle" || group == "TatooStyle")
        {
            if (value > static_cast<std::uint64_t>(
                    std::numeric_limits<std::int32_t>::max()) ||
                !catalog.Faces().IsStyleAllowed(
                    group,
                    static_cast<std::int32_t>(value)))
            {
                error = "Face detail style is not allowed for " +
                    std::string(group) + ".";
                return false;
            }

            if (group == "EyebrowsStyle")
            {
                face_.eyebrowStyle = static_cast<std::int32_t>(value);
            }
            else
            {
                face_.tattooStyle = static_cast<std::int32_t>(value);
            }

            return true;
        }

        if (group == "SkinColor" || group == "EyeColor" ||
            group == "HairColor" || group == "TatooColor")
        {
            if (value > std::numeric_limits<std::uint32_t>::max() ||
                !catalog.Faces().IsColourAllowed(
                    group,
                    static_cast<std::uint32_t>(value)))
            {
                error = "Face colour is not allowed for " +
                    std::string(group) + ".";
                return false;
            }

            const std::uint32_t colour = static_cast<std::uint32_t>(value);

            if (group == "SkinColor") face_.skinColor = colour;
            else if (group == "EyeColor") face_.eyeColor = colour;
            else if (group == "HairColor") face_.hairColor = colour;
            else face_.tattooColor = colour;

            return true;
        }

        if (value > 100u)
        {
            error = "Face slider value must be between 0 and 100.";
            return false;
        }

        const std::uint8_t scalar = PercentageToByte(value);

        if (group == "HairLength") face_.hairLength = scalar;
        else if (group == "BeardLength") face_.beardLength = scalar;
        else if (group == "MustacheLength") face_.moustacheLength = scalar;
        else if (group == "Age") face_.age = scalar;
        else if (group == "Details") face_.details = scalar;
        else if (group == "Unshaven") face_.unshaven = scalar;
        else if (group == "EyebrowsPosition") face_.eyebrowPosition = scalar;
        else if (group == "EyebrowsRotation") face_.eyebrowRotation = scalar;
        else
        {
            error = "Unknown face choice group: " + std::string(group);
            return false;
        }

        return true;
    }

    bool State::ApplyFaceState(
        const Catalog& catalog,
        const FaceState& face,
        std::string& error)
    {
        State backup = *this;
        bool modelChanged = false;

        const auto apply =
            [&](const std::string_view group,
                const std::uint64_t value) -> bool
            {
                bool changed = false;

                if (!ApplyFaceValue(
                        catalog,
                        group,
                        value,
                        changed,
                        error))
                {
                    return false;
                }

                modelChanged = modelChanged || changed;
                return true;
            };

        if (!apply("HairStyle", face.hairStyle) ||
            !apply("MustacheStyle", face.moustacheStyle) ||
            !apply("BeardStyle", face.beardStyle) ||
            !apply("EyebrowsStyle", face.eyebrowStyle) ||
            !apply("TatooStyle", face.tattooStyle) ||
            !apply("SkinColor", face.skinColor) ||
            !apply("EyeColor", face.eyeColor) ||
            !apply("HairColor", face.hairColor) ||
            !apply("TatooColor", face.tattooColor))
        {
            *this = std::move(backup);
            return false;
        }

        face_.hairLength = face.hairLength;
        face_.beardLength = face.beardLength;
        face_.moustacheLength = face.moustacheLength;
        face_.age = face.age;
        face_.details = face.details;
        face_.unshaven = face.unshaven;
        face_.eyebrowPosition = face.eyebrowPosition;
        face_.eyebrowRotation = face.eyebrowRotation;
        face_.faceForm = face.faceForm;

        return true;
    }

    bool State::RandomizeFace(
        const Catalog& catalog,
        std::mt19937& random,
        std::string& error)
    {
        FaceState randomized = face_;
        randomized.hairStyle = static_cast<std::int32_t>(
            catalog.Faces().RandomWeighted("HairStyle", random));
        randomized.moustacheStyle = static_cast<std::int32_t>(
            catalog.Faces().RandomWeighted("MustacheStyle", random));
        randomized.beardStyle = static_cast<std::int32_t>(
            catalog.Faces().RandomWeighted("BeardStyle", random));
        randomized.eyebrowStyle = static_cast<std::int32_t>(
            catalog.Faces().RandomWeighted("EyebrowsStyle", random));
        randomized.skinColor =
            catalog.Faces().RandomWeighted("SkinColor", random);
        randomized.eyeColor =
            catalog.Faces().RandomWeighted("EyeColor", random);
        randomized.hairColor =
            catalog.Faces().RandomWeighted("HairColor", random);

        std::uniform_int_distribution<int> scalar(0, 255);
        randomized.hairLength = static_cast<std::uint8_t>(scalar(random));
        randomized.beardLength = static_cast<std::uint8_t>(scalar(random));
        randomized.moustacheLength = static_cast<std::uint8_t>(scalar(random));
        randomized.age = static_cast<std::uint8_t>(scalar(random));
        randomized.details = static_cast<std::uint8_t>(scalar(random));
        randomized.unshaven = static_cast<std::uint8_t>(scalar(random));
        randomized.eyebrowPosition = static_cast<std::uint8_t>(scalar(random));
        randomized.eyebrowRotation = static_cast<std::uint8_t>(scalar(random));
        randomized.faceForm = GenerateRandomFaceForm(random);

        return ApplyFaceState(catalog, randomized, error);
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
