#include "Application.h"

#include "Preview/WorldPreviewLoader.h"
#include "Preview/CharacterDummyRenderDataBuilder.h"

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

    bool Application::InitializeCharacterSelectScene(
        std::string& error)
    {
        error.clear();

        if (rendererInitialized_)
        {
            return true;
        }

        core::Log::Info(
            "Initializing original personages_select scene.");

        if (!preview::LoadCharacterSelectStage(
                runtime_.Resources(),
                "personages_select",
                characterSelectStage_,
                error))
        {
            return false;
        }

        graphics::SceneRenderData
            scene;

        if (!preview::LoadWorldPreview(
                runtime_,
                "personages_select",
                scene,
                error))
        {
            error =
                "Unable to load personages_select: " +
                error;

            return false;
        }

        preview::CharacterDummyRenderDataBuilder
            characterDummyBuilder;

        if (!characterDummyBuilder.BuildDefault(
                runtime_.Resources(),
                characterSelectStage_.
                    dummyTransform,
                scene,
                error))
        {
            error =
                "Unable to build CharacterDummy: " +
                error;

            return false;
        }

        if (!renderer_.Initialize(
                window_.NativeHandle(),
                window_.Width(),
                window_.Height(),
                error))
        {
            error =
                "Character selection renderer init failed: " +
                error;

            return false;
        }

        if (!renderer_.SetScene(
                scene,
                error))
        {
            renderer_.Shutdown();

            error =
                "Unable to upload personages_select: " +
                error;

            return false;
        }

        renderer_.SetCamera(
            characterSelectStage_.
                camera);

        rendererInitialized_ =
            true;

        core::Log::Info(
            "Original personages_select scene activated.");

        return true;
    }


    void Application::ShutdownCharacterSelectScene()
    {
        if (!rendererInitialized_)
        {
            return;
        }

        renderer_.Shutdown();

        rendererInitialized_ =
            false;

        characterSelectStage_ =
            {};

        core::Log::Info(
            "personages_select scene deactivated.");
    }

    bool Application::Update()
    {
        if (rendererInitialized_)
        {
            std::string
                renderError;

            renderer_.SetCamera(
                characterSelectStage_.
                    camera);

            if (!renderer_.Render(
                    renderError))
            {
                core::Log::Error(
                    renderError);

                return false;
            }
        }
        
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

                        core::Log::Info(std::string("Selected server id: ") +
                            event.serverId);

                        std::string
                            characterSceneError;

                        if (!InitializeCharacterSelectScene(
                                characterSceneError))
                        {
                            core::Log::Error(
                                characterSceneError);

                            accountSession_.
                                Clear();

                            frontend_.
                                SendLoginError(
                                    "Unable to initialize character selection scene.");

                            break;
                        }

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
                        core::Log::Info(
                            "Frontend requested Play.");

                        //
                        // personages_select принадлежит только
                        // character/account selection flow.
                        //
                        // Перед загрузкой игрового world он больше
                        // не должен оставаться за WebView.
                        //
                        ShutdownCharacterSelectScene();

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

        renderer_.Shutdown();

        rendererInitialized_ =
            false;

        accountSession_.Clear();

        window_.Shutdown();
        runtime_.Shutdown();

        state_ =
            states::ClientState::Exit;
    }
}