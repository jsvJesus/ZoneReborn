#pragma once

#include "Core/World/Particles/ParticleCollisionQuery.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

namespace core::world::xray
{
    // On-disk CDB::TRI, without compiler-dependent bitfields.
    struct CformFace final
    {
        std::array<std::uint32_t, 3> vertices{};
        std::uint32_t attributes = 0;

        std::uint16_t Material() const noexcept { return static_cast<std::uint16_t>(attributes & 0x3fffu); }
        std::uint16_t Sector() const noexcept { return static_cast<std::uint16_t>(attributes >> 16u); }
        bool SuppressShadows() const noexcept { return (attributes & 0x4000u) != 0; }
        bool SuppressWallmarks() const noexcept { return (attributes & 0x8000u) != 0; }
    };
    static_assert(sizeof(CformFace) == 16u);

    struct CformHit final
    {
        float fraction = 1.0f;
        math::Vector3 position{};
        math::Vector3 normal{};
        std::uint32_t face = 0xffffffffu;
        std::uint16_t material = 0;
        std::uint16_t sector = 0xffffu;
        std::uint32_t attributes = 0;
    };

    struct CformStatistics final
    {
        std::uint32_t vertices = 0;
        std::uint32_t faces = 0;
        std::size_t blocks = 0;
        std::size_t nodes = 0;
        std::size_t materials = 0;
        std::size_t sectors = 0;
        std::uint64_t mappedBytes = 0;
        std::size_t indexBytes = 0;
    };

    // Studio/Release loader: mapped native arrays, no expanded triangles.
    // Failed loads leave the old resource intact; queries are read-only.
    // Load/Clear require exclusive access (do not mutate during concurrent queries).
    class CformCollision final : public particles::ParticleCollisionQuery
    {
    public:
        CformCollision();
        ~CformCollision() override;
        CformCollision(const CformCollision&) = delete;
        CformCollision& operator=(const CformCollision&) = delete;

        bool Load(const std::filesystem::path& file, std::size_t sectorCount,
            std::string& error);
        void Clear() noexcept;
        CformStatistics Statistics() const noexcept;
        bool ReadVertex(std::uint32_t index, math::Vector3& vertex) const noexcept;
        bool ReadFace(std::uint32_t index, CformFace& face) const noexcept;
        bool Raycast(const math::Vector3& start, const math::Vector3& end,
            CformHit& hit) const noexcept;
        bool Raycast(const math::Vector3& start, const math::Vector3& end,
            float& fraction, math::Vector3& normal) const noexcept override;
        bool FindGround(const math::Vector3& position, float stepUp,
            float probeDown, CformHit& hit) const noexcept;

    private:
        struct State;
        std::unique_ptr<State> state_;
    };
}
