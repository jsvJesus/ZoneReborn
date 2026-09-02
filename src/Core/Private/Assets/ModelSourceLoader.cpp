#include "Core/Assets/ModelSourceLoader.h"

#include "Core/Assets/ModelResolver.h"

#include <utility>

namespace core::assets
{
    bool ModelSourceLoader::Load(
        const resources::ResourceFileSystem& resources,
        const std::string_view modelReference,
        ModelSource& output,
        std::string& error) const
    {
        output = {};
        error.clear();

        if (!resources.IsInitialized())
        {
            error =
                "Resource filesystem is not initialized.";

            return false;
        }

        ModelResolver resolver;

        ModelResource resource;

        if (!resolver.Resolve(
                resources,
                modelReference,
                resource))
        {
            error =
                "Model reference is invalid.";

            return false;
        }

        if (!resource.exists)
        {
            error =
                "Model resource was not found: " +
                resource.logicalPath;

            return false;
        }

        std::vector<std::byte> data;

        if (!resources.ReadBinary(
                resource.logicalPath,
                data))
        {
            error =
                "Unable to read model resource: " +
                resource.logicalPath;

            return false;
        }

        ModelSource source;

        source.resource =
            std::move(resource);

        source.data =
            std::move(data);

        output =
            std::move(source);

        return true;
    }
}