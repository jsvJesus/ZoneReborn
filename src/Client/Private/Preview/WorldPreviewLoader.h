#pragma once

#include "Graphics/SceneRenderData.h"

#include "Core/Runtime.h"

#include <string>

namespace client::preview
{
    [[nodiscard]]
    bool LoadWorldPreview(
        core::Runtime& runtime,
        graphics::SceneRenderData& output,
        std::string& error);
}