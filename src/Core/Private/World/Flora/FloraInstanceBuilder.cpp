#include "Core/World/Flora/FloraInstanceBuilder.h"

#include "Core/Resources/ResourcePath.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    constexpr float TerrainBlockSize =
        100.0f;

    constexpr float PlacementStep =
        2.5f;

    constexpr float Pi =
        3.14159265358979323846f;

    constexpr std::uint32_t MaximumSpawnPerCell =
        8;

    std::uint32_t HashMix(
        std::uint32_t value) noexcept
    {
        value ^= value >> 16u;
        value *= 0x7feb352du;
        value ^= value >> 15u;
        value *= 0x846ca68bu;
        value ^= value >> 16u;

        return value;
    }

    std::uint32_t HashCombine(
        const std::uint32_t seed,
        const std::uint32_t value) noexcept
    {
        return HashMix(
            seed ^
            (
                value +
                0x9e3779b9u +
                (seed << 6u) +
                (seed >> 2u)
            ));
    }

    std::uint32_t HashString(
        const std::string_view value) noexcept
    {
        std::uint32_t hash =
            2166136261u;

        for (const char character :
             value)
        {
            hash ^=
                static_cast<std::uint8_t>(
                    character);

            hash *=
                16777619u;
        }

        return HashMix(
            hash);
    }

    float HashFloat(
        const std::uint32_t value) noexcept
    {
        constexpr float Divisor =
            1.0f /
            static_cast<float>(
                0x00FFFFFFu);

        return
            static_cast<float>(
                HashMix(value) &
                0x00FFFFFFu) *
            Divisor;
    }

    float ClampTerrainCoordinate(
        const float value) noexcept
    {
        return std::clamp(
            value,
            0.001f,
            TerrainBlockSize -
                0.001f);
    }

    float SampleTerrainHeight(
        const core::world::TerrainHeightData& heightData,
        const float localX,
        const float localZ) noexcept
    {
        const std::uint32_t visibleWidth =
            heightData.VisibleWidth();

        const std::uint32_t visibleHeight =
            heightData.VisibleHeight();

        if (visibleWidth < 2 ||
            visibleHeight < 2)
        {
            return 0.0f;
        }

        const float normalizedX =
            std::clamp(
                localX / TerrainBlockSize,
                0.0f,
                1.0f);

        const float normalizedZ =
            std::clamp(
                localZ / TerrainBlockSize,
                0.0f,
                1.0f);

        const float sampleX =
            normalizedX *
            static_cast<float>(
                visibleWidth - 1);

        const float sampleZ =
            normalizedZ *
            static_cast<float>(
                visibleHeight - 1);

        const std::uint32_t x0 =
            static_cast<std::uint32_t>(
                std::floor(
                    sampleX));

        const std::uint32_t z0 =
            static_cast<std::uint32_t>(
                std::floor(
                    sampleZ));

        const std::uint32_t x1 =
            std::min(
                x0 + 1,
                visibleWidth - 1);

        const std::uint32_t z1 =
            std::min(
                z0 + 1,
                visibleHeight - 1);

        const float tx =
            sampleX -
            static_cast<float>(
                x0);

        const float tz =
            sampleZ -
            static_cast<float>(
                z0);

        const std::uint32_t offset =
            heightData.visibleOffset;

        const float h00 =
            heightData.At(
                x0 + offset,
                z0 + offset);

        const float h10 =
            heightData.At(
                x1 + offset,
                z0 + offset);

        const float h01 =
            heightData.At(
                x0 + offset,
                z1 + offset);

        const float h11 =
            heightData.At(
                x1 + offset,
                z1 + offset);

        const float h0 =
            h00 +
            (h10 - h00) *
                tx;

        const float h1 =
            h01 +
            (h11 - h01) *
                tx;

        return
            h0 +
            (h1 - h0) *
                tz;
    }

    bool IsTerrainHole(
        const core::world::TerrainHoleData& holes,
        const float localX,
        const float localZ) noexcept
    {
        if (!holes.present ||
            holes.width == 0 ||
            holes.height == 0)
        {
            return false;
        }

        const float normalizedX =
            std::clamp(
                localX / TerrainBlockSize,
                0.0f,
                0.999999f);

        const float normalizedZ =
            std::clamp(
                localZ / TerrainBlockSize,
                0.0f,
                0.999999f);

        const std::uint32_t x =
            std::min(
                static_cast<std::uint32_t>(
                    normalizedX *
                    static_cast<float>(
                        holes.width)),
                holes.width - 1);

        const std::uint32_t z =
            std::min(
                static_cast<std::uint32_t>(
                    normalizedZ *
                    static_cast<float>(
                        holes.height)),
                holes.height - 1);

        return holes.IsHole(
            x,
            z);
    }

    const std::string*
    DominantTextureAt(
        const core::world::TerrainDominantTextureData& dominant,
        const float localX,
        const float localZ) noexcept
    {
        if (!dominant.present ||
            dominant.width == 0 ||
            dominant.height == 0 ||
            dominant.textureReferences.empty())
        {
            return nullptr;
        }

        const float normalizedX =
            std::clamp(
                localX / TerrainBlockSize,
                0.0f,
                0.999999f);

        const float normalizedZ =
            std::clamp(
                localZ / TerrainBlockSize,
                0.0f,
                0.999999f);

        const std::uint32_t x =
            std::min(
                static_cast<std::uint32_t>(
                    normalizedX *
                    static_cast<float>(
                        dominant.width)),
                dominant.width - 1);

        const std::uint32_t z =
            std::min(
                static_cast<std::uint32_t>(
                    normalizedZ *
                    static_cast<float>(
                        dominant.height)),
                dominant.height - 1);

        const std::size_t index =
            static_cast<std::size_t>(
                z) *
                dominant.width +
            x;

        if (index >=
            dominant.indices.size())
        {
            return nullptr;
        }

        const std::uint8_t textureIndex =
            dominant.indices[index];

        if (textureIndex >=
            dominant.textureReferences.size())
        {
            return nullptr;
        }

        return
            &dominant.textureReferences[
                textureIndex];
    }

    std::uint8_t SampleLayerBlend(
        const core::world::TerrainLayerData& layer,
        const float localX,
        const float localZ) noexcept
    {
        if (layer.width == 0 ||
            layer.height == 0 ||
            layer.blend.size() <
                static_cast<std::size_t>(
                    layer.width) *
                    layer.height)
        {
            return 0;
        }

        const float normalizedX =
            std::clamp(
                localX / TerrainBlockSize,
                0.0f,
                0.999999f);

        const float normalizedZ =
            std::clamp(
                localZ / TerrainBlockSize,
                0.0f,
                0.999999f);

        const std::uint32_t x =
            std::min(
                static_cast<std::uint32_t>(
                    normalizedX *
                    static_cast<float>(
                        layer.width)),
                layer.width - 1);

        const std::uint32_t z =
            std::min(
                static_cast<std::uint32_t>(
                    normalizedZ *
                    static_cast<float>(
                        layer.height)),
                layer.height - 1);

        return layer.BlendAt(
            x,
            z);
    }

    std::string_view LayerTextureReference(
        const core::world::TerrainLayerData& layer) noexcept
    {
        if (!layer.texture.sourceReference.empty())
        {
            return
                layer.texture.sourceReference;
        }

        if (!layer.texture.sourceLogicalPath.empty())
        {
            return
                layer.texture.sourceLogicalPath;
        }

        return {};
    }

    std::string_view SelectTerrainTexture(
        const std::vector<
            core::world::TerrainLayerData>& layers,
        const core::world::TerrainDominantTextureData& dominant,
        const float localX,
        const float localZ,
        const std::uint32_t seed) noexcept
    {
        std::uint32_t totalWeight =
            0;

        for (const core::world::TerrainLayerData& layer :
             layers)
        {
            if (LayerTextureReference(
                    layer).empty())
            {
                continue;
            }

            totalWeight +=
                SampleLayerBlend(
                    layer,
                    localX,
                    localZ);
        }

        if (totalWeight != 0)
        {
            std::uint32_t selectedWeight =
                static_cast<std::uint32_t>(
                    HashFloat(seed) *
                    static_cast<float>(
                        totalWeight));

            if (selectedWeight >=
                totalWeight)
            {
                selectedWeight =
                    totalWeight - 1;
            }

            for (const core::world::TerrainLayerData& layer :
                 layers)
            {
                const std::string_view reference =
                    LayerTextureReference(
                        layer);

                if (reference.empty())
                {
                    continue;
                }

                const std::uint32_t weight =
                    SampleLayerBlend(
                        layer,
                        localX,
                        localZ);

                if (weight == 0)
                {
                    continue;
                }

                if (selectedWeight <
                    weight)
                {
                    return reference;
                }

                selectedWeight -=
                    weight;
            }
        }

        const std::string* dominantTexture =
            DominantTextureAt(
                dominant,
                localX,
                localZ);

        if (dominantTexture !=
            nullptr)
        {
            return
                *dominantTexture;
        }

        return {};
    }

    float EcotypeTextureWeight(
        const core::world::flora::FloraEcotype& ecotype,
        const std::string_view textureReference)
    {
        const std::string key =
            core::world::flora::BuildFloraTextureKey(
                textureReference);

        if (key.empty())
        {
            return 0.0f;
        }

        float weight =
            0.0f;

        for (const core::world::flora::FloraTextureRule& rule :
             ecotype.textures)
        {
            if (core::world::flora::BuildFloraTextureKey(
                    rule.textureReference) !=
                key)
            {
                continue;
            }

            weight +=
                std::max(
                    rule.weight,
                    0.0f);
        }

        return weight;
    }

    const core::world::flora::FloraEcotype*
    SelectEcotype(
        const core::world::flora::FloraConfig& config,
        const std::string_view textureReference,
        const std::uint32_t seed)
    {
        const std::vector<
            const core::world::flora::FloraEcotype*>
            ecotypes =
                config.FindEcotypesByTexture(
                    textureReference);

        if (ecotypes.empty())
        {
            return nullptr;
        }

        float totalWeight =
            0.0f;

        for (const core::world::flora::FloraEcotype* ecotype :
             ecotypes)
        {
            if (ecotype == nullptr)
            {
                continue;
            }

            totalWeight +=
                EcotypeTextureWeight(
                    *ecotype,
                    textureReference);
        }

        if (totalWeight <=
            0.0f)
        {
            const std::size_t index =
                static_cast<std::size_t>(
                    HashMix(seed)) %
                ecotypes.size();

            return
                ecotypes[index];
        }

        float selected =
            HashFloat(seed) *
            totalWeight;

        for (const core::world::flora::FloraEcotype* ecotype :
             ecotypes)
        {
            if (ecotype == nullptr)
            {
                continue;
            }

            const float weight =
                EcotypeTextureWeight(
                    *ecotype,
                    textureReference);

            if (weight <=
                0.0f)
            {
                continue;
            }

            if (selected <
                weight)
            {
                return ecotype;
            }

            selected -=
                weight;
        }

        return
            ecotypes.back();
    }

    bool TextureMatchesEcotype(
        const core::world::flora::FloraConfig& config,
        const std::string_view texture,
        const std::string_view ecotypeName)
    {
        const std::vector<
            const core::world::flora::FloraEcotype*>
            matches =
                config.FindEcotypesByTexture(
                    texture);

        for (const core::world::flora::FloraEcotype* match :
             matches)
        {
            if (match != nullptr &&
                match->name ==
                    ecotypeName)
            {
                return true;
            }
        }

        return false;
    }

    const core::world::flora::FloraGeneratorRule*
    SelectGenerator(
        const core::world::flora::FloraEcotype& ecotype,
        const float localX,
        const float localZ,
        const std::uint32_t seed) noexcept
    {
        const core::world::flora::FloraGeneratorRule*
            selected =
                nullptr;

        float bestScore =
            -1.0f;

        for (std::size_t generatorIndex = 0;
             generatorIndex <
                ecotype.generators.size();
             ++generatorIndex)
        {
            const core::world::flora::FloraGeneratorRule&
                generator =
                    ecotype.generators[
                        generatorIndex];

            if (generator.visuals.empty())
            {
                continue;
            }

            const float frequency =
                std::max(
                    std::abs(
                        generator.frequency),
                    0.001f);

            const std::int32_t noiseX =
                static_cast<std::int32_t>(
                    std::floor(
                        localX *
                        frequency));

            const std::int32_t noiseZ =
                static_cast<std::int32_t>(
                    std::floor(
                        localZ *
                        frequency));

            std::uint32_t value =
                HashCombine(
                    seed,
                    static_cast<std::uint32_t>(
                        generatorIndex));

            value =
                HashCombine(
                    value,
                    static_cast<std::uint32_t>(
                        noiseX));

            value =
                HashCombine(
                    value,
                    static_cast<std::uint32_t>(
                        noiseZ));

            const float score =
                HashFloat(
                    value);

            if (score >
                bestScore)
            {
                bestScore =
                    score;

                selected =
                    &generator;
            }
        }

        return selected;
    }

    const core::world::flora::FloraVisualRule*
    SelectVisual(
        const core::world::flora::FloraGeneratorRule& generator,
        const std::uint32_t seed) noexcept
    {
        if (generator.visuals.empty())
        {
            return nullptr;
        }

        const std::size_t index =
            static_cast<std::size_t>(
                HashMix(seed)) %
            generator.visuals.size();

        return
            &generator.visuals[index];
    }

    std::uint32_t ResolveSpawnCount(
        const float density,
        const std::uint32_t seed) noexcept
    {
        if (density <=
            0.0f)
        {
            return 0;
        }

        const float limitedDensity =
            std::min(
                density,
                static_cast<float>(
                    MaximumSpawnPerCell));

        std::uint32_t count =
            static_cast<std::uint32_t>(
                std::floor(
                    limitedDensity));

        const float fraction =
            limitedDensity -
            static_cast<float>(
                count);

        if (fraction > 0.0f &&
            HashFloat(seed) <
                fraction)
        {
            ++count;
        }

        return
            std::min(
                count,
                MaximumSpawnPerCell);
    }

    float ResolveScale(
        const float variation,
        const std::uint32_t seed) noexcept
    {
        if (variation <=
            0.0f)
        {
            return 1.0f;
        }

        const float random =
            HashFloat(seed) *
                2.0f -
            1.0f;

        return
            std::max(
                1.0f +
                    random *
                    variation *
                    0.25f,
                0.25f);
    }

    core::math::Transform3x4
    BuildLocalTransform(
        const float x,
        const float y,
        const float z,
        const float rotation,
        const float scale) noexcept
    {
        const float cosine =
            std::cos(
                rotation);

        const float sine =
            std::sin(
                rotation);

        core::math::Transform3x4 transform;

        transform.values[0] =
            cosine * scale;

        transform.values[1] =
            0.0f;

        transform.values[2] =
            -sine * scale;

        transform.values[3] =
            0.0f;

        transform.values[4] =
            scale;

        transform.values[5] =
            0.0f;

        transform.values[6] =
            sine * scale;

        transform.values[7] =
            0.0f;

        transform.values[8] =
            cosine * scale;

        transform.values[9] =
            x;

        transform.values[10] =
            y;

        transform.values[11] =
            z;

        return transform;
    }
}

namespace core::world::flora
{
    bool FloraInstanceBuilder::Build(
        const std::string_view chunkId,
        const TerrainHeightData& heightData,
        const std::vector<TerrainLayerData>& layers,
        const TerrainAuxiliaryData& auxiliary,
        const math::Transform3x4& terrainTransform,
        const FloraConfig& config,
        std::vector<FloraInstance>& output,
        std::string& error) const
    {
        output.clear();
        error.clear();

        const TerrainDominantTextureData& dominant =
            auxiliary.dominantTextures;

        const bool hasLayerData =
            std::any_of(
                layers.begin(),
                layers.end(),
                [](
                    const TerrainLayerData& layer)
                {
                    return
                        layer.width != 0 &&
                        layer.height != 0 &&
                        !layer.blend.empty();
                });

        const bool hasDominantData =
            dominant.present &&
            dominant.width != 0 &&
            dominant.height != 0 &&
            !dominant.textureReferences.empty();

        if (!hasLayerData &&
            !hasDominantData)
        {
            return true;
        }

        if (dominant.present)
        {
            if (dominant.width == 0 ||
                dominant.height == 0)
            {
                error =
                    "Flora dominant texture dimensions are invalid.";

                return false;
            }

            const std::size_t expectedCells =
                static_cast<std::size_t>(
                    dominant.width) *
                dominant.height;

            if (dominant.indices.size() <
                expectedCells)
            {
                error =
                    "Flora dominant texture map is truncated.";

                return false;
            }
        }

        if (heightData.VisibleWidth() < 2 ||
            heightData.VisibleHeight() < 2)
        {
            error =
                "Flora terrain height map is invalid.";

            return false;
        }

        const std::uint32_t chunkSeed =
            HashString(
                chunkId);

        const std::uint32_t cellsPerAxis =
            static_cast<std::uint32_t>(
                std::ceil(
                    TerrainBlockSize /
                    PlacementStep));

        for (std::uint32_t gridZ = 0;
             gridZ < cellsPerAxis;
             ++gridZ)
        {
            for (std::uint32_t gridX = 0;
                 gridX < cellsPerAxis;
                 ++gridX)
            {
                std::uint32_t candidateSeed =
                    HashCombine(
                        chunkSeed,
                        gridX);

                candidateSeed =
                    HashCombine(
                        candidateSeed,
                        gridZ);

                const float jitterX =
                    (
                        HashFloat(
                            HashCombine(
                                candidateSeed,
                                0x13579BDFu)) *
                            2.0f -
                        1.0f
                    ) *
                    PlacementStep *
                    0.35f;

                const float jitterZ =
                    (
                        HashFloat(
                            HashCombine(
                                candidateSeed,
                                0x2468ACE0u)) *
                            2.0f -
                        1.0f
                    ) *
                    PlacementStep *
                    0.35f;

                const float localX =
                    ClampTerrainCoordinate(
                        (
                            static_cast<float>(
                                gridX) +
                            0.5f
                        ) *
                            PlacementStep +
                        jitterX);

                const float localZ =
                    ClampTerrainCoordinate(
                        (
                            static_cast<float>(
                                gridZ) +
                            0.5f
                        ) *
                            PlacementStep +
                        jitterZ);

                if (IsTerrainHole(
                        auxiliary.holes,
                        localX,
                        localZ))
                {
                    continue;
                }

                const std::string_view terrainTexture =
                    SelectTerrainTexture(
                        layers,
                        dominant,
                        localX,
                        localZ,
                        HashCombine(
                            candidateSeed,
                            0x93A4D21Fu));

                if (terrainTexture.empty())
                {
                    continue;
                }

                const FloraEcotype* ecotype =
                    SelectEcotype(
                        config,
                        terrainTexture,
                        HashCombine(
                            candidateSeed,
                            0xA511E9B3u));

                if (ecotype == nullptr ||
                    ecotype->generators.empty())
                {
                    continue;
                }

                const FloraGeneratorRule* generator =
                    SelectGenerator(
                        *ecotype,
                        localX,
                        localZ,
                        candidateSeed);

                if (generator == nullptr)
                {
                    continue;
                }

                const FloraVisualRule* visual =
                    SelectVisual(
                        *generator,
                        HashCombine(
                            candidateSeed,
                            0x6C8E9CF5u));

                if (visual == nullptr ||
                    visual->visualReference.empty())
                {
                    continue;
                }

                const std::uint32_t spawnCount =
                    ResolveSpawnCount(
                        visual->density,
                        HashCombine(
                            candidateSeed,
                            0xB5297A4Du));

                for (std::uint32_t spawnIndex = 0;
                     spawnIndex < spawnCount;
                     ++spawnIndex)
                {
                    const std::uint32_t spawnSeed =
                        HashCombine(
                            candidateSeed,
                            spawnIndex + 1u);

                    const float offsetX =
                        (
                            HashFloat(
                                HashCombine(
                                    spawnSeed,
                                    0x68E31DA4u)) *
                                2.0f -
                            1.0f
                        ) *
                        PlacementStep *
                        0.4f;

                    const float offsetZ =
                        (
                            HashFloat(
                                HashCombine(
                                    spawnSeed,
                                    0x1B56C4E9u)) *
                                2.0f -
                            1.0f
                        ) *
                        PlacementStep *
                        0.4f;

                    const float spawnX =
                        ClampTerrainCoordinate(
                            localX +
                            offsetX);

                    const float spawnZ =
                        ClampTerrainCoordinate(
                            localZ +
                            offsetZ);

                    if (IsTerrainHole(
                            auxiliary.holes,
                            spawnX,
                            spawnZ))
                    {
                        continue;
                    }

                    const std::string_view spawnTexture =
                        SelectTerrainTexture(
                            layers,
                            dominant,
                            spawnX,
                            spawnZ,
                            HashCombine(
                                spawnSeed,
                                0xC2B2AE35u));

                    if (spawnTexture.empty() ||
                        !TextureMatchesEcotype(
                            config,
                            spawnTexture,
                            ecotype->name))
                    {
                        continue;
                    }

                    const float height =
                        SampleTerrainHeight(
                            heightData,
                            spawnX,
                            spawnZ);

                    const float rotation =
                        HashFloat(
                            HashCombine(
                                spawnSeed,
                                0x9E3779B9u)) *
                        Pi *
                        2.0f;

                    const float scale =
                        ResolveScale(
                            visual->scaleVariation,
                            HashCombine(
                                spawnSeed,
                                0x85EBCA6Bu));

                    const math::Transform3x4 localTransform =
                        BuildLocalTransform(
                            spawnX,
                            height,
                            spawnZ,
                            rotation,
                            scale);

                    FloraInstance instance;

                    instance.chunkId =
                        std::string(
                            chunkId);

                    instance.ecotypeName =
                        ecotype->name;

                    instance.visualReference =
                        resources::ResourcePath::Normalize(
                            visual->visualReference);

                    instance.transform =
                        math::Transform3x4::Multiply(
                            localTransform,
                            terrainTransform);

                    output.push_back(
                        std::move(
                            instance));
                }
            }
        }

        return true;
    }
}