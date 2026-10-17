
#pragma once

#include "WorldEditor/Terrain/TerrainData.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace engine::renderer {
    class TerrainRenderer;
}

namespace editor::viewport {

    struct ChunkCoordinate {
        std::uint32_t x = 0;
        std::uint32_t z = 0;

        bool operator==(const ChunkCoordinate&) const = default;
    };

    class TerrainWorldStreamer final {
    public:
        explicit TerrainWorldStreamer(
            std::filesystem::path directory
        );

        void update(
            float focusX,
            float focusZ,
            engine::renderer::TerrainRenderer& renderer
        );

        void set_radius(int radius) noexcept;

        [[nodiscard]] int radius() const noexcept {
            return radius_;
        }

        [[nodiscard]] std::size_t loaded_count() const noexcept {
            return loaded_.size();
        }

        [[nodiscard]] std::uint32_t total_count() const noexcept {
            return countX_ * countZ_;
        }

        [[nodiscard]] std::uint64_t total_loads() const noexcept {
            return totalLoads_;
        }

        [[nodiscard]] std::uint64_t total_unloads() const noexcept {
            return totalUnloads_;
        }

        [[nodiscard]] bool paused() const noexcept {
            return paused_;
        }

        [[nodiscard]] const std::string& error() const noexcept {
            return error_;
        }

        [[nodiscard]] const world_editor::terrain::Settings&
        settings() const noexcept {
            return settings_;
        }

    private:
        [[nodiscard]] bool is_loaded(
            const ChunkCoordinate& coordinate
        ) const;

        std::filesystem::path directory_;
        world_editor::terrain::Settings settings_;

        std::vector<ChunkCoordinate> loaded_;

        std::uint32_t countX_ = 0;
        std::uint32_t countZ_ = 0;

        std::uint64_t totalLoads_ = 0;
        std::uint64_t totalUnloads_ = 0;

        int radius_ = 1;
        bool paused_ = false;

        std::string error_;
    };

}
