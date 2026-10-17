#pragma once

#include "WorldEditor/Terrain/TerrainRelief.h"

#include <filesystem>

namespace editor::world_generator {

    class TerrainReliefPanel final {
    public:
        void draw();

        [[nodiscard]] world_editor::terrain::Heightfield generate(
            const world_editor::terrain::Settings& settings
        );

        void save_for_world(
            const world_editor::terrain::Heightfield& terrain,
            const std::filesystem::path& directory
        ) const;

        void restore_from_world(
            const world_editor::terrain::Heightfield& terrain,
            const std::filesystem::path& directory
        );

        [[nodiscard]] static bool has_saved_profile(
            const std::filesystem::path& directory
        );

        void reset_to_original() noexcept;

    private:
        world_editor::terrain::ReliefProfile pending_;
        world_editor::terrain::ReliefProfile active_;
    };

}
