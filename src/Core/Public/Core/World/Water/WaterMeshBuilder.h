#pragma once

#include "Core/Assets/MeshData.h"
#include "Core/Math/Transform3x4.h"
#include "Core/World/Water/WaterDefinition.h"

#include <string>

namespace core::world::water
{
    class WaterMeshBuilder final
    {
    public:
        [[nodiscard]]
        bool Build(
            const WaterDefinition& definition,
            assets::MeshData& mesh,
            math::Transform3x4& transform,
            std::string& error) const;
    };
}