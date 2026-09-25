#include "Graphics/Renderer.h"

#include "Graphics/Shaders/ShaderCompiler.h"
#include "Graphics/ParticleRenderer.h"

#include "Core/Animation/ScalarAnimation.h"
#include "Core/World/Sky/SkyEvaluator.h"
#include "Core/World/Particles/ParticleRuntime.h"

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

    constexpr std::uint32_t MaxOmniLights =
        32;

    struct GpuOmniLight final
    {
        DirectX::XMFLOAT4 positionOuterRadius;
        DirectX::XMFLOAT4 colourMultiplier;
        DirectX::XMFLOAT4 parameters;
    };

    struct OmniLightConstants final
    {
        std::array<
            GpuOmniLight,
            MaxOmniLights>
            lights{};

        std::uint32_t lightCount =
            0;

        std::uint32_t padding0 =
            0;

        std::uint32_t padding1 =
            0;

        std::uint32_t padding2 =
            0;
    };

    static_assert(
        sizeof(OmniLightConstants) %
            16u ==
        0u);

    constexpr std::uint32_t MaxSpotLights =
        32;

    struct GpuSpotLight final
    {
        DirectX::XMFLOAT4 positionOuterRadius;
        DirectX::XMFLOAT4 directionCosConeAngle;
        DirectX::XMFLOAT4 colourMultiplier;
        DirectX::XMFLOAT4 parameters;
    };

    struct SpotLightConstants final
    {
        std::array<
            GpuSpotLight,
            MaxSpotLights>
            lights{};

        std::uint32_t lightCount =
            0;

        std::uint32_t padding0 =
            0;

        std::uint32_t padding1 =
            0;

        std::uint32_t padding2 =
            0;
    };

    static_assert(
        sizeof(SpotLightConstants) %
            16u ==
        0u);

    struct SkyConstants final
    {
        DirectX::XMFLOAT4
            sunDirectionDaylight;

        DirectX::XMFLOAT4
            sunColour;

        DirectX::XMFLOAT4
            ambientColour;

        DirectX::XMFLOAT4
            atmosphere0;

        DirectX::XMFLOAT4
            atmosphere1;
    };

    static_assert(
        sizeof(SkyConstants) %
            16u ==
        0u);

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

        DirectX::XMFLOAT4 modelTint;

        DirectX::XMFLOAT4 modelOverlayColour;

        DirectX::XMFLOAT4 modelOverlayParameters;

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

    struct FlareConstants final
    {
        DirectX::XMFLOAT4X4
            viewProjection;

        DirectX::XMFLOAT4
            sourcePositionSize;

        DirectX::XMFLOAT4
            colour;

        DirectX::XMFLOAT4
            parameters;

        DirectX::XMFLOAT4
            cameraPosition;

        DirectX::XMFLOAT4
            screenParameters;
    };

    static_assert(
        sizeof(FlareConstants) %
            16u ==
        0u);

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

    SkyConstants BuildSkyConstants(
        const client::graphics::SceneSky& sky,
        const float elapsedSeconds)
    {
        SkyConstants
            constants{};

        constants.sunDirectionDaylight =
        {
            -0.35f,
            0.85f,
            -0.40f,
            1.0f
        };

        constants.sunColour =
        {
            1.0f,
            1.0f,
            1.0f,
            1.0f
        };

        constants.ambientColour =
        {
            0.57f,
            0.57f,
            0.57f,
            1.0f
        };

        if (!sky.enabled)
        {
            return constants;
        }

        const core::world::sky::SkySample sample =
            core::world::sky::SkyEvaluator::Evaluate(
                sky.definition,
                elapsedSeconds);

        const auto NormalizeColour =
            [](
                const float value) noexcept
            {
                return
                    std::clamp(
                        value /
                            255.0f,
                        0.0f,
                        1.0f);
            };

        constants.sunDirectionDaylight =
        {
            sample.sunDirection.x,
            sample.sunDirection.y,
            sample.sunDirection.z,
            sample.daylight
        };

        constants.sunColour =
        {
            NormalizeColour(
                sample.lightColour.x),

            NormalizeColour(
                sample.lightColour.y),

            NormalizeColour(
                sample.lightColour.z),

            1.0f
        };

        constants.ambientColour =
        {
            NormalizeColour(
                sample.ambientColour.x),

            NormalizeColour(
                sample.ambientColour.y),

            NormalizeColour(
                sample.ambientColour.z),

            1.0f
        };

        constants.atmosphere0 =
        {
            sky.definition.mieAmount,
            sky.definition.turbidityOffset,
            sky.definition.turbidityFactor,
            sky.definition.vertexHeightEffect
        };

        constants.atmosphere1 =
        {
            sky.definition.sunHeightEffect,
            sky.definition.power,
            sample.timeHours,

            sky.gradientTextureIndex >=
                0
                ? 1.0f
                : 0.0f
        };

        return constants;
    }

    OmniLightConstants BuildOmniLightConstants(
        const std::vector<
            client::graphics::SceneOmniLight>& omniLights,
        const std::vector<
            client::graphics::ScenePulseLight>& pulseLights,
        const client::graphics::CameraView& camera,
        const float elapsedSeconds)
    {
        struct RankedLight final
        {
            GpuOmniLight gpu{};

            std::int32_t priority =
                0;

            float distanceSquared =
                0.0f;
        };

        std::vector<RankedLight>
            ranked;

        ranked.reserve(
            omniLights.size() +
            pulseLights.size());

        for (const client::graphics::SceneOmniLight& light :
             omniLights)
        {
            if (light.outerRadius <=
                    0.0f ||
                light.multiplier <=
                    0.0f)
            {
                continue;
            }

            const float deltaX =
                light.position[0] -
                camera.position.x;

            const float deltaY =
                light.position[1] -
                camera.position.y;

            const float deltaZ =
                light.position[2] -
                camera.position.z;

            RankedLight
                entry;

            entry.priority =
                light.priority;

            entry.distanceSquared =
                deltaX * deltaX +
                deltaY * deltaY +
                deltaZ * deltaZ;

            entry.gpu.positionOuterRadius =
            {
                light.position[0],
                light.position[1],
                light.position[2],
                light.outerRadius
            };

            entry.gpu.colourMultiplier =
            {
                light.colour[0],
                light.colour[1],
                light.colour[2],
                light.multiplier
            };

            entry.gpu.parameters =
            {
                light.innerRadius,

                light.specular
                    ? 1.0f
                    : 0.0f,

                light.isStatic
                    ? 1.0f
                    : 0.0f,

                light.isDynamic
                    ? 1.0f
                    : 0.0f
            };

            ranked.push_back(
                entry);
        }

        for (const client::graphics::ScenePulseLight& light :
             pulseLights)
        {
            if (light.outerRadius <=
                    0.0f ||
                light.multiplier <=
                    0.0f)
            {
                continue;
            }

            const float animationValue =
                core::animation::ScalarAnimationEvaluator::Evaluate(
                    light.animation,
                    elapsedSeconds);

            const float animatedMultiplier =
                light.multiplier *
                animationValue;

            if (animatedMultiplier <=
                0.000001f)
            {
                continue;
            }

            const float deltaX =
                light.position[0] -
                camera.position.x;

            const float deltaY =
                light.position[1] -
                camera.position.y;

            const float deltaZ =
                light.position[2] -
                camera.position.z;

            RankedLight
                entry;

            entry.priority =
                light.priority;

            entry.distanceSquared =
                deltaX * deltaX +
                deltaY * deltaY +
                deltaZ * deltaZ;

            entry.gpu.positionOuterRadius =
            {
                light.position[0],
                light.position[1],
                light.position[2],
                light.outerRadius
            };

            entry.gpu.colourMultiplier =
            {
                light.colour[0],
                light.colour[1],
                light.colour[2],
                animatedMultiplier
            };

            entry.gpu.parameters =
            {
                light.innerRadius,
                0.0f,
                0.0f,
                1.0f
            };

            ranked.push_back(
                entry);
        }

        std::stable_sort(
            ranked.begin(),
            ranked.end(),
            [](
                const RankedLight& left,
                const RankedLight& right)
            {
                if (left.priority !=
                    right.priority)
                {
                    return
                        left.priority >
                        right.priority;
                }

                return
                    left.distanceSquared <
                    right.distanceSquared;
            });

        OmniLightConstants
            constants{};

        constants.lightCount =
            static_cast<std::uint32_t>(
                std::min<std::size_t>(
                    ranked.size(),
                    MaxOmniLights));

        for (std::uint32_t index = 0;
             index <
                constants.lightCount;
             ++index)
        {
            constants.lights[index] =
                ranked[index].gpu;
        }

        return constants;
    }

    SpotLightConstants BuildSpotLightConstants(
        const std::vector<
            client::graphics::SceneSpotLight>& lights,
        const client::graphics::CameraView& camera)
    {
        struct RankedLight final
        {
            const client::graphics::SceneSpotLight*
                light = nullptr;

            float distanceSquared =
                0.0f;
        };

        std::vector<RankedLight>
            ranked;

        ranked.reserve(
            lights.size());

        for (const client::graphics::SceneSpotLight& light :
             lights)
        {
            if (light.outerRadius <=
                    0.0f ||
                light.multiplier <=
                    0.0f)
            {
                continue;
            }

            const float directionLengthSquared =
                light.direction[0] *
                    light.direction[0] +
                light.direction[1] *
                    light.direction[1] +
                light.direction[2] *
                    light.direction[2];

            if (directionLengthSquared <=
                0.000001f)
            {
                continue;
            }

            const float deltaX =
                light.position[0] -
                camera.position.x;

            const float deltaY =
                light.position[1] -
                camera.position.y;

            const float deltaZ =
                light.position[2] -
                camera.position.z;

            RankedLight entry;

            entry.light =
                &light;

            entry.distanceSquared =
                deltaX * deltaX +
                deltaY * deltaY +
                deltaZ * deltaZ;

            ranked.push_back(
                entry);
        }

        std::stable_sort(
            ranked.begin(),
            ranked.end(),
            [](
                const RankedLight& left,
                const RankedLight& right)
            {
                if (left.light->priority !=
                    right.light->priority)
                {
                    return
                        left.light->priority >
                        right.light->priority;
                }

                return
                    left.distanceSquared <
                    right.distanceSquared;
            });

        SpotLightConstants
            constants{};

        constants.lightCount =
            static_cast<std::uint32_t>(
                std::min<std::size_t>(
                    ranked.size(),
                    MaxSpotLights));

        for (std::uint32_t index = 0;
             index <
                constants.lightCount;
             ++index)
        {
            const client::graphics::SceneSpotLight&
                source =
                    *ranked[index].light;

            GpuSpotLight& target =
                constants.lights[index];

            const float directionLength =
                std::sqrt(
                    source.direction[0] *
                        source.direction[0] +
                    source.direction[1] *
                        source.direction[1] +
                    source.direction[2] *
                        source.direction[2]);

            const float inverseDirectionLength =
                1.0f /
                std::max(
                    directionLength,
                    0.000001f);

            target.positionOuterRadius =
            {
                source.position[0],
                source.position[1],
                source.position[2],
                source.outerRadius
            };

            target.directionCosConeAngle =
            {
                source.direction[0] *
                    inverseDirectionLength,

                source.direction[1] *
                    inverseDirectionLength,

                source.direction[2] *
                    inverseDirectionLength,

                source.cosConeAngle
            };

            target.colourMultiplier =
            {
                source.colour[0],
                source.colour[1],
                source.colour[2],
                source.multiplier
            };

            target.parameters =
            {
                source.innerRadius,

                source.specular
                    ? 1.0f
                    : 0.0f,

                source.isStatic
                    ? 1.0f
                    : 0.0f,

                source.isDynamic
                    ? 1.0f
                    : 0.0f
            };
        }

        return constants;
    }

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
            std::size_t vertexCount = 0;

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
        
        ComPtr<ID3D11ShaderResourceView>
            backgroundTextureView; // Static frontend background.

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

        ComPtr<ID3D11DepthStencilState>
            flareDepthState;

        ComPtr<ID3D11BlendState>
            additiveBlendState;

        ComPtr<ID3D11BlendState>
            alphaBlendState;

        ComPtr<ID3D11SamplerState>
            terrainTextureSampler;

        ComPtr<ID3D11SamplerState>
            terrainBlendSampler;

        ComPtr<ID3D11SamplerState>
            backgroundSampler;

        ComPtr<ID3D11RasterizerState>
            rasterizerState;

        ComPtr<ID3D11VertexShader>
            vertexShader;

        ComPtr<ID3D11PixelShader>
            pixelShader;

        ComPtr<ID3D11VertexShader>
            backgroundVertexShader;

        ComPtr<ID3D11PixelShader>
            backgroundPixelShader;

        ComPtr<ID3D11VertexShader>
            skyVertexShader;

        ComPtr<ID3D11PixelShader>
            skyPixelShader;

        ComPtr<ID3D11VertexShader>
            flareVertexShader;

        ComPtr<ID3D11PixelShader>
            flarePixelShader;

        ComPtr<ID3D11InputLayout>
            inputLayout;

        ComPtr<ID3D11Buffer>
            constantBuffer;

        ComPtr<ID3D11Buffer>
            omniLightConstantBuffer;

        ComPtr<ID3D11Buffer>
            spotLightConstantBuffer;

        ComPtr<ID3D11Buffer>
            skyConstantBuffer;

        ComPtr<ID3D11Buffer>
            flareConstantBuffer;

        std::vector<GpuMesh>
            meshes;

        std::vector<
            ComPtr<ID3D11ShaderResourceView>>
            textures;

        std::vector<GpuTerrainMaterial>
            terrainMaterials;

        std::vector<SceneWaterMaterial>
            waterMaterials;

        std::vector<SceneOmniLight>
            omniLights;

        std::vector<SceneSpotLight>
            spotLights;

        std::vector<ScenePulseLight>
            pulseLights;

        std::vector<SceneFlare>
            flares;

        std::vector<
            core::world::particles::ParticleRuntimeSystem>
            particleSystems;

        std::vector<SceneParticleEmitter>
            particleEmitters;

        ParticleRenderer
            particleRenderer;

        bool particleGpuReported =
            false;

        std::chrono::steady_clock::time_point
            particleUpdateTime =
                std::chrono::steady_clock::now();

        bool particleRuntimeReported =
            false;

        SceneSky
            sky;

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
            flareDepthDescription{};

        flareDepthDescription.DepthEnable =
            FALSE;

        flareDepthDescription.DepthWriteMask =
            D3D11_DEPTH_WRITE_MASK_ZERO;

        flareDepthDescription.DepthFunc =
            D3D11_COMPARISON_ALWAYS;

        flareDepthDescription.StencilEnable =
            FALSE;

        result =
            state_->device->CreateDepthStencilState(
                &flareDepthDescription,
                &state_->flareDepthState);

        if (FAILED(result))
        {
            error =
                "Unable to create flare depth state.";

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

        D3D11_SAMPLER_DESC backgroundSamplerDescription =
            blendSamplerDescription;

        backgroundSamplerDescription.Filter =
            D3D11_FILTER_MIN_MAG_MIP_LINEAR;

        backgroundSamplerDescription.AddressU =
            D3D11_TEXTURE_ADDRESS_CLAMP;

        backgroundSamplerDescription.AddressV =
            D3D11_TEXTURE_ADDRESS_CLAMP;

        backgroundSamplerDescription.AddressW =
            D3D11_TEXTURE_ADDRESS_CLAMP;

        backgroundSamplerDescription.MaxAnisotropy =
            1;

        backgroundSamplerDescription.MipLODBias =
            0.0f;

        backgroundSamplerDescription.MinLOD =
            0.0f;

        backgroundSamplerDescription.MaxLOD =
            D3D11_FLOAT32_MAX;

        result =
            state_->device->CreateSamplerState(
                &backgroundSamplerDescription,
                &state_->backgroundSampler);

        if (FAILED(result))
        {
            error =
                "Unable to create background sampler.";

            return false;
        }

        ComPtr<ID3DBlob>
            vertexShaderCode;

        if (!shaders::CompileFromFile(
                L"World\\WorldVertex.hlsl",
                "VSMain",
                "vs_5_0",
                &vertexShaderCode,
                error))
        {
            return false;
        }

        ComPtr<ID3DBlob>
            pixelShaderCode;

        if (!shaders::CompileFromFile(
                L"World\\WorldPixel.hlsl",
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

        ComPtr<ID3DBlob>
            backgroundVertexShaderCode;

        if (!shaders::CompileFromFile(
                L"World\\WorldBackground.hlsl",
                "VSBackground",
                "vs_5_0",
                &backgroundVertexShaderCode,
                error))
        {
            return false;
        }

        ComPtr<ID3DBlob>
            backgroundPixelShaderCode;

        if (!shaders::CompileFromFile(
                L"World\\WorldBackground.hlsl",
                "PSBackground",
                "ps_5_0",
                &backgroundPixelShaderCode,
                error))
        {
            return false;
        }

        result =
            state_->device->CreateVertexShader(
                backgroundVertexShaderCode->GetBufferPointer(),
                backgroundVertexShaderCode->GetBufferSize(),
                nullptr,
                &state_->backgroundVertexShader);

        if (FAILED(result))
        {
            error =
                "Unable to create background vertex shader.";

            return false;
        }

        result =
            state_->device->CreatePixelShader(
                backgroundPixelShaderCode->GetBufferPointer(),
                backgroundPixelShaderCode->GetBufferSize(),
                nullptr,
                &state_->backgroundPixelShader);

        if (FAILED(result))
        {
            error =
                "Unable to create background pixel shader.";

            return false;
        }

        ComPtr<ID3DBlob>
            skyVertexShaderCode;

        if (!shaders::CompileFromFile(
                L"World\\WorldSky.hlsl",
                "VSSky",
                "vs_5_0",
                &skyVertexShaderCode,
                error))
        {
            return false;
        }

        ComPtr<ID3DBlob>
            skyPixelShaderCode;

        if (!shaders::CompileFromFile(
                L"World\\WorldSky.hlsl",
                "PSSky",
                "ps_5_0",
                &skyPixelShaderCode,
                error))
        {
            return false;
        }

        result =
            state_->device->CreateVertexShader(
                skyVertexShaderCode->GetBufferPointer(),
                skyVertexShaderCode->GetBufferSize(),
                nullptr,
                &state_->skyVertexShader);

        if (FAILED(result))
        {
            error =
                "Unable to create sky vertex shader.";

            return false;
        }

        result =
            state_->device->CreatePixelShader(
                skyPixelShaderCode->GetBufferPointer(),
                skyPixelShaderCode->GetBufferSize(),
                nullptr,
                &state_->skyPixelShader);

        if (FAILED(result))
        {
            error =
                "Unable to create sky pixel shader.";

            return false;
        }

        ComPtr<ID3DBlob>
            flareVertexShaderCode;

        if (!shaders::CompileFromFile(
                L"World\\WorldFlare.hlsl",
                "VSFlare",
                "vs_5_0",
                &flareVertexShaderCode,
                error))
        {
            return false;
        }

        ComPtr<ID3DBlob>
            flarePixelShaderCode;

        if (!shaders::CompileFromFile(
                L"World\\WorldFlare.hlsl",
                "PSFlare",
                "ps_5_0",
                &flarePixelShaderCode,
                error))
        {
            return false;
        }

        result =
            state_->device->CreateVertexShader(
                flareVertexShaderCode->GetBufferPointer(),
                flareVertexShaderCode->GetBufferSize(),
                nullptr,
                &state_->flareVertexShader);

        if (FAILED(result))
        {
            error =
                "Unable to create flare vertex shader.";

            return false;
        }

        result =
            state_->device->CreatePixelShader(
                flarePixelShaderCode->GetBufferPointer(),
                flarePixelShaderCode->GetBufferSize(),
                nullptr,
                &state_->flarePixelShader);

        if (FAILED(result))
        {
            error =
                "Unable to create flare pixel shader.";

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

        D3D11_BUFFER_DESC
    skyConstantDescription{};

        skyConstantDescription.ByteWidth =
            sizeof(SkyConstants);

        skyConstantDescription.Usage =
            D3D11_USAGE_DEFAULT;

        skyConstantDescription.BindFlags =
            D3D11_BIND_CONSTANT_BUFFER;

        result =
            state_->device->CreateBuffer(
                &skyConstantDescription,
                nullptr,
                &state_->skyConstantBuffer);

        if (FAILED(result))
        {
            error =
                "Unable to create sky constant buffer.";

            return false;
        }

        D3D11_BUFFER_DESC
            flareConstantDescription{};

        flareConstantDescription.ByteWidth =
            sizeof(FlareConstants);

        flareConstantDescription.Usage =
            D3D11_USAGE_DEFAULT;

        flareConstantDescription.BindFlags =
            D3D11_BIND_CONSTANT_BUFFER;

        result =
            state_->device->CreateBuffer(
                &flareConstantDescription,
                nullptr,
                &state_->flareConstantBuffer);

        if (FAILED(result))
        {
            error =
                "Unable to create flare constant buffer.";

            return false;
        }

        D3D11_BUFFER_DESC
            omniLightConstantDescription{};

        omniLightConstantDescription.ByteWidth =
            sizeof(OmniLightConstants);

        omniLightConstantDescription.Usage =
            D3D11_USAGE_DEFAULT;

        omniLightConstantDescription.BindFlags =
            D3D11_BIND_CONSTANT_BUFFER;

        result =
            state_->device->CreateBuffer(
                &omniLightConstantDescription,
                nullptr,
                &state_->omniLightConstantBuffer);

        if (FAILED(result))
        {
            error =
                "Unable to create OmniLight constant buffer.";

            return false;
        }

        D3D11_BUFFER_DESC
            spotLightConstantDescription{};

        spotLightConstantDescription.ByteWidth =
            sizeof(SpotLightConstants);

        spotLightConstantDescription.Usage =
            D3D11_USAGE_DEFAULT;

        spotLightConstantDescription.BindFlags =
            D3D11_BIND_CONSTANT_BUFFER;

        result =
            state_->device->CreateBuffer(
                &spotLightConstantDescription,
                nullptr,
                &state_->spotLightConstantBuffer);

        if (FAILED(result))
        {
            error =
                "Unable to create SpotLight constant buffer.";

            return false;
        }

        if (!state_->particleRenderer.Initialize(
                state_->device.Get(),
                error))
        {
            error =
                "Unable to initialize particle renderer: " +
                error;

            return false;
        }

        state_->width =
            width;

        state_->height =
            height;

        return true;
    }

    bool Renderer::SetBackgroundImage(
        const core::images::RgbaImage& image,
        std::string& error)
    {
        error.clear();

        if (!state_ ||
            !state_->device ||
            !state_->context)
        {
            error =
                "Renderer is not initialized.";

            return false;
        }

        ComPtr<ID3D11ShaderResourceView>
            backgroundView;

        if (!CreateRgbaTexture(
                state_->device.Get(),
                state_->context.Get(),
                image,
                true,
                backgroundView,
                error))
        {
            error =
                "Unable to create background texture: " +
                error;

            return false;
        }

        state_->backgroundTextureView =
            std::move(
                backgroundView);

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

        const bool hasGeometry =
        !scene.meshes.empty() &&
        (
            !scene.instances.empty() ||
            !scene.lodInstances.empty()
        );

        const bool hasBackground =
            state_->backgroundTextureView.Get() !=
                nullptr;

        if (!hasGeometry &&
            !scene.sky.enabled &&
            !hasBackground)
        {
            error =
                "Scene contains no geometry or background.";

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

            gpuMesh.vertexCount =
                vertices.size();

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

        state_->omniLights =
            scene.omniLights;

        state_->spotLights =
            scene.spotLights;

        state_->pulseLights =
            scene.pulseLights;

        state_->flares =
            scene.flares;

        state_->particleEmitters =
            scene.particleEmitters;

        state_->particleSystems.clear();

        state_->particleSystems.reserve(
            state_->particleEmitters.size());

        std::size_t particleRuntimeCapacity =
            0;

        std::uint32_t particleSeed =
            0x6D2B79F5u;

        for (const SceneParticleEmitter& emitter :
             state_->particleEmitters)
        {
            core::world::particles::ParticleRuntimeSystem
                runtimeSystem;

            std::string
                particleError;

            if (!runtimeSystem.Initialize(
                    emitter.system,
                    emitter.transform,
                    particleSeed,
                    particleError))
            {
                error =
                    "Unable to initialize particle runtime " +
                    emitter.resource +
                    "/" +
                    emitter.system.name +
                    ": " +
                    particleError;

                return false;
            }

            particleRuntimeCapacity +=
                runtimeSystem.Capacity();

            state_->particleSystems.push_back(
                std::move(
                    runtimeSystem));

            particleSeed +=
                0x9E3779B9u;
        }

        if (state_->particleSystems.size() !=
            state_->particleEmitters.size())
        {
            error =
                "Particle runtime/emitter initialization count mismatch.";

            return false;
        }

        state_->particleUpdateTime =
            std::chrono::steady_clock::now();

        state_->particleRuntimeReported =
            false;

        core::Log::Info(
            std::string(
                "Particle runtime systems initialized: ") +
            std::to_string(
                state_->particleSystems.size()));

        core::Log::Info(
            std::string(
                "Particle runtime total capacity: ") +
            std::to_string(
                particleRuntimeCapacity));

        if (!state_->particleRenderer.SetCapacity(
                particleRuntimeCapacity,
                error))
        {
            error =
                "Unable to configure particle GPU buffer: " +
                error;

            return false;
        }

        for (const SceneParticleEmitter& emitter :
             state_->particleEmitters)
        {
            if (emitter.textureIndex >=
                0)
            {
                const std::size_t textureIndex =
                    static_cast<std::size_t>(
                        emitter.textureIndex);

                if (textureIndex >=
                    state_->textures.size())
                {
                    error =
                        "Particle emitter references invalid GPU texture.";

                    return false;
                }
            }

            if (!emitter.animatedTexture)
            {
                continue;
            }

            if (emitter.textureAnimationFps <=
                    0.0f ||
                emitter.textureFrameIndices.empty())
            {
                error =
                    "Animated particle emitter contains invalid animation data: " +
                    emitter.resource +
                    "/" +
                    emitter.system.name;

                return false;
            }

            for (const std::int32_t frameTextureIndex :
                 emitter.textureFrameIndices)
            {
                if (frameTextureIndex <
                    0)
                {
                    error =
                        "Animated particle frame contains invalid texture index: " +
                        emitter.resource +
                        "/" +
                        emitter.system.name;

                    return false;
                }

                if (static_cast<std::size_t>(
                        frameTextureIndex) >=
                    state_->textures.size())
                {
                    error =
                        "Animated particle frame references invalid GPU texture: " +
                        emitter.resource +
                        "/" +
                        emitter.system.name;

                    return false;
                }
            }
        }

        state_->particleGpuReported =
            false;

        core::Log::Info(
            std::string(
                "GPU particle emitters: ") +
            std::to_string(
                state_->particleEmitters.size()));

        for (const SceneFlare& flare :
             state_->flares)
        {
            if (flare.textureIndex >=
                state_->textures.size())
            {
                error =
                    "Flare references invalid texture.";

                return false;
            }
        }

        state_->sky =
            scene.sky;

        if (state_->sky.enabled &&
            state_->sky.gradientTextureIndex >=
                0)
        {
            const std::size_t textureIndex =
                static_cast<std::size_t>(
                    state_->sky.gradientTextureIndex);

            if (textureIndex >=
                state_->textures.size())
            {
                error =
                    "Sky references invalid gradient texture.";

                return false;
            }
        }

        state_->startTime =
            std::chrono::steady_clock::now();

        state_->lodInstances =
            scene.lodInstances;

        state_->renderInstances.clear();

        state_->renderInstances.reserve(
            state_->instances.size() +
            state_->lodInstances.size() *
                3u);

        //
        // Static/non-LOD scene instances.
        //
        state_->renderInstances.insert(state_->renderInstances.end(), state_->instances.begin(), state_->instances.end());

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

        if (!hasBounds &&
            !state_->sky.enabled &&
            !hasBackground)
        {
            error =
                "Unable to calculate world bounds.";

            return false;
        }

        if (!hasBounds)
        {
            sceneMinimum =
            {
                0.0f,
                0.0f,
                0.0f
            };

            sceneMaximum =
                sceneMinimum;
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

        core::Log::Info(
            std::string(
                "GPU OmniLight sources: ") +
            std::to_string(
                state_->omniLights.size()));

        core::Log::Info(
            std::string(
                "GPU SpotLight sources: ") +
            std::to_string(
                state_->spotLights.size()));

        core::Log::Info(
            std::string(
                "GPU PulseLight sources: ") +
            std::to_string(
                state_->pulseLights.size()));

        core::Log::Info(
            std::string(
                "GPU flare elements: ") +
            std::to_string(
                state_->flares.size()));

        return true;
    }

    bool Renderer::UpdateMeshVertices(
        const std::size_t meshIndex,
        const core::assets::MeshData& mesh,
        std::string& error)
    {
        error.clear();

        if (!state_ ||
            !state_->context)
        {
            error =
                "Renderer is not initialized.";

            return false;
        }

        if (meshIndex >=
            state_->meshes.size())
        {
            error =
                "Animated mesh index is invalid.";

            return false;
        }

        State::GpuMesh& gpuMesh =
            state_->meshes[
                meshIndex];

        if (mesh.vertices.size() !=
            gpuMesh.vertexCount)
        {
            error =
                "Animated mesh vertex count changed.";

            return false;
        }

        std::vector<GpuVertex>
            vertices;

        vertices.reserve(
            mesh.vertices.size());

        for (const core::assets::MeshVertex& vertex :
             mesh.vertices)
        {
            const DirectX::XMFLOAT3 normal =
                UnpackNormal(
                    vertex.packedNormal);

            vertices.push_back(
            {
                vertex.position.x,
                vertex.position.y,
                vertex.position.z,

                normal.x,
                normal.y,
                normal.z,

                vertex.u,
                vertex.v
            });
        }

        state_->context->UpdateSubresource(
            gpuMesh.vertexBuffer.Get(),
            0,
            nullptr,
            vertices.data(),
            0,
            0);

        return true;
    }

    bool Renderer::SetInstanceTransformRange(
        const std::size_t firstInstance,
        const std::size_t instanceCount,
        const core::math::Transform3x4& transform) noexcept
    {
        if (!state_)
        {
            return false;
        }

        if (firstInstance >
            state_->instances.size())
        {
            return false;
        }

        if (instanceCount >
            state_->instances.size() -
                firstInstance)
        {
            return false;
        }

        for (std::size_t index = 0;
             index <
                instanceCount;
             ++index)
        {
            state_->instances[
                firstInstance +
                index]
                .transform =
                    transform;
        }

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
        (
            state_->meshes.empty() &&
            !state_->sky.enabled &&
            state_->backgroundTextureView.Get() ==
                nullptr
        ))
        {
            error =
                "Renderer has nothing to render.";

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

        //
        // Main menu static background.
        //
        // Rendered before the 3D character.
        //
        if (state_->backgroundTextureView.Get() !=
            nullptr)
        {
            state_->context->IASetInputLayout(
                nullptr);

            state_->context->IASetVertexBuffers(
                0,
                0,
                nullptr,
                nullptr,
                nullptr);

            state_->context->IASetIndexBuffer(
                nullptr,
                DXGI_FORMAT_UNKNOWN,
                0);

            state_->context->IASetPrimitiveTopology(
                D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            state_->context->VSSetShader(
                state_->backgroundVertexShader.Get(),
                nullptr,
                0);

            state_->context->PSSetShader(
                state_->backgroundPixelShader.Get(),
                nullptr,
                0);

            //
            // Background must never touch depth.
            //
            state_->context->OMSetDepthStencilState(
                state_->flareDepthState.Get(),
                0);

            state_->context->OMSetBlendState(
                nullptr,
                nullptr,
                0xFFFFFFFFu);

            ID3D11SamplerState*
                backgroundSamplers[] =
            {
                state_->backgroundSampler.Get()
            };

            state_->context->PSSetSamplers(
                0,
                1,
                backgroundSamplers);

            ID3D11ShaderResourceView*
                backgroundViews[] =
            {
                state_->backgroundTextureView.Get()
            };

            state_->context->PSSetShaderResources(
                0,
                1,
                backgroundViews);

            state_->context->Draw(
                3,
                0);

            ID3D11ShaderResourceView*
                emptyBackgroundViews[] =
            {
                nullptr
            };

            state_->context->PSSetShaderResources(
                0,
                1,
                emptyBackgroundViews);

            state_->context->OMSetDepthStencilState(
                state_->depthState.Get(),
                0);
        }

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
            state_->sky.enabled &&
            state_->sky.definition.farPlane >
                NearPlane
                ? state_->sky.definition.farPlane
                : std::max(
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

        const auto currentTime =
            std::chrono::steady_clock::now();

        const float elapsedSeconds =
            std::chrono::duration<float>(
                currentTime -
                state_->startTime).count();

        float particleDeltaSeconds =
            std::chrono::duration<float>(
                currentTime -
                state_->particleUpdateTime).count();

        state_->particleUpdateTime =
            currentTime;

        particleDeltaSeconds =
            std::clamp(
                particleDeltaSeconds,
                0.0f,
                0.1f);

        std::size_t activeParticleCount =
            0;

        std::size_t spawnedParticleCount =
            0;

        std::size_t killedParticleCount =
            0;

        std::size_t particleBarrierInteractions =
            0;

        std::size_t particleCollisionInteractions =
            0;

        for (core::world::particles::ParticleRuntimeSystem& particleSystem :
             state_->particleSystems)
        {
            particleSystem.Update(
                particleDeltaSeconds);

            activeParticleCount +=
                particleSystem.ActiveParticleCount();

            spawnedParticleCount +=
                particleSystem.Statistics().spawned;

            killedParticleCount +=
                particleSystem.Statistics().killed;

            particleBarrierInteractions +=
                particleSystem.Statistics()
                    .barrierInteractions;

            particleCollisionInteractions +=
                particleSystem.Statistics()
                    .collisionInteractions;
        }

        if (!state_->particleRuntimeReported &&
            elapsedSeconds >=
                1.0f)
        {
            core::Log::Info(
                std::string(
                    "Particle runtime active after 1s: ") +
                std::to_string(
                    activeParticleCount));

            core::Log::Info(
                std::string(
                    "Particle runtime spawned after 1s: ") +
                std::to_string(
                    spawnedParticleCount));

            core::Log::Info(
                std::string(
                    "Particle runtime killed after 1s: ") +
                std::to_string(
                    killedParticleCount));

            core::Log::Info(
                std::string(
                    "Particle Barrier interactions after 1s: ") +
                std::to_string(
                    particleBarrierInteractions));

            core::Log::Info(
                std::string(
                    "Particle Collide interactions after 1s: ") +
                std::to_string(
                    particleCollisionInteractions));

            state_->particleRuntimeReported =
                true;
        }

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

        const OmniLightConstants
            omniLightConstants =
                BuildOmniLightConstants(
                    state_->omniLights,
                    state_->pulseLights,
                    state_->camera,
                    elapsedSeconds);

        state_->context->UpdateSubresource(
            state_->omniLightConstantBuffer.Get(),
            0,
            nullptr,
            &omniLightConstants,
            0,
            0);

        ID3D11Buffer*
            omniLightBuffers[] =
        {
            state_->omniLightConstantBuffer.Get()
        };

        state_->context->PSSetConstantBuffers(
            1,
            1,
            omniLightBuffers);

        const SpotLightConstants
            spotLightConstants =
                BuildSpotLightConstants(
                    state_->spotLights,
                    state_->camera);

        state_->context->UpdateSubresource(
            state_->spotLightConstantBuffer.Get(),
            0,
            nullptr,
            &spotLightConstants,
            0,
            0);

        ID3D11Buffer*
            spotLightBuffers[] =
        {
            state_->spotLightConstantBuffer.Get()
        };

        state_->context->PSSetConstantBuffers(
            2,
            1,
            spotLightBuffers);

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

        state_->context->UpdateSubresource(
            state_->constantBuffer.Get(),
            0,
            nullptr,
            &constants,
            0,
            0);

        const SkyConstants
            skyConstants =
                BuildSkyConstants(
                    state_->sky,
                    elapsedSeconds);

        state_->context->UpdateSubresource(
            state_->skyConstantBuffer.Get(),
            0,
            nullptr,
            &skyConstants,
            0,
            0);

        ID3D11Buffer*
            skyBuffers[] =
        {
            state_->skyConstantBuffer.Get()
        };

        state_->context->PSSetConstantBuffers(
            3,
            1,
            skyBuffers);

        if (state_->sky.enabled)
        {
            state_->context->IASetInputLayout(
                nullptr);

            state_->context->IASetVertexBuffers(
                0,
                0,
                nullptr,
                nullptr,
                nullptr);

            state_->context->IASetIndexBuffer(
                nullptr,
                DXGI_FORMAT_UNKNOWN,
                0);

            state_->context->IASetPrimitiveTopology(
                D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            state_->context->VSSetShader(
                state_->skyVertexShader.Get(),
                nullptr,
                0);

            state_->context->PSSetShader(
                state_->skyPixelShader.Get(),
                nullptr,
                0);

            state_->context->OMSetDepthStencilState(
                state_->flareDepthState.Get(),
                0);

            ID3D11ShaderResourceView*
                skyView =
                    nullptr;

            if (state_->sky.gradientTextureIndex >=
                0)
            {
                const std::size_t textureIndex =
                    static_cast<std::size_t>(
                        state_->sky.gradientTextureIndex);

                skyView =
                    state_->textures[
                        textureIndex].Get();
            }

            state_->context->PSSetShaderResources(
                10,
                1,
                &skyView);

            state_->context->Draw(
                3,
                0);

            ID3D11ShaderResourceView*
                emptySkyView =
                    nullptr;

            state_->context->PSSetShaderResources(
                10,
                1,
                &emptySkyView);

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

            state_->context->OMSetDepthStencilState(
                state_->depthState.Get(),
                0);
        }

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

                constants.modelTint =
                {
                    1.0f,
                    1.0f,
                    1.0f,
                    0.0f
                };

                constants.modelOverlayColour =
                {
                    1.0f,
                    1.0f,
                    1.0f,
                    1.0f
                };

                constants.modelOverlayParameters = {};

                ID3D11ShaderResourceView*
                    modelTextureView =
                        nullptr;

                ID3D11ShaderResourceView*
                    overlayTextureView =
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

                    constants.modelTint =
                    {
                        material.tintColour[0],
                        material.tintColour[1],
                        material.tintColour[2],
                        material.tintColour[3]
                    };

                    constants.modelOverlayColour =
                    {
                        material.overlayColour[0],
                        material.overlayColour[1],
                        material.overlayColour[2],
                        material.overlayColour[3]
                    };

                    constants.modelOverlayParameters =
                    {
                        material.overlayParameters[0],
                        material.overlayParameters[1],
                        material.overlayParameters[2],
                        material.overlayParameters[3]
                    };

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

                    if (material.overlayTextureIndex >= 0)
                    {
                        const std::size_t overlayIndex = static_cast<std::size_t>(material.overlayTextureIndex);
                        if (overlayIndex >= state_->textures.size())
                        {
                            error = "Model material references invalid overlay texture.";
                            return false;
                        }
                        overlayTextureView = state_->textures[overlayIndex].Get();
                        constants.modelParameters.z = 1.0f;
                    }
                }

                state_->context->PSSetShaderResources(
                    5,
                    1,
                    &modelTextureView);

                state_->context->PSSetShaderResources(
                    11,
                    1,
                    &overlayTextureView);

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

            state_->context->PSSetShaderResources(
                11,
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

        DirectX::XMFLOAT4X4
            particleViewProjection{};

        DirectX::XMStoreFloat4x4(
            &particleViewProjection,
            viewProjection);

        std::string
            particleRenderError;

        if (!state_->particleRenderer.Render(
                state_->context.Get(),
                state_->particleSystems,
                state_->particleEmitters,
                state_->textures,
                state_->camera,
                particleViewProjection,
                particleRenderError))
        {
            error =
                "Particle rendering failed: " +
                particleRenderError;

            return false;
        }

        if (!state_->particleGpuReported &&
            elapsedSeconds >=
                1.0f)
        {
            core::Log::Info(
                std::string(
                    "GPU particles rendered after 1s: ") +
                std::to_string(
                    state_->particleRenderer
                        .LastRenderedParticleCount()));

            core::Log::Info(
                std::string(
                    "GPU particle draw calls after 1s: ") +
                std::to_string(
                    state_->particleRenderer
                        .LastDrawCallCount()));

            state_->particleGpuReported =
                true;
        }

        if (!state_->flares.empty())
        {
            state_->context->IASetInputLayout(
                nullptr);

            state_->context->IASetVertexBuffers(
                0,
                0,
                nullptr,
                nullptr,
                nullptr);

            state_->context->IASetIndexBuffer(
                nullptr,
                DXGI_FORMAT_UNKNOWN,
                0);

            state_->context->IASetPrimitiveTopology(
                D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

            state_->context->VSSetShader(
                state_->flareVertexShader.Get(),
                nullptr,
                0);

            state_->context->PSSetShader(
                state_->flarePixelShader.Get(),
                nullptr,
                0);

            ID3D11Buffer*
                flareBuffers[] =
            {
                state_->flareConstantBuffer.Get()
            };

            state_->context->VSSetConstantBuffers(
                0,
                1,
                flareBuffers);

            state_->context->PSSetConstantBuffers(
                0,
                1,
                flareBuffers);

            ID3D11SamplerState*
                flareSamplers[] =
            {
                state_->terrainBlendSampler.Get()
            };

            state_->context->PSSetSamplers(
                0,
                1,
                flareSamplers);

            constexpr float
                FlareBlendFactor[4]
            {
                0.0f,
                0.0f,
                0.0f,
                0.0f
            };

            state_->context->OMSetBlendState(
                state_->additiveBlendState.Get(),
                FlareBlendFactor,
                0xFFFFFFFFu);

            state_->context->OMSetDepthStencilState(
                state_->flareDepthState.Get(),
                0);

            for (const SceneFlare& flare :
                 state_->flares)
            {
                if (flare.textureIndex >=
                    state_->textures.size())
                {
                    continue;
                }

                const float deltaX =
                    flare.position[0] -
                    state_->camera.position.x;

                const float deltaY =
                    flare.position[1] -
                    state_->camera.position.y;

                const float deltaZ =
                    flare.position[2] -
                    state_->camera.position.z;

                const float distanceSquared =
                    deltaX * deltaX +
                    deltaY * deltaY +
                    deltaZ * deltaZ;

                if (flare.maxDistance >
                    0.0f)
                {
                    const float maxDistanceSquared =
                        flare.maxDistance *
                        flare.maxDistance;

                    if (distanceSquared >
                        maxDistanceSquared)
                    {
                        continue;
                    }
                }

                FlareConstants
                    flareConstants{};

                XMStoreFloat4x4(
                    &flareConstants.viewProjection,
                    viewProjection);

                flareConstants.sourcePositionSize =
                {
                    flare.position[0],
                    flare.position[1],
                    flare.position[2],
                    flare.size
                };

                flareConstants.colour =
                {
                    flare.colour[0],
                    flare.colour[1],
                    flare.colour[2],
                    flare.colour[3]
                };

                flareConstants.parameters =
                {
                    flare.maxDistance,
                    flare.area,
                    flare.fadeSpeed,
                    flare.depth
                };

                flareConstants.cameraPosition =
                {
                    state_->camera.position.x,
                    state_->camera.position.y,
                    state_->camera.position.z,
                    0.0f
                };

                flareConstants.screenParameters =
                {
                    static_cast<float>(
                        state_->width),

                    static_cast<float>(
                        state_->height),

                    0.0f,
                    0.0f
                };

                state_->context->UpdateSubresource(
                    state_->flareConstantBuffer.Get(),
                    0,
                    nullptr,
                    &flareConstants,
                    0,
                    0);

                ID3D11ShaderResourceView*
                    flareViews[2]
                {
                    state_->textures[
                        flare.textureIndex].Get(),

                    state_->depthShaderResourceView.Get()
                };

                state_->context->PSSetShaderResources(
                    0,
                    2,
                    flareViews);

                state_->context->Draw(
                    4,
                    0);
            }

            ID3D11ShaderResourceView*
                emptyFlareViews[2]
            {
                nullptr,
                nullptr
            };

            state_->context->PSSetShaderResources(
                0,
                2,
                emptyFlareViews);

            state_->context->OMSetBlendState(
                nullptr,
                nullptr,
                0xFFFFFFFFu);

            state_->context->OMSetDepthStencilState(
                state_->depthReadState.Get(),
                0);
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
