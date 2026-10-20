#include "Engine/Core/Engine.h"
#include "Engine/Renderer/Device.h"

#include "WorldEditor/WorldEditor.h"
#include "WorldEditor/Terrain/TerrainGenerator.h"
#include "WorldEditor/Terrain/TerrainExporter.h"

#include "Editor/Core/Paths.h"
#include "Editor/Viewport/TerrainViewport.h"
#include "Editor/WorldGenerator/MapLayoutPanel.h"
#include "Editor/WorldGenerator/WorldStoragePanel.h"
#include "Editor/WorldGenerator/TerrainReliefPanel.h"
#include "Editor/WorldGenerator/BiomeMaterialsPanel.h"
#include "Editor/WorldGenerator/VegetationPanel.h"
#include "Editor/WorldGenerator/WorldGeneratorPanel.h"

#include "Editor/UI/EditorWorkspace.h"
#include "Editor/UI/EditorShell.h"
#include "Editor/UI/ContentDrawer.h"
#include "Editor/UI/MapInspectorPanel.h"
#include "Editor/UI/OutputLog.h"
#include "Editor/UI/WorldInfoPanel.h"
#include "Editor/UI/WorldOutlinerPanel.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include <Windows.h>

#include <exception>
#include <filesystem>
#include <stdexcept>
#include <string>

extern "C++" IMGUI_IMPL_API LRESULT
ImGui_ImplWin32_WndProcHandler(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
);

namespace {

    LRESULT CALLBACK window_proc(
        HWND window,
        UINT message,
        WPARAM wParam,
        LPARAM lParam
    ) {
        if (ImGui::GetCurrentContext() &&
            ImGui_ImplWin32_WndProcHandler(
                window,
                message,
                wParam,
                lParam
            )) {
            return 1;
        }

        switch (message) {
        case WM_SYSCOMMAND:
            if ((wParam & 0xFFF0) == SC_KEYMENU) {
                return 0;
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }

        return DefWindowProcW(
            window,
            message,
            wParam,
            lParam
        );
    }

    class Window final {
    public:
        Window() {
            instance_ = GetModuleHandleW(nullptr);

            WNDCLASSEXW windowClass{};
            windowClass.cbSize = sizeof(windowClass);
            windowClass.lpfnWndProc = window_proc;
            windowClass.hInstance = instance_;
            windowClass.hCursor = LoadCursorW(
                nullptr,
                IDC_ARROW
            );

            windowClass.lpszClassName =
                L"TerrainEditorWindow";

            if (!RegisterClassExW(&windowClass)) {
                throw std::runtime_error(
                    "Failed to register editor window class"
                );
            }

            handle_ = CreateWindowExW(
                0,
                windowClass.lpszClassName,
                L"ZoneReborn - World Editor",
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                1600,
                960,
                nullptr,
                nullptr,
                instance_,
                nullptr
            );

            if (!handle_) {
                UnregisterClassW(
                    windowClass.lpszClassName,
                    instance_
                );

                throw std::runtime_error(
                    "Failed to create editor window"
                );
            }
        }

        ~Window() {
            if (handle_) {
                DestroyWindow(handle_);
            }

            UnregisterClassW(
                L"TerrainEditorWindow",
                instance_
            );
        }

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        [[nodiscard]] HWND handle() const noexcept {
            return handle_;
        }

        void show() const {
            ShowWindow(handle_, SW_SHOWDEFAULT);
            UpdateWindow(handle_);
        }

    private:
        HINSTANCE instance_ = nullptr;
        HWND handle_ = nullptr;
    };

    class ImGuiSession final {
    public:
        ImGuiSession(
            HWND window,
            ID3D11Device* device,
            ID3D11DeviceContext* context
        ) : iniPath_(
            editor::paths::imgui_settings_path().string()
        ) {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();

            ImGui::GetIO().IniFilename = iniPath_.c_str();

            ImGui::GetIO().ConfigFlags |=
                ImGuiConfigFlags_DockingEnable;

            editor::ui::apply_editor_theme();

            if (!ImGui_ImplWin32_Init(window)) {
                ImGui::DestroyContext();

                throw std::runtime_error(
                    "Failed to initialize ImGui Win32 backend"
                );
            }

            if (!ImGui_ImplDX11_Init(device, context)) {
                ImGui_ImplWin32_Shutdown();
                ImGui::DestroyContext();

                throw std::runtime_error(
                    "Failed to initialize ImGui DX11 backend"
                );
            }
        }

        ~ImGuiSession() {
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
        }

        ImGuiSession(const ImGuiSession&) = delete;
        ImGuiSession& operator=(const ImGuiSession&) = delete;

    private:
        std::string iniPath_;
    };

    void draw_terrain_tools(
        const world_editor::terrain::Heightfield& terrain,
        world_editor::terrain::Settings& settings,
        editor::viewport::TerrainViewport& viewport,
        editor::world_generator::MapLayoutPanel& mapPanel,
        editor::world_generator::TerrainReliefPanel& reliefPanel
    ) {
        static std::string status;
        static bool hasError = false;

        ImGui::Begin("Terrain Tools");

        ImGui::TextColored(
            ImVec4(0.38f, 0.85f, 0.74f, 1.0f),
            "TERRAIN SETTINGS"
        );

        ImGui::TextDisabled(
            "Heightfield / Noise / Relief"
        );

        ImGui::Separator();

        if (ImGui::CollapsingHeader(
            "01  Terrain Parameters",
            ImGuiTreeNodeFlags_DefaultOpen
        )) {
            ImGui::InputScalar(
                "Terrain Seed",
                ImGuiDataType_U32,
                &mapPanel.generation_seeds().terrain
            );

            ImGui::SliderFloat(
                "Relief",
                &settings.reliefMeters,
                2.0f,
                50.0f,
                "%.1f m"
            );

            ImGui::SliderFloat(
                "Macro Scale",
                &settings.macroScaleMeters,
                250.0f,
                1800.0f,
                "%.0f m"
            );

            ImGui::SliderFloat(
                "Detail Scale",
                &settings.detailScaleMeters,
                32.0f,
                400.0f,
                "%.0f m"
            );
        }

        settings.seed = mapPanel.generation_seeds().terrain;
        reliefPanel.draw();

        ImGui::Separator();

        ImGui::TextDisabled(
            "Select Terrain in World Generator to apply these settings."
        );

        ImGui::BeginDisabled(viewport.is_streaming());

        if (ImGui::Button(
            "Export OBJ / PGM",
            ImVec2(-1.0f, 32.0f)
        )) {
            try {
                const auto outputDir =
                    editor::paths::generated_directory() /
                    "terrain";

                std::filesystem::create_directories(
                    outputDir
                );

                world_editor::terrain::save_obj(
                    terrain,
                    outputDir / "terrain.obj"
                );

                world_editor::terrain::save_pgm(
                    terrain,
                    outputDir / "heightmap.pgm"
                );

                status = "Terrain exported";
                hasError = false;
            } catch (const std::exception& error) {
                status = error.what();
                hasError = true;
            }
        }

        ImGui::EndDisabled();

        ImGui::Spacing();
        ImGui::Separator();

        if (viewport.is_streaming()) {
            const auto& world = viewport.streaming_settings();

            ImGui::Text(
                "Samples: %u x %u",
                world.samplesX,
                world.samplesZ
            );

            ImGui::Text(
                "Spacing: %.1f m",
                world.spacingMeters
            );

            ImGui::TextDisabled("Streaming active");
        } else {
            ImGui::Text(
                "Samples: %u x %u",
                terrain.samplesX,
                terrain.samplesZ
            );

            ImGui::Text(
                "Spacing: %.1f m",
                terrain.spacingMeters
            );
        }

        if (!status.empty()) {
            ImGui::TextColored(
                hasError
                    ? ImVec4(1.0f, 0.48f, 0.42f, 1.0f)
                    : ImVec4(0.38f, 0.85f, 0.74f, 1.0f),
                "%s",
                status.c_str()
            );
        }

        ImGui::End();
    }

}

int main() {
    try {
        engine::initialize();
        world_editor::initialize();

        Window window;

        engine::renderer::Device graphics(
            window.handle()
        );

        editor::ui::EditorShellState shell;
        shell.resetLayout = !std::filesystem::exists(
            editor::paths::imgui_settings_path()
        );

        ImGuiSession ui(
            window.handle(),
            graphics.device(),
            graphics.context()
        );

        editor::viewport::TerrainViewport viewport(
            graphics.device(),
            graphics.context()
        );

        editor::world_generator::MapLayoutPanel mapPanel;
        editor::world_generator::WorldStoragePanel storagePanel;
        editor::world_generator::TerrainReliefPanel reliefPanel;
        editor::world_generator::BiomeMaterialsPanel biomePanel;
        editor::world_generator::VegetationPanel vegetationPanel;
        editor::world_generator::WorldGeneratorPanel worldGeneratorPanel;

        editor::ui::WorldOutlinerPanel outlinerPanel;
        editor::ui::MapInspectorPanel inspectorPanel;
        editor::ui::ContentDrawer contentDrawer;
        editor::ui::OutputLog outputLog;
        outputLog.add("Editor workspace ready");
        auto previousMode = shell.mode;
        auto previousPage = shell.generatorPage;
        auto previousSelection = viewport.selection();
        auto previousCameraView = viewport.camera_view();

        world_editor::terrain::Settings settings;
        world_editor::terrain::Settings activeSettings = settings;

        auto terrain = world_editor::terrain::generate(
            settings
        );

        viewport.set_terrain(terrain);

        window.show();

        bool running = true;

        while (running) {
            MSG message{};

            while (PeekMessageW(
                &message,
                nullptr,
                0,
                0,
                PM_REMOVE
            )) {
                if (message.message == WM_QUIT) {
                    running = false;
                    break;
                }

                TranslateMessage(&message);
                DispatchMessageW(
                    &message
                );
            }

            if (!running) {
                break;
            }

            if (IsIconic(window.handle())) {
                Sleep(20);
                continue;
            }

            RECT clientRect{};
            GetClientRect(
                window.handle(),
                &clientRect
            );

            const UINT width = static_cast<UINT>(
                clientRect.right - clientRect.left
            );

            const UINT height = static_cast<UINT>(
                clientRect.bottom - clientRect.top
            );

            if (width == 0 || height == 0) {
                Sleep(20);
                continue;
            }

            graphics.resize(width, height);

            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            editor::ui::draw_editor_menu(shell);
            editor::ui::draw_editor_workspace(shell.resetLayout);

            if (shell.focusSelection) {
                viewport.focus_selection();
                shell.focusSelection = false;
            }

            if (shell.clearSelection) {
                viewport.select({});
                shell.clearSelection = false;
            }

            if (shell.mode != previousMode) {
                const char* modeName = shell.mode == editor::ui::EditorMode::Selection
                    ? "Selection"
                    : shell.mode == editor::ui::EditorMode::Edit
                        ? "Edit"
                        : "Generator";
                outputLog.add(std::string("Mode: ") + modeName);
                previousMode = shell.mode;
            }

            if (shell.generatorPage != previousPage) {
                const char* pageName = shell.generatorPage == editor::ui::GeneratorPage::Layout
                    ? "Layout"
                    : shell.generatorPage == editor::ui::GeneratorPage::Biomes
                        ? "Biomes"
                        : shell.generatorPage == editor::ui::GeneratorPage::Vegetation
                            ? "Vegetation"
                            : "World";
                outputLog.add(std::string("Generator page: ") + pageName);
                previousPage = shell.generatorPage;
            }

            if (shell.mode == editor::ui::EditorMode::Edit) {
                draw_terrain_tools(
                    terrain,
                    settings,
                    viewport,
                    mapPanel,
                    reliefPanel
                );
            } else if (shell.mode == editor::ui::EditorMode::Generator) {
                switch (shell.generatorPage) {
                case editor::ui::GeneratorPage::World:
                    worldGeneratorPanel.draw(
                        terrain,
                        settings,
                        activeSettings,
                        viewport,
                        mapPanel,
                        biomePanel,
                        reliefPanel,
                        outputLog
                    );
                    break;
                case editor::ui::GeneratorPage::Layout:
                    mapPanel.draw(
                        terrain,
                        activeSettings,
                        viewport,
                        storagePanel
                    );
                    break;
                case editor::ui::GeneratorPage::Biomes:
                    biomePanel.draw(
                        activeSettings,
                        viewport,
                        mapPanel.generation_seeds()
                    );
                    break;
                case editor::ui::GeneratorPage::Vegetation:
                    vegetationPanel.draw(
                        terrain,
                        biomePanel.active_layer(),
                        biomePanel.source_fingerprint(),
                        viewport,
                        mapPanel.generation_seeds()
                    );
                    break;
                }
            }

            outlinerPanel.draw(viewport);
            inspectorPanel.draw(viewport);
            editor::ui::draw_world_info(viewport);

            storagePanel.draw(
                terrain,
                settings,
                activeSettings,
                viewport,
                mapPanel,
                reliefPanel,
                biomePanel
            );

            if (shell.contentDrawerVisible) {
                contentDrawer.draw();
            }

            if (shell.outputLogVisible) {
                outputLog.draw();
            }

            mapPanel.synchronize(viewport, activeSettings);
            biomePanel.synchronize(viewport, mapPanel.generation_seeds(), activeSettings);
            vegetationPanel.synchronize(viewport, mapPanel.generation_seeds());

            viewport.set_selection_enabled(
                shell.mode == editor::ui::EditorMode::Selection
            );
            viewport.draw();

            const auto selection = viewport.selection();

            if (selection.kind != previousSelection.kind ||
                selection.id != previousSelection.id) {
                if (selection.valid()) {
                    const char* kind = selection.kind == editor::ui::MapSelectionKind::POI
                        ? "POI "
                        : selection.kind == editor::ui::MapSelectionKind::Road
                            ? "Road "
                            : "Exit ";
                    outputLog.add(std::string("Selected ") + kind +
                        std::to_string(selection.id + 1));
                } else {
                    outputLog.add("Selection cleared");
                }

                previousSelection = selection;
            }

            const auto cameraView = viewport.camera_view();

            if (cameraView != previousCameraView) {
                const char* name = cameraView == editor::viewport::CameraView::Top
                    ? "Top"
                    : cameraView == editor::viewport::CameraView::Front
                        ? "Front"
                        : cameraView == editor::viewport::CameraView::Right
                            ? "Right"
                            : "Perspective";
                outputLog.add(std::string("Camera: ") + name);
                previousCameraView = cameraView;
            }

            ImGui::Render();

            graphics.begin_frame();

            ImGui_ImplDX11_RenderDrawData(
                ImGui::GetDrawData()
            );

            graphics.present();

            if (shell.exitRequested) {
                running = false;
            }
        }

        return 0;
    } catch (const std::exception& exception) {
        MessageBoxA(
            nullptr,
            exception.what(),
            "Editor error",
            MB_OK | MB_ICONERROR
        );

        return 1;
    }
}
