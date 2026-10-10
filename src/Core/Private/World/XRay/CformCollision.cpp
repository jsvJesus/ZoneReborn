#include "Core/World/XRay/CformCollision.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstring>
#include <limits>
#include <new>
#include <utility>
#include <vector>

namespace core::world::xray
{
    namespace
    {
        constexpr std::uint32_t FacesPerBlock = 512;
        constexpr std::size_t HeaderBytes = 36;
        constexpr std::size_t VertexBytes = 12;
        constexpr std::size_t FaceBytes = 16;
        static_assert(sizeof(math::Vector3) == VertexBytes);

        struct Bounds
        {
            math::Vector3 minimum{
                std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max()};
            math::Vector3 maximum{
                std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest()};

            void Include(const math::Vector3& p) noexcept
            {
                minimum.x = std::min(minimum.x, p.x);
                minimum.y = std::min(minimum.y, p.y);
                minimum.z = std::min(minimum.z, p.z);
                maximum.x = std::max(maximum.x, p.x);
                maximum.y = std::max(maximum.y, p.y);
                maximum.z = std::max(maximum.z, p.z);
            }
            void Include(const Bounds& b) noexcept { Include(b.minimum); Include(b.maximum); }
        };

        struct Block { Bounds bounds; std::uint32_t first; std::uint32_t count; };
        static_assert(sizeof(Bounds) == 24u);
        struct Node
        {
            Bounds bounds;
            std::uint32_t left = 0;
            std::uint32_t right = 0;
            std::uint32_t first = 0;
            std::uint32_t count = 0; // Zero means internal node.
        };
        static_assert(sizeof(Node) == 40u);

        bool Finite(const math::Vector3& p) noexcept
        { return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z); }
        double Axis(const math::Vector3& p, const int axis) noexcept
        { return axis == 0 ? p.x : (axis == 1 ? p.y : p.z); }

        struct Double3
        {
            double x, y, z;
            Double3 operator-(const Double3& b) const noexcept { return {x-b.x, y-b.y, z-b.z}; }
        };
        Double3 Wide(const math::Vector3& p) noexcept { return {p.x, p.y, p.z}; }
        double Dot(const Double3& a, const Double3& b) noexcept
        { return a.x*b.x + a.y*b.y + a.z*b.z; }
        Double3 Cross(const Double3& a, const Double3& b) noexcept
        { return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x}; }

        bool IntersectBounds(const Bounds& b, const Double3& start,
            const Double3& delta, const double limit, double& entry) noexcept
        {
            const double origin[3]{start.x, start.y, start.z};
            const double direction[3]{delta.x, delta.y, delta.z};
            double low = 0.0, high = limit;
            for (int axis = 0; axis < 3; ++axis)
            {
                const double minimum = Axis(b.minimum, axis);
                const double maximum = Axis(b.maximum, axis);
                if (direction[axis] == 0.0)
                {
                    if (origin[axis] < minimum || origin[axis] > maximum) return false;
                    continue;
                }
                double nearValue = (minimum-origin[axis])/direction[axis];
                double farValue = (maximum-origin[axis])/direction[axis];
                if (nearValue > farValue) std::swap(nearValue, farValue);
                low = std::max(low, nearValue);
                high = std::min(high, farValue);
                if (low > high) return false;
            }
            entry = low;
            return true;
        }

        bool IntersectTriangle(const Double3& start, const Double3& delta,
            const math::Vector3& a, const math::Vector3& b, const math::Vector3& c,
            double& fraction, math::Vector3& normal) noexcept
        {
            const auto edge1 = Wide(b)-Wide(a);
            const auto edge2 = Wide(c)-Wide(a);
            const auto n = Cross(edge1, edge2);
            const double normalLength = std::sqrt(Dot(n,n));
            const double rayLength = std::sqrt(Dot(delta,delta));
            if (normalLength == 0.0 || rayLength == 0.0) return false;
            const auto p = Cross(delta, edge2);
            const double determinant = Dot(edge1,p);
            if (std::abs(determinant) <= 1e-12*normalLength*rayLength) return false;
            const double inverse = 1.0/determinant;
            const auto offset = start-Wide(a);
            const double u = Dot(offset,p)*inverse;
            if (u < 0.0 || u > 1.0) return false;
            const auto q = Cross(offset,edge1);
            const double v = Dot(delta,q)*inverse;
            if (v < 0.0 || u+v > 1.0) return false;
            const double t = Dot(edge2,q)*inverse;
            if (t < 0.0 || t > fraction) return false;
            fraction = t;
            const double sign = Dot(n,delta) > 0.0 ? -1.0 : 1.0;
            normal = {static_cast<float>(sign*n.x/normalLength),
                static_cast<float>(sign*n.y/normalLength), static_cast<float>(sign*n.z/normalLength)};
            return true;
        }
    }

    struct CformCollision::State
    {
        HANDLE file = INVALID_HANDLE_VALUE;
        HANDLE mapping = nullptr;
        const std::byte* bytes = nullptr;
        std::uint64_t faceOffset = 0;
        CformStatistics statistics;
        std::vector<Node> nodes;

        ~State()
        {
            if (bytes) UnmapViewOfFile(bytes);
            if (mapping) CloseHandle(mapping);
            if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
        }
        math::Vector3 Vertex(const std::uint32_t index) const noexcept
        {
            math::Vector3 p;
            std::memcpy(&p, bytes+HeaderBytes+std::size_t(index)*VertexBytes, VertexBytes);
            return p;
        }
        CformFace Face(const std::uint32_t index) const noexcept
        {
            CformFace face;
            std::memcpy(&face, bytes+faceOffset+std::size_t(index)*FaceBytes, FaceBytes);
            return face;
        }
        std::uint32_t Build(std::vector<Block>& blocks, std::size_t begin, std::size_t end)
        {
            const auto index = static_cast<std::uint32_t>(nodes.size());
            nodes.emplace_back();
            Bounds bounds;
            for (auto i = begin; i < end; ++i) bounds.Include(blocks[i].bounds);
            nodes[index].bounds = bounds;
            if (end-begin == 1)
            {
                nodes[index].first = blocks[begin].first;
                nodes[index].count = blocks[begin].count;
                return index;
            }
            int axis = 0;
            for (int candidate = 1; candidate < 3; ++candidate)
                if (Axis(bounds.maximum,candidate)-Axis(bounds.minimum,candidate) >
                    Axis(bounds.maximum,axis)-Axis(bounds.minimum,axis)) axis = candidate;
            const auto middle = begin+(end-begin)/2;
            std::nth_element(blocks.begin()+begin, blocks.begin()+middle, blocks.begin()+end,
                [axis](const Block& a, const Block& b)
                {
                    return Axis(a.bounds.minimum,axis)+Axis(a.bounds.maximum,axis) <
                        Axis(b.bounds.minimum,axis)+Axis(b.bounds.maximum,axis);
                });
            const auto left = Build(blocks,begin,middle);
            const auto right = Build(blocks,middle,end);
            nodes[index].left = left;
            nodes[index].right = right;
            return index;
        }
    };

    CformCollision::CformCollision() = default;
    CformCollision::~CformCollision() = default;
    void CformCollision::Clear() noexcept { state_.reset(); }
    CformStatistics CformCollision::Statistics() const noexcept
    { return state_ ? state_->statistics : CformStatistics{}; }

    bool CformCollision::Load(const std::filesystem::path& path,
        const std::size_t sectorCount, std::string& error)
    {
        error.clear();
        const auto fail = [&error](const std::string& message)
        { error = "level.cform: " + message; return false; };
        try
        {
            auto candidate = std::make_unique<State>();
            candidate->file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (candidate->file == INVALID_HANDLE_VALUE)
                return fail("unable to open file (Win32 " + std::to_string(GetLastError()) + ").");
            LARGE_INTEGER fileSize{};
            if (!GetFileSizeEx(candidate->file,&fileSize)) return fail("unable to read file size.");
            if (fileSize.QuadPart < static_cast<LONGLONG>(HeaderBytes)) return fail("truncated header.");
            candidate->mapping = CreateFileMappingW(candidate->file,nullptr,PAGE_READONLY,0,0,nullptr);
            if (!candidate->mapping) return fail("unable to create read-only mapping.");
            candidate->bytes = static_cast<const std::byte*>(
                MapViewOfFile(candidate->mapping,FILE_MAP_READ,0,0,0));
            if (!candidate->bytes) return fail("unable to map file.");
            std::uint32_t header[3]{};
            std::memcpy(header,candidate->bytes,sizeof(header));
            if (header[0] != 4) return fail("unsupported version " + std::to_string(header[0]) + ".");
            auto& stats = candidate->statistics;
            stats.vertices = header[1];
            stats.faces = header[2];
            candidate->faceOffset = HeaderBytes+std::uint64_t(stats.vertices)*VertexBytes;
            const auto expectedSize = candidate->faceOffset+std::uint64_t(stats.faces)*FaceBytes;
            if (expectedSize != static_cast<std::uint64_t>(fileSize.QuadPart))
                return fail("file size does not match the indexed vertex/face counts.");
            Bounds headerBounds;
            std::memcpy(&headerBounds,candidate->bytes+12,sizeof(headerBounds));
            if (!Finite(headerBounds.minimum) || !Finite(headerBounds.maximum) ||
                headerBounds.minimum.x > headerBounds.maximum.x ||
                headerBounds.minimum.y > headerBounds.maximum.y ||
                headerBounds.minimum.z > headerBounds.maximum.z)
                return fail("invalid header bounds.");
            for (std::uint32_t i = 0; i < stats.vertices; ++i)
                if (!Finite(candidate->Vertex(i)))
                    return fail("non-finite vertex " + std::to_string(i) + ".");

            stats.mappedBytes = expectedSize;
            stats.blocks = (std::uint64_t(stats.faces)+FacesPerBlock-1)/FacesPerBlock;
            std::vector<Block> blocks;
            blocks.reserve(stats.blocks);
            std::bitset<16384> materials;
            std::bitset<65536> sectors;
            for (std::uint64_t first = 0; first < stats.faces; first += FacesPerBlock)
            {
                Block block{{},static_cast<std::uint32_t>(first),
                    static_cast<std::uint32_t>(std::min<std::uint64_t>(FacesPerBlock,stats.faces-first))};
                for (std::uint32_t local = 0; local < block.count; ++local)
                {
                    const auto faceId = block.first+local;
                    const auto face = candidate->Face(faceId);
                    for (const auto vertex : face.vertices)
                    {
                        if (vertex >= stats.vertices)
                            return fail("out-of-range vertex index in face " + std::to_string(faceId) + ".");
                        block.bounds.Include(candidate->Vertex(vertex));
                    }
                    if (face.Sector() != 0xffffu)
                    {
                        if (sectorCount != 0 && face.Sector() >= sectorCount)
                            return fail("out-of-range sector in face " + std::to_string(faceId) + ".");
                        sectors.set(face.Sector());
                    }
                    materials.set(face.Material());
                }
                blocks.push_back(block);
            }
            if (!blocks.empty())
            {
                candidate->nodes.reserve(blocks.size()*2-1);
                candidate->Build(blocks,0,blocks.size());
            }
            stats.materials = materials.count();
            stats.sectors = sectors.count();
            stats.nodes = candidate->nodes.size();
            stats.indexBytes = candidate->nodes.capacity()*sizeof(Node);
            state_ = std::move(candidate); // Temporary blocks are freed on return.
            return true;
        }
        catch (const std::bad_alloc&)
        { return fail("insufficient memory for mapping metadata/block BVH."); }
    }

    bool CformCollision::ReadVertex(const std::uint32_t index, math::Vector3& vertex) const noexcept
    {
        if (!state_ || index >= state_->statistics.vertices) return false;
        vertex = state_->Vertex(index);
        return true;
    }
    bool CformCollision::ReadFace(const std::uint32_t index, CformFace& face) const noexcept
    {
        if (!state_ || index >= state_->statistics.faces) return false;
        face = state_->Face(index);
        return true;
    }
    bool CformCollision::Raycast(const math::Vector3& start, const math::Vector3& end,
        CformHit& hit) const noexcept
    {
        hit = {};
        if (!state_ || state_->nodes.empty() || !Finite(start) || !Finite(end)) return false;
        const auto origin = Wide(start);
        const auto delta = Wide(end)-origin;
        if (Dot(delta,delta) == 0.0) return false;
        double nearest = 1.0;
        struct Pending { std::uint32_t node; double entry; };
        // Median splitting with at most ceil(UINT32_MAX/512) leaves has depth <= 24.
        // A depth-first traversal therefore needs fewer than 64 pending siblings.
        std::array<Pending,64> stack{};
        std::size_t count = 0;
        double entry = 0.0;
        if (!IntersectBounds(state_->nodes[0].bounds,origin,delta,nearest,entry)) return false;
        stack[count++] = {0,entry};
        bool found = false;
        while (count != 0)
        {
            const auto pending = stack[--count];
            if (pending.entry > nearest) continue;
            const auto& node = state_->nodes[pending.node];
            if (node.count != 0)
            {
                for (std::uint32_t local = 0; local < node.count; ++local)
                {
                    const auto faceId = node.first+local;
                    const auto face = state_->Face(faceId);
                    math::Vector3 normal;
                    if (!IntersectTriangle(origin,delta,state_->Vertex(face.vertices[0]),
                        state_->Vertex(face.vertices[1]),state_->Vertex(face.vertices[2]),nearest,normal)) continue;
                    found = true;
                    hit.face = faceId;
                    hit.normal = normal;
                    hit.material = face.Material();
                    hit.sector = face.Sector();
                    hit.attributes = face.attributes;
                }
                continue;
            }
            double leftEntry = 0.0, rightEntry = 0.0;
            const bool left = IntersectBounds(state_->nodes[node.left].bounds,origin,delta,nearest,leftEntry);
            const bool right = IntersectBounds(state_->nodes[node.right].bounds,origin,delta,nearest,rightEntry);
            if (left && right)
            {
                if (leftEntry <= rightEntry)
                { stack[count++] = {node.right,rightEntry}; stack[count++] = {node.left,leftEntry}; }
                else
                { stack[count++] = {node.left,leftEntry}; stack[count++] = {node.right,rightEntry}; }
            }
            else if (left) stack[count++] = {node.left,leftEntry};
            else if (right) stack[count++] = {node.right,rightEntry};
        }
        if (!found) return false;
        hit.fraction = static_cast<float>(nearest);
        hit.position = {static_cast<float>(origin.x+delta.x*nearest),
            static_cast<float>(origin.y+delta.y*nearest), static_cast<float>(origin.z+delta.z*nearest)};
        return true;
    }
    bool CformCollision::Raycast(const math::Vector3& start, const math::Vector3& end,
        float& fraction, math::Vector3& normal) const noexcept
    {
        CformHit hit;
        if (!Raycast(start,end,hit)) return false;
        fraction = hit.fraction;
        normal = hit.normal;
        return true;
    }
    bool CformCollision::FindGround(const math::Vector3& position,
        const float stepUp, const float probeDown, CformHit& hit) const noexcept
    {
        hit = {};
        if (!std::isfinite(stepUp) || !std::isfinite(probeDown) || stepUp < 0.0f || probeDown <= 0.0f)
            return false;
        return Raycast({position.x,position.y+stepUp,position.z},
            {position.x,position.y-probeDown,position.z},hit);
    }
}
