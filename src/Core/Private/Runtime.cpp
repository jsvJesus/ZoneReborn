#include "Core/Runtime.h"

#include "Core/Log.h"

namespace core
{
    Runtime::~Runtime()
    {
        Shutdown();
    }

    bool Runtime::Initialize()
    {
        if (initialized_)
        {
            return true;
        }

        Log::Info("Runtime initialization");

        initialized_ = true;

        Log::Info("Runtime initialized");

        return true;
    }

    void Runtime::Shutdown()
    {
        if (!initialized_)
        {
            return;
        }

        Log::Info("Runtime shutdown");

        initialized_ = false;
    }

    bool Runtime::IsInitialized() const noexcept
    {
        return initialized_;
    }
}