#pragma once

#include "Graphics/SceneRenderData.h"

#include "Core/Math/Transform3x4.h"
#include "Core/Resources/ResourceFileSystem.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace client::preview
{
    struct CharacterDummyAppearance final
    {
        std::int32_t head =
            1;

        std::int32_t body =
            108;

        std::int32_t palms =
            110247;

        std::int32_t legs =
            104;

        std::int32_t feet =
            110251;

        [[nodiscard]]
        bool SetPart(
            std::string_view choiceGroup,
            std::int32_t value) noexcept;
    };

    class CharacterDummyRenderDataBuilder final
    {
    public:
        [[nodiscard]]
        bool Build(
            const core::resources::ResourceFileSystem& resources,
            const CharacterDummyAppearance& appearance,
            const core::math::Transform3x4& transform,
            graphics::SceneRenderData& scene,
            std::string& error);

        [[nodiscard]]
        bool BuildDefault(
            const core::resources::ResourceFileSystem& resources,
            const core::math::Transform3x4& transform,
            graphics::SceneRenderData& scene,
            std::string& error);
    };
}