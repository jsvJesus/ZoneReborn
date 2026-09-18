#include "Application.h"

#include "Core/Log.h"

#include <string>

namespace client
{
    int Application::Run()
    {
        if (!Initialize())
        {
            core::Log::Error(
                "Client initialization failed.");

            Shutdown();

            return 1;
        }

        while (window_.ProcessMessages())
        {
            if (!Update())
            {
                Shutdown();

                return 2;
            }

            if (state_ ==
                states::ClientState::Exit)
            {
                break;
            }
        }

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

        if (!rememberedLogin_.Initialize(
                runtime_.GameRoot()))
        {
            core::Log::Warning(
                "Remembered login storage initialization failed.");
        }

        if (!frontend_.Initialize(
                window_.NativeHandle(),
                runtime_.GameRoot(),
                runtime_.Resources(),
                error))
        {
            core::Log::Error(
                error);

            return false;
        }

        state_ =
            states::ClientState::Frontend;

        core::Log::Info(
            "Client state: Frontend");

        return true;
    }

    bool Application::Update()
    {
        frontend_.Resize();

        std::string frontendError;

        if (frontend_.ConsumeFatalError(
                frontendError))
        {
            core::Log::Error(
                frontendError);

            return false;
        }

        frontend::FrontendEvent event;

        while (frontend_.ConsumeEvent(
            event))
        {
            switch (event.type)
            {
                case frontend::FrontendEventType::Login:
                {
                    const account::AuthResult result =
                        authService_.
                            Authenticate(
                                event.login,
                                event.password);

                    if (!result.success)
                    {
                        frontend_.
                            SendLoginError(
                                result.error);

                        core::Log::Warning(
                            "Authentication failed.");

                        break;
                    }

                    accountSession_.
                        Establish(
                            result.login,
                            result.sessionToken);

                    if (event.rememberLogin)
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

                    core::Log::Info(
                        std::string(
                            "Authentication successful: ") +
                        result.login);

                    core::Log::Info(
                        std::string(
                            "Selected server id: ") +
                        event.serverId);

                    frontend_.
                        SendLoginAccepted();

                    break;
                }

                case frontend::FrontendEventType::OpenUrl:
                {
                    //
                    // Пока сайт ZoneReborn не готов,
                    // ничего наружу не открываем.
                    //
                    core::Log::Info(
                        std::string(
                            "Frontend URL request: ") +
                        event.url);

                    break;
                }

                case frontend::FrontendEventType::Play:
                {
                    //
                    // Подключим сюда Character /
                    // Server / World flow следующим этапом.
                    //
                    core::Log::Info(
                        "Frontend requested Play.");

                    break;
                }

                case frontend::FrontendEventType::Exit:
                {
                    state_ =
                        states::ClientState::Exit;

                    break;
                }
            }
        }

        return true;
    }

    void Application::Shutdown()
    {
        frontend_.Shutdown();

        accountSession_.Clear();

        window_.Shutdown();
        runtime_.Shutdown();

        state_ =
            states::ClientState::Exit;
    }
}