
#include "Engine/Core/Engine.h"
#include "Engine/Renderer/Device.h"

#include "WorldEditor/WorldEditor.h"
#include "WorldEditor/Terrain/TerrainGenerator.h"
#include "WorldEditor/Terrain/TerrainExporter.h"

#include "Editor/Viewport/TerrainViewport.h"
#include "Editor/WorldGenerator/WorldStoragePanel.h"

#include "Editor/Core/Paths.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include <Windows.h>

#include <exception>
#include <filesystem>
#include <stdexcept>
#include <utility>
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
        windowClass.lpszClassName = L"TerrainEditorWindow";

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
            1500,
            900,
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
        ) : iniPath_(editor::paths::imgui_settings_path().string())
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGui::GetIO().IniFilename = iniPath_.c_str();

        ImGui::GetIO().ConfigFlags |=
            ImGuiConfigFlags_DockingEnable;

        ImGui::StyleColorsDark();

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

}

int main() {
    try {
        engine::initialize();
        world_editor::initialize();

        Window window;

        engine::renderer::Device graphics(
            window.handle()
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

        editor::world_generator::WorldStoragePanel storagePanel;

        world_editor::terrain::Settings settings;
        world_editor::terrain::Settings activeSettings = settings;

        auto terrain = world_editor::terrain::generate(settings);

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
                DispatchMessageW(&message);
            }

            if (!running) {
                break;
            }

            if (IsIconic(window.handle())) {
                Sleep(20);
                continue;
            }

            RECT clientRect{};
            GetClientRect(window.handle(), &clientRect);

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

            ImGui::DockSpaceOverViewport();

            ImGui::Begin("World Generator");

            ImGui::TextUnformatted("Terrain Settings");
            ImGui::Separator();

            ImGui::InputScalar(
                "Seed",
                ImGuiDataType_U32,
                &settings.seed
            );

            ImGui::SliderFloat(
                "Relief (m)",
                &settings.reliefMeters,
                2.0f,
                50.0f
            );

            ImGui::SliderFloat(
                "Macro Scale (m)",
                &settings.macroScaleMeters,
                250.0f,
                1800.0f
            );

            ImGui::SliderFloat(
                "Detail Scale (m)",
                &settings.detailScaleMeters,
                32.0f,
                400.0f
            );

            ImGui::Separator();

            if (ImGui::Button("Generate Terrain")) {
                auto generated = world_editor::terrain::generate(settings);

                viewport.set_terrain(generated);

                terrain = std::move(generated);
                activeSettings = settings;
            }

            ImGui::BeginDisabled(viewport.is_streaming());

            if (ImGui::Button("Export OBJ / PGM")) {
                const auto outputDir =
                    editor::paths::generated_directory() / "terrain";

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
            }

            ImGui::EndDisabled();

            ImGui::Separator();

            if (viewport.is_streaming())
            {
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
            }
            else
            {
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

            storagePanel.draw(
                terrain,
                settings,
                activeSettings,
                viewport
            );

            ImGui::End();

            viewport.draw();

            ImGui::Render();

            graphics.begin_frame();

            ImGui_ImplDX11_RenderDrawData(
                ImGui::GetDrawData()
            );

            graphics.present();
        }

        return 0;
    }
    catch (const std::exception& exception) {
        MessageBoxA(
            nullptr,
            exception.what(),
            "Editor error",
            MB_OK | MB_ICONERROR
        );

        return 1;
    }
}
