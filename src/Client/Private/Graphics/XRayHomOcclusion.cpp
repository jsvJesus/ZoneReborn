#include "Graphics/XRayHomOcclusion.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace
{
    constexpr double DepthBias = 0.00001;
    constexpr double CoverageInset = 0.02;

    struct ClipVertex final
    {
        double x, y, z, w;
    };

    ClipVertex Transform(
        const std::array<float, 16>& matrix,
        double x, double y, double z) noexcept
    {
        // DirectXMath row-vector convention, before perspective division.
        return {
            x * matrix[0] + y * matrix[4] + z * matrix[8] + matrix[12],
            x * matrix[1] + y * matrix[5] + z * matrix[9] + matrix[13],
            x * matrix[2] + y * matrix[6] + z * matrix[10] + matrix[14],
            x * matrix[3] + y * matrix[7] + z * matrix[11] + matrix[15]
        };
    }

    bool Finite(ClipVertex vertex) noexcept
    {
        return std::isfinite(vertex.x) && std::isfinite(vertex.y) &&
            std::isfinite(vertex.z) && std::isfinite(vertex.w);
    }

    double PlaneDistance(ClipVertex vertex, int plane) noexcept
    {
        switch (plane)
        {
        case 0: return vertex.z;             // DX11 near: z >= 0
        case 1: return vertex.w - vertex.z;
        case 2: return vertex.w + vertex.x;
        case 3: return vertex.w - vertex.x;
        case 4: return vertex.w + vertex.y;
        default: return vertex.w - vertex.y;
        }
    }

    bool ClipTriangle(
        std::array<ClipVertex, 12>& polygon,
        std::size_t& count) noexcept
    {
        std::array<ClipVertex, 12> scratch{};
        for (int plane = 0; plane < 6 && count != 0; ++plane)
        {
            std::size_t nextCount = 0;
            auto previous = polygon[count - 1];
            double previousDistance = PlaneDistance(previous, plane);
            for (std::size_t i = 0; i < count; ++i)
            {
                const auto current = polygon[i];
                const double distance = PlaneDistance(current, plane);
                if ((previousDistance >= 0.0) != (distance >= 0.0))
                {
                    if (nextCount == scratch.size())
                        return false;
                    const double t = previousDistance / (previousDistance - distance);
                    scratch[nextCount++] = {
                        previous.x + (current.x - previous.x) * t,
                        previous.y + (current.y - previous.y) * t,
                        previous.z + (current.z - previous.z) * t,
                        previous.w + (current.w - previous.w) * t
                    };
                }
                if (distance >= 0.0)
                {
                    if (nextCount == scratch.size())
                        return false;
                    scratch[nextCount++] = current;
                }
                previous = current;
                previousDistance = distance;
            }
            polygon = scratch;
            count = nextCount;
        }
        return count >= 3;
    }
}

namespace client::graphics
{
    void XRayHomOcclusion::SetTriangles(
        const std::vector<core::world::xray::HomTriangle>& triangles)
    {
        triangles_ = triangles;
        frameValid_ = false;
        statistics_ = {};
        statistics_.triangles = triangles_.size();
    }

    void XRayHomOcclusion::Clear() noexcept
    {
        // Also release the previous map's capacity when switching scenes.
        std::vector<core::world::xray::HomTriangle>{}.swap(triangles_);
        frameValid_ = false;
        statistics_ = {};
    }

    void XRayHomOcclusion::Build(
        const std::array<float, 16>& viewProjection,
        const core::math::Vector3 cameraPosition) noexcept
    {
        statistics_ = {};
        statistics_.triangles = triangles_.size();
        frameValid_ = false;
        rasterCells_ = 0;
        if (triangles_.empty() || !std::isfinite(cameraPosition.x) ||
            !std::isfinite(cameraPosition.y) || !std::isfinite(cameraPosition.z))
            return;
        for (float value : viewProjection)
            if (!std::isfinite(value))
                return;

        viewProjection_ = viewProjection;
        depth_[0].fill(1.0f);

        std::size_t consideredTriangles = 0;
        for (const auto& triangle : triangles_)
        {
            // Charge before any early rejection: offscreen/backfacing triangles
            // must not bypass the transform/clip-work budget.
            if (consideredTriangles++ >= TriangleBudget || rasterCells_ >= CellBudget)
                break; // An incomplete map under-culls; it cannot hide extra objects.

            if (triangle.flags == 0u)
            {
                const auto a = triangle.vertices[0];
                const auto b = triangle.vertices[1];
                const auto c = triangle.vertices[2];
                const double abx = double(b.x) - a.x, aby = double(b.y) - a.y, abz = double(b.z) - a.z;
                const double acx = double(c.x) - a.x, acy = double(c.y) - a.y, acz = double(c.z) - a.z;
                const double facing = (aby * acz - abz * acy) * (double(cameraPosition.x) - a.x) +
                    (abz * acx - abx * acz) * (double(cameraPosition.y) - a.y) +
                    (abx * acy - aby * acx) * (double(cameraPosition.z) - a.z);
                if (!(facing > 0.0))
                    continue;
            }

            std::array<ClipVertex, 12> polygon{};
            for (std::size_t i = 0; i < 3; ++i)
            {
                const auto point = triangle.vertices[i];
                polygon[i] = Transform(viewProjection_, point.x, point.y, point.z);
            }
            if (!Finite(polygon[0]) || !Finite(polygon[1]) || !Finite(polygon[2]))
                continue;
            std::size_t count = 3;
            if (!ClipTriangle(polygon, count))
                continue;

            std::array<ScreenVertex, 12> projected{};
            bool valid = true;
            for (std::size_t i = 0; i < count; ++i)
            {
                const auto point = polygon[i];
                if (!Finite(point) || point.w <= 0.000001)
                {
                    valid = false;
                    break;
                }
                projected[i] = {
                    (point.x / point.w * 0.5 + 0.5) * Dimension,
                    (0.5 - point.y / point.w * 0.5) * Dimension,
                    point.z / point.w
                };
            }
            if (!valid)
                continue;

            for (std::size_t i = 1; i + 1 < count && rasterCells_ < CellBudget; ++i)
                RasterTriangle(projected[0], projected[i], projected[i + 1]);
        }

        BuildPyramid();
        frameValid_ = true;
    }

    void XRayHomOcclusion::RasterTriangle(
        ScreenVertex a, ScreenVertex b, ScreenVertex c) noexcept
    {
        const auto edge = [](ScreenVertex p, ScreenVertex q, double x, double y)
        {
            return (q.x - p.x) * (y - p.y) - (q.y - p.y) * (x - p.x);
        };
        double area = edge(a, b, c.x, c.y);
        if (!std::isfinite(area) || std::abs(area) < 0.00000001)
            return;
        if (area < 0.0)
        {
            std::swap(b, c);
            area = -area;
        }

        const auto firstCell = [](double coordinate)
        {
            return static_cast<int>(std::clamp(std::floor(coordinate), 0.0, double(Dimension - 1)));
        };
        const int firstX = firstCell(std::min({a.x, b.x, c.x}));
        const int lastX = firstCell(std::max({a.x, b.x, c.x}));
        const int firstY = firstCell(std::min({a.y, b.y, c.y}));
        const int lastY = firstCell(std::max({a.y, b.y, c.y}));
        const double insetA = CoverageInset * std::hypot(c.x - b.x, c.y - b.y);
        const double insetB = CoverageInset * std::hypot(a.x - c.x, a.y - c.y);
        const double insetC = CoverageInset * std::hypot(b.x - a.x, b.y - a.y);
        bool wroteCell = false;

        for (int y = firstY; y <= lastY; ++y)
        {
            for (int x = firstX; x <= lastX; ++x)
            {
                if (rasterCells_++ >= CellBudget)
                    return;

                bool covered = true;
                double farthestDepth = 0.0;
                for (int corner = 0; corner < 4; ++corner)
                {
                    const double px = x + (corner & 1);
                    const double py = y + ((corner >> 1) & 1);
                    const double wa = edge(b, c, px, py);
                    const double wb = edge(c, a, px, py);
                    const double wc = edge(a, b, px, py);
                    if (wa < insetA || wb < insetB || wc < insetC)
                    {
                        covered = false;
                        break;
                    }
                    // NDC depth is affine in screen space; its maximum over
                    // the entire cell occurs at one of these four corners.
                    farthestDepth = std::max(farthestDepth,
                        (wa * a.depth + wb * b.depth + wc * c.depth) / area);
                }
                if (!covered || !std::isfinite(farthestDepth) || farthestDepth >= 1.0)
                    continue;

                // Round away from the camera, never towards it.
                const float safeDepth = std::nextafter(
                    static_cast<float>(farthestDepth + DepthBias),
                    std::numeric_limits<float>::infinity());
                auto& cell = depth_[0][static_cast<std::size_t>(y) * Dimension + x];
                cell = std::min(cell, safeDepth);
                wroteCell = true;
            }
        }
        if (wroteCell)
            ++statistics_.rasterizedTriangles;
    }

    void XRayHomOcclusion::BuildPyramid() noexcept
    {
        for (std::size_t level = 1; level < LevelCount; ++level)
        {
            const std::size_t size = Dimension >> level;
            const std::size_t previousSize = size * 2;
            for (std::size_t y = 0; y < size; ++y)
            {
                for (std::size_t x = 0; x < size; ++x)
                {
                    const std::size_t first = y * 2 * previousSize + x * 2;
                    depth_[level][y * size + x] = std::max({
                        depth_[level - 1][first], depth_[level - 1][first + 1],
                        depth_[level - 1][first + previousSize],
                        depth_[level - 1][first + previousSize + 1]
                    });
                }
            }
        }
    }

    bool XRayHomOcclusion::IsOccluded(
        const core::math::Vector3 minimum,
        const core::math::Vector3 maximum,
        const core::math::Transform3x4& world) noexcept
    {
        if (!frameValid_)
            return false;
        ++statistics_.testedVisuals;
        if (!std::isfinite(minimum.x) || !std::isfinite(minimum.y) || !std::isfinite(minimum.z) ||
            !std::isfinite(maximum.x) || !std::isfinite(maximum.y) || !std::isfinite(maximum.z) ||
            minimum.x > maximum.x || minimum.y > maximum.y || minimum.z > maximum.z)
            return false;

        double firstX = double(Dimension), firstY = double(Dimension);
        double lastX = 0.0, lastY = 0.0, nearestDepth = 1.0;
        const auto& m = world.values;
        for (int corner = 0; corner < 8; ++corner)
        {
            const double x = (corner & 1) ? maximum.x : minimum.x;
            const double y = (corner & 2) ? maximum.y : minimum.y;
            const double z = (corner & 4) ? maximum.z : minimum.z;
            const auto point = Transform(viewProjection_,
                x * m[0] + y * m[3] + z * m[6] + m[9],
                x * m[1] + y * m[4] + z * m[7] + m[10],
                x * m[2] + y * m[5] + z * m[8] + m[11]);
            // Near-plane intersections / behind-camera bounds stay visible.
            if (!Finite(point) || point.w <= 0.000001 || point.z <= 0.0 || point.z >= point.w)
                return false;
            const double px = (point.x / point.w * 0.5 + 0.5) * Dimension;
            const double py = (0.5 - point.y / point.w * 0.5) * Dimension;
            firstX = std::min(firstX, px);
            firstY = std::min(firstY, py);
            lastX = std::max(lastX, px);
            lastY = std::max(lastY, py);
            nearestDepth = std::min(nearestDepth, point.z / point.w);
        }
        // Keep partially offscreen objects visible; do not clamp away uncertainty.
        if (firstX < 0.0 || firstY < 0.0 || lastX >= Dimension || lastY >= Dimension)
            return false;

        const int x0 = std::max(0, static_cast<int>(std::floor(firstX)) - 1);
        const int y0 = std::max(0, static_cast<int>(std::floor(firstY)) - 1);
        const int x1 = std::min(int(Dimension) - 1, static_cast<int>(std::floor(lastX)) + 1);
        const int y1 = std::min(int(Dimension) - 1, static_cast<int>(std::floor(lastY)) + 1);

        std::size_t level = 0;
        const int extent = std::max(x1 - x0 + 1, y1 - y0 + 1);
        while (level + 1 < LevelCount && (extent >> level) > 4)
            ++level;
        const std::size_t size = Dimension >> level;
        const double testDepth = nearestDepth - DepthBias;
        for (int y = y0 >> level; y <= (y1 >> level); ++y)
        {
            for (int x = x0 >> level; x <= (x1 >> level); ++x)
            {
                // The maximum includes uncovered cells (depth 1), preventing
                // holes and triangle silhouettes from hiding visible geometry.
                if (depth_[level][static_cast<std::size_t>(y) * size + x] >= testDepth)
                    return false;
            }
        }
        ++statistics_.culledVisuals;
        return true;
    }
}
