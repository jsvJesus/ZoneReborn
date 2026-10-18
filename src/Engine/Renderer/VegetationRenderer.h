#pragma once

#include <DirectXMath.h>
#include <d3d11.h>
#include <wrl/client.h>

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace engine::renderer {

    struct VegetationDrawInstance {
        std::uint32_t assetIndex = 0;

        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

        float yawRadians = 0.0f;
        float scale = 1.0f;

        std::uint32_t kind = 0;
    };

    struct VegetationRenderStats {
        std::uint32_t loadedVariants = 0;
        std::uint32_t failedVariants = 0;

        std::uint32_t loadedMaterialGroups = 0;
        std::uint32_t loadedTextures = 0;

        std::uint32_t renderedInstances = 0;
        std::uint32_t drawCalls = 0;

        std::vector<std::string> failedPaths;
    };

    class VegetationRenderer final {
    public:
        VegetationRenderer(
            ID3D11Device* device,
            ID3D11DeviceContext* context
        );

        void set_scene(
            const std::filesystem::path& packsDirectory,
            std::span<const std::string> assetPaths,
            std::span<const VegetationDrawInstance> instances
        );

        void clear() noexcept;

        void set_visible(bool value) noexcept {
            visible_ = value;
        }

        [[nodiscard]] bool visible() const noexcept {
            return visible_;
        }

        [[nodiscard]] const VegetationRenderStats&
        stats() const noexcept {
            return stats_;
        }

        void render(
            DirectX::FXMMATRIX viewProjection
        );

    private:
        struct MaterialGroup {
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>
                diffuse;

            UINT startIndex = 0;
            UINT indexCount = 0;

            float alphaReference = 0.0f;

            bool alphaTest = false;
            bool doubleSided = false;
        };

        struct Batch {
            Microsoft::WRL::ComPtr<ID3D11Buffer> vertices;
            Microsoft::WRL::ComPtr<ID3D11Buffer> indices;
            Microsoft::WRL::ComPtr<ID3D11Buffer> instances;

            std::vector<MaterialGroup> materials;

            UINT instanceCount = 0;
        };

        void create_pipeline();

        ID3D11Device* device_ = nullptr;
        ID3D11DeviceContext* context_ = nullptr;

        Microsoft::WRL::ComPtr<ID3D11VertexShader>
            vertexShader_;

        Microsoft::WRL::ComPtr<ID3D11PixelShader>
            pixelShader_;

        Microsoft::WRL::ComPtr<ID3D11InputLayout>
            inputLayout_;

        Microsoft::WRL::ComPtr<ID3D11Buffer>
            cameraConstants_;

        Microsoft::WRL::ComPtr<ID3D11Buffer>
            materialConstants_;

        Microsoft::WRL::ComPtr<ID3D11SamplerState>
            textureSampler_;

        Microsoft::WRL::ComPtr<ID3D11RasterizerState>
            singleSidedState_;

        Microsoft::WRL::ComPtr<ID3D11RasterizerState>
            doubleSidedState_;

        std::vector<Batch> batches_;

        VegetationRenderStats stats_{};

        bool visible_ = false;
    };

}
