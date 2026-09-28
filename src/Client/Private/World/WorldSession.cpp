#include "World/WorldSession.h"

#include "Preview/WorldPreviewLoader.h"

#include "Core/Log.h"

namespace
{
    constexpr std::string_view TutorialWarehouseSpace =
        "start_tutorial_warehouse";

    constexpr float TutorialWarehouseStartHour =
        12.0f;
}

namespace client::world
{
    bool Session::Load(
        core::Runtime& runtime,
        platform::Window& window,
        graphics::Renderer& renderer,
        const std::string_view spaceName,
        std::string& error)
    {
        error.clear();

        if (spaceName.empty())
        {
            error =
                "World space name is empty.";

            return false;
        }

        if (loaded_)
        {
            Unload(
                renderer);
        }

        graphics::SceneRenderData
            scene;

        core::Log::Info(
            std::string(
                "Loading world: ") +
            std::string(
                spaceName));

        if (!preview::LoadWorldPreview(
                runtime,
                spaceName,
                scene,
                error))
        {
            error =
                "Unable to load world '" +
                std::string(spaceName) +
                "': " +
                error;

            return false;
        }

        if (spaceName == TutorialWarehouseSpace &&
            scene.sky.enabled)
        {
            scene.sky.definition.startTimeHours =
                TutorialWarehouseStartHour;

            core::Log::Info(
                "Tutorial warehouse TimeOfDay overridden to 12:00.");
        }

        if (!renderer.Initialize(
                window.NativeHandle(),
                window.Width(),
                window.Height(),
                error))
        {
            renderer.Shutdown();

            error =
                "World renderer initialization failed: " +
                error;

            return false;
        }

        if (!renderer.SetScene(
                scene,
                error))
        {
            renderer.Shutdown();

            error =
                "Unable to activate world scene: " +
                error;

            return false;
        }

        cameraController_.Reset(
            renderer.SceneCenter(),
            renderer.SceneRadius());

        renderer.SetCamera(
            cameraController_.View());

        previousUpdateTime_ =
            std::chrono::steady_clock::now();

        spaceName_ =
            spaceName;

        loaded_ =
            true;

        core::Log::Info(
            std::string(
                "World initialized: ") +
            spaceName_);

        return true;
    }

    void Session::Update(
        platform::Window& window,
        graphics::Renderer& renderer) noexcept
    {
        if (!loaded_)
        {
            return;
        }

        const auto currentTime =
            std::chrono::steady_clock::now();

        const float deltaSeconds =
            std::chrono::duration<float>(
                currentTime -
                previousUpdateTime_).
                count();

        previousUpdateTime_ =
            currentTime;

        cameraController_.Update(
            window.NativeHandle(),
            window.ConsumeMouseWheelDelta(),
            deltaSeconds);

        renderer.SetCamera(
            cameraController_.View());
    }

    void Session::Unload(
        graphics::Renderer& renderer) noexcept
    {
        if (!loaded_)
        {
            return;
        }

        renderer.Shutdown();

        loaded_ =
            false;

        spaceName_.clear();

        core::Log::Info(
            "World unloaded.");
    }

    bool Session::IsLoaded() const noexcept
    {
        return loaded_;
    }

    std::string_view
    Session::SpaceName() const noexcept
    {
        return spaceName_;
    }
}
