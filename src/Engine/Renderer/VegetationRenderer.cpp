#include "Engine/Renderer/VegetationRenderer.h"

#include "Engine/Assets/DDSTextureLoader.h"
#include "Engine/Assets/PrimitivesMeshLoader.h"
#include "Engine/Assets/VisualMaterialLoader.h"

#include <d3dcompiler.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#pragma comment(lib, "d3dcompiler.lib")

namespace engine::renderer {

    namespace {

        using Microsoft::WRL::ComPtr;

        struct InstanceData {
            float x;
            float y;
            float z;

            float yaw;
            float scale;
        };

        struct CameraConstants {
            DirectX::XMFLOAT4X4 viewProjection;
        };

        struct MaterialConstants {
            DirectX::XMFLOAT4 parameters;
        };

        static_assert(sizeof(InstanceData) == 20);
        static_assert(sizeof(MaterialConstants) == 16);
        static_assert(sizeof(assets::PrimitivesVertex) == 20);

        constexpr char shaderSource[] = R"(

            cbuffer CameraConstants : register(b0)
            {
                float4x4 viewProjection;
            };

            cbuffer MaterialConstants : register(b1)
            {
                float4 materialParameters;
            };

            Texture2D diffuseTexture : register(t0);
            SamplerState textureSampler : register(s0);

            struct VertexInput
            {
                float3 position : POSITION;
                float2 uv : TEXCOORD0;

                float3 instancePosition : INSTANCE_POSITION;
                float2 yawScale : INSTANCE_ROT_SCALE;
            };

            struct PixelInput
            {
                float4 position : SV_POSITION;
                float3 worldPosition : TEXCOORD0;
                float2 uv : TEXCOORD1;
            };

            PixelInput VSMain(VertexInput input)
            {
                PixelInput output;

                float angle = input.yawScale.x;
                float scale = input.yawScale.y;

                float s = sin(angle);
                float c = cos(angle);

                float3 local = input.position * scale;

                float3 world;

                world.x =
                    local.x * c +
                    local.z * s +
                    input.instancePosition.x;

                world.y =
                    local.y +
                    input.instancePosition.y;

                world.z =
                    -local.x * s +
                    local.z * c +
                    input.instancePosition.z;

                output.worldPosition = world;
                output.uv = input.uv;

                output.position = mul(
                    float4(world, 1.0f),
                    viewProjection
                );

                return output;
            }

            float4 PSMain(PixelInput input) : SV_TARGET
            {
                float4 albedo = diffuseTexture.Sample(
                    textureSampler,
                    input.uv
                );

                if (materialParameters.y > 0.5f)
                {
                    clip(
                        albedo.a - materialParameters.x
                    );
                }

                float3 derivativeX = ddx(
                    input.worldPosition
                );

                float3 derivativeY = ddy(
                    input.worldPosition
                );

                float3 normal = normalize(
                    cross(derivativeX, derivativeY)
                );

                float3 lightDirection = normalize(
                    float3(-0.45f, 0.85f, 0.35f)
                );

                float light = 0.40f +
                    0.60f * abs(
                        dot(normal, lightDirection)
                    );

                return float4(
                    albedo.rgb * light,
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
                    "Failed to compile vegetation shader"
                );
            }

            return code;
        }

        [[nodiscard]] ComPtr<ID3D11Buffer> create_buffer(
            ID3D11Device* device,
            std::uint64_t byteWidth,
            UINT bindFlags,
            const void* data
        ) {
            if (!data ||
                byteWidth == 0 ||
                byteWidth >
                    (std::numeric_limits<UINT>::max)()) {
                throw std::invalid_argument(
                    "Invalid vegetation GPU buffer"
                );
            }

            D3D11_BUFFER_DESC desc{};

            desc.ByteWidth = static_cast<UINT>(byteWidth);
            desc.Usage = D3D11_USAGE_IMMUTABLE;
            desc.BindFlags = bindFlags;

            D3D11_SUBRESOURCE_DATA initial{};
            initial.pSysMem = data;

            ComPtr<ID3D11Buffer> buffer;

            check(
                device->CreateBuffer(
                    &desc,
                    &initial,
                    buffer.GetAddressOf()
                ),
                "Failed to allocate vegetation buffer"
            );

            return buffer;
        }

        [[nodiscard]] bool safe_asset_path(
            const std::string& text
        ) {
            const std::filesystem::path path(text);

            if (path.empty() ||
                path.is_absolute() ||
                path.has_root_name() ||
                path.extension() != ".primitives") {
                return false;
            }

            for (const auto& component : path) {
                if (component == "..") {
                    return false;
                }
            }

            return path.lexically_normal()
                .generic_string()
                .starts_with("models/flora/");
        }

    }

    VegetationRenderer::VegetationRenderer(
        ID3D11Device* device,
        ID3D11DeviceContext* context
    ) : device_(device),
        context_(context) {
        if (!device_ || !context_) {
            throw std::invalid_argument(
                "Invalid vegetation DirectX device"
            );
        }

        create_pipeline();
    }

    void VegetationRenderer::create_pipeline() {
        const auto vertexCode = compile_shader(
            "VSMain",
            "vs_5_0"
        );

        const auto pixelCode = compile_shader(
            "PSMain",
            "ps_5_0"
        );

        check(
            device_->CreateVertexShader(
                vertexCode->GetBufferPointer(),
                vertexCode->GetBufferSize(),
                nullptr,
                vertexShader_.GetAddressOf()
            ),
            "Failed to create vegetation vertex shader"
        );

        check(
            device_->CreatePixelShader(
                pixelCode->GetBufferPointer(),
                pixelCode->GetBufferSize(),
                nullptr,
                pixelShader_.GetAddressOf()
            ),
            "Failed to create vegetation pixel shader"
        );

        constexpr D3D11_INPUT_ELEMENT_DESC layout[] = {
            {
                "POSITION", 0,
                DXGI_FORMAT_R32G32B32_FLOAT,
                0, 0,
                D3D11_INPUT_PER_VERTEX_DATA, 0
            },
            {
                "TEXCOORD", 0,
                DXGI_FORMAT_R32G32_FLOAT,
                0, 12,
                D3D11_INPUT_PER_VERTEX_DATA, 0
            },
            {
                "INSTANCE_POSITION", 0,
                DXGI_FORMAT_R32G32B32_FLOAT,
                1, 0,
                D3D11_INPUT_PER_INSTANCE_DATA, 1
            },
            {
                "INSTANCE_ROT_SCALE", 0,
                DXGI_FORMAT_R32G32_FLOAT,
                1, 12,
                D3D11_INPUT_PER_INSTANCE_DATA, 1
            }
        };

        check(
            device_->CreateInputLayout(
                layout,
                4,
                vertexCode->GetBufferPointer(),
                vertexCode->GetBufferSize(),
                inputLayout_.GetAddressOf()
            ),
            "Failed to create vegetation input layout"
        );

        D3D11_BUFFER_DESC bufferDesc{};

        bufferDesc.ByteWidth = sizeof(CameraConstants);
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        check(
            device_->CreateBuffer(
                &bufferDesc,
                nullptr,
                cameraConstants_.GetAddressOf()
            ),
            "Failed to create vegetation camera buffer"
        );

        bufferDesc.ByteWidth = sizeof(MaterialConstants);

        check(
            device_->CreateBuffer(
                &bufferDesc,
                nullptr,
                materialConstants_.GetAddressOf()
            ),
            "Failed to create vegetation material buffer"
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
                textureSampler_.GetAddressOf()
            ),
            "Failed to create vegetation sampler"
        );

        D3D11_RASTERIZER_DESC rasterDesc{};

        rasterDesc.FillMode = D3D11_FILL_SOLID;
        rasterDesc.CullMode = D3D11_CULL_BACK;
        rasterDesc.DepthClipEnable = TRUE;

        check(
            device_->CreateRasterizerState(
                &rasterDesc,
                singleSidedState_.GetAddressOf()
            ),
            "Failed to create vegetation rasterizer"
        );

        rasterDesc.CullMode = D3D11_CULL_NONE;

        check(
            device_->CreateRasterizerState(
                &rasterDesc,
                doubleSidedState_.GetAddressOf()
            ),
            "Failed to create double-sided rasterizer"
        );
    }

    void VegetationRenderer::clear() noexcept {
        batches_.clear();
        stats_ = {};
    }

    void VegetationRenderer::set_scene(
        const std::filesystem::path& packsDirectory,
        std::span<const std::string> assetPaths,
        std::span<const VegetationDrawInstance> instances,
        std::uint32_t maxVariants
    ) {
        if (maxVariants == 0 ||
            maxVariants > 512 ||
            assetPaths.size() > 100000) {
            throw std::invalid_argument(
                "Invalid vegetation rendering budget"
            );
        }

        std::vector<std::vector<InstanceData>> grouped(
            assetPaths.size()
        );

        for (const auto& instance : instances) {
            if (instance.assetIndex >= grouped.size() ||
                instance.kind > 2 ||
                !std::isfinite(instance.x) ||
                !std::isfinite(instance.y) ||
                !std::isfinite(instance.z) ||
                !std::isfinite(instance.scale) ||
                !std::isfinite(instance.yawRadians) ||
                instance.scale <= 0.0f) {
                continue;
            }

            grouped[instance.assetIndex].push_back({
                instance.x,
                instance.y,
                instance.z,
                instance.yawRadians,
                instance.scale
            });
        }

        std::vector<std::uint32_t> order;

        for (std::size_t i = 0; i < grouped.size(); ++i) {
            if (!grouped[i].empty()) {
                order.push_back(
                    static_cast<std::uint32_t>(i)
                );
            }
        }

        std::sort(
            order.begin(),
            order.end(),
            [&](std::uint32_t a, std::uint32_t b) {
                if (grouped[a].size() != grouped[b].size()) {
                    return grouped[a].size() > grouped[b].size();
                }

                return a < b;
            }
        );

        VegetationRenderStats newStats{};

        if (order.size() > maxVariants) {
            newStats.skippedVariants =
                static_cast<std::uint32_t>(
                    order.size() - maxVariants
                );

            order.resize(maxVariants);
        }

        // One GPU texture per DDS path, shared by all batches.
        std::unordered_map<
            std::string,
            ComPtr<ID3D11ShaderResourceView>
        > textureCache;

        std::vector<Batch> newBatches;
        newBatches.reserve(order.size());

        for (const std::uint32_t id : order) {
            if (!safe_asset_path(assetPaths[id])) {
                ++newStats.failedVariants;

                newStats.failedPaths.push_back(
                    "Invalid asset path: " + assetPaths[id]
                );

                continue;
            }

            try {
                const auto mesh = assets::load_primitives_mesh(
                    packsDirectory / assetPaths[id]
                );

                const auto materialSet =
                    assets::load_visual_materials(
                        packsDirectory,
                        assetPaths[id]
                    );

                if (materialSet.materials.size() !=
                    mesh.groups.size()) {
                    throw std::runtime_error(
                        "Visual and primitives group counts differ"
                    );
                }

                Batch batch;

                batch.vertices = create_buffer(
                    device_,
                    static_cast<std::uint64_t>(
                        mesh.vertices.size()
                    ) * sizeof(assets::PrimitivesVertex),
                    D3D11_BIND_VERTEX_BUFFER,
                    mesh.vertices.data()
                );

                batch.indices = create_buffer(
                    device_,
                    static_cast<std::uint64_t>(
                        mesh.indices.size()
                    ) * sizeof(std::uint32_t),
                    D3D11_BIND_INDEX_BUFFER,
                    mesh.indices.data()
                );

                const auto& placements = grouped[id];

                batch.instances = create_buffer(
                    device_,
                    static_cast<std::uint64_t>(
                        placements.size()
                    ) * sizeof(InstanceData),
                    D3D11_BIND_VERTEX_BUFFER,
                    placements.data()
                );

                batch.instanceCount = static_cast<UINT>(
                    placements.size()
                );

                for (const auto& material :
                     materialSet.materials) {
                    if (material.primitiveGroup >=
                        mesh.groups.size()) {
                        throw std::runtime_error(
                            "Material group index out of range"
                        );
                    }

                    if (material.diffusePath.empty()) {
                        continue;
                    }

                    const std::string key =
                        material.diffusePath.generic_string();

                    auto textureIt = textureCache.find(key);

                    if (textureIt == textureCache.end()) {
                        const auto fullTexturePath =
                            packsDirectory /
                            material.diffusePath;

                        auto texture =
                            assets::load_dds_texture(
                                device_,
                                fullTexturePath
                            );

                        textureIt = textureCache.emplace(
                            key,
                            std::move(texture)
                        ).first;
                    }

                    const auto& sourceGroup =
                        mesh.groups[material.primitiveGroup];

                    MaterialGroup group;

                    group.diffuse = textureIt->second;

                    group.startIndex =
                        sourceGroup.startIndex;

                    group.indexCount =
                        sourceGroup.indexCount;

                    group.alphaTest =
                        material.alphaTest;

                    group.doubleSided =
                        material.doubleSided;

                    group.alphaReference =
                        static_cast<float>(
                            material.alphaReference
                        ) / 255.0f;

                    batch.materials.push_back(
                        std::move(group)
                    );
                }

                if (batch.materials.empty()) {
                    throw std::runtime_error(
                        "Visual has no renderable material groups"
                    );
                }

                newStats.loadedMaterialGroups +=
                    static_cast<std::uint32_t>(
                        batch.materials.size()
                    );

                newStats.renderedInstances +=
                    batch.instanceCount;

                ++newStats.loadedVariants;

                newBatches.push_back(
                    std::move(batch)
                );
            } catch (const std::exception& error) {
                ++newStats.failedVariants;

                newStats.failedPaths.push_back(
                    assetPaths[id] + ": " + error.what()
                );
            }
        }

        newStats.loadedTextures =
            static_cast<std::uint32_t>(
                textureCache.size()
            );

        batches_ = std::move(newBatches);
        stats_ = std::move(newStats);
    }

    void VegetationRenderer::render(
        DirectX::FXMMATRIX viewProjection
    ) {
        stats_.drawCalls = 0;

        if (!visible_ || batches_.empty()) {
            return;
        }

        CameraConstants camera{};

        DirectX::XMStoreFloat4x4(
            &camera.viewProjection,
            DirectX::XMMatrixTranspose(
                viewProjection
            )
        );

        context_->UpdateSubresource(
            cameraConstants_.Get(),
            0,
            nullptr,
            &camera,
            0,
            0
        );

        ID3D11Buffer* cameraBuffer =
            cameraConstants_.Get();

        ID3D11Buffer* materialBuffer =
            materialConstants_.Get();

        ID3D11SamplerState* sampler =
            textureSampler_.Get();

        context_->IASetInputLayout(
            inputLayout_.Get()
        );

        context_->IASetPrimitiveTopology(
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
        );

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

        context_->VSSetConstantBuffers(
            0,
            1,
            &cameraBuffer
        );

        context_->PSSetConstantBuffers(
            1,
            1,
            &materialBuffer
        );

        context_->PSSetSamplers(
            0,
            1,
            &sampler
        );

        constexpr UINT strides[2] = {
            sizeof(assets::PrimitivesVertex),
            sizeof(InstanceData)
        };

        constexpr UINT offsets[2] = {0, 0};

        for (const Batch& batch : batches_) {
            ID3D11Buffer* buffers[2] = {
                batch.vertices.Get(),
                batch.instances.Get()
            };

            context_->IASetVertexBuffers(
                0,
                2,
                buffers,
                strides,
                offsets
            );

            context_->IASetIndexBuffer(
                batch.indices.Get(),
                DXGI_FORMAT_R32_UINT,
                0
            );

            for (const MaterialGroup& group :
                 batch.materials) {
                MaterialConstants material{};

                material.parameters = {
                    group.alphaReference,
                    group.alphaTest ? 1.0f : 0.0f,
                    0.0f,
                    0.0f
                };

                context_->UpdateSubresource(
                    materialConstants_.Get(),
                    0,
                    nullptr,
                    &material,
                    0,
                    0
                );

                ID3D11ShaderResourceView* texture =
                    group.diffuse.Get();

                context_->PSSetShaderResources(
                    0,
                    1,
                    &texture
                );

                context_->RSSetState(
                    group.doubleSided
                        ? doubleSidedState_.Get()
                        : singleSidedState_.Get()
                );

                context_->DrawIndexedInstanced(
                    group.indexCount,
                    batch.instanceCount,
                    group.startIndex,
                    0,
                    0
                );

                ++stats_.drawCalls;
            }
        }

        ID3D11ShaderResourceView* emptyTexture = nullptr;
        ID3D11SamplerState* emptySampler = nullptr;
        ID3D11Buffer* emptyBuffer = nullptr;

        context_->PSSetShaderResources(
            0,
            1,
            &emptyTexture
        );

        context_->PSSetSamplers(
            0,
            1,
            &emptySampler
        );

        context_->PSSetConstantBuffers(
            1,
            1,
            &emptyBuffer
        );

        ID3D11Buffer* emptyBuffers[2] = {};
        constexpr UINT emptyStrides[2] = {0, 0};
        constexpr UINT emptyOffsets[2] = {0, 0};

        context_->IASetVertexBuffers(
            0,
            2,
            emptyBuffers,
            emptyStrides,
            emptyOffsets
        );

        context_->RSSetState(nullptr);
    }

}
