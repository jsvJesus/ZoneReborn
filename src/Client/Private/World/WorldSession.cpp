#include "World/WorldSession.h"

#include "Character/CharacterRenderDataBuilder.h"
#include "Preview/WorldPreviewLoader.h"
#include "Core/Log.h"

#include <algorithm>

namespace
{
    constexpr float ForcedWorldStartHour =
        12.0f;
}

namespace client::world
{
    bool Session::Load(
        core::Runtime& runtime,
        platform::Window& window,
        graphics::Renderer& renderer,
        const std::string_view spaceName,
        const character::Profile& profile,
        const loading::ProgressCallback& progress,
        std::string& error)
    {
        error.clear();

        loading::Report(
            progress,
            2,
            loading::stage::Preparing);

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
            error,
            progress))
        {
            error =
                "Unable to load world '" +
                std::string(
                    spaceName) +
                "': " +
                error;

            return false;
        }

        loading::Report(
            progress,
            86,
            loading::stage::Collision);

        if (!collision_.Build(
                scene,
                error))
        {
            error =
                "Unable to build world collision: " +
                error;

            return false;
        }

        core::math::Vector3
            spawnPosition;

        if (!collision_.FindSpawn(
                spawnPosition))
        {
            collision_.Clear();

            error =
                "Unable to find player spawn position.";

            return false;
        }

        core::Log::Info(
            std::string(
                "Player spawn: ") +
            std::to_string(
                spawnPosition.x) +
            ", " +
            std::to_string(
                spawnPosition.y) +
            ", " +
            std::to_string(
                spawnPosition.z));

        loading::Report(
            progress,
            90,
            loading::stage::Character);

        if (!playerCatalog_.Load(
                runtime.Resources(),
                error))
        {
            collision_.Clear();

            error =
                "Unable to load player character catalog: " +
                error;

            return false;
        }

        if (!playerState_.ApplyCreatorSet(
                playerCatalog_,
                profile.appearance,
                error))
        {
            playerCatalog_.Clear();
            collision_.Clear();

            error =
                "Unable to apply player appearance: " +
                error;

            return false;
        }

        const bool faceApplied =
            profile.face.faceForm.empty()
                ? playerState_.ResetFace(
                    playerCatalog_,
                    error)
                : playerState_.ApplyFaceState(
                    playerCatalog_,
                    profile.face,
                    error);

        if (!faceApplied)
        {
            playerState_.Reset();
            playerCatalog_.Clear();
            collision_.Clear();

            error =
                "Unable to apply player face: " +
                error;

            return false;
        }

        playerController_.Reset(
            spawnPosition,
            0.0f);

        playerAnimationStateMachine_.Reset(
            playerController_.IsGrounded());

        playerFirstInstance_ =
            scene.instances.size();

        playerInstanceCount_ =
            0;

        character::RenderDataBuilder
            characterBuilder;

        loading::Report(
            progress,
            93,
            loading::stage::Character);

        if (!characterBuilder.Build(
                runtime.Resources(),
                playerCatalog_,
                playerState_,
                playerController_.Transform(),
                scene,
                playerInstanceCount_,
                playerAnimator_,
                error))
        {
            playerAnimator_.Reset();
            playerState_.Reset();
            playerCatalog_.Clear();
            collision_.Clear();

            error =
                "Unable to build player character: " +
                error;

            return false;
        }

        if (scene.sky.enabled)
        {
            scene.sky.definition.startTimeHours =
                ForcedWorldStartHour;

            core::Log::Info(
                "World TimeOfDay overridden to 12:00.");
        }

        loading::Report(
            progress,
            96,
            loading::stage::Renderer);

        if (!renderer.Initialize(
                window.NativeHandle(),
                window.Width(),
                window.Height(),
                error))
        {
            renderer.Shutdown();

            playerAnimator_.Reset();
            playerState_.Reset();
            playerCatalog_.Clear();
            collision_.Clear();

            error =
                "World renderer initialization failed: " +
                error;

            return false;
        }

        loading::Report(
            progress,
            98,
            loading::stage::Renderer);

        if (!renderer.SetScene(
                scene,
                error))
        {
            renderer.Shutdown();

            playerAnimator_.Reset();
            playerState_.Reset();
            playerCatalog_.Clear();
            collision_.Clear();

            error =
                "Unable to activate world scene: " +
                error;

            return false;
        }

        if (playerAnimator_.IsReady())
        {
            std::string animationError;

            if (!playerAnimator_.Update(
                    playerAnimationStateMachine_.Current(),
                    playerAnimationStateMachine_.StateTime(),
                    renderer,
                    animationError))
            {
                core::Log::Warning(
                    std::string(
                        "Unable to initialize player animation: ") +
                    animationError);
            }
        }

        playerCamera_.Reset(
            playerController_.Yaw());

        playerCamera_.UpdateView(
            playerController_.Position(),
            collision_);

        renderer.SetCamera(
            playerCamera_.View());

        loading::Report(
            progress,
            99,
            loading::stage::Finalizing);

        previousUpdateTime_ =
            std::chrono::steady_clock::now();

        spaceName_ =
            spaceName;

        loaded_ =
            true;

        core::Log::Info(
            std::string(
                "Player runtime initialized: instances=") +
            std::to_string(
                playerInstanceCount_));

        core::Log::Info(
            std::string(
                "World initialized: ") +
            spaceName_);

        loading::Report(
            progress,
            100,
            loading::stage::Ready);

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

        float deltaSeconds =
            std::chrono::duration<float>(
                currentTime -
                previousUpdateTime_).
                count();

        previousUpdateTime_ =
            currentTime;

        deltaSeconds =
            std::clamp(
                deltaSeconds,
                0.0f,
                0.05f);

        playerCamera_.UpdateInput(
            window.NativeHandle(),
            window.ConsumeMouseWheelDelta());

        playerController_.Update(
            window.NativeHandle(),
            deltaSeconds,
            playerCamera_.Yaw(),
            collision_);

        const bool animationFinished =
            playerAnimator_.IsReady() &&
            playerAnimator_.IsFinished(
                playerAnimationStateMachine_.Current(),
                playerAnimationStateMachine_.StateTime());

        character::AnimationInput
            animationInput;

        animationInput.moving =
            playerController_.IsMoving();

        animationInput.running =
            playerController_.IsRunning();

        animationInput.grounded =
            playerController_.IsGrounded();

        animationInput.verticalVelocity =
            playerController_.VerticalVelocity();

        playerAnimationStateMachine_.Update(
            deltaSeconds,
            animationInput,
            animationFinished);

        if (playerInstanceCount_ >
            0)
        {
            if (!renderer.SetInstanceTransformRange(
                    playerFirstInstance_,
                    playerInstanceCount_,
                    playerController_.Transform()))
            {
                core::Log::Warning(
                    "Unable to update player transform.");
            }
        }

        playerCamera_.UpdateView(
            playerController_.Position(),
            collision_);

        renderer.SetCamera(
            playerCamera_.View());

        if (playerAnimator_.IsReady())
        {
            std::string
                animationError;

            if (!playerAnimator_.Update(
                    playerAnimationStateMachine_.Current(),
                    playerAnimationStateMachine_.StateTime(),
                    renderer,
                    animationError))
            {
                core::Log::Warning(
                    std::string(
                        "Player animation update failed: ") +
                    animationError);
            }
        }
    }

    void Session::Unload(
        graphics::Renderer& renderer) noexcept
    {
        if (!loaded_)
        {
            return;
        }

        renderer.Shutdown();

        playerAnimator_.Reset();

        playerState_.Reset();

        playerCatalog_.Clear();

        collision_.Clear();

        playerFirstInstance_ =
            0;

        playerInstanceCount_ =
            0;

        playerAnimationStateMachine_.Reset(
            true);

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