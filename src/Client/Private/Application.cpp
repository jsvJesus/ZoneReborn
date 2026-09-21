#include "Application.h"

#include "Preview/WorldPreviewLoader.h"

#include "Core/Log.h"

#include <string>
#include <cmath>

namespace
{
    core::math::Transform3x4
    ApplyYaw(
        const core::math::Transform3x4& base,
        const float yaw) noexcept
    {
        core::math::Transform3x4
            rotation =
                core::math::
                    Transform3x4::
                    Identity();

        const float cosine =
            std::cos(
                yaw);

        const float sine =
            std::sin(
                yaw);

        rotation.values[0] =
            cosine;

        rotation.values[1] =
            0.0f;

        rotation.values[2] =
            -sine;

        rotation.values[3] =
            0.0f;

        rotation.values[4] =
            1.0f;

        rotation.values[5] =
            0.0f;

        rotation.values[6] =
            sine;

        rotation.values[7] =
            0.0f;

        rotation.values[8] =
            cosine;

        return
            core::math::
                Transform3x4::
                Multiply(
                    rotation,
                    base);
    }
}

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

        if (!window_.Initialize(1600, 900, L"Zone Reborn", error))
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

        if (!audio_.Initialize(
                runtime_.GameRoot(),
                error))
        {
            core::Log::Warning(
                std::string(
                    "Audio initialization failed: ") +
                error);

            error.clear();
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

        characterSelectBaseScene_ =
            {};

        //
        // Пока сохраняем personages_select как 3D stage.
        //
        // Flash UI к этой сцене отношения не имеет:
        // он просто рисуется поверх DX11.
        //
        if (!preview::LoadWorldPreview(
                runtime_,
                "personages_select",
                characterSelectBaseScene_,
                error))
        {
            error =
                "Unable to load personages_select: " +
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

        if (!characterCatalog_.Load(
        runtime_.Resources(),
        error))
        {
            renderer_.Shutdown();

            error =
                "Character catalog initialization failed: " +
                error;

            return false;
        }

        if (!characterState_.ResetCreator(
                characterCatalog_,
                error))
        {
            characterCatalog_.Clear();

            renderer_.Shutdown();

            error =
                "Character state initialization failed: " +
                error;

            return false;
        }

        characterVisible_ =
            true;

        characterYaw_ =
            0.0f;

        if (!RebuildCharacter(
                error))
        {
            characterCatalog_.Clear();

            renderer_.Shutdown();

            return false;
        }

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

        characterSelectBaseScene_ =
            {};

        characterState_.Reset();

        characterCatalog_.Clear();

        characterVisible_ =
            true;

        characterYaw_ =
            0.0f;

        characterFirstInstance_ =
            0;

        characterInstanceCount_ =
            0;

        core::Log::Info(
            "personages_select scene deactivated.");
    }

    core::math::Transform3x4
Application::CharacterTransform() const noexcept
    {
        return ApplyYaw(
            characterSelectStage_.
                characterTransform,
            characterYaw_);
    }

    bool Application::RebuildCharacter(
        std::string& error)
    {
        error.clear();

        graphics::SceneRenderData scene =
            characterSelectBaseScene_;

        characterFirstInstance_ =
            scene.instances.size();

        characterInstanceCount_ =
            0;

        if (characterVisible_)
        {
            character::RenderDataBuilder
                builder;

            if (!builder.Build(
                    runtime_.Resources(),
                    characterCatalog_,
                    characterState_,
                    CharacterTransform(),
                    scene,
                    characterInstanceCount_,
                    error))
            {
                return false;
            }
        }

        if (!renderer_.SetScene(
                scene,
                error))
        {
            return false;
        }

        renderer_.SetCamera(
            characterSelectStage_.
                camera);

        return true;
    }

    bool Application::Update()
    {
        if (audio_.IsInitialized())
        {
            std::string audioError;

            if (!audio_.Update(
                    audioError))
            {
                core::Log::Warning(
                    std::string(
                        "Audio update failed: ") +
                    audioError);

                audio_.Shutdown();
            }
        }
        
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

                case frontend::FrontendEventType::UiSound:
                {
                        std::string audioError;

                        if (!audio_.PlayUiSound(
                                event.soundName,
                                audioError))
                        {
                            core::Log::Warning(
                                std::string(
                                    "UI sound failed: ") +
                                audioError);
                        }

                        break;
                }

                case frontend::FrontendEventType::CharacterShow:
                {
                    if (!rendererInitialized_ ||
                        characterVisible_)
                    {
                        break;
                    }

                    characterVisible_ =
                        true;

                    std::string rebuildError;

                    if (!RebuildCharacter(
                            rebuildError))
                    {
                        core::Log::Error(
                            std::string(
                                "Character show failed: ") +
                            rebuildError);

                        return false;
                    }

                    break;
                }

                case frontend::FrontendEventType::CharacterHide:
                {
                    if (!rendererInitialized_ ||
                        !characterVisible_)
                    {
                        break;
                    }

                    characterVisible_ =
                        false;

                    std::string rebuildError;

                    if (!RebuildCharacter(
                            rebuildError))
                    {
                        core::Log::Error(
                            std::string(
                                "Character hide failed: ") +
                            rebuildError);

                        return false;
                    }

                    break;
                }

                case frontend::FrontendEventType::CharacterPart:
                {
                    if (!rendererInitialized_)
                    {
                        break;
                    }

                    std::string stateError;

                    if (!characterState_.
                            ApplyCreatorSelection(
                                characterCatalog_,
                                event.characterGroup,
                                event.characterItemType,
                                stateError))
                    {
                        core::Log::Warning(
                            stateError);

                        break;
                    }

                    core::Log::Info(
                        std::string(
                            "Character item changed: ") +
                        event.characterGroup +
                        " -> " +
                        std::to_string(
                            event.characterItemType));

                    std::string rebuildError;

                    if (!RebuildCharacter(
                            rebuildError))
                    {
                        core::Log::Error(
                            std::string(
                                "Character rebuild failed: ") +
                            rebuildError);

                        return false;
                    }

                    break;
                }

                case frontend::FrontendEventType::CharacterFull:
                {
                    if (!rendererInitialized_)
                    {
                        break;
                    }

                    std::string stateError;

                    if (!characterState_.
                            ApplyCreatorSet(
                                characterCatalog_,
                                event.characterParts,
                                stateError))
                    {
                        core::Log::Error(
                            std::string(
                                "Unable to apply character data: ") +
                            stateError);

                        return false;
                    }

                    std::string rebuildError;

                    if (!RebuildCharacter(
                            rebuildError))
                    {
                        core::Log::Error(
                            std::string(
                                "Character full rebuild failed: ") +
                            rebuildError);

                        return false;
                    }

                    break;
                }

                case frontend::FrontendEventType::CharacterRotate:
                {
                    if (!rendererInitialized_ ||
                        !characterVisible_)
                    {
                        break;
                    }

                    characterYaw_ +=
                        event.characterDeltaX *
                        0.01f;

                    constexpr float Pi =
                        3.14159265358979323846f;

                    constexpr float FullTurn =
                        Pi *
                        2.0f;

                    while (characterYaw_ >
                           FullTurn)
                    {
                        characterYaw_ -=
                            FullTurn;
                    }

                    while (characterYaw_ <
                           -FullTurn)
                    {
                        characterYaw_ +=
                            FullTurn;
                    }

                    if (!renderer_.
                            SetInstanceTransformRange(
                                characterFirstInstance_,
                                characterInstanceCount_,
                                CharacterTransform()))
                    {
                        core::Log::Warning(
                            "Unable to rotate character instances.");
                    }

                    break;
                }

                case frontend::FrontendEventType::Play:
                    {
                        core::Log::Info(
                            "Frontend requested Play.");

                        audio_.StopMenuMusic();
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

        audio_.Shutdown();

        accountSession_.Clear();

        window_.Shutdown();
        runtime_.Shutdown();

        state_ =
            states::ClientState::Exit;
    }
}