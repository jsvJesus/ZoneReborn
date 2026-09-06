#include "Core/World/Flora/FloraConfigLoader.h"

#include "Core/Resources/DataSection.h"
#include "Core/Resources/PackedSectionReader.h"

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace
{
    constexpr std::string_view FloraConfigPath =
        "res/environments/flora.xml";

    bool TryReadFloat(
        const core::resources::DataSection& section,
        float& output)
    {
        if (section.TryGetFloat(
                output))
        {
            return true;
        }

        const std::string* text =
            section.AsString();

        if (text == nullptr ||
            text->empty())
        {
            return false;
        }

        const char* begin =
            text->data();

        const char* end =
            begin +
            text->size();

        const auto result =
            std::from_chars(
                begin,
                end,
                output);

        return
            result.ec ==
                std::errc{} &&
            result.ptr ==
                end;
    }

    float ReadFloat(
        const core::resources::DataSection& section,
        const std::string_view childName,
        const float fallback)
    {
        const core::resources::DataSection* child =
            section.FindChild(
                childName);

        if (child == nullptr)
        {
            return fallback;
        }

        float value =
            fallback;

        if (!TryReadFloat(
                *child,
                value))
        {
            return fallback;
        }

        return value;
    }

    std::uint32_t ReadUInt32(
        const core::resources::DataSection& root,
        const std::string_view childName,
        const std::uint32_t fallback)
    {
        const core::resources::DataSection* child =
            root.FindChild(
                childName);

        if (child == nullptr)
        {
            return fallback;
        }

        if (const std::int64_t* integer =
                child->AsInteger())
        {
            if (*integer >=
                    0 &&
                *integer <=
                    0xFFFFFFFFll)
            {
                return
                    static_cast<std::uint32_t>(
                        *integer);
            }
        }

        float value =
            0.0f;

        if (!TryReadFloat(
                *child,
                value) ||
            value <
                0.0f)
        {
            return fallback;
        }

        return
            static_cast<std::uint32_t>(
                value);
    }

    std::size_t ReadSize(
        const core::resources::DataSection& root,
        const std::string_view childName,
        const std::size_t fallback)
    {
        const core::resources::DataSection* child =
            root.FindChild(
                childName);

        if (child == nullptr)
        {
            return fallback;
        }

        if (const std::int64_t* integer =
                child->AsInteger())
        {
            if (*integer >=
                0)
            {
                return
                    static_cast<std::size_t>(
                        *integer);
            }
        }

        float value =
            0.0f;

        if (!TryReadFloat(
                *child,
                value) ||
            value <
                0.0f)
        {
            return fallback;
        }

        return
            static_cast<std::size_t>(
                value);
    }

    std::string ReadString(
        const core::resources::DataSection& section,
        const std::string_view childName)
    {
        const core::resources::DataSection* child =
            section.FindChild(
                childName);

        if (child == nullptr)
        {
            return {};
        }

        const std::string* value =
            child->AsString();

        if (value == nullptr)
        {
            return {};
        }

        return
            *value;
    }

    void AddVisualGenerator(
        const core::resources::DataSection& section,
        const float frequency,
        std::vector<
            core::world::flora::FloraGeneratorRule>& output)
    {
        const std::vector<
            const core::resources::DataSection*>
            visualSections =
                section.FindChildren(
                    "visual");

        if (visualSections.empty())
        {
            return;
        }

        core::world::flora::FloraGeneratorRule
            generator;

        generator.frequency =
            frequency;

        const float generatorDensity =
            ReadFloat(
                section,
                "density",
                1.0f);

        const float generatorScaleVariation =
            ReadFloat(
                section,
                "scaleVariation",
                0.0f);

        const float generatorFlex =
            ReadFloat(
                section,
                "flex",
                1.0f);

        generator.visuals.reserve(
            visualSections.size());

        for (const core::resources::DataSection* visualSection :
             visualSections)
        {
            if (visualSection ==
                nullptr)
            {
                continue;
            }

            const std::string* reference =
                visualSection->AsString();

            if (reference ==
                    nullptr ||
                reference->empty())
            {
                continue;
            }

            core::world::flora::FloraVisualRule
                visual;

            visual.visualReference =
                *reference;

            visual.density =
                ReadFloat(
                    *visualSection,
                    "density",
                    generatorDensity);

            visual.scaleVariation =
                ReadFloat(
                    *visualSection,
                    "scaleVariation",
                    generatorScaleVariation);

            visual.flex =
                ReadFloat(
                    *visualSection,
                    "flex",
                    generatorFlex);

            generator.visuals.push_back(
                std::move(
                    visual));
        }

        if (!generator.visuals.empty())
        {
            output.push_back(
                std::move(
                    generator));
        }
    }

    void CollectGenerators(
        const core::resources::DataSection& section,
        const float inheritedFrequency,
        std::vector<
            core::world::flora::FloraGeneratorRule>& output)
    {
        const float frequency =
            ReadFloat(
                section,
                "frequency",
                inheritedFrequency);

        if (section.name ==
            "generator")
        {
            const std::string* type =
                section.AsString();

            if (type != nullptr &&
                *type ==
                    "visual")
            {
                AddVisualGenerator(
                    section,
                    frequency,
                    output);
            }
        }

        for (const core::resources::DataSection& child :
             section.children)
        {
            if (child.name ==
                "visual")
            {
                continue;
            }

            CollectGenerators(
                child,
                frequency,
                output);
        }
    }

    bool ParseEcotype(
        const core::resources::DataSection& section,
        core::world::flora::FloraEcotype& output)
    {
        output = {};

        output.name =
            section.name;

        output.soundTag =
            ReadString(
                section,
                "sound_tag");

        const std::vector<
            const core::resources::DataSection*>
            textures =
                section.FindChildren(
                    "texture");

        output.textures.reserve(
            textures.size());

        for (const core::resources::DataSection* textureSection :
             textures)
        {
            if (textureSection ==
                nullptr)
            {
                continue;
            }

            const std::string* reference =
                textureSection->AsString();

            if (reference ==
                    nullptr ||
                reference->empty())
            {
                continue;
            }

            core::world::flora::FloraTextureRule
                texture;

            texture.textureReference =
                *reference;

            texture.weight =
                ReadFloat(
                    *textureSection,
                    "weight",
                    1.0f);

            output.textures.push_back(
                std::move(
                    texture));
        }

        AddVisualGenerator(
            section,
            1.0f,
            output.generators);

        for (const core::resources::DataSection& child :
             section.children)
        {
            if (child.name ==
                    "visual" ||
                child.name ==
                    "texture")
            {
                continue;
            }

            CollectGenerators(
                child,
                1.0f,
                output.generators);
        }

        return true;
    }
}

namespace core::world::flora
{
    bool FloraConfigLoader::Load(
        const resources::ResourceFileSystem& resources,
        FloraConfig& output,
        std::string& error) const
    {
        output = {};
        error.clear();

        if (!resources.Exists(
                FloraConfigPath))
        {
            error =
                "SO flora configuration was not found: " +
                std::string(
                    FloraConfigPath);

            return false;
        }

        std::vector<std::byte>
            data;

        if (!resources.ReadBinary(
                FloraConfigPath,
                data))
        {
            error =
                "Unable to read SO flora configuration.";

            return false;
        }

        const std::span<const std::byte>
            encoded(
                data.data(),
                data.size());

        if (!resources::PackedSectionReader::HasSignature(
                encoded))
        {
            error =
                "SO flora configuration is not a packed section.";

            return false;
        }

        resources::PackedSectionReader
            reader;

        resources::DataSection
            root;

        if (!reader.Read(
                encoded,
                root,
                error))
        {
            error =
                "Unable to parse SO flora configuration: " +
                error;

            return false;
        }

        const resources::DataSection* ecotypes =
            root.FindChild(
                "ecotypes");

        if (ecotypes ==
            nullptr)
        {
            error =
                "SO flora configuration contains no ecotypes.";

            return false;
        }

        FloraConfig config;

        config.vertexBufferSize =
            ReadSize(
                root,
                "vb_size",
                0);

        config.textureWidth =
            ReadUInt32(
                root,
                "texture_width",
                0);

        config.textureHeight =
            ReadUInt32(
                root,
                "texture_height",
                0);

        config.alphaTestReference =
            ReadUInt32(
                root,
                "AlphaTestRef",
                0);

        config.shadowAlphaTestReference =
            ReadUInt32(
                root,
                "ShadowAlphaTestRef",
                0);

        config.alphaTestDistance =
            ReadFloat(
                root,
                "AlphaTestDistance",
                0.0f);

        config.alphaBlendDistance =
            ReadFloat(
                root,
                "AlphaBlendDistance",
                0.0f);

        config.alphaTestFadePercent =
            ReadFloat(
                root,
                "AlphaTestFadePercent",
                0.0f);

        config.alphaBlendFadePercent =
            ReadFloat(
                root,
                "AlphaBlendFadePercent",
                0.0f);

        config.ecotypes.reserve(
            ecotypes->children.size());

        for (const resources::DataSection& section :
             ecotypes->children)
        {
            FloraEcotype
                ecotype;

            if (!ParseEcotype(
                    section,
                    ecotype))
            {
                continue;
            }

            if (ecotype.name.empty())
            {
                continue;
            }

            config.ecotypes.push_back(
                std::move(
                    ecotype));
        }

        if (config.ecotypes.empty())
        {
            error =
                "SO flora configuration contains no valid ecotypes.";

            return false;
        }

        output =
            std::move(
                config);

        return true;
    }
}