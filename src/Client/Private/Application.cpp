#include "Application.h"

#include "Core/Images/WicImageDecoder.h"
#include "Core/Log.h"

#include <cmath>
#include <span>
#include <string>
#include <vector>

namespace
{
    constexpr char MainServerId[] =
        "zone_main";

    constexpr char MainMenuBackgroundPath[] =
        "res/soGUI/maps/MainMenu/main_bg.jpg";

    constexpr float CharacterVerticalOffset =
        0.15f;
    
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

        if (!characterService_.Initialize(
                runtime_.GameRoot()))
        {
            core::Log::Error(
                "Character storage initialization failed.");

            return false;
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
            "Initializing character menu renderer.");

        //
        // personages_select теперь используется ТОЛЬКО
        // как источник:
        //
        // cs_camera
        // AvatarDummy transform
        //
        if (!preview::LoadCharacterSelectStage(
                runtime_.Resources(),
                "personages_select",
                characterSelectStage_,
                error))
        {
            return false;
        }

        //
        // Базовая 3D сцена теперь пустая.
        //
        // В неё ниже будет добавляться только персонаж.
        //
        characterSelectBaseScene_ =
            {};

        if (!renderer_.Initialize(
                window_.NativeHandle(),
                window_.Width(),
                window_.Height(),
                error))
        {
            error =
                "Character menu renderer init failed: " +
                error;

            return false;
        }

        //
        // Load static main-menu background.
        //
        std::vector<std::byte>
            backgroundBytes;

        if (!runtime_.Resources().ReadBinary(
                MainMenuBackgroundPath,
                backgroundBytes))
        {
            renderer_.Shutdown();

            error =
                std::string(
                    "Main menu background not found: ") +
                MainMenuBackgroundPath;

            return false;
        }

        core::images::RgbaImage
            backgroundImage;

        core::images::WicImageDecoder
            backgroundDecoder;

        std::string
            backgroundError;

        if (!backgroundDecoder.Decode(
                std::span<const std::byte>(
                    backgroundBytes.data(),
                    backgroundBytes.size()),
                backgroundImage,
                backgroundError))
        {
            renderer_.Shutdown();

            error =
                std::string(
                    "Unable to decode main menu background: ") +
                backgroundError;

            return false;
        }

        if (!renderer_.SetBackgroundImage(
                backgroundImage,
                backgroundError))
        {
            renderer_.Shutdown();

            error =
                std::string(
                    "Unable to upload main menu background: ") +
                backgroundError;

            return false;
        }

        core::Log::Info(
            std::string(
                "Main menu background loaded: ") +
            std::to_string(
                backgroundImage.width) +
            "x" +
            std::to_string(
                backgroundImage.height));

        //
        // Character system remains exactly as before.
        //
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

        if (characterProfile_.has_value())
        {
            if (!ApplyCharacterProfile(
                    *characterProfile_,
                    error))
            {
                characterCatalog_.Clear();

                renderer_.Shutdown();

                error =
                    "Unable to apply character profile: " +
                    error;

                return false;
            }
        }

        characterVisible_ =
            characterProfile_.
                has_value();

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
            "Character menu renderer activated.");

        return true;
    }

    bool Application::ApplyCharacterProfile(
        const character::Profile& profile,
        std::string& error)
    {
        std::vector<
            std::pair<
                std::string,
                std::int32_t>>
            values;

        values.reserve(
            profile.appearance.size());

        for (const character::AppearancePart& part :
             profile.appearance)
        {
            values.emplace_back(
                part.group,
                part.itemType);
        }

        return
            characterState_.
                ApplyCreatorSet(
                    characterCatalog_,
                    values,
                    error);
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
        "Character menu renderer deactivated.");
    }

    core::math::Transform3x4
Application::CharacterTransform() const noexcept
    {
        core::math::Transform3x4 transform =
            ApplyYaw(
                characterSelectStage_.
                    characterTransform,
                characterYaw_);

        transform.values[10] +=
            CharacterVerticalOffset;

        return transform;
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

        characterAnimator_.Reset();

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
                characterAnimator_,
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

        characterAnimationStart_ =
            std::chrono::steady_clock::now();

        if (characterVisible_ &&
            characterAnimator_.IsReady())
        {
            if (!characterAnimator_.Update(
                    0.0f,
                    renderer_,
                    error))
            {
                error =
                    "Unable to apply initial character idle pose: " +
                    error;

                return false;
            }
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
            
            if (characterVisible_ &&
                characterAnimator_.IsReady())
            {
                const auto now =
                    std::chrono::steady_clock::now();

                const float elapsedSeconds =
                    std::chrono::duration<float>(
                        now -
                        characterAnimationStart_).
                        count();

                std::string
                    animationError;

                if (!characterAnimator_.Update(
                        elapsedSeconds,
                        renderer_,
                        animationError))
                {
                    core::Log::Error(
                        std::string(
                            "Character idle update failed: ") +
                        animationError);

                    return false;
                }
            }
            
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
                            result.sessionToken,
                            MainServerId);

                    if (event.rememberLogin)
                    {
                        rememberedLogin_.Save(result.login);
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
                            "Assigned server id: ") +
                        accountSession_.
                            ServerId());

                    std::string
                        characterSceneError;
                        
                    characterProfile_.
                        reset();

                    std::string
                        characterLoadError;

                    if (!characterService_.Load(
                            accountSession_.Login(),
                            characterProfile_,
                            characterLoadError))
                    {
                        core::Log::Error(
                            std::string(
                                "Character load failed: ") +
                            characterLoadError);

                        accountSession_.Clear();

                        frontend_.
                            SendLoginError(
                                "Unable to load character.");

                        break;
                    }

                    if (characterProfile_.
                        has_value())
                    {
                        core::Log::Info(
                            std::string(
                                "Character loaded: ") +
                            characterProfile_->
                                name);
                    }
                    else
                    {
                        core::Log::Info(
                            "Account has no character.");
                    }
                        
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
                        SendLoginComplete(
                            characterProfile_.
                                has_value()
                                    ? &*characterProfile_
                                    : nullptr);

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

                case frontend::FrontendEventType::
                    CharacterRequest:
                {
                    if (!accountSession_.
                            IsAuthenticated())
                    {
                        frontend_.
                            SendCharacterState(
                                nullptr);

                        break;
                    }

                    frontend_.
                        SendCharacterState(
                            characterProfile_.
                                has_value()
                                    ? &*characterProfile_
                                    : nullptr);

                    break;
                }

                case frontend::FrontendEventType::
                    CharacterCreate:
                {
                    if (!accountSession_.
                            IsAuthenticated())
                    {
                        frontend_.
                            SendCharacterCreateResult(
                                false,
                                "Account is not authenticated.",
                                nullptr);

                        break;
                    }

                    if (!rendererInitialized_)
                    {
                        frontend_.
                            SendCharacterCreateResult(
                                false,
                                "Character renderer is not initialized.",
                                nullptr);

                        break;
                    }

                    if (characterProfile_.
                        has_value())
                    {
                        frontend_.
                            SendCharacterCreateResult(
                                false,
                                "Character already exists.",
                                &*characterProfile_);

                        break;
                    }

                    character::Profile
                        createdProfile;

                    std::string
                        createError;

                    if (!characterService_.Create(
                            accountSession_.Login(),
                            event.characterName,
                            characterCatalog_,
                            createdProfile,
                            createError))
                    {
                        core::Log::Warning(
                            std::string(
                                "Character creation failed: ") +
                            createError);

                        frontend_.
                            SendCharacterCreateResult(
                                false,
                                createError,
                                nullptr);

                        break;
                    }

                    characterProfile_ =
                        std::move(
                            createdProfile);

                    std::string
                        stateError;

                    if (!ApplyCharacterProfile(
                            *characterProfile_,
                            stateError))
                    {
                        std::string
                            deleteError;

                        characterService_.Delete(
                            accountSession_.Login(),
                            deleteError);

                        characterProfile_.
                            reset();

                        frontend_.
                            SendCharacterCreateResult(
                                false,
                                stateError,
                                nullptr);

                        break;
                    }

                    characterVisible_ =
                        true;

                    characterYaw_ =
                        0.0f;

                    std::string
                        rebuildError;

                    if (!RebuildCharacter(
                            rebuildError))
                    {
                        core::Log::Error(
                            std::string(
                                "Character creation rebuild failed: ") +
                            rebuildError);

                        return false;
                    }

                    core::Log::Info(
                        std::string(
                            "Character created: ") +
                        characterProfile_->
                            name);

                    frontend_.
                        SendCharacterCreateResult(
                            true,
                            {},
                            &*characterProfile_);

                    break;
                }

                case frontend::FrontendEventType::
                    CharacterDelete:
                {
                    if (!accountSession_.
                            IsAuthenticated())
                    {
                        frontend_.
                            SendCharacterDeleteResult(
                                false,
                                "Account is not authenticated.");

                        break;
                    }

                    if (!characterProfile_.
                            has_value())
                    {
                        frontend_.
                            SendCharacterDeleteResult(
                                false,
                                "Character does not exist.");

                        break;
                    }

                    std::string
                        deleteError;

                    if (!characterService_.Delete(
                            accountSession_.Login(),
                            deleteError))
                    {
                        core::Log::Warning(
                            std::string(
                                "Character deletion failed: ") +
                            deleteError);

                        frontend_.
                            SendCharacterDeleteResult(
                                false,
                                deleteError);

                        break;
                    }

                    const std::string
                        deletedName =
                            characterProfile_->
                                name;

                    characterProfile_.
                        reset();

                    std::string
                        resetError;

                    if (!characterState_.
                            ResetCreator(
                                characterCatalog_,
                                resetError))
                    {
                        core::Log::Error(
                            std::string(
                                "Unable to reset character state: ") +
                            resetError);

                        return false;
                    }

                    characterVisible_ =
                        false;

                    characterYaw_ =
                        0.0f;

                    std::string
                        rebuildError;

                    if (!RebuildCharacter(
                            rebuildError))
                    {
                        core::Log::Error(
                            std::string(
                                "Character deletion rebuild failed: ") +
                            rebuildError);

                        return false;
                    }

                    core::Log::Info(
                        std::string(
                            "Character deleted: ") +
                        deletedName);

                    frontend_.
                        SendCharacterDeleteResult(
                            true,
                            {});

                    break;
                }

                case frontend::FrontendEventType::CharacterShow:
                {
                    if (!rendererInitialized_)
                    {
                        break;
                    }

                    if (!characterProfile_.
                            has_value())
                    {
                        core::Log::Warning(
                        "Character show ignored: account has no character.");

                        break;
                    }

                    if (characterVisible_)
                    {
                        break;
                    }

                    characterVisible_ =
                        true;

                    std::string
                        rebuildError;

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
                    if (!rendererInitialized_)
                    {
                        break;
                    }

                    //
                    // UI не имеет права скрывать реально существующего
                    // персонажа в главном меню.
                    //
                    if (characterProfile_.
                            has_value())
                    {
                        core::Log::Warning(
                        "Character hide ignored: account has an active character.");

                        break;
                    }

                    if (!characterVisible_)
                    {
                        break;
                    }

                    characterVisible_ =
                        false;

                    std::string
                        rebuildError;

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
                    if (!characterProfile_.
                            has_value())
                    {
                        core::Log::Warning(
                            "Play ignored: account has no character.");

                        break;
                    }

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

        characterProfile_.reset();
        
        accountSession_.Clear();

        window_.Shutdown();
        runtime_.Shutdown();

        state_ =
            states::ClientState::Exit;
    }
}
