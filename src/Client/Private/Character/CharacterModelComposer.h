#pragma once

#include "Character/CharacterCatalog.h"
#include "Character/CharacterState.h"

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

namespace client::character
{
    struct VisibleModel final
    {
        std::string reference;
        std::string tintMaterial;

        std::uint32_t colour =
            0xFFFFFFu;
    };

    struct ModelPlan final
    {
        std::string skeletonModel =
            "characters2/basemodel/StalkerBase2.model";

        std::string lodModel =
            "characters2/basemodel/Basem/Basem.model";

        std::vector<VisibleModel>
            visibleModels;

        std::unordered_set<Slot>
            hiddenSlots;
    };

    class ModelComposer final
    {
    public:
        [[nodiscard]]
        bool Compose(
            const Catalog& catalog,
            const State& state,
            ModelPlan& output,
            std::string& error) const;
    };
}
