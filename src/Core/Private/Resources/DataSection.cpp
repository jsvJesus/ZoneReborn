#include "Core/Resources/DataSection.h"

namespace core::resources
{
    const DataSection* DataSection::FindChild(
        const std::string_view childName) const noexcept
    {
        for (const DataSection& child : children)
        {
            if (child.name == childName)
            {
                return &child;
            }
        }

        return nullptr;
    }

    std::vector<const DataSection*> DataSection::FindChildren(
        const std::string_view childName) const
    {
        std::vector<const DataSection*> result;

        for (const DataSection& child : children)
        {
            if (child.name == childName)
            {
                result.push_back(&child);
            }
        }

        return result;
    }

    const std::string* DataSection::AsString() const noexcept
    {
        return std::get_if<std::string>(&value);
    }

    const std::int64_t* DataSection::AsInteger() const noexcept
    {
        return std::get_if<std::int64_t>(&value);
    }

    const DataSection::FloatArray*
    DataSection::AsFloats() const noexcept
    {
        return std::get_if<FloatArray>(&value);
    }

    const bool* DataSection::AsBoolean() const noexcept
    {
        return std::get_if<bool>(&value);
    }

    const DataSection::BinaryData*
    DataSection::AsBinary() const noexcept
    {
        return std::get_if<BinaryData>(&value);
    }

    bool DataSection::TryGetFloat(
        float& output) const noexcept
    {
        if (const FloatArray* values = AsFloats())
        {
            if (values->size() == 1)
            {
                output = (*values)[0];
                return true;
            }
        }

        if (const std::int64_t* integer = AsInteger())
        {
            output = static_cast<float>(*integer);
            return true;
        }

        return false;
    }
}