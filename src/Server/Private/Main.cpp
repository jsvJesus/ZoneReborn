#include "Application.h"
#include "Core/Log.h"

#include <exception>

int main()
{
    try
    {
        server::Application application;
        return application.Run();
    }
    catch (const std::exception& exception)
    {
        core::Log::Error(exception.what());
        return 1;
    }
    catch (...)
    {
        core::Log::Error("Unhandled server exception");
        return 1;
    }
}