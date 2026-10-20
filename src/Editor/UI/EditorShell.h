#pragma once

namespace editor::ui {

    enum class EditorMode {
        Selection,
        Generator,
        Edit
    };

    enum class GeneratorPage {
        World,
        Layout,
        Biomes,
        Vegetation
    };

    struct EditorShellState {
        EditorMode mode = EditorMode::Selection;
        GeneratorPage generatorPage = GeneratorPage::World;
        bool resetLayout = false;
        bool exitRequested = false;
        bool contentDrawerVisible = true;
        bool outputLogVisible = true;
    };

    void draw_editor_menu(EditorShellState& state);

}
