#pragma once

#include "Core/World/XRay/XRayLevelData.h"

#include <array>
#include <cstddef>
#include <vector>

namespace client::graphics
{
    struct HomOcclusionStatistics final
    {
        std::size_t triangles = 0;
        std::size_t rasterizedTriangles = 0;
        std::size_t testedVisuals = 0;
        std::size_t culledVisuals = 0;
    };

    // A conservative CPU occlusion map, not a renderer or a collision mesh.
    // A cell is written only when its entire area is covered by an occluder.
    class XRayHomOcclusion final
    {
    public:
        void SetTriangles(const std::vector<core::world::xray::HomTriangle>& triangles);
        void Clear() noexcept;

        void Build(
            const std::array<float, 16>& viewProjection,
            core::math::Vector3 cameraPosition) noexcept;

        [[nodiscard]]
        bool IsOccluded(
            core::math::Vector3 minimum,
            core::math::Vector3 maximum,
            const core::math::Transform3x4& world) noexcept;

        [[nodiscard]]
        bool Active() const noexcept { return frameValid_; }

        [[nodiscard]]
        const HomOcclusionStatistics& Statistics() const noexcept { return statistics_; }

    private:
        static constexpr std::size_t Dimension = 128;
        static constexpr std::size_t LevelCount = 8;
        static constexpr std::size_t CellBudget = 2 * 1024 * 1024;
        static constexpr std::size_t TriangleBudget = 65536;

        struct ScreenVertex final
        {
            double x, y, depth;
        };

        void RasterTriangle(ScreenVertex a, ScreenVertex b, ScreenVertex c) noexcept;
        void BuildPyramid() noexcept;

        std::vector<core::world::xray::HomTriangle> triangles_;
        std::array<float, 16> viewProjection_{};
        // Each level uses only its (Dimension >> level)^2 leading entries.
        std::array<std::array<float, Dimension * Dimension>, LevelCount> depth_{};
        HomOcclusionStatistics statistics_{};
        std::size_t rasterCells_ = 0;
        bool frameValid_ = false;
    };
}
