#include "Core/World/Flora/FloraConfig.h"

#include "Core/Resources/ResourcePath.h"

#include <string>
#include <string_view>
#include <vector>

namespace
{
    std::string BuildTextureKey(
        const std::string_view reference)
    {
        std::string normalized =
            core::resources::ResourcePath::Normalize(
                reference);

        if (normalized.starts_with(
                "res/"))
        {
            normalized.erase(
                0,
                4);
        }

        return normalized;
    }
}

namespace core::world::flora
{
    std::vector<const FloraEcotype*>
    FloraConfig::FindEcotypesByTexture(
        const std::string_view textureReference) const
    {
        std::vector<const FloraEcotype*>
            result;

        const std::string key =
            BuildTextureKey(
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
                if (BuildTextureKey(
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