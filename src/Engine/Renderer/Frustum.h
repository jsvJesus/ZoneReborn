
#pragma once

#include <DirectXMath.h>

#include <array>

namespace engine::renderer {

    struct Bounds3 {
        DirectX::XMFLOAT3 minimum{};
        DirectX::XMFLOAT3 maximum{};
    };

    class Frustum final {
    public:
        explicit Frustum(DirectX::FXMMATRIX viewProjection) noexcept;

        [[nodiscard]] bool intersects(const Bounds3& bounds) const noexcept;

    private:
        struct Plane {
            float a = 0.0f;
            float b = 0.0f;
            float c = 0.0f;
            float d = 0.0f;
        };

        [[nodiscard]] static Plane normalize(
            float a,
            float b,
            float c,
            float d
        ) noexcept;

        std::array<Plane, 6> planes_{};
    };

}
