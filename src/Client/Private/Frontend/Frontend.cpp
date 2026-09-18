#include "Frontend/Frontend.h"

#include "Core/Log.h"
#include "Core/Resources/DataSection.h"
#include "Core/Resources/PackedSectionReader.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <locale>
#include <map>
#include <sstream>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    using Microsoft::WRL::Callback;
    using Microsoft::WRL::ComPtr;

    constexpr wchar_t FrontendHost[] =
        L"zone.local";

    constexpr wchar_t FrontendUrl[] =
        L"https://zone.local/packs/frontend/index.html";

    int HexValue(
        const char value)
    {
        if (value >= '0' &&
            value <= '9')
        {
            return value - '0';
        }

        if (value >= 'a' &&
            value <= 'f')
        {
            return
                10 +
                value -
                'a';
        }

        if (value >= 'A' &&
            value <= 'F')
        {
            return
                10 +
                value -
                'A';
        }

        return -1;
    }

    std::string PercentDecode(
        const std::string_view value)
    {
        std::string result;

        result.reserve(
            value.size());

        for (std::size_t index = 0;
             index < value.size();
             ++index)
        {
            if (value[index] == '%' &&
                index + 2 <
                    value.size())
            {
                const int high =
                    HexValue(
                        value[
                            index + 1]);

                const int low =
                    HexValue(
                        value[
                            index + 2]);

                if (high >= 0 &&
                    low >= 0)
                {
                    result.push_back(
                        static_cast<char>(
                            (high << 4) |
                            low));

                    index += 2;

                    continue;
                }
            }

            result.push_back(
                value[index]);
        }

        return result;
    }

    std::vector<std::string>
    SplitMessage(
        const std::string& message)
    {
        std::vector<std::string>
            result;

        std::size_t begin =
            0;

        while (true)
        {
            const std::size_t tab =
                message.find(
                    '\t',
                    begin);

            if (tab ==
                std::string::npos)
            {
                result.push_back(
                    message.substr(
                        begin));

                break;
            }

            result.push_back(
                message.substr(
                    begin,
                    tab - begin));

            begin =
                tab + 1;
        }

        for (std::size_t index = 1;
             index < result.size();
             ++index)
        {
            result[index] =
                PercentDecode(
                    result[index]);
        }

        return result;
    }

    std::wstring Utf8ToWide(
        const std::string_view value)
    {
        if (value.empty())
        {
            return {};
        }

        int length =
            MultiByteToWideChar(
                CP_UTF8,
                MB_ERR_INVALID_CHARS,
                value.data(),
                static_cast<int>(
                    value.size()),
                nullptr,
                0);

        UINT codePage =
            CP_UTF8;

        DWORD flags =
            MB_ERR_INVALID_CHARS;

        if (length <= 0)
        {
            codePage =
                1251;

            flags =
                0;

            length =
                MultiByteToWideChar(
                    codePage,
                    flags,
                    value.data(),
                    static_cast<int>(
                        value.size()),
                    nullptr,
                    0);
        }

        if (length <= 0)
        {
            return {};
        }

        std::wstring result(
            static_cast<std::size_t>(
                length),
            L'\0');

        MultiByteToWideChar(
            codePage,
            flags,
            value.data(),
            static_cast<int>(
                value.size()),
            result.data(),
            length);

        return result;
    }

    std::string WideToUtf8(
        const std::wstring_view value)
    {
        if (value.empty())
        {
            return {};
        }

        const int length =
            WideCharToMultiByte(
                CP_UTF8,
                0,
                value.data(),
                static_cast<int>(
                    value.size()),
                nullptr,
                0,
                nullptr,
                nullptr);

        if (length <= 0)
        {
            return {};
        }

        std::string result(
            static_cast<std::size_t>(
                length),
            '\0');

        WideCharToMultiByte(
            CP_UTF8,
            0,
            value.data(),
            static_cast<int>(
                value.size()),
            result.data(),
            length,
            nullptr,
            nullptr);

        return result;
    }

    std::string NormalizeUtf8(
        const std::string_view value)
    {
        const std::wstring wide =
            Utf8ToWide(
                value);

        if (wide.empty() &&
            !value.empty())
        {
            return {};
        }

        return
            WideToUtf8(
                wide);
    }

    std::string JsonString(
        const std::string_view value)
    {
        const std::string utf8 =
            NormalizeUtf8(
                value);

        std::string result;

        result.push_back(
            '"');

        constexpr char Hex[] =
            "0123456789ABCDEF";

        for (const unsigned char character :
             utf8)
        {
            switch (character)
            {
                case '"':
                {
                    result +=
                        "\\\"";

                    break;
                }

                case '\\':
                {
                    result +=
                        "\\\\";

                    break;
                }

                case '\b':
                {
                    result +=
                        "\\b";

                    break;
                }

                case '\f':
                {
                    result +=
                        "\\f";

                    break;
                }

                case '\n':
                {
                    result +=
                        "\\n";

                    break;
                }

                case '\r':
                {
                    result +=
                        "\\r";

                    break;
                }

                case '\t':
                {
                    result +=
                        "\\t";

                    break;
                }

                default:
                {
                    if (character <
                        0x20)
                    {
                        result +=
                            "\\u00";

                        result.push_back(
                            Hex[
                                character >>
                                4]);

                        result.push_back(
                            Hex[
                                character &
                                0x0F]);
                    }
                    else
                    {
                        result.push_back(
                            static_cast<char>(
                                character));
                    }

                    break;
                }
            }
        }

        result.push_back(
            '"');

        return result;
    }

    std::string SerializeSection(
        const core::resources::DataSection&
            section);

    std::string SerializeSectionValue(
        const core::resources::DataSection&
            section)
    {
        if (const std::string* value =
                section.AsString())
        {
            return
                JsonString(
                    *value);
        }

        if (const std::int64_t* value =
                section.AsInteger())
        {
            return
                std::to_string(
                    *value);
        }

        if (const bool* value =
                section.AsBoolean())
        {
            return
                *value
                    ? "true"
                    : "false";
        }

        if (const auto* values =
                section.AsFloats())
        {
            std::ostringstream stream;

            stream.imbue(
                std::locale::classic());

            stream <<
                std::setprecision(
                    9);

            if (values->size() ==
                1)
            {
                stream <<
                    (*values)[0];

                return
                    stream.str();
            }

            stream <<
                '[';

            for (std::size_t index = 0;
                 index < values->size();
                 ++index)
            {
                if (index !=
                    0)
                {
                    stream <<
                        ',';
                }

                stream <<
                    (*values)[index];
            }

            stream <<
                ']';

            return
                stream.str();
        }

        if (const auto* binary =
                section.AsBinary())
        {
            if (binary->empty())
            {
                return "\"\"";
            }

            const std::string value(
                reinterpret_cast<
                    const char*>(
                        binary->data()),
                binary->size());

            return
                JsonString(
                    value);
        }

        return "\"\"";
    }

    std::string SerializeSection(
        const core::resources::DataSection&
            section)
    {
        if (section.children.empty())
        {
            return
                SerializeSectionValue(
                    section);
        }
        
        std::map<
            std::string,
            const core::resources::DataSection*>
                children;

        for (const auto& child :
             section.children)
        {
            children[
                child.name] =
                    &child;
        }

        std::string result =
            "{";

        bool first =
            true;

        for (const auto& [
                 name,
                 child] :
             children)
        {
            if (!first)
            {
                result +=
                    ',';
            }

            first =
                false;

            result +=
                JsonString(
                    name);

            result +=
                ':';

            result +=
                SerializeSection(
                    *child);
        }

        result +=
            '}';

        return result;
    }

    std::string ReadLocalizationFile(
        const core::resources::ResourceFileSystem&
            resources,
        const std::string& language,
        const std::string& requestedPath)
    {
        const std::filesystem::path
            requested(
                requestedPath);

        std::string fileName =
            requested.
                filename().
                string();

        if (fileName.empty())
        {
            return "{}";
        }

        const bool english =
            language ==
                "english" ||
            language ==
                "English";

        const std::string folder =
            english
                ? "English"
                : "Russian";

        std::string logicalPath =
            "res/local/" +
            folder +
            "/" +
            fileName;

        if (!resources.Exists(
                logicalPath) &&
            requested.
                extension().
                empty())
        {
            logicalPath +=
                ".xml";
        }

        std::vector<std::byte>
            data;

        if (!resources.ReadBinary(
                logicalPath,
                data))
        {
            core::Log::Warning(
                std::string(
                    "Frontend localization not found: ") +
                logicalPath);

            return "{}";
        }

        const std::span<
            const std::byte>
                bytes(
                    data.data(),
                    data.size());

        if (core::resources::
                PackedSectionReader::
                HasSignature(
                    bytes))
        {
            core::resources::
                DataSection root;

            std::string error;

            core::resources::
                PackedSectionReader
                    reader;

            if (!reader.Read(
                    bytes,
                    root,
                    error))
            {
                core::Log::Warning(
                    std::string(
                        "Frontend localization decode failed: ") +
                    logicalPath +
                    " : " +
                    error);

                return "{}";
            }

            return
                SerializeSection(
                    root);
        }
        
        std::string raw(
            reinterpret_cast<
                const char*>(
                    data.data()),
            data.size());

        const auto first =
            std::find_if_not(
                raw.begin(),
                raw.end(),
                [](const unsigned char value)
                {
                    return
                        std::isspace(
                            value) !=
                        0;
                });

        if (first !=
                raw.end() &&
            (*first == '{' ||
             *first == '['))
        {
            return raw;
        }

        return "{}";
    }

    std::string BuildLocalizationJson(
        const core::resources::ResourceFileSystem&
            resources,
        const std::string& language,
        const std::vector<std::string>& paths)
    {
        std::string result =
            "{";

        for (std::size_t index = 0;
             index < paths.size();
             ++index)
        {
            if (index !=
                0)
            {
                result +=
                    ',';
            }

            result +=
                JsonString(
                    paths[index]);

            result +=
                ':';

            result +=
                ReadLocalizationFile(
                    resources,
                    language,
                    paths[index]);
        }

        result +=
            '}';

        return result;
    }

    std::string HResultText(
        const HRESULT result)
    {
        std::ostringstream stream;

        stream <<
            "0x" <<
            std::hex <<
            std::uppercase <<
            static_cast<
                unsigned long>(
                    result);

        return
            stream.str();
    }
}

namespace client::frontend
{
    OriginalFrontend::~OriginalFrontend()
    {
        Shutdown();
    }

    bool OriginalFrontend::Initialize(
        const HWND parentWindow,
        const std::filesystem::path& gameRoot,
        const core::resources::ResourceFileSystem& resources,
        std::string& error)
    {
        Shutdown();

        error.clear();

        parentWindow_ =
            parentWindow;

        gameRoot_ =
            gameRoot;

        resources_ =
            &resources;

        shuttingDown_ =
            false;

        fatalError_.clear();
        events_.clear();

        if (parentWindow_ ==
            nullptr)
        {
            error =
                "Frontend parent window is null.";

            return false;
        }

        if (gameRoot_.empty())
        {
            error =
                "Frontend game root is empty.";

            return false;
        }

        const std::filesystem::path
            frontendIndex =
                gameRoot_ /
                "packs" /
                "frontend" /
                "index.html";

        if (!std::filesystem::exists(
                frontendIndex))
        {
            error =
                "Missing packs/frontend/index.html";

            return false;
        }

        const std::filesystem::path
            ruffleScript =
                gameRoot_ /
                "packs" /
                "frontend" /
                "ruffle" /
                "ruffle.js";

        if (!std::filesystem::exists(
                ruffleScript))
        {
            error =
                "Missing packs/frontend/ruffle/ruffle.js";

            return false;
        }

        HRESULT result =
            CoInitializeEx(
                nullptr,
                COINIT_APARTMENTTHREADED);

        if (result ==
            RPC_E_CHANGED_MODE)
        {
            error =
                "WebView2 requires STA COM initialization.";

            return false;
        }

        if (FAILED(result))
        {
            error =
                "COM initialization failed: " +
                HResultText(
                    result);

            return false;
        }

        comInitialized_ =
            true;

        const std::filesystem::path
            userDataDirectory =
                gameRoot_ /
                "user" /
                "webview2";

        std::error_code
            directoryError;

        std::filesystem::
            create_directories(
                userDataDirectory,
                directoryError);

        result =
            CreateCoreWebView2EnvironmentWithOptions(
                nullptr,
                userDataDirectory.
                    c_str(),
                nullptr,
                Callback<
                    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                    [this](
                        const HRESULT environmentResult,
                        ICoreWebView2Environment* environment)
                        -> HRESULT
                    {
                        if (shuttingDown_)
                        {
                            return S_OK;
                        }

                        if (FAILED(
                                environmentResult) ||
                            environment ==
                                nullptr)
                        {
                            SetFatalError(
                                "WebView2 environment creation failed: " +
                                HResultText(
                                    environmentResult));

                            return S_OK;
                        }

                        environment_ =
                            environment;

                        const HRESULT controllerResult =
                            environment_->
                                CreateCoreWebView2Controller(
                                    parentWindow_,
                                    Callback<
                                        ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                                        [this](
                                            const HRESULT result,
                                            ICoreWebView2Controller* controller)
                                            -> HRESULT
                                        {
                                            if (shuttingDown_)
                                            {
                                                return S_OK;
                                            }

                                            if (FAILED(
                                                    result) ||
                                                controller ==
                                                    nullptr)
                                            {
                                                SetFatalError(
                                                    "WebView2 controller creation failed: " +
                                                    HResultText(
                                                        result));

                                                return S_OK;
                                            }

                                            controller_ =
                                                controller;

                                            HRESULT webViewResult =
                                                controller_->
                                                    get_CoreWebView2(
                                                        webView_.
                                                            GetAddressOf());

                                            if (FAILED(
                                                    webViewResult) ||
                                                !webView_)
                                            {
                                                SetFatalError(
                                                    "Unable to obtain CoreWebView2.");

                                                return S_OK;
                                            }

                                            ComPtr<
                                                ICoreWebView2Settings>
                                                    settings;

                                            if (SUCCEEDED(
                                                    webView_->
                                                        get_Settings(
                                                            settings.
                                                                GetAddressOf())) &&
                                                settings)
                                            {
                                                settings->
                                                    put_AreDefaultContextMenusEnabled(
                                                        FALSE);

                                                settings->
                                                    put_AreDevToolsEnabled(
                                                        FALSE);

                                                settings->
                                                    put_IsStatusBarEnabled(
                                                        FALSE);

                                                settings->
                                                    put_IsZoomControlEnabled(
                                                        FALSE);
                                            }

                                            ComPtr<
                                                ICoreWebView2_3>
                                                    webView3;

                                            if (FAILED(
                                                    webView_.
                                                        As(
                                                            &webView3)) ||
                                                !webView3)
                                            {
                                                SetFatalError(
                                                    "ICoreWebView2_3 is unavailable.");

                                                return S_OK;
                                            }

                                            const HRESULT mappingResult =
                                                webView3->
                                                    SetVirtualHostNameToFolderMapping(
                                                        FrontendHost,
                                                        gameRoot_.
                                                            c_str(),
                                                        COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW);

                                            if (FAILED(
                                                    mappingResult))
                                            {
                                                SetFatalError(
                                                    "Unable to map frontend resource folder: " +
                                                    HResultText(
                                                        mappingResult));

                                                return S_OK;
                                            }

                                            const HRESULT messageResult =
                                                webView_->
                                                    add_WebMessageReceived(
                                                        Callback<
                                                            ICoreWebView2WebMessageReceivedEventHandler>(
                                                            [this](
                                                                ICoreWebView2*,
                                                                ICoreWebView2WebMessageReceivedEventArgs* args)
                                                                -> HRESULT
                                                            {
                                                                HandleWebMessage(
                                                                    args);

                                                                return S_OK;
                                                            }).
                                                            Get(),
                                                        &webMessageToken_);

                                            if (FAILED(
                                                    messageResult))
                                            {
                                                SetFatalError(
                                                    "Unable to register frontend message bridge.");

                                                return S_OK;
                                            }

                                            webMessageRegistered_ =
                                                true;

                                            Resize();

                                            controller_->
                                                put_IsVisible(
                                                    TRUE);

                                            const HRESULT navigationResult =
                                                webView_->
                                                    Navigate(
                                                        FrontendUrl);

                                            if (FAILED(
                                                    navigationResult))
                                            {
                                                SetFatalError(
                                                    "Unable to navigate to original frontend.");

                                                return S_OK;
                                            }

                                            return S_OK;
                                        }).
                                        Get());

                        if (FAILED(
                                controllerResult))
                        {
                            SetFatalError(
                                "CreateCoreWebView2Controller failed: " +
                                HResultText(
                                    controllerResult));
                        }

                        return S_OK;
                    }).
                    Get());

        if (FAILED(result))
        {
            error =
                "CreateCoreWebView2EnvironmentWithOptions failed: " +
                HResultText(
                    result);

            Shutdown();

            return false;
        }

        return true;
    }

    void OriginalFrontend::Shutdown()
    {
        shuttingDown_ =
            true;

        events_.clear();

        if (webView_ &&
            webMessageRegistered_)
        {
            webView_->
                remove_WebMessageReceived(
                    webMessageToken_);

            webMessageRegistered_ =
                false;
        }

        if (controller_)
        {
            controller_->
                put_IsVisible(
                    FALSE);

            controller_->
                Close();
        }

        webView_.Reset();
        controller_.Reset();
        environment_.Reset();

        resources_ =
            nullptr;

        parentWindow_ =
            nullptr;

        gameRoot_.clear();

        fatalError_.clear();

        if (comInitialized_)
        {
            CoUninitialize();

            comInitialized_ =
                false;
        }
    }

    void OriginalFrontend::Resize()
    {
        if (!controller_ ||
            parentWindow_ ==
                nullptr)
        {
            return;
        }

        RECT bounds{};

        if (!GetClientRect(
                parentWindow_,
                &bounds))
        {
            return;
        }

        controller_->
            put_Bounds(
                bounds);
    }

    bool OriginalFrontend::ConsumeEvent(
        FrontendEvent& event)
    {
        if (events_.empty())
        {
            return false;
        }

        event =
            std::move(
                events_.front());

        events_.pop_front();

        return true;
    }

    bool OriginalFrontend::ConsumeFatalError(
        std::string& error)
    {
        if (fatalError_.empty())
        {
            return false;
        }

        error =
            std::move(
                fatalError_);

        fatalError_.clear();

        return true;
    }

    void OriginalFrontend::SendLoginError(
        const std::string& message)
    {
        ExecuteScriptUtf8(
            "if(window.ZoneFrontend){"
            "window.ZoneFrontend.loginError(" +
            JsonString(
                message) +
            ");"
            "}");
    }

    void OriginalFrontend::SendLoginAccepted()
    {
        ExecuteScriptUtf8(
            "if(window.ZoneFrontend){"
            "window.ZoneFrontend.loginAccepted();"
            "}");
    }

    void OriginalFrontend::Hide()
    {
        if (controller_)
        {
            controller_->
                put_IsVisible(
                    FALSE);
        }
    }

    void OriginalFrontend::Show()
    {
        if (controller_)
        {
            controller_->
                put_IsVisible(
                    TRUE);
        }
    }

    void OriginalFrontend::HandleWebMessage(
        ICoreWebView2WebMessageReceivedEventArgs* args)
    {
        if (args ==
            nullptr)
        {
            return;
        }

        LPWSTR rawMessage =
            nullptr;

        const HRESULT result =
            args->
                TryGetWebMessageAsString(
                    &rawMessage);

        if (FAILED(result) ||
            rawMessage ==
                nullptr)
        {
            return;
        }

        const std::wstring
            messageWide(
                rawMessage);

        CoTaskMemFree(
            rawMessage);

        const std::string message =
            WideToUtf8(
                messageWide);

        std::vector<std::string>
            fields =
                SplitMessage(
                    message);

        if (fields.empty())
        {
            return;
        }

        const std::string&
            command =
                fields[0];

        if (command ==
            "ready")
        {
            core::Log::Info(
                "Original SOnline frontend ready");

            return;
        }

        if (command ==
            "trace")
        {
            if (fields.size() >
                1)
            {
                core::Log::Info(
                    std::string(
                        "[Frontend] ") +
                    fields[1]);
            }

            return;
        }

        if (command ==
            "localize")
        {
            if (fields.size() <
                3)
            {
                return;
            }

            std::vector<std::string>
                paths;

            paths.reserve(
                fields.size() -
                2);

            for (std::size_t index = 2;
                 index < fields.size();
                 ++index)
            {
                paths.push_back(
                    fields[index]);
            }

            SendLocalization(
                fields[1],
                paths);

            return;
        }

        if (command ==
            "login")
        {
            if (fields.size() <
                5)
            {
                core::Log::Warning(
                    "Invalid frontend login message.");

                return;
            }

            FrontendEvent event;

            event.type =
                FrontendEventType::Login;

            event.login =
                fields[1];

            event.password =
                fields[2];

            event.rememberLogin =
                fields[3] ==
                "1";

            event.serverId =
                fields[4];

            events_.push_back(
                std::move(
                    event));

            return;
        }

        if (command ==
            "quit")
        {
            FrontendEvent event;

            event.type =
                FrontendEventType::Exit;

            events_.push_back(
                std::move(
                    event));

            return;
        }

        if (command ==
            "open_url")
        {
            FrontendEvent event;

            event.type =
                FrontendEventType::OpenUrl;

            if (fields.size() >
                1)
            {
                event.url =
                    fields[1];
            }

            events_.push_back(
                std::move(
                    event));

            return;
        }

        if (command ==
            "play")
        {
            FrontendEvent event;

            event.type =
                FrontendEventType::Play;

            events_.push_back(
                std::move(
                    event));
        }
    }

    void OriginalFrontend::SendLocalization(
        const std::string& language,
        const std::vector<std::string>& paths)
    {
        if (resources_ ==
            nullptr)
        {
            return;
        }

        const std::string json =
            BuildLocalizationJson(
                *resources_,
                language,
                paths);

        ExecuteScriptUtf8(
            "if(window.ZoneFrontend){"
            "window.ZoneFrontend.localizationResult(" +
            json +
            ");"
            "}");
    }

    void OriginalFrontend::ExecuteScriptUtf8(
        const std::string& script)
    {
        if (!webView_)
        {
            return;
        }

        const std::wstring
            wide =
                Utf8ToWide(
                    script);

        if (wide.empty())
        {
            return;
        }

        webView_->
            ExecuteScript(
                wide.c_str(),
                nullptr);
    }

    void OriginalFrontend::SetFatalError(
        std::string error)
    {
        if (!fatalError_.empty())
        {
            return;
        }

        fatalError_ =
            std::move(
                error);
    }
}