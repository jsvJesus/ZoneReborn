#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include <filesystem>

namespace engine::assets {

    [[nodiscard]]
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>
    load_dds_texture(
        ID3D11Device* device,
        const std::filesystem::path& path
    );

}
