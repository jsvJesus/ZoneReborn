
#include "Studio/Application.h"

#include "Core/Log.h"

#include <Windows.h>

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
        {
            return false;
        }

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
        {
            return false;
        }

        if (!editorUI_.Initialize(
                window_.NativeHandle(),
                renderer_.Device(),
                renderer_.Context()))
        {
            core::Log::Error(
                "Unable to initialize Dear ImGui.");

            return false;
        }

        uiInitialized_ = true;

        renderer_.SetFrameOverlay(
            &Application::RenderOverlay,
            this);

        core::Log::Info(
            "Studio editor initialized.");

        core::Log::Info(
            "Startup scene: empty.");

        return true;
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

        renderer_.SetCamera(
            camera_.View());

        core::Log::Info(
            "New empty scene created.");

        return true;
    }

    bool Application::Update()
    {
        editorUI_.BeginFrame(
            scene_,
            renderer_.ViewportImage());

        if (editorUI_.ConsumeNewSceneRequest())
        {
            if (!NewScene())
            {
                return false;
            }
        }

        if (editorUI_.ConsumeExitRequest())
        {
            PostMessageW(
                window_.NativeHandle(),
                WM_CLOSE,
                0,
                0);
        }

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
        {
            return;
        }

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