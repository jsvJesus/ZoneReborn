#include "Graphics/Studio/SkyTextures.h"

#include "Core/Log.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <vector>

namespace client::graphics
{
    namespace
    {
        constexpr std::uint32_t DdsMagic = 0x20534444u;

        constexpr std::uint32_t Dxt1 = 0x31545844u;
        constexpr std::uint32_t Dxt3 = 0x33545844u;
        constexpr std::uint32_t Dxt5 = 0x35545844u;
        constexpr std::uint32_t Dx10 = 0x30315844u;

        constexpr std::uint32_t CubeFlag = 0x200u;
        constexpr std::uint32_t CubeFaces = 0xFC00u;
        constexpr std::uint32_t VolumeFlag = 0x200000u;

        constexpr std::uint32_t Dx10Cube = 0x4u;

        constexpr std::size_t MaxFileSize =
            128u * 1024u * 1024u;

        constexpr std::size_t MaxCacheEntries = 32;

        std::uint32_t U32(
            const std::byte* data,
            std::size_t offset)
        {
            std::uint32_t value = 0;

            std::memcpy(
                &value,
                data + offset,
                sizeof(value));

            return value;
        }

        bool NormalizeName(
            const std::string& name,
            std::filesystem::path& relative)
        {
            relative.clear();

            if (name.empty() || name.size() > 240)
                return false;

            std::string normalized = name;

            for (char& c : normalized)
            {
                if (c == '\\')
                    c = '/';

                const unsigned char value =
                    static_cast<unsigned char>(c);

                if (value < 32 ||
                    c == ':' ||
                    c == '*' ||
                    c == '?' ||
                    c == '"' ||
                    c == '<' ||
                    c == '>' ||
                    c == '|')
                {
                    return false;
                }
            }

            if (normalized.front() == '/')
                return false;

            relative =
                std::filesystem::path(normalized);

            if (relative.is_absolute() ||
                relative.has_root_path())
            {
                return false;
            }

            for (const auto& part : relative)
            {
                if (part == ".." || part == ".")
                    return false;
            }

            relative.replace_extension(".dds");

            return true;
        }

        bool ReadFile(
            const std::filesystem::path& path,
            std::vector<std::byte>& bytes,
            std::string& error)
        {
            std::error_code ec;

            const auto size =
                std::filesystem::file_size(path, ec);

            if (ec || size < 128 ||
                size > MaxFileSize)
            {
                error = "Invalid DDS file size.";
                return false;
            }

            std::ifstream stream(
                path,
                std::ios::binary);

            if (!stream)
            {
                error = "Unable to open DDS file.";
                return false;
            }

            bytes.resize(
                static_cast<std::size_t>(size));

            if (!stream.read(
                reinterpret_cast<char*>(bytes.data()),
                static_cast<std::streamsize>(size)))
            {
                error = "Unable to read DDS file.";
                return false;
            }

            return true;
        }

        bool ResolveFormat(
            const std::vector<std::byte>& bytes,
            DXGI_FORMAT& format,
            std::uint32_t& blockSize,
            bool& dx10)
        {
            const auto* data = bytes.data();

            const std::uint32_t fourCc =
                U32(data, 84);

            dx10 = fourCc == Dx10;
            blockSize = 0;

            if (fourCc == Dxt1)
            {
                format = DXGI_FORMAT_BC1_UNORM;
                blockSize = 8;
                return true;
            }

            if (fourCc == Dxt3)
            {
                format = DXGI_FORMAT_BC2_UNORM;
                blockSize = 16;
                return true;
            }

            if (fourCc == Dxt5)
            {
                format = DXGI_FORMAT_BC3_UNORM;
                blockSize = 16;
                return true;
            }

            if (dx10)
            {
                if (bytes.size() < 148)
                    return false;

                format = static_cast<DXGI_FORMAT>(
                    U32(data, 128));

                switch (format)
                {
                    case DXGI_FORMAT_BC1_UNORM:
                    case DXGI_FORMAT_BC1_UNORM_SRGB:
                        blockSize = 8;
                        return true;

                    case DXGI_FORMAT_BC2_UNORM:
                    case DXGI_FORMAT_BC2_UNORM_SRGB:
                    case DXGI_FORMAT_BC3_UNORM:
                    case DXGI_FORMAT_BC3_UNORM_SRGB:
                        blockSize = 16;
                        return true;

                    case DXGI_FORMAT_R8G8B8A8_UNORM:
                    case DXGI_FORMAT_B8G8R8A8_UNORM:
                    case DXGI_FORMAT_B8G8R8X8_UNORM:
                        return true;

                    default:
                        return false;
                }
            }

            if (fourCc != 0 ||
                U32(data, 88) != 32)
            {
                return false;
            }

            const auto r = U32(data, 92);
            const auto g = U32(data, 96);
            const auto b = U32(data, 100);
            const auto a = U32(data, 104);

            if (r == 0x00FF0000u &&
                g == 0x0000FF00u &&
                b == 0x000000FFu)
            {
                format = a == 0xFF000000u
                    ? DXGI_FORMAT_B8G8R8A8_UNORM
                    : DXGI_FORMAT_B8G8R8X8_UNORM;

                return a == 0xFF000000u || a == 0;
            }

            if (r == 0x000000FFu &&
                g == 0x0000FF00u &&
                b == 0x00FF0000u &&
                a == 0xFF000000u)
            {
                format = DXGI_FORMAT_R8G8B8A8_UNORM;
                return true;
            }

            return false;
        }
    }

    void StudioSkyTextures::SetRoot(
        const std::filesystem::path& root)
    {
        root_ = root;
        Clear();
    }

    void StudioSkyTextures::Clear() noexcept
    {
        entries_.clear();
        sequence_ = 0;
    }

    bool StudioSkyTextures::Load(
        ID3D11Device* device,
        const std::filesystem::path& path,
        bool cubemap,
        Microsoft::WRL::ComPtr<
            ID3D11ShaderResourceView>& output,
        std::string& error) const
    {
        output.Reset();
        error.clear();

        if (!device)
        {
            error = "DX11 device is unavailable.";
            return false;
        }

        std::vector<std::byte> bytes;

        if (!ReadFile(path, bytes, error))
            return false;

        const auto* data = bytes.data();

        const std::uint32_t headerSize = U32(data, 4);

        const bool standardHeader =
            headerSize == 124;

        const bool legacyHeader =
            headerSize == 24 &&
            U32(data, 84) == Dxt1;

        if (U32(data, 0) != DdsMagic ||
            (!standardHeader && !legacyHeader) ||
            U32(data, 76) != 32)
        {
            error = "Unsupported DDS header.";
            return false;
        }

        const std::uint32_t width = U32(data, 16);
        const std::uint32_t height = U32(data, 12);

        if (width == 0 || height == 0 ||
            width > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION ||
            height > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION)
        {
            error = "Invalid DDS dimensions.";
            return false;
        }

        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

        std::uint32_t blockSize = 0;
        bool dx10 = false;

        if (!ResolveFormat(
            bytes,
            format,
            blockSize,
            dx10))
        {
            error = "Unsupported DDS pixel format.";
            return false;
        }

        const std::uint32_t caps2 = U32(data, 112);

        if ((caps2 & VolumeFlag) != 0)
        {
            error = "Volume DDS is unsupported.";
            return false;
        }

        bool isCube = false;

        if (dx10)
        {
            if (U32(data, 132) != 3 ||
                U32(data, 140) != 1)
            {
                error = "Unsupported DX10 DDS resource.";
                return false;
            }

            isCube =
                (U32(data, 136) & Dx10Cube) != 0;
        }
        else
        {
            isCube = (caps2 & CubeFlag) != 0;

            if (isCube &&
                (caps2 & CubeFaces) != CubeFaces)
            {
                error = "Incomplete DDS cubemap faces.";
                return false;
            }
        }

        if (isCube != cubemap)
        {
            error = cubemap
                ? "DDS is not a cubemap."
                : "Cloud DDS must be 2D.";

            return false;
        }

        if (isCube && width != height)
        {
            error = "Cubemap faces must be square.";
            return false;
        }

        const std::uint32_t mipCount =
            std::max(1u, U32(data, 28));

        std::uint32_t maxMips = 1;

        for (std::uint32_t dimension =
                 std::max(width, height);
             dimension > 1;
             dimension >>= 1)
        {
            ++maxMips;
        }

        if (mipCount > maxMips ||
            mipCount > D3D11_REQ_MIP_LEVELS)
        {
            error = "Invalid DDS mip count.";
            return false;
        }

        const std::uint32_t faceCount =
            isCube ? 6u : 1u;

        const std::size_t dataStart =
            dx10 ? 148u : 128u;

        std::size_t offset = dataStart;

        std::vector<D3D11_SUBRESOURCE_DATA>
            subresources;

        subresources.reserve(
            static_cast<std::size_t>(
                faceCount) * mipCount);

        for (std::uint32_t face = 0;
             face < faceCount;
             ++face)
        {
            std::uint32_t mipWidth = width;
            std::uint32_t mipHeight = height;

            for (std::uint32_t mip = 0;
                 mip < mipCount;
                 ++mip)
            {
                const std::uint64_t rowPitch =
                    blockSize != 0
                    ? static_cast<std::uint64_t>(
                        std::max(
                            1u,
                            (mipWidth + 3u) / 4u)) *
                        blockSize
                    : static_cast<std::uint64_t>(
                        mipWidth) * 4u;

                const std::uint64_t rows =
                    blockSize != 0
                    ? std::max(
                        1u,
                        (mipHeight + 3u) / 4u)
                    : mipHeight;

                const std::uint64_t slicePitch =
                    rowPitch * rows;

                if (rowPitch >
                        std::numeric_limits<UINT>::max() ||
                    slicePitch >
                        std::numeric_limits<UINT>::max() ||
                    offset > bytes.size() ||
                    slicePitch > bytes.size() - offset)
                {
                    error = "DDS subresource is truncated.";
                    return false;
                }

                D3D11_SUBRESOURCE_DATA subresource{};

                subresource.pSysMem =
                    bytes.data() + offset;

                subresource.SysMemPitch =
                    static_cast<UINT>(rowPitch);

                subresource.SysMemSlicePitch =
                    static_cast<UINT>(slicePitch);

                subresources.push_back(subresource);

                offset += static_cast<std::size_t>(
                    slicePitch);

                mipWidth = std::max(1u, mipWidth / 2u);
                mipHeight = std::max(1u, mipHeight / 2u);
            }
        }

        D3D11_TEXTURE2D_DESC description{};

        description.Width = width;
        description.Height = height;

        description.MipLevels = mipCount;
        description.ArraySize = faceCount;

        description.Format = format;

        description.SampleDesc.Count = 1;

        description.Usage = D3D11_USAGE_IMMUTABLE;

        description.BindFlags =
            D3D11_BIND_SHADER_RESOURCE;

        if (isCube)
        {
            description.MiscFlags =
                D3D11_RESOURCE_MISC_TEXTURECUBE;
        }

        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;

        HRESULT hr = device->CreateTexture2D(
            &description,
            subresources.data(),
            &texture);

        if (FAILED(hr))
        {
            error = "CreateTexture2D failed.";
            return false;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC view{};

        view.Format = format;

        if (isCube)
        {
            view.ViewDimension =
                D3D11_SRV_DIMENSION_TEXTURECUBE;

            view.TextureCube.MostDetailedMip = 0;
            view.TextureCube.MipLevels = mipCount;
        }
        else
        {
            view.ViewDimension =
                D3D11_SRV_DIMENSION_TEXTURE2D;

            view.Texture2D.MostDetailedMip = 0;
            view.Texture2D.MipLevels = mipCount;
        }

        hr = device->CreateShaderResourceView(
            texture.Get(),
            &view,
            &output);

        if (FAILED(hr))
        {
            error = "CreateShaderResourceView failed.";
            return false;
        }

        return true;
    }

    ID3D11ShaderResourceView* StudioSkyTextures::Get(
        ID3D11Device* device,
        const std::string& logicalName,
        bool cubemap)
    {
        if (!device ||
            root_.empty() ||
            logicalName.empty())
        {
            return nullptr;
        }

        std::filesystem::path relative;

        if (!NormalizeName(logicalName, relative))
            return nullptr;

        const std::string key =
            std::string(cubemap ? "cube/" : "cloud/") +
            relative.generic_string();

        auto existing = entries_.find(key);

        if (existing != entries_.end())
        {
            existing->second.lastUsed = ++sequence_;
            return existing->second.view.Get();
        }

        Entry entry;
        entry.lastUsed = ++sequence_;

        const auto path = root_ / relative;

        std::string error;

        if (Load(
            device,
            path,
            cubemap,
            entry.view,
            error))
        {
            core::Log::Info(
                "Weather DDS loaded: " +
                path.string());
        }
        else
        {
            core::Log::Info(
                "Weather DDS failed: " +
                path.string() +
                " | " + error);
        }

        entries_.emplace(key, std::move(entry));

        TrimCache();

        return entries_.at(key).view.Get();
    }

    void StudioSkyTextures::TrimCache()
    {
        while (entries_.size() > MaxCacheEntries)
        {
            const auto oldest =
                std::min_element(
                    entries_.begin(),
                    entries_.end(),
                    [](const auto& a, const auto& b)
                    {
                        return a.second.lastUsed <
                            b.second.lastUsed;
                    });

            if (oldest == entries_.end())
                break;

            entries_.erase(oldest);
        }
    }
}