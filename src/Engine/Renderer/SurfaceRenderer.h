#pragma once

#include "Engine/Renderer/SurfaceMesh.h"

#include <DirectXMath.h>
#include <d3d11.h>
#include <wrl/client.h>

#include <chrono>
#include <cstdint>
#include <filesystem>

namespace engine::renderer {

    struct SurfaceMaterial {
        std::filesystem::path diffuse;
        std::filesystem::path normal;
        DirectX::XMFLOAT3 tint{1.0f, 1.0f, 1.0f};
        bool animatedWater = false;
    };

    class SurfaceRenderer final {
    public:
        SurfaceRenderer(ID3D11Device* device, ID3D11DeviceContext* context);

        void set_mesh(const SurfaceMesh& mesh, const SurfaceMaterial& material);
        void clear() noexcept;
        void set_visible(bool visible) noexcept;

        [[nodiscard]] std::uint32_t render(DirectX::FXMMATRIX viewProjection);

    private:
        ID3D11Device* device_ = nullptr;
        ID3D11DeviceContext* context_ = nullptr;

        Microsoft::WRL::ComPtr<ID3D11Buffer> vertices_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> indices_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> cameraConstants_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> materialConstants_;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader_;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader_;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout_;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizer_;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> diffuse_;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normal_;

        std::chrono::steady_clock::time_point startTime_ =
            std::chrono::steady_clock::now();
        DirectX::XMFLOAT3 tint_{1.0f, 1.0f, 1.0f};
        std::uint32_t indexCount_ = 0;
        bool animatedWater_ = false;
        bool visible_ = true;
    };

}
