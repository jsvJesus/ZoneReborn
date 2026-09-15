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
        DirectX::XMFLOAT4X4 inverseViewProjection;

        std::array<
            DirectX::XMFLOAT4,
            4>
            terrainU{};

        std::array<
            DirectX::XMFLOAT4,
            4>
            terrainV{};

        DirectX::XMFLOAT4 groupColour;

        std::uint32_t useTerrain =
            0;

        std::uint32_t terrainLayerCount =
            0;

        std::uint32_t useModelTexture =
            0;

        std::uint32_t useWater =
            0;

        DirectX::XMFLOAT4 modelParameters;

        DirectX::XMFLOAT4 waterDeepColour;
        DirectX::XMFLOAT4 waterReflectionTint;
        DirectX::XMFLOAT4 waterRefractionTint;

        DirectX::XMFLOAT4 waterParameters0;
        DirectX::XMFLOAT4 waterParameters1;
        DirectX::XMFLOAT4 waterScrollSpeed1;
        DirectX::XMFLOAT4 waterScrollSpeed2;
        DirectX::XMFLOAT4 waterParameters2;
        DirectX::XMFLOAT4 waterParameters3;

        DirectX::XMFLOAT4 cameraPosition;
        DirectX::XMFLOAT4 screenParameters;
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
            row_major float4x4 inverseViewProjection;

            float4 terrainU[4];
            float4 terrainV[4];

            float4 groupColour;

            uint useTerrain;
            uint terrainLayerCount;
            uint useModelTexture;
            uint useWater;

            float4 modelParameters;

            float4 waterDeepColour;
            float4 waterReflectionTint;
            float4 waterRefractionTint;

            float4 waterParameters0;
            float4 waterParameters1;
            float4 waterScrollSpeed1;
            float4 waterScrollSpeed2;
            float4 waterParameters2;
            float4 waterParameters3;

            float4 cameraPosition;
            float4 screenParameters;
        }

        Texture2D terrainTexture0 : register(t0);
        Texture2D terrainTexture1 : register(t1);
        Texture2D terrainTexture2 : register(t2);
        Texture2D terrainTexture3 : register(t3);

        Texture2D terrainBlend : register(t4);
        Texture2D modelTexture : register(t5);

        Texture2D sceneColourTexture : register(t6);
        Texture2D sceneDepthTexture  : register(t7);
        Texture2D waterNormalTexture : register(t8);
        Texture2D waterFoamTexture   : register(t9);

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
            float3 worldPosition : TEXCOORD2;
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

            output.worldPosition =
                worldPosition.xyz;

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

        float2 GetScreenUV(
            PixelInput input)
        {
            return
                input.position.xy /
                screenParameters.xy;
        }

        float3 ReconstructWorldPosition(
            float2 uv,
            float depth)
        {
            float2 ndc;

            ndc.x =
                uv.x * 2.0f -
                1.0f;

            ndc.y =
                1.0f -
                uv.y * 2.0f;

            float4 position =
                mul(
                    float4(
                        ndc,
                        depth,
                        1.0f),
                    inverseViewProjection);

            position.xyz /=
                max(
                    position.w,
                    0.00001f);

            return
                position.xyz;
        }

        float2 ProjectWorldPosition(
            float3 position,
            out float depth)
        {
            float4 clipPosition =
                mul(
                    float4(
                        position,
                        1.0f),
                    viewProjection);

            float inverseW =
                1.0f /
                max(
                    clipPosition.w,
                    0.00001f);

            float2 ndc =
                clipPosition.xy *
                inverseW;

            depth =
                clipPosition.z *
                inverseW;

            return float2(
                ndc.x * 0.5f +
                    0.5f,
                -ndc.y * 0.5f +
                    0.5f);
        }

        float3 CalculateWaterNormal(
            PixelInput input)
        {
            float textureScale =
                max(
                    waterParameters2.x,
                    0.001f);

            float time =
                waterParameters1.w;

            float wind =
                waterParameters1.z;

            float2 baseUV =
                input.localPosition.xz /
                textureScale;

            float2 uv1 =
                baseUV +
                waterScrollSpeed1.xy *
                wind *
                time;

            float2 uv2 =
                baseUV +
                waterScrollSpeed2.xy *
                wind *
                time;

            float2 normal1 =
                waterNormalTexture.Sample(
                    terrainTextureSampler,
                    uv1).rg *
                    2.0f -
                1.0f;

            float2 normal2 =
                waterNormalTexture.Sample(
                    terrainTextureSampler,
                    uv2).rg *
                    2.0f -
                1.0f;

            float2 wave =
                (
                    normal1 +
                    normal2
                ) *
                0.5f;

            wave.x *=
                waterParameters1.x;

            wave.y *=
                waterParameters1.y;

            return normalize(
                float3(
                    wave.x,
                    1.0f,
                    wave.y));
        }

        float3 TraceWaterReflection(
            float3 worldPosition,
            float3 normal,
            float2 fallbackUV)
        {
            float3 incident =
                normalize(
                    worldPosition -
                    cameraPosition.xyz);

            float3 direction =
                normalize(
                    reflect(
                        incident,
                        normal));

            float3 position =
                worldPosition +
                normal *
                    0.05f;

            float stepLength =
                1.0f;

            float2 lastUV =
                fallbackUV;

            [loop]
            for (int index = 0;
                 index < 20;
                 ++index)
            {
                position +=
                    direction *
                    stepLength;

                float projectedDepth =
                    0.0f;

                float2 uv =
                    ProjectWorldPosition(
                        position,
                        projectedDepth);

                if (uv.x <= 0.001f ||
                    uv.x >= 0.999f ||
                    uv.y <= 0.001f ||
                    uv.y >= 0.999f ||
                    projectedDepth <= 0.0f ||
                    projectedDepth >= 1.0f)
                {
                    break;
                }

                lastUV =
                    uv;

                float sceneDepth =
                    sceneDepthTexture.SampleLevel(
                        terrainBlendSampler,
                        uv,
                        0.0f).r;

                if (sceneDepth <
                    0.99999f)
                {
                    float3 scenePosition =
                        ReconstructWorldPosition(
                            uv,
                            sceneDepth);

                    float rayDistance =
                        distance(
                            cameraPosition.xyz,
                            position);

                    float sceneDistance =
                        distance(
                            cameraPosition.xyz,
                            scenePosition);

                    float difference =
                        rayDistance -
                        sceneDistance;

                    if (difference >= 0.0f &&
                        difference <
                            stepLength *
                            2.0f)
                    {
                        return
                            sceneColourTexture.SampleLevel(
                                terrainBlendSampler,
                                uv,
                                0.0f).rgb *
                            waterReflectionTint.rgb;
                    }
                }

                stepLength =
                    min(
                        stepLength *
                            1.18f,
                        8.0f);
            }

            return
                sceneColourTexture.SampleLevel(
                    terrainBlendSampler,
                    lastUV,
                    0.0f).rgb *
                waterReflectionTint.rgb;
            }
    )"
    R"(

        float4 ShadeWater(
            PixelInput input)
        {
            float reflectionStrength =
                waterParameters0.x;

            float refractionStrength =
                waterParameters0.y;

            float fresnelConstant =
                waterParameters0.z;

            float fresnelExponent =
                max(
                    waterParameters0.w,
                    0.001f);

            float3 waterNormal =
                CalculateWaterNormal(
                    input);

            float3 viewDirection =
                normalize(
                    cameraPosition.xyz -
                    input.worldPosition);

            float viewDot =
                saturate(
                    dot(
                        waterNormal,
                        viewDirection));

            float fresnel =
                fresnelConstant +
                (
                    1.0f -
                    fresnelConstant
                ) *
                pow(
                    1.0f -
                        viewDot,
                    fresnelExponent);

            fresnel =
                saturate(
                    fresnel);

            float2 screenUV =
                GetScreenUV(
                    input);

            float2 distortion =
                waterNormal.xz *
                0.0125f *
                refractionStrength;

            float2 refractionUV =
                clamp(
                    screenUV +
                        distortion,
                    float2(
                        0.001f,
                        0.001f),
                    float2(
                        0.999f,
                        0.999f));

            float3 refractionColour =
                sceneColourTexture.SampleLevel(
                    terrainBlendSampler,
                    refractionUV,
                    0.0f).rgb;

            refractionColour *=
                waterRefractionTint.rgb;

            float sceneDepth =
                sceneDepthTexture.SampleLevel(
                    terrainBlendSampler,
                    refractionUV,
                    0.0f).r;

            float verticalDepth =
                waterParameters3.x;

            if (sceneDepth <
                0.99999f)
            {
                float3 scenePosition =
                    ReconstructWorldPosition(
                        refractionUV,
                        sceneDepth);

                verticalDepth =
                    max(
                        0.0f,
                        input.worldPosition.y -
                        scenePosition.y);
            }

            float configuredDepth =
                max(
                    waterParameters3.x,
                    0.01f);

            float fadeDistance =
                waterParameters3.y >
                    0.0f
                    ? waterParameters3.y
                    : configuredDepth;

            float deepFactor =
                saturate(
                    verticalDepth /
                    max(
                        fadeDistance,
                        0.01f));

            refractionColour =
                lerp(
                    refractionColour,
                    waterDeepColour.rgb,
                    deepFactor);

            float3 reflectionColour =
                TraceWaterReflection(
                    input.worldPosition,
                    waterNormal,
                    screenUV);

            float reflectionWeight =
                saturate(
                    fresnel *
                    reflectionStrength);

            float refractionWeight =
                saturate(
                    (
                        1.0f -
                        fresnel
                    ) *
                    refractionStrength);

            float totalWeight =
                max(
                    reflectionWeight +
                        refractionWeight,
                    0.0001f);

            float3 colour =
                (
                    reflectionColour *
                        reflectionWeight +
                    refractionColour *
                        refractionWeight
                ) /
                totalWeight;

            float foamIntersectionDistance =
                max(
                    waterParameters2.y *
                        0.00016f,
                    0.025f);

            float foamSceneDepth =
                sceneDepthTexture.SampleLevel(
                    terrainBlendSampler,
                    screenUV,
                    0.0f).r;

            float foamDepth =
                foamIntersectionDistance;

            if (foamSceneDepth <
                0.99999f)
            {
                float3 foamScenePosition =
                    ReconstructWorldPosition(
                        screenUV,
                        foamSceneDepth);

                foamDepth =
                    max(
                        0.0f,
                        input.worldPosition.y -
                        foamScenePosition.y);
            }

            float foamAmount =
                1.0f -
                saturate(
                    foamDepth /
                    foamIntersectionDistance);

            foamAmount =
                smoothstep(
                    0.55f,
                    1.0f,
                    foamAmount);

            foamAmount =
                foamAmount *
                foamAmount;

            float foamTextureScale =
                waterParameters2.w;

            if (foamTextureScale <=
                0.0f)
            {
                foamTextureScale =
                    max(
                        waterParameters2.x,
                        0.001f);
            }

            float2 foamUV =
                input.localPosition.xz /
                    foamTextureScale +
                waterScrollSpeed1.xy *
                    waterParameters1.z *
                    waterParameters1.w *
                    0.18f;

            float foamSample =
                waterFoamTexture.Sample(
                    terrainTextureSampler,
                    foamUV).r;

            foamSample =
                smoothstep(
                    0.62f,
                    0.90f,
                    foamSample);

            foamAmount *=
                foamSample;

            foamAmount *=
                saturate(
                    waterParameters2.z);

            foamAmount *=
                0.38f;

            float3 foamColour =
                lerp(
                    waterDeepColour.rgb,
                    float3(
                        0.68f,
                        0.72f,
                        0.70f),
                    0.72f);

            colour =
                lerp(
                    colour,
                    foamColour,
                    saturate(
                        foamAmount));

            float3 lightDirection =
                normalize(
                    float3(
                        -0.35f,
                        0.85f,
                        -0.40f));

            float3 halfDirection =
                normalize(
                    lightDirection +
                    viewDirection);

            float specular =
                pow(
                    saturate(
                        dot(
                            waterNormal,
                            halfDirection)),
                    max(
                        waterParameters3.w,
                        1.0f));

            specular *=
                cameraPosition.w;

            colour +=
                specular;

            return float4(
                colour,
                1.0f);
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

            float outputAlpha =
                1.0f;

            if (useWater != 0)
            {
                return
                    ShadeWater(
                        input);
            }

            else if (useTerrain != 0)
            {
                baseColour =
                    SampleTerrain(
                        input);
            }

            else if (useModelTexture != 0)
            {
                float4 modelSample =
                    modelTexture.Sample(
                        terrainTextureSampler,
                        input.terrainUV);

                const float alphaMode =
                    modelParameters.y;

                if (alphaMode > 0.5f &&
                    alphaMode < 1.5f)
                {
                    clip(
                        modelSample.a -
                        modelParameters.x);
                }

                if (alphaMode > 1.5f)
                {
                    outputAlpha =
                        modelSample.a;
                }

                baseColour =
                    modelSample.rgb;
            }

            return float4(
                baseColour *
                    lighting,
                outputAlpha);
        }
    )";

    bool CreateRgbaTexture(
        ID3D11Device* device,
        ID3D11DeviceContext* context,
        const core::images::RgbaImage& image,
        const bool generateMipmaps,
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

        D3D11_TEXTURE2D_DESC
            description{};

        description.Width =
            image.width;

        description.Height =
            image.height;

        description.MipLevels =
            generateMipmaps
                ? 0
                : 1;

        description.ArraySize =
            1;

        description.Format =
            DXGI_FORMAT_R8G8B8A8_UNORM;

        description.SampleDesc.Count =
            1;

        description.SampleDesc.Quality =
            0;

        description.Usage =
            D3D11_USAGE_DEFAULT;

        description.BindFlags =
            D3D11_BIND_SHADER_RESOURCE;

        if (generateMipmaps)
        {
            description.BindFlags |=
                D3D11_BIND_RENDER_TARGET;

            description.MiscFlags =
                D3D11_RESOURCE_MISC_GENERATE_MIPS;
        }

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
            generateMipmaps
                ? static_cast<UINT>(-1)
                : 1;

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

        if (generateMipmaps)
        {
            context->GenerateMips(
                output.Get());
        }

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
            std::int32_t waterMaterialIndex = -1;

            std::vector<
                core::assets::MeshPrimitiveGroup>
                primitiveGroups;

            std::vector<SceneModelMaterial>
                modelMaterials;

            DirectX::XMFLOAT3 minimum{};
            DirectX::XMFLOAT3 maximum{};
        };

        ComPtr<ID3D11Device> device;
        ComPtr<ID3D11DeviceContext> context;
        ComPtr<IDXGISwapChain> swapChain;

        ComPtr<ID3D11RenderTargetView>
            renderTargetView;

        ComPtr<ID3D11Texture2D>
            backBufferTexture;

        ComPtr<ID3D11Texture2D>
            sceneColourTexture;

        ComPtr<ID3D11RenderTargetView>
            sceneColourRenderTargetView;

        ComPtr<ID3D11ShaderResourceView>
            sceneColourShaderResourceView;

        ComPtr<ID3D11Texture2D>
            depthTexture;

        ComPtr<ID3D11DepthStencilView>
            depthStencilView;

        ComPtr<ID3D11DepthStencilView>
            depthReadOnlyView;

        ComPtr<ID3D11ShaderResourceView>
            depthShaderResourceView;

        ComPtr<ID3D11DepthStencilState>
            depthState;

        ComPtr<ID3D11DepthStencilState>
            depthReadState;

        ComPtr<ID3D11BlendState>
            additiveBlendState;

        ComPtr<ID3D11BlendState>
            alphaBlendState;

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

        std::vector<SceneWaterMaterial>
            waterMaterials;

        std::vector<SceneInstance>
            instances;

        std::vector<SceneLodInstance>
            lodInstances;

        std::vector<SceneInstance>
            renderInstances;

        std::uint32_t width = 0;
        std::uint32_t height = 0;

        DirectX::XMFLOAT3 sceneCenter{};

        std::chrono::steady_clock::time_point
            startTime = std::chrono::steady_clock::now();

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

        result =
            state_->swapChain->GetBuffer(
                0,
                IID_PPV_ARGS(
                    &state_->backBufferTexture));

        if (FAILED(result))
        {
            error =
                "Unable to get back buffer.";

            return false;
        }

        result =
            state_->device->CreateRenderTargetView(
                state_->backBufferTexture.Get(),
                nullptr,
                &state_->renderTargetView);

        if (FAILED(result))
        {
            error =
                "Unable to create render target.";

            return false;
        }

        D3D11_TEXTURE2D_DESC
            sceneColourDescription{};

        state_->backBufferTexture->GetDesc(
            &sceneColourDescription);

        sceneColourDescription.BindFlags =
            D3D11_BIND_RENDER_TARGET |
            D3D11_BIND_SHADER_RESOURCE;

        sceneColourDescription.CPUAccessFlags =
            0;

        sceneColourDescription.MiscFlags =
            0;

        sceneColourDescription.Usage =
            D3D11_USAGE_DEFAULT;

        result =
            state_->device->CreateTexture2D(
                &sceneColourDescription,
                nullptr,
                &state_->sceneColourTexture);

        if (FAILED(result))
        {
            error =
                "Unable to create scene colour texture.";

            return false;
        }

        result =
            state_->device->CreateRenderTargetView(
                state_->sceneColourTexture.Get(),
                nullptr,
                &state_->sceneColourRenderTargetView);

        if (FAILED(result))
        {
            error =
                "Unable to create scene colour render target.";

            return false;
        }

        result =
            state_->device->CreateShaderResourceView(
                state_->sceneColourTexture.Get(),
                nullptr,
                &state_->sceneColourShaderResourceView);

        if (FAILED(result))
        {
            error =
                "Unable to create scene colour shader resource.";

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
            DXGI_FORMAT_R24G8_TYPELESS;

        depthDescription.SampleDesc.Count =
            1;

        depthDescription.Usage =
            D3D11_USAGE_DEFAULT;

        depthDescription.BindFlags =
            D3D11_BIND_DEPTH_STENCIL |
            D3D11_BIND_SHADER_RESOURCE;

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

        D3D11_DEPTH_STENCIL_VIEW_DESC
            depthViewDescription{};

        depthViewDescription.Format =
            DXGI_FORMAT_D24_UNORM_S8_UINT;

        depthViewDescription.ViewDimension =
            D3D11_DSV_DIMENSION_TEXTURE2D;

        depthViewDescription.Texture2D.MipSlice =
            0;

        result =
            state_->device->CreateDepthStencilView(
                state_->depthTexture.Get(),
                &depthViewDescription,
                &state_->depthStencilView);

        if (FAILED(result))
        {
            error =
                "Unable to create depth view.";

            return false;
        }

        D3D11_DEPTH_STENCIL_VIEW_DESC
            depthReadOnlyViewDescription =
                depthViewDescription;

        depthReadOnlyViewDescription.Flags =
            D3D11_DSV_READ_ONLY_DEPTH |
            D3D11_DSV_READ_ONLY_STENCIL;

        result =
            state_->device->CreateDepthStencilView(
                state_->depthTexture.Get(),
                &depthReadOnlyViewDescription,
                &state_->depthReadOnlyView);

        if (FAILED(result))
        {
            error =
                "Unable to create read-only depth view.";

            return false;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC
            depthResourceDescription{};

        depthResourceDescription.Format =
            DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

        depthResourceDescription.ViewDimension =
            D3D11_SRV_DIMENSION_TEXTURE2D;

        depthResourceDescription.Texture2D.MostDetailedMip =
            0;

        depthResourceDescription.Texture2D.MipLevels =
            1;

        result =
            state_->device->CreateShaderResourceView(
                state_->depthTexture.Get(),
                &depthResourceDescription,
                &state_->depthShaderResourceView);

        if (FAILED(result))
        {
            error =
                "Unable to create depth shader resource.";

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

        if (FAILED(result))
        {
            error =
                "Unable to create rasterizer.";

            return false;
        }

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

        D3D11_BLEND_DESC
            alphaBlendDescription{};

        alphaBlendDescription
            .RenderTarget[0]
            .BlendEnable =
                TRUE;

        alphaBlendDescription
            .RenderTarget[0]
            .SrcBlend =
                D3D11_BLEND_SRC_ALPHA;

        alphaBlendDescription
            .RenderTarget[0]
            .DestBlend =
                D3D11_BLEND_INV_SRC_ALPHA;

        alphaBlendDescription
            .RenderTarget[0]
            .BlendOp =
                D3D11_BLEND_OP_ADD;

        alphaBlendDescription
            .RenderTarget[0]
            .SrcBlendAlpha =
                D3D11_BLEND_ONE;

        alphaBlendDescription
            .RenderTarget[0]
            .DestBlendAlpha =
                D3D11_BLEND_INV_SRC_ALPHA;

        alphaBlendDescription
            .RenderTarget[0]
            .BlendOpAlpha =
                D3D11_BLEND_OP_ADD;

        alphaBlendDescription
            .RenderTarget[0]
            .RenderTargetWriteMask =
                D3D11_COLOR_WRITE_ENABLE_ALL;

        result =
            state_->device->CreateBlendState(
                &alphaBlendDescription,
                &state_->alphaBlendState);

        if (FAILED(result))
        {
            error =
                "Unable to create model alpha blend state.";

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
            D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;

        blendSamplerDescription.AddressU =
            D3D11_TEXTURE_ADDRESS_CLAMP;

        blendSamplerDescription.AddressV =
            D3D11_TEXTURE_ADDRESS_CLAMP;

        blendSamplerDescription.AddressW =
            D3D11_TEXTURE_ADDRESS_CLAMP;

        blendSamplerDescription.MaxAnisotropy =
            1;

        blendSamplerDescription.MipLODBias =
            0.0f;

        blendSamplerDescription.MinLOD =
            0.0f;

        blendSamplerDescription.MaxLOD =
            0.0f;

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

            gpuMesh.modelMaterials =
                sceneMesh.modelMaterials;

            gpuMesh.minimum =
                minimum;

            gpuMesh.maximum =
                maximum;

            gpuMesh.terrainMaterialIndex =
                sceneMesh.terrainMaterialIndex;

            gpuMesh.waterMaterialIndex =
                sceneMesh.waterMaterialIndex;

            if (gpuMesh.waterMaterialIndex >= 0)
            {
                const std::size_t waterMaterialIndex =
                    static_cast<std::size_t>(
                        gpuMesh.waterMaterialIndex);

                if (waterMaterialIndex >=
                    scene.waterMaterials.size())
                {
                    error =
                        "Scene mesh references invalid water material.";

                    return false;
                }
            }

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
                texture.generateMipmaps,
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
                false,
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

        state_->waterMaterials =
            scene.waterMaterials;

        state_->lodInstances =
            scene.lodInstances;

        state_->renderInstances.clear();

        state_->renderInstances.reserve(
            state_->instances.size() +
            state_->lodInstances.size() *
                3u);

        bool hasBounds = false;

        DirectX::XMFLOAT3 sceneMinimum{};
        DirectX::XMFLOAT3 sceneMaximum{};

        const auto includeMeshBounds =
            [&](
                const std::size_t meshIndex,
                const core::math::Transform3x4& transform)
            {
                if (meshIndex >=
                    state_->meshes.size())
                {
                    return false;
                }

                const State::GpuMesh& mesh =
                    state_->meshes[
                        meshIndex];

                const DirectX::XMMATRIX world =
                    ToMatrix(
                        transform);

                const std::array<
                    DirectX::XMFLOAT3,
                    8>
                    corners
                {{
                    {mesh.minimum.x, mesh.minimum.y, mesh.minimum.z},
                    {mesh.maximum.x, mesh.minimum.y, mesh.minimum.z},
                    {mesh.minimum.x, mesh.maximum.y, mesh.minimum.z},
                    {mesh.maximum.x, mesh.maximum.y, mesh.minimum.z},
                    {mesh.minimum.x, mesh.minimum.y, mesh.maximum.z},
                    {mesh.maximum.x, mesh.minimum.y, mesh.maximum.z},
                    {mesh.minimum.x, mesh.maximum.y, mesh.maximum.z},
                    {mesh.maximum.x, mesh.maximum.y, mesh.maximum.z}
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

                        hasBounds =
                            true;

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

                return true;
            };

        for (const SceneLodInstance& lodInstance :
             state_->lodInstances)
        {
            if (lodInstance.levelCount ==
                0)
            {
                continue;
            }

            const float objectX =
                lodInstance.transform.values[9];

            const float objectY =
                lodInstance.transform.values[10];

            const float objectZ =
                lodInstance.transform.values[11];

            const float deltaX =
                objectX -
                state_->camera.position.x;

            const float deltaY =
                objectY -
                state_->camera.position.y;

            const float deltaZ =
                objectZ -
                state_->camera.position.z;

            const float distance =
                std::sqrt(
                    deltaX * deltaX +
                    deltaY * deltaY +
                    deltaZ * deltaZ);

            const float scaleX =
                std::sqrt(
                    lodInstance.transform.values[0] *
                        lodInstance.transform.values[0] +
                    lodInstance.transform.values[1] *
                        lodInstance.transform.values[1] +
                    lodInstance.transform.values[2] *
                        lodInstance.transform.values[2]);

            const float scaleY =
                std::sqrt(
                    lodInstance.transform.values[3] *
                        lodInstance.transform.values[3] +
                    lodInstance.transform.values[4] *
                        lodInstance.transform.values[4] +
                    lodInstance.transform.values[5] *
                        lodInstance.transform.values[5]);

            const float scaleZ =
                std::sqrt(
                    lodInstance.transform.values[6] *
                        lodInstance.transform.values[6] +
                    lodInstance.transform.values[7] *
                        lodInstance.transform.values[7] +
                    lodInstance.transform.values[8] *
                        lodInstance.transform.values[8]);

            const float instanceScale =
                std::max(
                    {
                        scaleX,
                        scaleY,
                        scaleZ,
                        0.001f
                    });

            const float localDistance =
                distance /
                instanceScale;

            std::uint32_t selectedLevel =
                lodInstance.levelCount -
                1;

            for (std::uint32_t levelIndex = 0;
                 levelIndex <
                    lodInstance.levelCount;
                 ++levelIndex)
            {
                if (localDistance <=
                    lodInstance.levels[
                        levelIndex]
                        .maximumDistance)
                {
                    selectedLevel =
                        levelIndex;

                    break;
                }
            }

            const SceneLodLevel&
                level =
                    lodInstance.levels[
                        selectedLevel];

            for (const std::size_t meshIndex :
                 level.meshIndices)
            {
                SceneInstance
                    renderInstance;

                renderInstance.meshIndex =
                    meshIndex;

                renderInstance.transform =
                    lodInstance.transform;

                state_->renderInstances.push_back(
                    std::move(
                        renderInstance));
            }
        }

        std::stable_sort(
            state_->renderInstances.begin(),
            state_->renderInstances.end(),
            [this](
                const SceneInstance& left,
                const SceneInstance& right)
            {
                const bool leftIsWater =
                    left.meshIndex <
                        state_->meshes.size() &&
                    state_->meshes[
                        left.meshIndex]
                        .waterMaterialIndex >= 0;

                const bool rightIsWater =
                    right.meshIndex <
                        state_->meshes.size() &&
                    state_->meshes[
                        right.meshIndex]
                        .waterMaterialIndex >= 0;

                return
                    !leftIsWater &&
                    rightIsWater;
            });

        for (const SceneInstance& instance :
            state_->renderInstances)
        {
            if (!includeMeshBounds(
                    instance.meshIndex,
                    instance.transform))
            {
                error =
                    "Scene contains invalid mesh index.";

                return false;
            }
        }

        for (const SceneLodInstance& instance :
             state_->lodInstances)
        {
            if (instance.levelCount ==
                    0 ||
                instance.levelCount >
                    instance.levels.size())
            {
                error =
                    "Scene contains invalid LOD instance.";

                return false;
            }

            const SceneLodLevel&
                highestDetail =
                    instance.levels[0];

            if (highestDetail.meshIndices.empty())
            {
                error =
                    "Scene LOD0 contains no meshes.";

                return false;
            }

            for (const std::size_t meshIndex :
                 highestDetail.meshIndices)
            {
                if (!includeMeshBounds(
                        meshIndex,
                        instance.transform))
                {
                    error =
                        "Scene LOD contains invalid mesh index.";

                    return false;
                }
            }

            for (std::uint32_t levelIndex = 0;
                 levelIndex <
                    instance.levelCount;
                 ++levelIndex)
            {
                for (const std::size_t meshIndex :
                     instance.levels[
                         levelIndex]
                         .meshIndices)
                {
                    if (meshIndex >=
                        state_->meshes.size())
                    {
                        error =
                            "Scene LOD references invalid mesh.";

                        return false;
                    }
                }
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

        state_->context->OMSetRenderTargets(
            0,
            nullptr,
            nullptr);

        state_->context->ClearRenderTargetView(
            state_->sceneColourRenderTargetView.Get(),
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
            state_->sceneColourRenderTargetView.Get()
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

        constexpr float NearPlane =
            0.1f;

        const float farPlane =
            std::max(
                5000.0f,
                state_->sceneRadius *
                    10.0f);

        const XMMATRIX projection =
            XMMatrixPerspectiveFovLH(
                XMConvertToRadians(
                    std::clamp(
                        state_->camera.fieldOfViewDegrees,
                        20.0f,
                        90.0f)),
                aspect,
                NearPlane,
                farPlane);

        const XMMATRIX viewProjection =
            view *
            projection;

        const XMMATRIX inverseViewProjection =
            XMMatrixInverse(
                nullptr,
                viewProjection);

        const float elapsedSeconds =
            std::chrono::duration<float>(
                std::chrono::steady_clock::now() -
                state_->startTime).count();

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

        XMStoreFloat4x4(
            &constants.inverseViewProjection,
            inverseViewProjection);

        constants.cameraPosition =
        {
            state_->camera.position.x,
            state_->camera.position.y,
            state_->camera.position.z,
            0.0f
        };

        constants.screenParameters =
        {
            static_cast<float>(
                state_->width),

            static_cast<float>(
                state_->height),

            NearPlane,
            farPlane
        };

        state_->renderInstances.clear();

        for (const SceneInstance& instance :
             state_->instances)
        {
            if (instance.maximumDistance >
                0.0f)
            {
                const core::math::Vector3 position =
                    instance.transform.Translation();

                const float deltaX =
                    position.x -
                    state_->camera.position.x;

                const float deltaY =
                    position.y -
                    state_->camera.position.y;

                const float deltaZ =
                    position.z -
                    state_->camera.position.z;

                const float distanceSquared =
                    deltaX * deltaX +
                    deltaY * deltaY +
                    deltaZ * deltaZ;

                const float maximumDistanceSquared =
                    instance.maximumDistance *
                    instance.maximumDistance;

                if (distanceSquared >
                    maximumDistanceSquared)
                {
                    continue;
                }
            }

            state_->renderInstances.push_back(
                instance);
        }

        for (const SceneLodInstance& lodInstance :
             state_->lodInstances)
        {
            if (lodInstance.levelCount ==
                0)
            {
                continue;
            }

            const core::math::Vector3 position =
                lodInstance.transform.Translation();

            const float deltaX =
                position.x -
                state_->camera.position.x;

            const float deltaY =
                position.y -
                state_->camera.position.y;

            const float deltaZ =
                position.z -
                state_->camera.position.z;

            const float distance =
                std::sqrt(
                    deltaX * deltaX +
                    deltaY * deltaY +
                    deltaZ * deltaZ);

            const float scaleX =
                std::sqrt(
                    lodInstance.transform.values[0] *
                        lodInstance.transform.values[0] +
                    lodInstance.transform.values[1] *
                        lodInstance.transform.values[1] +
                    lodInstance.transform.values[2] *
                        lodInstance.transform.values[2]);

            const float scaleY =
                std::sqrt(
                    lodInstance.transform.values[3] *
                        lodInstance.transform.values[3] +
                    lodInstance.transform.values[4] *
                        lodInstance.transform.values[4] +
                    lodInstance.transform.values[5] *
                        lodInstance.transform.values[5]);

            const float scaleZ =
                std::sqrt(
                    lodInstance.transform.values[6] *
                        lodInstance.transform.values[6] +
                    lodInstance.transform.values[7] *
                        lodInstance.transform.values[7] +
                    lodInstance.transform.values[8] *
                        lodInstance.transform.values[8]);

            const float instanceScale =
                std::max(
                    {
                        scaleX,
                        scaleY,
                        scaleZ,
                        0.001f
                    });

            const float localDistance =
                distance /
                instanceScale;

            std::uint32_t selectedLevel =
                lodInstance.levelCount -
                1;

            for (std::uint32_t levelIndex = 0;
                 levelIndex <
                    lodInstance.levelCount;
                 ++levelIndex)
            {
                const SceneLodLevel& level =
                    lodInstance.levels[
                        levelIndex];

                if (localDistance <=
                    level.maximumDistance)
                {
                    selectedLevel =
                        levelIndex;

                    break;
                }
            }

            const SceneLodLevel& selected =
                lodInstance.levels[
                    selectedLevel];

            for (const std::size_t meshIndex :
                 selected.meshIndices)
            {
                if (meshIndex >=
                    state_->meshes.size())
                {
                    error =
                        "Selected LOD references invalid mesh.";

                    return false;
                }

                SceneInstance instance;

                instance.meshIndex =
                    meshIndex;

                instance.transform =
                    lodInstance.transform;

                state_->renderInstances.push_back(
                    std::move(
                        instance));
            }
        }

        for (const SceneInstance& instance :
            state_->renderInstances)
        {
            const State::GpuMesh& mesh =
                state_->meshes[
                    instance.meshIndex];

            if (mesh.waterMaterialIndex >= 0)
            {
                continue;
            }

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

                    constants.useModelTexture =
                        0;

                    constants.useWater =
                        0;

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

            constants.useModelTexture =
                0;

            constants.useWater =
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

                constants.useModelTexture =
                    0;

                constants.modelParameters =
                {
                    0.5f,
                    0.0f,
                    0.0f,
                    0.0f
                };

                ID3D11ShaderResourceView*
                    modelTextureView =
                        nullptr;

                SceneAlphaMode alphaMode =
                    SceneAlphaMode::Opaque;

                if (groupIndex <
                    mesh.modelMaterials.size())
                {
                    const SceneModelMaterial& material =
                        mesh.modelMaterials[
                            groupIndex];

                    alphaMode =
                        material.alphaMode;

                    constants.modelParameters.x =
                        material.alphaCutoff;

                    constants.modelParameters.y =
                        static_cast<float>(
                            static_cast<std::uint8_t>(
                                material.alphaMode));

                    if (material.diffuseTextureIndex >= 0)
                    {
                        const std::size_t textureIndex =
                            static_cast<std::size_t>(
                                material.diffuseTextureIndex);

                        if (textureIndex >=
                            state_->textures.size())
                        {
                            error =
                                "Model material references invalid texture.";

                            return false;
                        }

                        modelTextureView =
                            state_->textures[
                                textureIndex].Get();

                        constants.useModelTexture =
                            1;
                    }
                }

                state_->context->PSSetShaderResources(
                    5,
                    1,
                    &modelTextureView);

                constexpr float BlendFactor[4]
                {
                    0.0f,
                    0.0f,
                    0.0f,
                    0.0f
                };

                if (alphaMode ==
                    SceneAlphaMode::Blend)
                {
                    state_->context->OMSetBlendState(
                        state_->alphaBlendState.Get(),
                        BlendFactor,
                        0xFFFFFFFFu);

                    state_->context->OMSetDepthStencilState(
                        state_->depthReadState.Get(),
                        0);
                }
                else
                {
                    state_->context->OMSetBlendState(
                        nullptr,
                        BlendFactor,
                        0xFFFFFFFFu);

                    state_->context->OMSetDepthStencilState(
                        state_->depthState.Get(),
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
                    group.primitiveCount *
                        3u,
                    group.startIndex,
                    0);
            }

            ID3D11ShaderResourceView*
                emptyModelTexture =
                    nullptr;

            state_->context->PSSetShaderResources(
                5,
                1,
                &emptyModelTexture);

            state_->context->OMSetBlendState(
                nullptr,
                nullptr,
                0xFFFFFFFFu);

            state_->context->OMSetDepthStencilState(
                state_->depthState.Get(),
                0);
        }

        // The opaque scene must be unbound before it can be used as an SRV.
        state_->context->OMSetRenderTargets(
            0,
            nullptr,
            nullptr);

        state_->context->CopyResource(
            state_->backBufferTexture.Get(),
            state_->sceneColourTexture.Get());

        ID3D11RenderTargetView*
            finalRenderTargets[] =
        {
            state_->renderTargetView.Get()
        };

        state_->context->OMSetRenderTargets(
            1,
            finalRenderTargets,
            state_->depthReadOnlyView.Get());

        state_->context->OMSetDepthStencilState(
            state_->depthReadState.Get(),
            0);

        for (const SceneInstance& instance :
             state_->renderInstances)
        {
            if (instance.meshIndex >=
                state_->meshes.size())
            {
                error =
                    "Water pass contains invalid mesh index.";

                return false;
            }

            const State::GpuMesh& mesh =
                state_->meshes[
                    instance.meshIndex];

            if (mesh.waterMaterialIndex <
                0)
            {
                continue;
            }

            const std::size_t materialIndex =
                static_cast<std::size_t>(
                    mesh.waterMaterialIndex);

            if (materialIndex >=
                state_->waterMaterials.size())
            {
                error =
                    "Water mesh references invalid material.";

                return false;
            }

            const SceneWaterMaterial& material =
                state_->waterMaterials[
                    materialIndex];

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

            constants.useTerrain =
                0;

            constants.terrainLayerCount =
                0;

            constants.useModelTexture =
                0;

            constants.useWater =
                1;

            constants.waterDeepColour =
            {
                material.deepColour[0],
                material.deepColour[1],
                material.deepColour[2],
                material.deepColour[3]
            };

            constants.waterReflectionTint =
            {
                material.reflectionTint[0],
                material.reflectionTint[1],
                material.reflectionTint[2],
                material.reflectionTint[3]
            };

            constants.waterRefractionTint =
            {
                material.refractionTint[0],
                material.refractionTint[1],
                material.refractionTint[2],
                material.refractionTint[3]
            };

            constants.waterParameters0 =
            {
                material.reflectionStrength,
                material.refractionStrength,
                material.fresnelConstant,
                material.fresnelExponent
            };

            constants.waterParameters1 =
            {
                material.waveScale[0],
                material.waveScale[1],
                material.windVelocity,
                elapsedSeconds
            };

            constants.waterScrollSpeed1 =
            {
                material.scrollSpeed1[0],
                material.scrollSpeed1[1],
                0.0f,
                0.0f
            };

            constants.waterScrollSpeed2 =
            {
                material.scrollSpeed2[0],
                material.scrollSpeed2[1],
                0.0f,
                0.0f
            };

            constants.waterParameters2 =
            {
                material.textureTessellation,
                material.foamIntersection,
                material.foamMultiplier,
                material.foamTiling
            };

            constants.waterParameters3 =
            {
                material.depth,
                material.fadeDepth,
                material.smoothness,
                material.sunPower
            };

            constants.cameraPosition.w =
                material.sunScale;

            ID3D11ShaderResourceView*
                waveTextureView =
                    nullptr;

            ID3D11ShaderResourceView*
                foamTextureView =
                    nullptr;

            if (material.waveTextureIndex >=
                0)
            {
                const std::size_t textureIndex =
                    static_cast<std::size_t>(
                        material.waveTextureIndex);

                if (textureIndex >=
                    state_->textures.size())
                {
                    error =
                        "Water normal texture index is invalid.";

                    return false;
                }

                waveTextureView =
                    state_->textures[
                        textureIndex].Get();
            }

            if (material.foamTextureIndex >=
                0)
            {
                const std::size_t textureIndex =
                    static_cast<std::size_t>(
                        material.foamTextureIndex);

                if (textureIndex >=
                    state_->textures.size())
                {
                    error =
                        "Water foam texture index is invalid.";

                    return false;
                }

                foamTextureView =
                    state_->textures[
                        textureIndex].Get();
            }

            ID3D11ShaderResourceView*
                waterViews[4] =
            {
                state_->sceneColourShaderResourceView.Get(),
                state_->depthShaderResourceView.Get(),
                waveTextureView,
                foamTextureView
            };

            state_->context->PSSetShaderResources(
                6,
                4,
                waterViews);

            state_->context->OMSetBlendState(
                nullptr,
                nullptr,
                0xFFFFFFFFu);

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
            emptyWaterViews[4] =
        {
            nullptr,
            nullptr,
            nullptr,
            nullptr
        };

        state_->context->PSSetShaderResources(
            6,
            4,
            emptyWaterViews);

        constants.useWater =
            0;

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
