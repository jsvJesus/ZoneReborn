#pragma once

#include "Core/Math/Vector3.h"
#include "Graphics/SceneRenderData.h"

#include <array>
#include <vector>
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

    enum class LightType : std::uint8_t
    {
        Point = 0,
        Spot = 1,
        Directional = 2
    };

    struct LightObject final
    {
        LightType type = LightType::Point;

        std::string name;

        bool enabled = true;
        bool specular = true;

        std::array<float, 3> position
        {
            0.0f, 2.0f, 0.0f
        };

        std::array<float, 3> direction
        {
            0.0f, -1.0f, 0.0f
        };

        std::array<float, 3> colour
        {
            1.0f, 1.0f, 1.0f
        };

        float intensity = 8.0f;
        float radius = 70.0f;
        float innerRadius = 10.0f;
        float coneAngleDegrees = 35.0f;
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

        [[nodiscard]]
        const std::vector<LightObject>& Lights() const noexcept;

        [[nodiscard]]
        int SelectedLightIndex() const noexcept;

        [[nodiscard]]
        const LightObject* SelectedLight() const noexcept;

        void SelectLight(int index) noexcept;

        void AddLight(
            LightType type,
            const core::math::Vector3& position);

        void UpdateLight(
            std::size_t index,
            const LightObject& light);

        void RemoveSelectedLight();

        [[nodiscard]]
        const std::array<float, 3>& Ambient() const noexcept;

        void SetAmbient(
            const std::array<float, 3>& colour);

        [[nodiscard]]
        client::graphics::StudioLightingData BuildLighting() const;

        [[nodiscard]]
        bool SaveLighting(std::string& error);

        [[nodiscard]]
        bool LoadLighting(std::string& error);

    private:
        std::string name_;
        std::filesystem::path directory_;

        SceneStatistics statistics_{};
        std::shared_ptr<const core::world::xray::CformCollision> collision_;

        std::uint64_t revision_ = 0;

        bool empty_ = true;
        bool dirty_ = false;

        [[nodiscard]]
        std::filesystem::path LightingPath() const;

        std::vector<LightObject> lights_;

        std::array<float, 3> ambient_
        {
            0.0f, 0.0f, 0.0f
        };

        int selectedLightIndex_ = -1;
        std::uint64_t nextLightNumber_ = 1;
    };
}
