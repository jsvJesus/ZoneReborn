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

    bool IsTextureReferenceExtension(
        const std::string& extension) noexcept
    {
        return
            extension.empty() ||
            extension == ".dds" ||
            extension == ".tga" ||
            extension == ".png" ||
            extension == ".jpg" ||
            extension == ".jpeg" ||
            extension == ".bmp";
    }

    std::string BuildRuntimeDdsPath(
        const std::string& logicalPath)
    {
        std::filesystem::path path(
            logicalPath);

        const std::string extension =
            ToLower(
                path.extension().string());

        if (!IsTextureReferenceExtension(
                extension))
        {
            return {};
        }

        if (extension != ".dds")
        {
            path.replace_extension(
                ".dds");
        }

        return
            core::resources::ResourcePath::Normalize(
                path.generic_string());
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

        const std::string normalizedReference =
            resources::ResourcePath::Normalize(
                textureReference);

        if (normalizedReference.empty())
        {
            return false;
        }

        std::filesystem::path
            sourcePath(
                normalizedReference);

        const std::string extension =
            ToLower(
                sourcePath.extension().string());

        if (!IsTextureReferenceExtension(
                extension))
        {
            return false;
        }

        if (extension !=
            ".dds")
        {
            sourcePath.replace_extension(
                ".dds");
        }

        const std::string runtimeReference =
            resources::ResourcePath::Normalize(
                sourcePath.generic_string());

        if (runtimeReference.empty())
        {
            return false;
        }

        std::string logicalPath;

        if (runtimeReference.starts_with(
                "res/") ||
            runtimeReference.starts_with(
                "sys/"))
        {
            logicalPath =
                runtimeReference;
        }
        else
        {
            const std::string resPath =
                "res/" +
                runtimeReference;

            const std::string sysPath =
                "sys/" +
                runtimeReference;

            if (resources.Exists(
                    resPath))
            {
                logicalPath =
                    resPath;
            }
            else if (resources.Exists(
                         sysPath))
            {
                logicalPath =
                    sysPath;
            }
            else
            {
                logicalPath =
                    resPath;
            }
        }

        TextureResource
            resource;

        resource.sourceReference =
            std::string(
                textureReference);

        resource.sourceLogicalPath =
            normalizedReference;

        resource.logicalPath =
            logicalPath;

        resource.exists =
            resources.Exists(
                logicalPath);

        output =
            std::move(
                resource);

        return true;
    }
}