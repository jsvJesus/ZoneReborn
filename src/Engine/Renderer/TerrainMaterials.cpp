#include "Engine/Renderer/TerrainMaterials.h"

#include "Engine/Assets/DDSTextureLoader.h"

#include <d3dcompiler.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

#pragma comment(lib, "d3dcompiler.lib")

namespace engine::renderer {

    namespace {

        using Microsoft::WRL::ComPtr;

        struct MaterialConstants {
            float originX;
            float originZ;

            float inverseMaskX;
            float inverseMaskZ;

            float inverseTextureScale;
            float halfTexelX;
            float halfTexelZ;
            float normalStrength;
        };

        static_assert(sizeof(MaterialConstants) == 32);

        constexpr char shaderSource[] = R"(

            cbuffer CameraConstants : register(b0)
            {
                float4x4 viewProjection;
            };

            cbuffer MaterialConstants : register(b1)
            {
                float4 mapParameters;
                float4 materialParameters;
            };

            Texture2D maskTexture : register(t0);

            Texture2D grassDiffuse : register(t1);
            Texture2D soilDiffuse : register(t2);
            Texture2D sandDiffuse : register(t3);
            Texture2D rockDiffuse : register(t4);
            Texture2D marshDiffuse : register(t5);

            Texture2D grassNormal : register(t6);
            Texture2D soilNormal : register(t7);
            Texture2D sandNormal : register(t8);
            Texture2D rockNormal : register(t9);
            Texture2D marshNormal : register(t10);

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

            float3 unpack_normal(float3 encoded)
            {
                return encoded * 2.0f - 1.0f;
            }

            float4 PSMain(PixelInput input) : SV_TARGET
            {
                float2 maskUV = (
                    input.worldPosition.xz -
                    mapParameters.xy
                ) * mapParameters.zw +
                    materialParameters.yz;

                float4 weights = saturate(
                    maskTexture.Sample(
                        maskSampler,
                        saturate(maskUV)
                    )
                );

                float marshWeight = saturate(
                    1.0f -
                    dot(weights, float4(1.0f, 1.0f, 1.0f, 1.0f))
                );

                float totalWeight =
                    weights.r +
                    weights.g +
                    weights.b +
                    weights.a +
                    marshWeight;

                float inverseWeight = rcp(
                    max(totalWeight, 0.0001f)
                );

                float2 uv =
                    input.worldPosition.xz *
                    materialParameters.x;

                float3 grassColor = grassDiffuse.Sample(
                    tileSampler, uv
                ).rgb;

                float3 soilColor = soilDiffuse.Sample(
                    tileSampler, uv
                ).rgb;

                float3 sandColor = sandDiffuse.Sample(
                    tileSampler, uv
                ).rgb;

                float3 rockColor = rockDiffuse.Sample(
                    tileSampler, uv
                ).rgb;

                float3 marshColor = marshDiffuse.Sample(
                    tileSampler, uv
                ).rgb;

                float3 color = (
                    grassColor * weights.r +
                    soilColor * weights.g +
                    sandColor * weights.b +
                    rockColor * weights.a +
                    marshColor * marshWeight
                ) * inverseWeight;

                float3 grassN = unpack_normal(
                    grassNormal.Sample(tileSampler, uv).rgb
                );

                float3 soilN = unpack_normal(
                    soilNormal.Sample(tileSampler, uv).rgb
                );

                float3 sandN = unpack_normal(
                    sandNormal.Sample(tileSampler, uv).rgb
                );

                float3 rockN = unpack_normal(
                    rockNormal.Sample(tileSampler, uv).rgb
                );

                float3 marshN = unpack_normal(
                    marshNormal.Sample(tileSampler, uv).rgb
                );

                float3 detailNormal = normalize(
                    (
                        grassN * weights.r +
                        soilN * weights.g +
                        sandN * weights.b +
                        rockN * weights.a +
                        marshN * marshWeight
                    ) * inverseWeight
                );

                float3 geometryNormal =
                    normalize(input.normal);

                float3 referenceAxis =
                    abs(geometryNormal.x) < 0.95f
                        ? float3(1.0f, 0.0f, 0.0f)
                        : float3(0.0f, 0.0f, 1.0f);

                float3 tangent = normalize(
                    referenceAxis -
                    geometryNormal *
                    dot(geometryNormal, referenceAxis)
                );

                float3 bitangent = normalize(
                    cross(tangent, geometryNormal)
                );

                float3 surfaceNormal = normalize(
                    tangent * detailNormal.x *
                        materialParameters.w +
                    bitangent * detailNormal.y *
                        materialParameters.w +
                    geometryNormal *
                        max(detailNormal.z, 0.001f)
                );

                float3 lightDirection = normalize(
                    float3(-0.45f, 0.85f, 0.35f)
                );

                float diffuseLighting = saturate(
                    dot(surfaceNormal, lightDirection)
                );

                float lighting =
                    0.34f + diffuseLighting * 0.66f;

                return float4(
                    saturate(color * lighting),
                    1.0f
                );
            }

        )";

        void check(
            HRESULT result,
            const char* message
        ) {
            if (FAILED(result)) {
                throw std::runtime_error(message);
            }
        }

        [[nodiscard]] ComPtr<ID3DBlob> compile_shader(
            const char* entry,
            const char* target
        ) {
            ComPtr<ID3DBlob> shader;
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
                shader.GetAddressOf(),
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
                    "Failed to compile Terrain Material shader"
                );
            }

            return shader;
        }

    }

    TerrainMaterials::TerrainMaterials(
        ID3D11Device* device,
        ID3D11DeviceContext* context
    ) : device_(device),
        context_(context) {
        if (!device_ || !context_) {
            throw std::invalid_argument(
                "Invalid Terrain Material graphics device"
            );
        }
    }

    void TerrainMaterials::create_pipeline() {
        if (vertexShader_ &&
            pixelShader_ &&
            constants_ &&
            tileSampler_ &&
            maskSampler_) {
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
        ComPtr<ID3D11Buffer> constants;
        ComPtr<ID3D11SamplerState> tileSampler;
        ComPtr<ID3D11SamplerState> maskSampler;

        check(
            device_->CreateVertexShader(
                vs->GetBufferPointer(),
                vs->GetBufferSize(),
                nullptr,
                vertexShader.GetAddressOf()
            ),
            "Failed to create Terrain Material vertex shader"
        );

        check(
            device_->CreatePixelShader(
                ps->GetBufferPointer(),
                ps->GetBufferSize(),
                nullptr,
                pixelShader.GetAddressOf()
            ),
            "Failed to create Terrain Material pixel shader"
        );

        D3D11_BUFFER_DESC bufferDesc{};
        bufferDesc.ByteWidth = sizeof(MaterialConstants);
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        check(
            device_->CreateBuffer(
                &bufferDesc,
                nullptr,
                constants.GetAddressOf()
            ),
            "Failed to create Terrain Material constants"
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

        check(
            device_->CreateSamplerState(
                &samplerDesc,
                tileSampler.GetAddressOf()
            ),
            "Failed to create Terrain tile sampler"
        );

        samplerDesc.Filter =
            D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;

        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.MaxAnisotropy = 1;

        check(
            device_->CreateSamplerState(
                &samplerDesc,
                maskSampler.GetAddressOf()
            ),
            "Failed to create Terrain mask sampler"
        );

        vertexShader_ = std::move(vertexShader);
        pixelShader_ = std::move(pixelShader);
        constants_ = std::move(constants);
        tileSampler_ = std::move(tileSampler);
        maskSampler_ = std::move(maskSampler);
    }

    void TerrainMaterials::load(
        const TerrainMaterialMask& mask,
        const std::array<TerrainMaterialPaths, 5>& paths
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

        // Never keep stale materials after a failed reload.
        clear();

        create_pipeline();

        std::array<
            ComPtr<ID3D11ShaderResourceView>,
            5
        > newDiffuse;

        std::array<
            ComPtr<ID3D11ShaderResourceView>,
            5
        > newNormals;

        for (std::size_t i = 0; i < paths.size(); ++i) {
            newDiffuse[i] = assets::load_dds_texture(
                device_,
                paths[i].diffuse
            );

            newNormals[i] = assets::load_dds_texture(
                device_,
                paths[i].normal
            );
        }

        D3D11_TEXTURE2D_DESC maskDesc{};
        maskDesc.Width = mask.samplesX;
        maskDesc.Height = mask.samplesZ;
        maskDesc.MipLevels = 1;
        maskDesc.ArraySize = 1;
        maskDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        maskDesc.SampleDesc.Count = 1;
        maskDesc.Usage = D3D11_USAGE_DEFAULT;
        maskDesc.BindFlags =
            D3D11_BIND_SHADER_RESOURCE;

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
            "Failed to create Biome material mask"
        );

        ComPtr<ID3D11ShaderResourceView> maskView;

        check(
            device_->CreateShaderResourceView(
                maskTexture.Get(),
                nullptr,
                maskView.GetAddressOf()
            ),
            "Failed to create Biome mask view"
        );

        MaterialConstants parameters{};

        parameters.originX = mask.originX;
        parameters.originZ = mask.originZ;

        parameters.inverseMaskX = 1.0f / (
            static_cast<float>(mask.samplesX) *
            mask.spacingMeters
        );

        parameters.inverseMaskZ = 1.0f / (
            static_cast<float>(mask.samplesZ) *
            mask.spacingMeters
        );

        parameters.inverseTextureScale =
            1.0f / mask.textureScaleMeters;

        parameters.halfTexelX =
            0.5f / static_cast<float>(mask.samplesX);

        parameters.halfTexelZ =
            0.5f / static_cast<float>(mask.samplesZ);

        parameters.normalStrength = 1.0f;

        context_->UpdateSubresource(
            constants_.Get(),
            0,
            nullptr,
            &parameters,
            0,
            0
        );

        diffuse_ = std::move(newDiffuse);
        normals_ = std::move(newNormals);
        mask_ = std::move(maskView);
    }

    void TerrainMaterials::clear() noexcept {
        mask_.Reset();

        for (auto& texture : diffuse_) {
            texture.Reset();
        }

        for (auto& texture : normals_) {
            texture.Reset();
        }
    }

    bool TerrainMaterials::ready() const noexcept {
        if (!mask_ ||
            !vertexShader_ ||
            !pixelShader_ ||
            !constants_ ||
            !tileSampler_ ||
            !maskSampler_) {
            return false;
        }

        for (std::size_t i = 0; i < 5; ++i) {
            if (!diffuse_[i] || !normals_[i]) {
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

        ID3D11Buffer* constantBuffer =
            constants_.Get();

        context_->PSSetConstantBuffers(
            1,
            1,
            &constantBuffer
        );

        ID3D11ShaderResourceView* textures[11] = {
            mask_.Get(),

            diffuse_[0].Get(),
            diffuse_[1].Get(),
            diffuse_[2].Get(),
            diffuse_[3].Get(),
            diffuse_[4].Get(),

            normals_[0].Get(),
            normals_[1].Get(),
            normals_[2].Get(),
            normals_[3].Get(),
            normals_[4].Get()
        };

        context_->PSSetShaderResources(
            0,
            11,
            textures
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
        ID3D11ShaderResourceView* emptyTextures[11] = {};

        context_->PSSetShaderResources(
            0,
            11,
            emptyTextures
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
