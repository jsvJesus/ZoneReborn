#pragma once

#include "Core/Math/Transform3x4.h"
#include "Core/World/Flora/FloraConfig.h"
#include "Core/World/Flora/FloraInstance.h"
#include "Core/World/TerrainAuxiliaryData.h"
#include "Core/World/TerrainHeightData.h"

#include <string>
#include <string_view>
#include <vector>

namespace core::world::flora
{
    class FloraInstanceBuilder final
    {
    public:
        [[nodiscard]]
        bool Build(
            std::string_view chunkId,
            const TerrainHeightData& heightData,
            const TerrainAuxiliaryData& auxiliary,
            const math::Transform3x4& terrainTransform,
            const FloraConfig& config,
            std::vector<FloraInstance>& output,
            std::string& error) const;
    };
}