#pragma once

#include "Character/CharacterCatalog.h"
#include "Character/CharacterState.h"

#include <string>
#include <unordered_set>
#include <vector>

namespace client::character
{
    struct ModelPlan final
    {
        std::string skeletonModel =
            "characters2/basemodel/StalkerBase2.model";

        std::string lodModel =
            "characters2/basemodel/Basem/Basem.model";

        std::vector<std::string>
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