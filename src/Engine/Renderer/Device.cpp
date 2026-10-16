
#include "Engine/Renderer/Device.h"

#include <stdexcept>

namespace engine::renderer {

namespace {

void check(HRESULT result, const char* message) {
    if (FAILED(result)) {
        throw std::runtime_error(message);
    }
}

}

Device::Device(HWND window) {
    if (!window) {
        throw std::invalid_argument("Invalid window handle");
    }

    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferCount = 2;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = window;
    desc.SampleDesc.Count = 1;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    constexpr D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_0
    };

    auto create = [&](D3D_DRIVER_TYPE type) {
        return D3D11CreateDeviceAndSwapChain(
            nullptr,
            type,
            nullptr,
            0,
            levels,
            1,
            D3D11_SDK_VERSION,
            &desc,
            swapChain_.GetAddressOf(),
            device_.GetAddressOf(),
            nullptr,
            context_.GetAddressOf()
        );
    };

    HRESULT result = create(D3D_DRIVER_TYPE_HARDWARE);

    if (FAILED(result)) {
        swapChain_.Reset();
        device_.Reset();
        context_.Reset();

        result = create(D3D_DRIVER_TYPE_WARP);
    }

    check(result, "Failed to initialize DirectX 11");

    Microsoft::WRL::ComPtr<IDXGIFactory> factory;

    if (SUCCEEDED(swapChain_->GetParent(
        IID_PPV_ARGS(factory.GetAddressOf())
    ))) {
        factory->MakeWindowAssociation(
            window,
            DXGI_MWA_NO_ALT_ENTER
        );
    }

    RECT rect{};
    GetClientRect(window, &rect);

    width_ = static_cast<UINT>(rect.right - rect.left);
    height_ = static_cast<UINT>(rect.bottom - rect.top);

    create_target();
}

void Device::create_target() {
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;

    check(
        swapChain_->GetBuffer(
            0,
            IID_PPV_ARGS(backBuffer.GetAddressOf())
        ),
        "Failed to obtain back buffer"
    );

    check(
        device_->CreateRenderTargetView(
            backBuffer.Get(),
            nullptr,
            target_.GetAddressOf()
        ),
        "Failed to create render target"
    );
}

void Device::resize(UINT width, UINT height) {
    if (width == 0 || height == 0) {
        return;
    }

    if (width == width_ && height == height_) {
        return;
    }

    context_->OMSetRenderTargets(0, nullptr, nullptr);
    context_->ClearState();

    target_.Reset();

    check(
        swapChain_->ResizeBuffers(
            0,
            width,
            height,
            DXGI_FORMAT_UNKNOWN,
            0
        ),
        "Failed to resize swap chain"
    );

    width_ = width;
    height_ = height;

    create_target();
}

void Device::begin_frame() {
    constexpr float clearColor[4] = {
        0.055f, 0.060f, 0.070f, 1.0f
    };

    ID3D11RenderTargetView* target = target_.Get();

    context_->OMSetRenderTargets(
        1,
        &target,
        nullptr
    );

    context_->ClearRenderTargetView(
        target,
        clearColor
    );

    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(width_);
    viewport.Height = static_cast<float>(height_);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    context_->RSSetViewports(1, &viewport);
}

void Device::present() {
    check(
        swapChain_->Present(1, 0),
        "Failed to present frame"
    );
}

}
