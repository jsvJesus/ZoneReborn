#include "Character/CharacterCatalog.h"

#include "Core/Log.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    std::size_t SkipWhitespace(
        const std::string_view text,
        std::size_t position) noexcept
    {
        while (position < text.size() &&
               std::isspace(
                   static_cast<unsigned char>(
                       text[position])) != 0)
        {
            ++position;
        }

        return position;
    }

    bool ReadInteger(
        const std::string_view text,
        std::size_t& position,
        std::int32_t& output)
    {
        position =
            SkipWhitespace(
                text,
                position);

        if (position >= text.size())
        {
            return false;
        }

        const char* begin =
            text.data() +
            position;

        const char* end =
            text.data() +
            text.size();

        const auto result =
            std::from_chars(
                begin,
                end,
                output);

        if (result.ec !=
            std::errc())
        {
            return false;
        }

        position =
            static_cast<std::size_t>(
                result.ptr -
                text.data());

        return true;
    }

    bool ReadFloatValue(
        const std::string_view text,
        std::size_t& position,
        float& output)
    {
        position = SkipWhitespace(text, position);

        if (position >= text.size())
        {
            return false;
        }

        const auto result = std::from_chars(
            text.data() + position,
            text.data() + text.size(),
            output);

        if (result.ec != std::errc())
        {
            return false;
        }

        position = static_cast<std::size_t>(result.ptr - text.data());
        return true;
    }

    bool ReadQuotedString(
        const std::string_view text,
        std::size_t& position,
        std::string& output)
    {
        output.clear();

        position =
            SkipWhitespace(
                text,
                position);

        if (position < text.size() &&
            text[position] == 'u' &&
            position + 1 < text.size() &&
            (
                text[position + 1] == '"' ||
                text[position + 1] == '\''
            ))
        {
            ++position;
        }

        if (position >= text.size())
        {
            return false;
        }

        const char quote =
            text[position];

        if (quote != '"' &&
            quote != '\'')
        {
            return false;
        }

        ++position;

        bool escaped =
            false;

        while (position < text.size())
        {
            const char value =
                text[position++];

            if (escaped)
            {
                output.push_back(
                    value);

                escaped =
                    false;

                continue;
            }

            if (value == '\\')
            {
                escaped =
                    true;

                continue;
            }

            if (value == quote)
            {
                return true;
            }

            output.push_back(
                value);
        }

        output.clear();

        return false;
    }

    std::size_t FindKeyValue(
        const std::string_view block,
        const std::string_view key)
    {
        std::string quotedKey;

        quotedKey.reserve(
            key.size() +
            2);

        quotedKey.push_back('"');
        quotedKey.append(key);
        quotedKey.push_back('"');

        const std::size_t keyPosition =
            block.find(
                quotedKey);

        if (keyPosition ==
            std::string_view::npos)
        {
            return
                std::string_view::npos;
        }

        const std::size_t colon =
            block.find(
                ':',
                keyPosition +
                quotedKey.size());

        if (colon ==
            std::string_view::npos)
        {
            return
                std::string_view::npos;
        }

        return
            SkipWhitespace(
                block,
                colon + 1);
    }

    bool ExtractDelimited(
        const std::string_view text,
        const std::size_t beginPosition,
        const char open,
        const char close,
        std::string_view& output)
    {
        output = {};

        std::size_t begin =
            SkipWhitespace(
                text,
                beginPosition);

        if (begin >= text.size() ||
            text[begin] != open)
        {
            return false;
        }

        std::int32_t depth =
            0;

        bool inString =
            false;

        bool escaped =
            false;

        char quote =
            0;

        for (std::size_t index = begin;
             index < text.size();
             ++index)
        {
            const char value =
                text[index];

            if (inString)
            {
                if (escaped)
                {
                    escaped =
                        false;

                    continue;
                }

                if (value == '\\')
                {
                    escaped =
                        true;

                    continue;
                }

                if (value == quote)
                {
                    inString =
                        false;
                }

                continue;
            }

            if (value == '"' ||
                value == '\'')
            {
                inString =
                    true;

                quote =
                    value;

                continue;
            }

            if (value == open)
            {
                ++depth;

                continue;
            }

            if (value == close)
            {
                --depth;

                if (depth == 0)
                {
                    output =
                        text.substr(
                            begin,
                            index -
                            begin +
                            1);

                    return true;
                }
            }
        }

        return false;
    }

    std::vector<std::string_view>
    ExtractTopLevelObjects(
        const std::string_view text)
    {
        std::vector<std::string_view>
            result;

        std::size_t objectBegin =
            std::string_view::npos;

        std::int32_t depth =
            0;

        bool inString =
            false;

        bool escaped =
            false;

        char quote =
            0;

        for (std::size_t index = 0;
             index < text.size();
             ++index)
        {
            const char value =
                text[index];

            if (inString)
            {
                if (escaped)
                {
                    escaped =
                        false;

                    continue;
                }

                if (value == '\\')
                {
                    escaped =
                        true;

                    continue;
                }

                if (value == quote)
                {
                    inString =
                        false;
                }

                continue;
            }

            if (value == '"' ||
                value == '\'')
            {
                inString =
                    true;

                quote =
                    value;

                continue;
            }

            if (value == '{')
            {
                if (depth == 0)
                {
                    objectBegin =
                        index;
                }

                ++depth;

                continue;
            }

            if (value == '}')
            {
                if (depth <= 0)
                {
                    continue;
                }

                --depth;

                if (depth == 0 &&
                    objectBegin !=
                        std::string_view::npos)
                {
                    result.push_back(
                        text.substr(
                            objectBegin,
                            index -
                            objectBegin +
                            1));

                    objectBegin =
                        std::string_view::npos;
                }
            }
        }

        return result;
    }

    bool ParseIntegerField(
        const std::string_view block,
        const std::string_view key,
        std::int32_t& output)
    {
        const std::size_t position =
            FindKeyValue(
                block,
                key);

        if (position ==
            std::string_view::npos)
        {
            return false;
        }

        std::size_t cursor =
            position;

        return ReadInteger(
            block,
            cursor,
            output);
    }

    bool ParseStringField(
        const std::string_view block,
        const std::string_view key,
        std::string& output)
    {
        const std::size_t position =
            FindKeyValue(
                block,
                key);

        if (position ==
            std::string_view::npos)
        {
            return false;
        }

        std::size_t cursor =
            position;

        return ReadQuotedString(
            block,
            cursor,
            output);
    }

    std::vector<std::string>
    ParseModelStrings(
        const std::string_view text)
    {
        std::vector<std::string>
            result;

        std::size_t position =
            0;

        while (position <
               text.size())
        {
            if (text[position] != '"' &&
                text[position] != '\'' &&
                !(
                    text[position] == 'u' &&
                    position + 1 <
                        text.size() &&
                    (
                        text[position + 1] == '"' ||
                        text[position + 1] == '\''
                    )
                ))
            {
                ++position;

                continue;
            }

            std::string value;

            if (!ReadQuotedString(
                    text,
                    position,
                    value))
            {
                ++position;

                continue;
            }

            if (value.size() >= 6 &&
                value.ends_with(".model"))
            {
                result.push_back(
                    std::move(value));
            }
        }

        return result;
    }

    std::vector<client::character::Slot>
    ParseSlots(
        const std::string_view block)
    {
        std::vector<client::character::Slot>
            result;

        const std::size_t valuePosition =
            FindKeyValue(
                block,
                "SlotType");

        if (valuePosition ==
            std::string_view::npos)
        {
            return result;
        }

        std::string_view section;

        if (!ExtractDelimited(
                block,
                valuePosition,
                '[',
                ']',
                section))
        {
            return result;
        }

        std::size_t position =
            0;

        while (position <
               section.size())
        {
            if (!std::isdigit(
                    static_cast<unsigned char>(
                        section[position])) &&
                section[position] != '-')
            {
                ++position;

                continue;
            }

            std::int32_t value =
                0;

            if (!ReadInteger(
                    section,
                    position,
                    value))
            {
                ++position;

                continue;
            }

            const auto slot =
                client::character::
                    ToSlot(value);

            if (slot.has_value())
            {
                result.push_back(
                    *slot);
            }
        }

        return result;
    }

    std::vector<std::string>
    ParseDefaultModels(
        const std::string_view block)
    {
        const std::size_t valuePosition =
            FindKeyValue(
                block,
                "ModelNames");

        if (valuePosition ==
            std::string_view::npos)
        {
            return {};
        }

        std::string_view section;

        if (!ExtractDelimited(
                block,
                valuePosition,
                '{',
                '}',
                section))
        {
            return {};
        }

        return
            ParseModelStrings(
                section);
    }

    std::vector<client::character::SlotOverride>
    ParseOverrides(
        const std::string_view block)
    {
        using client::character::SlotOverride;

        std::vector<SlotOverride>
            result;

        const std::size_t valuePosition =
            FindKeyValue(
                block,
                "OverrideSlots");

        if (valuePosition ==
            std::string_view::npos)
        {
            return result;
        }

        std::string_view section;

        if (!ExtractDelimited(
                block,
                valuePosition,
                '[',
                ']',
                section))
        {
            return result;
        }

        std::size_t position =
            0;

        while (position <
               section.size())
        {
            const std::size_t tupleBegin =
                section.find(
                    '(',
                    position);

            if (tupleBegin ==
                std::string_view::npos)
            {
                break;
            }

            std::string_view tuple;

            if (!ExtractDelimited(
                    section,
                    tupleBegin,
                    '(',
                    ')',
                    tuple))
            {
                break;
            }

            std::size_t cursor =
                1;

            std::int32_t slotValue =
                0;

            if (!ReadInteger(
                    tuple,
                    cursor,
                    slotValue))
            {
                position =
                    tupleBegin +
                    tuple.size();

                continue;
            }

            const auto slot =
                client::character::
                    ToSlot(slotValue);

            if (!slot.has_value())
            {
                position =
                    tupleBegin +
                    tuple.size();

                continue;
            }

            const std::size_t comma =
                tuple.find(
                    ',',
                    cursor);

            if (comma ==
                std::string_view::npos)
            {
                position =
                    tupleBegin +
                    tuple.size();

                continue;
            }

            cursor =
                SkipWhitespace(
                    tuple,
                    comma + 1);

            SlotOverride value;

            value.slot =
                *slot;

            if (tuple.substr(
                    cursor,
                    4) ==
                "None")
            {
                value.hide =
                    true;
            }
            else
            {
                std::string tag;

                if (ReadQuotedString(
                        tuple,
                        cursor,
                        tag))
                {
                    value.tag =
                        std::move(tag);
                }
                else
                {
                    std::int32_t integerTag =
                        0;

                    if (ReadInteger(
                            tuple,
                            cursor,
                            integerTag))
                    {
                        value.tag =
                            "#" +
                            std::to_string(
                                integerTag);
                    }
                }
            }

            result.push_back(
                std::move(value));

            position =
                tupleBegin +
                tuple.size();
        }

        return result;
    }

    std::vector<client::character::ModelAlternative>
    ParseAlternatives(
        const std::string_view block)
    {
        using client::character::ModelAlternative;

        std::vector<ModelAlternative>
            result;

        const std::size_t valuePosition =
            FindKeyValue(
                block,
                "AlternativeModelsList");

        if (valuePosition ==
            std::string_view::npos)
        {
            return result;
        }

        std::string_view section;

        if (!ExtractDelimited(
                block,
                valuePosition,
                '[',
                ']',
                section))
        {
            return result;
        }

        std::size_t position =
            0;

        while (position <
               section.size())
        {
            const std::size_t tupleBegin =
                section.find(
                    '(',
                    position);

            if (tupleBegin ==
                std::string_view::npos)
            {
                break;
            }

            std::string_view tuple;

            if (!ExtractDelimited(
                    section,
                    tupleBegin,
                    '(',
                    ')',
                    tuple))
            {
                break;
            }

            std::size_t cursor =
                1;

            cursor =
                SkipWhitespace(
                    tuple,
                    cursor);

            ModelAlternative alternative;

            if (cursor < tuple.size() &&
                (
                    tuple[cursor] == '"' ||
                    tuple[cursor] == '\'' ||
                    tuple[cursor] == 'u'
                ))
            {
                ReadQuotedString(
                    tuple,
                    cursor,
                    alternative.tag);
            }
            else
            {
                std::int32_t integerTag =
                    0;

                if (ReadInteger(
                        tuple,
                        cursor,
                        integerTag))
                {
                    alternative.tag =
                        "#" +
                        std::to_string(
                            integerTag);
                }
            }

            alternative.models =
                ParseModelStrings(
                    tuple);

            result.push_back(
                std::move(
                    alternative));

            position =
                tupleBegin +
                tuple.size();
        }

        return result;
    }

    bool ParseBooleanField(
        const std::string_view block,
        const std::string_view key,
        const bool fallback)
    {
        const std::size_t position =
            FindKeyValue(
                block,
                key);

        if (position ==
            std::string_view::npos)
        {
            return fallback;
        }

        if (block.substr(
                position,
                4) ==
            "True")
        {
            return true;
        }

        if (block.substr(
                position,
                5) ==
            "False")
        {
            return false;
        }

        return fallback;
    }

    bool ParseTintMaterial(
        const std::string_view block,
        std::string& output)
    {
        const std::size_t valuePosition = FindKeyValue(block, "Tint");
        std::string_view section;

        if (valuePosition == std::string_view::npos ||
            !ExtractDelimited(block, valuePosition, '{', '}', section))
        {
            return false;
        }

        const std::size_t colon = section.find(':');

        if (colon == std::string_view::npos)
        {
            return false;
        }

        std::size_t cursor = colon + 1;
        return ReadQuotedString(section, cursor, output);
    }

    bool ParseLengthLimits(
        const std::string_view block,
        std::array<float, 2>& output)
    {
        const std::size_t valuePosition = FindKeyValue(block, "LengthLimits");
        std::string_view section;

        if (valuePosition == std::string_view::npos ||
            !ExtractDelimited(block, valuePosition, '(', ')', section))
        {
            return false;
        }

        std::size_t cursor = 1;

        if (!ReadFloatValue(section, cursor, output[0]))
        {
            return false;
        }

        const std::size_t comma = section.find(',', cursor);

        if (comma == std::string_view::npos)
        {
            return false;
        }

        cursor = comma + 1;
        return ReadFloatValue(section, cursor, output[1]);
    }

    std::vector<
        std::pair<
            std::string,
            std::string_view>>
    ExtractJsonTopLevelArrays(
        const std::string_view text)
    {
        std::vector<
            std::pair<
                std::string,
                std::string_view>>
            result;

        std::size_t position =
            text.find('{');

        if (position ==
            std::string_view::npos)
        {
            return result;
        }

        ++position;

        while (position <
               text.size())
        {
            position =
                SkipWhitespace(
                    text,
                    position);

            while (position <
                   text.size() &&
                   (
                       text[position] == ',' ||
                       text[position] == '\r' ||
                       text[position] == '\n'
                   ))
            {
                ++position;

                position =
                    SkipWhitespace(
                        text,
                        position);
            }

            if (position >=
                    text.size() ||
                text[position] == '}')
            {
                break;
            }

            std::string key;

            if (!ReadQuotedString(
                    text,
                    position,
                    key))
            {
                ++position;

                continue;
            }

            const std::size_t colon =
                text.find(
                    ':',
                    position);

            if (colon ==
                std::string_view::npos)
            {
                break;
            }

            std::string_view array;

            if (!ExtractDelimited(
                    text,
                    colon + 1,
                    '[',
                    ']',
                    array))
            {
                break;
            }

            result.emplace_back(
                std::move(key),
                array);

            position =
                static_cast<std::size_t>(
                    array.data() -
                    text.data()) +
                array.size();
        }

        return result;
    }
}

namespace client::character
{
    bool Catalog::Load(
        const core::resources::ResourceFileSystem& resources,
        std::string& error)
    {
        Clear();

        if (!LoadItems(
                resources,
                error))
        {
            Clear();

            return false;
        }

        if (!LoadCreator(
                resources,
                error))
        {
            Clear();

            return false;
        }

        if (!faceCatalog_.Load(
                resources,
                error))
        {
            Clear();
            return false;
        }

        core::Log::Info(
            std::string(
                "Character catalog loaded: items=") +
            std::to_string(
                items_.size()) +
            ", creator groups=" +
            std::to_string(
                creatorGroups_.size()) +
            ", face groups=" +
            std::to_string(
                faceCatalog_.GroupCount()));

        return true;
    }

    void Catalog::Clear()
    {
        items_.clear();
        creatorGroups_.clear();
        faceCatalog_.Clear();
    }

    const ItemDefinition* Catalog::Find(
        const std::int32_t typeId) const noexcept
    {
        const auto found =
            items_.find(
                typeId);

        if (found ==
            items_.end())
        {
            return nullptr;
        }

        return
            &found->second;
    }

    const CreatorGroup* Catalog::FindCreatorGroup(
        const std::string_view name) const noexcept
    {
        const auto found =
            std::find_if(
                creatorGroups_.begin(),
                creatorGroups_.end(),
                [name](
                    const CreatorGroup& group)
                {
                    return
                        group.name ==
                        name;
                });

        if (found ==
            creatorGroups_.end())
        {
            return nullptr;
        }

        return
            &*found;
    }

    const std::vector<CreatorGroup>&
    Catalog::CreatorGroups() const noexcept
    {
        return
            creatorGroups_;
    }

    const FaceCatalog& Catalog::Faces() const noexcept
    {
        return faceCatalog_;
    }

    std::size_t Catalog::ItemCount() const noexcept
    {
        return
            items_.size();
    }

    bool Catalog::LoadItems(
        const core::resources::ResourceFileSystem& resources,
        std::string& error)
    {
        constexpr std::string_view Path =
            "res/scripts/common/data/items_pyson/CLOTH.pyson";

        std::string text;

        if (!resources.ReadText(
                Path,
                text))
        {
            error =
                "Unable to read " +
                std::string(Path);

            return false;
        }

        for (const std::string_view block :
             ExtractTopLevelObjects(text))
        {
            ItemDefinition definition;

            if (!ParseIntegerField(
                    block,
                    "TypeID",
                    definition.typeId))
            {
                continue;
            }

            ParseStringField(
                block,
                "_script_name_",
                definition.scriptName);

            definition.slots =
                ParseSlots(
                    block);

            definition.models =
                ParseDefaultModels(
                    block);

            definition.overrides =
                ParseOverrides(
                    block);

            definition.alternatives =
                ParseAlternatives(
                    block);

            definition.fixRollLeftHand =
                ParseBooleanField(
                    block,
                    "FixRollLeftHand",
                    false);

            ParseStringField(
                block,
                "Substrate",
                definition.substrate);

            ParseTintMaterial(
                block,
                definition.tintMaterial);

            definition.hasLengthLimits =
                ParseLengthLimits(
                    block,
                    definition.lengthLimits);

            items_.insert_or_assign(
                definition.typeId,
                std::move(
                    definition));
        }

        if (items_.empty())
        {
            error =
                "CLOTH.pyson contains no parsed character items.";

            return false;
        }

        return true;
    }

    bool Catalog::LoadCreator(
        const core::resources::ResourceFileSystem& resources,
        std::string& error)
    {
        constexpr std::string_view Path =
            "res/scripts/common/data/charMakerCfg.json";

        std::string text;

        if (!resources.ReadText(
                Path,
                text))
        {
            error =
                "Unable to read " +
                std::string(Path);

            return false;
        }

        for (auto& [groupName, array] :
             ExtractJsonTopLevelArrays(
                 text))
        {
            CreatorGroup group;

            group.name =
                std::move(
                    groupName);

            for (const std::string_view block :
                 ExtractTopLevelObjects(array))
            {
                CreatorOption option;

                if (!ParseIntegerField(
                        block,
                        "item_id",
                        option.itemType))
                {
                    continue;
                }

                ParseStringField(
                    block,
                    "texture",
                    option.texture);

                if (Find(
                        option.itemType) ==
                    nullptr)
                {
                    core::Log::Warning(
                        std::string(
                            "Character creator references unknown item: ") +
                        std::to_string(
                            option.itemType));

                    continue;
                }

                group.options.push_back(
                    std::move(
                        option));
            }

            if (!group.options.empty())
            {
                creatorGroups_.push_back(
                    std::move(
                        group));
            }
        }

        if (creatorGroups_.empty())
        {
            error =
                "charMakerCfg.json contains no usable groups.";

            return false;
        }

        return true;
    }
}
