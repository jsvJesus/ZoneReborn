#include "Studio/XRayLighting.h"
#include "Core/World/XRay/XRayLevelData.h"
#include "Core/World/XRay/XRaySpawnData.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>

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
        const float length = std::sqrt(key.sunDirection[0] * key.sunDirection[0] +
            key.sunDirection[1] * key.sunDirection[1] + key.sunDirection[2] * key.sunDirection[2]);
        if (length > 0.0001f) for (float& value : key.sunDirection) value /= length;
        else key.sunDirection = {0.0f, -1.0f, 0.0f};
        return key;
    }
}
