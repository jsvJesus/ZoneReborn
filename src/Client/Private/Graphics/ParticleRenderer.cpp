#include "Graphics/ParticleRenderer.h"

#include "Graphics/Shaders/ShaderCompiler.h"

#include <d3dcompiler.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace
{
    using Microsoft::WRL::ComPtr;

    struct ParticleGpuVertex final
    {
        DirectX::XMFLOAT3
            position{};

        DirectX::XMFLOAT2
            uv{};

        DirectX::XMFLOAT4
            colour{};
    };

    struct ParticleDrawCommand final
    {
        std::uint32_t firstVertex =
            0;

        std::uint32_t vertexCount =
            0;

        std::size_t textureIndex =
            0;

        bool additive =
            false;
    };

    struct ParticleConstants final
    {
        DirectX::XMFLOAT4X4
            viewProjection;
    };

    static_assert(
        sizeof(ParticleConstants) %
            16u ==
        0u);

    DirectX::XMFLOAT3 Add(
        const DirectX::XMFLOAT3& first,
        const DirectX::XMFLOAT3& second) noexcept
    {
        return
        {
            first.x + second.x,
            first.y + second.y,
            first.z + second.z
        };
    }

    DirectX::XMFLOAT3 Subtract(
        const DirectX::XMFLOAT3& first,
        const DirectX::XMFLOAT3& second) noexcept
    {
        return
        {
            first.x - second.x,
            first.y - second.y,
            first.z - second.z
        };
    }

    DirectX::XMFLOAT3 Scale(
        const DirectX::XMFLOAT3& value,
        const float amount) noexcept
    {
        return
        {
            value.x * amount,
            value.y * amount,
            value.z * amount
        };
    }

    float LengthSquared(
        const DirectX::XMFLOAT3& value) noexcept
    {
        return
            value.x * value.x +
            value.y * value.y +
            value.z * value.z;
    }

    DirectX::XMFLOAT3 Normalize(
        const DirectX::XMFLOAT3& value) noexcept
    {
        const float lengthSquared =
            LengthSquared(
                value);

        if (lengthSquared <=
            0.0000001f)
        {
            return {};
        }

        const float inverseLength =
            1.0f /
            std::sqrt(
                lengthSquared);

        return
            Scale(
                value,
                inverseLength);
    }

    DirectX::XMFLOAT3 Cross(
        const DirectX::XMFLOAT3& first,
        const DirectX::XMFLOAT3& second) noexcept
    {
        return
        {
            first.y * second.z -
                first.z * second.y,

            first.z * second.x -
                first.x * second.z,

            first.x * second.y -
                first.y * second.x
        };
    }

    DirectX::XMFLOAT3 LerpPoint(
        const DirectX::XMFLOAT3& first,
        const DirectX::XMFLOAT3& second,
        const float factor) noexcept
    {
        return
            Add(
                first,
                Scale(
                    Subtract(
                        second,
                        first),
                    factor));
    }

    DirectX::XMFLOAT3 ParticlePosition(
        const core::math::Vector3& value) noexcept
    {
        return
        {
            value.x,
            value.y,
            value.z
        };
    }

    DirectX::XMFLOAT4 ParticleColour(
        const std::array<float, 4>& colour) noexcept
    {
        return
        {
            std::clamp(
                colour[0],
                0.0f,
                1.0f),

            std::clamp(
                colour[1],
                0.0f,
                1.0f),

            std::clamp(
                colour[2],
                0.0f,
                1.0f),

            std::clamp(
                colour[3],
                0.0f,
                1.0f)
        };
    }

    void AppendQuad(
        std::vector<ParticleGpuVertex>& vertices,
        const DirectX::XMFLOAT3& bottomLeft,
        const DirectX::XMFLOAT3& topLeft,
        const DirectX::XMFLOAT3& topRight,
        const DirectX::XMFLOAT3& bottomRight,
        const DirectX::XMFLOAT4& colour)
    {
        vertices.push_back(
        {
            bottomLeft,
            {0.0f, 1.0f},
            colour
        });

        vertices.push_back(
        {
            topLeft,
            {0.0f, 0.0f},
            colour
        });

        vertices.push_back(
        {
            topRight,
            {1.0f, 0.0f},
            colour
        });

        vertices.push_back(
        {
            bottomLeft,
            {0.0f, 1.0f},
            colour
        });

        vertices.push_back(
        {
            topRight,
            {1.0f, 0.0f},
            colour
        });

        vertices.push_back(
        {
            bottomRight,
            {1.0f, 1.0f},
            colour
        });
    }

    void AppendBillboard(
        std::vector<ParticleGpuVertex>& vertices,
        const core::world::particles::ParticleRuntimeParticle& particle,
        const DirectX::XMFLOAT3& cameraRight,
        const DirectX::XMFLOAT3& cameraUp)
    {
        const DirectX::XMFLOAT3 centre =
            ParticlePosition(
                particle.position);

        const float cosine =
            std::cos(
                particle.rotation);

        const float sine =
            std::sin(
                particle.rotation);

        const DirectX::XMFLOAT3 rotatedRight =
            Add(
                Scale(
                    cameraRight,
                    cosine),
                Scale(
                    cameraUp,
                    sine));

        const DirectX::XMFLOAT3 rotatedUp =
            Add(
                Scale(
                    cameraUp,
                    cosine),
                Scale(
                    cameraRight,
                    -sine));

        const float halfSize =
            std::max(
                particle.size *
                    0.5f,
                0.001f);

        const DirectX::XMFLOAT3 right =
            Scale(
                rotatedRight,
                halfSize);

        const DirectX::XMFLOAT3 up =
            Scale(
                rotatedUp,
                halfSize);

        const DirectX::XMFLOAT3 bottomLeft =
            Subtract(
                Subtract(
                    centre,
                    right),
                up);

        const DirectX::XMFLOAT3 topLeft =
            Add(
                Subtract(
                    centre,
                    right),
                up);

        const DirectX::XMFLOAT3 topRight =
            Add(
                Add(
                    centre,
                    right),
                up);

        const DirectX::XMFLOAT3 bottomRight =
            Subtract(
                Add(
                    centre,
                    right),
                up);

        AppendQuad(
            vertices,
            bottomLeft,
            topLeft,
            topRight,
            bottomRight,
            ParticleColour(
                particle.colour));
    }

    void AppendTrail(
        std::vector<ParticleGpuVertex>& vertices,
        const core::world::particles::ParticleRuntimeParticle& particle,
        const core::world::particles::ParticleRendererDefinition& renderer,
        const DirectX::XMFLOAT3& cameraPosition,
        const DirectX::XMFLOAT3& cameraRight)
    {
        DirectX::XMFLOAT3 end =
            ParticlePosition(
                particle.position);

        DirectX::XMFLOAT3 start =
            ParticlePosition(
                particle.previousPosition);

        DirectX::XMFLOAT3 direction =
            Subtract(
                end,
                start);

        if (LengthSquared(
                direction) <=
            0.000001f)
        {
            const DirectX::XMFLOAT3 velocity =
                ParticlePosition(
                    particle.velocity);

            const DirectX::XMFLOAT3 velocityDirection =
                Normalize(
                    velocity);

            if (LengthSquared(
                    velocityDirection) <=
                0.000001f)
            {
                return;
            }

            const float fallbackLength =
                std::max(
                    renderer.width *
                        4.0f,
                    0.05f);

            start =
                Subtract(
                    end,
                    Scale(
                        velocityDirection,
                        fallbackLength));

            direction =
                Subtract(
                    end,
                    start);
        }

        direction =
            Normalize(
                direction);

        const DirectX::XMFLOAT3 midpoint =
            Scale(
                Add(
                    start,
                    end),
                0.5f);

        DirectX::XMFLOAT3 viewDirection =
            Normalize(
                Subtract(
                    cameraPosition,
                    midpoint));

        DirectX::XMFLOAT3 side =
            Normalize(
                Cross(
                    direction,
                    viewDirection));

        if (LengthSquared(
                side) <=
            0.000001f)
        {
            side =
                cameraRight;
        }

        float width =
            renderer.width;

        if (width <=
            0.0f)
        {
            width =
                std::max(
                    particle.size *
                        0.2f,
                    0.01f);
        }

        const DirectX::XMFLOAT3 halfWidth =
            Scale(
                side,
                width *
                    0.5f);

        const DirectX::XMFLOAT3 startLeft =
            Subtract(
                start,
                halfWidth);

        const DirectX::XMFLOAT3 startRight =
            Add(
                start,
                halfWidth);

        const DirectX::XMFLOAT3 endLeft =
            Subtract(
                end,
                halfWidth);

        const DirectX::XMFLOAT3 endRight =
            Add(
                end,
                halfWidth);

        AppendQuad(
            vertices,
            startLeft,
            endLeft,
            endRight,
            startRight,
            ParticleColour(
                particle.colour));
    }

    void AppendBlur(
        std::vector<ParticleGpuVertex>& vertices,
        const core::world::particles::ParticleRuntimeParticle& particle,
        const core::world::particles::ParticleRendererDefinition& renderer,
        const DirectX::XMFLOAT3& cameraPosition,
        const DirectX::XMFLOAT3& cameraRight)
    {
        const DirectX::XMFLOAT3 end =
            ParticlePosition(
                particle.position);

        const DirectX::XMFLOAT3 velocity =
            ParticlePosition(
                particle.velocity);

        DirectX::XMFLOAT3 start =
            renderer.time >
                    0.0f &&
                LengthSquared(
                    velocity) >
                    0.000001f
                ? Subtract(
                    end,
                    Scale(
                        velocity,
                        renderer.time))
                : ParticlePosition(
                    particle.previousPosition);

        DirectX::XMFLOAT3 direction =
            Subtract(
                end,
                start);

        if (LengthSquared(
                direction) <=
            0.000001f)
        {
            start =
                ParticlePosition(
                    particle.previousPosition);

            direction =
                Subtract(
                    end,
                    start);
        }

        if (LengthSquared(
                direction) <=
            0.000001f)
        {
            return;
        }

        direction =
            Normalize(
                direction);

        const DirectX::XMFLOAT3 midpoint =
            Scale(
                Add(
                    start,
                    end),
                0.5f);

        const DirectX::XMFLOAT3 viewDirection =
            Normalize(
                Subtract(
                    cameraPosition,
                    midpoint));

        DirectX::XMFLOAT3 side =
            Normalize(
                Cross(
                    direction,
                    viewDirection));

        if (LengthSquared(
                side) <=
            0.000001f)
        {
            side =
                cameraRight;
        }

        const float width =
            std::abs(
                renderer.width) >
                0.0f
                ? std::abs(
                    renderer.width)
                : std::max(
                    particle.size *
                        0.2f,
                    0.01f);

        const DirectX::XMFLOAT3 halfWidth =
            Scale(
                side,
                width *
                    0.5f);

        AppendQuad(
            vertices,
            Subtract(
                start,
                halfWidth),
            Subtract(
                end,
                halfWidth),
            Add(
                end,
                halfWidth),
            Add(
                start,
                halfWidth),
            ParticleColour(
                particle.colour));
    }

    std::uint32_t MixBits(
        std::uint32_t value) noexcept
    {
        value ^=
            value >>
            16u;

        value *=
            0x7FEB352Du;

        value ^=
            value >>
            15u;

        value *=
            0x846CA68Bu;

        value ^=
            value >>
            16u;

        return value;
    }

    float DeterministicSigned(
        const std::size_t systemIndex,
        const std::size_t segmentIndex,
        const std::size_t stepIndex,
        const std::uint32_t axis) noexcept
    {
        std::uint32_t value =
            static_cast<std::uint32_t>(
                systemIndex);

        value ^=
            static_cast<std::uint32_t>(
                segmentIndex) *
            0x9E3779B9u;

        value ^=
            static_cast<std::uint32_t>(
                stepIndex) *
            0x85EBCA6Bu;

        value ^=
            axis *
            0xC2B2AE35u;

        const std::uint32_t mixed =
            MixBits(
                value);

        return
            static_cast<float>(
                mixed &
                0x00FFFFFFu) /
                static_cast<float>(
                    0x007FFFFFu) -
            1.0f;
    }

    DirectX::XMFLOAT3 AmpPoint(
        const DirectX::XMFLOAT3& start,
        const DirectX::XMFLOAT3& end,
        const std::size_t systemIndex,
        const std::size_t segmentIndex,
        const std::size_t stepIndex,
        const std::size_t stepCount,
        const core::world::particles::ParticleRendererDefinition& renderer,
        const DirectX::XMFLOAT3& cameraRight,
        const DirectX::XMFLOAT3& cameraUp) noexcept
    {
        const float factor =
            static_cast<float>(
                stepIndex) /
            static_cast<float>(
                stepCount);

        DirectX::XMFLOAT3 point =
            LerpPoint(
                start,
                end,
                factor);

        if (stepIndex ==
                0 ||
            stepIndex ==
                stepCount ||
            renderer.variation ==
                0.0f)
        {
            return point;
        }

        const float widthScale =
            std::max(
                std::abs(
                    renderer.width),
                0.01f);

        const float heightScale =
            std::max(
                std::abs(
                    renderer.height),
                widthScale);

        const float envelope =
            std::sin(
                factor *
                3.14159265358979323846f);

        point =
            Add(
                point,
                Add(
                    Scale(
                        cameraRight,
                        DeterministicSigned(
                            systemIndex,
                            segmentIndex,
                            stepIndex,
                            0u) *
                            renderer.variation *
                            widthScale *
                            envelope),
                    Scale(
                        cameraUp,
                        DeterministicSigned(
                            systemIndex,
                            segmentIndex,
                            stepIndex,
                            1u) *
                            renderer.variation *
                            heightScale *
                            envelope)));

        return point;
    }

    void AppendAmp(
        std::vector<ParticleGpuVertex>& vertices,
        const std::vector<
            core::world::particles::ParticleRuntimeParticle>& particles,
        const core::world::particles::ParticleRendererDefinition& renderer,
        const DirectX::XMFLOAT3& cameraPosition,
        const DirectX::XMFLOAT3& cameraRight,
        const DirectX::XMFLOAT3& cameraUp,
        const std::size_t systemIndex)
    {
        if (particles.size() <
            2)
        {
            return;
        }

        const std::size_t segmentCount =
            renderer.circular
                ? particles.size()
                : particles.size() -
                    1;

        const std::size_t stepCount =
            renderer.steps >
                0
                ? static_cast<std::size_t>(
                    renderer.steps)
                : 1;

        const float stripWidth =
            std::max(
                std::max(
                    std::abs(
                        renderer.width),
                    std::abs(
                        renderer.height)),
                0.01f);

        for (std::size_t segmentIndex = 0;
             segmentIndex < segmentCount;
             ++segmentIndex)
        {
            const std::size_t nextIndex =
                (segmentIndex + 1) %
                particles.size();

            const DirectX::XMFLOAT3 segmentStart =
                ParticlePosition(
                    particles[
                        segmentIndex].position);

            const DirectX::XMFLOAT3 segmentEnd =
                ParticlePosition(
                    particles[
                        nextIndex].position);

            for (std::size_t stepIndex = 0;
                 stepIndex < stepCount;
                 ++stepIndex)
            {
                const DirectX::XMFLOAT3 start =
                    AmpPoint(
                        segmentStart,
                        segmentEnd,
                        systemIndex,
                        segmentIndex,
                        stepIndex,
                        stepCount,
                        renderer,
                        cameraRight,
                        cameraUp);

                const DirectX::XMFLOAT3 end =
                    AmpPoint(
                        segmentStart,
                        segmentEnd,
                        systemIndex,
                        segmentIndex,
                        stepIndex +
                            1,
                        stepCount,
                        renderer,
                        cameraRight,
                        cameraUp);

                DirectX::XMFLOAT3 direction =
                    Subtract(
                        end,
                        start);

                if (LengthSquared(
                        direction) <=
                    0.000001f)
                {
                    continue;
                }

                direction =
                    Normalize(
                        direction);

                const DirectX::XMFLOAT3 midpoint =
                    Scale(
                        Add(
                            start,
                            end),
                        0.5f);

                const DirectX::XMFLOAT3 viewDirection =
                    Normalize(
                        Subtract(
                            cameraPosition,
                            midpoint));

                DirectX::XMFLOAT3 side =
                    Normalize(
                        Cross(
                            direction,
                            viewDirection));

                if (LengthSquared(
                        side) <=
                    0.000001f)
                {
                    side =
                        cameraRight;
                }

                const DirectX::XMFLOAT3 halfWidth =
                    Scale(
                        side,
                        stripWidth *
                            0.5f);

                AppendQuad(
                    vertices,
                    Subtract(
                        start,
                        halfWidth),
                    Subtract(
                        end,
                        halfWidth),
                    Add(
                        end,
                        halfWidth),
                    Add(
                        start,
                        halfWidth),
                    ParticleColour(
                        particles[
                            segmentIndex].colour));
            }
        }
    }

    bool CheckedMultiply(
        const std::size_t first,
        const std::size_t second,
        std::size_t& output) noexcept
    {
        if (first !=
                0 &&
            second >
                std::numeric_limits<std::size_t>::max() /
                    first)
        {
            return false;
        }

        output =
            first *
            second;

        return true;
    }

    bool CheckedAdd(
        const std::size_t first,
        const std::size_t second,
        std::size_t& output) noexcept
    {
        if (second >
            std::numeric_limits<std::size_t>::max() -
                first)
        {
            return false;
        }

        output =
            first +
            second;

        return true;
    }

    bool CalculateVertexCapacity(
        const std::vector<
            core::world::particles::ParticleRuntimeSystem>& systems,
        const std::vector<client::graphics::SceneParticleEmitter>& emitters,
        std::size_t& output,
        std::string& error)
    {
        output =
            0;

        constexpr std::size_t VerticesPerQuad =
            6;

        constexpr std::size_t MaximumVertexCount =
            static_cast<std::size_t>(
                std::numeric_limits<UINT>::max()) /
            sizeof(ParticleGpuVertex);

        for (std::size_t systemIndex = 0;
             systemIndex < systems.size();
             ++systemIndex)
        {
            const client::graphics::SceneParticleEmitter& emitter =
                emitters[
                    systemIndex];

            if (!emitter.renderable ||
                !emitter.system.hasRenderer)
            {
                continue;
            }

            const auto type =
                emitter.system.renderer.type;

            std::size_t quadCount =
                0;

            switch (type)
            {
                case core::world::particles::ParticleRendererType::Sprite:
                case core::world::particles::ParticleRendererType::SpriteBlend:
                case core::world::particles::ParticleRendererType::Trail:
                case core::world::particles::ParticleRendererType::PointSprite:
                case core::world::particles::ParticleRendererType::Blur:
                {
                    quadCount =
                        systems[
                            systemIndex].Capacity();

                    break;
                }

                case core::world::particles::ParticleRendererType::Amp:
                {
                    const std::size_t particleCapacity =
                        systems[
                            systemIndex].Capacity();

                    if (particleCapacity <
                        2)
                    {
                        break;
                    }

                    const std::size_t segmentCount =
                        emitter.system.renderer.circular
                            ? particleCapacity
                            : particleCapacity -
                                1;

                    const std::size_t stepCount =
                        emitter.system.renderer.steps >
                            0
                            ? static_cast<std::size_t>(
                                emitter.system.renderer.steps)
                            : 1;

                    if (!CheckedMultiply(
                            segmentCount,
                            stepCount,
                            quadCount))
                    {
                        error =
                            "Particle vertex capacity overflow: " +
                            emitter.resource +
                            "/" +
                            emitter.system.name;

                        return false;
                    }

                    break;
                }

                case core::world::particles::ParticleRendererType::None:
                case core::world::particles::ParticleRendererType::Mesh:
                case core::world::particles::ParticleRendererType::Visual:
                case core::world::particles::ParticleRendererType::Unsupported:
                default:
                {
                    continue;
                }
            }

            std::size_t vertexCount =
                0;

            if (!CheckedMultiply(
                    quadCount,
                    VerticesPerQuad,
                    vertexCount) ||
                vertexCount >
                    MaximumVertexCount ||
                !CheckedAdd(
                    output,
                    vertexCount,
                    output) ||
                output >
                    MaximumVertexCount)
            {
                error =
                    "Particle vertex capacity overflow: " +
                    emitter.resource +
                    "/" +
                    emitter.system.name;

                return false;
            }
        }

        return true;
    }

    bool UsesAdditiveBlend(
        const core::world::particles::ParticleRendererDefinition&
            renderer) noexcept
    {
        if (renderer.type ==
                core::world::particles::ParticleRendererType::Trail ||
            renderer.type ==
                core::world::particles::ParticleRendererType::Blur ||
            renderer.type ==
                core::world::particles::ParticleRendererType::Amp)
        {
            return true;
        }

        if (renderer.type ==
                core::world::particles::ParticleRendererType::Sprite ||
            renderer.type ==
                core::world::particles::ParticleRendererType::PointSprite)
        {
            return
                renderer.materialFx ==
                0;
        }

        return false;
    }
}

namespace client::graphics
{
    struct ParticleRenderer::State final
    {
        ComPtr<ID3D11Device>
            device;

        ComPtr<ID3D11VertexShader>
            vertexShader;

        ComPtr<ID3D11PixelShader>
            pixelShader;

        ComPtr<ID3D11InputLayout>
            inputLayout;

        ComPtr<ID3D11Buffer>
            vertexBuffer;

        ComPtr<ID3D11Buffer>
            constantBuffer;

        ComPtr<ID3D11SamplerState>
            sampler;

        ComPtr<ID3D11BlendState>
            additiveBlendState;

        ComPtr<ID3D11BlendState>
            alphaBlendState;

        ComPtr<ID3D11DepthStencilState>
            depthReadState;

        std::vector<ParticleGpuVertex>
            vertices;

        std::vector<ParticleDrawCommand>
            commands;

        std::size_t maxVertexCount =
            0;

        std::size_t lastRenderedParticleCount =
            0;

        std::size_t lastDrawCallCount =
            0;
    };

    ParticleRenderer::ParticleRenderer()
        :
        state_(
            std::make_unique<State>())
    {
    }

    ParticleRenderer::~ParticleRenderer()
    {
        Shutdown();
    }

    bool ParticleRenderer::Initialize(
        ID3D11Device* device,
        std::string& error)
    {
        Shutdown();

        state_ =
            std::make_unique<State>();

        error.clear();

        if (device == nullptr)
        {
            error =
                "Particle renderer received null D3D11 device.";

            return false;
        }

        state_->device =
            device;

        ComPtr<ID3DBlob>
            vertexShaderCode;

        if (!shaders::CompileFromFile(
                L"World\\WorldParticle.hlsl",
                "VSParticle",
                "vs_5_0",
                &vertexShaderCode,
                error))
        {
            return false;
        }

        ComPtr<ID3DBlob>
            pixelShaderCode;

        if (!shaders::CompileFromFile(
                L"World\\WorldParticle.hlsl",
                "PSParticle",
                "ps_5_0",
                &pixelShaderCode,
                error))
        {
            return false;
        }

        HRESULT result =
            device->CreateVertexShader(
                vertexShaderCode->GetBufferPointer(),
                vertexShaderCode->GetBufferSize(),
                nullptr,
                &state_->vertexShader);

        if (FAILED(result))
        {
            error =
                "Unable to create particle vertex shader.";

            return false;
        }

        result =
            device->CreatePixelShader(
                pixelShaderCode->GetBufferPointer(),
                pixelShaderCode->GetBufferSize(),
                nullptr,
                &state_->pixelShader);

        if (FAILED(result))
        {
            error =
                "Unable to create particle pixel shader.";

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
                "COLOR",
                0,
                DXGI_FORMAT_R32G32B32A32_FLOAT,
                0,
                20,
                D3D11_INPUT_PER_VERTEX_DATA,
                0
            }
        };

        result =
            device->CreateInputLayout(
                inputElements,
                3,
                vertexShaderCode->GetBufferPointer(),
                vertexShaderCode->GetBufferSize(),
                &state_->inputLayout);

        if (FAILED(result))
        {
            error =
                "Unable to create particle input layout.";

            return false;
        }

        D3D11_BUFFER_DESC constantDescription{};

        constantDescription.ByteWidth =
            sizeof(ParticleConstants);

        constantDescription.Usage =
            D3D11_USAGE_DEFAULT;

        constantDescription.BindFlags =
            D3D11_BIND_CONSTANT_BUFFER;

        result =
            device->CreateBuffer(
                &constantDescription,
                nullptr,
                &state_->constantBuffer);

        if (FAILED(result))
        {
            error =
                "Unable to create particle constant buffer.";

            return false;
        }

        D3D11_SAMPLER_DESC samplerDescription{};

        samplerDescription.Filter =
            D3D11_FILTER_MIN_MAG_MIP_LINEAR;

        samplerDescription.AddressU =
            D3D11_TEXTURE_ADDRESS_CLAMP;

        samplerDescription.AddressV =
            D3D11_TEXTURE_ADDRESS_CLAMP;

        samplerDescription.AddressW =
            D3D11_TEXTURE_ADDRESS_CLAMP;

        samplerDescription.ComparisonFunc =
            D3D11_COMPARISON_ALWAYS;

        samplerDescription.MinLOD =
            0.0f;

        samplerDescription.MaxLOD =
            D3D11_FLOAT32_MAX;

        result =
            device->CreateSamplerState(
                &samplerDescription,
                &state_->sampler);

        if (FAILED(result))
        {
            error =
                "Unable to create particle sampler.";

            return false;
        }

        D3D11_BLEND_DESC additiveDescription{};

        additiveDescription
            .RenderTarget[0]
            .BlendEnable =
                TRUE;

        additiveDescription
            .RenderTarget[0]
            .SrcBlend =
                D3D11_BLEND_SRC_ALPHA;

        additiveDescription
            .RenderTarget[0]
            .DestBlend =
                D3D11_BLEND_ONE;

        additiveDescription
            .RenderTarget[0]
            .BlendOp =
                D3D11_BLEND_OP_ADD;

        additiveDescription
            .RenderTarget[0]
            .SrcBlendAlpha =
                D3D11_BLEND_ONE;

        additiveDescription
            .RenderTarget[0]
            .DestBlendAlpha =
                D3D11_BLEND_ONE;

        additiveDescription
            .RenderTarget[0]
            .BlendOpAlpha =
                D3D11_BLEND_OP_ADD;

        additiveDescription
            .RenderTarget[0]
            .RenderTargetWriteMask =
                D3D11_COLOR_WRITE_ENABLE_ALL;

        result =
            device->CreateBlendState(
                &additiveDescription,
                &state_->additiveBlendState);

        if (FAILED(result))
        {
            error =
                "Unable to create particle additive blend state.";

            return false;
        }

        D3D11_BLEND_DESC alphaDescription{};

        alphaDescription
            .RenderTarget[0]
            .BlendEnable =
                TRUE;

        alphaDescription
            .RenderTarget[0]
            .SrcBlend =
                D3D11_BLEND_SRC_ALPHA;

        alphaDescription
            .RenderTarget[0]
            .DestBlend =
                D3D11_BLEND_INV_SRC_ALPHA;

        alphaDescription
            .RenderTarget[0]
            .BlendOp =
                D3D11_BLEND_OP_ADD;

        alphaDescription
            .RenderTarget[0]
            .SrcBlendAlpha =
                D3D11_BLEND_ONE;

        alphaDescription
            .RenderTarget[0]
            .DestBlendAlpha =
                D3D11_BLEND_INV_SRC_ALPHA;

        alphaDescription
            .RenderTarget[0]
            .BlendOpAlpha =
                D3D11_BLEND_OP_ADD;

        alphaDescription
            .RenderTarget[0]
            .RenderTargetWriteMask =
                D3D11_COLOR_WRITE_ENABLE_ALL;

        result =
            device->CreateBlendState(
                &alphaDescription,
                &state_->alphaBlendState);

        if (FAILED(result))
        {
            error =
                "Unable to create particle alpha blend state.";

            return false;
        }

        D3D11_DEPTH_STENCIL_DESC depthDescription{};

        depthDescription.DepthEnable =
            TRUE;

        depthDescription.DepthWriteMask =
            D3D11_DEPTH_WRITE_MASK_ZERO;

        depthDescription.DepthFunc =
            D3D11_COMPARISON_LESS_EQUAL;

        depthDescription.StencilEnable =
            FALSE;

        result =
            device->CreateDepthStencilState(
                &depthDescription,
                &state_->depthReadState);

        if (FAILED(result))
        {
            error =
                "Unable to create particle depth state.";

            return false;
        }

        return true;
    }

    bool ParticleRenderer::ConfigureVertexCapacity(
        const std::size_t vertexCount,
        std::string& error)
    {
        error.clear();

        if (!state_ ||
            !state_->device)
        {
            error =
                "Particle renderer is not initialized.";

            return false;
        }

        state_->vertexBuffer.Reset();

        state_->maxVertexCount =
            0;

        if (vertexCount ==
            0)
        {
            return true;
        }

        if (vertexCount >
            std::numeric_limits<UINT>::max() /
                sizeof(ParticleGpuVertex))
        {
            error =
                "Particle vertex buffer is too large.";

            return false;
        }

        const std::size_t byteSize =
            vertexCount *
            sizeof(ParticleGpuVertex);

        D3D11_BUFFER_DESC description{};

        description.ByteWidth =
            static_cast<UINT>(
                byteSize);

        description.Usage =
            D3D11_USAGE_DYNAMIC;

        description.BindFlags =
            D3D11_BIND_VERTEX_BUFFER;

        description.CPUAccessFlags =
            D3D11_CPU_ACCESS_WRITE;

        const HRESULT result =
            state_->device->CreateBuffer(
                &description,
                nullptr,
                &state_->vertexBuffer);

        if (FAILED(result))
        {
            error =
                "Unable to create particle dynamic vertex buffer.";

            return false;
        }

        state_->maxVertexCount =
            vertexCount;

        state_->vertices.reserve(
            vertexCount);

        return true;
    }

    bool ParticleRenderer::SetCapacity(
        const std::size_t particleCapacity,
        std::string& error)
    {
        error.clear();

        if (!state_ ||
            !state_->device)
        {
            error =
                "Particle renderer is not initialized.";

            return false;
        }

        state_->vertices.clear();
        state_->commands.clear();

        constexpr std::size_t VerticesPerParticle =
            6;

        if (particleCapacity >
            std::numeric_limits<std::size_t>::max() /
                VerticesPerParticle)
        {
            error =
                "Particle vertex capacity overflow.";

            return false;
        }

        return
            ConfigureVertexCapacity(
                particleCapacity *
                    VerticesPerParticle,
                error);
    }

    bool ParticleRenderer::Render(
        ID3D11DeviceContext* context,
        const std::vector<
            core::world::particles::ParticleRuntimeSystem>& systems,
        const std::vector<SceneParticleEmitter>& emitters,
        const std::vector<
            ComPtr<ID3D11ShaderResourceView>>& textures,
        const CameraView& camera,
        const DirectX::XMFLOAT4X4& viewProjection,
        std::string& error)
    {
        error.clear();

        if (!state_ ||
            !state_->device ||
            context == nullptr)
        {
            error =
                "Particle renderer is not initialized.";

            return false;
        }

        state_->lastRenderedParticleCount =
            0;

        state_->lastDrawCallCount =
            0;

        if (systems.size() !=
            emitters.size())
        {
            error =
                "Particle runtime/emitter count mismatch.";

            return false;
        }

        std::size_t requiredVertexCapacity =
            0;

        if (!CalculateVertexCapacity(
                systems,
                emitters,
                requiredVertexCapacity,
                error))
        {
            return false;
        }

        if (requiredVertexCapacity >
                state_->maxVertexCount ||
            (requiredVertexCapacity !=
                 0 &&
             !state_->vertexBuffer))
        {
            if (!ConfigureVertexCapacity(
                    requiredVertexCapacity,
                    error))
            {
                return false;
            }
        }

        if (systems.empty() ||
            requiredVertexCapacity ==
                0 ||
            !state_->vertexBuffer)
        {
            return true;
        }

        state_->vertices.clear();
        state_->commands.clear();

        DirectX::XMFLOAT3 cameraForward
        {
            camera.forward.x,
            camera.forward.y,
            camera.forward.z
        };

        DirectX::XMFLOAT3 cameraUp
        {
            camera.up.x,
            camera.up.y,
            camera.up.z
        };

        DirectX::XMFLOAT3 cameraPosition
        {
            camera.position.x,
            camera.position.y,
            camera.position.z
        };

        cameraForward =
            Normalize(
                cameraForward);

        cameraUp =
            Normalize(
                cameraUp);

        DirectX::XMFLOAT3 cameraRight =
            Normalize(
                Cross(
                    cameraUp,
                    cameraForward));

        if (LengthSquared(
                cameraRight) <=
            0.000001f)
        {
            cameraRight =
            {
                1.0f,
                0.0f,
                0.0f
            };
        }

        for (std::size_t systemIndex = 0;
             systemIndex < systems.size();
             ++systemIndex)
        {
            const SceneParticleEmitter& emitter =
                emitters[
                    systemIndex];

            if (!emitter.renderable ||
                !emitter.system.hasRenderer)
            {
                continue;
            }

            const auto rendererType =
                emitter.system.renderer.type;

            switch (rendererType)
            {
                case core::world::particles::ParticleRendererType::Sprite:
                case core::world::particles::ParticleRendererType::SpriteBlend:
                case core::world::particles::ParticleRendererType::Trail:
                case core::world::particles::ParticleRendererType::PointSprite:
                case core::world::particles::ParticleRendererType::Blur:
                case core::world::particles::ParticleRendererType::Amp:
                {
                    break;
                }

                case core::world::particles::ParticleRendererType::None:
                case core::world::particles::ParticleRendererType::Mesh:
                case core::world::particles::ParticleRendererType::Visual:
                case core::world::particles::ParticleRendererType::Unsupported:
                default:
                {
                    continue;
                }
            }

            std::int32_t selectedTextureIndex =
                emitter.textureIndex;

            if (emitter.animatedTexture)
            {
                if (emitter.textureFrameIndices.empty() ||
                    emitter.textureAnimationFps <=
                        0.0f)
                {
                    continue;
                }

                const float animationTime =
                    std::max(
                        systems[
                            systemIndex]
                            .Age(),
                        0.0f);

                const std::size_t frameIndex =
                    static_cast<std::size_t>(
                        std::floor(
                            animationTime *
                            emitter.textureAnimationFps)) %
                    emitter.textureFrameIndices.size();

                selectedTextureIndex =
                    emitter.textureFrameIndices[
                        frameIndex];
            }

            if (selectedTextureIndex <
                0)
            {
                continue;
            }

            const std::size_t textureIndex =
                static_cast<std::size_t>(
                    selectedTextureIndex);

            if (textureIndex >=
                textures.size())
            {
                error =
                    "Particle emitter references invalid texture.";

                return false;
            }

            const auto& particles =
                systems[
                    systemIndex]
                    .Particles();

            if (particles.empty())
            {
                continue;
            }

            if (state_->vertices.size() >
                static_cast<std::size_t>(
                    std::numeric_limits<std::uint32_t>::max()))
            {
                error =
                    "Particle first vertex exceeds uint32 range.";

                return false;
            }

            const std::uint32_t firstVertex =
                static_cast<std::uint32_t>(
                    state_->vertices.size());

            if (rendererType ==
                core::world::particles::ParticleRendererType::Amp)
            {
                AppendAmp(
                    state_->vertices,
                    particles,
                    emitter.system.renderer,
                    cameraPosition,
                    cameraRight,
                    cameraUp,
                    systemIndex);
            }
            else
            {
                for (const auto& particle :
                     particles)
                {
                    switch (rendererType)
                    {
                        case core::world::particles::ParticleRendererType::Trail:
                        {
                            AppendTrail(
                                state_->vertices,
                                particle,
                                emitter.system.renderer,
                                cameraPosition,
                                cameraRight);

                            break;
                        }

                        case core::world::particles::ParticleRendererType::Blur:
                        {
                            AppendBlur(
                                state_->vertices,
                                particle,
                                emitter.system.renderer,
                                cameraPosition,
                                cameraRight);

                            break;
                        }

                        case core::world::particles::ParticleRendererType::Sprite:
                        case core::world::particles::ParticleRendererType::SpriteBlend:
                        case core::world::particles::ParticleRendererType::PointSprite:
                        {
                            AppendBillboard(
                                state_->vertices,
                                particle,
                                cameraRight,
                                cameraUp);

                            break;
                        }

                        case core::world::particles::ParticleRendererType::None:
                        case core::world::particles::ParticleRendererType::Amp:
                        case core::world::particles::ParticleRendererType::Mesh:
                        case core::world::particles::ParticleRendererType::Visual:
                        case core::world::particles::ParticleRendererType::Unsupported:
                        default:
                        {
                            break;
                        }
                    }
                }
            }

            const std::size_t generatedVertices =
                state_->vertices.size() -
                firstVertex;

            if (generatedVertices ==
                0)
            {
                continue;
            }

            if (generatedVertices >
                std::numeric_limits<std::uint32_t>::max())
            {
                error =
                    "Particle draw range exceeds uint32 range.";

                return false;
            }

            ParticleDrawCommand
                command;

            command.firstVertex =
                firstVertex;

            command.vertexCount =
                static_cast<std::uint32_t>(
                    generatedVertices);

            command.textureIndex =
                textureIndex;

            command.additive =
                UsesAdditiveBlend(
                    emitter.system.renderer);

            state_->commands.push_back(
                command);

            state_->lastRenderedParticleCount +=
                rendererType ==
                    core::world::particles::ParticleRendererType::Amp
                    ? particles.size()
                    : generatedVertices /
                        6u;
        }

        if (state_->vertices.empty())
        {
            return true;
        }

        if (state_->vertices.size() >
            state_->maxVertexCount)
        {
            error =
                "Active particles exceed particle GPU buffer capacity.";

            return false;
        }

        D3D11_MAPPED_SUBRESOURCE
            mapped{};

        HRESULT result =
            context->Map(
                state_->vertexBuffer.Get(),
                0,
                D3D11_MAP_WRITE_DISCARD,
                0,
                &mapped);

        if (FAILED(result) ||
            mapped.pData == nullptr)
        {
            error =
                "Unable to map particle vertex buffer.";

            return false;
        }

        std::memcpy(
            mapped.pData,
            state_->vertices.data(),
            state_->vertices.size() *
                sizeof(ParticleGpuVertex));

        context->Unmap(
            state_->vertexBuffer.Get(),
            0);

        ParticleConstants
            constants{};

        constants.viewProjection =
            viewProjection;

        context->UpdateSubresource(
            state_->constantBuffer.Get(),
            0,
            nullptr,
            &constants,
            0,
            0);

        const UINT stride =
            sizeof(ParticleGpuVertex);

        const UINT offset =
            0;

        ID3D11Buffer*
            vertexBuffers[] =
        {
            state_->vertexBuffer.Get()
        };

        context->IASetInputLayout(
            state_->inputLayout.Get());

        context->IASetVertexBuffers(
            0,
            1,
            vertexBuffers,
            &stride,
            &offset);

        context->IASetIndexBuffer(
            nullptr,
            DXGI_FORMAT_UNKNOWN,
            0);

        context->IASetPrimitiveTopology(
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        context->VSSetShader(
            state_->vertexShader.Get(),
            nullptr,
            0);

        context->PSSetShader(
            state_->pixelShader.Get(),
            nullptr,
            0);

        ID3D11Buffer*
            constantBuffers[] =
        {
            state_->constantBuffer.Get()
        };

        context->VSSetConstantBuffers(
            0,
            1,
            constantBuffers);

        ID3D11SamplerState*
            samplers[] =
        {
            state_->sampler.Get()
        };

        context->PSSetSamplers(
            0,
            1,
            samplers);

        context->OMSetDepthStencilState(
            state_->depthReadState.Get(),
            0);

        constexpr float BlendFactor[4]
        {
            0.0f,
            0.0f,
            0.0f,
            0.0f
        };

        for (const ParticleDrawCommand& command :
             state_->commands)
        {
            ID3D11ShaderResourceView*
                texture =
                    textures[
                        command.textureIndex]
                        .Get();

            context->PSSetShaderResources(
                0,
                1,
                &texture);

            context->OMSetBlendState(
                command.additive
                    ? state_->additiveBlendState.Get()
                    : state_->alphaBlendState.Get(),
                BlendFactor,
                0xFFFFFFFFu);

            context->Draw(
                command.vertexCount,
                command.firstVertex);

            ++state_->lastDrawCallCount;
        }

        ID3D11ShaderResourceView*
            emptyTexture =
                nullptr;

        context->PSSetShaderResources(
            0,
            1,
            &emptyTexture);

        context->OMSetBlendState(
            nullptr,
            nullptr,
            0xFFFFFFFFu);

        return true;
    }

    std::size_t
    ParticleRenderer::LastRenderedParticleCount() const noexcept
    {
        if (!state_)
        {
            return 0;
        }

        return
            state_->lastRenderedParticleCount;
    }

    std::size_t
    ParticleRenderer::LastDrawCallCount() const noexcept
    {
        if (!state_)
        {
            return 0;
        }

        return
            state_->lastDrawCallCount;
    }

    void ParticleRenderer::Shutdown() noexcept
    {
        state_.reset();
    }
}
