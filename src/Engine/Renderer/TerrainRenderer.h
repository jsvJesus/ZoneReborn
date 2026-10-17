
#pragma once

#include "Engine/Renderer/Frustum.h"
#include "Engine/Renderer/TerrainChunkMesh.h"
#include "Engine/Renderer/TerrainMaterials.h"

#include <DirectXMath.h>
#include <d3d11.h>
#include <wrl/client.h>

#include <filesystem>
#include <array>
#include <cstdint>
#include <vector>

namespace engine::renderer {

    struct TerrainRenderStats {
        std::uint32_t chunks = 0;
        std::uint32_t visibleChunks = 0;
        std::uint32_t culledChunks = 0;
        std::uint32_t drawCalls = 0;

        std::uint64_t triangles = 0;

        std::array<std::uint32_t, TerrainLodCount> lodCounts{};
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
        void clear_chunks() noexcept;

        void add_stream_chunk(
            std::uint32_t chunkX,
            std::uint32_t chunkZ,
            const TerrainMeshData& data
        );

        void remove_stream_chunk(
            std::uint32_t chunkX,
            std::uint32_t chunkZ
        );
        
        void resize(UINT width, UINT height);

        void render(
            const DirectX::XMMATRIX& viewProjection,
            const DirectX::XMFLOAT3& cameraPosition
        );

        void set_wireframe(bool enabled) noexcept {
            wireframe_ = enabled;
        }

        void set_auto_lod(bool enabled) noexcept {
            autoLod_ = enabled;
        }

        void set_frustum_culling(bool enabled) noexcept {
            frustumCulling_ = enabled;
        }

        void set_fixed_lod(std::uint32_t level) noexcept {
            fixedLod_ = level < TerrainLodCount
                ? level
                : TerrainLodCount - 1;
        }

        void set_lod_distance(float scale) noexcept {
            lodDistanceScale_ = scale > 0.1f ? scale : 0.1f;
        }

        [[nodiscard]] const TerrainRenderStats& stats() const noexcept {
            return stats_;
        }

        [[nodiscard]] ID3D11ShaderResourceView* texture() const noexcept {
            return shaderResource_.Get();
        }

        void set_terrain_materials(
            const TerrainMaterialMask& mask,
            const std::array<TerrainMaterialPaths, 5>& paths
        ) {
            materials_.load(mask, paths);
        }

        void clear_terrain_materials() noexcept {
            materials_.clear();
        }

        [[nodiscard]] bool has_terrain_materials() const noexcept {
            return materials_.ready();
        }

    private:
        struct ChunkLevel {
            Microsoft::WRL::ComPtr<ID3D11Buffer> vertices;
            Microsoft::WRL::ComPtr<ID3D11Buffer> indices;

            UINT indexCount = 0;
        };

        struct Chunk {
            std::uint32_t gridX = 0;
            std::uint32_t gridZ = 0;

            Bounds3 bounds{};
            std::array<ChunkLevel, TerrainLodCount> levels;
        };

        void create_pipeline();

        [[nodiscard]] std::uint32_t select_lod(
            const Chunk& chunk,
            const DirectX::XMFLOAT3& cameraPosition
        ) const noexcept;

        ID3D11Device* device_ = nullptr;
        ID3D11DeviceContext* context_ = nullptr;

        std::vector<Chunk> chunks_;
        TerrainRenderStats stats_{};

        float chunkWorldSize_ = 512.0f;
        float lodDistanceScale_ = 1.0f;

        bool autoLod_ = true;
        bool wireframe_ = false;
        bool frustumCulling_ = true;

        std::uint32_t fixedLod_ = 0;

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

        UINT width_ = 0;
        UINT height_ = 0;

        TerrainMaterials materials_;
    };

}
