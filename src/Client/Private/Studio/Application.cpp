#include "Studio/Application.h"

#include "Core/Log.h"
#include "Core/Platform/Paths.h"
#include "Core/World/XRay/XRayLevelLoader.h"

#include "Preview/XRayLevelRenderDataBuilder.h"

#include <Windows.h>

#include <chrono>
#include <string>

namespace studio
{
    int Application::Run()
    {
        if (!Initialize())
        {
            Shutdown();
            return 1;
        }

        while (window_.ProcessMessages())
        {
            if (!Update())
            {
                Shutdown();
                return 2;
            }
        }

        Shutdown();
        return 0;
    }

    bool Application::Initialize()
    {
        if (!runtime_.Initialize())
            return false;

        runtimeInitialized_ = true;

        std::string error;

        if (!window_.Initialize(
            1600,
            900,
            L"Studio",
            error))
        {
            core::Log::Error(error);
            return false;
        }

        windowInitialized_ = true;

        if (!renderer_.Initialize(
            window_.NativeHandle(),
            window_.Width(),
            window_.Height(),
            error))
        {
            core::Log::Error(error);
            return false;
        }

        rendererInitialized_ = true;

        if (!NewScene())
            return false;

        if (!editorUI_.Initialize(
            window_.NativeHandle(),
            renderer_.Device(),
            renderer_.Context()))
        {
            core::Log::Error(
                "Unable to initialize editor interface.");

            return false;
        }

        uiInitialized_ = true;

        renderer_.SetFrameOverlay(
            &Application::RenderOverlay,
            this);

        RefreshLevels();

        previousFrame_ =
            std::chrono::steady_clock::now();

        core::Log::Info(
            "Studio editor initialized.");

        return true;
    }

    void Application::RefreshLevels()
    {
        catalog_.Refresh(
            runtime_.GameRoot(),
            core::platform::Paths::ExecutableDirectory());

        const std::string message =
            "Available X-Ray levels: " +
            std::to_string(catalog_.Entries().size());

        core::Log::Info(message);
        editorUI_.AddConsoleMessage(message);

        for (const auto& root : catalog_.Roots())
        {
            editorUI_.AddConsoleMessage(
                "Level directory: " +
                PathToUtf8(root));
        }

        if (catalog_.Roots().empty())
        {
            editorUI_.AddConsoleMessage(
                "WARNING: gamedata/levels was not found.");
        }
    }

    bool Application::NewScene()
    {
        std::string error;

        if (!renderer_.BeginStreamedScene(error))
        {
            core::Log::Error(
                "Unable to clear scene: " + error);

            return false;
        }

        scene_.New();

        camera_.Reset(
            {0.0f, 0.0f, 0.0f},
            10.0f);

        renderer_.SetCamera(camera_.View());

        if (uiInitialized_)
        {
            editorUI_.AddConsoleMessage(
                "New empty scene created.");
        }

        return true;
    }

    void Application::OpenLevel(
        const std::filesystem::path& directory)
    {
        const std::string levelName =
            PathToUtf8(directory.filename());

        const std::string startMessage =
            "Loading X-Ray level: " + levelName;

        core::Log::Info(startMessage);
        editorUI_.AddConsoleMessage(startMessage);

        core::world::xray::LevelData level;
        core::world::xray::LevelLoader loader;

        std::string error;

        if (!loader.Load(directory, level, error))
        {
            const std::string message =
                "Level parse failed: " + error;

            core::Log::Error(message);
            editorUI_.AddConsoleMessage(message);

            return;
        }

        core::Log::Info(
            "Level metadata parsed successfully.");

        client::preview::XRayLevelRenderStatistics
            renderStatistics;

        if (!client::preview::StreamXRayLevelRenderData(
            level,
            renderer_,
            renderStatistics,
            error))
        {
            const std::string message =
                "Level rendering failed: " + error;

            core::Log::Error(message);
            editorUI_.AddConsoleMessage(message);

            std::string cleanupError;

            if (!NewScene())
            {
                core::Log::Error(
                    "Unable to reset scene after load failure.");
            }

            return;
        }

        SceneStatistics statistics;

        statistics.visuals =
            renderStatistics.staticVisualCount;

        statistics.meshes =
            renderStatistics.meshCount;

        statistics.vertices =
            renderStatistics.vertexCount;

        statistics.triangles =
            renderStatistics.triangleCount;

        statistics.textures =
            renderStatistics.loadedTextureCount;

        statistics.missingTextures =
            renderStatistics.missingTextureCount;

        statistics.homPresent = level.homPresent;
        statistics.homTriangles = level.homTriangles.size();

        scene_.Open(directory, statistics);

        camera_.Reset(
            renderer_.SceneCenter(),
            renderer_.SceneRadius());

        renderer_.SetCamera(
            camera_.View());

        const std::string message =
            "Level loaded: " +
            levelName +
            " | meshes=" +
            std::to_string(statistics.meshes) +
            " | triangles=" +
            std::to_string(statistics.triangles) +
            " | textures=" +
            std::to_string(statistics.textures) +
            " | missing textures=" +
            std::to_string(statistics.missingTextures);

        core::Log::Info(message);
        editorUI_.AddConsoleMessage(message);
        const std::string homMessage = !level.homPresent
            ? "HOM: level.hom is absent; occlusion culling disabled."
            : "HOM: loaded " + std::to_string(level.homTriangles.size()) +
                " occluder triangles; occlusion culling " +
                (level.homTriangles.empty() ? "disabled (empty map)." : "enabled.");
        core::Log::Info(homMessage);
        editorUI_.AddConsoleMessage(homMessage);
    }

    bool Application::Update()
    {
        const auto now =
            std::chrono::steady_clock::now();

        const float deltaSeconds =
            std::chrono::duration<float>(
                now - previousFrame_).count();

        previousFrame_ = now;

        if (window_.Width() == 0 ||
            window_.Height() == 0)
        {
            return true;
        }

        std::string resizeError;

        if (!renderer_.Resize(
                window_.Width(),
                window_.Height(),
                resizeError))
        {
            core::Log::Error(
                "Unable to resize editor renderer: " + resizeError);

            return false;
        }

        const auto homStatistics = renderer_.HomStatistics();
        scene_.SetHomFrameStatistics(
            homStatistics.testedVisuals, homStatistics.culledVisuals);

        editorUI_.BeginFrame(
            scene_,
            catalog_,
            renderer_.ViewportImage());

        if (editorUI_.ConsumeRefreshLevelsRequest())
        {
            RefreshLevels();
        }

        if (editorUI_.ConsumeNewSceneRequest())
        {
            if (!NewScene())
                return false;
        }

        std::filesystem::path selectedLevel;

        if (editorUI_.ConsumeOpenLevelRequest(
            selectedLevel))
        {
            OpenLevel(selectedLevel);
        }

        if (editorUI_.ConsumeExitRequest())
        {
            PostMessageW(
                window_.NativeHandle(),
                WM_CLOSE,
                0,
                0);
        }

        const float mouseWheel =
            editorUI_.ViewportWheel();

        const bool viewportActive =
            editorUI_.ViewportHovered();

        camera_.Update(
            window_.NativeHandle(),
            mouseWheel,
            deltaSeconds,
            viewportActive,
            &editorUI_.ViewportRectangle());

        renderer_.SetCamera(
            camera_.View());

        std::string error;

        if (!renderer_.Render(error))
        {
            core::Log::Error(error);
            return false;
        }

        return true;
    }

    void Application::RenderOverlay(
        void* userData)
    {
        if (!userData)
            return;

        auto* application =
            static_cast<Application*>(userData);

        application->editorUI_.Render();
    }

    void Application::Shutdown()
    {
        if (rendererInitialized_)
        {
            renderer_.SetFrameOverlay(
                nullptr,
                nullptr);
        }

        if (uiInitialized_)
        {
            editorUI_.Shutdown();
            uiInitialized_ = false;
        }

        if (rendererInitialized_)
        {
            renderer_.Shutdown();
            rendererInitialized_ = false;
        }

        if (windowInitialized_)
        {
            window_.Shutdown();
            windowInitialized_ = false;
        }

        if (runtimeInitialized_)
        {
            runtime_.Shutdown();
            runtimeInitialized_ = false;
        }
    }
}
