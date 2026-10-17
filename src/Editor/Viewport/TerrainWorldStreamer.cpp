
#include "Editor/Viewport/TerrainWorldStreamer.h"

#include "Engine/Renderer/TerrainChunkMesh.h"
#include "Engine/Renderer/TerrainRenderer.h"
#include "WorldEditor/Terrain/TerrainWorldStorage.h"

#include <algorithm>
#include <cmath>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace editor::viewport {

    TerrainWorldStreamer::TerrainWorldStreamer(
        std::filesystem::path directory
    ) : directory_(std::move(directory)) {
        settings_ = world_editor::terrain::read_world_settings(directory_);

        countX_ = (settings_.samplesX - 2) /
            engine::renderer::TerrainChunkCells + 1;

        countZ_ = (settings_.samplesZ - 2) /
            engine::renderer::TerrainChunkCells + 1;
    }

    void TerrainWorldStreamer::set_radius(int radius) noexcept {
        radius_ = std::clamp(radius, 1, 4);
    }

    bool TerrainWorldStreamer::is_loaded(
        const ChunkCoordinate& coordinate
    ) const {
        return std::find(
            loaded_.begin(),
            loaded_.end(),
            coordinate
        ) != loaded_.end();
    }

    void TerrainWorldStreamer::update(
        float focusX,
        float focusZ,
        engine::renderer::TerrainRenderer& renderer
    ) {
        if (paused_ || !std::isfinite(focusX) || !std::isfinite(focusZ)) {
            return;
        }

        const float chunkSize =
            engine::renderer::TerrainChunkCells * settings_.spacingMeters;

        const float localX = std::clamp(
            (focusX - settings_.originX) / chunkSize,
            0.0f,
            static_cast<float>(countX_ - 1)
        );

        const float localZ = std::clamp(
            (focusZ - settings_.originZ) / chunkSize,
            0.0f,
            static_cast<float>(countZ_ - 1)
        );

        const int centerX = static_cast<int>(localX);
        const int centerZ = static_cast<int>(localZ);

        const int unloadRadius = radius_ + 1;

        for (auto it = loaded_.begin(); it != loaded_.end();) {
            const int dx = static_cast<int>(it->x) - centerX;
            const int dz = static_cast<int>(it->z) - centerZ;

            if (std::abs(dx) > unloadRadius ||
                std::abs(dz) > unloadRadius) {
                renderer.remove_stream_chunk(it->x, it->z);
                it = loaded_.erase(it);
                ++totalUnloads_;
            } else {
                ++it;
            }
        }

        std::vector<ChunkCoordinate> candidates;

        const int minimumX = (std::max)(0, centerX - radius_);
        const int minimumZ = (std::max)(0, centerZ - radius_);

        const int maximumX = (std::min)(
            static_cast<int>(countX_) - 1,
            centerX + radius_
        );

        const int maximumZ = (std::min)(
            static_cast<int>(countZ_) - 1,
            centerZ + radius_
        );

        for (int z = minimumZ; z <= maximumZ; ++z) {
            for (int x = minimumX; x <= maximumX; ++x) {
                const ChunkCoordinate coordinate{
                    static_cast<std::uint32_t>(x),
                    static_cast<std::uint32_t>(z)
                };

                if (!is_loaded(coordinate)) {
                    candidates.push_back(coordinate);
                }
            }
        }

        std::sort(
            candidates.begin(),
            candidates.end(),
            [centerX, centerZ](
                const ChunkCoordinate& left,
                const ChunkCoordinate& right
            ) {
                const auto distance = [centerX, centerZ](
                    const ChunkCoordinate& coordinate
                ) {
                    const int dx = static_cast<int>(coordinate.x) - centerX;
                    const int dz = static_cast<int>(coordinate.z) - centerZ;

                    return dx * dx + dz * dz;
                };

                return distance(left) < distance(right);
            }
        );

        if (candidates.empty()) {
            return;
        }

        const ChunkCoordinate next = candidates.front();

        try {
            const auto chunk = world_editor::terrain::read_world_chunk(
                directory_,
                settings_,
                next.x,
                next.z
            );

            renderer.add_stream_chunk(
                next.x,
                next.z,
                {
                    chunk.samplesX,
                    chunk.samplesZ,
                    chunk.originX,
                    chunk.originZ,
                    chunk.spacingMeters,
                    std::span<const float>(chunk.heights)
                }
            );

            loaded_.push_back(next);
            ++totalLoads_;
        } catch (const std::exception& exception) {
            error_ = exception.what();
            paused_ = true;
        }
    }
}
