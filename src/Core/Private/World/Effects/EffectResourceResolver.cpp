#include "Core/World/Effects/EffectResourceResolver.h"

#include "Core/Resources/ResourceFileSystem.h"
#include "Core/Resources/ResourcePath.h"

#include <filesystem>
#include <string>
#include <string_view>

namespace
{
    const core::resources::ResourceEntry* FindResource(
        const core::resources::ResourceFileSystem& resources,
        const std::string_view path)
    {
        return
            resources.Find(
                path);
    }

    std::string ResolveEntry(
        const core::resources::ResourceFileSystem& resources,
        const std::string_view path)
    {
        const core::resources::ResourceEntry* entry =
            FindResource(
                resources,
                path);

        if (entry == nullptr)
        {
            return {};
        }

        return
            entry->logicalPath;
    }
}

namespace core::world::effects
{
    std::string EffectResourceResolver::Resolve(
        const resources::ResourceFileSystem& resources,
        const std::string_view reference,
        const EffectResourceKind kind)
    {
        std::string normalized =
            resources::ResourcePath::Normalize(
                reference);

        if (normalized.empty())
        {
            return {};
        }

        std::filesystem::path path(
            normalized);

        if (path.has_extension())
        {
            if (path.extension() !=
                ".xml")
            {
                return {};
            }
        }
        else
        {
            normalized +=
                ".xml";
        }

        const bool rooted =
            normalized.starts_with(
                "res/") ||
            normalized.starts_with(
                "sys/");

        if (rooted)
        {
            return
                ResolveEntry(
                    resources,
                    normalized);
        }

        const std::string resPath =
            "res/" +
            normalized;

        if (std::string resolved =
                ResolveEntry(
                    resources,
                    resPath);
            !resolved.empty())
        {
            return resolved;
        }

        const std::string sysPath =
            "sys/" +
            normalized;

        if (std::string resolved =
                ResolveEntry(
                    resources,
                    sysPath);
            !resolved.empty())
        {
            return resolved;
        }

        const std::string_view kindDirectory =
            kind == EffectResourceKind::Particle
                ? "res/particles/"
                : "res/sfx/";

        return
            ResolveEntry(
                resources,
                std::string(kindDirectory) +
                    normalized);
    }
}
