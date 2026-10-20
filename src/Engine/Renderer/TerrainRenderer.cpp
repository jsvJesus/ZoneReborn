
#include "Engine/Renderer/TerrainRenderer.h"

#include <d3dcompiler.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>

#pragma comment(lib, "d3dcompiler.lib")

namespace engine::renderer
{
    namespace
    {

        using Microsoft::WRL::ComPtr;

        struct Constants {
            DirectX::XMFLOAT4X4 viewProjection;
        };

        constexpr char shaderSource[] = R"(

        cbuffer CameraConstants : register(b0)
        {
            float4x4 viewProjection;
        };

        struct VertexInput
        {
            float3 position : POSITION;
            float3 normal : NORMAL;
        };

        struct PixelInput
        {
            float4 position : SV_POSITION;
            float3 normal : NORMAL;
        };

        PixelInput VSMain(VertexInput input)
        {
            PixelInput output;
            output.position = mul(
                float4(input.position, 1.0),
                viewProjection
            );
            output.normal = input.normal;
            return output;
        }

        float4 PSMain(PixelInput input) : SV_TARGET
        {
            float3 normal = normalize(input.normal);

            float3 lightDirection = normalize(
                float3(-0.45, 0.85, 0.35)
            );

            float lighting = 0.35 +
                saturate(dot(normal, lightDirection)) * 0.65;

            float slope = 1.0 - saturate(normal.y);

            float3 grass = float3(0.25, 0.36, 0.20);
            float3 soil = float3(0.42, 0.37, 0.29);

            float3 color = lerp(
                grass,
                soil,
                saturate(slope * 3.5)
            );

            return float4(color * lighting, 1.0);
        }

        )";

        void check(HRESULT result, const char* message)
        {
            if (FAILED(result)) {
                throw std::runtime_error(message);
            }
        }

        ComPtr<ID3DBlob> compile_shader(
            const char* entry,
            const char* target
        )
        {
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
                    "Failed to compile terrain shader"
                );
            }

            return bytecode;
        }

    }

    TerrainRenderer::TerrainRenderer(
        ID3D11Device* device,
        ID3D11DeviceContext* context
    ) : device_(device),
        context_(context),
        materials_(device, context),
        roadSurface_(device, context),
        openWaterSurface_(device, context),
        marshSurface_(device, context),
        vegetationRenderer_(device, context),
        bridgeRenderer_(
            device,
            context,
            "models/props/bridges/"
        )
    {
        if (!device_ || !context_) {
            throw std::invalid_argument(
                "Invalid DirectX device or context"
            );
        }

        create_pipeline();
        bridgeRenderer_.set_visible(true);
    }

    void TerrainRenderer::create_pipeline()
    {
        const auto vs = compile_shader("VSMain", "vs_5_0");
        const auto ps = compile_shader("PSMain", "ps_5_0");

        check(
            device_->CreateVertexShader(
                vs->GetBufferPointer(),
                vs->GetBufferSize(),
                nullptr,
                vertexShader_.GetAddressOf()
            ),
            "Failed to create terrain vertex shader"
        );

        check(
            device_->CreatePixelShader(
                ps->GetBufferPointer(),
                ps->GetBufferSize(),
                nullptr,
                pixelShader_.GetAddressOf()
            ),
            "Failed to create terrain pixel shader"
        );

        constexpr D3D11_INPUT_ELEMENT_DESC layout[] = {
            {
                "POSITION", 0,
                DXGI_FORMAT_R32G32B32_FLOAT,
                0, 0,
                D3D11_INPUT_PER_VERTEX_DATA, 0
            },
            {
                "NORMAL", 0,
                DXGI_FORMAT_R32G32B32_FLOAT,
                0, 12,
                D3D11_INPUT_PER_VERTEX_DATA, 0
            }
        };

        check(
            device_->CreateInputLayout(
                layout,
                2,
                vs->GetBufferPointer(),
                vs->GetBufferSize(),
                inputLayout_.GetAddressOf()
            ),
            "Failed to create terrain input layout"
        );

        D3D11_BUFFER_DESC constantDesc{};
        constantDesc.ByteWidth = sizeof(Constants);
        constantDesc.Usage = D3D11_USAGE_DEFAULT;
        constantDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        check(
            device_->CreateBuffer(
                &constantDesc,
                nullptr,
                constants_.GetAddressOf()
            ),
            "Failed to create terrain constant buffer"
        );

        D3D11_RASTERIZER_DESC rasterDesc{};
        rasterDesc.FillMode = D3D11_FILL_SOLID;
        rasterDesc.CullMode = D3D11_CULL_NONE;
        rasterDesc.DepthClipEnable = TRUE;

        check(
            device_->CreateRasterizerState(
                &rasterDesc,
                solidState_.GetAddressOf()
            ),
            "Failed to create solid rasterizer"
        );

        rasterDesc.FillMode = D3D11_FILL_WIREFRAME;

        check(
            device_->CreateRasterizerState(
                &rasterDesc,
                wireState_.GetAddressOf()
            ),
            "Failed to create wireframe rasterizer"
        );

        D3D11_DEPTH_STENCIL_DESC depthStateDesc{};
        depthStateDesc.DepthEnable = TRUE;
        depthStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depthStateDesc.DepthFunc = D3D11_COMPARISON_LESS;
        depthStateDesc.StencilEnable = FALSE;

        check(
            device_->CreateDepthStencilState(
                &depthStateDesc,
                depthState_.GetAddressOf()
            ),
            "Failed to create terrain depth state"
        );
    }

    
    void TerrainRenderer::set_mesh(const TerrainMeshData& data)
    {
        if (
            data.samplesX < 2 ||
            data.samplesZ < 2 ||
            data.samplesX > 2049 ||
            data.samplesZ > 2049 ||
            !std::isfinite(data.originX) ||
            !std::isfinite(data.originZ) ||
            !std::isfinite(data.spacingMeters) ||
            data.spacingMeters <= 0.0f ||
            data.heights.size() !=
                static_cast<std::size_t>(data.samplesX) * data.samplesZ
        ) {
            throw std::invalid_argument("Invalid terrain mesh data");
        }

        const std::uint32_t chunkCountX =
            (data.samplesX - 2) / TerrainChunkCells + 1;

        const std::uint32_t chunkCountZ =
            (data.samplesZ - 2) / TerrainChunkCells + 1;

        std::vector<Chunk> newChunks;
        newChunks.reserve(
            static_cast<std::size_t>(chunkCountX) * chunkCountZ
        );

        constexpr float skirtDepth = 64.0f;

        const auto height_at = [&](std::uint32_t x, std::uint32_t z) {
            return data.heights[
                static_cast<std::size_t>(z) * data.samplesX + x
            ];
        };

        for (std::uint32_t z = 0; z + 1 < data.samplesZ; z += TerrainChunkCells) {
            for (std::uint32_t x = 0; x + 1 < data.samplesX; x += TerrainChunkCells) {
                const std::uint32_t endX = (std::min)(
                    x + TerrainChunkCells,
                    data.samplesX - 1
                );

                const std::uint32_t endZ = (std::min)(
                    z + TerrainChunkCells,
                    data.samplesZ - 1
                );

                Chunk chunk;

                float minimumHeight = height_at(x, z);
                float maximumHeight = minimumHeight;

                for (std::uint32_t sampleZ = z; sampleZ <= endZ; ++sampleZ) {
                    for (std::uint32_t sampleX = x; sampleX <= endX; ++sampleX) {
                        const float height = height_at(sampleX, sampleZ);

                        if (!std::isfinite(height)) {
                            throw std::invalid_argument(
                                "Terrain contains invalid height values"
                            );
                        }

                        minimumHeight = (std::min)(minimumHeight, height);
                        maximumHeight = (std::max)(maximumHeight, height);
                    }
                }

                chunk.bounds.minimum = {
                    data.originX + static_cast<float>(x) * data.spacingMeters,
                    minimumHeight - skirtDepth,
                    data.originZ + static_cast<float>(z) * data.spacingMeters
                };

                chunk.bounds.maximum = {
                    data.originX + static_cast<float>(endX) * data.spacingMeters,
                    maximumHeight,
                    data.originZ + static_cast<float>(endZ) * data.spacingMeters
                };

                for (std::uint32_t level = 0; level < TerrainLodCount; ++level) {
                    const std::uint32_t stride = 1u << level;

                    const TerrainChunkGeometry mesh = build_chunk_geometry(
                        data,
                        x,
                        z,
                        stride,
                        skirtDepth
                    );

                    ChunkLevel& gpu = chunk.levels[level];

                    D3D11_BUFFER_DESC vertexDesc{};
                    vertexDesc.ByteWidth = static_cast<UINT>(
                        mesh.vertices.size() * sizeof(TerrainVertex)
                    );

                    vertexDesc.Usage = D3D11_USAGE_DEFAULT;
                    vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

                    D3D11_SUBRESOURCE_DATA vertexData{};
                    vertexData.pSysMem = mesh.vertices.data();

                    check(
                        device_->CreateBuffer(
                            &vertexDesc,
                            &vertexData,
                            gpu.vertices.GetAddressOf()
                        ),
                        "Failed to create terrain chunk vertex buffer"
                    );

                    D3D11_BUFFER_DESC indexDesc{};
                    indexDesc.ByteWidth = static_cast<UINT>(
                        mesh.indices.size() * sizeof(std::uint32_t)
                    );

                    indexDesc.Usage = D3D11_USAGE_DEFAULT;
                    indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

                    D3D11_SUBRESOURCE_DATA indexData{};
                    indexData.pSysMem = mesh.indices.data();

                    check(
                        device_->CreateBuffer(
                            &indexDesc,
                            &indexData,
                            gpu.indices.GetAddressOf()
                        ),
                        "Failed to create terrain chunk index buffer"
                    );

                    gpu.indexCount = static_cast<UINT>(mesh.indices.size());
                }

                newChunks.push_back(std::move(chunk));
            }
        }

        chunks_ = std::move(newChunks);
        chunkWorldSize_ = TerrainChunkCells * data.spacingMeters;

        stats_ = {};
        stats_.chunks = static_cast<std::uint32_t>(chunks_.size());
        materials_.clear();
        roadSurface_.clear();
        clear_water_surfaces();
        vegetationRenderer_.clear();
        bridgeRenderer_.clear();
    }

    void TerrainRenderer::clear_chunks() noexcept {
        chunks_.clear();
        stats_ = {};
        materials_.clear();
        roadSurface_.clear();
        clear_water_surfaces();
        vegetationRenderer_.clear();
        bridgeRenderer_.clear();
    }

    void TerrainRenderer::remove_stream_chunk(
        std::uint32_t chunkX,
        std::uint32_t chunkZ
    )
    {
        std::erase_if(chunks_, [&](const Chunk& chunk) {
            return chunk.gridX == chunkX && chunk.gridZ == chunkZ;
        });
    }

    void TerrainRenderer::add_stream_chunk(
        std::uint32_t chunkX,
        std::uint32_t chunkZ,
        const TerrainMeshData& data
    )
    {
        if (data.samplesX < 2 || data.samplesX > TerrainChunkCells + 1 ||
            data.samplesZ < 2 || data.samplesZ > TerrainChunkCells + 1 ||
            !std::isfinite(data.originX) ||
            !std::isfinite(data.originZ) ||
            !std::isfinite(data.spacingMeters) ||
            data.spacingMeters <= 0.0f ||
            data.heights.size() !=
                static_cast<std::size_t>(data.samplesX) * data.samplesZ) {
            throw std::invalid_argument("Invalid streaming chunk");
        }

        constexpr float skirtDepth = 64.0f;

        const auto [minIt, maxIt] = std::minmax_element(
            data.heights.begin(),
            data.heights.end()
        );

        for (float height : data.heights) {
            if (!std::isfinite(height)) {
                throw std::invalid_argument(
                    "Streaming chunk contains invalid height"
                );
            }
        }

        Chunk chunk;

        chunk.gridX = chunkX;
        chunk.gridZ = chunkZ;

        chunk.bounds.minimum = {
            data.originX,
            *minIt - skirtDepth,
            data.originZ
        };

        chunk.bounds.maximum = {
            data.originX + (data.samplesX - 1) * data.spacingMeters,
            *maxIt,
            data.originZ + (data.samplesZ - 1) * data.spacingMeters
        };

        for (std::uint32_t level = 0; level < TerrainLodCount; ++level) {
            const TerrainChunkGeometry mesh = build_chunk_geometry(
                data,
                0,
                0,
                1u << level,
                skirtDepth,
                true
            );

            ChunkLevel& gpu = chunk.levels[level];

            D3D11_BUFFER_DESC vertexDesc{};
            vertexDesc.ByteWidth = static_cast<UINT>(
                mesh.vertices.size() * sizeof(TerrainVertex)
            );
            vertexDesc.Usage = D3D11_USAGE_DEFAULT;
            vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

            D3D11_SUBRESOURCE_DATA vertexData{};
            vertexData.pSysMem = mesh.vertices.data();

            check(
                device_->CreateBuffer(
                    &vertexDesc,
                    &vertexData,
                    gpu.vertices.GetAddressOf()
                ),
                "Failed to create streaming vertex buffer"
            );

            D3D11_BUFFER_DESC indexDesc{};
            indexDesc.ByteWidth = static_cast<UINT>(
                mesh.indices.size() * sizeof(std::uint32_t)
            );
            indexDesc.Usage = D3D11_USAGE_DEFAULT;
            indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

            D3D11_SUBRESOURCE_DATA indexData{};
            indexData.pSysMem = mesh.indices.data();

            check(
                device_->CreateBuffer(
                    &indexDesc,
                    &indexData,
                    gpu.indices.GetAddressOf()
                ),
                "Failed to create streaming index buffer"
            );

            gpu.indexCount = static_cast<UINT>(mesh.indices.size());
        }

        chunks_.reserve(chunks_.size() + 1);
        remove_stream_chunk(chunkX, chunkZ);
        chunks_.push_back(std::move(chunk));

        chunkWorldSize_ = TerrainChunkCells * data.spacingMeters;
    }
    
    std::uint32_t TerrainRenderer::select_lod(
        const Chunk& chunk,
        const DirectX::XMFLOAT3& cameraPosition
    ) const noexcept
    {
        if (!autoLod_) {
            return fixedLod_;
        }

        const Bounds3& bounds = chunk.bounds;

        const float closestX = std::clamp(
            cameraPosition.x,
            bounds.minimum.x,
            bounds.maximum.x
        );

        const float closestY = std::clamp(
            cameraPosition.y,
            bounds.minimum.y,
            bounds.maximum.y
        );

        const float closestZ = std::clamp(
            cameraPosition.z,
            bounds.minimum.z,
            bounds.maximum.z
        );

        const float dx = cameraPosition.x - closestX;
        const float dy = cameraPosition.y - closestY;
        const float dz = cameraPosition.z - closestZ;

        const float distance = std::sqrt(
            dx * dx + dy * dy + dz * dz
        );

        const float threshold = chunkWorldSize_ * lodDistanceScale_;

        if (distance < threshold * 2.0f) {
            return 0;
        }

        if (distance < threshold * 4.0f) {
            return 1;
        }

        if (distance < threshold * 8.0f) {
            return 2;
        }

        return 3;
    }

    void TerrainRenderer::resize(UINT width, UINT height)
    {
        if (width == 0 || height == 0) {
            return;
        }

        if (
            width == width_ &&
            height == height_ &&
            shaderResource_
        ) {
            return;
        }

        context_->OMSetRenderTargets(0, nullptr, nullptr);
        ID3D11ShaderResourceView* emptyResource = nullptr;
        context_->PSSetShaderResources(0, 1, &emptyResource);

        shaderResource_.Reset();
        renderTarget_.Reset();
        colorTexture_.Reset();
        depthView_.Reset();
        depthTexture_.Reset();

        D3D11_TEXTURE2D_DESC colorDesc{};
        colorDesc.Width = width;
        colorDesc.Height = height;
        colorDesc.MipLevels = 1;
        colorDesc.ArraySize = 1;
        colorDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        colorDesc.SampleDesc.Count = 1;
        colorDesc.Usage = D3D11_USAGE_DEFAULT;
        colorDesc.BindFlags =
            D3D11_BIND_RENDER_TARGET |
            D3D11_BIND_SHADER_RESOURCE;

        check(
            device_->CreateTexture2D(
                &colorDesc,
                nullptr,
                colorTexture_.GetAddressOf()
            ),
            "Failed to create terrain color texture"
        );

        check(
            device_->CreateRenderTargetView(
                colorTexture_.Get(),
                nullptr,
                renderTarget_.GetAddressOf()
            ),
            "Failed to create terrain render target"
        );

        check(
            device_->CreateShaderResourceView(
                colorTexture_.Get(),
                nullptr,
                shaderResource_.GetAddressOf()
            ),
            "Failed to create terrain shader resource"
        );

        D3D11_TEXTURE2D_DESC depthDesc = colorDesc;
        depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        check(
            device_->CreateTexture2D(
                &depthDesc,
                nullptr,
                depthTexture_.GetAddressOf()
            ),
            "Failed to create terrain depth texture"
        );

        check(
            device_->CreateDepthStencilView(
                depthTexture_.Get(),
                nullptr,
                depthView_.GetAddressOf()
            ),
            "Failed to create terrain depth view"
        );

        width_ = width;
        height_ = height;
    }

    
    void TerrainRenderer::render(
        const DirectX::XMMATRIX& viewProjection,
        const DirectX::XMFLOAT3& cameraPosition
    ) {
        const Frustum frustum(viewProjection);
    
        stats_ = {};
        stats_.chunks = static_cast<std::uint32_t>(
            chunks_.size()
        );

        if (!renderTarget_ || !depthView_) {
            return;
        }

        constexpr float clearColor[4] = {
            0.11f, 0.16f, 0.19f, 1.0f
        };

        context_->ClearRenderTargetView(
            renderTarget_.Get(),
            clearColor
        );

        context_->ClearDepthStencilView(
            depthView_.Get(),
            D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
            1.0f,
            0
        );

        ID3D11RenderTargetView* target = renderTarget_.Get();

        context_->OMSetRenderTargets(
            1,
            &target,
            depthView_.Get()
        );

        context_->OMSetDepthStencilState(
            depthState_.Get(),
            0
        );

        D3D11_VIEWPORT viewport{};
        viewport.Width = static_cast<float>(width_);
        viewport.Height = static_cast<float>(height_);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;

        context_->RSSetViewports(1, &viewport);

        Constants constants{};

        DirectX::XMStoreFloat4x4(
            &constants.viewProjection,
            DirectX::XMMatrixTranspose(viewProjection)
        );

        context_->UpdateSubresource(
            constants_.Get(),
            0,
            nullptr,
            &constants,
            0,
            0
        );

        context_->IASetInputLayout(inputLayout_.Get());

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

        if (materials_.ready()) {
            materials_.bind();
        }

        context_->GSSetShader(nullptr, nullptr, 0);
        context_->HSSetShader(nullptr, nullptr, 0);
        context_->DSSetShader(nullptr, nullptr, 0);

        ID3D11Buffer* constantBuffer = constants_.Get();

        context_->VSSetConstantBuffers(
            0,
            1,
            &constantBuffer
        );

        context_->RSSetState(
            wireframe_
                ? wireState_.Get()
                : solidState_.Get()
        );

        constexpr UINT stride = sizeof(TerrainVertex);
        constexpr UINT offset = 0;

        for (const Chunk& chunk : chunks_) {
            if (frustumCulling_ && !frustum.intersects(chunk.bounds)) {
                ++stats_.culledChunks;
                continue;
            }
            
            const std::uint32_t level = select_lod(
                chunk,
                cameraPosition
            );

            const ChunkLevel& mesh = chunk.levels[level];

            if (!mesh.vertices ||
                !mesh.indices ||
                mesh.indexCount == 0) {
                continue;
            }

            ID3D11Buffer* vertexBuffer =
                mesh.vertices.Get();

            context_->IASetVertexBuffers(
                0,
                1,
                &vertexBuffer,
                &stride,
                &offset
            );

            context_->IASetIndexBuffer(
                mesh.indices.Get(),
                DXGI_FORMAT_R32_UINT,
                0
            );

            context_->DrawIndexed(
                mesh.indexCount,
                0,
                0
            );

            ++stats_.visibleChunks;
            ++stats_.drawCalls;
            ++stats_.lodCounts[level];

            stats_.triangles += mesh.indexCount / 3;
        }

        if (materials_.ready()) {
            materials_.unbind();
        }

        stats_.drawCalls += openWaterSurface_.render(viewProjection);
        stats_.drawCalls += marshSurface_.render(viewProjection);
        stats_.drawCalls += roadSurface_.render(viewProjection);
        bridgeRenderer_.render(viewProjection);
        stats_.drawCalls += bridgeRenderer_.stats().drawCalls;
        vegetationRenderer_.render(viewProjection);

        context_->OMSetRenderTargets(
            0,
            nullptr,
            nullptr
        );

        context_->OMSetDepthStencilState(
            nullptr,
            0
        );
    }
}
