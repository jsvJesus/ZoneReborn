#include "Application.h"
#include "Core/Log.h"

#include <exception>

int main()
{
    try
    {
        client::Application application;
        return application.Run();
    }
    catch (const std::exception& exception)
    {
        core::Log::Error(exception.what());
        return 1;
    }
    catch (...)
    {
        core::Log::Error("Unhandled client exception");
        return 1;
    }
}