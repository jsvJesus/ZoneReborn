#pragma once

#include "Graphics/SceneRenderData.h"
#include "Studio/BuildLightsLoader.h"

#include <array>
#include <string>
#include <vector>

namespace core::world::xray { struct LevelData; }

namespace studio
{
    struct XRayWeatherKey final
    {
        float seconds = 0.0f;

        std::array<float, 3> ambient
        {
            0.05f, 0.05f, 0.05f
        };

        std::array<float, 3> hemisphere
        {
            0.5f, 0.5f, 0.5f
        };

        std::array<float, 3> sunColour
        {
            0.9f, 0.84f, 0.69f
        };

        std::array<float, 3> sunDirection
        {
            0.0f, -1.0f, 0.0f
        };

        std::array<float, 3> skyColour
        {
            0.7f, 0.7f, 0.7f
        };

        std::array<float, 3> fogColour
        {
            0.3f, 0.3f, 0.35f
        };

        std::array<float, 5> cloudsColour
        {
            0.0f, 0.0f, 0.0f, 1.0f, 0.0f
        };

        std::string skyTexture;
        std::string cloudsTexture;

        float skyRotation = 0.0f;

        float fogDistance = 700.0f;
        float fogDensity = 0.0f;
        float farPlane = 700.0f;

        float rainDensity = 0.0f;

        float windVelocity = 0.0f;
        float windDirection = 0.0f;
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

        BuildLightsData buildLights;

        bool buildLightsLoaded = false;

        std::size_t buildRgbImported = 0;
        std::size_t buildRgbDuplicates = 0;
        std::size_t buildRgbRejected = 0;
    };

    XRayLighting LoadXRayLighting(const core::world::xray::LevelData& level);
    XRayWeatherKey EvaluateXRayWeather(const XRayWeather* weather, float hours);
}
