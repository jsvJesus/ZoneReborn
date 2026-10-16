
#include "Engine/Renderer/Frustum.h"

#include <cmath>

namespace engine::renderer {

    Frustum::Plane Frustum::normalize(
        float a,
        float b,
        float c,
        float d
    ) noexcept {
        const float length = std::sqrt(a * a + b * b + c * c);

        if (!std::isfinite(length) || length < 0.000001f) {
            return {0.0f, 0.0f, 0.0f, 1.0f};
        }

        const float inverseLength = 1.0f / length;

        return {
            a * inverseLength,
            b * inverseLength,
            c * inverseLength,
            d * inverseLength
        };
    }

    Frustum::Frustum(DirectX::FXMMATRIX viewProjection) noexcept {
        DirectX::XMFLOAT4X4 matrix{};
        DirectX::XMStoreFloat4x4(&matrix, viewProjection);

        // Left.
        planes_[0] = normalize(
            matrix._14 + matrix._11,
            matrix._24 + matrix._21,
            matrix._34 + matrix._31,
            matrix._44 + matrix._41
        );

        // Right.
        planes_[1] = normalize(
            matrix._14 - matrix._11,
            matrix._24 - matrix._21,
            matrix._34 - matrix._31,
            matrix._44 - matrix._41
        );

        // Bottom.
        planes_[2] = normalize(
            matrix._14 + matrix._12,
            matrix._24 + matrix._22,
            matrix._34 + matrix._32,
            matrix._44 + matrix._42
        );

        // Top.
        planes_[3] = normalize(
            matrix._14 - matrix._12,
            matrix._24 - matrix._22,
            matrix._34 - matrix._32,
            matrix._44 - matrix._42
        );

        // Near.
        planes_[4] = normalize(
            matrix._13,
            matrix._23,
            matrix._33,
            matrix._43
        );

        // Far.
        planes_[5] = normalize(
            matrix._14 - matrix._13,
            matrix._24 - matrix._23,
            matrix._34 - matrix._33,
            matrix._44 - matrix._43
        );
    }

    bool Frustum::intersects(const Bounds3& bounds) const noexcept {
        const float centerX = (bounds.minimum.x + bounds.maximum.x) * 0.5f;
        const float centerY = (bounds.minimum.y + bounds.maximum.y) * 0.5f;
        const float centerZ = (bounds.minimum.z + bounds.maximum.z) * 0.5f;

        const float extentX = (bounds.maximum.x - bounds.minimum.x) * 0.5f;
        const float extentY = (bounds.maximum.y - bounds.minimum.y) * 0.5f;
        const float extentZ = (bounds.maximum.z - bounds.minimum.z) * 0.5f;

        constexpr float tolerance = 0.01f;

        for (const Plane& plane : planes_) {
            const float distance =
                plane.a * centerX +
                plane.b * centerY +
                plane.c * centerZ +
                plane.d;

            const float radius =
                std::abs(plane.a) * extentX +
                std::abs(plane.b) * extentY +
                std::abs(plane.c) * extentZ;

            if (distance + radius < -tolerance) {
                return false;
            }
        }

        return true;
    }

}
