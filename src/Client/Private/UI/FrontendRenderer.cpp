#include "UI/FrontendRenderer.h"

#include "Frontend/LoginLayout.h"

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <dxgiformat.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace
{
    using Microsoft::WRL::ComPtr;

    constexpr char LoginBackgroundResource[] =
        "res/soGUI/maps/Login/login_bg.jpg";

    constexpr char StartupLogoResource[] =
        "res/soGUI/maps/loadingScreen/appStart.tga";

    bool LoadWicBitmap(
        IWICImagingFactory* imagingFactory,
        ID2D1HwndRenderTarget* target,
        const std::filesystem::path& path,
        ComPtr<ID2D1Bitmap>& bitmap,
        std::string& error)
    {
        ComPtr<IWICBitmapDecoder>
            decoder;

        HRESULT result =
            imagingFactory->
                CreateDecoderFromFilename(
                    path.c_str(),
                    nullptr,
                    GENERIC_READ,
                    WICDecodeMetadataCacheOnLoad,
                    decoder.GetAddressOf());

        if (FAILED(result))
        {
            error =
                "Unable to open frontend image: " +
                path.string();

            return false;
        }

        ComPtr<IWICBitmapFrameDecode>
            frame;

        result =
            decoder->GetFrame(
                0,
                frame.GetAddressOf());

        if (FAILED(result))
        {
            error =
                "Unable to decode frontend image.";

            return false;
        }

        ComPtr<IWICFormatConverter>
            converter;

        result =
            imagingFactory->
                CreateFormatConverter(
                    converter.GetAddressOf());

        if (FAILED(result))
        {
            error =
                "Unable to create WIC converter.";

            return false;
        }

        result =
            converter->Initialize(
                frame.Get(),
                GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone,
                nullptr,
                0.0,
                WICBitmapPaletteTypeMedianCut);

        if (FAILED(result))
        {
            error =
                "Unable to convert frontend image.";

            return false;
        }

        result =
            target->CreateBitmapFromWicBitmap(
                converter.Get(),
                nullptr,
                bitmap.GetAddressOf());

        if (FAILED(result))
        {
            error =
                "Unable to create Direct2D bitmap.";

            return false;
        }

        return true;
    }

    bool LoadTgaBitmap(
        const core::resources::ResourceFileSystem& resources,
        const char* logicalPath,
        ID2D1HwndRenderTarget* target,
        ComPtr<ID2D1Bitmap>& bitmap,
        std::string& error)
    {
        std::vector<std::byte> file;

        if (!resources.ReadBinary(
                logicalPath,
                file))
        {
            error =
                std::string(
                    "TGA resource not found: ") +
                logicalPath;

            return false;
        }

        if (file.size() <
            18)
        {
            error =
                "Invalid TGA header.";

            return false;
        }

        const auto* bytes =
            reinterpret_cast<
                const std::uint8_t*>(
                    file.data());

        const std::uint8_t idLength =
            bytes[0];

        const std::uint8_t colorMapType =
            bytes[1];

        const std::uint8_t imageType =
            bytes[2];

        const std::uint16_t width =
            static_cast<std::uint16_t>(
                bytes[12] |
                (bytes[13] << 8));

        const std::uint16_t height =
            static_cast<std::uint16_t>(
                bytes[14] |
                (bytes[15] << 8));

        const std::uint8_t bitsPerPixel =
            bytes[16];

        const std::uint8_t descriptor =
            bytes[17];

        if (colorMapType !=
            0)
        {
            error =
                "Color mapped TGA is not supported.";

            return false;
        }

        if (width ==
                0 ||
            height ==
                0)
        {
            error =
                "Invalid TGA size.";

            return false;
        }

        const bool trueColor =
            imageType ==
            2;

        const bool grayscale =
            imageType ==
            3;

        if (!trueColor &&
            !grayscale)
        {
            error =
                "Unsupported TGA image type.";

            return false;
        }

        std::size_t sourceBytesPerPixel =
            0;

        if (grayscale)
        {
            if (bitsPerPixel !=
                8)
            {
                error =
                    "Unsupported grayscale TGA format.";

                return false;
            }

            sourceBytesPerPixel =
                1;
        }
        else
        {
            if (bitsPerPixel ==
                24)
            {
                sourceBytesPerPixel =
                    3;
            }
            else if (
                bitsPerPixel ==
                32)
            {
                sourceBytesPerPixel =
                    4;
            }
            else
            {
                error =
                    "Unsupported TGA pixel format.";

                return false;
            }
        }

        const std::size_t pixelOffset =
            18 +
            static_cast<std::size_t>(
                idLength);

        const std::size_t requiredSize =
            pixelOffset +
            static_cast<std::size_t>(
                width) *
            static_cast<std::size_t>(
                height) *
            sourceBytesPerPixel;

        if (requiredSize >
            file.size())
        {
            error =
                "Incomplete TGA image.";

            return false;
        }

        std::vector<std::uint8_t>
            pixels;

        pixels.resize(
            static_cast<std::size_t>(
                width) *
            static_cast<std::size_t>(
                height) *
            4);

        const bool topOrigin =
            (descriptor &
             0x20) !=
            0;

        for (std::uint32_t y = 0;
             y < height;
             ++y)
        {
            const std::uint32_t sourceY =
                topOrigin
                    ? y
                    : static_cast<
                        std::uint32_t>(
                            height - 1 - y);

            for (std::uint32_t x = 0;
                 x < width;
                 ++x)
            {
                const std::size_t sourceIndex =
                    pixelOffset +
                    (
                        static_cast<std::size_t>(
                            sourceY) *
                            width +
                        x
                    ) *
                    sourceBytesPerPixel;

                const std::size_t destinationIndex =
                    (
                        static_cast<std::size_t>(
                            y) *
                            width +
                        x
                    ) *
                    4;

                std::uint8_t blue =
                    0;

                std::uint8_t green =
                    0;

                std::uint8_t red =
                    0;

                std::uint8_t alpha =
                    255;

                if (grayscale)
                {
                    red =
                        bytes[sourceIndex];

                    green =
                        red;

                    blue =
                        red;
                }
                else
                {
                    blue =
                        bytes[
                            sourceIndex];

                    green =
                        bytes[
                            sourceIndex +
                            1];

                    red =
                        bytes[
                            sourceIndex +
                            2];

                    if (sourceBytesPerPixel ==
                        4)
                    {
                        alpha =
                            bytes[
                                sourceIndex +
                                3];
                    }
                }

                const std::uint32_t a =
                    alpha;

                pixels[
                    destinationIndex] =
                    static_cast<std::uint8_t>(
                        blue * a /
                        255);

                pixels[
                    destinationIndex +
                    1] =
                    static_cast<std::uint8_t>(
                        green * a /
                        255);

                pixels[
                    destinationIndex +
                    2] =
                    static_cast<std::uint8_t>(
                        red * a /
                        255);

                pixels[
                    destinationIndex +
                    3] =
                    alpha;
            }
        }

        const D2D1_BITMAP_PROPERTIES properties =
            D2D1::BitmapProperties(
                D2D1::PixelFormat(
                    DXGI_FORMAT_B8G8R8A8_UNORM,
                    D2D1_ALPHA_MODE_PREMULTIPLIED));

        const HRESULT result =
            target->CreateBitmap(
                D2D1::SizeU(
                    width,
                    height),
                pixels.data(),
                static_cast<UINT32>(
                    width) *
                    4,
                properties,
                bitmap.GetAddressOf());

        if (FAILED(result))
        {
            error =
                "Unable to create TGA bitmap.";

            return false;
        }

        return true;
    }

    void DrawText(
        ID2D1HwndRenderTarget* target,
        IDWriteTextFormat* format,
        ID2D1SolidColorBrush* brush,
        const std::wstring& text,
        const D2D1_RECT_F& rect)
    {
        if (text.empty())
        {
            return;
        }

        target->DrawTextW(
            text.c_str(),
            static_cast<UINT32>(
                text.size()),
            format,
            rect,
            brush,
            D2D1_DRAW_TEXT_OPTIONS_CLIP,
            DWRITE_MEASURING_MODE_NATURAL);
    }
}

namespace client::ui
{
    struct FrontendRenderer::State final
    {
        ComPtr<ID2D1Factory>
            d2dFactory;

        ComPtr<ID2D1HwndRenderTarget>
            renderTarget;

        ComPtr<IDWriteFactory>
            writeFactory;

        ComPtr<IWICImagingFactory>
            imagingFactory;

        ComPtr<ID2D1SolidColorBrush>
            brush;

        ComPtr<ID2D1Bitmap>
            loginBackground;

        ComPtr<ID2D1Bitmap>
            startupLogo;

        ComPtr<IDWriteTextFormat>
            headerFormat;

        ComPtr<IDWriteTextFormat>
            labelFormat;

        ComPtr<IDWriteTextFormat>
            inputFormat;

        ComPtr<IDWriteTextFormat>
            smallFormat;

        ComPtr<IDWriteTextFormat>
            buttonFormat;

        ComPtr<IDWriteTextFormat>
            versionFormat;

        ComPtr<IDWriteTextFormat>
            symbolFormat;

        std::uint32_t width =
            0;

        std::uint32_t height =
            0;

        bool comOwned =
            false;
    };

    FrontendRenderer::FrontendRenderer()
        :
        state_(
            std::make_unique<State>())
    {
    }

    FrontendRenderer::~FrontendRenderer()
    {
        Shutdown();
    }

    bool FrontendRenderer::Initialize(
        const HWND window,
        const std::uint32_t width,
        const std::uint32_t height,
        const core::resources::ResourceFileSystem& resources,
        std::string& error)
    {
        Shutdown();

        error.clear();

        state_ =
            std::make_unique<State>();

        HRESULT result =
            CoInitializeEx(
                nullptr,
                COINIT_APARTMENTTHREADED);

        if (SUCCEEDED(result))
        {
            state_->comOwned =
                true;
        }
        else if (
            result !=
            RPC_E_CHANGED_MODE)
        {
            error =
                "COM initialization failed.";

            return false;
        }

        result =
            D2D1CreateFactory(
                D2D1_FACTORY_TYPE_SINGLE_THREADED,
                state_->
                    d2dFactory.
                    GetAddressOf());

        if (FAILED(result))
        {
            error =
                "Direct2D initialization failed.";

            return false;
        }

        result =
            DWriteCreateFactory(
                DWRITE_FACTORY_TYPE_SHARED,
                __uuidof(
                    IDWriteFactory),
                reinterpret_cast<
                    IUnknown**>(
                        state_->
                            writeFactory.
                            GetAddressOf()));

        if (FAILED(result))
        {
            error =
                "DirectWrite initialization failed.";

            return false;
        }

        result =
            CoCreateInstance(
                CLSID_WICImagingFactory,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(
                    state_->
                        imagingFactory.
                        GetAddressOf()));

        if (FAILED(result))
        {
            error =
                "WIC initialization failed.";

            return false;
        }

        result =
            state_->
                d2dFactory->
                CreateHwndRenderTarget(
                    D2D1::RenderTargetProperties(),
                    D2D1::HwndRenderTargetProperties(
                        window,
                        D2D1::SizeU(
                            width,
                            height),
                        D2D1_PRESENT_OPTIONS_IMMEDIATELY),
                    state_->
                        renderTarget.
                        GetAddressOf());

        if (FAILED(result))
        {
            error =
                "Frontend render target initialization failed.";

            return false;
        }

        result =
            state_->
                renderTarget->
                CreateSolidColorBrush(
                    D2D1::ColorF(
                        D2D1::ColorF::White),
                    state_->
                        brush.
                        GetAddressOf());

        if (FAILED(result))
        {
            error =
                "Frontend brush initialization failed.";

            return false;
        }

        auto createFormat =
            [&](const wchar_t* family,
                const float size,
                const DWRITE_FONT_WEIGHT weight,
                ComPtr<IDWriteTextFormat>& output)
            {
                return
                    state_->
                        writeFactory->
                        CreateTextFormat(
                            family,
                            nullptr,
                            weight,
                            DWRITE_FONT_STYLE_NORMAL,
                            DWRITE_FONT_STRETCH_NORMAL,
                            size,
                            L"ru-RU",
                            output.GetAddressOf());
            };

        if (FAILED(
                createFormat(
                    L"Arial Narrow",
                    18.0f,
                    DWRITE_FONT_WEIGHT_BOLD,
                    state_->headerFormat)) ||
            FAILED(
                createFormat(
                    L"Arial Narrow",
                    18.0f,
                    DWRITE_FONT_WEIGHT_BOLD,
                    state_->labelFormat)) ||
            FAILED(
                createFormat(
                    L"Arial Narrow",
                    21.0f,
                    DWRITE_FONT_WEIGHT_NORMAL,
                    state_->inputFormat)) ||
            FAILED(
                createFormat(
                    L"Arial Narrow",
                    16.0f,
                    DWRITE_FONT_WEIGHT_NORMAL,
                    state_->smallFormat)) ||
            FAILED(
                createFormat(
                    L"Arial Narrow",
                    21.0f,
                    DWRITE_FONT_WEIGHT_BOLD,
                    state_->buttonFormat)) ||
            FAILED(
                createFormat(
                    L"Arial Narrow",
                    12.0f,
                    DWRITE_FONT_WEIGHT_NORMAL,
                    state_->versionFormat)) ||
            FAILED(
                createFormat(
                    L"Segoe UI Symbol",
                    27.0f,
                    DWRITE_FONT_WEIGHT_NORMAL,
                    state_->symbolFormat)))
        {
            error =
                "Frontend font initialization failed.";

            return false;
        }

        state_->width =
            width;

        state_->height =
            height;

        const core::resources::ResourceEntry*
            background =
                resources.Find(
                    LoginBackgroundResource);

        if (background ==
            nullptr)
        {
            error =
                "Original login background was not found.";

            return false;
        }

        if (!LoadWicBitmap(
                state_->
                    imagingFactory.
                    Get(),
                state_->
                    renderTarget.
                    Get(),
                background->
                    physicalPath,
                state_->
                    loginBackground,
                error))
        {
            return false;
        }

        if (!LoadTgaBitmap(
                resources,
                StartupLogoResource,
                state_->
                    renderTarget.
                    Get(),
                state_->
                    startupLogo,
                error))
        {
            return false;
        }

        return true;
    }

    bool FrontendRenderer::RenderStartupSplash(
        std::string& error)
    {
        error.clear();

        ID2D1HwndRenderTarget* target =
            state_->
                renderTarget.
                Get();

        target->BeginDraw();

        target->SetTransform(
            D2D1::Matrix3x2F::Identity());

        target->Clear(
            D2D1::ColorF(
                D2D1::ColorF::Black));

        const float scaleX =
            static_cast<float>(
                state_->width) /
            frontend::layout::
                ReferenceWidth;

        const float scaleY =
            static_cast<float>(
                state_->height) /
            frontend::layout::
                ReferenceHeight;

        target->SetTransform(
            D2D1::Matrix3x2F::Scale(
                scaleX,
                scaleY));

        if (state_->
            startupLogo)
        {
            target->DrawBitmap(
                state_->
                    startupLogo.
                    Get(),
                D2D1::RectF(
                    340.0f,
                    248.0f,
                    660.0f,
                    568.0f),
                1.0f,
                D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
        }

        const HRESULT result =
            target->EndDraw();

        if (FAILED(result))
        {
            error =
                "Startup splash rendering failed.";

            return false;
        }

        return true;
    }

    bool FrontendRenderer::RenderLogin(
        const LoginView& view,
        std::string& error)
    {
        error.clear();

        ID2D1HwndRenderTarget* target =
            state_->
                renderTarget.
                Get();

        ID2D1SolidColorBrush* brush =
            state_->
                brush.
                Get();

        target->BeginDraw();

        target->SetTransform(
            D2D1::Matrix3x2F::Identity());

        target->Clear(
            D2D1::ColorF(
                D2D1::ColorF::Black));

        const float scaleX =
            static_cast<float>(
                state_->width) /
            frontend::layout::
                ReferenceWidth;

        const float scaleY =
            static_cast<float>(
                state_->height) /
            frontend::layout::
                ReferenceHeight;

        target->SetTransform(
            D2D1::Matrix3x2F::Scale(
                scaleX,
                scaleY));

        target->DrawBitmap(
            state_->
                loginBackground.
                Get(),
            D2D1::RectF(
                0.0f,
                0.0f,
                1280.0f,
                768.0f),
            1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);

        brush->SetColor(
            D2D1::ColorF(
                0.0f,
                0.0f,
                0.0f,
                0.79f));

        target->FillRectangle(
            D2D1::RectF(
                407.0f,
                234.0f,
                868.0f,
                524.0f),
            brush);

        brush->SetColor(
            D2D1::ColorF(
                0.0f,
                0.0f,
                0.0f,
                0.90f));

        target->FillRectangle(
            D2D1::RectF(
                407.0f,
                524.0f,
                868.0f,
                585.0f),
            brush);

        brush->SetColor(
            D2D1::ColorF(
                0.20f,
                0.20f,
                0.20f,
                1.0f));

        target->DrawLine(
            D2D1::Point2F(
                407.0f,
                286.0f),
            D2D1::Point2F(
                868.0f,
                286.0f),
            brush,
            1.0f);

        const bool russian =
            view.language ==
            FrontendLanguage::Russian;

        brush->SetColor(
            D2D1::ColorF(
                0.92f,
                0.92f,
                0.92f,
                1.0f));

        DrawText(
            target,
            state_->
                headerFormat.
                Get(),
            brush,
            russian
                ? L"ВОЙДИТЕ"
                : L"SIGN IN",
            D2D1::RectF(
                429.0f,
                250.0f,
                575.0f,
                278.0f));

        brush->SetColor(
            D2D1::ColorF(
                0.45f,
                0.45f,
                0.45f,
                1.0f));

        DrawText(
            target,
            state_->
                smallFormat.
                Get(),
            brush,
            russian
                ? L"или"
                : L"or",
            D2D1::RectF(
                577.0f,
                251.0f,
                610.0f,
                278.0f));

        brush->SetColor(
            D2D1::ColorF(
                0.02f,
                0.67f,
                0.86f,
                1.0f));

        DrawText(
            target,
            state_->
                smallFormat.
                Get(),
            brush,
            russian
                ? L"Зарегистрируйтесь"
                : L"Register",
            D2D1::RectF(
                612.0f,
                251.0f,
                790.0f,
                278.0f));

        brush->SetColor(
            D2D1::ColorF(
                0.94f,
                0.94f,
                0.94f,
                1.0f));

        DrawText(
            target,
            state_->
                labelFormat.
                Get(),
            brush,
            russian
                ? L"сервер:"
                : L"server:",
            D2D1::RectF(
                429.0f,
                318.0f,
                530.0f,
                350.0f));

        DrawText(
            target,
            state_->
                labelFormat.
                Get(),
            brush,
            russian
                ? L"имя:"
                : L"name:",
            D2D1::RectF(
                429.0f,
                389.0f,
                530.0f,
                420.0f));

        DrawText(
            target,
            state_->
                labelFormat.
                Get(),
            brush,
            russian
                ? L"пароль:"
                : L"password:",
            D2D1::RectF(
                429.0f,
                439.0f,
                530.0f,
                470.0f));

        const D2D1_COLOR_F fieldColor =
            D2D1::ColorF(
                0.80f,
                0.80f,
                0.80f,
                1.0f);

        brush->SetColor(
            fieldColor);

        target->FillRectangle(
            D2D1::RectF(
                547.0f,
                310.0f,
                807.0f,
                352.0f),
            brush);

        target->FillRectangle(
            D2D1::RectF(
                547.0f,
                381.0f,
                847.0f,
                422.0f),
            brush);

        target->FillRectangle(
            D2D1::RectF(
                547.0f,
                431.0f,
                847.0f,
                472.0f),
            brush);

        brush->SetColor(
            D2D1::ColorF(
                0.63f,
                0.63f,
                0.63f,
                1.0f));

        target->FillRectangle(
            D2D1::RectF(
                807.0f,
                310.0f,
                847.0f,
                352.0f),
            brush);

        brush->SetColor(
            D2D1::ColorF(
                0.12f,
                0.12f,
                0.12f,
                1.0f));

        DrawText(
            target,
            state_->
                inputFormat.
                Get(),
            brush,
            view.serverName,
            D2D1::RectF(
                558.0f,
                317.0f,
                800.0f,
                348.0f));

        DrawText(
            target,
            state_->
                inputFormat.
                Get(),
            brush,
            L"+",
            D2D1::RectF(
                820.0f,
                315.0f,
                842.0f,
                348.0f));

        DrawText(
            target,
            state_->
                inputFormat.
                Get(),
            brush,
            view.login,
            D2D1::RectF(
                558.0f,
                388.0f,
                838.0f,
                418.0f));

        std::wstring password;

        password.assign(
            view.password.size(),
            L'\x2022');

        DrawText(
            target,
            state_->
                inputFormat.
                Get(),
            brush,
            password,
            D2D1::RectF(
                558.0f,
                438.0f,
                838.0f,
                468.0f));

        if (view.focus ==
            LoginFocus::Login)
        {
            brush->SetColor(
                D2D1::ColorF(
                    0.02f,
                    0.67f,
                    0.86f,
                    1.0f));

            target->DrawRectangle(
                D2D1::RectF(
                    546.0f,
                    380.0f,
                    848.0f,
                    423.0f),
                brush,
                1.0f);
        }

        if (view.focus ==
            LoginFocus::Password)
        {
            brush->SetColor(
                D2D1::ColorF(
                    0.02f,
                    0.67f,
                    0.86f,
                    1.0f));

            target->DrawRectangle(
                D2D1::RectF(
                    546.0f,
                    430.0f,
                    848.0f,
                    473.0f),
                brush,
                1.0f);
        }

        brush->SetColor(
            D2D1::ColorF(
                0.92f,
                0.92f,
                0.92f,
                1.0f));

        DrawText(
            target,
            state_->
                labelFormat.
                Get(),
            brush,
            russian
                ? L"запомнить меня:"
                : L"remember me:",
            D2D1::RectF(
                656.0f,
                488.0f,
                794.0f,
                518.0f));

        brush->SetColor(
            view.rememberLogin
                ? D2D1::ColorF(
                    0.05f,
                    0.39f,
                    0.09f,
                    1.0f)
                : D2D1::ColorF(
                    0.20f,
                    0.20f,
                    0.20f,
                    1.0f));

        target->FillRoundedRectangle(
            D2D1::RoundedRect(
                D2D1::RectF(
                    798.0f,
                    489.0f,
                    848.0f,
                    516.0f),
                13.0f,
                13.0f),
            brush);

        brush->SetColor(
            D2D1::ColorF(
                0.92f,
                0.92f,
                0.92f,
                1.0f));

        const float knobLeft =
            view.rememberLogin
                ? 825.0f
                : 801.0f;

        target->FillEllipse(
            D2D1::Ellipse(
                D2D1::Point2F(
                    knobLeft +
                        10.0f,
                    502.5f),
                10.0f,
                10.0f),
            brush);

        brush->SetColor(
            view.canLogin
                ? D2D1::ColorF(
                    0.74f,
                    0.74f,
                    0.74f,
                    1.0f)
                : D2D1::ColorF(
                    0.26f,
                    0.26f,
                    0.26f,
                    1.0f));

        DrawText(
            target,
            state_->
                buttonFormat.
                Get(),
            brush,
            view.authenticating
                ? (
                    russian
                        ? L"ПОДКЛЮЧЕНИЕ..."
                        : L"CONNECTING..."
                  )
                : (
                    russian
                        ? L"ВОЙТИ"
                        : L"LOGIN"
                  ),
            D2D1::RectF(
                607.0f,
                542.0f,
                720.0f,
                575.0f));

        brush->SetColor(
            D2D1::ColorF(
                0.72f,
                0.72f,
                0.72f,
                1.0f));

        DrawText(
            target,
            state_->
                symbolFormat.
                Get(),
            brush,
            L"\x2699",
            D2D1::RectF(
                1084.0f,
                7.0f,
                1120.0f,
                43.0f));

        brush->SetColor(
            view.language ==
                    FrontendLanguage::English
                ? D2D1::ColorF(
                    0.02f,
                    0.67f,
                    0.86f,
                    1.0f)
                : D2D1::ColorF(
                    0.55f,
                    0.55f,
                    0.55f,
                    1.0f));

        DrawText(
            target,
            state_->
                inputFormat.
                Get(),
            brush,
            L"EN",
            D2D1::RectF(
                1138.0f,
                14.0f,
                1168.0f,
                42.0f));

        brush->SetColor(
            D2D1::ColorF(
                0.52f,
                0.52f,
                0.52f,
                1.0f));

        DrawText(
            target,
            state_->
                inputFormat.
                Get(),
            brush,
            L"|",
            D2D1::RectF(
                1168.0f,
                14.0f,
                1178.0f,
                42.0f));

        brush->SetColor(
            view.language ==
                    FrontendLanguage::Russian
                ? D2D1::ColorF(
                    0.02f,
                    0.67f,
                    0.86f,
                    1.0f)
                : D2D1::ColorF(
                    0.55f,
                    0.55f,
                    0.55f,
                    1.0f));

        DrawText(
            target,
            state_->
                inputFormat.
                Get(),
            brush,
            L"RU",
            D2D1::RectF(
                1179.0f,
                14.0f,
                1210.0f,
                42.0f));

        brush->SetColor(
            D2D1::ColorF(
                0.82f,
                0.82f,
                0.82f,
                1.0f));

        DrawText(
            target,
            state_->
                inputFormat.
                Get(),
            brush,
            L"X",
            D2D1::RectF(
                1240.0f,
                14.0f,
                1260.0f,
                42.0f));

        brush->SetColor(
            D2D1::ColorF(
                0.50f,
                0.50f,
                0.50f,
                1.0f));

        DrawText(
            target,
            state_->
                versionFormat.
                Get(),
            brush,
            view.version,
            D2D1::RectF(
                1183.0f,
                735.0f,
                1267.0f,
                755.0f));

        if (!view.message.empty())
        {
            brush->SetColor(
                D2D1::ColorF(
                    0.90f,
                    0.52f,
                    0.25f,
                    1.0f));

            DrawText(
                target,
                state_->
                    smallFormat.
                    Get(),
                brush,
                view.message,
                D2D1::RectF(
                    407.0f,
                    594.0f,
                    868.0f,
                    626.0f));
        }

        if (view.serverListOpen)
        {
            brush->SetColor(
                D2D1::ColorF(
                    0.0f,
                    0.0f,
                    0.0f,
                    0.92f));

            const float popupBottom =
                310.0f +
                static_cast<float>(
                    view.servers.size()) *
                36.0f;

            target->FillRectangle(
                D2D1::RectF(
                    875.0f,
                    310.0f,
                    1120.0f,
                    popupBottom),
                brush);

            for (std::size_t index = 0;
                 index < view.servers.size();
                 ++index)
            {
                const ui::Rect item =
                    frontend::layout::
                        ServerListItem(
                            index);

                if (index ==
                    view.selectedServer)
                {
                    brush->SetColor(
                        D2D1::ColorF(
                            0.12f,
                            0.12f,
                            0.12f,
                            1.0f));

                    target->FillRectangle(
                        D2D1::RectF(
                            item.left,
                            item.top,
                            item.right,
                            item.bottom),
                        brush);
                }

                brush->SetColor(
                    D2D1::ColorF(
                        0.86f,
                        0.86f,
                        0.86f,
                        1.0f));

                DrawText(
                    target,
                    state_->
                        smallFormat.
                        Get(),
                    brush,
                    view.servers[
                        index],
                    D2D1::RectF(
                        item.left +
                            10.0f,
                        item.top +
                            7.0f,
                        item.right -
                            5.0f,
                        item.bottom));
            }
        }

        const HRESULT result =
            target->EndDraw();

        if (FAILED(result))
        {
            error =
                "Login screen rendering failed.";

            return false;
        }

        return true;
    }

    bool FrontendRenderer::RenderBackground(
        std::string& error)
    {
        error.clear();

        ID2D1HwndRenderTarget* target =
            state_->
                renderTarget.
                Get();

        target->BeginDraw();

        target->SetTransform(
            D2D1::Matrix3x2F::Identity());

        target->Clear(
            D2D1::ColorF(
                D2D1::ColorF::Black));

        target->SetTransform(
            D2D1::Matrix3x2F::Scale(
                static_cast<float>(
                    state_->width) /
                    1280.0f,
                static_cast<float>(
                    state_->height) /
                    768.0f));

        target->DrawBitmap(
            state_->
                loginBackground.
                Get(),
            D2D1::RectF(
                0.0f,
                0.0f,
                1280.0f,
                768.0f));

        const HRESULT result =
            target->EndDraw();

        if (FAILED(result))
        {
            error =
                "Frontend background rendering failed.";

            return false;
        }

        return true;
    }

    void FrontendRenderer::Shutdown()
    {
        if (!state_)
        {
            return;
        }

        const bool comOwned =
            state_->
                comOwned;

        state_.reset();

        state_ =
            std::make_unique<State>();

        if (comOwned)
        {
            CoUninitialize();
        }
    }
}