#include "Core/Assets/SpeedTree/SpeedTreeResourceInspector.h"

#include "Core/Resources/ResourcePath.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <set>
#include <sstream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    std::string ReplaceExtension(
        std::string path,
        const std::string_view extension)
    {
        const std::size_t slash =
            path.find_last_of('/');

        const std::size_t dot =
            path.find_last_of('.');

        if (dot != std::string::npos &&
            (
                slash == std::string::npos ||
                dot > slash
            ))
        {
            path.resize(dot);
        }

        path.append(extension);

        return path;
    }

    bool IsPrintable(
        const unsigned char value) noexcept
    {
        return
            value >= 32 &&
            value <= 126;
    }

    std::string ToLower(
        std::string value)
    {
        for (char& character : value)
        {
            character =
                static_cast<char>(
                    std::tolower(
                        static_cast<unsigned char>(
                            character)));
        }

        return value;
    }

    bool IsTextureReference(
        const std::string_view value)
    {
        const std::string lower =
            ToLower(
                std::string(value));

        return
            lower.ends_with(".dds") ||
            lower.ends_with(".jpg") ||
            lower.ends_with(".jpeg") ||
            lower.ends_with(".tga") ||
            lower.ends_with(".png");
    }

    std::vector<std::string>
    ExtractTextureReferences(
        const std::span<const std::byte> data)
    {
        std::set<std::string>
            unique;

        std::string current;

        const auto flush =
            [&]()
            {
                if (current.size() >= 4 &&
                    IsTextureReference(current))
                {
                    unique.insert(current);
                }

                current.clear();
            };

        for (const std::byte byte : data)
        {
            const unsigned char value =
                std::to_integer<unsigned char>(
                    byte);

            if (IsPrintable(value))
            {
                current.push_back(
                    static_cast<char>(value));

                if (current.size() > 1024)
                {
                    flush();
                }

                continue;
            }

            flush();
        }

        flush();

        return
        {
            unique.begin(),
            unique.end()
        };
    }

    bool ContainsPossibleZlib(
        const std::span<const std::byte> data) noexcept
    {
        if (data.size() < 2)
        {
            return false;
        }

        for (std::size_t index = 0;
             index + 1 < data.size();
             ++index)
        {
            const std::uint8_t first =
                std::to_integer<std::uint8_t>(
                    data[index]);

            const std::uint8_t second =
                std::to_integer<std::uint8_t>(
                    data[index + 1]);

            if (first != 0x78u)
            {
                continue;
            }

            if (second == 0x01u ||
                second == 0x5Eu ||
                second == 0x9Cu ||
                second == 0xDAu)
            {
                return true;
            }
        }

        return false;
    }

    std::string HeaderToHex(
        const std::span<const std::byte> data)
    {
        constexpr std::size_t HeaderLength =
            32;

        const std::size_t count =
            std::min(
                HeaderLength,
                data.size());

        std::ostringstream stream;

        stream
            << std::hex
            << std::setfill('0');

        for (std::size_t index = 0;
             index < count;
             ++index)
        {
            if (index != 0)
            {
                stream << ' ';
            }

            stream
                << std::setw(2)
                << static_cast<unsigned int>(
                    std::to_integer<std::uint8_t>(
                        data[index]));
        }

        return stream.str();
    }
}

namespace core::assets::speedtree
{
    bool SpeedTreeResourceInspector::Inspect(
        const resources::ResourceFileSystem& resources,
        const std::string_view sptLogicalPath,
        SpeedTreeResourceInfo& output,
        std::string& error) const
    {
        output = {};
        error.clear();

        std::string normalized =
            resources::ResourcePath::Normalize(
                sptLogicalPath);

        if (normalized.empty())
        {
            error =
                "SpeedTree SPT path is empty.";

            return false;
        }

        if (!normalized.starts_with("res/"))
        {
            normalized =
                "res/" +
                normalized;
        }

        if (!normalized.ends_with(".spt"))
        {
            error =
                "SpeedTree resource does not have .spt extension: " +
                normalized;

            return false;
        }

        if (!resources.Exists(normalized))
        {
            error =
                "SpeedTree SPT resource was not found: " +
                normalized;

            return false;
        }

        SpeedTreeResourceInfo info;

        info.sptLogicalPath =
            normalized;

        info.ctreeLogicalPath =
            ReplaceExtension(
                normalized,
                ".ctree");

        info.bspLogicalPath =
            ReplaceExtension(
                normalized,
                ".bsp2");

        std::vector<std::byte>
            sptData;

        if (!resources.ReadBinary(
                info.sptLogicalPath,
                sptData))
        {
            error =
                "Unable to read SpeedTree SPT: " +
                info.sptLogicalPath;

            return false;
        }

        info.sptSize =
            sptData.size();

        info.ctreeExists =
            resources.Exists(
                info.ctreeLogicalPath);

        if (info.ctreeExists)
        {
            std::vector<std::byte>
                ctreeData;

            if (!resources.ReadBinary(
                    info.ctreeLogicalPath,
                    ctreeData))
            {
                error =
                    "Unable to read SpeedTree CTREE: " +
                    info.ctreeLogicalPath;

                return false;
            }

            info.ctreeSize =
                ctreeData.size();

            info.ctreeHeaderHex =
                HeaderToHex(
                    ctreeData);

            info.containsPossibleZlibStream =
                ContainsPossibleZlib(
                    ctreeData);

            info.textureReferences =
                ExtractTextureReferences(
                    ctreeData);
        }

        info.bspExists =
            resources.Exists(
                info.bspLogicalPath);

        if (info.bspExists)
        {
            std::vector<std::byte>
                bspData;

            if (!resources.ReadBinary(
                    info.bspLogicalPath,
                    bspData))
            {
                error =
                    "Unable to read SpeedTree BSP2: " +
                    info.bspLogicalPath;

                return false;
            }

            info.bspSize =
                bspData.size();
        }

        output =
            std::move(info);

        return true;
    }
}