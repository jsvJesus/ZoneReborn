#pragma once

#include "Character/CharacterAnimationState.h"

#include "Core/Resources/ResourceFileSystem.h"

#include <array>
#include <string>
#include <string_view>

namespace client::character
{
    struct AnimationSet final
    {
        std::array<
            std::string,
            AnimationStateCount>
            paths;

        std::array<
            float,
            AnimationStateCount>
            frameRates{};

        [[nodiscard]]
        std::string_view Path(
            AnimationState state) const noexcept;

        [[nodiscard]]
        float FrameRate(
            AnimationState state) const noexcept;
    };

    class AnimationCatalog final
    {
    public:
        [[nodiscard]]
        bool Resolve(
            const core::resources::ResourceFileSystem& resources,
            AnimationSet& output,
            std::string& error) const;
    };
}