#pragma once

#include "Core/Math/Vector3.h"

namespace client::graphics
{
    struct CameraView final
    {
        core::math::Vector3 position
        {
            0.0f,
            0.0f,
            0.0f
        };

        core::math::Vector3 forward
        {
            0.0f,
            0.0f,
            1.0f
        };

        core::math::Vector3 up
        {
            0.0f,
            1.0f,
            0.0f
        };

        float fieldOfViewDegrees =
            60.0f;
    };
}