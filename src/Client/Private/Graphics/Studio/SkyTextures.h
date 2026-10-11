#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace client::graphics
{
    class StudioSkyTextures final
    {
    public:
        void SetRoot(
            const std::filesystem::path& root);

        void Clear() noexcept;

        [[nodiscard]]
        ID3D11ShaderResourceView* Get(
            ID3D11Device* device,
            const std::string& logicalName,
            bool cubemap);

    private:
        struct Entry final
        {
            Microsoft::WRL::ComPtr<
                ID3D11ShaderResourceView> view;

            std::uint64_t lastUsed = 0;
        };

        [[nodiscard]]
        bool Load(
            ID3D11Device* device,
            const std::filesystem::path& path,
            bool cubemap,
            Microsoft::WRL::ComPtr<
                ID3D11ShaderResourceView>& output,
            std::string& error) const;

        void TrimCache();

        std::filesystem::path root_;

        std::unordered_map<
            std::string,
            Entry> entries_;

        std::uint64_t sequence_ = 0;
    };
}