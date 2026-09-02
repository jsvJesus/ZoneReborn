#include "Core/Runtime.h"

#include "Core/Log.h"
#include "Core/Platform/Paths.h"

#include <string>

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

        gameRoot_ = platform::Paths::FindGameRoot();

        if (gameRoot_.empty())
        {
            Log::Error(
                "Game root was not found. "
                "Expected packs/res and packs/sys.");

            return false;
        }

        if (!resources_.Initialize(gameRoot_ / "packs"))
        {
            Log::Error(
                "Resource filesystem initialization failed.");

            gameRoot_.clear();

            return false;
        }

        Log::Info(
            std::string("Game root: ") +
            gameRoot_.string());

        Log::Info(
            std::string("Resources indexed: ") +
            std::to_string(resources_.ResourceCount()));

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

        resources_.Shutdown();
        gameRoot_.clear();

        initialized_ = false;
    }

    bool Runtime::IsInitialized() const noexcept
    {
        return initialized_;
    }

    resources::ResourceFileSystem&
    Runtime::Resources() noexcept
    {
        return resources_;
    }

    const resources::ResourceFileSystem&
    Runtime::Resources() const noexcept
    {
        return resources_;
    }

    const std::filesystem::path&
    Runtime::GameRoot() const noexcept
    {
        return gameRoot_;
    }
}