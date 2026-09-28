#include "Application.h"

#include "Core/Images/WicImageDecoder.h"
#include "Core/Log.h"

#include "Loading/LoadingProgress.h"

#include <cmath>
#include <random>
#include <span>
#include <string>
#include <vector>

namespace
{
    constexpr char MainServerId[] =
        "zone_main";

    constexpr char TestWorldSpace[] =
        "start_tutorial_warehouse"; // load map for Client

    constexpr char MainMenuBackgroundPath[] =
        "res/soGUI/maps/MainMenu/main_bg.jpg";

    constexpr float CharacterVerticalOffset =
        0.15f;

    constexpr float CharacterRotationReturnRate =
        6.0f;

    constexpr float CharacterRotationReturnEpsilon =
        0.001f;
    
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
        character::State loaded;

        if (!loaded.ApplyCreatorSet(
                characterCatalog_,
                profile.appearance,
                error))
        {
            return false;
        }

        const bool faceApplied = profile.face.faceForm.empty()
            ? loaded.ResetFace(characterCatalog_, error)
            : loaded.ApplyFaceState(characterCatalog_, profile.face, error);

        if (!faceApplied)
        {
            return false;
        }

        characterState_ = std::move(loaded);
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

    graphics::CameraView Application::CharacterCamera() const noexcept
    {
        if (!characterFaceCamera_)
        {
            return characterSelectStage_.camera;
        }

        graphics::CameraView camera = characterSelectStage_.camera;
        const core::math::Vector3 origin = CharacterTransform().Translation();
        float x = camera.forward.x;
        float z = camera.forward.z;
        const float length = std::sqrt(x * x + z * z);
        if (length > 0.0001f)
        {
            x /= length;
            z /= length;
        }
        else
        {
            x = 0.0f;
            z = 1.0f;
        }
        const core::math::Vector3 target{origin.x, origin.y + 1.57f, origin.z};
        camera.position = {target.x - x * 1.25f, target.y, target.z - z * 1.25f};
        camera.forward = {x, 0.0f, z};
        camera.up = {0.0f, 1.0f, 0.0f};
        camera.fieldOfViewDegrees = 45.0f;
        return camera;
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

        // Build the default creator model even when the account has no
        // character. Keeping its render resources in the scene avoids a
        // completely empty character scene during login; only its instances
        // are suppressed until the creator is opened.
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

            if (!characterVisible_)
            {
                scene.instances.resize(
                    characterFirstInstance_);

                characterInstanceCount_ =
                    0;

                characterAnimator_.Reset();
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
                    character::AnimationState::Idle,
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
            CharacterCamera());

        return true;
    }

    bool Application::Update()
    {
        const auto updateNow =
            std::chrono::steady_clock::now();

        float updateSeconds =
            std::chrono::duration<float>(
                updateNow -
                characterRotationUpdateTime_).
                count();

        characterRotationUpdateTime_ =
            updateNow;

        if (updateSeconds < 0.0f)
        {
            updateSeconds =
                0.0f;
        }
        else if (updateSeconds > 0.1f)
        {
            updateSeconds =
                0.1f;
        }

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
        
        if (state_ == states::ClientState::World && worldSession_.IsLoaded())
        {
            worldSession_.Update(
                window_,
                renderer_);

            std::string
                renderError;

            if (!renderer_.Render(
                    renderError))
            {
                core::Log::Error(
                    std::string(
                        "World render failed: ") +
                    renderError);

                return false;
            }

            if (worldFirstFramePending_)
            {
                worldFirstFramePending_ =
                    false;

                frontend_.Hide();

                frontend_.SetWorldLoading(
                    false);
            }
        }
        else if (rendererInitialized_)
        {
            std::string
                renderError;

            renderer_.SetCamera(
                CharacterCamera());

            if (characterYawReturning_ &&
                characterVisible_)
            {
                characterYaw_ *=
                    std::exp(
                        -CharacterRotationReturnRate *
                        updateSeconds);

                if (std::abs(characterYaw_) <=
                    CharacterRotationReturnEpsilon)
                {
                    characterYaw_ =
                        0.0f;

                    characterYawReturning_ =
                        false;
                }

                if (!renderer_.
                        SetInstanceTransformRange(
                            characterFirstInstance_,
                            characterInstanceCount_,
                            CharacterTransform()))
                {
                    core::Log::Warning(
                        "Unable to restore character rotation.");
                }
            }

            if (characterVisible_ &&
                characterAnimator_.IsReady())
            {
                const float elapsedSeconds =
                    std::chrono::duration<float>(
                        updateNow -
                        characterAnimationStart_).
                        count();

                std::string
                    animationError;

                if (!characterAnimator_.Update(
                        character::AnimationState::Idle,
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
                        if (!rememberedLogin_.Save(
                                result.login))
                        {
                            core::Log::Warning(
                                "Unable to save remembered login.");
                        }
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

                    characterEditSnapshot_.reset();
                    characterFaceSnapshot_.reset();
                    characterFaceCamera_ = false;

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
                    CharacterCreatorReset:
                {
                    if (!accountSession_.
                            IsAuthenticated() ||
                        !rendererInitialized_ ||
                        characterProfile_.
                            has_value())
                    {
                        break;
                    }

                    characterFaceSnapshot_.reset();
                    characterFaceCamera_ = false;

                    std::string
                        resetError;

                    if (!characterState_.
                            ResetCreator(
                                characterCatalog_,
                                resetError))
                    {
                        core::Log::Error(
                            std::string(
                                "Unable to reset character creator: ") +
                            resetError);

                        return false;
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
                                "Character creator reset failed: ") +
                            rebuildError);

                        return false;
                    }

                    break;
                }

                case frontend::FrontendEventType::
                    CharacterCreatorRandom:
                {
                    if (!accountSession_.IsAuthenticated() ||
                        !rendererInitialized_ ||
                        characterProfile_.has_value())
                    {
                        break;
                    }

                    character::State randomized =
                        characterState_;

                    std::string randomError;

                    if (!randomized.ApplyCreatorSet(
                            characterCatalog_,
                            event.characterParts,
                            randomError))
                    {
                        core::Log::Warning(
                            "Unable to randomize creator clothes: " +
                            randomError);

                        break;
                    }

                    static std::mt19937 random(
                        std::random_device{}());

                    if (!randomized.RandomizeFace(
                            characterCatalog_,
                            random,
                            randomError))
                    {
                        core::Log::Warning(
                            "Unable to randomize creator face: " +
                            randomError);

                        break;
                    }

                    characterState_ =
                        std::move(
                            randomized);

                    characterFaceSnapshot_.reset();
                    characterFaceCamera_ = false;
                    characterVisible_ = true;
                    characterYaw_ = 0.0f;
                    renderer_.SetCamera(
                        CharacterCamera());

                    std::string rebuildError;

                    if (!RebuildCharacter(
                            rebuildError))
                    {
                        core::Log::Error(
                            "Character creator random rebuild failed: " +
                            rebuildError);

                        return false;
                    }

                    break;
                }

                case frontend::FrontendEventType::
                    CharacterCreatorCancel:
                {
                    if (!rendererInitialized_ ||
                        characterProfile_.has_value())
                    {
                        break;
                    }

                    std::string resetError;

                    if (!characterState_.ResetCreator(
                            characterCatalog_,
                            resetError))
                    {
                        core::Log::Error(
                            "Unable to cancel character creator: " +
                            resetError);

                        return false;
                    }

                    characterFaceSnapshot_.reset();
                    characterFaceCamera_ = false;
                    characterVisible_ = false;
                    characterYaw_ = 0.0f;
                    renderer_.SetCamera(
                        CharacterCamera());

                    std::string rebuildError;

                    if (!RebuildCharacter(
                            rebuildError))
                    {
                        core::Log::Error(
                            "Character creator cancel rebuild failed: " +
                            rebuildError);

                        return false;
                    }

                    break;
                }

                case frontend::FrontendEventType::
                    CharacterEditOpen:
                {
                    if (!accountSession_.IsAuthenticated() ||
                        !rendererInitialized_ ||
                        !characterProfile_.has_value())
                    {
                        break;
                    }

                    if (!characterEditSnapshot_.has_value())
                    {
                        characterEditSnapshot_ =
                            characterState_;
                    }

                    characterFaceSnapshot_.reset();
                    characterState_.ClearPreviewMask();
                    characterFaceCamera_ = false;
                    characterVisible_ = true;
                    characterYaw_ = 0.0f;
                    renderer_.SetCamera(
                        CharacterCamera());

                    std::string rebuildError;

                    if (!RebuildCharacter(
                            rebuildError))
                    {
                        core::Log::Error(
                            "Character editor open rebuild failed: " +
                            rebuildError);

                        return false;
                    }

                    break;
                }

                case frontend::FrontendEventType::
                    CharacterEditReset:
                {
                    if (!rendererInitialized_ ||
                        !characterProfile_.has_value() ||
                        !characterEditSnapshot_.has_value())
                    {
                        break;
                    }

                    std::string resetError;

                    if (!characterState_.ResetCreator(
                            characterCatalog_,
                            resetError))
                    {
                        core::Log::Error(
                            "Unable to reset character editor: " +
                            resetError);

                        return false;
                    }

                    characterFaceSnapshot_.reset();
                    characterFaceCamera_ = false;
                    characterVisible_ = true;
                    characterYaw_ = 0.0f;
                    renderer_.SetCamera(
                        CharacterCamera());

                    std::string rebuildError;

                    if (!RebuildCharacter(
                            rebuildError))
                    {
                        core::Log::Error(
                            "Character editor reset rebuild failed: " +
                            rebuildError);

                        return false;
                    }

                    break;
                }

                case frontend::FrontendEventType::
                    CharacterEditRandom:
                {
                    if (!rendererInitialized_ ||
                        !characterProfile_.has_value() ||
                        !characterEditSnapshot_.has_value())
                    {
                        break;
                    }

                    character::State randomized =
                        characterState_;

                    std::string randomError;

                    if (!randomized.ApplyCreatorSet(
                            characterCatalog_,
                            event.characterParts,
                            randomError))
                    {
                        core::Log::Warning(
                            "Unable to randomize character editor clothes: " +
                            randomError);

                        break;
                    }

                    static std::mt19937 random(
                        std::random_device{}());

                    if (!randomized.RandomizeFace(
                            characterCatalog_,
                            random,
                            randomError))
                    {
                        core::Log::Warning(
                            "Unable to randomize character editor face: " +
                            randomError);

                        break;
                    }

                    characterState_ =
                        std::move(
                            randomized);

                    characterFaceSnapshot_.reset();
                    characterFaceCamera_ = false;
                    characterVisible_ = true;
                    characterYaw_ = 0.0f;
                    renderer_.SetCamera(
                        CharacterCamera());

                    std::string rebuildError;

                    if (!RebuildCharacter(
                            rebuildError))
                    {
                        core::Log::Error(
                            "Character editor random rebuild failed: " +
                            rebuildError);

                        return false;
                    }

                    break;
                }

                case frontend::FrontendEventType::
                    CharacterEditApply:
                {
                    if (!rendererInitialized_ ||
                        !characterProfile_.has_value() ||
                        !characterEditSnapshot_.has_value())
                    {
                        frontend_.SendCharacterEditResult(
                            false,
                            "Character editor is not active.",
                            characterProfile_.has_value()
                                ? &*characterProfile_
                                : nullptr);

                        break;
                    }

                    std::vector<character::AppearancePart>
                        appearance;

                    std::string updateError;

                    if (!characterState_.CreatorAppearance(
                            characterCatalog_,
                            appearance,
                            updateError))
                    {
                        frontend_.SendCharacterEditResult(
                            false,
                            updateError,
                            &*characterProfile_);

                        break;
                    }

                    character::Profile updated =
                        *characterProfile_;

                    updated.appearance =
                        std::move(
                            appearance);

                    updated.face =
                        characterState_.Face();

                    if (!characterService_.Update(
                            accountSession_.Login(),
                            updated,
                            updateError))
                    {
                        core::Log::Warning(
                            "Character editor save failed: " +
                            updateError);

                        frontend_.SendCharacterEditResult(
                            false,
                            updateError,
                            &*characterProfile_);

                        break;
                    }

                    characterProfile_ =
                        std::move(
                            updated);

                    characterState_.ClearPreviewMask();
                    characterEditSnapshot_.reset();
                    characterFaceSnapshot_.reset();
                    characterFaceCamera_ = false;
                    characterVisible_ = true;
                    characterYaw_ = 0.0f;
                    renderer_.SetCamera(
                        CharacterCamera());

                    std::string rebuildError;

                    if (!RebuildCharacter(
                            rebuildError))
                    {
                        core::Log::Error(
                            "Character editor apply rebuild failed: " +
                            rebuildError);

                        return false;
                    }

                    frontend_.SendCharacterEditResult(
                        true,
                        {},
                        &*characterProfile_);

                    break;
                }

                case frontend::FrontendEventType::
                    CharacterEditCancel:
                {
                    if (!rendererInitialized_ ||
                        !characterProfile_.has_value() ||
                        !characterEditSnapshot_.has_value())
                    {
                        break;
                    }

                    characterState_ =
                        std::move(
                            *characterEditSnapshot_);

                    characterEditSnapshot_.reset();
                    characterFaceSnapshot_.reset();
                    characterFaceCamera_ = false;
                    characterVisible_ = true;
                    characterYaw_ = 0.0f;
                    renderer_.SetCamera(
                        CharacterCamera());

                    std::string rebuildError;

                    if (!RebuildCharacter(
                            rebuildError))
                    {
                        core::Log::Error(
                            "Character editor cancel rebuild failed: " +
                            rebuildError);

                        return false;
                    }

                    break;
                }

                case frontend::FrontendEventType::CharacterClothesOpen:
                {
                    if (!rendererInitialized_ ||
                        (characterProfile_.has_value() &&
                         !characterEditSnapshot_.has_value())) break;
                    characterFaceSnapshot_.reset();
                    characterFaceCamera_ = false;
                    renderer_.SetCamera(CharacterCamera());
                    break;
                }

                case frontend::FrontendEventType::CharacterFaceOpen:
                {
                    if (!rendererInitialized_ ||
                        (characterProfile_.has_value() &&
                         !characterEditSnapshot_.has_value())) break;
                    characterFaceSnapshot_ = characterState_.Face();
                    characterFaceCamera_ = true;
                    renderer_.SetCamera(CharacterCamera());
                    frontend_.SendCharacterFaceState(characterState_.Face());
                    break;
                }

                case frontend::FrontendEventType::CharacterFaceValue:
                {
                    if (!rendererInitialized_ || !characterFaceSnapshot_.has_value()) break;
                    bool modelChanged = false;
                    std::string faceError;
                    if (!characterState_.ApplyFaceValue(
                            characterCatalog_, event.faceChoiceGroup, event.faceValue,
                            modelChanged, faceError))
                    {
                        core::Log::Warning(faceError);
                        frontend_.SendCharacterFaceState(characterState_.Face());
                        break;
                    }
                    std::string rebuildError;
                    if (!RebuildCharacter(rebuildError))
                    {
                        core::Log::Error("Character face rebuild failed: " + rebuildError);
                        return false;
                    }
                    frontend_.SendCharacterFaceState(characterState_.Face());
                    break;
                }

                case frontend::FrontendEventType::CharacterFaceRandom:
                {
                    if (!rendererInitialized_ || !characterFaceSnapshot_.has_value()) break;
                    static std::mt19937 random(std::random_device{}());
                    std::string faceError;
                    if (!characterState_.RandomizeFace(characterCatalog_, random, faceError))
                    {
                        core::Log::Warning(faceError);
                        break;
                    }
                    std::string rebuildError;
                    if (!RebuildCharacter(rebuildError)) return false;
                    frontend_.SendCharacterFaceState(characterState_.Face());
                    break;
                }

                case frontend::FrontendEventType::CharacterFaceReset:
                {
                    if (!rendererInitialized_ || !characterFaceSnapshot_.has_value()) break;
                    std::string faceError;
                    if (!characterState_.ResetFace(characterCatalog_, faceError))
                    {
                        core::Log::Warning(faceError);
                        break;
                    }
                    std::string rebuildError;
                    if (!RebuildCharacter(rebuildError)) return false;
                    frontend_.SendCharacterFaceState(characterState_.Face());
                    break;
                }

                case frontend::FrontendEventType::CharacterFaceApply:
                {
                    characterFaceSnapshot_.reset();
                    characterFaceCamera_ = false;
                    if (rendererInitialized_) renderer_.SetCamera(CharacterCamera());
                    break;
                }

                case frontend::FrontendEventType::CharacterFaceCancel:
                {
                    if (rendererInitialized_ && characterFaceSnapshot_.has_value())
                    {
                        std::string faceError;
                        if (!characterState_.ApplyFaceState(
                                characterCatalog_, *characterFaceSnapshot_, faceError))
                        {
                            core::Log::Warning(faceError);
                        }
                        else
                        {
                            std::string rebuildError;
                            if (!RebuildCharacter(rebuildError)) return false;
                        }
                    }
                    characterFaceSnapshot_.reset();
                    characterFaceCamera_ = false;
                    if (rendererInitialized_) renderer_.SetCamera(CharacterCamera());
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

                    std::vector<character::AppearancePart>
                        currentAppearance;

                    std::string
                        createError;

                    if (!characterState_.CreatorAppearance(
                            characterCatalog_,
                            currentAppearance,
                            createError))
                    {
                        frontend_.SendCharacterCreateResult(
                            false,
                            createError,
                            nullptr);

                        break;
                    }

                    if (!characterService_.Create(
                            accountSession_.Login(),
                            event.characterName,
                            characterCatalog_,
                            currentAppearance,
                            characterState_.Face(),
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

                        if (!characterService_.Delete(
                                accountSession_.Login(),
                                deleteError))
                        {
                            core::Log::Warning(
                                std::string(
                                    "Unable to rollback created character: ") +
                                deleteError);
                        }

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

                    characterEditSnapshot_.reset();
                    characterFaceSnapshot_.reset();
                    characterFaceCamera_ = false;

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
                    if (!rendererInitialized_ ||
                        (characterProfile_.has_value() &&
                         !characterEditSnapshot_.has_value()))
                    {
                        break;
                    }

                    std::string stateError;

                    if (!characterState_.
                            ApplyCreatorSelection(
                                characterCatalog_,
                                event.characterGroup,
                                event.characterItemType,
                                event.characterColour,
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
                    if (!rendererInitialized_ ||
                        (characterProfile_.has_value() &&
                         !characterEditSnapshot_.has_value()))
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

                    characterYawReturning_ =
                        false;

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

                case frontend::FrontendEventType::CharacterRotateReset:
                {
                    if (!rendererInitialized_ ||
                        !characterVisible_)
                    {
                        break;
                    }

                    characterYawReturning_ =
                        std::abs(characterYaw_) >
                        CharacterRotationReturnEpsilon;

                    if (!characterYawReturning_)
                    {
                        characterYaw_ =
                            0.0f;
                    }

                    break;
                }

                case frontend::FrontendEventType::Play:
                {
                    if (!characterProfile_.
                            has_value())
                    {
                        frontend_.SetWorldLoading(
                            false);

                        core::Log::Warning(
                            "Play ignored: account has no character.");

                        break;
                    }

                    if (state_ !=
                        states::ClientState::Frontend)
                    {
                        frontend_.SetWorldLoading(
                            false);

                        core::Log::Warning(
                            "Play ignored: client is not in Frontend state.");

                        break;
                    }

                    core::Log::Info(
                        "Frontend requested Play.");

                    worldFirstFramePending_ =
                        false;

                    state_ =
                        states::ClientState::LoadingWorld;

                    core::Log::Info(
                        "Client state: LoadingWorld");

                    audio_.StopMenuMusic();

                    ShutdownCharacterSelectScene();

                        frontend_.SetWorldLoadingProgress(
                            1,
                            loading::stage::Preparing);

                        const loading::ProgressCallback
                            loadingProgress =
                                [this](
                                    const std::uint32_t percent,
                                    const std::string_view stage)
                                {
                                    frontend_.
                                        SetWorldLoadingProgress(
                                            percent,
                                            stage);
                                };
                        
                    std::string
                        worldError;

                    if (!worldSession_.Load(
                        runtime_,
                        window_,
                        renderer_,
                        TestWorldSpace,
                        *characterProfile_,
                        loadingProgress,
                        worldError))
                    {
                        core::Log::Error(
                            worldError);

                        state_ =
                            states::ClientState::Frontend;

                        core::Log::Info(
                            "Client state: Frontend");

                        std::string
                            characterSceneError;

                        if (!InitializeCharacterSelectScene(
                                characterSceneError))
                        {
                            core::Log::Error(
                                std::string(
                                    "Unable to restore character menu after world load failure: ") +
                                characterSceneError);

                            return false;
                        }

                        frontend_.Show();

                        frontend_.SetWorldLoading(
                            false);

                        if (audio_.IsInitialized())
                        {
                            std::string
                                musicError;

                            if (!audio_.StartMenuMusic(
                                    musicError))
                            {
                                core::Log::Warning(
                                    std::string(
                                        "Unable to restore menu music: ") +
                                    musicError);
                            }
                        }

                        break;
                    }

                    state_ =
                        states::ClientState::World;

                    worldFirstFramePending_ =
                        true;

                    core::Log::Info(
                        std::string(
                            "Client state: World, space=") +
                        std::string(
                            worldSession_.SpaceName()));

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
        worldFirstFramePending_ =
            false;

        frontend_.Shutdown();

        worldSession_.Unload(
            renderer_);

        ShutdownCharacterSelectScene();

        renderer_.Shutdown();

        rendererInitialized_ =
            false;

        audio_.Shutdown();

        characterProfile_.reset();
        characterEditSnapshot_.reset();
        characterFaceSnapshot_.reset();
        characterFaceCamera_ = false;
        
        accountSession_.Clear();

        window_.Shutdown();
        runtime_.Shutdown();

        state_ =
            states::ClientState::Exit;
    }
}
