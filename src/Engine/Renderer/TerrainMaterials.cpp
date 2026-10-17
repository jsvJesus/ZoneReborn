#include "Engine/Renderer/TerrainMaterials.h"

#include <Windows.h>
#include <wincodec.h>
#include <d3dcompiler.h>

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "windowscodecs.lib")

namespace engine::renderer {

    namespace {

        using Microsoft::WRL::ComPtr;

        struct MaterialConstants {
            float originX;
            float originZ;
            float inverseWidth;
            float inverseDepth;

            float inverseTextureScale;
            float padding[3];
        };

        constexpr char shaderSource[] = R"(

            cbuffer CameraConstants : register(b0)
            {
                float4x4 viewProjection;
            };

            cbuffer MaterialConstants : register(b1)
            {
                float4 mapParameters;
                float4 textureParameters;
            };

            Texture2D maskTexture : register(t0);
            Texture2D grassTexture : register(t1);
            Texture2D soilTexture : register(t2);
            Texture2D sandTexture : register(t3);
            Texture2D rockTexture : register(t4);
            Texture2D marshTexture : register(t5);

            SamplerState tileSampler : register(s0);
            SamplerState maskSampler : register(s1);

            struct VertexInput
            {
                float3 position : POSITION;
                float3 normal : NORMAL;
            };

            struct PixelInput
            {
                float4 position : SV_POSITION;
                float3 worldPosition : TEXCOORD0;
                float3 normal : TEXCOORD1;
            };

            PixelInput VSMain(VertexInput input)
            {
                PixelInput output;

                output.position = mul(
                    float4(input.position, 1.0f),
                    viewProjection
                );

                output.worldPosition = input.position;
                output.normal = input.normal;

                return output;
            }

            float4 PSMain(PixelInput input) : SV_TARGET
            {
                float2 maskUV = (
                    input.worldPosition.xz -
                    mapParameters.xy
                ) * mapParameters.zw;

                float4 weights = maskTexture.Sample(
                    maskSampler,
                    saturate(maskUV)
                );

                float marshWeight = saturate(
                    1.0f -
                    weights.r -
                    weights.g -
                    weights.b -
                    weights.a
                );

                float totalWeight =
                    weights.r +
                    weights.g +
                    weights.b +
                    weights.a +
                    marshWeight;

                float2 tiledUV =
                    input.worldPosition.xz *
                    textureParameters.x;

                float3 grass = grassTexture.Sample(
                    tileSampler,
                    tiledUV
                ).rgb;

                float3 soil = soilTexture.Sample(
                    tileSampler,
                    tiledUV
                ).rgb;

                float3 sand = sandTexture.Sample(
                    tileSampler,
                    tiledUV
                ).rgb;

                float3 rock = rockTexture.Sample(
                    tileSampler,
                    tiledUV
                ).rgb;

                float3 marsh = marshTexture.Sample(
                    tileSampler,
                    tiledUV
                ).rgb;

                float3 color =
                    grass * weights.r +
                    soil * weights.g +
                    sand * weights.b +
                    rock * weights.a +
                    marsh * marshWeight;

                color /= max(totalWeight, 0.0001f);

                float3 normal = normalize(input.normal);

                float3 lightDirection = normalize(
                    float3(-0.45f, 0.85f, 0.35f)
                );

                float lighting =
                    0.35f +
                    saturate(
                        dot(normal, lightDirection)
                    ) * 0.65f;

                return float4(color * lighting, 1.0f);
            }

        )";

        void check(HRESULT result, const char* message) {
            if (FAILED(result)) {
                throw std::runtime_error(message);
            }
        }

        [[nodiscard]] ComPtr<ID3DBlob> compile_shader(
            const char* entry,
            const char* target
        ) {
            ComPtr<ID3DBlob> bytecode;
            ComPtr<ID3DBlob> errors;

            const HRESULT result = D3DCompile(
                shaderSource,
                sizeof(shaderSource) - 1,
                nullptr,
                nullptr,
                nullptr,
                entry,
                target,
                D3DCOMPILE_ENABLE_STRICTNESS,
                0,
                bytecode.GetAddressOf(),
                errors.GetAddressOf()
            );

            if (FAILED(result)) {
                if (errors) {
                    throw std::runtime_error(
                        std::string(
                            static_cast<const char*>(
                                errors->GetBufferPointer()
                            ),
                            errors->GetBufferSize()
                        )
                    );
                }

                throw std::runtime_error(
                    "Terrain material shader compilation failed"
                );
            }

            return bytecode;
        }

        class ComApartment final {
        public:
            ComApartment() {
                result_ = CoInitializeEx(
                    nullptr,
                    COINIT_MULTITHREADED
                );

                if (FAILED(result_) &&
                    result_ != RPC_E_CHANGED_MODE) {
                    throw std::runtime_error(
                        "Failed to initialize COM"
                    );
                }
            }

            ~ComApartment() {
                if (SUCCEEDED(result_)) {
                    CoUninitialize();
                }
            }

            ComApartment(const ComApartment&) = delete;
            ComApartment& operator=(const ComApartment&) = delete;

        private:
            HRESULT result_ = E_FAIL;
        };

    }

    TerrainMaterials::TerrainMaterials(
        ID3D11Device* device,
        ID3D11DeviceContext* context
    ) : device_(device), context_(context) {
        if (!device_ || !context_) {
            throw std::invalid_argument(
                "Invalid Terrain Materials device"
            );
        }
    }

    void TerrainMaterials::create_pipeline() {
        if (vertexShader_ && pixelShader_) {
            return;
        }

        const auto vs = compile_shader(
            "VSMain",
            "vs_5_0"
        );

        const auto ps = compile_shader(
            "PSMain",
            "ps_5_0"
        );

        ComPtr<ID3D11VertexShader> vertexShader;
        ComPtr<ID3D11PixelShader> pixelShader;

        check(
            device_->CreateVertexShader(
                vs->GetBufferPointer(),
                vs->GetBufferSize(),
                nullptr,
                vertexShader.GetAddressOf()
            ),
            "Failed to create material vertex shader"
        );

        check(
            device_->CreatePixelShader(
                ps->GetBufferPointer(),
                ps->GetBufferSize(),
                nullptr,
                pixelShader.GetAddressOf()
            ),
            "Failed to create material pixel shader"
        );

        D3D11_BUFFER_DESC constantDesc{};
        constantDesc.ByteWidth = sizeof(MaterialConstants);
        constantDesc.Usage = D3D11_USAGE_DEFAULT;
        constantDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        ComPtr<ID3D11Buffer> constants;

        check(
            device_->CreateBuffer(
                &constantDesc,
                nullptr,
                constants.GetAddressOf()
            ),
            "Failed to create material constant buffer"
        );

        D3D11_SAMPLER_DESC samplerDesc{};
        samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.MaxAnisotropy = 8;
        samplerDesc.MinLOD = 0.0f;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

        ComPtr<ID3D11SamplerState> tileSampler;

        check(
            device_->CreateSamplerState(
                &samplerDesc,
                tileSampler.GetAddressOf()
            ),
            "Failed to create material tile sampler"
        );

        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.MaxAnisotropy = 1;

        ComPtr<ID3D11SamplerState> maskSampler;

        check(
            device_->CreateSamplerState(
                &samplerDesc,
                maskSampler.GetAddressOf()
            ),
            "Failed to create material mask sampler"
        );

        vertexShader_ = std::move(vertexShader);
        pixelShader_ = std::move(pixelShader);
        constants_ = std::move(constants);
        tileSampler_ = std::move(tileSampler);
        maskSampler_ = std::move(maskSampler);
    }

    ComPtr<ID3D11ShaderResourceView>
    TerrainMaterials::load_texture(
        const std::filesystem::path& path
    ) const {
        if (!std::filesystem::is_regular_file(path)) {
            throw std::runtime_error(
                "Terrain material not found: " +
                path.string()
            );
        }

        ComApartment apartment;

        ComPtr<IWICImagingFactory> factory;

        check(
            CoCreateInstance(
                CLSID_WICImagingFactory,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(factory.GetAddressOf())
            ),
            "Failed to create WIC factory"
        );

        ComPtr<IWICBitmapDecoder> decoder;

        check(
            factory->CreateDecoderFromFilename(
                path.c_str(),
                nullptr,
                GENERIC_READ,
                WICDecodeMetadataCacheOnDemand,
                decoder.GetAddressOf()
            ),
            "Failed to decode material texture"
        );

        ComPtr<IWICBitmapFrameDecode> frame;

        check(
            decoder->GetFrame(
                0,
                frame.GetAddressOf()
            ),
            "Failed to read material frame"
        );

        UINT width = 0;
        UINT height = 0;

        check(
            frame->GetSize(&width, &height),
            "Failed to read texture dimensions"
        );

        if (width == 0 || height == 0 ||
            width > 8192 || height > 8192) {
            throw std::runtime_error(
                "Invalid terrain material dimensions"
            );
        }

        ComPtr<IWICFormatConverter> converter;

        check(
            factory->CreateFormatConverter(
                converter.GetAddressOf()
            ),
            "Failed to create format converter"
        );

        check(
            converter->Initialize(
                frame.Get(),
                GUID_WICPixelFormat32bppRGBA,
                WICBitmapDitherTypeNone,
                nullptr,
                0.0,
                WICBitmapPaletteTypeCustom
            ),
            "Failed to convert material to RGBA8"
        );

        const UINT pitch = width * 4;
        const UINT byteCount = pitch * height;

        std::vector<std::uint8_t> pixels(byteCount);

        check(
            converter->CopyPixels(
                nullptr,
                pitch,
                byteCount,
                pixels.data()
            ),
            "Failed to read material pixels"
        );

        D3D11_TEXTURE2D_DESC textureDesc{};
        textureDesc.Width = width;
        textureDesc.Height = height;
        textureDesc.MipLevels = 0;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags =
            D3D11_BIND_SHADER_RESOURCE |
            D3D11_BIND_RENDER_TARGET;

        textureDesc.MiscFlags =
            D3D11_RESOURCE_MISC_GENERATE_MIPS;

        ComPtr<ID3D11Texture2D> texture;

        check(
            device_->CreateTexture2D(
                &textureDesc,
                nullptr,
                texture.GetAddressOf()
            ),
            "Failed to create material texture"
        );

        context_->UpdateSubresource(
            texture.Get(),
            0,
            nullptr,
            pixels.data(),
            pitch,
            0
        );

        ComPtr<ID3D11ShaderResourceView> view;

        check(
            device_->CreateShaderResourceView(
                texture.Get(),
                nullptr,
                view.GetAddressOf()
            ),
            "Failed to create material resource view"
        );

        context_->GenerateMips(view.Get());

        return view;
    }

    void TerrainMaterials::load(
        const TerrainMaterialMask& mask,
        const std::array<std::filesystem::path, 5>& paths
    ) {
        if (mask.samplesX < 2 ||
            mask.samplesZ < 2 ||
            !std::isfinite(mask.originX) ||
            !std::isfinite(mask.originZ) ||
            !std::isfinite(mask.spacingMeters) ||
            !std::isfinite(mask.textureScaleMeters) ||
            mask.spacingMeters <= 0.0f ||
            mask.textureScaleMeters <= 0.0f ||
            mask.weights.size() !=
                static_cast<std::size_t>(mask.samplesX) *
                mask.samplesZ * 4) {
            throw std::invalid_argument(
                "Invalid Terrain Material mask"
            );
        }

        // Previous surface must not remain attached
        // after a failed material reload.
        mask_.Reset();

        create_pipeline();

        std::array<
            ComPtr<ID3D11ShaderResourceView>,
            5
        > newTextures;

        for (std::size_t i = 0; i < newTextures.size(); ++i) {
            newTextures[i] = load_texture(paths[i]);
        }

        D3D11_TEXTURE2D_DESC maskDesc{};
        maskDesc.Width = mask.samplesX;
        maskDesc.Height = mask.samplesZ;
        maskDesc.MipLevels = 1;
        maskDesc.ArraySize = 1;
        maskDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        maskDesc.SampleDesc.Count = 1;
        maskDesc.Usage = D3D11_USAGE_DEFAULT;
        maskDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA maskData{};
        maskData.pSysMem = mask.weights.data();
        maskData.SysMemPitch = mask.samplesX * 4;

        ComPtr<ID3D11Texture2D> maskTexture;

        check(
            device_->CreateTexture2D(
                &maskDesc,
                &maskData,
                maskTexture.GetAddressOf()
            ),
            "Failed to create biome weight texture"
        );

        ComPtr<ID3D11ShaderResourceView> maskView;

        check(
            device_->CreateShaderResourceView(
                maskTexture.Get(),
                nullptr,
                maskView.GetAddressOf()
            ),
            "Failed to create biome weight SRV"
        );

        MaterialConstants parameters{};

        parameters.originX = mask.originX;
        parameters.originZ = mask.originZ;

        parameters.inverseWidth = 1.0f / (
            static_cast<float>(mask.samplesX - 1) *
            mask.spacingMeters
        );

        parameters.inverseDepth = 1.0f / (
            static_cast<float>(mask.samplesZ - 1) *
            mask.spacingMeters
        );

        parameters.inverseTextureScale =
            1.0f / mask.textureScaleMeters;

        context_->UpdateSubresource(
            constants_.Get(),
            0,
            nullptr,
            &parameters,
            0,
            0
        );

        textures_ = std::move(newTextures);
        mask_ = std::move(maskView);
    }

    void TerrainMaterials::clear() noexcept {
        mask_.Reset();
    }

    bool TerrainMaterials::ready() const noexcept {
        if (!mask_ || !vertexShader_ || !pixelShader_) {
            return false;
        }

        for (const auto& texture : textures_) {
            if (!texture) {
                return false;
            }
        }

        return true;
    }

    void TerrainMaterials::bind() const {
        if (!ready()) {
            return;
        }

        context_->VSSetShader(
            vertexShader_.Get(),
            nullptr,
            0
        );

        context_->PSSetShader(
            pixelShader_.Get(),
            nullptr,
            0
        );

        ID3D11Buffer* materialConstants = constants_.Get();

        context_->PSSetConstantBuffers(
            1,
            1,
            &materialConstants
        );

        ID3D11ShaderResourceView* views[6] = {
            mask_.Get(),
            textures_[0].Get(),
            textures_[1].Get(),
            textures_[2].Get(),
            textures_[3].Get(),
            textures_[4].Get()
        };

        context_->PSSetShaderResources(
            0,
            6,
            views
        );

        ID3D11SamplerState* samplers[2] = {
            tileSampler_.Get(),
            maskSampler_.Get()
        };

        context_->PSSetSamplers(
            0,
            2,
            samplers
        );
    }

    void TerrainMaterials::unbind() const noexcept {
        ID3D11ShaderResourceView* emptyViews[6] = {};

        context_->PSSetShaderResources(
            0,
            6,
            emptyViews
        );

        ID3D11SamplerState* emptySamplers[2] = {};

        context_->PSSetSamplers(
            0,
            2,
            emptySamplers
        );

        ID3D11Buffer* emptyBuffer = nullptr;

        context_->PSSetConstantBuffers(
            1,
            1,
            &emptyBuffer
        );
    }

}
