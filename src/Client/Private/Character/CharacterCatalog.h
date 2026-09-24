#pragma once

#include "Character/CharacterFaceCatalog.h"
#include "Character/CharacterSlots.h"

#include "Core/Resources/ResourceFileSystem.h"

#include <cstdint>
#include <array>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace client::character
{
    struct ModelAlternative final
    {
        std::string tag;

        std::vector<std::string>
            models;
    };

    struct SlotOverride final
    {
        Slot slot =
            Slot::Body;

        bool hide =
            false;

        std::string tag;
    };

    struct ItemDefinition final
    {
        std::int32_t typeId =
            0;

        std::string scriptName;

        std::vector<Slot>
            slots;

        std::vector<std::string>
            models;

        std::vector<ModelAlternative>
            alternatives;

        std::vector<SlotOverride>
            overrides;

        bool fixRollLeftHand =
            false;

        std::string substrate;
        std::string tintMaterial;

        std::array<float, 2> lengthLimits{};

        bool hasLengthLimits =
            false;
    };

    struct CreatorOption final
    {
        std::int32_t itemType =
            0;

        std::string texture;
    };

    struct CreatorGroup final
    {
        std::string name;

        std::vector<CreatorOption>
            options;
    };

    class Catalog final
    {
    public:
        [[nodiscard]]
        bool Load(
            const core::resources::ResourceFileSystem& resources,
            std::string& error);

        void Clear();

        [[nodiscard]]
        const ItemDefinition* Find(
            std::int32_t typeId) const noexcept;

        [[nodiscard]]
        const CreatorGroup* FindCreatorGroup(
            std::string_view name) const noexcept;

        [[nodiscard]]
        const std::vector<CreatorGroup>&
        CreatorGroups() const noexcept;

        [[nodiscard]]
        const FaceCatalog& Faces() const noexcept;

        [[nodiscard]]
        std::size_t ItemCount() const noexcept;

    private:
        [[nodiscard]]
        bool LoadItems(
            const core::resources::ResourceFileSystem& resources,
            std::string& error);

        [[nodiscard]]
        bool LoadCreator(
            const core::resources::ResourceFileSystem& resources,
            std::string& error);

        std::unordered_map<
            std::int32_t,
            ItemDefinition>
            items_;

        std::vector<CreatorGroup>
            creatorGroups_;

        FaceCatalog faceCatalog_;
    };
}
