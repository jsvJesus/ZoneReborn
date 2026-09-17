#pragma once

#include "Core/Resources/ResourceFileSystem.h"

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace core::world::particles
{
    struct ParticlePackAuditFailure final
    {
        std::string resource;
        std::string error;
    };

    struct ParticlePackAuditResult final
    {
        std::size_t resourceCount =
            0;

        std::size_t loadedResourceCount =
            0;

        std::size_t failedResourceCount =
            0;

        std::size_t systemCount =
            0;

        std::size_t actionCount =
            0;

        std::size_t rendererCount =
            0;

        std::size_t vectorGeneratorCount =
            0;

        std::size_t textureReferenceCount =
            0;

        std::size_t animatedTextureReferenceCount =
            0;

        std::size_t missingTextureCount =
            0;

        std::size_t unsupportedActionCount =
            0;

        std::size_t unsupportedRendererCount =
            0;

        std::size_t unsupportedVectorGeneratorCount =
            0;

        std::map<std::string, std::size_t>
            unsupportedActions;

        std::map<std::string, std::size_t>
            unsupportedRenderers;

        std::map<std::string, std::size_t>
            unsupportedVectorGenerators;

        std::vector<ParticlePackAuditFailure>
            failures;
    };

    class ParticlePackAuditor final
    {
    public:
        [[nodiscard]]
        bool Run(
            const resources::ResourceFileSystem& resources,
            ParticlePackAuditResult& output,
            std::string& error) const;
    };
}