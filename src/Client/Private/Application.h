#pragma once

#include "Account/AccountSession.h"
#include "Account/AuthService.h"
#include "Account/RememberedLogin.h"
#include "Frontend/Frontend.h"
#include "Platform/Window.h"
#include "States/ClientState.h"

#include "Audio/AudioSystem.h"
#include "Core/Runtime.h"

#include "Graphics/Renderer.h"
#include "Preview/CharacterSelectStage.h"

#include "Character/CharacterCatalog.h"
#include "Character/CharacterRenderDataBuilder.h"
#include "Character/CharacterState.h"
#include "Character/CharacterAnimator.h"
#include "Character/CharacterProfile.h"
#include "Character/CharacterService.h"

#include <cstddef>
#include <chrono>
#include <optional>

namespace client
{
    class Application final
    {
    public:
        Application() = default;

        int Run();

    private:
        [[nodiscard]]
        bool Initialize();

        [[nodiscard]]
        bool Update();

        void Shutdown();

        core::Runtime
            runtime_;

        platform::Window
            window_;

        audio::AudioSystem audio_;

        frontend::OriginalFrontend
            frontend_;

        account::AuthService
            authService_;

        account::AccountSession
            accountSession_;

        account::RememberedLogin
            rememberedLogin_;

        states::ClientState
            state_ =
                states::ClientState::Boot;

        graphics::Renderer
            renderer_;

        preview::CharacterSelectStageData
            characterSelectStage_;

        graphics::SceneRenderData
            characterSelectBaseScene_;

        character::Catalog
            characterCatalog_;

        character::State
            characterState_;

        character::Service
            characterService_;

        std::optional<
            character::Profile>
            characterProfile_;

        bool characterVisible_ =
            true;

        float characterYaw_ =
            0.0f;

        std::size_t
            characterFirstInstance_ =
                0;

        std::size_t
            characterInstanceCount_ =
                0;

        character::Animator
            characterAnimator_;

        std::chrono::steady_clock::time_point
            characterAnimationStart_ =
                std::chrono::steady_clock::now();

        bool rendererInitialized_ =
            false;

        [[nodiscard]]
        bool InitializeCharacterSelectScene(
            std::string& error);

        void ShutdownCharacterSelectScene();

        [[nodiscard]]
        bool RebuildCharacter(
            std::string& error);

        [[nodiscard]]
        core::math::Transform3x4
            CharacterTransform() const noexcept;

        [[nodiscard]]
        bool ApplyCharacterProfile(
            const character::Profile& profile,
            std::string& error);
    };
}