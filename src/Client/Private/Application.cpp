#include "Application.h"

#include "Preview/WorldPreviewLoader.h"
#include "Preview/CharacterDummyRenderDataBuilder.h"

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

        characterDummyAppearance_ =
            {};

        characterDummyVisible_ =
            true;

        characterDummyYaw_ =
            0.0f;

        if (!RebuildCharacterDummy(
                error))
        {
            renderer_.Shutdown();

            error =
                "Unable to build CharacterDummy scene: " +
                error;

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

        characterDummyAppearance_ =
            {};

        characterDummyVisible_ =
            true;

        characterDummyYaw_ =
            0.0f;

        characterDummyFirstInstance_ =
            0;

        characterDummyInstanceCount_ =
            0;

        core::Log::Info(
            "personages_select scene deactivated.");
    }

    core::math::Transform3x4
        Application::CharacterDummyTransform() const noexcept
    {
        return ApplyYaw(
            characterSelectStage_.
                dummyTransform,
            characterDummyYaw_);
    }

    bool Application::RebuildCharacterDummy(
        std::string& error)
    {
        error.clear();

        graphics::SceneRenderData
            scene =
                characterSelectBaseScene_;

        characterDummyFirstInstance_ =
            scene.instances.size();

        characterDummyInstanceCount_ =
            0;

        if (characterDummyVisible_)
        {
            preview::
                CharacterDummyRenderDataBuilder
                    builder;

            if (!builder.Build(
                    runtime_.Resources(),
                    characterDummyAppearance_,
                    CharacterDummyTransform(),
                    scene,
                    error))
            {
                return false;
            }

            characterDummyInstanceCount_ =
                scene.instances.size() -
                characterDummyFirstInstance_;
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

                case frontend::FrontendEventType::DummyShow:
                {
                    if (!rendererInitialized_)
                    {
                        break;
                    }

                    if (characterDummyVisible_)
                    {
                        break;
                    }

                    characterDummyVisible_ =
                        true;

                    std::string rebuildError;

                    if (!RebuildCharacterDummy(
                            rebuildError))
                    {
                        core::Log::Error(
                            std::string(
                                "CharacterDummy show failed: ") +
                            rebuildError);

                        return false;
                    }

                    core::Log::Info(
                        "CharacterDummy shown.");

                    break;
                }
            
                case frontend::FrontendEventType::DummyHide:
                {
                    if (!rendererInitialized_)
                    {
                        break;
                    }

                    if (!characterDummyVisible_)
                    {
                        break;
                    }

                    characterDummyVisible_ =
                        false;

                    std::string rebuildError;

                    if (!RebuildCharacterDummy(
                            rebuildError))
                    {
                        core::Log::Error(
                            std::string(
                                "CharacterDummy hide failed: ") +
                            rebuildError);

                        return false;
                    }

                    core::Log::Info(
                        "CharacterDummy hidden.");

                    break;
                }
            
                case frontend::FrontendEventType::DummyPart:
                {
                    if (!rendererInitialized_)
                    {
                        break;
                    }

                    if (!characterDummyAppearance_.
                            SetPart(
                                event.dummyGroup,
                                event.dummyPartId))
                    {
                        core::Log::Warning(
                            std::string(
                                "Unknown CharacterDummy group: ") +
                            event.dummyGroup);

                        break;
                    }

                    core::Log::Info(
                        std::string(
                            "CharacterDummy part update: ") +
                        event.dummyGroup +
                        " -> " +
                        std::to_string(
                            event.dummyPartId));

                    std::string rebuildError;

                    if (!RebuildCharacterDummy(
                            rebuildError))
                    {
                        core::Log::Error(
                            std::string(
                                "CharacterDummy rebuild failed: ") +
                            rebuildError);

                        return false;
                    }

                    break;
                }
            
                case frontend::FrontendEventType::DummyFull:
                {
                    if (!rendererInitialized_)
                    {
                        break;
                    }

                    for (const auto& part :
                         event.dummyParts)
                    {
                        characterDummyAppearance_.
                            SetPart(
                                part.first,
                                part.second);
                    }

                    core::Log::Info(
                        std::string(
                            "CharacterDummy full rebuild, parts=") +
                        std::to_string(
                            event.dummyParts.size()));

                    std::string rebuildError;

                    if (!RebuildCharacterDummy(
                            rebuildError))
                    {
                        core::Log::Error(
                            std::string(
                                "CharacterDummy full rebuild failed: ") +
                            rebuildError);

                        return false;
                    }

                    break;
                }
            
                case frontend::FrontendEventType::DummyRotate:
                {
                    if (!rendererInitialized_ ||
                        !characterDummyVisible_)
                    {
                        break;
                    }

                    //
                    // Flash передаёт mouse delta.
                    //
                    characterDummyYaw_ +=
                        event.dummyDeltaX *
                        0.01f;

                    constexpr float Pi =
                        3.14159265358979323846f;

                    if (characterDummyYaw_ >
                        Pi * 2.0f)
                    {
                        characterDummyYaw_ -=
                            Pi * 2.0f;
                    }
                    else if (
                        characterDummyYaw_ <
                        -Pi * 2.0f)
                    {
                        characterDummyYaw_ +=
                            Pi * 2.0f;
                    }

                    if (!renderer_.
                            SetInstanceTransformRange(
                                characterDummyFirstInstance_,
                                characterDummyInstanceCount_,
                                CharacterDummyTransform()))
                    {
                        core::Log::Warning(
                            "Unable to rotate CharacterDummy instances.");
                    }

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