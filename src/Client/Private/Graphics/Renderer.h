#pragma once

#include "Graphics/CameraView.h"
#include "Graphics/SceneRenderData.h"

#include "Core/Images/RgbaImage.h"
#include "Core/Math/Vector3.h"

#include <Windows.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>


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
        bool SetBackgroundImage(
            const core::images::RgbaImage& image,
            std::string& error);

        [[nodiscard]]
        bool SetScene(
            const SceneRenderData& scene,
            std::string& error);

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

    private:
        struct State;

        std::unique_ptr<State>
            state_;
    };
}