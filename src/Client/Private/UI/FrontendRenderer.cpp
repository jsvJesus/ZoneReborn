#include "UI/FrontendRenderer.h"

#include "Frontend/LoginLayout.h"

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <filesystem>
#include <string>

namespace
{
    using Microsoft::WRL::ComPtr;

    constexpr char LoginBackground[] =
        "res/soGUI/maps/Login/login_bg.jpg";

    D2D1_RECT_F ToD2DRect(
        const client::ui::Rect& rect)
    {
        return D2D1::RectF(
            rect.left,
            rect.top,
            rect.right,
            rect.bottom);
    }

    bool LoadBitmap(
        IWICImagingFactory* imagingFactory,
        ID2D1HwndRenderTarget* renderTarget,
        const std::filesystem::path& path,
        ComPtr<ID2D1Bitmap>& result,
        std::string& error)
    {
        result.Reset();

        ComPtr<IWICBitmapDecoder>
            decoder;

        HRESULT hr =
            imagingFactory->
                CreateDecoderFromFilename(
                    path.c_str(),
                    nullptr,
                    GENERIC_READ,
                    WICDecodeMetadataCacheOnLoad,
                    decoder.GetAddressOf());

        if (FAILED(hr))
        {
            error =
                "Unable to open frontend bitmap: " +
                path.string();

            return false;
        }

        ComPtr<IWICBitmapFrameDecode>
            frame;

        hr =
            decoder->
                GetFrame(
                    0,
                    frame.GetAddressOf());

        if (FAILED(hr))
        {
            error =
                "Unable to decode frontend bitmap.";

            return false;
        }

        ComPtr<IWICFormatConverter>
            converter;

        hr =
            imagingFactory->
                CreateFormatConverter(
                    converter.GetAddressOf());

        if (FAILED(hr))
        {
            error =
                "Unable to create bitmap converter.";

            return false;
        }

        hr =
            converter->
                Initialize(
                    frame.Get(),
                    GUID_WICPixelFormat32bppPBGRA,
                    WICBitmapDitherTypeNone,
                    nullptr,
                    0.0,
                    WICBitmapPaletteTypeMedianCut);

        if (FAILED(hr))
        {
            error =
                "Unable to convert frontend bitmap.";

            return false;
        }

        hr =
            renderTarget->
                CreateBitmapFromWicBitmap(
                    converter.Get(),
                    nullptr,
                    result.GetAddressOf());

        if (FAILED(hr))
        {
            error =
                "Unable to create frontend bitmap.";

            return false;
        }

        return true;
    }

    void DrawTextValue(
        ID2D1HwndRenderTarget* target,
        IDWriteTextFormat* format,
        ID2D1SolidColorBrush* brush,
        const std::wstring& text,
        const D2D1_RECT_F& rectangle)
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
            rectangle,
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

        ComPtr<IDWriteTextFormat>
            titleFormat;

        ComPtr<IDWriteTextFormat>
            normalFormat;

        ComPtr<IDWriteTextFormat>
            smallFormat;

        ComPtr<IWICImagingFactory>
            imagingFactory;

        ComPtr<ID2D1SolidColorBrush>
            brush;

        ComPtr<ID2D1Bitmap>
            loginBackground;

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
        const core::resources::ResourceFileSystem&
            resources,
        std::string& error)
    {
        Shutdown();

        error.clear();

        state_ =
            std::make_unique<State>();

        HRESULT hr =
            CoInitializeEx(
                nullptr,
                COINIT_APARTMENTTHREADED);

        if (SUCCEEDED(hr))
        {
            state_->comOwned =
                true;
        }
        else if (hr !=
                 RPC_E_CHANGED_MODE)
        {
            error =
                "COM initialization failed.";

            return false;
        }

        hr =
            D2D1CreateFactory(
                D2D1_FACTORY_TYPE_SINGLE_THREADED,
                state_->
                    d2dFactory.
                    GetAddressOf());

        if (FAILED(hr))
        {
            error =
                "Direct2D factory initialization failed.";

            return false;
        }

        hr =
            DWriteCreateFactory(
                DWRITE_FACTORY_TYPE_SHARED,
                __uuidof(
                    IDWriteFactory),
                reinterpret_cast<IUnknown**>(
                    state_->
                        writeFactory.
                        GetAddressOf()));

        if (FAILED(hr))
        {
            error =
                "DirectWrite initialization failed.";

            return false;
        }

        hr =
            CoCreateInstance(
                CLSID_WICImagingFactory,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(
                    state_->
                        imagingFactory.
                        GetAddressOf()));

        if (FAILED(hr))
        {
            error =
                "WIC initialization failed.";

            return false;
        }

        hr =
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

        if (FAILED(hr))
        {
            error =
                "Frontend render target initialization failed.";

            return false;
        }

        hr =
            state_->
                renderTarget->
                CreateSolidColorBrush(
                    D2D1::ColorF(
                        D2D1::ColorF::White),
                    state_->
                        brush.
                        GetAddressOf());

        if (FAILED(hr))
        {
            error =
                "Frontend brush initialization failed.";

            return false;
        }

        hr =
            state_->
                writeFactory->
                CreateTextFormat(
                    L"Segoe UI",
                    nullptr,
                    DWRITE_FONT_WEIGHT_NORMAL,
                    DWRITE_FONT_STYLE_NORMAL,
                    DWRITE_FONT_STRETCH_NORMAL,
                    32.0f,
                    L"ru-RU",
                    state_->
                        titleFormat.
                        GetAddressOf());

        if (FAILED(hr))
        {
            error =
                "Frontend title font initialization failed.";

            return false;
        }

        hr =
            state_->
                writeFactory->
                CreateTextFormat(
                    L"Segoe UI",
                    nullptr,
                    DWRITE_FONT_WEIGHT_NORMAL,
                    DWRITE_FONT_STYLE_NORMAL,
                    DWRITE_FONT_STRETCH_NORMAL,
                    20.0f,
                    L"ru-RU",
                    state_->
                        normalFormat.
                        GetAddressOf());

        if (FAILED(hr))
        {
            error =
                "Frontend font initialization failed.";

            return false;
        }

        hr =
            state_->
                writeFactory->
                CreateTextFormat(
                    L"Segoe UI",
                    nullptr,
                    DWRITE_FONT_WEIGHT_NORMAL,
                    DWRITE_FONT_STYLE_NORMAL,
                    DWRITE_FONT_STRETCH_NORMAL,
                    16.0f,
                    L"ru-RU",
                    state_->
                        smallFormat.
                        GetAddressOf());

        if (FAILED(hr))
        {
            error =
                "Frontend small font initialization failed.";

            return false;
        }

        state_->
            titleFormat->
            SetTextAlignment(
                DWRITE_TEXT_ALIGNMENT_CENTER);

        state_->
            normalFormat->
            SetParagraphAlignment(
                DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        state_->width =
            width;

        state_->height =
            height;

        const core::resources::ResourceEntry*
            backgroundEntry =
                resources.Find(
                    LoginBackground);

        if (backgroundEntry ==
            nullptr)
        {
            error =
                "Original login background not found: " +
                std::string(
                    LoginBackground);

            return false;
        }

        if (!LoadBitmap(
                state_->
                    imagingFactory.
                    Get(),
                state_->
                    renderTarget.
                    Get(),
                backgroundEntry->
                    physicalPath,
                state_->
                    loginBackground,
                error))
        {
            return false;
        }

        return true;
    }

    bool FrontendRenderer::RenderLogin(
        const LoginView& view,
        std::string& error)
    {
        error.clear();

        if (!state_ ||
            !state_->renderTarget)
        {
            error =
                "Frontend renderer is not initialized.";

            return false;
        }

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

        if (state_->
            loginBackground)
        {
            target->DrawBitmap(
                state_->
                    loginBackground.
                    Get(),
                D2D1::RectF(
                    0.0f,
                    0.0f,
                    static_cast<float>(
                        state_->width),
                    static_cast<float>(
                        state_->height)),
                1.0f,
                D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
        }

        brush->SetColor(
            D2D1::ColorF(
                0.0f,
                0.0f,
                0.0f,
                0.68f));

        target->FillRoundedRectangle(
            D2D1::RoundedRect(
                D2D1::RectF(
                    560.0f,
                    220.0f,
                    1040.0f,
                    735.0f),
                6.0f,
                6.0f),
            brush);

        brush->SetColor(
            D2D1::ColorF(
                0.78f,
                0.78f,
                0.72f,
                1.0f));

        DrawTextValue(
            target,
            state_->
                titleFormat.
                Get(),
            brush,
            L"ВХОД",
            D2D1::RectF(
                600.0f,
                245.0f,
                1000.0f,
                290.0f));

        const auto drawEdit =
            [&](const Rect& rect,
                const bool focused,
                const std::wstring& text)
            {
                brush->SetColor(
                    focused
                        ? D2D1::ColorF(
                            0.20f,
                            0.20f,
                            0.18f,
                            0.94f)
                        : D2D1::ColorF(
                            0.08f,
                            0.08f,
                            0.07f,
                            0.90f));

                target->FillRectangle(
                    ToD2DRect(
                        rect),
                    brush);

                brush->SetColor(
                    focused
                        ? D2D1::ColorF(
                            0.80f,
                            0.70f,
                            0.35f,
                            1.0f)
                        : D2D1::ColorF(
                            0.35f,
                            0.35f,
                            0.32f,
                            1.0f));

                target->DrawRectangle(
                    ToD2DRect(
                        rect),
                    brush,
                    focused
                        ? 2.0f
                        : 1.0f);

                brush->SetColor(
                    D2D1::ColorF(
                        0.90f,
                        0.90f,
                        0.86f,
                        1.0f));

                DrawTextValue(
                    target,
                    state_->
                        normalFormat.
                        Get(),
                    brush,
                    text,
                    D2D1::RectF(
                        rect.left +
                            12.0f,
                        rect.top +
                            10.0f,
                        rect.right -
                            12.0f,
                        rect.bottom));
            };

        drawEdit(
            frontend::layout::
                LoginEdit,
            view.focus ==
                LoginFocus::Login,
            view.login.empty()
                ? L"Логин"
                : view.login);

        std::wstring passwordText;

        if (view.password.empty())
        {
            passwordText =
                L"Пароль";
        }
        else
        {
            passwordText.assign(
                view.password.size(),
                L'\x2022');
        }

        drawEdit(
            frontend::layout::
                PasswordEdit,
            view.focus ==
                LoginFocus::Password,
            passwordText);

        brush->SetColor(
            D2D1::ColorF(
                0.08f,
                0.08f,
                0.07f,
                0.95f));

        target->FillRectangle(
            D2D1::RectF(
                620.0f,
                463.0f,
                640.0f,
                483.0f),
            brush);

        brush->SetColor(
            D2D1::ColorF(
                0.65f,
                0.60f,
                0.42f,
                1.0f));

        target->DrawRectangle(
            D2D1::RectF(
                620.0f,
                463.0f,
                640.0f,
                483.0f),
            brush);

        if (view.rememberLogin)
        {
            brush->SetColor(
                D2D1::ColorF(
                    0.83f,
                    0.72f,
                    0.32f,
                    1.0f));

            target->FillRectangle(
                D2D1::RectF(
                    625.0f,
                    468.0f,
                    635.0f,
                    478.0f),
                brush);
        }

        brush->SetColor(
            D2D1::ColorF(
                0.82f,
                0.82f,
                0.78f,
                1.0f));

        DrawTextValue(
            target,
            state_->
                smallFormat.
                Get(),
            brush,
            L"Запомнить логин",
            D2D1::RectF(
                650.0f,
                460.0f,
                900.0f,
                490.0f));

        brush->SetColor(
            D2D1::ColorF(
                0.18f,
                0.16f,
                0.10f,
                0.96f));

        target->FillRectangle(
            ToD2DRect(
                frontend::layout::
                    LoginButton),
            brush);

        brush->SetColor(
            D2D1::ColorF(
                0.72f,
                0.62f,
                0.30f,
                1.0f));

        target->DrawRectangle(
            ToD2DRect(
                frontend::layout::
                    LoginButton),
            brush,
            1.5f);

        brush->SetColor(
            D2D1::ColorF(
                0.95f,
                0.92f,
                0.80f,
                1.0f));

        DrawTextValue(
            target,
            state_->
                normalFormat.
                Get(),
            brush,
            view.authenticating
                ? L"ПОДКЛЮЧЕНИЕ..."
                : L"ВОЙТИ",
            D2D1::RectF(
                620.0f,
                535.0f,
                980.0f,
                565.0f));

        brush->SetColor(
            D2D1::ColorF(
                0.72f,
                0.72f,
                0.68f,
                1.0f));

        DrawTextValue(
            target,
            state_->
                smallFormat.
                Get(),
            brush,
            L"Создать аккаунт",
            D2D1::RectF(
                620.0f,
                610.0f,
                980.0f,
                642.0f));

        DrawTextValue(
            target,
            state_->
                smallFormat.
                Get(),
            brush,
            L"Восстановить аккаунт",
            D2D1::RectF(
                620.0f,
                655.0f,
                980.0f,
                687.0f));

        if (!view.message.empty())
        {
            brush->SetColor(
                D2D1::ColorF(
                    0.92f,
                    0.65f,
                    0.34f,
                    1.0f));

            DrawTextValue(
                target,
                state_->
                    smallFormat.
                    Get(),
                brush,
                view.message,
                D2D1::RectF(
                    580.0f,
                    695.0f,
                    1020.0f,
                    730.0f));
        }

        const HRESULT result =
            target->EndDraw();

        if (FAILED(result))
        {
            error =
                "Frontend login rendering failed.";

            return false;
        }

        return true;
    }

    bool FrontendRenderer::RenderMainMenuCheckpoint(
        const std::string& login,
        std::string& error)
    {
        error.clear();

        if (!state_ ||
            !state_->renderTarget)
        {
            error =
                "Frontend renderer is not initialized.";

            return false;
        }

        ID2D1HwndRenderTarget* target =
            state_->
                renderTarget.
                Get();

        ID2D1SolidColorBrush* brush =
            state_->
                brush.
                Get();

        target->BeginDraw();

        target->Clear(
            D2D1::ColorF(
                D2D1::ColorF::Black));

        if (state_->
            loginBackground)
        {
            target->DrawBitmap(
                state_->
                    loginBackground.
                    Get(),
                D2D1::RectF(
                    0.0f,
                    0.0f,
                    static_cast<float>(
                        state_->width),
                    static_cast<float>(
                        state_->height)),
                1.0f,
                D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
        }

        brush->SetColor(
            D2D1::ColorF(
                0.0f,
                0.0f,
                0.0f,
                0.72f));

        target->FillRectangle(
            D2D1::RectF(
                450.0f,
                300.0f,
                1150.0f,
                600.0f),
            brush);

        brush->SetColor(
            D2D1::ColorF(
                0.85f,
                0.80f,
                0.60f,
                1.0f));

        std::wstring text =
            L"Авторизация успешна";

        DrawTextValue(
            target,
            state_->
                titleFormat.
                Get(),
            brush,
            text,
            D2D1::RectF(
                500.0f,
                350.0f,
                1100.0f,
                410.0f));

        std::wstring loginText =
            L"Аккаунт: ";

        loginText.append(
            login.begin(),
            login.end());

        DrawTextValue(
            target,
            state_->
                normalFormat.
                Get(),
            brush,
            loginText,
            D2D1::RectF(
                500.0f,
                440.0f,
                1100.0f,
                480.0f));

        brush->SetColor(
            D2D1::ColorF(
                0.70f,
                0.70f,
                0.68f,
                1.0f));

        DrawTextValue(
            target,
            state_->
                smallFormat.
                Get(),
            brush,
            L"Следующий экран: оригинальное главное меню",
            D2D1::RectF(
                500.0f,
                510.0f,
                1100.0f,
                550.0f));

        const HRESULT result =
            target->EndDraw();

        if (FAILED(result))
        {
            error =
                "Frontend rendering failed.";

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