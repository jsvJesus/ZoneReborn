#include "Core/Resources/DataSection.h"

#include <charconv>
#include <cmath>
#include <string_view>
#include <system_error>

namespace
{
    bool IsPadding(
        const char value) noexcept
    {
        return
            value == '\0' ||
            value == ' ' ||
            value == '\t' ||
            value == '\r' ||
            value == '\n';
    }

    std::string_view Trim(
        std::string_view value) noexcept
    {
        while (!value.empty() &&
               IsPadding(
                   value.front()))
        {
            value.remove_prefix(1);
        }

        while (!value.empty() &&
               IsPadding(
                   value.back()))
        {
            value.remove_suffix(1);
        }

        return value;
    }

    bool ParseFloat(
        std::string_view text,
        float& output) noexcept
    {
        text =
            Trim(
                text);

        if (text.empty())
        {
            return false;
        }

        float value =
            0.0f;

        const char* begin =
            text.data();

        const char* end =
            begin +
            text.size();

        const auto result =
            std::from_chars(
                begin,
                end,
                value,
                std::chars_format::general);

        if (result.ec !=
                std::errc{} ||
            result.ptr !=
                end ||
            !std::isfinite(
                value))
        {
            return false;
        }

        output =
            value;

        return true;
    }

    bool EqualsIgnoreCase(
        const std::string_view left,
        const std::string_view right) noexcept
    {
        if (left.size() !=
            right.size())
        {
            return false;
        }

        for (std::size_t index = 0;
             index < left.size();
             ++index)
        {
            char a =
                left[index];

            char b =
                right[index];

            if (a >= 'A' &&
                a <= 'Z')
            {
                a =
                    static_cast<char>(
                        a -
                        'A' +
                        'a');
            }

            if (b >= 'A' &&
                b <= 'Z')
            {
                b =
                    static_cast<char>(
                        b -
                        'A' +
                        'a');
            }

            if (a != b)
            {
                return false;
            }
        }

        return true;
    }
}

namespace core::resources
{
    const DataSection* DataSection::FindChild(
        const std::string_view childName) const noexcept
    {
        for (const DataSection& child :
             children)
        {
            if (child.name ==
                childName)
            {
                return &child;
            }
        }

        return nullptr;
    }

    std::vector<const DataSection*> DataSection::FindChildren(
        const std::string_view childName) const
    {
        std::vector<const DataSection*>
            result;

        for (const DataSection& child :
             children)
        {
            if (child.name ==
                childName)
            {
                result.push_back(
                    &child);
            }
        }

        return result;
    }

    const std::string* DataSection::AsString() const noexcept
    {
        return
            std::get_if<std::string>(
                &value);
    }

    const std::int64_t* DataSection::AsInteger() const noexcept
    {
        return
            std::get_if<std::int64_t>(
                &value);
    }

    const DataSection::FloatArray*
    DataSection::AsFloats() const noexcept
    {
        return
            std::get_if<FloatArray>(
                &value);
    }

    const bool* DataSection::AsBoolean() const noexcept
    {
        return
            std::get_if<bool>(
                &value);
    }

    const DataSection::BinaryData*
    DataSection::AsBinary() const noexcept
    {
        return
            std::get_if<BinaryData>(
                &value);
    }

    bool DataSection::TryGetFloat(
        float& output) const noexcept
    {
        if (const FloatArray* values =
                AsFloats())
        {
            if (values->size() ==
                1)
            {
                const float value =
                    (*values)[0];

                if (!std::isfinite(
                        value))
                {
                    return false;
                }

                output =
                    value;

                return true;
            }

            return false;
        }

        if (const std::int64_t* integer =
                AsInteger())
        {
            const float value =
                static_cast<float>(
                    *integer);

            if (!std::isfinite(
                    value))
            {
                return false;
            }

            output =
                value;

            return true;
        }

        if (const bool* boolean =
                AsBoolean())
        {
            output =
                *boolean
                    ? 1.0f
                    : 0.0f;

            return true;
        }

        if (const std::string* string =
                AsString())
        {
            return
                ParseFloat(
                    *string,
                    output);
        }

        return false;
    }

    bool DataSection::TryGetBoolean(
        bool& output) const noexcept
    {
        if (const bool* boolean =
                AsBoolean())
        {
            output =
                *boolean;

            return true;
        }

        if (const std::int64_t* integer =
                AsInteger())
        {
            output =
                *integer != 0;

            return true;
        }

        if (const FloatArray* values =
                AsFloats())
        {
            if (values->size() !=
                1)
            {
                return false;
            }

            const float value =
                (*values)[0];

            if (!std::isfinite(
                    value))
            {
                return false;
            }

            output =
                value != 0.0f;

            return true;
        }

        if (const std::string* string =
                AsString())
        {
            const std::string_view text =
                Trim(
                    *string);

            if (EqualsIgnoreCase(
                    text,
                    "true") ||
                text ==
                    "1")
            {
                output =
                    true;

                return true;
            }

            if (EqualsIgnoreCase(
                    text,
                    "false") ||
                text ==
                    "0")
            {
                output =
                    false;

                return true;
            }

            float numericValue =
                0.0f;

            if (ParseFloat(
                    text,
                    numericValue))
            {
                output =
                    numericValue !=
                    0.0f;

                return true;
            }
        }

        return false;
    }
}