#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace FMOD
{
    class Channel;
    class EventProject;
    class EventSystem;
    class Sound;
    class System;
}

namespace client::audio
{
    class AudioSystem final
    {
    public:
        AudioSystem() = default;
        ~AudioSystem();

        AudioSystem(const AudioSystem&) = delete;
        AudioSystem& operator=(const AudioSystem&) = delete;

        [[nodiscard]]
        bool Initialize(
            const std::filesystem::path& gameRoot,
            std::string& error);

        [[nodiscard]]
        bool Update(
            std::string& error);

        [[nodiscard]]
        bool PlayUiSound(
            std::string_view soundName,
            std::string& error);

        [[nodiscard]]
        bool StartMenuMusic(
            std::string& error);

        void StopMenuMusic() noexcept;
        void Shutdown() noexcept;

        [[nodiscard]]
        bool IsInitialized() const noexcept;

    private:
        FMOD::EventSystem* eventSystem_ = nullptr;
        FMOD::System* coreSystem_ = nullptr;
        FMOD::EventProject* project_ = nullptr;

        FMOD::Sound* menuMusic_ = nullptr;
        FMOD::Channel* menuChannel_ = nullptr;

        bool initialized_ = false;
    };
}