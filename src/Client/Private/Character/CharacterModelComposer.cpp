#include "Character/CharacterModelComposer.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace
{
    using Slot =
        client::character::Slot;

    using ItemDefinition =
        client::character::ItemDefinition;

    using ModelAlternative =
        client::character::ModelAlternative;

    using VisibleModel =
        client::character::VisibleModel;

    constexpr std::int32_t NpHead =
        11010;

    constexpr std::int32_t NpTorso =
        11020;

    constexpr std::int32_t NpHands =
        11025;

    constexpr std::int32_t NpLegs =
        11030;

    constexpr std::int32_t NpFeet =
        11035;

    bool IsHidden(
        const ItemDefinition& definition,
        const std::unordered_set<Slot>& hiddenSlots)
    {
        for (const Slot slot :
             definition.slots)
        {
            if (hiddenSlots.contains(
                    slot))
            {
                return true;
            }
        }

        return false;
    }

    void AddUnique(
        std::vector<VisibleModel>& output,
        const std::vector<std::string>& models,
        const std::string& tintMaterial,
        const std::uint32_t colour)
    {
        for (const std::string& model :
             models)
        {
            if (model.empty())
            {
                continue;
            }

            if (std::find_if(
                    output.begin(),
                    output.end(),
                    [&model](const VisibleModel& value)
                    {
                        return
                            value.reference ==
                                model;
                    }) ==
                output.end())
            {
                output.push_back(
                    {
                        model,
                        tintMaterial,
                        colour
                    });
            }
        }
    }
}

namespace client::character
{
    bool ModelComposer::Compose(
        const Catalog& catalog,
        const State& state,
        ModelPlan& output,
        std::string& error) const
    {
        error.clear();

        output =
            {};

        output.hiddenSlots =
            state.HiddenSlots();

        std::vector<EquippedItem>
            equippedItems;

        std::unordered_set<std::int64_t>
            seenInstances;

        for (const EquippedItem& equipped :
             state.Slots())
        {
            if (equipped.Empty() ||
                equipped.instanceId == 0)
            {
                continue;
            }

            if (!seenInstances.insert(
                    equipped.instanceId).
                    second)
            {
                continue;
            }

            equippedItems.push_back(
                equipped);
        }

        std::unordered_map<
            Slot,
            std::unordered_set<std::string>>
            slotTags;

        std::unordered_set<Slot>
            hardHiddenSlots;

        for (const EquippedItem& equipped :
             equippedItems)
        {
            const ItemDefinition* definition =
                catalog.Find(
                    equipped.itemType);

            if (definition ==
                nullptr)
            {
                error =
                    "Unable to compose unknown item: " +
                    std::to_string(
                        equipped.itemType);

                return false;
            }

            if (IsHidden(
                    *definition,
                    output.hiddenSlots))
            {
                continue;
            }

            for (const SlotOverride& overrideData :
                 definition->overrides)
            {
                if (overrideData.hide)
                {
                    hardHiddenSlots.insert(
                        overrideData.slot);
                }
                else if (
                    !overrideData.tag.empty())
                {
                    slotTags[
                        overrideData.slot].
                        insert(
                            overrideData.tag);
                }
            }
        }

        //
        // Hood is numeric mode 0 in the original data.
        //
        if ((
                state.VisualModes() &
                (
                    1u <<
                    static_cast<std::uint32_t>(
                        VisualMode::Hood)
                )
            ) !=
            0)
        {
            for (auto& [slot, tags] :
                 slotTags)
            {
                static_cast<void>(
                    slot);

                tags.insert(
                    "#0");
            }
        }

        std::vector<EquippedItem>
            modelItems =
                equippedItems;

        //
        // Base naked body.
        //
        modelItems.push_back(
            {0, NpTorso, 0xFFFFFFu});

        modelItems.push_back(
            {0, NpLegs, 0xFFFFFFu});

        modelItems.push_back(
            {0, NpFeet, 0xFFFFFFu});

        if (state.Get(
                Slot::Head).
                Empty())
        {
            modelItems.push_back(
                {0, NpHead, 0xFFFFFFu});
        }

        if (state.Get(
                Slot::Hands).
                Empty())
        {
            modelItems.push_back(
                {0, NpHands, 0xFFFFFFu});
        }

        std::unordered_set<std::int32_t>
            uniqueItems;

        for (const EquippedItem& equipped :
             modelItems)
        {
            if (!uniqueItems.insert(
                    equipped.itemType).
                    second)
            {
                continue;
            }

            const ItemDefinition* definition =
                catalog.Find(
                    equipped.itemType);

            if (definition ==
                nullptr)
            {
                error =
                    "Character model definition not found: " +
                    std::to_string(
                        equipped.itemType);

                return false;
            }

            if (IsHidden(
                    *definition,
                    output.hiddenSlots))
            {
                continue;
            }

            std::unordered_set<std::string>
                tags;

            bool hidden =
                false;

            for (const Slot slot :
                 definition->slots)
            {
                if (hardHiddenSlots.contains(
                        slot))
                {
                    hidden =
                        true;

                    break;
                }

                const auto found =
                    slotTags.find(
                        slot);

                if (found !=
                    slotTags.end())
                {
                    tags.insert(
                        found->second.begin(),
                        found->second.end());
                }
            }

            if (hidden)
            {
                output.hiddenSlots.insert(
                    definition->slots.begin(),
                    definition->slots.end());

                continue;
            }

            const std::vector<std::string>*
                models =
                    &definition->models;

            for (const ModelAlternative& alternative :
                 definition->alternatives)
            {
                if (!alternative.tag.empty() &&
                    tags.contains(
                        alternative.tag))
                {
                    models =
                        &alternative.models;

                    break;
                }
            }

            if (models->empty())
            {
                output.hiddenSlots.insert(
                    definition->slots.begin(),
                    definition->slots.end());

                continue;
            }

            AddUnique(
                output.visibleModels,
                *models,
                definition->tintMaterial,
                equipped.colour);
        }

        if (output.visibleModels.empty())
        {
            error =
                "Character composition produced no visible models.";

            return false;
        }

        return true;
    }
}
