#include "Character/CharacterFaceCatalog.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <regex>

namespace
{
    bool ExtractDelimited(
        const std::string_view text,
        const std::size_t start,
        const char open,
        const char close,
        std::string_view& output)
    {
        const std::size_t begin = text.find(open, start);
        if (begin == std::string_view::npos)
        {
            return false;
        }

        int depth = 0;
        bool inString = false;
        bool escaped = false;

        for (std::size_t index = begin; index < text.size(); ++index)
        {
            const char value = text[index];

            if (inString)
            {
                if (escaped)
                {
                    escaped = false;
                }
                else if (value == '\\')
                {
                    escaped = true;
                }
                else if (value == '"')
                {
                    inString = false;
                }
                continue;
            }

            if (value == '"')
            {
                inString = true;
            }
            else if (value == open)
            {
                ++depth;
            }
            else if (value == close && --depth == 0)
            {
                output = text.substr(begin, index - begin + 1);
                return true;
            }
        }

        return false;
    }

    std::size_t FindId(
        const std::string& text,
        const std::string_view id)
    {
        const std::regex pattern(
            "\"id\"\\s*:\\s*\"" + std::string(id) + "\"");
        std::smatch match;
        return std::regex_search(text, match, pattern)
            ? static_cast<std::size_t>(match.position())
            : std::string::npos;
    }

    bool ParseUnsigned(
        const std::string_view text,
        std::uint32_t& output,
        const int base = 10)
    {
        const auto parsed = std::from_chars(
            text.data(), text.data() + text.size(), output, base);
        return parsed.ec == std::errc();
    }
}

namespace client::character
{
    bool FaceCatalog::Load(
        const core::resources::ResourceFileSystem& resources,
        std::string& error)
    {
        Clear();
        error.clear();

        constexpr std::string_view Path =
            "res/scripts/common/data/charMakerFaceCfg.json";
        std::string text;

        if (!resources.ReadText(Path, text))
        {
            error = "Unable to read " + std::string(Path);
            return false;
        }

        constexpr std::array StyleGroups
        {
            "HairStyle",
            "MustacheStyle",
            "BeardStyle",
            "EyebrowsStyle"
        };
        const std::regex objectPattern("\\{[^{}]*\\}");
        const std::regex itemPattern(
            "\"item_id\"\\s*:\\s*(-?[0-9]+)");
        const std::regex weightPattern(
            "\"weight\"\\s*:\\s*([0-9]+)");
        const std::regex donorPattern(
            "\"donat_only\"\\s*:\\s*true");

        for (const std::string_view group : StyleGroups)
        {
            const std::size_t id = FindId(text, group);
            const std::size_t data = id == std::string::npos
                ? std::string::npos
                : text.find("\"data\"", id);
            std::string_view array;

            if (data == std::string::npos ||
                !ExtractDelimited(text, data, '[', ']', array))
            {
                error = "Invalid face style group: " + std::string(group);
                Clear();
                return false;
            }

            const std::string arrayText(array);

            for (std::sregex_iterator it(
                     arrayText.begin(), arrayText.end(), objectPattern),
                 end;
                 it != end;
                 ++it)
            {
                const std::string object = it->str();

                if (std::regex_search(object, donorPattern))
                {
                    continue;
                }

                std::smatch itemMatch;

                if (!std::regex_search(object, itemMatch, itemPattern))
                {
                    continue;
                }

                const std::string itemText = itemMatch[1].str();
                std::int32_t item = 0;
                const auto parsed = std::from_chars(
                    itemText.data(), itemText.data() + itemText.size(), item);

                if (parsed.ec != std::errc() || item < 0)
                {
                    continue;
                }

                std::uint32_t weight = 100;
                std::smatch weightMatch;

                if (std::regex_search(object, weightMatch, weightPattern))
                {
                    const std::string weightText = weightMatch[1].str();
                    ParseUnsigned(weightText, weight);
                }

                values_[std::string(group)].push_back(
                    {static_cast<std::uint32_t>(item), weight});
                styles_[std::string(group)].push_back(item);
            }

            if (styles_[std::string(group)].empty())
            {
                error = "Face style group has no usable choices: " +
                    std::string(group);
                Clear();
                return false;
            }
        }

        constexpr std::array ColourGroups
        {
            "SkinColor",
            "EyeColor",
            "HairColor"
        };
        const std::regex colourPattern(
            "\"([0-9a-fA-F]{6}):([0-9]+)\"");

        for (const std::string_view group : ColourGroups)
        {
            const std::size_t id = FindId(text, group);
            const std::size_t colours = id == std::string::npos
                ? std::string::npos
                : text.rfind("\"colors\"", id);
            std::string_view array;

            if (colours == std::string::npos ||
                !ExtractDelimited(text, colours, '[', ']', array))
            {
                error = "Invalid face colour group: " + std::string(group);
                Clear();
                return false;
            }

            const std::string arrayText(array);

            for (std::sregex_iterator it(
                     arrayText.begin(), arrayText.end(), colourPattern),
                 end;
                 it != end;
                 ++it)
            {
                const std::string colourText = (*it)[1].str();
                const std::string weightText = (*it)[2].str();
                std::uint32_t colour = 0;
                std::uint32_t weight = 100;

                if (ParseUnsigned(colourText, colour, 16) &&
                    ParseUnsigned(weightText, weight))
                {
                    values_[std::string(group)].push_back({colour, weight});
                }
            }

            if (values_[std::string(group)].empty())
            {
                error = "Face colour group has no usable choices: " +
                    std::string(group);
                Clear();
                return false;
            }
        }

        return true;
    }

    void FaceCatalog::Clear()
    {
        values_.clear();
        styles_.clear();
    }

    bool FaceCatalog::IsStyleAllowed(
        const std::string_view group,
        const std::int32_t value) const noexcept
    {
        if (group == "TatooStyle")
        {
            return value == 0;
        }

        const auto found = styles_.find(std::string(group));

        return found != styles_.end() &&
            std::find(found->second.begin(), found->second.end(), value) !=
                found->second.end();
    }

    bool FaceCatalog::IsColourAllowed(
        const std::string_view group,
        const std::uint32_t value) const noexcept
    {
        if (group == "TatooColor")
        {
            return value == 0;
        }

        const auto found = values_.find(std::string(group));

        return found != values_.end() &&
            std::find_if(
                found->second.begin(),
                found->second.end(),
                [value](const FaceWeightedValue& item)
                {
                    return item.value == value;
                }) != found->second.end();
    }

    std::uint32_t FaceCatalog::RandomWeighted(
        const std::string_view group,
        std::mt19937& random) const
    {
        const auto found = values_.find(std::string(group));

        if (found == values_.end() || found->second.empty())
        {
            return 0;
        }

        std::uint64_t total = 0;

        for (const FaceWeightedValue& item : found->second)
        {
            total += std::max(item.weight, 1u);
        }

        std::uniform_int_distribution<std::uint64_t> distribution(1, total);
        std::uint64_t selected = distribution(random);

        for (const FaceWeightedValue& item : found->second)
        {
            const std::uint32_t weight = std::max(item.weight, 1u);

            if (selected <= weight)
            {
                return item.value;
            }

            selected -= weight;
        }

        return found->second.back().value;
    }

    const std::vector<std::int32_t>& FaceCatalog::Styles(
        const std::string_view group) const noexcept
    {
        static const std::vector<std::int32_t> Empty;
        const auto found = styles_.find(std::string(group));
        return found == styles_.end() ? Empty : found->second;
    }

    std::size_t FaceCatalog::GroupCount() const noexcept
    {
        return values_.size();
    }
}
