#include "Graphics/Renderer.h"

#include "Core/Log.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
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

        float nx;
        float ny;
        float nz;

        float u;
        float v;
    };

    struct SceneConstants final
    {
        DirectX::XMFLOAT4X4 world;
        DirectX::XMFLOAT4X4 viewProjection;

        std::array<
            DirectX::XMFLOAT4,
            4>
            terrainU{};

        std::array<
            DirectX::XMFLOAT4,
            4>
            terrainV{};

        DirectX::XMFLOAT4 groupColour;

        std::uint32_t useTerrain = 0;
        std::uint32_t terrainLayerCount = 0;

        float padding0 = 0.0f;
        float padding1 = 0.0f;
    };

    DirectX::XMFLOAT3 UnpackNormal(
        const std::uint32_t packed) noexcept
    {
        std::int32_t x =
            static_cast<std::int32_t>(
                packed & 0x7FFu);

        std::int32_t y =
            static_cast<std::int32_t>(
                (packed >> 11u) & 0x7FFu);

        std::int32_t z =
            static_cast<std::int32_t>(
                (packed >> 22u) & 0x3FFu);

        if ((x & 0x400) != 0)
        {
            x -= 0x800;
        }

        if ((y & 0x400) != 0)
        {
            y -= 0x800;
        }

        if ((z & 0x200) != 0)
        {
            z -= 0x400;
        }

        DirectX::XMVECTOR normal =
            DirectX::XMVectorSet(
                static_cast<float>(x) /
                    1023.0f,
                static_cast<float>(y) /
                    1023.0f,
                static_cast<float>(z) /
                    511.0f,
                0.0f);

        normal =
            DirectX::XMVector3Normalize(
                normal);

        DirectX::XMFLOAT3 result{};

        DirectX::XMStoreFloat3(
            &result,
            normal);

        return result;
    }

    DirectX::XMMATRIX ToMatrix(
        const core::math::Transform3x4& transform) noexcept
    {
        return DirectX::XMMATRIX(
            transform.values[0],
            transform.values[1],
            transform.values[2],
            0.0f,

            transform.values[3],
            transform.values[4],
            transform.values[5],
            0.0f,

            transform.values[6],
            transform.values[7],
            transform.values[8],
            0.0f,

            transform.values[9],
            transform.values[10],
            transform.values[11],
            1.0f);
    }

    DirectX::XMFLOAT4 PrimitiveGroupColour(
        const std::size_t index) noexcept
    {
        switch (index % 4)
        {
            case 0:
                return
                {
                    0.62f,
                    0.64f,
                    0.67f,
                    1.0f
                };

            case 1:
                return
                {
                    0.42f,
                    0.31f,
                    0.20f,
                    1.0f
                };

            case 2:
                return
                {
                    0.38f,
                    0.42f,
                    0.32f,
                    1.0f
                };

            default:
                return
                {
                    0.44f,
                    0.28f,
                    0.22f,
                    1.0f
                };
        }
    }

    constexpr char ShaderSource[] = R"(
        cbuffer SceneConstants : register(b0)
        {
            row_major float4x4 world;
            row_major float4x4 viewProjection;

            float4 terrainU[4];
            float4 terrainV[4];

            float4 groupColour;

            uint useTerrain;
            uint terrainLayerCount;

            float2 constantPadding;
        };

        Texture2D terrainTexture0 : register(t0);
        Texture2D terrainTexture1 : register(t1);
        Texture2D terrainTexture2 : register(t2);
        Texture2D terrainTexture3 : register(t3);

        Texture2D terrainBlend : register(t4);

        SamplerState terrainTextureSampler : register(s0);
        SamplerState terrainBlendSampler   : register(s1);

        struct VertexInput
        {
            float3 position : POSITION;
            float3 normal   : NORMAL;
            float2 texcoord : TEXCOORD0;
        };

        struct PixelInput
        {
            float4 position      : SV_POSITION;
            float3 normal        : NORMAL;
            float3 localPosition : TEXCOORD0;
            float2 terrainUV     : TEXCOORD1;
        };

        PixelInput VSMain(VertexInput input)
        {
            PixelInput output;

            float4 worldPosition =
                mul(
                    float4(input.position, 1.0f),
                    world);

            output.position =
                mul(
                    worldPosition,
                    viewProjection);

            output.normal =
                normalize(
                    mul(
                        float4(input.normal, 0.0f),
                        world).xyz);

            output.localPosition =
                input.position;

            output.terrainUV =
                input.texcoord;

            return output;
        }

        float2 ProjectTerrainUV(
            float3 position,
            float4 uProjection,
            float4 vProjection)
        {
            float4 p =
                float4(
                    position,
                    1.0f);

            return float2(
                dot(p, uProjection),
                dot(p, vProjection));
        }

        float3 SampleTerrain(PixelInput input)
        {
            float4 weights =
                terrainBlend.Sample(
                    terrainBlendSampler,
                    input.terrainUV);

            float3 colour =
                0.0f;

            if (terrainLayerCount >= 1)
            {
                colour +=
                    terrainTexture0.Sample(
                        terrainTextureSampler,
                        ProjectTerrainUV(
                            input.localPosition,
                            terrainU[0],
                            terrainV[0])).rgb *
                    weights.r;
            }

            if (terrainLayerCount >= 2)
            {
                colour +=
                    terrainTexture1.Sample(
                        terrainTextureSampler,
                        ProjectTerrainUV(
                            input.localPosition,
                            terrainU[1],
                            terrainV[1])).rgb *
                    weights.g;
            }

            if (terrainLayerCount >= 3)
            {
                colour +=
                    terrainTexture2.Sample(
                        terrainTextureSampler,
                        ProjectTerrainUV(
                            input.localPosition,
                            terrainU[2],
                            terrainV[2])).rgb *
                    weights.b;
            }

            if (terrainLayerCount >= 4)
            {
                colour +=
                    terrainTexture3.Sample(
                        terrainTextureSampler,
                        ProjectTerrainUV(
                            input.localPosition,
                            terrainU[3],
                            terrainV[3])).rgb *
                    weights.a;
            }

            return colour;
        }

        float4 PSMain(PixelInput input) : SV_TARGET
        {
            float3 normal =
                normalize(
                    input.normal);

            float3 lightDirection =
                normalize(
                    float3(
                        -0.35f,
                        0.85f,
                        -0.40f));

            float diffuse =
                abs(
                    dot(
                        normal,
                        lightDirection));

            float lighting =
                0.20f +
                diffuse * 0.80f;

            float3 baseColour =
                groupColour.rgb;

            if (useTerrain != 0)
            {
                baseColour =
                    SampleTerrain(
                        input);
            }

            return float4(
                baseColour *
                    lighting,
                1.0f);
        }
    )";

    bool CreateRgbaTexture(
        ID3D11Device* device,
        ID3D11DeviceContext* context,
        const core::images::RgbaImage& image,
        ComPtr<ID3D11ShaderResourceView>& output,
        std::string& error)
    {
        output.Reset();

        if (device == nullptr ||
            context == nullptr ||
            image.width == 0 ||
            image.height == 0)
        {
            error =
                "Invalid RGBA texture parameters.";

            return false;
        }

        const std::size_t expectedSize =
            static_cast<std::size_t>(
                image.width) *
            image.height *
            4;

        if (image.pixels.size() !=
            expectedSize)
        {
            error =
                "RGBA texture pixel size is invalid.";

            return false;
        }

        D3D11_TEXTURE2D_DESC description{};

        description.Width =
            image.width;

        description.Height =
            image.height;

        description.MipLevels =
            0;

        description.ArraySize =
            1;

        description.Format =
            DXGI_FORMAT_R8G8B8A8_UNORM;

        description.SampleDesc.Count =
            1;

        description.Usage =
            D3D11_USAGE_DEFAULT;

        description.BindFlags =
            D3D11_BIND_SHADER_RESOURCE |
            D3D11_BIND_RENDER_TARGET;

        description.MiscFlags =
            D3D11_RESOURCE_MISC_GENERATE_MIPS;

        ComPtr<ID3D11Texture2D>
            texture;

        HRESULT result =
            device->CreateTexture2D(
                &description,
                nullptr,
                &texture);

        if (FAILED(result))
        {
            error =
                "Unable to create RGBA texture.";

            return false;
        }

        context->UpdateSubresource(
            texture.Get(),
            0,
            nullptr,
            image.pixels.data(),
            image.width * 4,
            0);

        D3D11_SHADER_RESOURCE_VIEW_DESC
            viewDescription{};

        viewDescription.Format =
            description.Format;

        viewDescription.ViewDimension =
            D3D11_SRV_DIMENSION_TEXTURE2D;

        viewDescription.Texture2D.MostDetailedMip =
            0;

        viewDescription.Texture2D.MipLevels =
            static_cast<UINT>(-1);

        result =
            device->CreateShaderResourceView(
                texture.Get(),
                &viewDescription,
                &output);

        if (FAILED(result))
        {
            error =
                "Unable to create texture SRV.";

            return false;
        }

        context->GenerateMips(
            output.Get());

        return true;
    }

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
        struct GpuTerrainPass final
        {
            std::uint32_t layerCount = 0;

            std::array<
                std::size_t,
                4>
                textureIndices{};

            std::array<
                DirectX::XMFLOAT4,
                4>
                uProjection{};

            std::array<
                DirectX::XMFLOAT4,
                4>
                vProjection{};

            ComPtr<ID3D11ShaderResourceView>
                blendView;
        };

        struct GpuTerrainMaterial final
        {
            std::vector<GpuTerrainPass>
                passes;
        };

        struct GpuMesh final
        {
            ComPtr<ID3D11Buffer>
                vertexBuffer;

            ComPtr<ID3D11Buffer>
                indexBuffer;

            std::uint32_t indexCount = 0;
            std::int32_t terrainMaterialIndex = -1;

            std::vector<
                core::assets::MeshPrimitiveGroup>
                primitiveGroups;

            DirectX::XMFLOAT3 minimum{};
            DirectX::XMFLOAT3 maximum{};
        };

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

        ComPtr<ID3D11DepthStencilState>
            depthReadState;

        ComPtr<ID3D11BlendState>
            additiveBlendState;

        ComPtr<ID3D11SamplerState>
            terrainTextureSampler;

        ComPtr<ID3D11SamplerState>
            terrainBlendSampler;

        ComPtr<ID3D11RasterizerState>
            rasterizerState;

        ComPtr<ID3D11VertexShader>
            vertexShader;

        ComPtr<ID3D11PixelShader>
            pixelShader;

        ComPtr<ID3D11InputLayout>
            inputLayout;

        ComPtr<ID3D11Buffer>
            constantBuffer;

        std::vector<GpuMesh> meshes;

        std::vector<
            ComPtr<ID3D11ShaderResourceView>>
            textures;

        std::vector<GpuTerrainMaterial>
            terrainMaterials;

        std::vector<SceneInstance>
            instances;

        std::uint32_t width = 0;
        std::uint32_t height = 0;

        DirectX::XMFLOAT3 sceneCenter{};

        float sceneRadius =
            1.0f;

        CameraView camera{};
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

        D3D_FEATURE_LEVEL createdFeatureLevel{};

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
                "Unable to create D3D11 device.";

            return false;
        }

        ComPtr<ID3D11Texture2D>
            backBuffer;

        result =
            state_->swapChain->GetBuffer(
                0,
                IID_PPV_ARGS(
                    &backBuffer));

        if (FAILED(result))
        {
            error =
                "Unable to get back buffer.";

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
                "Unable to create render target.";

            return false;
        }

        D3D11_TEXTURE2D_DESC
            depthDescription{};

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
                "Unable to create depth view.";

            return false;
        }

        D3D11_DEPTH_STENCIL_DESC
            depthStateDescription{};

        depthStateDescription.DepthEnable =
            TRUE;

        depthStateDescription.DepthWriteMask =
            D3D11_DEPTH_WRITE_MASK_ALL;

        depthStateDescription.DepthFunc =
            D3D11_COMPARISON_LESS;

        depthStateDescription.StencilEnable =
            FALSE;

        HRESULT depthResult =
            state_->device->CreateDepthStencilState(
                &depthStateDescription,
                &state_->depthState);

        if (FAILED(depthResult))
        {
            error =
                "Unable to create depth state.";

            return false;
        }

        D3D11_DEPTH_STENCIL_DESC
            depthReadDescription =
                depthStateDescription;

        depthReadDescription.DepthWriteMask =
            D3D11_DEPTH_WRITE_MASK_ZERO;

        depthReadDescription.DepthFunc =
            D3D11_COMPARISON_LESS_EQUAL;

        depthResult =
            state_->device->CreateDepthStencilState(
                &depthReadDescription,
                &state_->depthReadState);

        if (FAILED(depthResult))
        {
            error =
                "Unable to create terrain depth read state.";

            return false;
        }

        D3D11_RASTERIZER_DESC
            rasterizerDescription{};

        rasterizerDescription.FillMode =
            D3D11_FILL_SOLID;

        rasterizerDescription.CullMode =
            D3D11_CULL_NONE;

        rasterizerDescription.DepthClipEnable =
            TRUE;

        result =
            state_->device->CreateRasterizerState(
                &rasterizerDescription,
                &state_->rasterizerState);

        D3D11_BLEND_DESC
            blendDescription{};

        blendDescription.RenderTarget[0].BlendEnable =
            TRUE;

        blendDescription.RenderTarget[0].SrcBlend =
            D3D11_BLEND_ONE;

        blendDescription.RenderTarget[0].DestBlend =
            D3D11_BLEND_ONE;

        blendDescription.RenderTarget[0].BlendOp =
            D3D11_BLEND_OP_ADD;

        blendDescription.RenderTarget[0].SrcBlendAlpha =
            D3D11_BLEND_ONE;

        blendDescription.RenderTarget[0].DestBlendAlpha =
            D3D11_BLEND_ONE;

        blendDescription.RenderTarget[0].BlendOpAlpha =
            D3D11_BLEND_OP_ADD;

        blendDescription.RenderTarget[0].RenderTargetWriteMask =
            D3D11_COLOR_WRITE_ENABLE_ALL;

        result =
            state_->device->CreateBlendState(
                &blendDescription,
                &state_->additiveBlendState);

        if (FAILED(result))
        {
            error =
                "Unable to create terrain additive blend state.";

            return false;
        }

        D3D11_SAMPLER_DESC
            textureSamplerDescription{};

        textureSamplerDescription.Filter =
            D3D11_FILTER_ANISOTROPIC;

        textureSamplerDescription.AddressU =
            D3D11_TEXTURE_ADDRESS_WRAP;

        textureSamplerDescription.AddressV =
            D3D11_TEXTURE_ADDRESS_WRAP;

        textureSamplerDescription.AddressW =
            D3D11_TEXTURE_ADDRESS_WRAP;

        textureSamplerDescription.MaxAnisotropy =
            16;

        textureSamplerDescription.ComparisonFunc =
            D3D11_COMPARISON_ALWAYS;

        textureSamplerDescription.MinLOD =
            0.0f;

        textureSamplerDescription.MaxLOD =
            D3D11_FLOAT32_MAX;

        result =
            state_->device->CreateSamplerState(
                &textureSamplerDescription,
                &state_->terrainTextureSampler);

        if (FAILED(result))
        {
            error =
                "Unable to create terrain texture sampler.";

            return false;
        }

        D3D11_SAMPLER_DESC
            blendSamplerDescription =
                textureSamplerDescription;

        blendSamplerDescription.Filter =
            D3D11_FILTER_MIN_MAG_MIP_LINEAR;

        blendSamplerDescription.AddressU =
            D3D11_TEXTURE_ADDRESS_CLAMP;

        blendSamplerDescription.AddressV =
            D3D11_TEXTURE_ADDRESS_CLAMP;

        blendSamplerDescription.AddressW =
            D3D11_TEXTURE_ADDRESS_CLAMP;

        blendSamplerDescription.MaxAnisotropy =
            1;

        result =
            state_->device->CreateSamplerState(
                &blendSamplerDescription,
                &state_->terrainBlendSampler);

        if (FAILED(result))
        {
            error =
                "Unable to create terrain blend sampler.";

            return false;
        }

        if (FAILED(result))
        {
            error =
                "Unable to create rasterizer.";

            return false;
        }

        ComPtr<ID3DBlob>
            vertexShaderCode;

        if (!CompileShader(
                "VSMain",
                "vs_5_0",
                &vertexShaderCode,
                error))
        {
            return false;
        }

        ComPtr<ID3DBlob>
            pixelShaderCode;

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

        const D3D11_INPUT_ELEMENT_DESC
            inputElements[] =
        {
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
                "NORMAL",
                0,
                DXGI_FORMAT_R32G32B32_FLOAT,
                0,
                12,
                D3D11_INPUT_PER_VERTEX_DATA,
                0
            },
            {
                "TEXCOORD",
                0,
                DXGI_FORMAT_R32G32_FLOAT,
                0,
                24,
                D3D11_INPUT_PER_VERTEX_DATA,
                0
            }
        };

        result =
            state_->device->CreateInputLayout(
                inputElements,
                3,
                vertexShaderCode->GetBufferPointer(),
                vertexShaderCode->GetBufferSize(),
                &state_->inputLayout);

        if (FAILED(result))
        {
            error =
                "Unable to create input layout.";

            return false;
        }

        D3D11_BUFFER_DESC
            constantDescription{};

        constantDescription.ByteWidth =
            sizeof(SceneConstants);

        constantDescription.Usage =
            D3D11_USAGE_DEFAULT;

        constantDescription.BindFlags =
            D3D11_BIND_CONSTANT_BUFFER;

        result =
            state_->device->CreateBuffer(
                &constantDescription,
                nullptr,
                &state_->constantBuffer);

        if (FAILED(result))
        {
            error =
                "Unable to create constant buffer.";

            return false;
        }

        state_->width =
            width;

        state_->height =
            height;

        return true;
    }

    bool Renderer::SetScene(
        const SceneRenderData& scene,
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

        if (scene.meshes.empty() ||
            scene.instances.empty())
        {
            error =
                "Scene contains no geometry.";

            return false;
        }

        state_->meshes.clear();
        state_->instances.clear();

        state_->meshes.reserve(
            scene.meshes.size());

        for (const SceneMesh& sceneMesh :
             scene.meshes)
        {
            const core::assets::MeshData& mesh =
                sceneMesh.geometry;
            
            if (mesh.vertices.empty() ||
                mesh.indices.empty())
            {
                error =
                    "Scene contains empty mesh.";

                return false;
            }

            std::vector<GpuVertex> vertices;

            vertices.reserve(
                mesh.vertices.size());

            DirectX::XMFLOAT3 minimum
            {
                mesh.vertices.front().position.x,
                mesh.vertices.front().position.y,
                mesh.vertices.front().position.z
            };

            DirectX::XMFLOAT3 maximum =
                minimum;

            for (const core::assets::MeshVertex& vertex :
                 mesh.vertices)
            {
                const DirectX::XMFLOAT3 normal =
                    UnpackNormal(
                        vertex.packedNormal);

                vertices.push_back({
                    vertex.position.x,
                    vertex.position.y,
                    vertex.position.z,

                    normal.x,
                    normal.y,
                    normal.z,

                    vertex.u,
                    vertex.v
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

            if (vertices.size() >
                std::numeric_limits<UINT>::max() /
                    sizeof(GpuVertex))
            {
                error =
                    "Scene vertex buffer is too large.";

                return false;
            }

            if (mesh.indices.size() >
                std::numeric_limits<UINT>::max() /
                    sizeof(std::uint16_t))
            {
                error =
                    "Scene index buffer is too large.";

                return false;
            }

            State::GpuMesh gpuMesh;

            D3D11_BUFFER_DESC
                vertexDescription{};

            vertexDescription.ByteWidth =
                static_cast<UINT>(
                    vertices.size() *
                    sizeof(GpuVertex));

            vertexDescription.Usage =
                D3D11_USAGE_DEFAULT;

            vertexDescription.BindFlags =
                D3D11_BIND_VERTEX_BUFFER;

            D3D11_SUBRESOURCE_DATA
                vertexData{};

            vertexData.pSysMem =
                vertices.data();

            HRESULT result =
                state_->device->CreateBuffer(
                    &vertexDescription,
                    &vertexData,
                    &gpuMesh.vertexBuffer);

            if (FAILED(result))
            {
                error =
                    "Unable to create world vertex buffer.";

                return false;
            }

            D3D11_BUFFER_DESC
                indexDescription{};

            indexDescription.ByteWidth =
                static_cast<UINT>(
                    mesh.indices.size() *
                    sizeof(std::uint16_t));

            indexDescription.Usage =
                D3D11_USAGE_DEFAULT;

            indexDescription.BindFlags =
                D3D11_BIND_INDEX_BUFFER;

            D3D11_SUBRESOURCE_DATA
                indexData{};

            indexData.pSysMem =
                mesh.indices.data();

            result =
                state_->device->CreateBuffer(
                    &indexDescription,
                    &indexData,
                    &gpuMesh.indexBuffer);

            if (FAILED(result))
            {
                error =
                    "Unable to create world index buffer.";

                return false;
            }

            gpuMesh.indexCount =
                static_cast<std::uint32_t>(
                    mesh.indices.size());

            gpuMesh.primitiveGroups =
                mesh.primitiveGroups;

            gpuMesh.minimum =
                minimum;

            gpuMesh.maximum =
                maximum;

            gpuMesh.terrainMaterialIndex =
                sceneMesh.terrainMaterialIndex;

            state_->meshes.push_back(
                std::move(gpuMesh));
        }

        state_->textures.clear();

        state_->textures.reserve(
            scene.textures.size());

        for (const SceneTextureData& texture :
             scene.textures)
        {
            ComPtr<ID3D11ShaderResourceView>
                view;

            if (!CreateRgbaTexture(
                    state_->device.Get(),
                    state_->context.Get(),
                    texture.image,
                    view,
                    error))
            {
                error =
                    texture.logicalPath +
                    ": " +
                    error;

                return false;
            }

            state_->textures.push_back(
                std::move(view));
        }

        state_->terrainMaterials.clear();

        state_->terrainMaterials.reserve(
            scene.terrainMaterials.size());

        for (const SceneTerrainMaterial& sourceMaterial :
             scene.terrainMaterials)
        {
            State::GpuTerrainMaterial
                material;

            material.passes.reserve(
                sourceMaterial.passes.size());

            for (const SceneTerrainPass& sourcePass :
                 sourceMaterial.passes)
            {
                State::GpuTerrainPass
                    pass;

                pass.layerCount =
                    sourcePass.layerCount;

                for (std::size_t layerIndex = 0;
                     layerIndex < 4;
                     ++layerIndex)
                {
                    pass.textureIndices[
                        layerIndex] =
                        sourcePass.layers[
                            layerIndex].textureIndex;

                    pass.uProjection[
                        layerIndex] =
                    {
                        sourcePass.layers[
                            layerIndex].uProjection[0],

                        sourcePass.layers[
                            layerIndex].uProjection[1],

                        sourcePass.layers[
                            layerIndex].uProjection[2],

                        sourcePass.layers[
                            layerIndex].uProjection[3]
                    };

                    pass.vProjection[
                        layerIndex] =
                    {
                        sourcePass.layers[
                            layerIndex].vProjection[0],

                        sourcePass.layers[
                            layerIndex].vProjection[1],

                        sourcePass.layers[
                            layerIndex].vProjection[2],

                        sourcePass.layers[
                            layerIndex].vProjection[3]
                    };

                    if (layerIndex <
                            pass.layerCount &&
                        pass.textureIndices[
                            layerIndex] >=
                            state_->textures.size())
                    {
                        error =
                            "Terrain material references invalid texture.";

                        return false;
                    }
                }

                if (!CreateRgbaTexture(
                        state_->device.Get(),
                        state_->context.Get(),
                        sourcePass.blendMap,
                        pass.blendView,
                        error))
                {
                    error =
                        "Unable to create terrain blend texture: " +
                        error;

                    return false;
                }

                material.passes.push_back(
                    std::move(pass));
            }

            state_->terrainMaterials.push_back(
                std::move(material));
        }

        state_->instances =
            scene.instances;

        bool hasBounds = false;

        DirectX::XMFLOAT3 sceneMinimum{};
        DirectX::XMFLOAT3 sceneMaximum{};

        for (const SceneInstance& instance :
             state_->instances)
        {
            if (instance.meshIndex >=
                state_->meshes.size())
            {
                error =
                    "Scene contains invalid mesh index.";

                return false;
            }

            const State::GpuMesh& mesh =
                state_->meshes[
                    instance.meshIndex];

            const DirectX::XMMATRIX world =
                ToMatrix(
                    instance.transform);

            const std::array<
                DirectX::XMFLOAT3,
                8>
                corners
            {{
                {
                    mesh.minimum.x,
                    mesh.minimum.y,
                    mesh.minimum.z
                },
                {
                    mesh.maximum.x,
                    mesh.minimum.y,
                    mesh.minimum.z
                },
                {
                    mesh.minimum.x,
                    mesh.maximum.y,
                    mesh.minimum.z
                },
                {
                    mesh.maximum.x,
                    mesh.maximum.y,
                    mesh.minimum.z
                },
                {
                    mesh.minimum.x,
                    mesh.minimum.y,
                    mesh.maximum.z
                },
                {
                    mesh.maximum.x,
                    mesh.minimum.y,
                    mesh.maximum.z
                },
                {
                    mesh.minimum.x,
                    mesh.maximum.y,
                    mesh.maximum.z
                },
                {
                    mesh.maximum.x,
                    mesh.maximum.y,
                    mesh.maximum.z
                }
            }};

            for (const DirectX::XMFLOAT3& corner :
                 corners)
            {
                DirectX::XMVECTOR point =
                    DirectX::XMLoadFloat3(
                        &corner);

                point =
                    DirectX::XMVector3TransformCoord(
                        point,
                        world);

                DirectX::XMFLOAT3 transformed{};

                DirectX::XMStoreFloat3(
                    &transformed,
                    point);

                if (!hasBounds)
                {
                    sceneMinimum =
                        transformed;

                    sceneMaximum =
                        transformed;

                    hasBounds = true;

                    continue;
                }

                sceneMinimum.x =
                    std::min(
                        sceneMinimum.x,
                        transformed.x);

                sceneMinimum.y =
                    std::min(
                        sceneMinimum.y,
                        transformed.y);

                sceneMinimum.z =
                    std::min(
                        sceneMinimum.z,
                        transformed.z);

                sceneMaximum.x =
                    std::max(
                        sceneMaximum.x,
                        transformed.x);

                sceneMaximum.y =
                    std::max(
                        sceneMaximum.y,
                        transformed.y);

                sceneMaximum.z =
                    std::max(
                        sceneMaximum.z,
                        transformed.z);
            }
        }

        if (!hasBounds)
        {
            error =
                "Unable to calculate world bounds.";

            return false;
        }

        state_->sceneCenter =
        {
            (sceneMinimum.x +
             sceneMaximum.x) *
                0.5f,

            (sceneMinimum.y +
             sceneMaximum.y) *
                0.5f,

            (sceneMinimum.z +
             sceneMaximum.z) *
                0.5f
        };

        const float sizeX =
            sceneMaximum.x -
            sceneMinimum.x;

        const float sizeY =
            sceneMaximum.y -
            sceneMinimum.y;

        const float sizeZ =
            sceneMaximum.z -
            sceneMinimum.z;

        state_->sceneRadius =
            std::sqrt(
                sizeX * sizeX +
                sizeY * sizeY +
                sizeZ * sizeZ) *
            0.5f;

        state_->sceneRadius =
            std::max(
                state_->sceneRadius,
                10.0f);

        core::Log::Info(
            std::string("World bounds: X=") +
            std::to_string(sizeX) +
            ", Y=" +
            std::to_string(sizeY) +
            ", Z=" +
            std::to_string(sizeZ));

        core::Log::Info(
            std::string("GPU meshes created: ") +
            std::to_string(
                state_->meshes.size()));

        core::Log::Info(
            std::string("GPU scene instances: ") +
            std::to_string(
                state_->instances.size()));

        return true;
    }

    void Renderer::SetCamera(
        const CameraView& camera) noexcept
    {
        if (!state_)
        {
            return;
        }

        state_->camera =
            camera;
    }

    core::math::Vector3
    Renderer::SceneCenter() const noexcept
    {
        if (!state_)
        {
            return {};
        }

        return
        {
            state_->sceneCenter.x,
            state_->sceneCenter.y,
            state_->sceneCenter.z
        };
    }

    float Renderer::SceneRadius() const noexcept
    {
        if (!state_)
        {
            return 1.0f;
        }

        return
            state_->sceneRadius;
    }

    bool Renderer::Render(
        std::string& error)
    {
        error.clear();

        if (!state_ ||
            !state_->context ||
            !state_->swapChain ||
            state_->meshes.empty())
        {
            error =
                "Renderer has no world scene.";

            return false;
        }

        constexpr float ClearColour[4]
        {
            0.018f,
            0.025f,
            0.035f,
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

        ID3D11RenderTargetView*
            renderTargets[] =
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

        using namespace DirectX;

        XMVECTOR eye =
            XMVectorSet(
                state_->camera.position.x,
                state_->camera.position.y,
                state_->camera.position.z,
                1.0f);

        XMVECTOR forward =
            XMVectorSet(
                state_->camera.forward.x,
                state_->camera.forward.y,
                state_->camera.forward.z,
                0.0f);

        forward =
            XMVector3Normalize(
                forward);

        const XMVECTOR target =
            XMVectorAdd(
                eye,
                forward);

        XMVECTOR up =
            XMVectorSet(
                state_->camera.up.x,
                state_->camera.up.y,
                state_->camera.up.z,
                0.0f);

        up =
            XMVector3Normalize(
                up);

        const XMMATRIX view =
            XMMatrixLookAtLH(
                eye,
                target,
                up);

        const float aspect =
            static_cast<float>(
                state_->width) /
            static_cast<float>(
                state_->height);

        const XMMATRIX projection =
            XMMatrixPerspectiveFovLH(
                XMConvertToRadians(
                std::clamp(
                    state_->camera.fieldOfViewDegrees,
                    20.0f,
                    90.0f)),
                aspect,
                0.1f,
                std::max(
                    5000.0f,
                    state_->sceneRadius *
                        10.0f));

        const XMMATRIX viewProjection =
            view *
            projection;

        const UINT stride =
            sizeof(GpuVertex);

        const UINT offset =
            0;

        state_->context->IASetInputLayout(
            state_->inputLayout.Get());

        state_->context->IASetPrimitiveTopology(
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        state_->context->VSSetShader(
            state_->vertexShader.Get(),
            nullptr,
            0);

        state_->context->PSSetShader(
            state_->pixelShader.Get(),
            nullptr,
            0);

        ID3D11SamplerState*
            terrainSamplers[] =
        {
            state_->terrainTextureSampler.Get(),
            state_->terrainBlendSampler.Get()
        };

        state_->context->PSSetSamplers(
            0,
            2,
            terrainSamplers);

        ID3D11Buffer*
            constantBuffers[] =
        {
            state_->constantBuffer.Get()
        };

        state_->context->VSSetConstantBuffers(
            0,
            1,
            constantBuffers);

        state_->context->PSSetConstantBuffers(
            0,
            1,
            constantBuffers);

        SceneConstants constants{};

        XMStoreFloat4x4(
            &constants.viewProjection,
            viewProjection);

        for (const SceneInstance& instance :
             state_->instances)
        {
            const State::GpuMesh& mesh =
                state_->meshes[
                    instance.meshIndex];

            ID3D11Buffer*
                vertexBuffers[] =
            {
                mesh.vertexBuffer.Get()
            };

            state_->context->IASetVertexBuffers(
                0,
                1,
                vertexBuffers,
                &stride,
                &offset);

            state_->context->IASetIndexBuffer(
                mesh.indexBuffer.Get(),
                DXGI_FORMAT_R16_UINT,
                0);

            const XMMATRIX world =
                ToMatrix(
                    instance.transform);

            XMStoreFloat4x4(
                &constants.world,
                world);

            if (mesh.terrainMaterialIndex >= 0)
            {
                const std::size_t materialIndex =
                    static_cast<std::size_t>(
                        mesh.terrainMaterialIndex);

                if (materialIndex >=
                    state_->terrainMaterials.size())
                {
                    error =
                        "GPU mesh contains invalid terrain material.";

                    return false;
                }

                const State::GpuTerrainMaterial& material =
                    state_->terrainMaterials[
                        materialIndex];

                constexpr float BlendFactor[4]
                {
                    0.0f,
                    0.0f,
                    0.0f,
                    0.0f
                };

                for (std::size_t passIndex = 0;
                     passIndex <
                        material.passes.size();
                     ++passIndex)
                {
                    const State::GpuTerrainPass& pass =
                        material.passes[
                            passIndex];

                    constants.useTerrain =
                        1;

                    constants.terrainLayerCount =
                        pass.layerCount;

                    for (std::size_t layerIndex = 0;
                         layerIndex < 4;
                         ++layerIndex)
                    {
                        constants.terrainU[
                            layerIndex] =
                            pass.uProjection[
                                layerIndex];

                        constants.terrainV[
                            layerIndex] =
                            pass.vProjection[
                                layerIndex];
                    }

                    ID3D11ShaderResourceView*
                        views[5]
                    {
                        nullptr,
                        nullptr,
                        nullptr,
                        nullptr,
                        pass.blendView.Get()
                    };

                    for (std::size_t layerIndex = 0;
                         layerIndex <
                            pass.layerCount;
                         ++layerIndex)
                    {
                        views[layerIndex] =
                            state_->textures[
                                pass.textureIndices[
                                    layerIndex]].Get();
                    }

                    state_->context->PSSetShaderResources(
                        0,
                        5,
                        views);

                    if (passIndex == 0)
                    {
                        state_->context->OMSetBlendState(
                            nullptr,
                            BlendFactor,
                            0xFFFFFFFFu);

                        state_->context->OMSetDepthStencilState(
                            state_->depthState.Get(),
                            0);
                    }
                    else
                    {
                        state_->context->OMSetBlendState(
                            state_->additiveBlendState.Get(),
                            BlendFactor,
                            0xFFFFFFFFu);

                        state_->context->OMSetDepthStencilState(
                            state_->depthReadState.Get(),
                            0);
                    }

                    state_->context->UpdateSubresource(
                        state_->constantBuffer.Get(),
                        0,
                        nullptr,
                        &constants,
                        0,
                        0);

                    state_->context->DrawIndexed(
                        mesh.indexCount,
                        0,
                        0);
                }

                ID3D11ShaderResourceView*
                    emptyViews[5]
                {
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr
                };

                state_->context->PSSetShaderResources(
                    0,
                    5,
                    emptyViews);

                state_->context->OMSetBlendState(
                    nullptr,
                    nullptr,
                    0xFFFFFFFFu);

                state_->context->OMSetDepthStencilState(
                    state_->depthState.Get(),
                    0);

                continue;
            }

            constants.useTerrain =
                0;

            constants.terrainLayerCount =
                0;

            if (mesh.primitiveGroups.empty())
            {
                constants.groupColour =
                {
                    0.62f,
                    0.64f,
                    0.67f,
                    1.0f
                };

                state_->context->UpdateSubresource(
                    state_->constantBuffer.Get(),
                    0,
                    nullptr,
                    &constants,
                    0,
                    0);

                state_->context->DrawIndexed(
                    mesh.indexCount,
                    0,
                    0);

                continue;
            }

            for (std::size_t groupIndex = 0;
                 groupIndex <
                    mesh.primitiveGroups.size();
                 ++groupIndex)
            {
                const core::assets::MeshPrimitiveGroup& group =
                    mesh.primitiveGroups[
                        groupIndex];

                constants.groupColour =
                    PrimitiveGroupColour(
                        groupIndex);

                state_->context->UpdateSubresource(
                    state_->constantBuffer.Get(),
                    0,
                    nullptr,
                    &constants,
                    0,
                    0);

                state_->context->DrawIndexed(
                    group.primitiveCount *
                        3u,
                    group.startIndex,
                    0);
            }
        }

        const HRESULT result =
            state_->swapChain->Present(
                1,
                0);

        if (FAILED(result))
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