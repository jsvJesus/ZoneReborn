#include "Application.h"

#include "Core/Log.h"

#include <exception>
#include <iostream>
#include <Windows.h>

namespace
{
    void WaitOnFailure()
    {
        std::cout
            << '\n'
            << "Press ENTER to close..."
            << std::endl;

        std::cin.get();
    }
}

int main()
{
    SetProcessDPIAware();
    
    int exitCode = 0;

    try
    {
        client::Application
            application;

        exitCode =
            application.Run();
    }
    catch (const std::exception& exception)
    {
        core::Log::Error(
            exception.what());

        exitCode =
            1;
    }
    catch (...)
    {
        core::Log::Error(
            "Unhandled client exception");

        exitCode =
            1;
    }

    if (exitCode !=
        0)
    {
        WaitOnFailure();
    }

    return exitCode;
}