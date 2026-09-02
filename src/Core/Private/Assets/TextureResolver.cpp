#include "Core/Assets/TextureResolver.h"

#include "Core/Resources/ResourcePath.h"

#include <filesystem>
#include <string>
#include <utility>

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

    bool UsesRuntimeDds(
        const std::filesystem::path& path)
    {
        const std::string extension =
            ToLower(
                path.extension().string());

        return
            extension.empty() ||
            extension == ".jpg" ||
            extension == ".jpeg" ||
            extension == ".png" ||
            extension == ".tga" ||
            extension == ".bmp";
    }
}

namespace core::assets
{
    bool TextureResolver::Resolve(
        const resources::ResourceFileSystem& resources,
        const std::string_view textureReference,
        TextureResource& output) const
    {
        output = {};

        if (!resources.IsInitialized())
        {
            return false;
        }

        const std::string sourceLogicalPath =
            resources::ResourcePath::ToResPath(
                textureReference);

        if (sourceLogicalPath.empty())
        {
            return false;
        }

        TextureResource resource;

        resource.sourceReference =
            std::string(textureReference);

        resource.sourceLogicalPath =
            sourceLogicalPath;

        const std::filesystem::path sourcePath(
            sourceLogicalPath);

        const std::string sourceExtension =
            ToLower(
                sourcePath.extension().string());

        if (sourceExtension == ".dds")
        {
            resource.logicalPath =
                sourceLogicalPath;

            resource.exists =
                resources.Exists(
                    resource.logicalPath);

            output =
                std::move(resource);

            return true;
        }

        if (UsesRuntimeDds(sourcePath))
        {
            std::filesystem::path ddsPath =
                sourcePath;

            ddsPath.replace_extension(
                ".dds");

            const std::string ddsLogicalPath =
                resources::ResourcePath::Normalize(
                    ddsPath.generic_string());

            if (!ddsLogicalPath.empty() &&
                resources.Exists(ddsLogicalPath))
            {
                resource.logicalPath =
                    ddsLogicalPath;

                resource.exists = true;

                output =
                    std::move(resource);

                return true;
            }

            resource.logicalPath =
                ddsLogicalPath;
        }

        if (resources.Exists(
                sourceLogicalPath))
        {
            resource.logicalPath =
                sourceLogicalPath;

            resource.exists = true;

            output =
                std::move(resource);

            return true;
        }

        if (resource.logicalPath.empty())
        {
            resource.logicalPath =
                sourceLogicalPath;
        }

        resource.exists = false;

        output =
            std::move(resource);

        return true;
    }
}