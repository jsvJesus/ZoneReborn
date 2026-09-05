#include "Application.h"

#include "Core/Log.h"

#include <exception>
#include <iostream>
#include <string>
#include <utility>

namespace
{
    constexpr const char*
        DefaultSpace =
            "so_origins";

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
    char* argv[])
{
    int exitCode =
        0;

    try
    {
        std::string spaceName =
            DefaultSpace;

        if (argc >=
                2 &&
            argv[1] !=
                nullptr &&
            argv[1][0] !=
                '\0')
        {
            spaceName =
                argv[1];
        }

        client::Application application(
            std::move(
                spaceName));

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