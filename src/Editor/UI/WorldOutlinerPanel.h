#pragma once

namespace editor::viewport {
    class TerrainViewport;
}

namespace editor::ui {

    class WorldOutlinerPanel final {
    public:
        void draw(viewport::TerrainViewport& viewport);

    private:
        char search_[96]{};
    };

}
