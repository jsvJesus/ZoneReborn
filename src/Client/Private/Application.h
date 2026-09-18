#pragma once

#include "Account/AccountSession.h"
#include "Account/AuthService.h"
#include "Account/RememberedLogin.h"
#include "Frontend/LoginScreen.h"
#include "Platform/Window.h"
#include "States/ClientState.h"
#include "UI/FrontendRenderer.h"

#include "Core/Runtime.h"

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

        [[nodiscard]]
        bool Render();

        void Shutdown();

        core::Runtime
            runtime_;

        platform::Window
            window_;

        ui::FrontendRenderer
            frontendRenderer_;

        frontend::LoginScreen
            loginScreen_;

        account::AuthService
            authService_;

        account::AccountSession
            accountSession_;

        account::RememberedLogin
            rememberedLogin_;

        states::ClientState
            state_ =
                states::ClientState::Boot;
    };
}