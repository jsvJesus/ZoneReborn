#if defined(STUDIO_BUILD)

#include "Studio/Application.h"

#else

#include "Application.h"

#endif

#include "Core/Log.h"

#include <exception>
#include <iostream>
#include <string>

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

int main(
    const int argc,
    char** argv)
{
    SetProcessDPIAware();

    int exitCode =
        0;

    try
    {
#if defined(STUDIO_BUILD)

        std::string spaceName =
            "shop_portals_apartments"; // load map for test

        if (argc >= 2 &&
            argv[1] != nullptr &&
            argv[1][0] != '\0')
        {
            spaceName =
                argv[1];
        }

        studio::Application
            application(
                std::move(
                    spaceName));

        exitCode =
            application.Run();

#elif defined(FINAL_BUILD)

        client::Application
            application;

        exitCode =
            application.Run();

#else

#error Build configuration is not defined.

#endif
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
#if defined(STUDIO_BUILD)

        core::Log::Error(
            "Unhandled Studio exception");

#else

        core::Log::Error(
            "Unhandled client exception");

#endif

        exitCode =
            1;
    }

    if (exitCode != 0)
    {
        WaitOnFailure();
    }

    return exitCode;
}