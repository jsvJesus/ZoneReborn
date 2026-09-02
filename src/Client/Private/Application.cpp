#include "Application.h"

#include "Preview/ModelPreviewLoader.h"

#include "Core/Assets/MeshData.h"
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
            "Render loop started");

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
            "Render loop stopped");

        Shutdown();

        return 0;
    }

    bool Application::Initialize()
    {
        if (!runtime_.Initialize())
        {
            return false;
        }

        core::assets::MeshData mesh;

        std::string error;

        if (!preview::LoadModelPreview(
                runtime_,
                mesh,
                error))
        {
            core::Log::Error(
                std::string(
                    "Unable to load preview mesh: ") +
                error);

            return false;
        }

        if (!window_.Initialize(
                1280,
                720,
                L"Resource Preview",
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

        if (!renderer_.SetMesh(
                mesh,
                error))
        {
            core::Log::Error(
                error);

            return false;
        }

        core::Log::Info(
            "D3D11 renderer initialized");

        return true;
    }

    void Application::Shutdown()
    {
        renderer_.Shutdown();
        window_.Shutdown();
        runtime_.Shutdown();
    }
}