#include "Engine/Renderer/SurfaceRenderer.h"

#include "Engine/Assets/DDSTextureLoader.h"

#include <d3dcompiler.h>

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>

#pragma comment(lib, "d3dcompiler.lib")

namespace engine::renderer {

    namespace {

        using Microsoft::WRL::ComPtr;

        struct CameraConstants {
            DirectX::XMFLOAT4X4 viewProjection;
        };

        struct MaterialConstants {
            DirectX::XMFLOAT4 tintAndMode;
            DirectX::XMFLOAT4 animation;
        };

        constexpr char shaderSource[] = R"(
            cbuffer CameraConstants : register(b0) {
                float4x4 viewProjection;
            };

            cbuffer MaterialConstants : register(b1) {
                float4 tintAndMode;
                float4 animation;
            };

            Texture2D diffuseTexture : register(t0);
            Texture2D normalTexture : register(t1);
            SamplerState textureSampler : register(s0);

            struct VertexInput {
                float3 position : POSITION;
                float2 uv : TEXCOORD0;
            };

            struct PixelInput {
                float4 position : SV_POSITION;
                float3 worldPosition : TEXCOORD0;
                float2 uv : TEXCOORD1;
            };

            PixelInput VSMain(VertexInput input) {
                PixelInput output;
                output.position = mul(float4(input.position, 1.0f), viewProjection);
                output.worldPosition = input.position;
                output.uv = input.uv;
                return output;
            }

            float4 PSMain(PixelInput input) : SV_TARGET {
                float2 uv = input.uv;
                float3 ripple = float3(0.5f, 0.5f, 1.0f);

                if (tintAndMode.w > 0.5f) {
                    float2 drift = float2(animation.x, animation.x * 0.37f);
                    float3 first = normalTexture.Sample(
                        textureSampler, uv + drift
                    ).rgb;
                    float3 second = normalTexture.Sample(
                        textureSampler, uv * 1.41f - drift * 0.65f
                    ).rgb;
                    ripple = (first + second) * 0.5f;
                    uv += (ripple.xy - 0.5f) * 0.055f;
                }

                float3 diffuse = diffuseTexture.Sample(
                    textureSampler, uv
                ).rgb;
                float3 detailNormal = normalTexture.Sample(
                    textureSampler, uv
                ).rgb;
                float3 normal = normalize(cross(
                    ddy(input.worldPosition),
                    ddx(input.worldPosition)
                ));
                float3 lightDirection = normalize(
                    float3(-0.45f, 0.85f, 0.35f)
                );
                float lighting = 0.55f + 0.45f * abs(
                    dot(normal, lightDirection)
                );
                lighting *= 0.90f + detailNormal.z * 0.18f;

                if (tintAndMode.w > 0.5f) {
                    float shimmer = saturate(ripple.x * 0.35f +
                        ripple.y * 0.25f);
                    float3 color = lerp(
                        diffuse * tintAndMode.rgb,
                        tintAndMode.rgb,
                        0.72f
                    );
                    return float4(
                        color * (0.78f + shimmer * 0.30f) * lighting,
                        1.0f
                    );
                }

                return float4(
                    diffuse * tintAndMode.rgb * lighting,
                    1.0f
                );
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
            ComPtr<ID3DBlob> code;
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
                code.GetAddressOf(),
                errors.GetAddressOf()
            );

            if (FAILED(result)) {
                throw std::runtime_error(errors
                    ? std::string(
                        static_cast<const char*>(errors->GetBufferPointer()),
                        errors->GetBufferSize()
                    )
                    : "Failed to compile surface shader"
                );
            }

            return code;
        }

        [[nodiscard]] ComPtr<ID3D11Buffer> create_buffer(
            ID3D11Device* device,
            const void* data,
            std::size_t size,
            UINT bindFlags
        ) {
            if (!data || size == 0 ||
                size > (std::numeric_limits<UINT>::max)()) {
                throw std::invalid_argument("Invalid surface GPU buffer");
            }

            D3D11_BUFFER_DESC desc{};
            desc.ByteWidth = static_cast<UINT>(size);
            desc.Usage = D3D11_USAGE_IMMUTABLE;
            desc.BindFlags = bindFlags;

            D3D11_SUBRESOURCE_DATA initial{};
            initial.pSysMem = data;

            ComPtr<ID3D11Buffer> buffer;
            check(
                device->CreateBuffer(&desc, &initial, buffer.GetAddressOf()),
                "Failed to create surface GPU buffer"
            );
            return buffer;
        }

    }

    SurfaceRenderer::SurfaceRenderer(
        ID3D11Device* device,
        ID3D11DeviceContext* context
    ) : device_(device), context_(context) {
        if (!device_ || !context_) {
            throw std::invalid_argument("Invalid surface rendering device");
        }

        const auto vertexCode = compile_shader("VSMain", "vs_5_0");
        const auto pixelCode = compile_shader("PSMain", "ps_5_0");

        check(device_->CreateVertexShader(
            vertexCode->GetBufferPointer(),
            vertexCode->GetBufferSize(),
            nullptr,
            vertexShader_.GetAddressOf()
        ), "Failed to create surface vertex shader");

        check(device_->CreatePixelShader(
            pixelCode->GetBufferPointer(),
            pixelCode->GetBufferSize(),
            nullptr,
            pixelShader_.GetAddressOf()
        ), "Failed to create surface pixel shader");

        constexpr D3D11_INPUT_ELEMENT_DESC layout[] = {
            {
                "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
                0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0
            },
            {
                "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,
                0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0
            }
        };

        check(device_->CreateInputLayout(
            layout,
            2,
            vertexCode->GetBufferPointer(),
            vertexCode->GetBufferSize(),
            inputLayout_.GetAddressOf()
        ), "Failed to create surface input layout");

        D3D11_BUFFER_DESC constantDesc{};
        constantDesc.ByteWidth = sizeof(CameraConstants);
        constantDesc.Usage = D3D11_USAGE_DEFAULT;
        constantDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        check(device_->CreateBuffer(
            &constantDesc, nullptr, cameraConstants_.GetAddressOf()
        ), "Failed to create surface camera buffer");

        constantDesc.ByteWidth = sizeof(MaterialConstants);
        check(device_->CreateBuffer(
            &constantDesc, nullptr, materialConstants_.GetAddressOf()
        ), "Failed to create surface material buffer");

        D3D11_SAMPLER_DESC samplerDesc{};
        samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.MaxAnisotropy = 8;
        samplerDesc.MinLOD = 0.0f;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        check(device_->CreateSamplerState(
            &samplerDesc, sampler_.GetAddressOf()
        ), "Failed to create surface sampler");

        D3D11_RASTERIZER_DESC rasterDesc{};
        rasterDesc.FillMode = D3D11_FILL_SOLID;
        rasterDesc.CullMode = D3D11_CULL_NONE;
        rasterDesc.DepthClipEnable = TRUE;
        check(device_->CreateRasterizerState(
            &rasterDesc, rasterizer_.GetAddressOf()
        ), "Failed to create surface rasterizer");
    }

    void SurfaceRenderer::set_mesh(
        const SurfaceMesh& mesh,
        const SurfaceMaterial& material
    ) {
        if (mesh.vertices.empty() || mesh.indices.empty()) {
            clear();
            return;
        }

        const auto newVertices = create_buffer(
            device_, mesh.vertices.data(),
            mesh.vertices.size() * sizeof(SurfaceVertex),
            D3D11_BIND_VERTEX_BUFFER
        );
        const auto newIndices = create_buffer(
            device_, mesh.indices.data(),
            mesh.indices.size() * sizeof(std::uint32_t),
            D3D11_BIND_INDEX_BUFFER
        );
        const auto newDiffuse = assets::load_dds_texture(
            device_, material.diffuse
        );
        const auto newNormal = assets::load_dds_texture(
            device_, material.normal
        );

        vertices_ = newVertices;
        indices_ = newIndices;
        diffuse_ = newDiffuse;
        normal_ = newNormal;
        tint_ = material.tint;
        animatedWater_ = material.animatedWater;
        indexCount_ = static_cast<std::uint32_t>(mesh.indices.size());
    }

    void SurfaceRenderer::clear() noexcept {
        vertices_.Reset();
        indices_.Reset();
        diffuse_.Reset();
        normal_.Reset();
        indexCount_ = 0;
    }

    void SurfaceRenderer::set_visible(bool visible) noexcept {
        visible_ = visible;
    }

    std::uint32_t SurfaceRenderer::render(
        DirectX::FXMMATRIX viewProjection
    ) {
        if (!visible_ || indexCount_ == 0) {
            return 0;
        }

        CameraConstants camera{};
        DirectX::XMStoreFloat4x4(
            &camera.viewProjection,
            DirectX::XMMatrixTranspose(viewProjection)
        );

        const float seconds = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - startTime_
        ).count();
        MaterialConstants material{};
        material.tintAndMode = {
            tint_.x, tint_.y, tint_.z,
            animatedWater_ ? 1.0f : 0.0f
        };
        material.animation = {seconds * 0.012f, 0.0f, 0.0f, 0.0f};

        context_->UpdateSubresource(
            cameraConstants_.Get(), 0, nullptr, &camera, 0, 0
        );
        context_->UpdateSubresource(
            materialConstants_.Get(), 0, nullptr, &material, 0, 0
        );

        ID3D11Buffer* constants = cameraConstants_.Get();
        ID3D11Buffer* materialBuffer = materialConstants_.Get();
        ID3D11Buffer* vertices = vertices_.Get();
        ID3D11ShaderResourceView* textures[2] = {
            diffuse_.Get(), normal_.Get()
        };
        ID3D11SamplerState* sampler = sampler_.Get();
        constexpr UINT stride = sizeof(SurfaceVertex);
        constexpr UINT offset = 0;

        context_->IASetInputLayout(inputLayout_.Get());
        context_->IASetPrimitiveTopology(
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
        );
        context_->IASetVertexBuffers(
            0, 1, &vertices, &stride, &offset
        );
        context_->IASetIndexBuffer(
            indices_.Get(), DXGI_FORMAT_R32_UINT, 0
        );
        context_->VSSetShader(vertexShader_.Get(), nullptr, 0);
        context_->PSSetShader(pixelShader_.Get(), nullptr, 0);
        context_->VSSetConstantBuffers(0, 1, &constants);
        context_->PSSetConstantBuffers(1, 1, &materialBuffer);
        context_->PSSetShaderResources(0, 2, textures);
        context_->PSSetSamplers(0, 1, &sampler);
        context_->RSSetState(rasterizer_.Get());
        context_->DrawIndexed(indexCount_, 0, 0);

        ID3D11ShaderResourceView* emptyTextures[2] = {};
        ID3D11SamplerState* emptySampler = nullptr;
        context_->PSSetShaderResources(0, 2, emptyTextures);
        context_->PSSetSamplers(0, 1, &emptySampler);
        return 1;
    }

}
