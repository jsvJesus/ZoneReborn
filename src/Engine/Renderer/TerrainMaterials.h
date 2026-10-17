#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <span>

namespace engine::renderer {

    struct TerrainMaterialMask {
        std::uint32_t samplesX = 0;
        std::uint32_t samplesZ = 0;

        float originX = 0.0f;
        float originZ = 0.0f;
        float spacingMeters = 0.0f;
        float textureScaleMeters = 6.0f;

        std::span<const std::uint8_t> weights;
    };

    class TerrainMaterials final {
    public:
        TerrainMaterials(
            ID3D11Device* device,
            ID3D11DeviceContext* context
        );

        void load(
            const TerrainMaterialMask& mask,
            const std::array<std::filesystem::path, 5>& paths
        );

        void clear() noexcept;

        [[nodiscard]] bool ready() const noexcept;

        void bind() const;
        void unbind() const noexcept;

    private:
        void create_pipeline();

        [[nodiscard]]
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>
        load_texture(const std::filesystem::path& path) const;

        ID3D11Device* device_ = nullptr;
        ID3D11DeviceContext* context_ = nullptr;

        Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader_;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader_;

        Microsoft::WRL::ComPtr<ID3D11Buffer> constants_;

        Microsoft::WRL::ComPtr<ID3D11SamplerState> tileSampler_;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> maskSampler_;

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mask_;

        std::array<
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>,
            5
        > textures_;
    };

}
