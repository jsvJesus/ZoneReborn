#include "Application.h"
#include "Core/Log.h"

namespace client
{
    int Application::Run()
    {
        if (!Initialize())
        {
            core::Log::Error("Client initialization failed");
            return 1;
        }

        core::Log::Info("Client started");

        Shutdown();

        return 0;
    }

    bool Application::Initialize()
    {
        return runtime_.Initialize();
    }

    void Application::Shutdown()
    {
        runtime_.Shutdown();
    }
}