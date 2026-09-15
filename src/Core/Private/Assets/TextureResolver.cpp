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

        const std::string sourceLogicalPath =
            resources::ResourcePath::ToResPath(
                textureReference);

        if (sourceLogicalPath.empty())
        {
            return false;
        }

        const std::string runtimeLogicalPath =
            BuildRuntimeDdsPath(
                sourceLogicalPath);

        if (runtimeLogicalPath.empty())
        {
            return false;
        }

        TextureResource resource;

        resource.sourceReference =
            std::string(
                textureReference);

        resource.sourceLogicalPath =
            sourceLogicalPath;

        resource.logicalPath =
            runtimeLogicalPath;

        resource.exists =
            resources.Exists(
                runtimeLogicalPath);

        output =
            std::move(
                resource);

        return true;
    }
}