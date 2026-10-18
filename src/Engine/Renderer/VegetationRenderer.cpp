#include "Engine/Renderer/VegetationRenderer.h"

#include "Engine/Assets/PrimitivesMeshLoader.h"

#include <d3dcompiler.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <stdexcept>
#include <string>
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
            DirectX::XMFLOAT4 tint;
        };

        static_assert(sizeof(InstanceData) == 20);
        static_assert(sizeof(MaterialConstants) == 16);

        constexpr char shaderSource[] = R"(

            cbuffer CameraConstants : register(b0)
            {
                float4x4 viewProjection;
            };

            cbuffer MaterialConstants : register(b1)
            {
                float4 materialTint;
            };

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

                output.position = mul(
                    float4(world, 1.0f),
                    viewProjection
                );

                return output;
            }

            float4 PSMain(PixelInput input) : SV_TARGET
            {
                float3 dx = ddx(input.worldPosition);
                float3 dy = ddy(input.worldPosition);

                float3 normal = normalize(
                    cross(dx, dy)
                );

                float3 lightDirection = normalize(
                    float3(-0.45f, 0.85f, 0.35f)
                );

                float light = 0.42f +
                    0.58f * abs(
                        dot(normal, lightDirection)
                    );

                // Diagnostic tint. Actual texture materials
                // will be resolved from .visual / .mfm.
                return float4(
                    materialTint.rgb * light,
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
                    "Vegetation shader compilation failed"
                );
            }

            return code;
        }

        [[nodiscard]] ComPtr<ID3D11Buffer> create_buffer(
            ID3D11Device* device,
            UINT byteWidth,
            UINT binding,
            const void* data
        ) {
            if (byteWidth == 0 || !data) {
                throw std::invalid_argument(
                    "Invalid vegetation GPU buffer"
                );
            }

            D3D11_BUFFER_DESC description{};

            description.ByteWidth = byteWidth;
            description.Usage = D3D11_USAGE_IMMUTABLE;
            description.BindFlags = binding;

            D3D11_SUBRESOURCE_DATA initialData{};
            initialData.pSysMem = data;

            ComPtr<ID3D11Buffer> result;

            check(
                device->CreateBuffer(
                    &description,
                    &initialData,
                    result.GetAddressOf()
                ),
                "Failed to create vegetation GPU buffer"
            );

            return result;
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

            const std::string normalized =
                path.lexically_normal().generic_string();

            return normalized.starts_with(
                "models/flora/"
            );
        }

        [[nodiscard]] DirectX::XMFLOAT4 kind_color(
            std::uint32_t kind
        ) noexcept {
            switch (kind) {
            case 0:
                return {0.30f, 0.69f, 0.31f, 1.0f};

            case 1:
                return {0.32f, 0.51f, 0.19f, 1.0f};

            case 2:
                return {0.21f, 0.53f, 0.38f, 1.0f};

            default:
                return {0.65f, 0.65f, 0.65f, 1.0f};
            }
        }

    }

    VegetationRenderer::VegetationRenderer(
        ID3D11Device* device,
        ID3D11DeviceContext* context
    ) : device_(device),
        context_(context) {
        if (!device_ || !context_) {
            throw std::invalid_argument(
                "Invalid Vegetation Renderer device"
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

        constexpr D3D11_INPUT_ELEMENT_DESC elements[] = {
            {
                "POSITION",
                0,
                DXGI_FORMAT_R32G32B32_FLOAT,
                0,
                0,
                D3D11_INPUT_PER_VERTEX_DATA,
                0
            },
            {
                "TEXCOORD",
                0,
                DXGI_FORMAT_R32G32_FLOAT,
                0,
                12,
                D3D11_INPUT_PER_VERTEX_DATA,
                0
            },
            {
                "INSTANCE_POSITION",
                0,
                DXGI_FORMAT_R32G32B32_FLOAT,
                1,
                0,
                D3D11_INPUT_PER_INSTANCE_DATA,
                1
            },
            {
                "INSTANCE_ROT_SCALE",
                0,
                DXGI_FORMAT_R32G32_FLOAT,
                1,
                12,
                D3D11_INPUT_PER_INSTANCE_DATA,
                1
            }
        };

        check(
            device_->CreateInputLayout(
                elements,
                4,
                vertexCode->GetBufferPointer(),
                vertexCode->GetBufferSize(),
                inputLayout_.GetAddressOf()
            ),
            "Failed to create vegetation input layout"
        );

        D3D11_BUFFER_DESC description{};
        description.ByteWidth = sizeof(CameraConstants);
        description.Usage = D3D11_USAGE_DEFAULT;
        description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        check(
            device_->CreateBuffer(
                &description,
                nullptr,
                cameraConstants_.GetAddressOf()
            ),
            "Failed to create vegetation camera buffer"
        );

        description.ByteWidth = sizeof(MaterialConstants);

        check(
            device_->CreateBuffer(
                &description,
                nullptr,
                materialConstants_.GetAddressOf()
            ),
            "Failed to create vegetation material buffer"
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

        std::vector<std::uint32_t> kinds(
            assetPaths.size(),
            0
        );

        for (const auto& instance : instances) {
            if (instance.assetIndex >= grouped.size() ||
                instance.kind > 2 ||
                !std::isfinite(instance.x) ||
                !std::isfinite(instance.y) ||
                !std::isfinite(instance.z) ||
                !std::isfinite(instance.yawRadians) ||
                !std::isfinite(instance.scale) ||
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

            kinds[instance.assetIndex] = instance.kind;
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

        newStats.skippedVariants =
            static_cast<std::uint32_t>(
                order.size() > maxVariants
                    ? order.size() - maxVariants
                    : 0
            );

        if (order.size() > maxVariants) {
            order.resize(maxVariants);
        }

        std::vector<Batch> newBatches;
        newBatches.reserve(order.size());

        for (const std::uint32_t id : order) {
            if (!safe_asset_path(assetPaths[id])) {
                ++newStats.failedVariants;
                continue;
            }

            try {
                const auto path =
                    packsDirectory /
                    std::filesystem::path(assetPaths[id]);

                auto geometry =
                    assets::load_primitives_mesh(path);

                const auto& placements = grouped[id];

                const std::uint64_t vertexBytes =
                    static_cast<std::uint64_t>(
                        geometry.vertices.size()
                    ) * sizeof(assets::PrimitivesVertex);

                const std::uint64_t indexBytes =
                    static_cast<std::uint64_t>(
                        geometry.indices.size()
                    ) * sizeof(std::uint32_t);

                const std::uint64_t instanceBytes =
                    static_cast<std::uint64_t>(
                        placements.size()
                    ) * sizeof(InstanceData);

                if (vertexBytes > UINT_MAX ||
                    indexBytes > UINT_MAX ||
                    instanceBytes > UINT_MAX ||
                    geometry.indices.size() > UINT_MAX ||
                    placements.size() > UINT_MAX) {
                    throw std::runtime_error(
                        "Vegetation GPU buffer exceeds limits"
                    );
                }

                Batch batch;

                batch.vertices = create_buffer(
                    device_,
                    static_cast<UINT>(vertexBytes),
                    D3D11_BIND_VERTEX_BUFFER,
                    geometry.vertices.data()
                );

                batch.indices = create_buffer(
                    device_,
                    static_cast<UINT>(indexBytes),
                    D3D11_BIND_INDEX_BUFFER,
                    geometry.indices.data()
                );

                batch.instances = create_buffer(
                    device_,
                    static_cast<UINT>(instanceBytes),
                    D3D11_BIND_VERTEX_BUFFER,
                    placements.data()
                );

                batch.indexCount = static_cast<UINT>(
                    geometry.indices.size()
                );

                batch.instanceCount = static_cast<UINT>(
                    placements.size()
                );

                batch.kind = kinds[id];

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

        batches_ = std::move(newBatches);
        stats_ = newStats;
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

        constexpr UINT strides[2] = {
            sizeof(assets::PrimitivesVertex),
            sizeof(InstanceData)
        };

        constexpr UINT offsets[2] = {0, 0};

        for (const Batch& batch : batches_) {
            MaterialConstants material{};
            material.tint = kind_color(batch.kind);

            context_->UpdateSubresource(
                materialConstants_.Get(),
                0,
                nullptr,
                &material,
                0,
                0
            );

            ID3D11Buffer* vertexBuffers[2] = {
                batch.vertices.Get(),
                batch.instances.Get()
            };

            context_->IASetVertexBuffers(
                0,
                2,
                vertexBuffers,
                strides,
                offsets
            );

            context_->IASetIndexBuffer(
                batch.indices.Get(),
                DXGI_FORMAT_R32_UINT,
                0
            );

            context_->DrawIndexedInstanced(
                batch.indexCount,
                batch.instanceCount,
                0,
                0,
                0
            );

            ++stats_.drawCalls;
        }

        // Clear the instance vertex stream. Other renderers
        // will install their own input layouts and shaders.
        ID3D11Buffer* emptyBuffers[2] = {};

        context_->IASetVertexBuffers(
            0,
            2,
            emptyBuffers,
            offsets,
            offsets
        );
    }

}
