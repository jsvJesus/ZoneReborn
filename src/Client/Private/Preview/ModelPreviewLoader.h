#pragma once

#include "Core/Assets/MeshData.h"
#include "Core/Runtime.h"

#include <string>

namespace client::preview
{
    [[nodiscard]]
    bool LoadModelPreview(
        core::Runtime& runtime,
        core::assets::MeshData& output,
        std::string& error);
}