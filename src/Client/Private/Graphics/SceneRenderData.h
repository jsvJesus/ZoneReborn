#pragma once

#include "Core/Assets/MeshData.h"
#include "Core/Math/Transform3x4.h"

#include <cstddef>
#include <vector>

namespace client::graphics
{
    struct SceneInstance final
    {
        std::size_t meshIndex = 0;

        core::math::Transform3x4 transform;
    };

    struct SceneRenderData final
    {
        std::vector<core::assets::MeshData> meshes;

        std::vector<SceneInstance> instances;
    };
}