#include "Core/Images/PngDecoder.h"

#include <Windows.h>
#include <objbase.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <cstdint>
#include <limits>
#include <utility>

namespace
{
    using Microsoft::WRL::ComPtr;

    class ComScope final
    {
    public:
        [[nodiscard]]
        bool Initialize(
            std::string& error)
        {
            const HRESULT result =
                CoInitializeEx(
                    nullptr,
                    COINIT_MULTITHREADED);

            if (result ==
                RPC_E_CHANGED_MODE)
            {
                return true;
            }

            if (FAILED(result))
            {
                error =
                    "Unable to initialize COM.";

                return false;
            }

            shouldUninitialize_ =
                true;

            return true;
        }

        ~ComScope()
        {
            if (shouldUninitialize_)
            {
                CoUninitialize();
            }
        }

    private:
        bool shouldUninitialize_ =
            false;
    };
}

namespace core::images
{
    bool PngDecoder::Decode(
        const std::span<const std::byte> data,
        RgbaImage& output,
        std::string& error) const
    {
        output = {};
        error.clear();

        if (data.empty())
        {
            error =
                "PNG data is empty.";

            return false;
        }

        if (data.size() >
            std::numeric_limits<DWORD>::max())
        {
            error =
                "PNG data is too large.";

            return false;
        }

        ComScope com;

        if (!com.Initialize(
                error))
        {
            return false;
        }

        ComPtr<IWICImagingFactory>
            factory;

        HRESULT result =
            CoCreateInstance(
                CLSID_WICImagingFactory,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(
                    &factory));

        if (FAILED(result))
        {
            error =
                "Unable to create WIC factory.";

            return false;
        }

        ComPtr<IWICStream>
            stream;

        result =
            factory->CreateStream(
                &stream);

        if (FAILED(result))
        {
            error =
                "Unable to create WIC stream.";

            return false;
        }

        result =
            stream->InitializeFromMemory(
                reinterpret_cast<BYTE*>(
                    const_cast<std::byte*>(
                        data.data())),
                static_cast<DWORD>(
                    data.size()));

        if (FAILED(result))
        {
            error =
                "Unable to initialize WIC stream.";

            return false;
        }

        ComPtr<IWICBitmapDecoder>
            decoder;

        result =
            factory->CreateDecoderFromStream(
                stream.Get(),
                nullptr,
                WICDecodeMetadataCacheOnLoad,
                &decoder);

        if (FAILED(result))
        {
            error =
                "Unable to create PNG decoder.";

            return false;
        }

        ComPtr<IWICBitmapFrameDecode>
            frame;

        result =
            decoder->GetFrame(
                0,
                &frame);

        if (FAILED(result))
        {
            error =
                "Unable to read PNG frame.";

            return false;
        }

        UINT width = 0;
        UINT height = 0;

        result =
            frame->GetSize(
                &width,
                &height);

        if (FAILED(result) ||
            width == 0 ||
            height == 0)
        {
            error =
                "PNG dimensions are invalid.";

            return false;
        }

        ComPtr<IWICFormatConverter>
            converter;

        result =
            factory->CreateFormatConverter(
                &converter);

        if (FAILED(result))
        {
            error =
                "Unable to create WIC format converter.";

            return false;
        }

        result =
            converter->Initialize(
                frame.Get(),
                GUID_WICPixelFormat32bppRGBA,
                WICBitmapDitherTypeNone,
                nullptr,
                0.0,
                WICBitmapPaletteTypeCustom);

        if (FAILED(result))
        {
            error =
                "Unable to convert PNG to RGBA.";

            return false;
        }

        const std::uint64_t stride =
            static_cast<std::uint64_t>(
                width) *
            4ull;

        const std::uint64_t imageSize =
            stride *
            static_cast<std::uint64_t>(
                height);

        if (stride >
                std::numeric_limits<UINT>::max() ||
            imageSize >
                std::numeric_limits<UINT>::max())
        {
            error =
                "Decoded PNG is too large.";

            return false;
        }

        RgbaImage image;

        image.width =
            width;

        image.height =
            height;

        image.pixels.resize(
            static_cast<std::size_t>(
                imageSize));

        result =
            converter->CopyPixels(
                nullptr,
                static_cast<UINT>(
                    stride),
                static_cast<UINT>(
                    imageSize),
                reinterpret_cast<BYTE*>(
                    image.pixels.data()));

        if (FAILED(result))
        {
            error =
                "Unable to copy PNG pixels.";

            return false;
        }

        output =
            std::move(image);

        return true;
    }
}