
#if defined(STUDIO_BUILD)

#include "Studio/Application.h"

#else

#include "Application.h"

#endif

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
#if defined(STUDIO_BUILD)

        studio::Application application;
        exitCode = application.Run();

#elif defined(FINAL_BUILD)

        client::Application application;
        exitCode = application.Run();

#else

#error Build configuration is not defined.

#endif
    }
    catch (const std::exception& exception)
    {
        core::Log::Error(
            exception.what());

        exitCode = 1;
    }
    catch (...)
    {
#if defined(STUDIO_BUILD)

        core::Log::Error(
            "Unhandled Studio exception");

#else

        core::Log::Error(
            "Unhandled client exception");

#endif

        exitCode = 1;
    }

    if (exitCode != 0)
    {
        WaitOnFailure();
    }

    return exitCode;
}