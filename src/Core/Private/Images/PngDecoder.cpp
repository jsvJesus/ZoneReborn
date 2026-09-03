#include "Core/Images/PngDecoder.h"

#include "Core/Images/WicImageDecoder.h"

namespace core::images
{
    bool PngDecoder::Decode(
        const std::span<const std::byte> data,
        RgbaImage& output,
        std::string& error) const
    {
        WicImageDecoder decoder;

        return decoder.Decode(
            data,
            output,
            error);
    }
}