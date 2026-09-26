#pragma once

#include "Character/CharacterProfile.h"
#include "Core/Resources/ResourceFileSystem.h"

#include <Windows.h>
#include <objbase.h>
#include <wrl.h>
#include <WebView2.h>

#include <cstdint>
#include <deque>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace client::frontend
{
    enum class FrontendEventType
    {
        Login,

        Exit,
        OpenUrl,
        Play,

        UiSound,

        CharacterRequest,
        CharacterCreatorReset,
        CharacterCreatorRandom,
        CharacterCreatorCancel,
        CharacterEditOpen,
        CharacterEditReset,
        CharacterEditRandom,
        CharacterEditApply,
        CharacterEditCancel,
        CharacterCreate,
        CharacterDelete,
        CharacterClothesOpen,
        CharacterFaceOpen,
        CharacterFaceValue,
        CharacterFaceRandom,
        CharacterFaceReset,
        CharacterFaceApply,
        CharacterFaceCancel,

        CharacterShow,
        CharacterHide,
        CharacterPart,
        CharacterFull,
        CharacterRotate
    };

    struct FrontendEvent final
    {
        FrontendEventType type =
            FrontendEventType::Exit;

        std::string login;
        std::string password;
        std::string url;
        std::string soundName;

        std::string characterName;

        std::string dummyGroup;

        std::int32_t dummyPartId =
            0;

        std::string characterGroup;

        std::int32_t characterItemType =
            0;

        std::uint32_t characterColour =
            0xFFFFFFu;

        std::string faceChoiceGroup;

        std::uint64_t faceValue =
            0;

        float characterDeltaX =
            0.0f;

        float characterDeltaY =
            0.0f;

        std::vector<character::AppearancePart>
            characterParts;

        float dummyDeltaX =
            0.0f;

        float dummyDeltaY =
            0.0f;

        std::vector<
            std::pair<
                std::string,
                std::int32_t>>
            dummyParts;

        bool rememberLogin =
            false;
    };

    class OriginalFrontend final
    {
    public:
        OriginalFrontend() = default;
        ~OriginalFrontend();

        OriginalFrontend(
            const OriginalFrontend&) =
                delete;

        OriginalFrontend& operator=(
            const OriginalFrontend&) =
                delete;

        [[nodiscard]]
        bool Initialize(
            HWND parentWindow,
            const std::filesystem::path& gameRoot,
            const core::resources::ResourceFileSystem& resources,
            std::string& error);

        void Shutdown();

        void Resize();

        [[nodiscard]]
        bool ConsumeEvent(
            FrontendEvent& event);

        [[nodiscard]]
        bool ConsumeFatalError(
            std::string& error);

        void SendLoginError(
            const std::string& message);

        void SendLoginComplete(
            const character::Profile* profile);

        void SendCharacterState(
            const character::Profile* profile);

        void SendCharacterCreateResult(
            bool success,
            const std::string& message,
            const character::Profile* profile);

        void SendCharacterDeleteResult(
            bool success,
            const std::string& message);

        void SendCharacterEditResult(
            bool success,
            const std::string& message,
            const character::Profile* profile);

        void SendCharacterFaceState(
            const character::FaceState& face);

        void Hide();

        void Show();

    private:
        void HandleWebMessage(
            ICoreWebView2WebMessageReceivedEventArgs* args);

        void SendLocalization(
            const std::string& language,
            const std::vector<std::string>& paths);

        void ExecuteScriptUtf8(
            const std::string& script);

        void SetFatalError(
            std::string error);

        HWND parentWindow_ =
            nullptr;

        std::filesystem::path
            gameRoot_;

        const core::resources::ResourceFileSystem*
            resources_ =
                nullptr;

        Microsoft::WRL::ComPtr<
            ICoreWebView2Environment>
                environment_;

        Microsoft::WRL::ComPtr<
            ICoreWebView2Controller>
                controller_;

        Microsoft::WRL::ComPtr<
            ICoreWebView2>
                webView_;

        EventRegistrationToken
            webMessageToken_{};

        std::deque<FrontendEvent>
            events_;

        std::string
            fatalError_;

        bool webMessageRegistered_ =
            false;

        bool comInitialized_ =
            false;

        bool shuttingDown_ =
            false;
    };
}
