#include "Studio/Application.h"

#include "Preview/WorldPreviewLoader.h"

#include "Core/Log.h"

#include <utility>

namespace studio
{
    Application::Application(
        std::string spaceName)
        :
        spaceName_(
            std::move(
                spaceName))
    {
    }

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

        runtimeInitialized_ =
            true;

        std::string error;

        if (!window_.Initialize(
                1600,
                900,
                L"Studio",
                error))
        {
            core::Log::Error(
                error);

            return false;
        }

        windowInitialized_ =
            true;

        core::Log::Info(
            std::string(
                "Studio loading space: ") +
            spaceName_);

        if (!client::preview::LoadWorldPreview(
                runtime_,
                spaceName_,
                scene_,
                error))
        {
            core::Log::Error(
                std::string(
                    "Unable to load space: ") +
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

        rendererInitialized_ =
            true;

        if (!renderer_.SetScene(
                scene_,
                error))
        {
            core::Log::Error(
                error);

            return false;
        }

        camera_.Reset(
            renderer_.SceneCenter(),
            renderer_.SceneRadius());

        renderer_.SetCamera(
            camera_.View());

        previousFrame_ =
            std::chrono::steady_clock::now();

        core::Log::Info(
            "Studio scene ready.");

        return true;
    }

    bool Application::Update()
    {
        const auto now =
            std::chrono::steady_clock::now();

        const float deltaSeconds =
            std::chrono::duration<float>(
                now -
                previousFrame_).
                count();

        previousFrame_ =
            now;

        camera_.Update(
            window_.NativeHandle(),
            window_.ConsumeMouseWheelDelta(),
            deltaSeconds);

        renderer_.SetCamera(
            camera_.View());

        std::string error;

        if (!renderer_.Render(
                error))
        {
            core::Log::Error(
                error);

            return false;
        }

        return true;
    }

    void Application::Shutdown()
    {
        scene_ =
            {};

        if (rendererInitialized_)
        {
            renderer_.Shutdown();

            rendererInitialized_ =
                false;
        }

        if (windowInitialized_)
        {
            window_.Shutdown();

            windowInitialized_ =
                false;
        }

        if (runtimeInitialized_)
        {
            runtime_.Shutdown();

            runtimeInitialized_ =
                false;
        }
    }
}