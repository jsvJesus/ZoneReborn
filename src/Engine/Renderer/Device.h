#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

namespace engine::renderer {

    class Device final {
    public:
        explicit Device(HWND window);

        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;

        void resize(UINT width, UINT height);
        void begin_frame();
        void present();

        [[nodiscard]] ID3D11Device* device() const noexcept {
            return device_.Get();
        }

        [[nodiscard]] ID3D11DeviceContext* context() const noexcept {
            return context_.Get();
        }

    private:
        void create_target();

        Microsoft::WRL::ComPtr<ID3D11Device> device_;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
        Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> target_;

        UINT width_ = 0;
        UINT height_ = 0;
    };

}
