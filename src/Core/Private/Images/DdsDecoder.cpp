#include "Core/Images/DdsDecoder.h"

#include "Core/Images/WicImageDecoder.h"

namespace core::images
{
    bool DdsDecoder::Decode(
        const std::span<const std::byte> data,
        RgbaImage& output,
        std::string& error) const
    {
        WicImageDecoder decoder;

        if (!decoder.Decode(
                data,
                output,
                error))
        {
            error =
                "Unable to decode DDS: " +
                error;

            return false;
        }

        return true;
    }
}