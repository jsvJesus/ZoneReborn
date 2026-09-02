#include "Core/Assets/ModelResolver.h"

#include "Core/Resources/ResourcePath.h"

#include <string>
#include <unordered_set>
#include <utility>

namespace core::assets
{
    bool ModelResolver::Resolve(
        const resources::ResourceFileSystem& resources,
        const std::string_view modelReference,
        ModelResource& output) const
    {
        output = {};

        if (!resources.IsInitialized())
        {
            return false;
        }

        const std::string logicalPath =
            resources::ResourcePath::ToResPath(
                modelReference);

        if (logicalPath.empty())
        {
            return false;
        }

        ModelResource model;

        model.sourceReference =
            std::string(modelReference);

        model.logicalPath =
            logicalPath;

        const resources::ResourceEntry* entry =
            resources.Find(
                logicalPath);

        if (entry != nullptr)
        {
            model.exists = true;
            model.fileSize = entry->size;
        }

        output =
            std::move(model);

        return true;
    }

    std::vector<ModelResource>
    ModelResolver::ResolveChunkModels(
        const resources::ResourceFileSystem& resources,
        const world::Chunk& chunk) const
    {
        std::vector<ModelResource> result;

        std::unordered_set<std::string> added;

        const auto addModel =
            [&](const world::ChunkModelInstance& instance)
            {
                ModelResource model;

                if (!Resolve(
                        resources,
                        instance.resource,
                        model))
                {
                    return;
                }

                if (!added.insert(
                        model.logicalPath).second)
                {
                    return;
                }

                result.push_back(
                    std::move(model));
            };

        for (const world::ChunkModelInstance& model :
             chunk.models)
        {
            addModel(model);
        }

        for (const world::ChunkModelInstance& shell :
             chunk.shells)
        {
            addModel(shell);
        }

        return result;
    }
}