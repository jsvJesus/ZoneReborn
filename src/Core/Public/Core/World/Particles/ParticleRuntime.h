#pragma once

#include "Core/Math/Transform3x4.h"
#include "Core/Math/Vector3.h"
#include "Core/World/Particles/ParticleDefinition.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace core::world::particles
{
    struct ParticleRuntimeParticle final
    {
        math::Vector3 position{};
        math::Vector3 previousPosition{};
        math::Vector3 velocity{};

        std::array<float, 4> colour
        {
            1.0f,
            1.0f,
            1.0f,
            1.0f
        };

        float age =
            0.0f;

        float initialSize =
            1.0f;

        float size =
            1.0f;

        float rotation =
            0.0f;

        float angularVelocity =
            0.0f;
    };

    struct ParticleRuntimeStatistics final
    {
        std::size_t spawned =
            0;

        std::size_t killed =
            0;
    };

    class ParticleRuntimeSystem final
    {
    public:
        [[nodiscard]]
        bool Initialize(
            const ParticleSystemDefinition& definition,
            const math::Transform3x4& transform,
            std::uint32_t randomSeed,
            std::string& error);

        void Update(
            float deltaSeconds) noexcept;

        [[nodiscard]]
        const ParticleSystemDefinition&
        Definition() const noexcept;

        [[nodiscard]]
        const math::Transform3x4&
        Transform() const noexcept;

        [[nodiscard]]
        const std::vector<ParticleRuntimeParticle>&
        Particles() const noexcept;

        [[nodiscard]]
        std::size_t ActiveParticleCount() const noexcept;

        [[nodiscard]]
        std::size_t Capacity() const noexcept;

        [[nodiscard]]
        float Age() const noexcept;

        [[nodiscard]]
        const ParticleRuntimeStatistics&
        Statistics() const noexcept;

    private:
        [[nodiscard]]
        float Random01() noexcept;

        [[nodiscard]]
        math::Vector3 SampleVector(
            const ParticleVectorGenerator& generator) noexcept;

        [[nodiscard]]
        bool IsSourceActive(
            const ParticleSourceAction& source) const noexcept;

        void SpawnFromSource(
            const ParticleSourceAction& source,
            float deltaSeconds,
            float& accumulator) noexcept;

        void SpawnParticle(
            const ParticleSourceAction& source) noexcept;

        [[nodiscard]]
        bool UpdateParticle(
            ParticleRuntimeParticle& particle,
            float deltaSeconds) noexcept;

        ParticleSystemDefinition
            definition_;

        math::Transform3x4
            transform_;

        std::vector<ParticleRuntimeParticle>
            particles_;

        std::vector<float>
            sourceAccumulators_;

        ParticleRuntimeStatistics
            statistics_;

        std::uint32_t randomState_ =
            0x12345678u;

        std::size_t capacity_ =
            0;

        float age_ =
            0.0f;

        bool initialized_ =
            false;
    };
}