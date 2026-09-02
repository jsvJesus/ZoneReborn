#include "Core/Assets/PrimitivesContainer.h"

namespace core::assets
{
    const PrimitivesSection*
    PrimitivesContainer::FindSection(
        const std::string_view name) const noexcept
    {
        for (const PrimitivesSection& section :
             sections)
        {
            if (section.name == name)
            {
                return &section;
            }
        }

        return nullptr;
    }

    std::span<const std::byte>
    PrimitivesContainer::SectionData(
        const PrimitivesSection& section) const noexcept
    {
        if (section.offset >
            data.size())
        {
            return {};
        }

        if (section.size >
            data.size() -
                section.offset)
        {
            return {};
        }

        return std::span<const std::byte>(
            data.data() + section.offset,
            section.size);
    }

    std::span<const std::byte>
    PrimitivesContainer::SectionData(
        const std::string_view name) const noexcept
    {
        const PrimitivesSection* section =
            FindSection(name);

        if (section == nullptr)
        {
            return {};
        }

        return SectionData(
            *section);
    }
}