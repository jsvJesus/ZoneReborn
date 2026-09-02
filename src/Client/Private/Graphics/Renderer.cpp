#include "Graphics/Renderer.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace
{
    using Microsoft::WRL::ComPtr;

    struct GpuVertex final
    {
        float x;
        float y;
        float z;
    };

    struct SceneConstants final
    {
        DirectX::XMFLOAT4X4 world;
        DirectX::XMFLOAT4X4 worldViewProjection;
    };

    constexpr char ShaderSource[] = R"(
cbuffer SceneConstants : register(b0)
{
    float4x4 world;
    float4x4 worldViewProjection;
};

struct VertexInput
{
    float3 position : POSITION;
};

struct PixelInput
{
    float4 position : SV_POSITION;
    float3 worldPosition : TEXCOORD0;
};

PixelInput VSMain(VertexInput input)
{
    PixelInput output;

    float4 localPosition =
        float4(input.position, 1.0f);

    output.position =
        mul(
            worldViewProjection,
            localPosition);

    output.worldPosition =
        mul(
            world,
            localPosition).xyz;

    return output;
}

float4 PSMain(PixelInput input) : SV_TARGET
{
    float3 dx =
        ddx(input.worldPosition);

    float3 dy =
        ddy(input.worldPosition);

    float3 normal =
        normalize(
            cross(dx, dy));

    float3 lightDirection =
        normalize(
            float3(
                0.35f,
                0.85f,
                -0.40f));

    float lighting =
        0.20f +
        abs(
            dot(
                normal,
                lightDirection)) *
        0.80f;

    float3 baseColour =
        float3(
            0.72f,
            0.76f,
            0.82f);

    return float4(
        baseColour * lighting,
        1.0f);
}
)";

    bool CompileShader(
        const char* entryPoint,
        const char* profile,
        ID3DBlob** output,
        std::string& error)
    {
        ComPtr<ID3DBlob> shader;
        ComPtr<ID3DBlob> errors;

        const HRESULT result =
            D3DCompile(
                ShaderSource,
                sizeof(ShaderSource) - 1,
                nullptr,
                nullptr,
                nullptr,
                entryPoint,
                profile,
                D3DCOMPILE_ENABLE_STRICTNESS |
                D3DCOMPILE_OPTIMIZATION_LEVEL3,
                0,
                &shader,
                &errors);

        if (FAILED(result))
        {
            if (errors)
            {
                error.assign(
                    static_cast<const char*>(
                        errors->GetBufferPointer()),
                    errors->GetBufferSize());
            }
            else
            {
                error =
                    "Unable to compile D3D11 shader.";
            }

            return false;
        }

        *output =
            shader.Detach();

        return true;
    }
}

namespace client::graphics
{
    struct Renderer::State final
    {
        ComPtr<ID3D11Device> device;
        ComPtr<ID3D11DeviceContext> context;

        ComPtr<IDXGISwapChain> swapChain;

        ComPtr<ID3D11RenderTargetView>
            renderTargetView;

        ComPtr<ID3D11Texture2D>
            depthTexture;

        ComPtr<ID3D11DepthStencilView>
            depthStencilView;

        ComPtr<ID3D11DepthStencilState>
            depthState;

        ComPtr<ID3D11RasterizerState>
            rasterizerState;

        ComPtr<ID3D11VertexShader>
            vertexShader;

        ComPtr<ID3D11PixelShader>
            pixelShader;

        ComPtr<ID3D11InputLayout>
            inputLayout;

        ComPtr<ID3D11Buffer>
            vertexBuffer;

        ComPtr<ID3D11Buffer>
            indexBuffer;

        ComPtr<ID3D11Buffer>
            constantBuffer;

        std::uint32_t width = 0;
        std::uint32_t height = 0;

        std::uint32_t indexCount = 0;

        DirectX::XMFLOAT3 center
        {
            0.0f,
            0.0f,
            0.0f
        };

        float radius = 1.0f;

        std::chrono::steady_clock::time_point
            startTime =
                std::chrono::steady_clock::now();
    };

    Renderer::Renderer()
        : state_(
            std::make_unique<State>())
    {
    }

    Renderer::~Renderer()
    {
        Shutdown();
    }

    bool Renderer::Initialize(
        const HWND window,
        const std::uint32_t width,
        const std::uint32_t height,
        std::string& error)
    {
        Shutdown();

        state_ =
            std::make_unique<State>();

        error.clear();

        if (window == nullptr ||
            width == 0 ||
            height == 0)
        {
            error =
                "Renderer received invalid window parameters.";

            return false;
        }

        DXGI_SWAP_CHAIN_DESC swapChainDescription{};

        swapChainDescription.BufferDesc.Width =
            width;

        swapChainDescription.BufferDesc.Height =
            height;

        swapChainDescription.BufferDesc.Format =
            DXGI_FORMAT_R8G8B8A8_UNORM;

        swapChainDescription.SampleDesc.Count =
            1;

        swapChainDescription.SampleDesc.Quality =
            0;

        swapChainDescription.BufferUsage =
            DXGI_USAGE_RENDER_TARGET_OUTPUT;

        swapChainDescription.BufferCount =
            2;

        swapChainDescription.OutputWindow =
            window;

        swapChainDescription.Windowed =
            TRUE;

        swapChainDescription.SwapEffect =
            DXGI_SWAP_EFFECT_DISCARD;

        const D3D_FEATURE_LEVEL featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_0
        };

        D3D_FEATURE_LEVEL createdFeatureLevel =
            D3D_FEATURE_LEVEL_11_0;

        HRESULT result =
            D3D11CreateDeviceAndSwapChain(
                nullptr,
                D3D_DRIVER_TYPE_HARDWARE,
                nullptr,
                D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                featureLevels,
                1,
                D3D11_SDK_VERSION,
                &swapChainDescription,
                &state_->swapChain,
                &state_->device,
                &createdFeatureLevel,
                &state_->context);

        if (FAILED(result))
        {
            result =
                D3D11CreateDeviceAndSwapChain(
                    nullptr,
                    D3D_DRIVER_TYPE_WARP,
                    nullptr,
                    D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                    featureLevels,
                    1,
                    D3D11_SDK_VERSION,
                    &swapChainDescription,
                    &state_->swapChain,
                    &state_->device,
                    &createdFeatureLevel,
                    &state_->context);
        }

        if (FAILED(result))
        {
            error =
                "Unable to create D3D11 device and swap chain.";

            return false;
        }

        ComPtr<ID3D11Texture2D> backBuffer;

        result =
            state_->swapChain->GetBuffer(
                0,
                IID_PPV_ARGS(
                    &backBuffer));

        if (FAILED(result))
        {
            error =
                "Unable to acquire swap chain back buffer.";

            return false;
        }

        result =
            state_->device->CreateRenderTargetView(
                backBuffer.Get(),
                nullptr,
                &state_->renderTargetView);

        if (FAILED(result))
        {
            error =
                "Unable to create render target view.";

            return false;
        }

        D3D11_TEXTURE2D_DESC depthDescription{};

        depthDescription.Width =
            width;

        depthDescription.Height =
            height;

        depthDescription.MipLevels =
            1;

        depthDescription.ArraySize =
            1;

        depthDescription.Format =
            DXGI_FORMAT_D24_UNORM_S8_UINT;

        depthDescription.SampleDesc.Count =
            1;

        depthDescription.Usage =
            D3D11_USAGE_DEFAULT;

        depthDescription.BindFlags =
            D3D11_BIND_DEPTH_STENCIL;

        result =
            state_->device->CreateTexture2D(
                &depthDescription,
                nullptr,
                &state_->depthTexture);

        if (FAILED(result))
        {
            error =
                "Unable to create depth texture.";

            return false;
        }

        result =
            state_->device->CreateDepthStencilView(
                state_->depthTexture.Get(),
                nullptr,
                &state_->depthStencilView);

        if (FAILED(result))
        {
            error =
                "Unable to create depth stencil view.";

            return false;
        }

        D3D11_DEPTH_STENCIL_DESC depthStateDescription{};

        depthStateDescription.DepthEnable =
            TRUE;

        depthStateDescription.DepthWriteMask =
            D3D11_DEPTH_WRITE_MASK_ALL;

        depthStateDescription.DepthFunc =
            D3D11_COMPARISON_LESS;

        result =
            state_->device->CreateDepthStencilState(
                &depthStateDescription,
                &state_->depthState);

        if (FAILED(result))
        {
            error =
                "Unable to create depth stencil state.";

            return false;
        }

        D3D11_RASTERIZER_DESC rasterizerDescription{};

        rasterizerDescription.FillMode =
            D3D11_FILL_SOLID;

        rasterizerDescription.CullMode =
            D3D11_CULL_NONE;

        rasterizerDescription.FrontCounterClockwise =
            FALSE;

        rasterizerDescription.DepthClipEnable =
            TRUE;

        result =
            state_->device->CreateRasterizerState(
                &rasterizerDescription,
                &state_->rasterizerState);

        if (FAILED(result))
        {
            error =
                "Unable to create rasterizer state.";

            return false;
        }

        ComPtr<ID3DBlob> vertexShaderCode;

        if (!CompileShader(
                "VSMain",
                "vs_5_0",
                &vertexShaderCode,
                error))
        {
            return false;
        }

        ComPtr<ID3DBlob> pixelShaderCode;

        if (!CompileShader(
                "PSMain",
                "ps_5_0",
                &pixelShaderCode,
                error))
        {
            return false;
        }

        result =
            state_->device->CreateVertexShader(
                vertexShaderCode->GetBufferPointer(),
                vertexShaderCode->GetBufferSize(),
                nullptr,
                &state_->vertexShader);

        if (FAILED(result))
        {
            error =
                "Unable to create vertex shader.";

            return false;
        }

        result =
            state_->device->CreatePixelShader(
                pixelShaderCode->GetBufferPointer(),
                pixelShaderCode->GetBufferSize(),
                nullptr,
                &state_->pixelShader);

        if (FAILED(result))
        {
            error =
                "Unable to create pixel shader.";

            return false;
        }

        const D3D11_INPUT_ELEMENT_DESC inputElements[] =
        {
            {
                "POSITION",
                0,
                DXGI_FORMAT_R32G32B32_FLOAT,
                0,
                0,
                D3D11_INPUT_PER_VERTEX_DATA,
                0
            }
        };

        result =
            state_->device->CreateInputLayout(
                inputElements,
                1,
                vertexShaderCode->GetBufferPointer(),
                vertexShaderCode->GetBufferSize(),
                &state_->inputLayout);

        if (FAILED(result))
        {
            error =
                "Unable to create input layout.";

            return false;
        }

        D3D11_BUFFER_DESC constantBufferDescription{};

        constantBufferDescription.ByteWidth =
            sizeof(SceneConstants);

        constantBufferDescription.Usage =
            D3D11_USAGE_DEFAULT;

        constantBufferDescription.BindFlags =
            D3D11_BIND_CONSTANT_BUFFER;

        result =
            state_->device->CreateBuffer(
                &constantBufferDescription,
                nullptr,
                &state_->constantBuffer);

        if (FAILED(result))
        {
            error =
                "Unable to create scene constant buffer.";

            return false;
        }

        state_->width =
            width;

        state_->height =
            height;

        state_->startTime =
            std::chrono::steady_clock::now();

        return true;
    }

    bool Renderer::SetMesh(
        const core::assets::MeshData& mesh,
        std::string& error)
    {
        error.clear();

        if (!state_ ||
            !state_->device)
        {
            error =
                "Renderer is not initialized.";

            return false;
        }

        if (mesh.vertices.empty())
        {
            error =
                "Mesh contains no vertices.";

            return false;
        }

        if (mesh.indices.empty())
        {
            error =
                "Mesh contains no indices.";

            return false;
        }

        if (mesh.vertices.size() >
            std::numeric_limits<UINT>::max() /
                sizeof(GpuVertex))
        {
            error =
                "Mesh vertex buffer is too large.";

            return false;
        }

        if (mesh.indices.size() >
            std::numeric_limits<UINT>::max() /
                sizeof(std::uint16_t))
        {
            error =
                "Mesh index buffer is too large.";

            return false;
        }

        std::vector<GpuVertex> vertices;

        vertices.reserve(
            mesh.vertices.size());

        DirectX::XMFLOAT3 minimum
        {
            mesh.vertices[0].position.x,
            mesh.vertices[0].position.y,
            mesh.vertices[0].position.z
        };

        DirectX::XMFLOAT3 maximum =
            minimum;

        for (const core::assets::MeshVertex& vertex :
             mesh.vertices)
        {
            vertices.push_back(
            {
                vertex.position.x,
                vertex.position.y,
                vertex.position.z
            });

            minimum.x =
                std::min(
                    minimum.x,
                    vertex.position.x);

            minimum.y =
                std::min(
                    minimum.y,
                    vertex.position.y);

            minimum.z =
                std::min(
                    minimum.z,
                    vertex.position.z);

            maximum.x =
                std::max(
                    maximum.x,
                    vertex.position.x);

            maximum.y =
                std::max(
                    maximum.y,
                    vertex.position.y);

            maximum.z =
                std::max(
                    maximum.z,
                    vertex.position.z);
        }

        D3D11_BUFFER_DESC vertexBufferDescription{};

        vertexBufferDescription.ByteWidth =
            static_cast<UINT>(
                vertices.size() *
                sizeof(GpuVertex));

        vertexBufferDescription.Usage =
            D3D11_USAGE_DEFAULT;

        vertexBufferDescription.BindFlags =
            D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vertexData{};

        vertexData.pSysMem =
            vertices.data();

        HRESULT result =
            state_->device->CreateBuffer(
                &vertexBufferDescription,
                &vertexData,
                &state_->vertexBuffer);

        if (FAILED(result))
        {
            error =
                "Unable to create GPU vertex buffer.";

            return false;
        }

        D3D11_BUFFER_DESC indexBufferDescription{};

        indexBufferDescription.ByteWidth =
            static_cast<UINT>(
                mesh.indices.size() *
                sizeof(std::uint16_t));

        indexBufferDescription.Usage =
            D3D11_USAGE_DEFAULT;

        indexBufferDescription.BindFlags =
            D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA indexData{};

        indexData.pSysMem =
            mesh.indices.data();

        result =
            state_->device->CreateBuffer(
                &indexBufferDescription,
                &indexData,
                &state_->indexBuffer);

        if (FAILED(result))
        {
            error =
                "Unable to create GPU index buffer.";

            return false;
        }

        state_->indexCount =
            static_cast<std::uint32_t>(
                mesh.indices.size());

        state_->center =
        {
            (minimum.x + maximum.x) * 0.5f,
            (minimum.y + maximum.y) * 0.5f,
            (minimum.z + maximum.z) * 0.5f
        };

        const float sizeX =
            maximum.x -
            minimum.x;

        const float sizeY =
            maximum.y -
            minimum.y;

        const float sizeZ =
            maximum.z -
            minimum.z;

        const float largestSize =
            std::max(
                sizeX,
                std::max(
                    sizeY,
                    sizeZ));

        state_->radius =
            std::max(
                largestSize * 0.5f,
                0.5f);

        return true;
    }

    bool Renderer::Render(
        std::string& error)
    {
        error.clear();

        if (!state_ ||
            !state_->context ||
            !state_->swapChain ||
            !state_->vertexBuffer ||
            !state_->indexBuffer)
        {
            error =
                "Renderer has no valid mesh.";

            return false;
        }

        constexpr float ClearColour[4]
        {
            0.035f,
            0.045f,
            0.060f,
            1.0f
        };

        state_->context->ClearRenderTargetView(
            state_->renderTargetView.Get(),
            ClearColour);

        state_->context->ClearDepthStencilView(
            state_->depthStencilView.Get(),
            D3D11_CLEAR_DEPTH |
            D3D11_CLEAR_STENCIL,
            1.0f,
            0);

        ID3D11RenderTargetView* renderTargets[] =
        {
            state_->renderTargetView.Get()
        };

        state_->context->OMSetRenderTargets(
            1,
            renderTargets,
            state_->depthStencilView.Get());

        state_->context->OMSetDepthStencilState(
            state_->depthState.Get(),
            0);

        D3D11_VIEWPORT viewport{};

        viewport.TopLeftX =
            0.0f;

        viewport.TopLeftY =
            0.0f;

        viewport.Width =
            static_cast<float>(
                state_->width);

        viewport.Height =
            static_cast<float>(
                state_->height);

        viewport.MinDepth =
            0.0f;

        viewport.MaxDepth =
            1.0f;

        state_->context->RSSetViewports(
            1,
            &viewport);

        state_->context->RSSetState(
            state_->rasterizerState.Get());

        const auto now =
            std::chrono::steady_clock::now();

        const float elapsed =
            std::chrono::duration<float>(
                now -
                state_->startTime).count();

        using namespace DirectX;

        const XMVECTOR center =
            XMLoadFloat3(
                &state_->center);

        const XMMATRIX moveToOrigin =
            XMMatrixTranslation(
                -state_->center.x,
                -state_->center.y,
                -state_->center.z);

        const XMMATRIX rotation =
            XMMatrixRotationY(
                elapsed *
                0.30f);

        const XMMATRIX moveBack =
            XMMatrixTranslation(
                state_->center.x,
                state_->center.y,
                state_->center.z);

        const XMMATRIX world =
            moveToOrigin *
            rotation *
            moveBack;

        const float distance =
            std::max(
                state_->radius *
                    2.8f,
                3.0f);

        const XMVECTOR eye =
            XMVectorSet(
                state_->center.x +
                    state_->radius *
                    1.15f,
                state_->center.y +
                    state_->radius *
                    0.35f,
                state_->center.z -
                    distance,
                1.0f);

        const XMVECTOR up =
            XMVectorSet(
                0.0f,
                1.0f,
                0.0f,
                0.0f);

        const XMMATRIX view =
            XMMatrixLookAtLH(
                eye,
                center,
                up);

        const float aspect =
            static_cast<float>(
                state_->width) /
            static_cast<float>(
                state_->height);

        const XMMATRIX projection =
            XMMatrixPerspectiveFovLH(
                XMConvertToRadians(
                    60.0f),
                aspect,
                0.05f,
                std::max(
                    100.0f,
                    state_->radius *
                        20.0f));

        const XMMATRIX worldViewProjection =
            world *
            view *
            projection;

        SceneConstants constants{};

        XMStoreFloat4x4(
            &constants.world,
            XMMatrixTranspose(
                world));

        XMStoreFloat4x4(
            &constants.worldViewProjection,
            XMMatrixTranspose(
                worldViewProjection));

        state_->context->UpdateSubresource(
            state_->constantBuffer.Get(),
            0,
            nullptr,
            &constants,
            0,
            0);

        const UINT stride =
            sizeof(GpuVertex);

        const UINT offset =
            0;

        ID3D11Buffer* vertexBuffers[] =
        {
            state_->vertexBuffer.Get()
        };

        state_->context->IASetVertexBuffers(
            0,
            1,
            vertexBuffers,
            &stride,
            &offset);

        state_->context->IASetIndexBuffer(
            state_->indexBuffer.Get(),
            DXGI_FORMAT_R16_UINT,
            0);

        state_->context->IASetInputLayout(
            state_->inputLayout.Get());

        state_->context->IASetPrimitiveTopology(
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        state_->context->VSSetShader(
            state_->vertexShader.Get(),
            nullptr,
            0);

        ID3D11Buffer* constantBuffers[] =
        {
            state_->constantBuffer.Get()
        };

        state_->context->VSSetConstantBuffers(
            0,
            1,
            constantBuffers);

        state_->context->PSSetShader(
            state_->pixelShader.Get(),
            nullptr,
            0);

        state_->context->DrawIndexed(
            state_->indexCount,
            0,
            0);

        const HRESULT presentResult =
            state_->swapChain->Present(
                1,
                0);

        if (FAILED(presentResult))
        {
            error =
                "D3D11 Present failed.";

            return false;
        }

        return true;
    }

    void Renderer::Shutdown()
    {
        if (!state_)
        {
            return;
        }

        if (state_->context)
        {
            state_->context->ClearState();
            state_->context->Flush();
        }

        state_.reset();
    }
}