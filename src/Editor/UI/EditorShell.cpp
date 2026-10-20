#include "Editor/UI/EditorShell.h"

#include "imgui.h"

namespace editor::ui {

    namespace {

        void mode_button(
            EditorShellState& state,
            const char* label,
            EditorMode mode
        ) {
            const bool selected = state.mode == mode;

            if (selected) {
                ImGui::PushStyleColor(
                    ImGuiCol_Button,
                    ImVec4(0.21f, 0.35f, 0.52f, 1.0f)
                );
            }

            if (ImGui::Button(label)) {
                state.mode = mode;
            }

            if (selected) {
                ImGui::PopStyleColor();
            }
        }

        void generator_item(
            EditorShellState& state,
            const char* label,
            GeneratorPage page
        ) {
            if (ImGui::MenuItem(
                label,
                nullptr,
                state.mode == EditorMode::Generator &&
                    state.generatorPage == page
            )) {
                state.mode = EditorMode::Generator;
                state.generatorPage = page;
            }
        }

        const char* active_page(GeneratorPage page) noexcept {
            switch (page) {
            case GeneratorPage::World:
                return "World";
            case GeneratorPage::Layout:
                return "Layout";
            case GeneratorPage::Biomes:
                return "Biomes";
            case GeneratorPage::Vegetation:
                return "Vegetation";
            }

            return "World";
        }

    }

    void draw_editor_menu(EditorShellState& state) {
        if (!ImGui::BeginMainMenuBar()) {
            return;
        }

        ImGui::TextColored(
            ImVec4(0.80f, 0.82f, 0.86f, 1.0f),
            "WORLD EDITOR"
        );
        ImGui::Separator();

        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("World Settings")) {
                ImGui::SetWindowFocus("World Settings");
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Exit")) {
                state.exitRequested = true;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Focus Selected", "F")) {
                state.focusSelection = true;
            }

            if (ImGui::MenuItem("Clear Selection", "Esc")) {
                state.clearSelection = true;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Tools")) {
            generator_item(state, "World Generator", GeneratorPage::World);
            generator_item(state, "Layout Editor", GeneratorPage::Layout);
            generator_item(state, "Biome Editor", GeneratorPage::Biomes);
            generator_item(
                state,
                "Vegetation Editor",
                GeneratorPage::Vegetation
            );
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Settings")) {
            if (ImGui::MenuItem("Reset Workspace")) {
                state.resetLayout = true;
            }

            ImGui::MenuItem(
                "Content Drawer",
                nullptr,
                &state.contentDrawerVisible
            );
            ImGui::MenuItem(
                "Output Log",
                nullptr,
                &state.outputLogVisible
            );
            ImGui::EndMenu();
        }

        ImGui::Separator();
        ImGui::PushID("EditorModes");
        mode_button(state, "Selection", EditorMode::Selection);
        ImGui::SameLine();
        mode_button(state, "Generator", EditorMode::Generator);
        ImGui::SameLine();
        mode_button(state, "Edit", EditorMode::Edit);
        ImGui::PopID();

        if (state.mode == EditorMode::Generator) {
            ImGui::Separator();

            if (ImGui::BeginMenu(active_page(state.generatorPage))) {
                generator_item(state, "World", GeneratorPage::World);
                generator_item(state, "Layout", GeneratorPage::Layout);
                generator_item(state, "Biomes", GeneratorPage::Biomes);
                generator_item(
                    state,
                    "Vegetation",
                    GeneratorPage::Vegetation
                );
                ImGui::EndMenu();
            }
        }

        ImGui::EndMainMenuBar();
    }

}
