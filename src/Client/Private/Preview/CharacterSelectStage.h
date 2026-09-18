#pragma once

#include "Graphics/CameraView.h"

#include "Core/Math/Transform3x4.h"
#include "Core/Resources/ResourceFileSystem.h"

#include <string>
#include <string_view>

namespace client::preview
{
    struct CharacterSelectStageData final
    {
        graphics::CameraView camera;

        core::math::Transform3x4
            dummyTransform;

        bool hasCamera =
            false;

        bool hasDummy =
            false;
    };

    [[nodiscard]]
    bool LoadCharacterSelectStage(
        const core::resources::ResourceFileSystem& resources,
        std::string_view spaceName,
        CharacterSelectStageData& output,
        std::string& error);
}