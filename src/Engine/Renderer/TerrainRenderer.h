
#pragma once

#include <DirectXMath.h>
#include <d3d11.h>
#include <wrl/client.h>

#include <cstdint>
#include <span>

namespace engine::renderer {

    struct TerrainMeshData {
        std::uint32_t samplesX = 0;
        std::uint32_t samplesZ = 0;

        float originX = 0.0f;
        float originZ = 0.0f;
        float spacingMeters = 1.0f;

        std::span<const float> heights;
    };

    class TerrainRenderer final {
    public:
        TerrainRenderer(
            ID3D11Device* device,
            ID3D11DeviceContext* context
        );

        TerrainRenderer(const TerrainRenderer&) = delete;
        TerrainRenderer& operator=(const TerrainRenderer&) = delete;

        void set_mesh(const TerrainMeshData& data);
        void resize(UINT width, UINT height);

        void render(const DirectX::XMMATRIX& viewProjection);

        void set_wireframe(bool enabled) noexcept {
            wireframe_ = enabled;
        }

        [[nodiscard]] ID3D11ShaderResourceView*
            texture() const noexcept {
            return shaderResource_.Get();
        }

    private:
        void create_pipeline();

        ID3D11Device* device_ = nullptr;
        ID3D11DeviceContext* context_ = nullptr;

        Microsoft::WRL::ComPtr<ID3D11Buffer> vertices_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> indices_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> constants_;

        Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader_;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader_;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout_;

        Microsoft::WRL::ComPtr<ID3D11RasterizerState> solidState_;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> wireState_;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> colorTexture_;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTarget_;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResource_;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> depthTexture_;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthView_;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthState_;

        UINT indexCount_ = 0;
        UINT width_ = 0;
        UINT height_ = 0;
        bool wireframe_ = false;
    };

}
