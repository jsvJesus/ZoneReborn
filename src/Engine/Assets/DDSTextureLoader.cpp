#include "Engine/Assets/DDSTextureLoader.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace engine::assets {

    namespace {

        using Microsoft::WRL::ComPtr;

        constexpr std::uint32_t ddsMagic = 0x20534444u;
        constexpr std::uint32_t flagFourCC = 0x00000004u;
        constexpr std::uint32_t flagRGB = 0x00000040u;

        constexpr std::uint32_t capsCubemap = 0x00000200u;
        constexpr std::uint32_t capsVolume = 0x00200000u;

        constexpr std::uint32_t dimensionTexture2D = 3u;
        constexpr std::uint32_t miscTextureCube = 0x4u;

        constexpr std::uint64_t maxFileBytes =
            256ull * 1024ull * 1024ull;

        struct DDSPixelFormat {
            std::uint32_t size;
            std::uint32_t flags;
            std::uint32_t fourCC;
            std::uint32_t rgbBitCount;
            std::uint32_t rMask;
            std::uint32_t gMask;
            std::uint32_t bMask;
            std::uint32_t aMask;
        };

        struct DDSHeader {
            std::uint32_t size;
            std::uint32_t flags;
            std::uint32_t height;
            std::uint32_t width;
            std::uint32_t pitchOrLinearSize;
            std::uint32_t depth;
            std::uint32_t mipMapCount;
            std::uint32_t reserved[11];

            DDSPixelFormat pixelFormat;

            std::uint32_t caps;
            std::uint32_t caps2;
            std::uint32_t caps3;
            std::uint32_t caps4;
            std::uint32_t reserved2;
        };

        struct DDSHeaderDX10 {
            std::uint32_t dxgiFormat;
            std::uint32_t resourceDimension;
            std::uint32_t miscFlag;
            std::uint32_t arraySize;
            std::uint32_t miscFlags2;
        };

        static_assert(sizeof(DDSPixelFormat) == 32);
        static_assert(sizeof(DDSHeader) == 124);
        static_assert(sizeof(DDSHeaderDX10) == 20);

        [[nodiscard]] constexpr std::uint32_t fourcc(
            char a,
            char b,
            char c,
            char d
        ) noexcept {
            return static_cast<std::uint32_t>(
                static_cast<std::uint8_t>(a)
            ) |
            (
                static_cast<std::uint32_t>(
                    static_cast<std::uint8_t>(b)
                ) << 8
            ) |
            (
                static_cast<std::uint32_t>(
                    static_cast<std::uint8_t>(c)
                ) << 16
            ) |
            (
                static_cast<std::uint32_t>(
                    static_cast<std::uint8_t>(d)
                ) << 24
            );
        }

        [[nodiscard]] std::vector<std::uint8_t> read_file(
            const std::filesystem::path& path
        ) {
            std::ifstream stream(
                path,
                std::ios::binary | std::ios::ate
            );

            if (!stream) {
                throw std::runtime_error(
                    "Unable to open DDS texture: " +
                    path.string()
                );
            }

            const std::streampos end = stream.tellg();

            if (end < std::streampos(128) ||
                static_cast<std::uint64_t>(end) > maxFileBytes) {
                throw std::runtime_error(
                    "Invalid DDS file size: " +
                    path.string()
                );
            }

            const std::size_t size =
                static_cast<std::size_t>(end);

            std::vector<std::uint8_t> data(size);

            stream.seekg(0, std::ios::beg);

            stream.read(
                reinterpret_cast<char*>(data.data()),
                static_cast<std::streamsize>(data.size())
            );

            if (!stream) {
                throw std::runtime_error(
                    "Failed to read DDS texture: " +
                    path.string()
                );
            }

            return data;
        }

        template<typename T>
        [[nodiscard]] T read_structure(
            const std::vector<std::uint8_t>& bytes,
            std::size_t offset
        ) {
            if (offset > bytes.size() ||
                sizeof(T) > bytes.size() - offset) {
                throw std::runtime_error(
                    "Truncated DDS header"
                );
            }

            T result{};

            std::memcpy(
                &result,
                bytes.data() + offset,
                sizeof(T)
            );

            return result;
        }

        [[nodiscard]] DXGI_FORMAT legacy_format(
            const DDSPixelFormat& format
        ) {
            if ((format.flags & flagFourCC) != 0) {
                switch (format.fourCC) {
                case fourcc('D', 'X', 'T', '1'):
                    return DXGI_FORMAT_BC1_UNORM;

                case fourcc('D', 'X', 'T', '3'):
                    return DXGI_FORMAT_BC2_UNORM;

                case fourcc('D', 'X', 'T', '5'):
                    return DXGI_FORMAT_BC3_UNORM;

                case fourcc('A', 'T', 'I', '1'):
                case fourcc('B', 'C', '4', 'U'):
                    return DXGI_FORMAT_BC4_UNORM;

                case fourcc('A', 'T', 'I', '2'):
                case fourcc('B', 'C', '5', 'U'):
                    return DXGI_FORMAT_BC5_UNORM;

                default:
                    return DXGI_FORMAT_UNKNOWN;
                }
            }

            if ((format.flags & flagRGB) == 0 ||
                format.rgbBitCount != 32) {
                return DXGI_FORMAT_UNKNOWN;
            }

            if (format.rMask == 0x000000FFu &&
                format.gMask == 0x0000FF00u &&
                format.bMask == 0x00FF0000u &&
                format.aMask == 0xFF000000u) {
                return DXGI_FORMAT_R8G8B8A8_UNORM;
            }

            if (format.rMask == 0x00FF0000u &&
                format.gMask == 0x0000FF00u &&
                format.bMask == 0x000000FFu) {
                if (format.aMask == 0xFF000000u) {
                    return DXGI_FORMAT_B8G8R8A8_UNORM;
                }

                if (format.aMask == 0) {
                    return DXGI_FORMAT_B8G8R8X8_UNORM;
                }
            }

            return DXGI_FORMAT_UNKNOWN;
        }

        [[nodiscard]] bool supported_format(
            DXGI_FORMAT format
        ) noexcept {
            switch (format) {
            case DXGI_FORMAT_BC1_UNORM:
            case DXGI_FORMAT_BC1_UNORM_SRGB:
            case DXGI_FORMAT_BC2_UNORM:
            case DXGI_FORMAT_BC2_UNORM_SRGB:
            case DXGI_FORMAT_BC3_UNORM:
            case DXGI_FORMAT_BC3_UNORM_SRGB:
            case DXGI_FORMAT_BC4_UNORM:
            case DXGI_FORMAT_BC4_SNORM:
            case DXGI_FORMAT_BC5_UNORM:
            case DXGI_FORMAT_BC5_SNORM:
            case DXGI_FORMAT_BC6H_UF16:
            case DXGI_FORMAT_BC6H_SF16:
            case DXGI_FORMAT_BC7_UNORM:
            case DXGI_FORMAT_BC7_UNORM_SRGB:
            case DXGI_FORMAT_R8_UNORM:
            case DXGI_FORMAT_R8G8_UNORM:
            case DXGI_FORMAT_R8G8B8A8_UNORM:
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            case DXGI_FORMAT_B8G8R8A8_UNORM:
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            case DXGI_FORMAT_B8G8R8X8_UNORM:
            case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
                return true;

            default:
                return false;
            }
        }

        [[nodiscard]] std::uint32_t block_bytes(
            DXGI_FORMAT format
        ) noexcept {
            switch (format) {
            case DXGI_FORMAT_BC1_UNORM:
            case DXGI_FORMAT_BC1_UNORM_SRGB:
            case DXGI_FORMAT_BC4_UNORM:
            case DXGI_FORMAT_BC4_SNORM:
                return 8;

            case DXGI_FORMAT_BC2_UNORM:
            case DXGI_FORMAT_BC2_UNORM_SRGB:
            case DXGI_FORMAT_BC3_UNORM:
            case DXGI_FORMAT_BC3_UNORM_SRGB:
            case DXGI_FORMAT_BC5_UNORM:
            case DXGI_FORMAT_BC5_SNORM:
            case DXGI_FORMAT_BC6H_UF16:
            case DXGI_FORMAT_BC6H_SF16:
            case DXGI_FORMAT_BC7_UNORM:
            case DXGI_FORMAT_BC7_UNORM_SRGB:
                return 16;

            default:
                return 0;
            }
        }

        [[nodiscard]] std::uint32_t pixel_bytes(
            DXGI_FORMAT format
        ) noexcept {
            switch (format) {
            case DXGI_FORMAT_R8_UNORM:
                return 1;

            case DXGI_FORMAT_R8G8_UNORM:
                return 2;

            case DXGI_FORMAT_R8G8B8A8_UNORM:
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            case DXGI_FORMAT_B8G8R8A8_UNORM:
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            case DXGI_FORMAT_B8G8R8X8_UNORM:
            case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
                return 4;

            default:
                return 0;
            }
        }

        void check(
            HRESULT result,
            const char* message
        ) {
            if (FAILED(result)) {
                throw std::runtime_error(message);
            }
        }

    }

    ComPtr<ID3D11ShaderResourceView> load_dds_texture(
        ID3D11Device* device,
        const std::filesystem::path& path
    ) {
        if (!device) {
            throw std::invalid_argument(
                "DDS Loader requires a DirectX device"
            );
        }

        const auto bytes = read_file(path);

        const std::uint32_t signature =
            read_structure<std::uint32_t>(bytes, 0);

        if (signature != ddsMagic) {
            throw std::runtime_error(
                "Invalid DDS signature: " +
                path.string()
            );
        }

        const DDSHeader header =
            read_structure<DDSHeader>(
                bytes,
                sizeof(std::uint32_t)
            );

        if (header.size != sizeof(DDSHeader) ||
            header.pixelFormat.size != sizeof(DDSPixelFormat) ||
            header.width == 0 ||
            header.height == 0 ||
            header.width > 8192 ||
            header.height > 8192 ||
            (header.caps2 & (capsCubemap | capsVolume)) != 0 ||
            header.depth > 1) {
            throw std::runtime_error(
                "Unsupported DDS texture dimensions: " +
                path.string()
            );
        }

        std::size_t dataOffset =
            sizeof(std::uint32_t) + sizeof(DDSHeader);

        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

        if ((header.pixelFormat.flags & flagFourCC) != 0 &&
            header.pixelFormat.fourCC ==
                fourcc('D', 'X', '1', '0')) {
            const DDSHeaderDX10 dx10 =
                read_structure<DDSHeaderDX10>(
                    bytes,
                    dataOffset
                );

            dataOffset += sizeof(DDSHeaderDX10);

            if (dx10.resourceDimension != dimensionTexture2D ||
                dx10.arraySize != 1 ||
                (dx10.miscFlag & miscTextureCube) != 0) {
                throw std::runtime_error(
                    "Only 2D DDS textures are supported: " +
                    path.string()
                );
            }

            format = static_cast<DXGI_FORMAT>(
                dx10.dxgiFormat
            );
        } else {
            format = legacy_format(header.pixelFormat);
        }

        if (!supported_format(format)) {
            throw std::runtime_error(
                "Unsupported DDS pixel format: " +
                path.string()
            );
        }

        const std::uint32_t mipCount =
            header.mipMapCount == 0
                ? 1
                : header.mipMapCount;

        std::uint32_t maximumDimension =
            std::max(header.width, header.height);

        std::uint32_t maximumMips = 0;

        while (maximumDimension > 0) {
            ++maximumMips;
            maximumDimension >>= 1;
        }

        if (mipCount > maximumMips) {
            throw std::runtime_error(
                "Invalid DDS mipmap count: " +
                path.string()
            );
        }

        const std::uint32_t compressedBlockBytes =
            block_bytes(format);

        const std::uint32_t uncompressedPixelBytes =
            pixel_bytes(format);

        std::vector<D3D11_SUBRESOURCE_DATA> subresources;
        subresources.reserve(mipCount);

        std::uint32_t mipWidth = header.width;
        std::uint32_t mipHeight = header.height;

        for (std::uint32_t level = 0;
             level < mipCount;
             ++level) {
            std::uint64_t rowPitch = 0;
            std::uint64_t rowCount = 0;

            if (compressedBlockBytes != 0) {
                rowPitch = static_cast<std::uint64_t>(
                    std::max(1u, (mipWidth + 3u) / 4u)
                ) * compressedBlockBytes;

                rowCount = std::max(
                    1u,
                    (mipHeight + 3u) / 4u
                );
            } else {
                rowPitch =
                    static_cast<std::uint64_t>(mipWidth) *
                    uncompressedPixelBytes;

                rowCount = mipHeight;
            }

            const std::uint64_t dataSize =
                rowPitch * rowCount;

            if (rowPitch >
                    std::numeric_limits<UINT>::max() ||
                dataOffset > bytes.size() ||
                dataSize > bytes.size() - dataOffset) {
                throw std::runtime_error(
                    "DDS mipmap data is truncated: " +
                    path.string()
                );
            }

            D3D11_SUBRESOURCE_DATA subresource{};

            subresource.pSysMem =
                bytes.data() + dataOffset;

            subresource.SysMemPitch =
                static_cast<UINT>(rowPitch);

            subresource.SysMemSlicePitch = 0;

            subresources.push_back(subresource);

            dataOffset += static_cast<std::size_t>(
                dataSize
            );

            mipWidth = std::max(1u, mipWidth / 2u);
            mipHeight = std::max(1u, mipHeight / 2u);
        }

        D3D11_TEXTURE2D_DESC description{};

        description.Width = header.width;
        description.Height = header.height;
        description.MipLevels = mipCount;
        description.ArraySize = 1;
        description.Format = format;
        description.SampleDesc.Count = 1;
        description.Usage = D3D11_USAGE_IMMUTABLE;
        description.BindFlags =
            D3D11_BIND_SHADER_RESOURCE;

        ComPtr<ID3D11Texture2D> texture;

        check(
            device->CreateTexture2D(
                &description,
                subresources.data(),
                texture.GetAddressOf()
            ),
            "Failed to create DDS GPU texture"
        );

        D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc{};

        viewDesc.Format = format;
        viewDesc.ViewDimension =
            D3D11_SRV_DIMENSION_TEXTURE2D;

        viewDesc.Texture2D.MostDetailedMip = 0;
        viewDesc.Texture2D.MipLevels = mipCount;

        ComPtr<ID3D11ShaderResourceView> view;

        check(
            device->CreateShaderResourceView(
                texture.Get(),
                &viewDesc,
                view.GetAddressOf()
            ),
            "Failed to create DDS shader resource view"
        );

        return view;
    }

}
