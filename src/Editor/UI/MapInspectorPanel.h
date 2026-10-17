#pragma once

namespace editor::viewport {
    class TerrainViewport;
}

namespace editor::ui {

    class MapInspectorPanel final {
    public:
        void draw(const viewport::TerrainViewport& viewport) const;
    };

}
