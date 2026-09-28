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
        for (char& character :
             value)
        {
            if (character >= 'A' &&
                character <= 'Z')
            {
                character =
                    static_cast<char>(
                        character -
                        'A' +
                        'a');
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

    bool IsDigit(
        const char value) noexcept
    {
        return
            value >= '0' &&
            value <= '9';
    }

    std::string RemoveNumericVariantSuffix(
        std::string value)
    {
        if (value.empty())
        {
            return value;
        }

        std::size_t digitStart =
            value.size();

        while (digitStart > 0 &&
               IsDigit(
                   value[digitStart - 1]))
        {
            --digitStart;
        }

        if (digitStart ==
            value.size())
        {
            return value;
        }

        if (digitStart == 0)
        {
            return value;
        }

        const char separator =
            value[digitStart - 1];

        if (separator != '_' &&
            separator != '-')
        {
            return value;
        }

        value.resize(
            digitStart - 1);

        return value;
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

        normalized =
            ToLower(
                std::move(
                    normalized));

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

        return
            ToLower(
                std::move(
                    normalized));
    }

    std::string BuildFloraTextureFamilyKey(
        const std::string_view textureReference)
    {
        const std::string key =
            BuildFloraTextureKey(
                textureReference);

        if (key.empty())
        {
            return {};
        }

        std::filesystem::path path(
            key);

        std::string filename =
            path.filename().string();

        filename =
            RemoveNumericVariantSuffix(
                std::move(
                    filename));

        if (filename.empty())
        {
            return key;
        }

        path.replace_filename(
            filename);

        return
            ToLower(
                resources::ResourcePath::Normalize(
                    path.generic_string()));
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

        //
        // Сначала обязательно ищем точное соответствие.
        //
        // Это не позволяет variant fallback перебить
        // явно настроенный ecotype.
        //
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

        if (!result.empty())
        {
            return result;
        }

        //
        // Старые карты SO содержат несколько вариантов
        // одной terrain texture:
        //
        // name
        // name_1
        // name_2
        // name_02
        // name_03
        //
        // Если точного правила нет, разрешаем такой
        // вариант наследовать ecotype своей texture-family.
        //
        const std::string familyKey =
            BuildFloraTextureFamilyKey(
                textureReference);

        if (familyKey.empty() ||
            familyKey == key)
        {
            return result;
        }

        for (const FloraEcotype& ecotype :
             ecotypes)
        {
            for (const FloraTextureRule& texture :
                 ecotype.textures)
            {
                if (BuildFloraTextureFamilyKey(
                        texture.textureReference) !=
                    familyKey)
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