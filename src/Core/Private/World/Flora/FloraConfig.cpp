#include "Core/World/Flora/FloraConfig.h"

#include "Core/Resources/ResourcePath.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    std::string ToLower(
        std::string value)
    {
        for (char& character : value)
        {
            if (character >= 'A' &&
                character <= 'Z')
            {
                character =
                    static_cast<char>(
                        character - 'A' + 'a');
            }
        }

        return value;
    }

    bool IsTextureExtension(
        const std::string& extension) noexcept
    {
        return
            extension == ".dds" ||
            extension == ".tga" ||
            extension == ".png" ||
            extension == ".jpg" ||
            extension == ".jpeg" ||
            extension == ".bmp";
    }
}

namespace core::world::flora
{
    std::string BuildFloraTextureKey(
        const std::string_view textureReference)
    {
        std::string normalized =
            resources::ResourcePath::Normalize(
                textureReference);

        if (normalized.empty())
        {
            return {};
        }

        if (normalized.starts_with(
                "res/"))
        {
            normalized.erase(
                0,
                4);
        }

        std::filesystem::path path(
            normalized);

        const std::string extension =
            ToLower(
                path.extension().string());

        if (IsTextureExtension(
                extension))
        {
            path.replace_extension();
        }

        normalized =
            resources::ResourcePath::Normalize(
                path.generic_string());

        return normalized;
    }

    std::vector<const FloraEcotype*>
    FloraConfig::FindEcotypesByTexture(
        const std::string_view textureReference) const
    {
        std::vector<const FloraEcotype*>
            result;

        const std::string key =
            BuildFloraTextureKey(
                textureReference);

        if (key.empty())
        {
            return result;
        }

        for (const FloraEcotype& ecotype :
             ecotypes)
        {
            for (const FloraTextureRule& texture :
                 ecotype.textures)
            {
                if (BuildFloraTextureKey(
                        texture.textureReference) !=
                    key)
                {
                    continue;
                }

                result.push_back(
                    &ecotype);

                break;
            }
        }

        return result;
    }
}