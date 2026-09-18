#pragma once

#include "Account/AccountSession.h"
#include "Account/AuthService.h"
#include "Account/RememberedLogin.h"
#include "Frontend/Frontend.h"
#include "Platform/Window.h"
#include "States/ClientState.h"

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

        void Shutdown();

        core::Runtime
            runtime_;

        platform::Window
            window_;

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
    };
}