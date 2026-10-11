#pragma once

#include "Core/World/XRay/XRayDetails.h"
#include "Core/World/XRay/CformCollision.h"

#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

namespace client::graphics
{
    struct DetailInstance final
    {
        std::array<float, 4> positionScale{};
        // cos(yaw), sin(yaw), hemi, sun; matches the HLSL structured buffer.
        std::array<float, 4> rotationLighting{};
    };
    static_assert(sizeof(DetailInstance) == 32u);

    struct DetailFrameStatistics final
    {
        bool enabled = false, collisionAvailable = false;
        std::size_t cachedCells = 0, pendingCells = 0, cachedInstances = 0;
        std::size_t visibleInstances = 0, groundQueries = 0;
    };

    class XRayDetailCache final
    {
    public:
        static constexpr float RenderDistance = 40.0f;
        static constexpr std::size_t MaxInstances = 65536;
        void Reset(std::shared_ptr<const core::world::xray::DetailsData> details = {},
            std::shared_ptr<const core::world::xray::CformCollision> collision = {});
        void Update(const core::math::Vector3& camera, const std::array<float, 16>& viewProjection);
        const std::vector<DetailInstance>& Instances() const noexcept { return instances_; }
        const std::array<std::uint32_t, 63>& Counts() const noexcept { return counts_; }
        const std::array<std::uint32_t, 63>& Starts() const noexcept { return starts_; }
        DetailFrameStatistics Statistics() const noexcept { return statistics_; }
        const core::world::xray::DetailsData* Data() const noexcept { return details_.get(); }
    private:
        struct Placement { DetailInstance instance; std::uint32_t model = 0; };
        struct Cell { std::vector<Placement> placements; unsigned nextSample = 0; };
        void Sample(Cell& cell, std::int64_t x, std::int64_t z,
            const core::world::xray::DetailSlot& slot);
        std::shared_ptr<const core::world::xray::DetailsData> details_;
        std::shared_ptr<const core::world::xray::CformCollision> collision_;
        std::unordered_map<std::uint64_t, Cell> cells_;
        std::vector<DetailInstance> instances_;
        std::array<std::uint32_t, 63> counts_{}, starts_{};
        DetailFrameStatistics statistics_;
    };
}
