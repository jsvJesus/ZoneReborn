#include "Studio/XRayLighting.h"
#include "Core/World/XRay/XRayLevelData.h"
#include "Core/World/XRay/XRaySpawnData.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <utility>

namespace studio
{
    namespace
    {
        constexpr float Pi = 3.14159265358979323846f;
        constexpr float Day = 86400.0f;
        using Values = std::map<std::string, std::string>;
        using Sections = std::map<std::string, Values>;

        std::string Trim(std::string text)
        {
            const auto first = text.find_first_not_of(" \t\r\n\"");
            if (first == std::string::npos) return {};
            return text.substr(first, text.find_last_not_of(" \t\r\n\"") - first + 1);
        }

        // Data-only LTX reader. Do not execute scripts/includes or invent inheritance.
        bool ReadLtx(const std::filesystem::path& path, Sections& sections)
        {
            std::error_code ec;
            const auto size = std::filesystem::file_size(path, ec);
            if (ec || size > 2u * 1024u * 1024u) return false;
            std::ifstream file(path);
            if (!file) return false;
            std::string line, section;
            while (std::getline(file, line))
            {
                if (line.size() > 16384u) return false;
                line = Trim(line.substr(0, line.find(';')));
                if (line.empty()) continue;
                if (line.front() == '#') return false;
                if (line.front() == '[')
                {
                    const auto end = line.find(']');
                    if (end == std::string::npos || !Trim(line.substr(end + 1)).empty()) return false;
                    section = Trim(line.substr(1, end - 1));
                    continue;
                }
                const auto equal = line.find('=');
                if (equal != std::string::npos && !section.empty())
                    sections[section][Trim(line.substr(0, equal))] = Trim(line.substr(equal + 1));
            }
            return !file.bad();
        }

        bool Numbers(std::string text, float* out, std::size_t count)
        {
            std::replace(text.begin(), text.end(), ',', ' ');
            std::istringstream stream(text);
            for (std::size_t i = 0; i < count; ++i)
                if (!(stream >> out[i]) || !std::isfinite(out[i])) return false;
            return true; // hemisphere_color can additionally contain alpha.
        }

        std::array<float, 3> Direction(float heading, float pitch)
        {
            // X-Ray Fvector::setHP convention (heading/pitch, radians).
            return {-std::cos(pitch) * std::sin(heading), std::sin(pitch),
                std::cos(pitch) * std::cos(heading)};
        }

        bool ReadWeather(const std::filesystem::path& path, XRayWeather& weather)
        {
            Sections sections;
            if (!ReadLtx(path, sections)) return false;
            weather.name = path.stem().string();
            for (const auto& [name, values] : sections)
            {
                int h = -1, m = -1, s = -1;
                char colon1 = 0, colon2 = 0;
                std::istringstream clock(name);
                if (!(clock >> h >> colon1 >> m >> colon2 >> s) || colon1 != ':' || colon2 != ':' ||
                    h < 0 || h > 23 || m < 0 || m > 59 || s < 0 || s > 59 ||
                    !(clock >> std::ws).eof()) continue;
                XRayWeatherKey key;
                key.seconds = static_cast<float>(h * 3600 + m * 60 + s);
                auto colour = [&](const char* field, std::array<float, 3>& output)
                {
                    const auto it = values.find(field);
                    if (it == values.end() || !Numbers(it->second, output.data(), 3)) return false;
                    return std::all_of(output.begin(), output.end(), [](float f) { return f >= 0.0f; });
                };
                if (!colour("ambient_color", key.ambient) || !colour("hemisphere_color", key.hemisphere) ||
                    !colour("sun_color", key.sunColour)) return false;
                float heading = 0.0f, pitch = 0.0f;
                const auto altitude = values.find("sun_altitude"), longitude = values.find("sun_longitude");
                if (altitude != values.end() && longitude != values.end())
                {
                    if (!Numbers(altitude->second, &heading, 1) || !Numbers(longitude->second, &pitch, 1)) return false;
                }
                else
                {
                    const auto direction = values.find("sun_dir");
                    float angles[2]{};
                    if (direction == values.end() || !Numbers(direction->second, angles, 2)) return false;
                    pitch = angles[0]; heading = angles[1];
                }
                // Real cycles can point above the horizon at dawn/night. Keep those
                // finite keys, and reduce angles before converting to avoid overflow.
                key.sunDirection = Direction(std::remainder(heading, 360.0f) * (Pi / 180.0f),
                    std::remainder(pitch, 360.0f) * (Pi / 180.0f));

                const auto readOptional = [&](const char* field, float* destination, std::size_t count)
                {
                    const auto it = values.find(field);

                    if (it == values.end())
                        return true;

                    return Numbers(
                        it->second,
                        destination,
                        count);
                };

                if (!readOptional(
                        "sky_color",
                        key.skyColour.data(),
                        3) ||
                    !readOptional(
                        "fog_color",
                        key.fogColour.data(),
                        3) ||
                    !readOptional(
                        "clouds_color",
                        key.cloudsColour.data(),
                        5) ||
                    !readOptional(
                        "sky_rotation",
                        &key.skyRotation,
                        1) ||
                    !readOptional(
                        "fog_distance",
                        &key.fogDistance,
                        1) ||
                    !readOptional(
                        "fog_density",
                        &key.fogDensity,
                        1) ||
                    !readOptional(
                        "far_plane",
                        &key.farPlane,
                        1) ||
                    !readOptional(
                        "rain_density",
                        &key.rainDensity,
                        1) ||
                    !readOptional(
                        "wind_velocity",
                        &key.windVelocity,
                        1) ||
                    !readOptional(
                        "wind_direction",
                        &key.windDirection,
                        1))
                {
                    return false;
                }

                const auto validColour =
                    [](const auto& colour)
                    {
                        return std::all_of(
                            colour.begin(),
                            colour.end(),
                            [](float value)
                            {
                                return std::isfinite(value) &&
                                    value >= 0.0f &&
                                    value <= 100.0f;
                            });
                    };

                if (!validColour(key.skyColour) ||
                    !validColour(key.fogColour) ||
                    !validColour(key.cloudsColour) ||
                    key.fogDistance <= 0.0f ||
                    key.farPlane <= 0.0f ||
                    key.fogDensity < 0.0f ||
                    key.rainDensity < 0.0f ||
                    key.windVelocity < 0.0f)
                {
                    return false;
                }

                key.fogDistance =
                    std::clamp(key.fogDistance, 1.0f, 50000.0f);

                key.farPlane =
                    std::clamp(key.farPlane, 1.0f, 50000.0f);

                key.fogDensity =
                    std::clamp(key.fogDensity, 0.0f, 10.0f);

                key.rainDensity =
                    std::clamp(key.rainDensity, 0.0f, 1.0f);

                key.windVelocity =
                    std::clamp(key.windVelocity, 0.0f, 10000.0f);

                if (const auto it = values.find("sky_texture");
                    it != values.end())
                {
                    key.skyTexture = Trim(it->second);
                }

                if (const auto it = values.find("clouds_texture");
                    it != values.end())
                {
                    key.cloudsTexture = Trim(it->second);
                }
                
                weather.keys.push_back(key);
            }
            std::sort(weather.keys.begin(), weather.keys.end(),
                [](const auto& a, const auto& b) { return a.seconds < b.seconds; });
            return !weather.keys.empty();
        }

        std::string WeatherReference(const Sections& sections, const std::string& section)
        {
            const auto it = sections.find(section);
            if (it == sections.end()) return {};
            for (const char* name : {"weathers", "weather"})
            {
                const auto value = it->second.find(name);
                if (value != it->second.end()) return value->second;
            }
            return {};
        }

        void AddLight(XRayLighting& output, bool lamp, bool spot,
            const std::array<float, 3>& position, const std::array<float, 3>& direction,
            const std::array<float, 3>& colour, float radius, float multiplier, float cone)
        {
            if (!(radius > 0.0f) || !(multiplier > 0.0f)) return;
            if (spot)
            {
                client::graphics::SceneSpotLight light;
                light.position = position; light.direction = direction; light.colour = colour;
                light.outerRadius = radius; light.multiplier = multiplier;
                light.cosConeAngle = std::cos(std::clamp(cone, 0.01f, Pi - 0.01f) * 0.5f);
                light.isDynamic = lamp; light.isStatic = !lamp; light.specular = true;
                (lamp ? output.lampSpots : output.staticSpots).push_back(light);
            }
            else
            {
                client::graphics::SceneOmniLight light;
                light.position = position; light.colour = colour;
                light.outerRadius = radius; light.multiplier = multiplier;
                light.isDynamic = lamp; light.isStatic = !lamp; light.specular = true;
                (lamp ? output.lamps : output.staticLights).push_back(light);
            }
        }
    }

    XRayLighting LoadXRayLighting(const core::world::xray::LevelData& level)
    {
        XRayLighting result;
        std::filesystem::path config;
        for (auto parent = level.levelDirectory; !parent.empty();)
        {
            std::error_code ec;
            if (std::filesystem::is_directory(parent / "configs", ec)) { config = parent / "configs"; break; }
            const auto next = parent.parent_path();
            if (next == parent) break;
            parent = next;
        }
        std::vector<std::filesystem::path> paths;
        std::error_code ec;
        if (!config.empty())
        {
            std::filesystem::directory_iterator it(config / "environment" / "weathers", ec), end;
            while (!ec && it != end)
            {
                if (it->is_regular_file(ec) && it->path().extension() == ".ltx") paths.push_back(it->path());
                if (!ec) it.increment(ec);
            }
        }
        std::sort(paths.begin(), paths.end());
        for (const auto& path : paths)
        {
            XRayWeather weather;
            if (ReadWeather(path, weather)) result.weather.push_back(std::move(weather));
            else result.warnings.push_back("Weather profile skipped (unsupported or invalid LTX): " + path.filename().string());
        }
        Sections settings;
        ReadLtx(level.levelDirectory / "level.ltx", settings);
        std::string requested = WeatherReference(settings, "environment");
        if (requested.empty()) requested = WeatherReference(settings, "map_usage");
        if (requested.empty() && !config.empty())
        {
            settings.clear();
            ReadLtx(config / "game_maps_single.ltx", settings);
            requested = WeatherReference(settings, level.levelDirectory.filename().string());
        }
        bool matched = false, clearFallback = false;
        for (std::size_t i = 0; i < result.weather.size(); ++i)
        {
            if (result.weather[i].name == "default_clear_clean")
            { result.defaultWeather = i; clearFallback = true; }
        }
        for (std::size_t i = 0; i < result.weather.size(); ++i)
            if (!requested.empty() && result.weather[i].name == requested)
            { result.defaultWeather = i; matched = true; break; }
        if (!matched && !clearFallback)
        {
            result.defaultWeather = result.weather.size();
            result.weather.push_back({"Editor daylight fallback", {XRayWeatherKey{}}});
        }
        result.source = matched ? "Map weather: " + requested : "Preview fallback: " + result.weather[result.defaultWeather].name;
        if (!matched) result.warnings.push_back(result.source +
            (requested.empty() ? " (no concrete map weather cycle)" : " (unresolved weather graph: " + requested + ")"));

        for (const auto& light : level.lights)
        {
            if (light.type == 3u) continue; // Sun comes from the weather, not a duplicate static sun.
            AddLight(result, false, light.type == 2u,
                {light.position.x, light.position.y, light.position.z},
                {light.direction.x, light.direction.y, light.direction.z},
                {std::max(0.0f, light.colour.x), std::max(0.0f, light.colour.y), std::max(0.0f, light.colour.z)},
                light.range, 1.0f, light.coneRadians);
        }
        if (level.spawn)
            for (const auto& object : level.spawn->objects)
            {
                if (!object.lamp) continue;
                const auto& lamp = *object.lamp;
                if ((lamp.flags & 8u) == 0u || lamp.health <= 0.0f) continue; // flR2, alive.
                const auto c = lamp.colour;
                AddLight(result, true, (lamp.flags & 16u) != 0u,
                    {object.position.x, object.position.y, object.position.z},
                    Direction(object.rotation.y, object.rotation.x),
                    {static_cast<float>((c >> 16u) & 255u) / 255.0f,
                     static_cast<float>((c >> 8u) & 255u) / 255.0f,
                     static_cast<float>(c & 255u) / 255.0f},
                    lamp.range, lamp.brightness, lamp.coneRadians);
            }
        if (!result.lamps.empty() || !result.lampSpots.empty())
            result.warnings.push_back("Spawn lamps use root transforms until OGF light-bone transforms are available; no shadows yet.");

        const std::filesystem::path buildPath =
            level.levelDirectory / "build.lights";

        std::error_code buildFileError;

        if (std::filesystem::is_regular_file(buildPath, buildFileError))
        {
            BuildLightsData buildData;

            std::string buildError;

            if (!LoadBuildLights(
                    buildPath,
                    buildData,
                    buildError))
            {
                result.warnings.push_back(
                    "BUILD LIGHTS: " + buildError);
            }
            else
            {
                result.buildLightsLoaded = true;

                const auto validRecord =[](const BuildLightRecord& record)
                {
                    if (record.type != 1u)
                        return false;

                    if (!std::isfinite(record.range) ||
                        !std::isfinite(record.energy))
                    {
                        return false;
                    }

                    if (record.range <= 0.0f ||
                        record.range > 100000.0f ||
                        record.energy <= 0.0f ||
                        record.energy > 1000.0f)
                    {
                        return false;
                    }

                    for (float value : record.position)
                    {
                        if (!std::isfinite(value) || std::abs(value) > 10000000.0f)
                            { return false; }
                    }

                    for (float value : record.diffuse)
                    {
                        if (!std::isfinite(value) || value < 0.0f || value > 1000.0f)
                        { return false; }
                    }

                    return true;
                };

                const auto isDuplicate = [](const client::graphics::SceneOmniLight& a, const client::graphics::SceneOmniLight& b)
                {
                    const float dx = a.position[0] - b.position[0];

                    const float dy = a.position[1] - b.position[1];
                    const float dz = a.position[2] - b.position[2];
                    constexpr float PositionTolerance = 0.15f;

                    const float distanceSquared = dx * dx + dy * dy + dz * dz;

                    if (distanceSquared > PositionTolerance * PositionTolerance)
                    {
                        return false;
                    }

                    const float radiusTolerance =
                        std::max(
                            0.25f,
                            0.05f * std::max(
                                a.outerRadius,
                                b.outerRadius));

                    return std::abs(
                        a.outerRadius - b.outerRadius) <=
                            radiusTolerance;
                };

                for (const BuildLightRecord& record : buildData.rgb)
                {
                    if (!validRecord(record))
                    {
                        ++result.buildRgbRejected;
                        continue;
                    }

                    client::graphics::SceneOmniLight light;

                    light.position = record.position;
                    light.colour = record.diffuse;

                    light.innerRadius = 0.0f;
                    light.outerRadius = record.range;

                    light.multiplier = record.energy;

                    light.isStatic = true;
                    light.isDynamic = false;
                    light.specular = false;

                    light.priority = 0;

                    bool duplicate = false;

                    for (const auto& existing :
                         result.staticLights)
                    {
                        if (isDuplicate(existing, light))
                        {
                            duplicate = true;
                            break;
                        }
                    }

                    if (duplicate)
                    {
                        ++result.buildRgbDuplicates;
                        continue;
                    }

                    result.staticLights.push_back(
                        std::move(light));

                    ++result.buildRgbImported;
                }

                const std::size_t rgbCount = buildData.rgb.size();
                const std::size_t hemiCount = buildData.hemi.size();
                const std::size_t sunCount = buildData.sun.size();
                const std::size_t auxiliaryCount = buildData.auxiliaryChunks;

                result.buildLights = std::move(buildData);

                result.warnings.push_back(
                    "BUILD LIGHTS: version=" +
                    std::to_string(result.buildLights.version) +
                    " | RGB=" + std::to_string(rgbCount) +
                    " | HEMI=" + std::to_string(hemiCount) +
                    " | SUN=" + std::to_string(sunCount) +
                    " | auxiliary=" + std::to_string(auxiliaryCount));

                result.warnings.push_back(
                    "BUILD LIGHTS: imported=" +
                    std::to_string(result.buildRgbImported) +
                    " | duplicates=" +
                    std::to_string(result.buildRgbDuplicates) +
                    " | rejected=" +
                    std::to_string(result.buildRgbRejected));

                result.warnings.push_back(
                    "BUILD LIGHTS: RGB preview uses approximate "
                    "distance attenuation; HEMI/SUN are retained "
                    "but not converted to runtime lights.");
            }
        }
        else if (buildFileError)
        {
            result.warnings.push_back(
                "BUILD LIGHTS: cannot access build.lights: " +
                buildFileError.message());
        }
        
        return result;
    }

    XRayWeatherKey EvaluateXRayWeather(const XRayWeather* weather, float hours)
    {
        if (!weather || weather->keys.empty()) return {};
        if (weather->keys.size() == 1u) return weather->keys.front();
        float seconds = std::isfinite(hours) ? std::fmod(hours, 24.0f) * 3600.0f : 43200.0f;
        if (seconds < 0.0f) seconds += Day;
        const auto next = std::upper_bound(weather->keys.begin(), weather->keys.end(), seconds,
            [](float time, const XRayWeatherKey& key) { return time < key.seconds; });
        const auto& b = next == weather->keys.end() ? weather->keys.front() : *next;
        const auto& a = next == weather->keys.begin() ? weather->keys.back() : *(next - 1);
        float span = b.seconds - a.seconds;
        if (span <= 0.0f) span += Day;
        float elapsed = seconds - a.seconds;
        if (elapsed < 0.0f) elapsed += Day;
        const float blend = std::clamp(elapsed / span, 0.0f, 1.0f);
        XRayWeatherKey key; key.seconds = seconds;
        
        for (std::size_t i = 0; i < 3u; ++i)
        {
            key.ambient[i] = std::lerp(a.ambient[i], b.ambient[i], blend);
            key.hemisphere[i] = std::lerp(a.hemisphere[i], b.hemisphere[i], blend);
            key.sunColour[i] = std::lerp(a.sunColour[i], b.sunColour[i], blend);
            key.sunDirection[i] = std::lerp(a.sunDirection[i], b.sunDirection[i], blend);
        }

        for (std::size_t i = 0; i < 3; ++i)
        {
            key.skyColour[i] =
                std::lerp(
                    a.skyColour[i],
                    b.skyColour[i],
                    blend);

            key.fogColour[i] =
                std::lerp(
                    a.fogColour[i],
                    b.fogColour[i],
                    blend);
        }

        for (std::size_t i = 0; i < 5; ++i)
        {
            key.cloudsColour[i] =
                std::lerp(
                    a.cloudsColour[i],
                    b.cloudsColour[i],
                    blend);
        }

        const auto interpolateAngle = [blend](float first, float second)
        {
            const float difference =
                std::remainder(
                    second - first,
                    360.0f);

            return first + difference * blend;
        };

        key.skyRotation =
            interpolateAngle(
        a.skyRotation,
        b.skyRotation);

        key.windDirection =
            interpolateAngle(
                a.windDirection,
                b.windDirection);

        key.fogDistance =
            std::lerp(
                a.fogDistance,
                b.fogDistance,
                blend);

        key.fogDensity =
            std::lerp(
                a.fogDensity,
                b.fogDensity,
                blend);

        key.farPlane =
            std::lerp(
                a.farPlane,
                b.farPlane,
                blend);

        key.rainDensity =
            std::lerp(
                a.rainDensity,
                b.rainDensity,
                blend);

        key.windVelocity =
            std::lerp(
                a.windVelocity,
                b.windVelocity,
                blend);

        key.skyTexture =
            blend < 0.5f
                ? a.skyTexture
                : b.skyTexture;

        key.cloudsTexture =
            blend < 0.5f
                ? a.cloudsTexture
                : b.cloudsTexture;
        
        const float length = std::sqrt(key.sunDirection[0] * key.sunDirection[0] +
            key.sunDirection[1] * key.sunDirection[1] + key.sunDirection[2] * key.sunDirection[2]);
        if (length > 0.0001f) for (float& value : key.sunDirection) value /= length;
        else key.sunDirection = {0.0f, -1.0f, 0.0f};
        return key;
    }
}
