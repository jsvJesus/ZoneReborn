#include "Graphics/XRayDetailCache.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <utility>

namespace client::graphics
{
    namespace
    {
        constexpr unsigned Steps = 5, Samples = (Steps+1)*(Steps+1);
        constexpr std::int64_t CellRadius = 22;
        constexpr unsigned QueryBudget = 256;
        std::uint32_t Hash(std::uint32_t value)
        {
            value ^= value >> 16u; value *= 0x7feb352du;
            value ^= value >> 15u; value *= 0x846ca68bu;
            return value ^ (value >> 16u);
        }
        float Random(std::uint32_t& seed)
        { seed = Hash(seed+0x9e3779b9u); return float(seed >> 8u)/16777216.0f; }
        bool Visible(const std::array<float, 16>& m, const core::math::Vector3& p, float radius)
        {
            // Column planes for row-vector, LH DirectX view-projection (near Z >= 0).
            for (unsigned plane = 0; plane < 6; ++plane)
            {
                const unsigned column = plane < 2 ? 0 : (plane < 4 ? 1 : 2);
                const float sign = (plane%2 == 0) ? 1.0f : -1.0f;
                std::array<float, 4> q{};
                for (unsigned row = 0; row < 4; ++row)
                    q[row] = plane == 4 ? m[row*4+2] : m[row*4+3]+sign*m[row*4+column];
                const float length = std::sqrt(q[0]*q[0]+q[1]*q[1]+q[2]*q[2]);
                if (q[0]*p.x+q[1]*p.y+q[2]*p.z+q[3] < -radius*length) return false;
            }
            return true;
        }
    }

    void XRayDetailCache::Reset(std::shared_ptr<const core::world::xray::DetailsData> details,
        std::shared_ptr<const core::world::xray::CformCollision> collision)
    {
        // Scene replacement/shutdown, never normal frame preparation. The worker
        // owns shared immutable data and must stop before releasing mapped files.
        if (job_.valid()) { job_.wait(); job_ = {}; }
        details_ = std::move(details); collision_ = std::move(collision);
        cells_.clear(); instances_.clear(); counts_ = {}; starts_ = {}; statistics_ = {};
        statistics_.collisionAvailable = collision_ && collision_->Statistics().faces != 0;
        statistics_.enabled = details_ && statistics_.collisionAvailable &&
            details_->Statistics().slots != 0 && !details_->Models().empty();
    }

    void XRayDetailCache::Sample(Cell& cell, std::int64_t x, std::int64_t z,
        const core::world::xray::DetailSlot& slot,
        const core::world::xray::DetailsData& data,
        const core::world::xray::CformCollision& collision, std::size_t& groundQueries)
    {
        const auto sample = cell.nextSample++;
        const auto stats = data.Statistics();
        const float u = float(sample%(Steps+1))/Steps, v = float(sample/(Steps+1))/Steps;
        auto seed = Hash(static_cast<std::uint32_t>(x)) ^ Hash(static_cast<std::uint32_t>(z)+0x12071980u) ^ Hash(sample);
        std::array<std::uint32_t, 4> selected{};
        unsigned count = 0;
        for (std::size_t part = 0; part < 4; ++part)
            if (slot.Model(part) != 63 && Random(seed) < slot.Density(part, u, v))
                selected[count++] = slot.Model(part);
        if (count == 0) return;
        const auto modelId = selected[std::min(count-1, static_cast<unsigned>(Random(seed)*count))];
        const float px = float(x-stats.offsetX)*2.0f+u*2.0f+(Random(seed)*2.0f-1.0f)*(0.4f/1.7f);
        const float pz = float(z-stats.offsetZ)*2.0f+v*2.0f+(Random(seed)*2.0f-1.0f)*(0.4f/1.7f);
        core::world::xray::CformHit hit;
        ++groundQueries;
        if (!collision.Raycast({px, slot.Base()+slot.Height()+0.05f, pz},
            {px, slot.Base()-0.05f, pz}, hit) || hit.position.y < slot.Base()-0.05f || hit.normal.y < 0.1f)
            return;
        const auto& model = data.Models()[modelId];
        const float minimum = model.minimumScale*0.5f, maximum = model.maximumScale*0.9f;
        const float scale = minimum+(maximum-minimum)*Random(seed);
        const float yaw = Random(seed)*6.28318530718f;
        Placement placement;
        placement.model = modelId;
        placement.instance.positionScale = {px, hit.position.y, pz, scale};
        placement.instance.rotationLighting = {std::cos(yaw), std::sin(yaw), slot.Hemi(), slot.Sun()};
        cell.placements.push_back(placement);
    }

    void XRayDetailCache::Update(const core::math::Vector3& camera, const std::array<float, 16>& projection)
    {
        instances_.clear(); counts_ = {}; starts_ = {};
        statistics_.cachedCells = statistics_.pendingCells = statistics_.cachedInstances = 0;
        statistics_.visibleInstances = statistics_.groundQueries = 0;
        if (!statistics_.enabled) return;
        if (job_.valid() && job_.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            auto result = job_.get();
            statistics_.groundQueries = result.groundQueries;
            for (auto& prepared : result.cells)
                cells_[prepared.key] = std::move(prepared.cell);
        }
        if (!std::isfinite(camera.x) || !std::isfinite(camera.y) || !std::isfinite(camera.z) ||
            std::abs(camera.x) > 1e8f || std::abs(camera.z) > 1e8f)
        { cells_.clear(); return; }
        const auto stats = details_->Statistics();
        const auto cx = static_cast<std::int64_t>(std::floor(double(camera.x)/2))+stats.offsetX;
        const auto cz = static_cast<std::int64_t>(std::floor(double(camera.z)/2))+stats.offsetZ;
        const auto minX = std::max<std::int64_t>(0, cx-CellRadius), minZ = std::max<std::int64_t>(0, cz-CellRadius);
        const auto maxX = std::min<std::int64_t>(std::int64_t(stats.sizeX)-1, cx+CellRadius);
        const auto maxZ = std::min<std::int64_t>(std::int64_t(stats.sizeZ)-1, cz+CellRadius);
        for (auto it = cells_.begin(); it != cells_.end();)
        {
            const auto x = std::int64_t(it->first%stats.sizeX), z = std::int64_t(it->first/stats.sizeX);
            if (x < minX || x > maxX || z < minZ || z > maxZ) it = cells_.erase(it);
            else ++it;
        }
        struct Candidate { std::int64_t x, z; float distance; };
        std::vector<Candidate> pending;
        for (auto z = minZ; z <= maxZ; ++z)
            for (auto x = minX; x <= maxX; ++x)
            {
                core::world::xray::DetailSlot slot;
                if (!details_->ReadSlot(x, z, slot)) continue;
                bool populated = false;
                for (std::size_t part = 0; part < 4; ++part)
                    populated |= slot.Model(part) != 63 && slot.palettes[part] != 0;
                if (!populated || camera.y < slot.Base()-RenderDistance ||
                    camera.y > slot.Base()+slot.Height()+RenderDistance) continue;
                const auto key = std::uint64_t(z)*stats.sizeX+std::uint64_t(x);
                const auto existing = cells_.find(key);
                if (existing != cells_.end() && existing->second.nextSample == Samples) continue;
                const float dx = float(x-cx), dz = float(z-cz);
                pending.push_back({x,z,dx*dx+dz*dz});
            }
        std::sort(pending.begin(), pending.end(), [](const Candidate& a, const Candidate& b)
        { return a.distance < b.distance || (a.distance == b.distance && (a.z < b.z || (a.z == b.z && a.x < b.x))); });
        if (!job_.valid() && !pending.empty())
        {
            JobResult batch;
            for (std::size_t i = 0; i < std::min<std::size_t>(16, pending.size()); ++i)
            {
                const auto& candidate = pending[i];
                const auto key = std::uint64_t(candidate.z)*stats.sizeX+std::uint64_t(candidate.x);
                const auto existing = cells_.find(key);
                batch.cells.push_back({key,existing == cells_.end() ? Cell{} : existing->second});
            }
            job_ = std::async(std::launch::async,
                [data = details_, collision = collision_, batch = std::move(batch)]() mutable
                {
                    const auto dimensions = data->Statistics();
                    const auto deadline = std::chrono::steady_clock::now()+std::chrono::milliseconds(3);
                    unsigned samples = 0;
                    for (auto& prepared : batch.cells)
                    {
                        const auto x = static_cast<std::int64_t>(prepared.key%dimensions.sizeX);
                        const auto z = static_cast<std::int64_t>(prepared.key/dimensions.sizeX);
                        core::world::xray::DetailSlot slot;
                        if (!data->ReadSlot(x,z,slot)) continue;
                        while (prepared.cell.nextSample < Samples && samples < QueryBudget &&
                            std::chrono::steady_clock::now() < deadline)
                        {
                            Sample(prepared.cell,x,z,slot,*data,*collision,batch.groundQueries);
                            ++samples;
                        }
                        if (samples == QueryBudget || std::chrono::steady_clock::now() >= deadline) break;
                    }
                    return batch;
                });
        }
        statistics_.pendingCells = pending.size();
        std::array<std::vector<DetailInstance>, 63> visible;
        std::size_t accepted = 0;
        for (const auto& [key, cell] : cells_)
        {
            statistics_.cachedInstances += cell.placements.size();
            for (const auto& placement : cell.placements)
            {
                const auto& instance = placement.instance;
                const auto& p = instance.positionScale;
                const auto& model = details_->Models()[placement.model];
                // An origin-centred sphere also encloses the distance-scaled model
                // as it shrinks towards the base, and includes its wind excursion.
                const core::math::Vector3 center{p[0],p[1],p[2]};
                const float ex = std::max(std::abs(model.minimum.x),std::abs(model.maximum.x))*p[3];
                const float ey = std::max(std::abs(model.minimum.y),std::abs(model.maximum.y))*p[3];
                const float ez = std::max(std::abs(model.minimum.z),std::abs(model.maximum.z))*p[3];
                const float radius = std::sqrt(ex*ex+ey*ey+ez*ez)+0.5f;
                const float dx = p[0]-camera.x, dy = p[1]-camera.y, dz = p[2]-camera.z;
                if (dx*dx+dy*dy+dz*dz > RenderDistance*RenderDistance || !Visible(projection, center, radius) ||
                    accepted == MaxInstances) continue;
                visible[placement.model].push_back(instance); ++accepted;
            }
        }
        instances_.reserve(accepted);
        for (std::size_t model = 0; model < visible.size(); ++model)
        {
            starts_[model] = static_cast<std::uint32_t>(instances_.size());
            counts_[model] = static_cast<std::uint32_t>(visible[model].size());
            instances_.insert(instances_.end(), visible[model].begin(), visible[model].end());
        }
        statistics_.cachedCells = cells_.size();
        statistics_.visibleInstances = instances_.size();
    }
}
