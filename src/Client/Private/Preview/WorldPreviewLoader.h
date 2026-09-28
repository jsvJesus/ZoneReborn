#pragma once

#include "Graphics/SceneRenderData.h"
#include "Loading/LoadingProgress.h"
#include "Core/Runtime.h"

#include <string>
#include <string_view>

namespace client::preview
{
    [[nodiscard]]
    bool LoadWorldPreview(
        core::Runtime& runtime,
        std::string_view spaceName,
        graphics::SceneRenderData& output,
        std::string& error,
        const loading::ProgressCallback& progress = {});
}