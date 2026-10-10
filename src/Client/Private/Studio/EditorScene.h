#pragma once

#include "Core/Math/Vector3.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

namespace core::world::xray { class CformCollision; }

namespace studio
{
    struct CollisionProbeStatistics final
    {
        bool performed = false;
        bool hit = false;
        std::uint32_t face = 0xffffffffu;
        std::uint16_t material = 0;
        std::uint16_t sector = 0xffffu;
        float distance = 0.0f;
        core::math::Vector3 position{};
        core::math::Vector3 normal{};
        bool suppressShadows = false;
        bool suppressWallmarks = false;
    };

    struct SceneStatistics final
    {
        std::size_t visuals = 0;
        std::size_t meshes = 0;
        std::size_t vertices = 0;
        std::size_t triangles = 0;
        std::size_t textures = 0;
        std::size_t missingTextures = 0;
        bool homPresent = false;
        std::size_t homTriangles = 0;
        std::size_t homTestedVisuals = 0;
        std::size_t homCulledVisuals = 0;
        bool cformPresent = false;
        std::uint32_t collisionVertices = 0;
        std::uint32_t collisionFaces = 0;
        std::size_t collisionNodes = 0;
        std::size_t collisionMaterials = 0;
        std::size_t collisionSectors = 0;
        std::uint64_t collisionMappedBytes = 0;
        std::size_t collisionIndexBytes = 0;
        CollisionProbeStatistics collisionProbe;
    };

    class EditorScene final
    {
    public:
        EditorScene();

        void New();

        void Open(
            const std::filesystem::path& directory,
            const SceneStatistics& statistics,
            std::shared_ptr<const core::world::xray::CformCollision> collision = {});

        const core::world::xray::CformCollision* Collision() const noexcept;
        void SetCollisionProbe(const CollisionProbeStatistics& probe) noexcept;

        void SetHomFrameStatistics(
            std::size_t testedVisuals,
            std::size_t culledVisuals) noexcept;

        [[nodiscard]]
        const std::string& Name() const noexcept;

        [[nodiscard]]
        const std::filesystem::path& Directory() const noexcept;

        [[nodiscard]]
        const SceneStatistics& Statistics() const noexcept;

        [[nodiscard]]
        std::uint64_t Revision() const noexcept;

        [[nodiscard]]
        bool IsEmpty() const noexcept;

        [[nodiscard]]
        bool IsDirty() const noexcept;

    private:
        std::string name_;
        std::filesystem::path directory_;

        SceneStatistics statistics_{};
        std::shared_ptr<const core::world::xray::CformCollision> collision_;

        std::uint64_t revision_ = 0;

        bool empty_ = true;
        bool dirty_ = false;
    };
}
