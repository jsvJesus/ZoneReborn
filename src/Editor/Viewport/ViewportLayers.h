#pragma once

namespace editor::viewport {

    struct ViewportLayers {
        bool poi = true;
        bool roads = false;
        bool exits = true;
        bool labels = true;
        bool water = true;
    };

}
