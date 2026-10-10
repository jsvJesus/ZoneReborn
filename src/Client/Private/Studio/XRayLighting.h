#pragma once

#include "Graphics/SceneRenderData.h"
#include <array>
#include <string>
#include <vector>

namespace core::world::xray { struct LevelData; }

namespace studio
{
    struct XRayWeatherKey final
    {
        float seconds = 0.0f;
        std::array<float, 3> ambient{0.05f, 0.05f, 0.05f};
        std::array<float, 3> hemisphere{0.5f, 0.5f, 0.5f};
        std::array<float, 3> sunColour{0.9f, 0.84f, 0.69f};
        std::array<float, 3> sunDirection{0.0f, -1.0f, 0.0f}; // Light ray direction.
    };

    struct XRayWeather final
    {
        std::string name;
        std::vector<XRayWeatherKey> keys;
    };

    struct XRayLighting final
    {
        std::vector<XRayWeather> weather;
        std::size_t defaultWeather = 0;
        std::vector<client::graphics::SceneOmniLight> lamps;
        std::vector<client::graphics::SceneSpotLight> lampSpots;
        std::vector<client::graphics::SceneOmniLight> staticLights;
        std::vector<client::graphics::SceneSpotLight> staticSpots;
        std::vector<std::string> warnings;
        std::string source;
    };

    XRayLighting LoadXRayLighting(const core::world::xray::LevelData& level);
    XRayWeatherKey EvaluateXRayWeather(const XRayWeather* weather, float hours);
}
