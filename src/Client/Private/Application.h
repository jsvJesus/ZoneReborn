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
#include "Preview/CharacterDummyRenderDataBuilder.h"

#include <cstddef>

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

        preview::CharacterDummyAppearance
            characterDummyAppearance_;

        bool characterDummyVisible_ =
            true;

        float characterDummyYaw_ =
            0.0f;

        std::size_t
            characterDummyFirstInstance_ =
                0;

        std::size_t
            characterDummyInstanceCount_ =
                0;

        bool rendererInitialized_ =
            false;

        [[nodiscard]]
        bool InitializeCharacterSelectScene(
            std::string& error);

        void ShutdownCharacterSelectScene();

        [[nodiscard]]
        bool RebuildCharacterDummy(
            std::string& error);

        [[nodiscard]]
        core::math::Transform3x4
            CharacterDummyTransform() const noexcept;
    };
}