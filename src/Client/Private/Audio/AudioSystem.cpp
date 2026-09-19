#include "Audio/AudioSystem.h"

#include <fmod_errors.h>
#include <fmod_event.hpp>

#include <system_error>

namespace
{
    bool CheckFmodResult(
        const FMOD_RESULT result,
        const std::string_view operation,
        std::string& error)
    {
        if (result == FMOD_OK)
        {
            return true;
        }

        error.assign(operation);
        error.append(": ");
        error.append(FMOD_ErrorString(result));

        return false;
    }
}

namespace client::audio
{
    AudioSystem::~AudioSystem()
    {
        Shutdown();
    }

    bool AudioSystem::Initialize(
        const std::filesystem::path& gameRoot,
        std::string& error)
    {
        Shutdown();
        error.clear();

        const std::filesystem::path audioDirectory =
            gameRoot /
            "packs" /
            "res" /
            "audio";

        const std::filesystem::path eventProjectFile =
            audioDirectory /
            "fd_sound.fev";

        const std::filesystem::path menuMusicFile =
            audioDirectory /
            "ZoneReborn_main_v1.mp3";

        std::error_code fileError;

        if (!std::filesystem::is_regular_file(
                eventProjectFile,
                fileError))
        {
            error =
                "FMOD event project not found: " +
                eventProjectFile.string();

            return false;
        }

        fileError.clear();

        if (!std::filesystem::is_regular_file(
                menuMusicFile,
                fileError))
        {
            error =
                "Menu music file not found: " +
                menuMusicFile.string();

            return false;
        }

        FMOD_RESULT result =
            FMOD::EventSystem_Create(
                &eventSystem_);

        if (!CheckFmodResult(
                result,
                "FMOD::EventSystem_Create",
                error))
        {
            Shutdown();
            return false;
        }

        result =
            eventSystem_->init(
                128,
                FMOD_INIT_NORMAL,
                nullptr,
                FMOD_EVENT_INIT_NORMAL);

        if (!CheckFmodResult(
                result,
                "FMOD::EventSystem::init",
                error))
        {
            Shutdown();
            return false;
        }

        result =
            eventSystem_->getSystemObject(
                &coreSystem_);

        if (!CheckFmodResult(
                result,
                "FMOD::EventSystem::getSystemObject",
                error))
        {
            Shutdown();
            return false;
        }

        std::string mediaPath =
            audioDirectory.generic_string();

        if (!mediaPath.empty() &&
            mediaPath.back() != '/')
        {
            mediaPath.push_back('/');
        }

        result =
            eventSystem_->setMediaPath(
                mediaPath.c_str());

        if (!CheckFmodResult(
                result,
                "FMOD::EventSystem::setMediaPath",
                error))
        {
            Shutdown();
            return false;
        }

        result =
            eventSystem_->load(
                "fd_sound.fev",
                nullptr,
                &project_);

        if (!CheckFmodResult(
                result,
                "FMOD::EventSystem::load",
                error))
        {
            Shutdown();
            return false;
        }

        const std::string menuMusicPath =
            menuMusicFile.generic_string();

        result =
            coreSystem_->createStream(
                menuMusicPath.c_str(),
                FMOD_LOOP_NORMAL |
                    FMOD_2D,
                nullptr,
                &menuMusic_);

        if (!CheckFmodResult(
                result,
                "FMOD::System::createStream",
                error))
        {
            Shutdown();
            return false;
        }

        initialized_ = true;

        if (!StartMenuMusic(
                error))
        {
            Shutdown();
            return false;
        }

        return true;
    }

    bool AudioSystem::Update(
        std::string& error)
    {
        error.clear();

        if (!initialized_ ||
            eventSystem_ == nullptr)
        {
            return true;
        }

        const FMOD_RESULT result =
            eventSystem_->update();

        if (result == FMOD_OK ||
            result == FMOD_ERR_NOTREADY)
        {
            return true;
        }

        return CheckFmodResult(
            result,
            "FMOD::EventSystem::update",
            error);
    }

    bool AudioSystem::PlayUiSound(
        const std::string_view soundName,
        std::string& error)
    {
        error.clear();

        if (!initialized_ ||
            eventSystem_ == nullptr)
        {
            return true;
        }

        const char* eventPath = nullptr;

        if (soundName == "click")
        {
            eventPath =
                "fd_sound/ui/click";
        }
        else if (soundName == "hover")
        {
            eventPath =
                "fd_sound/ui/hover";
        }
        else
        {
            return true;
        }

        FMOD::Event* event = nullptr;

        FMOD_RESULT result =
            eventSystem_->getEvent(
                eventPath,
                FMOD_EVENT_DEFAULT,
                &event);

        if (!CheckFmodResult(
                result,
                "FMOD UI getEvent",
                error))
        {
            return false;
        }

        result =
            event->start();

        return CheckFmodResult(
            result,
            "FMOD UI event start",
            error);
    }

    bool AudioSystem::StartMenuMusic(
        std::string& error)
    {
        error.clear();

        if (!initialized_ ||
            coreSystem_ == nullptr ||
            menuMusic_ == nullptr)
        {
            error =
                "Menu music is not initialized";

            return false;
        }

        StopMenuMusic();

        const FMOD_RESULT result =
            coreSystem_->playSound(
                FMOD_CHANNEL_FREE,
                menuMusic_,
                false,
                &menuChannel_);

        if (!CheckFmodResult(
                result,
                "FMOD menu music playSound",
                error))
        {
            menuChannel_ = nullptr;
            return false;
        }

        return true;
    }

    void AudioSystem::StopMenuMusic() noexcept
    {
        if (menuChannel_ == nullptr)
        {
            return;
        }

        menuChannel_->stop();
        menuChannel_ = nullptr;
    }

    void AudioSystem::Shutdown() noexcept
    {
        StopMenuMusic();

        if (menuMusic_ != nullptr)
        {
            menuMusic_->release();
            menuMusic_ = nullptr;
        }

        project_ = nullptr;
        coreSystem_ = nullptr;

        if (eventSystem_ != nullptr)
        {
            eventSystem_->unload();
            eventSystem_->release();
            eventSystem_ = nullptr;
        }

        initialized_ = false;
    }

    bool AudioSystem::IsInitialized() const noexcept
    {
        return initialized_;
    }
}