#include "Application.h"

#include "Core/Log.h"

#include <string>
#include <utility>

namespace
{
    std::wstring ToWide(
        const std::string& value)
    {
        return
            std::wstring(
                value.begin(),
                value.end());
    }
}

namespace client
{
    int Application::Run()
    {
        if (!Initialize())
        {
            core::Log::Error(
                "Client initialization failed");

            Shutdown();

            return 1;
        }

        core::Log::Info(
            "Client frontend loop started");

        while (window_.ProcessMessages())
        {
            if (!Update())
            {
                Shutdown();

                return 2;
            }

            if (!Render())
            {
                Shutdown();

                return 3;
            }

            if (state_ ==
                states::ClientState::Exit)
            {
                break;
            }
        }

        core::Log::Info(
            "Client frontend loop stopped");

        Shutdown();

        return 0;
    }

    bool Application::Initialize()
    {
        state_ =
            states::ClientState::Boot;

        if (!runtime_.Initialize())
        {
            return false;
        }

        std::string error;

        if (!window_.Initialize(
                1600,
                900,
                L"Zone Reborn",
                error))
        {
            core::Log::Error(
                error);

            return false;
        }

        if (!frontendRenderer_.Initialize(
                window_.NativeHandle(),
                window_.Width(),
                window_.Height(),
                runtime_.Resources(),
                error))
        {
            core::Log::Error(
                error);

            return false;
        }

        rememberedLogin_.Initialize(
            runtime_.GameRoot());

        loginScreen_.Initialize(
            rememberedLogin_.Load());

        state_ =
            states::ClientState::Login;

        core::Log::Info(
            "Frontend initialized");

        core::Log::Info(
            "Client state: Login");

        return true;
    }

    bool Application::Update()
    {
        switch (state_)
        {
            case states::ClientState::Login:
            {
                loginScreen_.Update(
                    window_);

                frontend::LoginRequest
                    request;

                if (loginScreen_.
                    ConsumeLoginRequest(
                        request))
                {
                    loginScreen_.
                        SetAuthenticating(
                            true);

                    state_ =
                        states::
                            ClientState::
                            Authenticating;

                    const account::AuthResult result =
                        authService_.
                            Authenticate(
                                request.login,
                                request.password);

                    if (!result.success)
                    {
                        loginScreen_.
                            SetAuthenticating(
                                false);

                        loginScreen_.
                            SetMessage(
                                ToWide(
                                    result.error));

                        state_ =
                            states::
                                ClientState::
                                Login;

                        core::Log::Warning(
                            "Authentication failed");

                        return true;
                    }

                    accountSession_.
                        Establish(
                            result.login,
                            result.sessionToken);

                    if (request.rememberLogin)
                    {
                        rememberedLogin_.
                            Save(
                                result.login);
                    }
                    else
                    {
                        rememberedLogin_.
                            Clear();
                    }

                    loginScreen_.
                        SetAuthenticating(
                            false);

                    state_ =
                        states::
                            ClientState::
                            MainMenu;

                    core::Log::Info(
                        "Authentication successful");

                    core::Log::Info(
                        "Client state: MainMenu");
                }

                break;
            }

            case states::ClientState::Authenticating:
            {
                break;
            }

            case states::ClientState::MainMenu:
            {
                if (window_.ConsumeKeyPress(
                        VK_ESCAPE))
                {
                    state_ =
                        states::
                            ClientState::
                            Exit;
                }

                break;
            }

            case states::ClientState::Exit:
            {
                break;
            }

            default:
            {
                break;
            }
        }

        return true;
    }

    bool Application::Render()
    {
        std::string error;

        switch (state_)
        {
            case states::ClientState::Login:
            case states::ClientState::Authenticating:
            {
                if (!frontendRenderer_.
                    RenderLogin(
                        loginScreen_.View(),
                        error))
                {
                    core::Log::Error(
                        error);

                    return false;
                }

                break;
            }

            case states::ClientState::MainMenu:
            {
                if (!frontendRenderer_.
                    RenderMainMenuCheckpoint(
                        accountSession_.
                            Login(),
                        error))
                {
                    core::Log::Error(
                        error);

                    return false;
                }

                break;
            }

            case states::ClientState::Exit:
            {
                break;
            }

            default:
            {
                break;
            }
        }

        return true;
    }

    void Application::Shutdown()
    {
        accountSession_.Clear();

        frontendRenderer_.Shutdown();
        window_.Shutdown();
        runtime_.Shutdown();

        state_ =
            states::ClientState::Exit;
    }
}