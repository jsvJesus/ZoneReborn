#include "Application.h"

#include "Preview/WorldPreviewLoader.h"

#include "Core/Log.h"

#include <string>

namespace client
{
    int Application::Run()
    {
        if (!Initialize())
        {
            core::Log::Error(
                "Client initialization failed");

            Shutdown();

            return 1;
        }

        core::Log::Info(
            "World render loop started");

        std::string error;

        while (window_.ProcessMessages())
        {
            if (!renderer_.Render(
                    error))
            {
                core::Log::Error(
                    error);

                Shutdown();

                return 2;
            }
        }

        core::Log::Info(
            "World render loop stopped");

        Shutdown();

        return 0;
    }

    bool Application::Initialize()
    {
        if (!runtime_.Initialize())
        {
            return false;
        }

        graphics::SceneRenderData scene;

        std::string error;

        core::Log::Info(
            "Loading so_origins world");

        if (!preview::LoadWorldPreview(
                runtime_,
                scene,
                error))
        {
            core::Log::Error(
                std::string(
                    "Unable to load world: ") +
                error);

            return false;
        }

        if (!window_.Initialize(
                1600,
                900,
                L"so_origins World Preview",
                error))
        {
            core::Log::Error(
                error);

            return false;
        }

        if (!renderer_.Initialize(
                window_.NativeHandle(),
                window_.Width(),
                window_.Height(),
                error))
        {
            core::Log::Error(
                error);

            return false;
        }

        if (!renderer_.SetScene(
                scene,
                error))
        {
            core::Log::Error(
                error);

            return false;
        }

        core::Log::Info(
            "so_origins world initialized");

        return true;
    }

    void Application::Shutdown()
    {
        renderer_.Shutdown();
        window_.Shutdown();
        runtime_.Shutdown();
    }
}