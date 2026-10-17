#pragma once

#include <cstdint>

namespace editor::ui {

    enum class MapSelectionKind : std::uint8_t {
        None,
        POI,
        Exit
    };

    struct MapSelection {
        MapSelectionKind kind = MapSelectionKind::None;
        std::uint32_t id = 0;

        [[nodiscard]] bool valid() const noexcept {
            return kind != MapSelectionKind::None;
        }
    };

}
