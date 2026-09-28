#pragma once

#include "Graphics/SceneRenderData.h"

#include "Core/Math/Vector3.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace client::world
{
    class Collision final
    {
    public:
        [[nodiscard]]
        bool Build(
            const graphics::SceneRenderData& scene,
            std::string& error);

        void Clear() noexcept;

        [[nodiscard]]
        bool FindSpawn(
            core::math::Vector3& position) const noexcept;

        [[nodiscard]]
        bool FindGround(
            const core::math::Vector3& position,
            float stepUp,
            float probeDown,
            float& groundHeight) const noexcept;

        [[nodiscard]]
        bool BlocksCapsule(
            const core::math::Vector3& position,
            float radius,
            float height) const noexcept;

        [[nodiscard]]
        bool Raycast(
            const core::math::Vector3& start,
            const core::math::Vector3& end,
            float& fraction) const noexcept;

    private:
        struct Triangle final
        {
            core::math::Vector3 a;
            core::math::Vector3 b;
            core::math::Vector3 c;

            core::math::Vector3 normal;

            float minX = 0.0f;
            float maxX = 0.0f;

            float minY = 0.0f;
            float maxY = 0.0f;

            float minZ = 0.0f;
            float maxZ = 0.0f;

            bool terrain = false;
        };

        [[nodiscard]]
        bool GroundAt(
            float x,
            float z,
            float minimumY,
            float maximumY,
            bool terrainOnly,
            float& height) const noexcept;

        [[nodiscard]]
        static std::int32_t CellCoordinate(
            float value) noexcept;

        [[nodiscard]]
        static std::uint64_t CellKey(
            std::int32_t x,
            std::int32_t z) noexcept;

        std::vector<Triangle>
            triangles_;

        std::unordered_map<
            std::uint64_t,
            std::vector<std::uint32_t>>
            cells_;

        float minimumX_ = 0.0f;
        float maximumX_ = 0.0f;

        float minimumY_ = 0.0f;
        float maximumY_ = 0.0f;

        float minimumZ_ = 0.0f;
        float maximumZ_ = 0.0f;

        float terrainMinimumX_ = 0.0f;
        float terrainMaximumX_ = 0.0f;

        float terrainMinimumZ_ = 0.0f;
        float terrainMaximumZ_ = 0.0f;

        bool hasBounds_ = false;
        bool hasTerrainBounds_ = false;
    };
}