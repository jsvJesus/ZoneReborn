#include "Core/World/Particles/ParticlePackAudit.h"

#include "Core/Resources/ResourceType.h"
#include "Core/World/Particles/ParticleLoader.h"

#include <algorithm>
#include <string>
#include <variant>
#include <vector>

namespace
{
    bool IsParticleResource(
        const std::string& path)
    {
        const bool particleDirectory =
            path.starts_with(
                "res/particles/") ||
            path.starts_with(
                "sys/particles/");

        return
            particleDirectory &&
            path.ends_with(
                ".xml");
    }

    void RegisterUnsupportedGenerator(
        const core::world::particles::ParticleVectorGenerator& generator,
        core::world::particles::ParticlePackAuditResult& output)
    {
        if (generator.type !=
            core::world::particles::ParticleVectorGeneratorType::Unsupported)
        {
            return;
        }

        const std::string name =
            generator.typeName.empty()
                ? "<empty>"
                : generator.typeName;

        ++output.unsupportedVectorGenerators[
            name];
    }

    void CollectUnsupportedTypes(
        const core::world::particles::ParticleDefinition& definition,
        core::world::particles::ParticlePackAuditResult& output)
    {
        using namespace core::world::particles;

        for (const ParticleSystemDefinition& system :
             definition.systems)
        {
            if (system.hasRenderer &&
                system.renderer.type ==
                    ParticleRendererType::Unsupported)
            {
                const std::string name =
                    system.renderer.typeName.empty()
                        ? "<empty>"
                        : system.renderer.typeName;

                ++output.unsupportedRenderers[
                    name];
            }

            for (const ParticleActionDefinition& action :
                 system.actions)
            {
                if (action.type ==
                    ParticleActionType::Unsupported)
                {
                    const std::string name =
                        action.typeName.empty()
                            ? "<empty>"
                            : action.typeName;

                    ++output.unsupportedActions[
                        name];

                    continue;
                }

                if (const ParticleSourceAction* source =
                        std::get_if<ParticleSourceAction>(
                            &action.data))
                {
                    RegisterUnsupportedGenerator(
                        source->positionSource,
                        output);

                    RegisterUnsupportedGenerator(
                        source->velocitySource,
                        output);

                    continue;
                }

                if (const ParticleJitterAction* jitter =
                        std::get_if<ParticleJitterAction>(
                            &action.data))
                {
                    RegisterUnsupportedGenerator(
                        jitter->positionSource,
                        output);

                    RegisterUnsupportedGenerator(
                        jitter->velocitySource,
                        output);
                }
            }
        }
    }
}

namespace core::world::particles
{
    bool ParticlePackAuditor::Run(
        const resources::ResourceFileSystem& resources,
        ParticlePackAuditResult& output,
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

        std::vector<const resources::ResourceEntry*>
            xmlResources =
                resources.FindByType(
                    resources::ResourceType::Xml);

        std::vector<const resources::ResourceEntry*>
            particleResources;

        particleResources.reserve(
            xmlResources.size());

        for (const resources::ResourceEntry* entry :
             xmlResources)
        {
            if (entry == nullptr ||
                !IsParticleResource(
                    entry->logicalPath))
            {
                continue;
            }

            particleResources.push_back(
                entry);
        }

        std::sort(
            particleResources.begin(),
            particleResources.end(),
            [](
                const resources::ResourceEntry* left,
                const resources::ResourceEntry* right)
            {
                return
                    left->logicalPath <
                    right->logicalPath;
            });

        output.resourceCount =
            particleResources.size();

        ParticleLoader
            loader;

        for (const resources::ResourceEntry* entry :
             particleResources)
        {
            ParticleDefinition
                definition;

            std::string
                loadError;

            if (!loader.Load(
                    resources,
                    entry->logicalPath,
                    definition,
                    loadError))
            {
                ++output.failedResourceCount;

                ParticlePackAuditFailure
                    failure;

                failure.resource =
                    entry->logicalPath;

                failure.error =
                    std::move(
                        loadError);

                output.failures.push_back(
                    std::move(
                        failure));

                continue;
            }

            ++output.loadedResourceCount;

            output.systemCount +=
                definition.statistics.systemCount;

            output.actionCount +=
                definition.statistics.actionCount;

            output.rendererCount +=
                definition.statistics.rendererCount;

            output.vectorGeneratorCount +=
                definition.statistics.vectorGeneratorCount;

            output.textureReferenceCount +=
                definition.statistics.textureReferenceCount;

            output.animatedTextureReferenceCount +=
                definition.statistics.animatedTextureReferenceCount;

            output.missingTextureCount +=
                definition.statistics.missingTextureCount;

            output.unsupportedActionCount +=
                definition.statistics.unsupportedActionCount;

            output.unsupportedRendererCount +=
                definition.statistics.unsupportedRendererCount;

            output.unsupportedVectorGeneratorCount +=
                definition.statistics.unsupportedVectorGeneratorCount;

            CollectUnsupportedTypes(
                definition,
                output);
        }

        return true;
    }
}