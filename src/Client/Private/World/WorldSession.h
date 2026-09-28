#pragma once

#include "Character/CharacterAnimator.h"
#include "Character/CharacterCatalog.h"
#include "Character/CharacterProfile.h"
#include "Character/CharacterState.h"
#include "Character/CharacterAnimationStateMachine.h"

#include "Graphics/Renderer.h"

#include "Platform/Window.h"

#include "Player/PlayerController.h"
#include "Player/ThirdPersonCamera.h"

#include "World/WorldCollision.h"

#include "Core/Runtime.h"

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>

namespace client::world
{
    class Session final
    {
    public:
        [[nodiscard]]
        bool Load(
            core::Runtime& runtime,
            platform::Window& window,
            graphics::Renderer& renderer,
            std::string_view spaceName,
            const character::Profile& profile,
            std::string& error);

        void Update(
            platform::Window& window,
            graphics::Renderer& renderer) noexcept;

        void Unload(
            graphics::Renderer& renderer) noexcept;

        [[nodiscard]]
        bool IsLoaded() const noexcept;

        [[nodiscard]]
        std::string_view SpaceName() const noexcept;

    private:
        Collision
            collision_;

        player::Controller
            playerController_;

        player::ThirdPersonCamera
            playerCamera_;

        character::Catalog
            playerCatalog_;

        character::State
            playerState_;

        character::Animator
            playerAnimator_;

        character::AnimationStateMachine
            playerAnimationStateMachine_;

        std::size_t
            playerFirstInstance_ =
                0;

        std::size_t
            playerInstanceCount_ =
                0;

        std::chrono::steady_clock::time_point
            previousUpdateTime_ =
                std::chrono::steady_clock::now();

        std::string
            spaceName_;

        bool loaded_ =
            false;
    };
}