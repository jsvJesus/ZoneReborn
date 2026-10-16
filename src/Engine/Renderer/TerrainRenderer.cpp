
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

namespace engine::renderer {

namespace {

using Microsoft::WRL::ComPtr;

struct Vertex {
    float x, y, z;
    float nx, ny, nz;
};

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

void check(HRESULT result, const char* message) {
    if (FAILED(result)) {
        throw std::runtime_error(message);
    }
}

ComPtr<ID3DBlob> compile_shader(
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
            "Failed to compile terrain shader"
        );
    }

    return bytecode;
}

}

TerrainRenderer::TerrainRenderer(
    ID3D11Device* device,
    ID3D11DeviceContext* context
) : device_(device), context_(context) {
    if (!device_ || !context_) {
        throw std::invalid_argument(
            "Invalid DirectX device or context"
        );
    }

    create_pipeline();
}

void TerrainRenderer::create_pipeline() {
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

void TerrainRenderer::set_mesh(
    const TerrainMeshData& data
) {
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
            static_cast<std::size_t>(data.samplesX) *
            data.samplesZ
    ) {
        throw std::invalid_argument(
            "Invalid terrain mesh data"
        );
    }

    const auto height = [&](std::uint32_t x,
                            std::uint32_t z) {
        return data.heights[
            static_cast<std::size_t>(z) *
            data.samplesX + x
        ];
    };

    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;

    vertices.reserve(data.heights.size());

    indices.reserve(
        static_cast<std::size_t>(data.samplesX - 1) *
        (data.samplesZ - 1) * 6
    );

    for (std::uint32_t z = 0; z < data.samplesZ; ++z) {
        for (std::uint32_t x = 0; x < data.samplesX; ++x) {
            const auto x0 = x == 0 ? x : x - 1;
            const auto z0 = z == 0 ? z : z - 1;

            const auto x1 = std::min(x + 1, data.samplesX - 1);
            const auto z1 = std::min(z + 1, data.samplesZ - 1);

            const float dx =
                (height(x1, z) - height(x0, z)) /
                ((x1 - x0) * data.spacingMeters);

            const float dz =
                (height(x, z1) - height(x, z0)) /
                ((z1 - z0) * data.spacingMeters);

            const float nx = -dx;
            const float ny = 1.0f;
            const float nz = -dz;

            const float length =
                std::sqrt(nx * nx + ny * ny + nz * nz);

            vertices.push_back({
                data.originX + x * data.spacingMeters,
                height(x, z),
                data.originZ + z * data.spacingMeters,
                nx / length,
                ny / length,
                nz / length
            });
        }
    }

    for (std::uint32_t z = 0;
         z + 1 < data.samplesZ;
         ++z) {

        for (std::uint32_t x = 0;
             x + 1 < data.samplesX;
             ++x) {

            const std::uint32_t a =
                z * data.samplesX + x;
            const std::uint32_t b = a + 1;
            const std::uint32_t c = a + data.samplesX;
            const std::uint32_t d = c + 1;

            indices.insert(indices.end(), {
                a, c, b,
                b, c, d
            });
        }
    }

    for (const Vertex& vertex : vertices) {
        if (!std::isfinite(vertex.x) ||
            !std::isfinite(vertex.y) ||
            !std::isfinite(vertex.z) ||
            !std::isfinite(vertex.nx) ||
            !std::isfinite(vertex.ny) ||
            !std::isfinite(vertex.nz)) {
            throw std::invalid_argument(
                "Terrain mesh contains non-finite values"
            );
        }
    }

    D3D11_BUFFER_DESC vertexDesc{};
    vertexDesc.ByteWidth = static_cast<UINT>(
        vertices.size() * sizeof(Vertex)
    );
    vertexDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexData{};
    vertexData.pSysMem = vertices.data();

    ComPtr<ID3D11Buffer> newVertices;

    check(
        device_->CreateBuffer(
            &vertexDesc,
            &vertexData,
            newVertices.GetAddressOf()
        ),
        "Failed to create terrain vertex buffer"
    );

    D3D11_BUFFER_DESC indexDesc{};
    indexDesc.ByteWidth = static_cast<UINT>(
        indices.size() * sizeof(std::uint32_t)
    );
    indexDesc.Usage = D3D11_USAGE_DEFAULT;
    indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA indexData{};
    indexData.pSysMem = indices.data();

    ComPtr<ID3D11Buffer> newIndices;

    check(
        device_->CreateBuffer(
            &indexDesc,
            &indexData,
            newIndices.GetAddressOf()
        ),
        "Failed to create terrain index buffer"
    );

    vertices_ = std::move(newVertices);
    indices_ = std::move(newIndices);
    indexCount_ = static_cast<UINT>(indices.size());
}

void TerrainRenderer::resize(UINT width, UINT height) {
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
    const DirectX::XMMATRIX& viewProjection
) {
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

    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(width_);
    viewport.Height = static_cast<float>(height_);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    context_->RSSetViewports(1, &viewport);

    if (vertices_ && indices_ && indexCount_ > 0) {
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

        constexpr UINT stride = sizeof(Vertex);
        constexpr UINT offset = 0;

        ID3D11Buffer* vertexBuffer = vertices_.Get();
        ID3D11Buffer* constantBuffer = constants_.Get();

        context_->IASetInputLayout(inputLayout_.Get());

        context_->IASetVertexBuffers(
            0,
            1,
            &vertexBuffer,
            &stride,
            &offset
        );

        context_->IASetIndexBuffer(
            indices_.Get(),
            DXGI_FORMAT_R32_UINT,
            0
        );

        context_->IASetPrimitiveTopology(
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
        );

        context_->VSSetShader(vertexShader_.Get(), nullptr, 0);
        context_->PSSetShader(pixelShader_.Get(), nullptr, 0);

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

        context_->DrawIndexed(indexCount_, 0, 0);
    }

    context_->OMSetRenderTargets(0, nullptr, nullptr);
    context_->OMSetDepthStencilState(depthState_.Get(), 0);
}

}
