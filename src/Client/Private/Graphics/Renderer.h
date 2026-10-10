#pragma once

#include "Graphics/CameraView.h"
#include "Graphics/SceneRenderData.h"

#if defined(STUDIO_BUILD)
#include "Graphics/XRayHomOcclusion.h"
#endif

#include "Core/Images/RgbaImage.h"
#include "Core/Math/Vector3.h"
#include "Core/World/Particles/ParticleCollisionQuery.h"

#include <Windows.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11ShaderResourceView;

namespace client::graphics
{
    class Renderer final
    {
    public:
        Renderer();
        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        [[nodiscard]]
        bool Initialize(
            HWND window,
            std::uint32_t width,
            std::uint32_t height,
            std::string& error);

        [[nodiscard]]
        bool Resize(
            std::uint32_t width,
            std::uint32_t height,
            std::string& error);

        [[nodiscard]]
        bool SetBackgroundImage(
            const core::images::RgbaImage& image,
            std::string& error);

        [[nodiscard]]
        bool SetScene(
            const SceneRenderData& scene,
            const core::world::particles::ParticleCollisionQuery* particleCollision,
            std::string& error);

        [[nodiscard]]
        bool BeginStreamedScene(
            std::string& error);

        [[nodiscard]]
        bool AppendStreamedTexture(
            const SceneTextureData& texture,
            std::int32_t& outputTextureIndex,
            std::string& error);

        [[nodiscard]]
        bool AppendStreamedMesh(
            const SceneMesh& mesh,
            std::string& error);

        [[nodiscard]]
        bool FinishStreamedScene(
            std::string& error);

#if defined(STUDIO_BUILD)
        void SetParticleCollisionQuery(
            std::shared_ptr<const core::world::particles::ParticleCollisionQuery> collision);

        void SetHomOccluders(
            const std::vector<core::world::xray::HomTriangle>& triangles);

        void SetStudioLighting(
            const StudioLightingData& lighting);

        [[nodiscard]]
        HomOcclusionStatistics HomStatistics() const noexcept;
#endif

        [[nodiscard]]
        bool UpdateMeshVertices(
            std::size_t meshIndex,
            const core::assets::MeshData& mesh,
            std::string& error);

        [[nodiscard]]
        bool SetInstanceTransformRange(
            std::size_t firstInstance,
            std::size_t instanceCount,
            const core::math::Transform3x4& transform) noexcept;

        void SetCamera(
            const CameraView& camera) noexcept;

        [[nodiscard]]
        core::math::Vector3 SceneCenter() const noexcept;

        [[nodiscard]]
        float SceneRadius() const noexcept;

        [[nodiscard]]
        bool Render(
            std::string& error);

        void Shutdown();

        using FrameOverlayCallback = void (*)(void* userData);

        [[nodiscard]]
        ID3D11Device* Device() const noexcept;

        [[nodiscard]]
        ID3D11DeviceContext* Context() const noexcept;

        [[nodiscard]]
        ID3D11ShaderResourceView*
        ViewportImage() const noexcept;

        void SetFrameOverlay(
            FrameOverlayCallback callback,
            void* userData) noexcept;

    private:
        [[nodiscard]]
        bool PresentFrame(
            std::string& error);
        
        struct State;

        std::unique_ptr<State>
            state_;

        [[nodiscard]]
        bool CreateFrameTargets(
            std::uint32_t width,
            std::uint32_t height,
            std::string& error);
    };
}
