#include "Application.h"

#include "Core/Log.h"

#include <exception>
#include <iostream>

namespace
{
    void WaitForExit()
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
    int exitCode = 0;

    try
    {
        client::Application application;

        exitCode =
            application.Run();
    }
    catch (const std::exception& exception)
    {
        core::Log::Error(
            exception.what());

        exitCode = 1;
    }
    catch (...)
    {
        core::Log::Error(
            "Unhandled client exception");

        exitCode = 1;
    }

    WaitForExit();

    return exitCode;
}