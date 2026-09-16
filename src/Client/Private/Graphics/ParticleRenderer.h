#pragma once

#include "Graphics/CameraView.h"
#include "Graphics/SceneRenderData.h"

#include "Core/World/Particles/ParticleRuntime.h"

#include <DirectXMath.h>

#include <d3d11.h>
#include <wrl/client.h>

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace client::graphics
{
    class ParticleRenderer final
    {
    public:
        ParticleRenderer();
        ~ParticleRenderer();

        ParticleRenderer(
            const ParticleRenderer&) = delete;

        ParticleRenderer&
        operator=(
            const ParticleRenderer&) = delete;

        [[nodiscard]]
        bool Initialize(
            ID3D11Device* device,
            std::string& error);

        [[nodiscard]]
        bool SetCapacity(
            std::size_t particleCapacity,
            std::string& error);

        [[nodiscard]]
        bool Render(
            ID3D11DeviceContext* context,
            const std::vector<
                core::world::particles::ParticleRuntimeSystem>& systems,
            const std::vector<SceneParticleEmitter>& emitters,
            const std::vector<
                Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>& textures,
            const CameraView& camera,
            const DirectX::XMFLOAT4X4& viewProjection,
            std::string& error);

        [[nodiscard]]
        std::size_t
        LastRenderedParticleCount() const noexcept;

        [[nodiscard]]
        std::size_t
        LastDrawCallCount() const noexcept;

        void Shutdown() noexcept;

    private:
        struct State;

        std::unique_ptr<State>
            state_;
    };
}