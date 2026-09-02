#include "Application.h"
#include "Core/Log.h"

namespace server
{
    int Application::Run()
    {
        if (!Initialize())
        {
            core::Log::Error("Server initialization failed");
            return 1;
        }

        core::Log::Info("Server started");

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