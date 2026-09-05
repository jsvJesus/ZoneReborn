#include "Core/Assets/SpeedTree/CTreeLoader.h"

#include "Core/Resources/ResourcePath.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    constexpr std::size_t IndexedVertexExtraSize =
        32;

    constexpr std::size_t LeafVertexExtraSize =
        76;

    constexpr std::size_t BillboardVertexExtraSize =
        44;

    constexpr std::uint32_t MaximumLodCount =
        16;

    constexpr std::uint32_t MaximumBillboardGroups =
        16;

    constexpr std::uint32_t MaximumVertexCount =
        2'000'000;

    constexpr std::uint32_t MaximumIndexCount =
        20'000'000;

    constexpr std::uint32_t MaximumStringLength =
        4096;

    class BinaryReader final
    {
    public:
        explicit BinaryReader(
            const std::span<const std::byte> data) noexcept
            :
            data_(
                data)
        {
        }

        [[nodiscard]]
        bool ReadUInt32(
            std::uint32_t& output) noexcept
        {
            if (!CanRead(
                    sizeof(output)))
            {
                return false;
            }

            std::memcpy(
                &output,
                data_.data() +
                    offset_,
                sizeof(output));

            offset_ +=
                sizeof(output);

            return true;
        }

        [[nodiscard]]
        bool ReadFloat(
            float& output) noexcept
        {
            if (!CanRead(
                    sizeof(output)))
            {
                return false;
            }

            std::memcpy(
                &output,
                data_.data() +
                    offset_,
                sizeof(output));

            offset_ +=
                sizeof(output);

            return true;
        }

        [[nodiscard]]
        bool ReadBytes(
            const std::span<std::byte> output) noexcept
        {
            if (!CanRead(
                    output.size()))
            {
                return false;
            }

            if (!output.empty())
            {
                std::memcpy(
                    output.data(),
                    data_.data() +
                        offset_,
                    output.size());
            }

            offset_ +=
                output.size();

            return true;
        }

        [[nodiscard]]
        bool ReadString(
            std::string& output) noexcept
        {
            output.clear();

            std::uint32_t length =
                0;

            if (!ReadUInt32(
                    length))
            {
                return false;
            }

            if (length >
                MaximumStringLength)
            {
                return false;
            }

            if (!CanRead(
                    length))
            {
                return false;
            }

            output.assign(
                reinterpret_cast<const char*>(
                    data_.data() +
                    offset_),
                length);

            offset_ +=
                length;

            return true;
        }

        [[nodiscard]]
        std::size_t Offset() const noexcept
        {
            return offset_;
        }

        [[nodiscard]]
        std::size_t Remaining() const noexcept
        {
            return
                data_.size() -
                offset_;
        }

    private:
        [[nodiscard]]
        bool CanRead(
            const std::size_t size) const noexcept
        {
            return
                offset_ <=
                    data_.size() &&
                size <=
                    data_.size() -
                    offset_;
        }

        std::span<const std::byte>
            data_;

        std::size_t offset_ =
            0;
    };

    std::string ReplaceExtension(
        std::string path,
        const std::string_view extension)
    {
        const std::size_t slash =
            path.find_last_of('/');

        const std::size_t dot =
            path.find_last_of('.');

        if (dot !=
                std::string::npos &&
            (
                slash ==
                    std::string::npos ||
                dot >
                    slash
            ))
        {
            path.resize(
                dot);
        }

        path.append(
            extension);

        return path;
    }

    bool ReadVector3(
        BinaryReader& reader,
        core::math::Vector3& output)
    {
        return
            reader.ReadFloat(
                output.x) &&
            reader.ReadFloat(
                output.y) &&
            reader.ReadFloat(
                output.z);
    }

    bool ReadMaterial(
        BinaryReader& reader,
        core::assets::speedtree::CTreeMaterial& output,
        std::string& error)
    {
        output = {};

        if (!reader.ReadString(
                output.diffuseReference))
        {
            error =
                "CTREE diffuse texture reference is invalid.";

            return false;
        }

        if (!reader.ReadString(
                output.normalReference))
        {
            error =
                "CTREE normal texture reference is invalid.";

            return false;
        }

        if (!output.diffuseReference.empty())
        {
            output.diffuseLogicalPath =
                core::resources::ResourcePath::ToResPath(
                    output.diffuseReference);
        }

        if (!output.normalReference.empty())
        {
            output.normalLogicalPath =
                core::resources::ResourcePath::ToResPath(
                    output.normalReference);
        }

        return true;
    }

    bool ReadLods(
        BinaryReader& reader,
        const std::uint32_t vertexCount,
        std::vector<
            core::assets::speedtree::CTreeLod>& output,
        std::string& error)
    {
        output.clear();

        std::uint32_t lodCount =
            0;

        if (!reader.ReadUInt32(
                lodCount))
        {
            error =
                "CTREE LOD count is truncated.";

            return false;
        }

        if (lodCount >
            MaximumLodCount)
        {
            error =
                "CTREE LOD count is invalid.";

            return false;
        }

        output.reserve(
            lodCount);

        for (std::uint32_t lodIndex = 0;
             lodIndex <
                lodCount;
             ++lodIndex)
        {
            std::uint32_t indexCount =
                0;

            if (!reader.ReadUInt32(
                    indexCount))
            {
                error =
                    "CTREE LOD index count is truncated.";

                return false;
            }

            if (indexCount >
                MaximumIndexCount)
            {
                error =
                    "CTREE LOD index count is invalid.";

                return false;
            }

            core::assets::speedtree::CTreeLod
                lod;

            lod.indices.resize(
                indexCount);

            for (std::uint32_t index = 0;
                 index <
                    indexCount;
                 ++index)
            {
                std::uint32_t vertexIndex =
                    0;

                if (!reader.ReadUInt32(
                        vertexIndex))
                {
                    error =
                        "CTREE LOD index data is truncated.";

                    return false;
                }

                if (vertexCount !=
                        0 &&
                    vertexIndex >=
                        vertexCount)
                {
                    error =
                        "CTREE LOD contains an invalid vertex index.";

                    return false;
                }

                lod.indices[
                    index] =
                    vertexIndex;
            }

            output.push_back(
                std::move(
                    lod));
        }

        return true;
    }

    bool ReadIndexedGeometry(
        BinaryReader& reader,
        core::assets::speedtree::CTreeIndexedGeometry& output,
        std::string& error)
    {
        output = {};

        std::uint32_t vertexCount =
            0;

        if (!reader.ReadUInt32(
                vertexCount))
        {
            error =
                "CTREE indexed geometry vertex count is truncated.";

            return false;
        }

        if (vertexCount >
            MaximumVertexCount)
        {
            error =
                "CTREE indexed geometry vertex count is invalid.";

            return false;
        }

        output.vertices.resize(
            vertexCount);

        for (std::uint32_t index = 0;
             index <
                vertexCount;
             ++index)
        {
            core::assets::speedtree::CTreeIndexedVertex&
                vertex =
                    output.vertices[
                        index];

            if (!ReadVector3(
                    reader,
                    vertex.position) ||
                !ReadVector3(
                    reader,
                    vertex.normal) ||
                !reader.ReadFloat(
                    vertex.u) ||
                !reader.ReadFloat(
                    vertex.v) ||
                !reader.ReadBytes(
                    std::span<std::byte>(
                        vertex.extra.data(),
                        vertex.extra.size())))
            {
                error =
                    "CTREE indexed vertex data is truncated.";

                return false;
            }
        }

        if (!ReadLods(
                reader,
                vertexCount,
                output.lods,
                error))
        {
            return false;
        }

        return ReadMaterial(
            reader,
            output.material,
            error);
    }

    bool ReadLeaves(
        BinaryReader& reader,
        core::assets::speedtree::CTreeLeafGeometry& output,
        std::string& error)
    {
        output = {};

        std::uint32_t vertexCount =
            0;

        if (!reader.ReadUInt32(
                vertexCount))
        {
            error =
                "CTREE leaf vertex count is truncated.";

            return false;
        }

        if (vertexCount >
            MaximumVertexCount)
        {
            error =
                "CTREE leaf vertex count is invalid.";

            return false;
        }

        output.vertices.resize(
            vertexCount);

        for (std::uint32_t index = 0;
             index <
                vertexCount;
             ++index)
        {
            core::assets::speedtree::CTreeLeafVertex&
                vertex =
                    output.vertices[
                        index];

            if (!ReadVector3(
                    reader,
                    vertex.position) ||
                !ReadVector3(
                    reader,
                    vertex.normal) ||
                !reader.ReadBytes(
                    std::span<std::byte>(
                        vertex.extra.data(),
                        vertex.extra.size())))
            {
                error =
                    "CTREE leaf vertex data is truncated.";

                return false;
            }
        }

        if (!ReadLods(
                reader,
                vertexCount,
                output.lods,
                error))
        {
            return false;
        }

        return ReadMaterial(
            reader,
            output.material,
            error);
    }

    bool ReadBillboard(
        BinaryReader& reader,
        core::assets::speedtree::CTreeBillboardGeometry& output,
        std::string& error)
    {
        output = {};

        std::uint32_t groupCount =
            0;

        if (!reader.ReadUInt32(
                groupCount))
        {
            error =
                "CTREE billboard group count is truncated.";

            return false;
        }

        if (groupCount >
            MaximumBillboardGroups)
        {
            error =
                "CTREE billboard group count is invalid.";

            return false;
        }

        output.groups.reserve(
            groupCount);

        for (std::uint32_t groupIndex = 0;
             groupIndex <
                groupCount;
             ++groupIndex)
        {
            std::uint32_t vertexCount =
                0;

            if (!reader.ReadUInt32(
                    vertexCount))
            {
                error =
                    "CTREE billboard vertex count is truncated.";

                return false;
            }

            if (vertexCount >
                MaximumVertexCount)
            {
                error =
                    "CTREE billboard vertex count is invalid.";

                return false;
            }

            core::assets::speedtree::CTreeBillboardGroup
                group;

            group.vertices.resize(
                vertexCount);

            for (std::uint32_t vertexIndex = 0;
                 vertexIndex <
                    vertexCount;
                 ++vertexIndex)
            {
                core::assets::speedtree::CTreeBillboardVertex&
                    vertex =
                        group.vertices[
                            vertexIndex];

                if (!ReadVector3(
                        reader,
                        vertex.position) ||
                    !ReadVector3(
                        reader,
                        vertex.normal) ||
                    !reader.ReadBytes(
                        std::span<std::byte>(
                            vertex.extra.data(),
                            vertex.extra.size())))
                {
                    error =
                        "CTREE billboard vertex data is truncated.";

                    return false;
                }
            }

            std::uint32_t indexCount =
                0;

            if (!reader.ReadUInt32(
                    indexCount))
            {
                error =
                    "CTREE billboard index count is truncated.";

                return false;
            }

            if (indexCount >
                MaximumIndexCount)
            {
                error =
                    "CTREE billboard index count is invalid.";

                return false;
            }

            group.indices.resize(
                indexCount);

            for (std::uint32_t index = 0;
                 index <
                    indexCount;
                 ++index)
            {
                std::uint32_t vertexIndex =
                    0;

                if (!reader.ReadUInt32(
                        vertexIndex))
                {
                    error =
                        "CTREE billboard indices are truncated.";

                    return false;
                }

                if (vertexIndex >=
                    vertexCount)
                {
                    error =
                        "CTREE billboard contains invalid vertex index.";

                    return false;
                }

                group.indices[
                    index] =
                    vertexIndex;
            }

            output.groups.push_back(
                std::move(
                    group));
        }

        return ReadMaterial(
            reader,
            output.material,
            error);
    }
}

namespace core::assets::speedtree
{
    bool CTreeLoader::Load(
        const resources::ResourceFileSystem& resources,
        const std::string_view sptLogicalPath,
        CTreeAsset& output,
        std::string& error) const
    {
        output = {};
        error.clear();

        std::string normalizedSpt =
            resources::ResourcePath::ToResPath(
                sptLogicalPath);

        if (normalizedSpt.empty())
        {
            error =
                "CTREE source SPT path is empty.";

            return false;
        }

        const std::string ctreePath =
            ReplaceExtension(
                normalizedSpt,
                ".ctree");

        if (!resources.Exists(
                ctreePath))
        {
            error =
                "CTREE resource was not found: " +
                ctreePath;

            return false;
        }

        std::vector<std::byte>
            encoded;

        if (!resources.ReadBinary(
                ctreePath,
                encoded))
        {
            error =
                "Unable to read CTREE resource: " +
                ctreePath;

            return false;
        }

        BinaryReader reader(
            std::span<const std::byte>(
                encoded.data(),
                encoded.size()));

        CTreeAsset tree;

        tree.sptLogicalPath =
            normalizedSpt;

        tree.ctreeLogicalPath =
            ctreePath;

        if (!reader.ReadUInt32(
                tree.version))
        {
            error =
                "CTREE version is truncated.";

            return false;
        }

        if (tree.version !=
            CTreeAsset::SupportedVersion)
        {
            error =
                "Unsupported CTREE version: " +
                std::to_string(
                    tree.version);

            return false;
        }

        if (!ReadVector3(
                reader,
                tree.boundsMinimum) ||
            !ReadVector3(
                reader,
                tree.boundsMaximum) ||
            !reader.ReadFloat(
                tree.parameter0) ||
            !reader.ReadFloat(
                tree.parameter1))
        {
            error =
                "CTREE header is truncated.";

            return false;
        }

        if (!ReadIndexedGeometry(
                reader,
                tree.branches,
                error))
        {
            error =
                "CTREE branches: " +
                error;

            return false;
        }

        if (!ReadIndexedGeometry(
                reader,
                tree.fronds,
                error))
        {
            error =
                "CTREE fronds: " +
                error;

            return false;
        }

        if (!ReadLeaves(
                reader,
                tree.leaves,
                error))
        {
            error =
                "CTREE leaves: " +
                error;

            return false;
        }

        if (!ReadBillboard(
                reader,
                tree.billboard,
                error))
        {
            error =
                "CTREE billboard: " +
                error;

            return false;
        }

        if (reader.Remaining() !=
            0)
        {
            error =
                "CTREE contains unparsed trailing data at offset " +
                std::to_string(
                    reader.Offset()) +
                ", bytes remaining=" +
                std::to_string(
                    reader.Remaining());

            return false;
        }

        output =
            std::move(
                tree);

        return true;
    }
}