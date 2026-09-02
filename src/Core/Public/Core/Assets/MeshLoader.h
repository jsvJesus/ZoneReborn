#pragma once

#include "Core/Assets/MeshData.h"
#include "Core/Assets/PrimitivesContainer.h"
#include "Core/Assets/VisualAsset.h"

#include <string>

namespace core::assets
{
    class MeshLoader final
    {
    public:
        [[nodiscard]]
        bool Load(
            const PrimitivesContainer& primitives,
            const VisualGeometry& geometry,
            MeshData& output,
            std::string& error) const;
    };
}