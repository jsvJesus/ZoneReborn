#include "Core/Assets/TextureAnimationLoader.h"

#include "Core/Assets/TextureResolver.h"
#include "Core/Resources/DataSection.h"
#include "Core/Resources/PackedSectionReader.h"
#include "Core/Resources/ResourcePath.h"

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    std::string BuildResourcePath(
        const core::resources::ResourceFileSystem& resources,
        const std::string_view reference)
    {
        std::string normalized =
            core::resources::ResourcePath::Normalize(
                reference);

        if (normalized.empty())
        {
            return {};
        }

        std::filesystem::path path(
            normalized);

        if (!path.has_extension())
        {
            path.replace_extension(
                ".texanim");

            normalized =
                core::resources::ResourcePath::Normalize(
                    path.generic_string());
        }

        if (normalized.starts_with(
                "res/") ||
            normalized.starts_with(
                "sys/"))
        {
            return normalized;
        }

        const std::string resPath =
            "res/" +
            normalized;

        if (resources.Exists(
                resPath))
        {
            return resPath;
        }

        const std::string sysPath =
            "sys/" +
            normalized;

        if (resources.Exists(
                sysPath))
        {
            return sysPath;
        }

        return resPath;
    }

    bool ParseFloatText(
        const std::string_view text,
        float& output)
    {
        if (text.empty())
        {
            return false;
        }

        const std::string buffer(
            text);

        char* end =
            nullptr;

        const float value =
            std::strtof(
                buffer.c_str(),
                &end);

        if (end ==
                buffer.c_str() ||
            end == nullptr ||
            *end !=
                '\0' ||
            !std::isfinite(
                value))
        {
            return false;
        }

        output =
            value;

        return true;
    }

    bool ReadFramesPerSecond(
        const core::resources::DataSection& root,
        float& output)
    {
        const core::resources::DataSection* section =
            root.FindChild(
                "fps");

        if (section == nullptr)
        {
            return false;
        }

        if (section->TryGetFloat(
                output))
        {
            return
                std::isfinite(
                    output) &&
                output >
                    0.0f;
        }

        if (const std::string* value =
                section->AsString())
        {
            if (!ParseFloatText(
                    *value,
                    output))
            {
                return false;
            }

            return
                output >
                0.0f;
        }

        return false;
    }

    std::string EncodeBase64(
        const std::span<const std::byte> data)
    {
        static constexpr char Alphabet[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";

        std::string output;

        output.reserve(
            (
                data.size() +
                2
            ) /
            3 *
            4);

        std::size_t index =
            0;

        while (index <
               data.size())
        {
            const std::size_t remaining =
                data.size() -
                index;

            const unsigned int first =
                std::to_integer<unsigned char>(
                    data[
                        index]);

            const unsigned int second =
                remaining >
                    1
                    ? std::to_integer<unsigned char>(
                        data[
                            index + 1])
                    : 0u;

            const unsigned int third =
                remaining >
                    2
                    ? std::to_integer<unsigned char>(
                        data[
                            index + 2])
                    : 0u;

            const unsigned int block =
                (
                    first <<
                    16u
                ) |
                (
                    second <<
                    8u
                ) |
                third;

            output.push_back(
                Alphabet[
                    (
                        block >>
                        18u
                    ) &
                    0x3Fu]);

            output.push_back(
                Alphabet[
                    (
                        block >>
                        12u
                    ) &
                    0x3Fu]);

            if (remaining >
                1)
            {
                output.push_back(
                    Alphabet[
                        (
                            block >>
                            6u
                        ) &
                        0x3Fu]);
            }

            if (remaining >
                2)
            {
                output.push_back(
                    Alphabet[
                        block &
                        0x3Fu]);
            }

            index +=
                3;
        }

        return output;
    }

    bool ReadFrameString(
        const core::resources::DataSection& root,
        std::string& output)
    {
        output.clear();

        const core::resources::DataSection* section =
            root.FindChild(
                "frames");

        if (section == nullptr)
        {
            return true;
        }

        if (const std::string* value =
                section->AsString())
        {
            output =
                *value;

            return true;
        }

        if (const core::resources::DataSection::BinaryData* value =
                section->AsBinary())
        {
            output =
                EncodeBase64(
                    std::span<const std::byte>(
                        value->data(),
                        value->size()));

            return true;
        }

        return false;
    }

    bool FrameCharacterToIndex(
        const char character,
        std::size_t& output) noexcept
    {
        if (character >=
                'a' &&
            character <=
                'z')
        {
            output =
                static_cast<std::size_t>(
                    character -
                    'a');

            return true;
        }

        if (character >=
                'A' &&
            character <=
                'Z')
        {
            output =
                26u +
                static_cast<std::size_t>(
                    character -
                    'A');

            return true;
        }

        if (character >=
                '0' &&
            character <=
                '9')
        {
            output =
                52u +
                static_cast<std::size_t>(
                    character -
                    '0');

            return true;
        }

        if (character ==
            '+')
        {
            output =
                62u;

            return true;
        }

        if (character ==
            '/')
        {
            output =
                63u;

            return true;
        }

        return false;
    }

    bool BuildFrameSequence(
        const std::string_view frames,
        const std::size_t textureCount,
        std::vector<std::size_t>& output,
        std::string& error)
    {
        output.clear();

        if (textureCount ==
            0)
        {
            error =
                "Texture animation contains no textures.";

            return false;
        }

        if (frames.empty())
        {
            output.reserve(
                textureCount);

            for (std::size_t index = 0;
                 index <
                    textureCount;
                 ++index)
            {
                output.push_back(
                    index);
            }

            return true;
        }

        output.reserve(
            frames.size());

        for (const char character :
             frames)
        {
            if (character ==
                    ' ' ||
                character ==
                    '\t' ||
                character ==
                    '\r' ||
                character ==
                    '\n')
            {
                continue;
            }

            std::size_t frameIndex =
                0;

            if (!FrameCharacterToIndex(
                    character,
                    frameIndex))
            {
                error =
                    "Texture animation contains invalid frame character.";

                return false;
            }

            if (frameIndex >=
                textureCount)
            {
                error =
                    "Texture animation frame references texture index " +
                    std::to_string(
                        frameIndex) +
                    ", texture count is " +
                    std::to_string(
                        textureCount) +
                    ".";

                return false;
            }

            output.push_back(
                frameIndex);
        }

        if (output.empty())
        {
            error =
                "Texture animation frame sequence is empty.";

            return false;
        }

        return true;
    }
}

namespace core::assets
{
    bool TextureAnimationLoader::Load(
        const resources::ResourceFileSystem& resources,
        const std::string_view resourceReference,
        TextureAnimation& output,
        std::string& error) const
    {
        output =
            {};

        error.clear();

        if (!resources.IsInitialized())
        {
            error =
                "Resource filesystem is not initialized.";

            return false;
        }

        const std::string logicalPath =
            BuildResourcePath(
                resources,
                resourceReference);

        if (logicalPath.empty())
        {
            error =
                "Texture animation reference is invalid.";

            return false;
        }

        if (!resources.Exists(
                logicalPath))
        {
            error =
                "Texture animation does not exist: " +
                logicalPath;

            return false;
        }

        std::vector<std::byte>
            encoded;

        if (!resources.ReadBinary(
                logicalPath,
                encoded))
        {
            error =
                "Unable to read texture animation: " +
                logicalPath;

            return false;
        }

        resources::PackedSectionReader
            reader;

        resources::DataSection
            root;

        if (!reader.Read(
                std::span<const std::byte>(
                    encoded.data(),
                    encoded.size()),
                root,
                error))
        {
            error =
                "Unable to parse texture animation " +
                logicalPath +
                ": " +
                error;

            return false;
        }

        TextureAnimation
            animation;

        animation.logicalPath =
            logicalPath;

        if (!ReadFramesPerSecond(
                root,
                animation.framesPerSecond))
        {
            error =
                "Texture animation contains invalid fps: " +
                logicalPath;

            return false;
        }

        const std::vector<const resources::DataSection*>
            textureSections =
                root.FindChildren(
                    "texture");

        if (textureSections.empty())
        {
            error =
                "Texture animation contains no texture frames: " +
                logicalPath;

            return false;
        }

        TextureResolver
            resolver;

        animation.textures.reserve(
            textureSections.size());

        for (const resources::DataSection* textureSection :
             textureSections)
        {
            if (textureSection ==
                nullptr)
            {
                continue;
            }

            const std::string* reference =
                textureSection->AsString();

            if (reference ==
                    nullptr ||
                reference->empty())
            {
                error =
                    "Texture animation contains invalid texture reference: " +
                    logicalPath;

                return false;
            }

            TextureResource
                texture;

            if (!resolver.Resolve(
                    resources,
                    *reference,
                    texture))
            {
                error =
                    "Unable to resolve texture animation frame: " +
                    *reference;

                return false;
            }

            if (!texture.exists)
            {
                error =
                    "Texture animation frame does not exist: " +
                    texture.logicalPath;

                return false;
            }

            animation.textures.push_back(
                std::move(
                    texture));
        }

        std::string frames;

        if (!ReadFrameString(
                root,
                frames))
        {
            error =
                "Texture animation contains invalid frames field: " +
                logicalPath;

            return false;
        }

        if (!BuildFrameSequence(
                frames,
                animation.textures.size(),
                animation.frameSequence,
                error))
        {
            error =
                logicalPath +
                ": " +
                error;

            return false;
        }

        output =
            std::move(
                animation);

        return true;
    }
}