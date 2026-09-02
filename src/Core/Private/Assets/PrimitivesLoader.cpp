#include "Core/Assets/PrimitivesLoader.h"

#include "Core/Resources/ResourcePath.h"

#include <cstring>
#include <limits>
#include <string>
#include <unordered_set>
#include <utility>

namespace
{
    bool ReadUInt32(
        const std::vector<std::byte>& data,
        const std::size_t offset,
        std::uint32_t& output) noexcept
    {
        if (offset >
            data.size())
        {
            return false;
        }

        if (sizeof(std::uint32_t) >
            data.size() - offset)
        {
            return false;
        }

        std::memcpy(
            &output,
            data.data() + offset,
            sizeof(output));

        return true;
    }

    std::size_t Align4(
        const std::size_t value) noexcept
    {
        return
            (value + 3u) &
            ~static_cast<std::size_t>(3u);
    }
}

namespace core::assets
{
    bool PrimitivesLoader::Load(
        const resources::ResourceFileSystem& resources,
        const std::string_view primitivesReference,
        PrimitivesContainer& output,
        std::string& error) const
    {
        output = {};
        error.clear();

        if (!resources.IsInitialized())
        {
            error =
                "Resource filesystem is not initialized.";

            return false;
        }

        const std::string logicalPath =
            resources::ResourcePath::ToResPath(
                primitivesReference);

        if (logicalPath.empty())
        {
            error =
                "Primitives resource path is invalid.";

            return false;
        }

        std::vector<std::byte> data;

        if (!resources.ReadBinary(
                logicalPath,
                data))
        {
            error =
                "Unable to read primitives: " +
                logicalPath;

            return false;
        }

        if (data.size() <
            sizeof(std::uint32_t) * 2)
        {
            error =
                "Primitives file is too small.";

            return false;
        }

        std::uint32_t signature = 0;

        if (!ReadUInt32(
                data,
                0,
                signature))
        {
            error =
                "Unable to read primitives signature.";

            return false;
        }

        if (signature != Signature)
        {
            error =
                "Primitives signature is invalid.";

            return false;
        }

        std::uint32_t tableSize32 = 0;

        if (!ReadUInt32(
                data,
                data.size() -
                    sizeof(std::uint32_t),
                tableSize32))
        {
            error =
                "Unable to read primitives table size.";

            return false;
        }

        const std::size_t tableSize =
            static_cast<std::size_t>(
                tableSize32);

        const std::size_t tableEnd =
            data.size() -
            sizeof(std::uint32_t);

        if (tableSize >
            tableEnd)
        {
            error =
                "Primitives table size is invalid.";

            return false;
        }

        const std::size_t tableStart =
            tableEnd -
            tableSize;

        if (tableStart <
            sizeof(std::uint32_t))
        {
            error =
                "Primitives table offset is invalid.";

            return false;
        }

        std::size_t cursor =
            tableStart;

        std::size_t contentOffset =
            sizeof(std::uint32_t);

        std::vector<PrimitivesSection> sections;

        std::unordered_set<std::string> sectionNames;

        while (cursor < tableEnd)
        {
            constexpr std::size_t HeaderSize =
                sizeof(std::uint32_t) +
                sizeof(std::uint32_t) * 4 +
                sizeof(std::uint32_t);

            if (HeaderSize >
                tableEnd - cursor)
            {
                error =
                    "Primitives section table is truncated.";

                return false;
            }

            std::uint32_t sectionSize32 = 0;

            if (!ReadUInt32(
                    data,
                    cursor,
                    sectionSize32))
            {
                return false;
            }

            cursor +=
                sizeof(std::uint32_t);

            PrimitivesSection section;

            section.size =
                static_cast<std::size_t>(
                    sectionSize32);

            for (std::size_t index = 0;
                 index < section.metadata.size();
                 ++index)
            {
                if (!ReadUInt32(
                        data,
                        cursor,
                        section.metadata[index]))
                {
                    error =
                        "Unable to read primitives metadata.";

                    return false;
                }

                cursor +=
                    sizeof(std::uint32_t);
            }

            std::uint32_t nameLength32 = 0;

            if (!ReadUInt32(
                    data,
                    cursor,
                    nameLength32))
            {
                error =
                    "Unable to read primitives section name length.";

                return false;
            }

            cursor +=
                sizeof(std::uint32_t);

            const std::size_t nameLength =
                static_cast<std::size_t>(
                    nameLength32);

            if (nameLength == 0)
            {
                error =
                    "Primitives section has empty name.";

                return false;
            }

            const std::size_t paddedNameLength =
                Align4(nameLength);

            if (paddedNameLength >
                tableEnd - cursor)
            {
                error =
                    "Primitives section name is truncated.";

                return false;
            }

            section.name.assign(
                reinterpret_cast<const char*>(
                    data.data() + cursor),
                nameLength);

            cursor +=
                paddedNameLength;

            if (!sectionNames.insert(
                    section.name).second)
            {
                error =
                    "Primitives contains duplicate section: " +
                    section.name;

                return false;
            }

            section.offset =
                contentOffset;

            if (section.size >
                tableStart - contentOffset)
            {
                error =
                    "Primitives section exceeds data region: " +
                    section.name;

                return false;
            }

            contentOffset +=
                section.size;

            sections.push_back(
                std::move(section));
        }

        if (cursor != tableEnd)
        {
            error =
                "Primitives table size does not match file.";

            return false;
        }

        if (Align4(contentOffset) !=
            tableStart)
        {
            error =
                "Primitives data size does not match section table.";

            return false;
        }

        PrimitivesContainer container;

        container.logicalPath =
            logicalPath;

        container.data =
            std::move(data);

        container.sections =
            std::move(sections);

        output =
            std::move(container);

        return true;
    }
}