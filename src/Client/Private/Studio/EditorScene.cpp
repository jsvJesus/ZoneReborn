#include "Studio/EditorScene.h"
#include "Studio/LevelCatalog.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <utility>

namespace studio
{
    EditorScene::EditorScene()
    {
        New();
    }

    void EditorScene::New()
    {
        name_ = "Untitled";
        directory_.clear();

        statistics_ = {};
        collision_.reset();

        lights_.clear();
        ambient_ = {0.0f, 0.0f, 0.0f};
        selectedLightIndex_ = -1;
        nextLightNumber_ = 1;

        ++revision_;

        empty_ = true;
        dirty_ = false;
    }

    void EditorScene::Open(
        const std::filesystem::path& directory,
        const SceneStatistics& statistics,
        std::shared_ptr<const core::world::xray::CformCollision> collision)
    {
        directory_ = directory;

        name_ = PathToUtf8(
            directory.filename());

        statistics_ = statistics;
        collision_ = std::move(collision);

        ++revision_;

        empty_ = false;
        dirty_ = false;
    }

    const core::world::xray::CformCollision* EditorScene::Collision() const noexcept
    {
        return collision_.get();
    }

    void EditorScene::SetCollisionProbe(const CollisionProbeStatistics& probe) noexcept
    {
        statistics_.collisionProbe = probe;
    }

    void EditorScene::SetHomFrameStatistics(
        const std::size_t testedVisuals,
        const std::size_t culledVisuals) noexcept
    {
        statistics_.homTestedVisuals = testedVisuals;
        statistics_.homCulledVisuals = culledVisuals;
    }

    const std::string&
    EditorScene::Name() const noexcept
    {
        return name_;
    }

    const std::filesystem::path&
    EditorScene::Directory() const noexcept
    {
        return directory_;
    }

    const SceneStatistics&
    EditorScene::Statistics() const noexcept
    {
        return statistics_;
    }

    std::uint64_t
    EditorScene::Revision() const noexcept
    {
        return revision_;
    }

    bool EditorScene::IsEmpty() const noexcept
    {
        return empty_;
    }

    bool EditorScene::IsDirty() const noexcept
    {
        return dirty_;
    }

    const std::vector<LightObject>& EditorScene::Lights() const noexcept
    {
        return lights_;
    }

    int EditorScene::SelectedLightIndex() const noexcept
    {
        return selectedLightIndex_;
    }

    const LightObject* EditorScene::SelectedLight() const noexcept
    {
        if (selectedLightIndex_ < 0 ||
            static_cast<std::size_t>(selectedLightIndex_) >=
                lights_.size())
        {
            return nullptr;
        }

        return &lights_[
            static_cast<std::size_t>(selectedLightIndex_)];
    }

    void EditorScene::SelectLight(int index) noexcept
    {
        if (index == -1 || index == -2)
        {
            selectedLightIndex_ = index;
            return;
        }

        if (index >= 0 &&
            static_cast<std::size_t>(index) < lights_.size())
        {
            selectedLightIndex_ = index;
        }
    }

    void EditorScene::AddLight(
        LightType type,
        const core::math::Vector3& position)
    {
        if (type == LightType::Directional)
        {
            for (std::size_t i = 0; i < lights_.size(); ++i)
            {
                if (lights_[i].type == LightType::Directional)
                {
                    selectedLightIndex_ = static_cast<int>(i);
                    return;
                }
            }
        }

        LightObject light;
        light.type = type;

        switch (type)
        {
            case LightType::Point:
                light.name = "Point Light ";
                break;

            case LightType::Spot:
                light.name = "Spot Light ";
                break;

            case LightType::Directional:
                light.name = "Directional Light ";
                light.intensity = 1.0f;
                break;
        }

        light.name += std::to_string(nextLightNumber_++);

        light.position =
        {
            position.x,
            position.y,
            position.z
        };

        lights_.push_back(std::move(light));

        selectedLightIndex_ =
            static_cast<int>(lights_.size() - 1);

        dirty_ = true;
        ++revision_;
    }

    void EditorScene::UpdateLight(
        std::size_t index,
        const LightObject& light)
    {
        if (index >= lights_.size())
            return;

        LightObject updated = light;

        updated.intensity =
            std::clamp(updated.intensity, 0.0f, 1000.0f);

        updated.radius =
            std::clamp(updated.radius, 0.1f, 100000.0f);

        updated.innerRadius =
            std::clamp(
                updated.innerRadius,
                0.0f,
                updated.radius);

        updated.coneAngleDegrees =
            std::clamp(
                updated.coneAngleDegrees,
                1.0f,
                89.0f);

        lights_[index] = std::move(updated);

        dirty_ = true;
        ++revision_;
    }

    void EditorScene::RemoveSelectedLight()
    {
        if (selectedLightIndex_ < 0 ||
            static_cast<std::size_t>(selectedLightIndex_) >=
                lights_.size())
        {
            return;
        }

        lights_.erase(
            lights_.begin() + selectedLightIndex_);

        selectedLightIndex_ = -1;

        dirty_ = true;
        ++revision_;
    }

    const std::array<float, 3>&
        EditorScene::Ambient() const noexcept
    {
        return ambient_;
    }

    void EditorScene::SetAmbient(
        const std::array<float, 3>& colour)
    {
        for (std::size_t i = 0; i < 3; ++i)
        {
            ambient_[i] =
                std::clamp(colour[i], 0.0f, 10.0f);
        }

        dirty_ = true;
        ++revision_;
    }

    client::graphics::StudioLightingData EditorScene::BuildLighting() const
    {
        using namespace client::graphics;

        StudioLightingData output;
        output.enabled = true;
        output.ambient = ambient_;

        constexpr float DegreesToRadians =
            3.14159265358979323846f / 180.0f;

        for (const LightObject& object : lights_)
        {
            if (!object.enabled || object.intensity <= 0.0f)
                continue;

            if (object.type == LightType::Directional)
            {
                output.sunDirection = object.direction;
                output.sunColour = object.colour;
                output.sunIntensity = object.intensity;
                continue;
            }

            if (object.type == LightType::Point)
            {
                SceneOmniLight light;

                light.position = object.position;
                light.colour = object.colour;

                light.innerRadius = object.innerRadius;
                light.outerRadius = object.radius;
                light.multiplier = object.intensity;

                light.isDynamic = true;
                light.isStatic = false;
                light.specular = object.specular;

                light.priority = 100;

                output.omniLights.push_back(std::move(light));

                continue;
            }

            if (object.type == LightType::Spot)
            {
                SceneSpotLight light;

                light.position = object.position;
                light.direction = object.direction;
                light.colour = object.colour;

                light.innerRadius = object.innerRadius;
                light.outerRadius = object.radius;

                light.cosConeAngle =
                    std::cos(
                        object.coneAngleDegrees *
                        DegreesToRadians);

                light.multiplier = object.intensity;

                light.isDynamic = true;
                light.isStatic = false;
                light.specular = object.specular;

                light.priority = 100;

                output.spotLights.push_back(std::move(light));
            }
        }

        return output;
    }

    std::filesystem::path EditorScene::LightingPath() const
    {
        if (directory_.empty())
            return {};

        return directory_ / "editor.lighting";
    }

    bool EditorScene::SaveLighting(std::string& error)
    {
        error.clear();

        const auto path = LightingPath();

        if (path.empty())
        {
            error = "Open a level before saving lighting.";
            return false;
        }

        std::ofstream stream(
            path,
            std::ios::binary | std::ios::trunc);

        if (!stream)
        {
            error = "Unable to write lighting file.";
            return false;
        }

        stream << std::setprecision(9);

        stream << "EDITOR_LIGHTING 1\n";

        stream
            << ambient_[0] << ' '
            << ambient_[1] << ' '
            << ambient_[2] << '\n';

        stream << lights_.size() << '\n';

        for (const LightObject& light : lights_)
        {
            stream
                << static_cast<int>(light.type) << ' '
                << std::quoted(light.name) << ' '
                << light.enabled << ' '
                << light.specular << ' ';

            for (float value : light.position)
                stream << value << ' ';

            for (float value : light.direction)
                stream << value << ' ';

            for (float value : light.colour)
                stream << value << ' ';

            stream
                << light.intensity << ' '
                << light.radius << ' '
                << light.innerRadius << ' '
                << light.coneAngleDegrees << '\n';
        }

        stream.flush();

        if (!stream)
        {
            error = "Failed to save lighting data.";
            return false;
        }

        dirty_ = false;
        return true;
    }

    bool EditorScene::LoadLighting(std::string& error)
    {
        error.clear();

        const auto path = LightingPath();

        if (path.empty())
            return true;

        std::error_code filesystemError;

        const bool exists =
            std::filesystem::exists(
                path,
                filesystemError);

        if (filesystemError)
        {
            error = "Unable to access lighting file.";
            return false;
        }

        if (!exists)
            return true;

        std::ifstream stream(path, std::ios::binary);

        if (!stream)
        {
            error = "Unable to open lighting file.";
            return false;
        }

        std::string signature;
        int version = 0;

        if (!(stream >> signature >> version) ||
            signature != "EDITOR_LIGHTING" ||
            version != 1)
        {
            error = "Unsupported lighting file format.";
            return false;
        }

        std::array<float, 3> ambient;

        if (!(stream
            >> ambient[0]
            >> ambient[1]
            >> ambient[2]))
        {
            error = "Invalid ambient lighting data.";
            return false;
        }

        std::size_t count = 0;

        if (!(stream >> count) || count > 10000)
        {
            error = "Invalid lighting object count.";
            return false;
        }

        std::vector<LightObject> loaded;
        loaded.reserve(count);

        bool directionalFound = false;

        for (std::size_t i = 0; i < count; ++i)
        {
            LightObject light;
            int type = -1;

            if (!(stream
                >> type
                >> std::quoted(light.name)
                >> light.enabled
                >> light.specular))
            {
                error = "Invalid light object header.";
                return false;
            }

            if (type < 0 || type > 2)
            {
                error = "Invalid light type.";
                return false;
            }

            light.type = static_cast<LightType>(type);

            for (float& value : light.position)
                stream >> value;

            for (float& value : light.direction)
                stream >> value;

            for (float& value : light.colour)
                stream >> value;

            stream
                >> light.intensity
                >> light.radius
                >> light.innerRadius
                >> light.coneAngleDegrees;

            if (!stream)
            {
                error = "Incomplete light object data.";
                return false;
            }

            if (light.type == LightType::Directional)
            {
                if (directionalFound)
                {
                    error = "Multiple directional lights found.";
                    return false;
                }

                directionalFound = true;
            }

            loaded.push_back(std::move(light));
        }

        const auto valid =
            [](float value)
            {
                return std::isfinite(value) &&
                    std::abs(value) <= 10000000.0f;
            };

        for (float value : ambient)
        {
            if (!valid(value) || value < 0.0f)
            {
                error = "Invalid ambient colour.";
                return false;
            }
        }

        for (const LightObject& light : loaded)
        {
            for (float value : light.position)
            {
                if (!valid(value))
                {
                    error = "Invalid light position.";
                    return false;
                }
            }

            for (float value : light.direction)
            {
                if (!valid(value))
                {
                    error = "Invalid light direction.";
                    return false;
                }
            }

            for (float value : light.colour)
            {
                if (!valid(value) || value < 0.0f)
                {
                    error = "Invalid light colour.";
                    return false;
                }
            }

            if (!valid(light.intensity) ||
                !valid(light.radius) ||
                !valid(light.innerRadius) ||
                !valid(light.coneAngleDegrees) ||
                light.intensity < 0.0f ||
                light.radius < 0.1f ||
                light.innerRadius < 0.0f ||
                light.innerRadius > light.radius ||
                light.coneAngleDegrees < 1.0f ||
                light.coneAngleDegrees > 89.0f)
            {
                error = "Invalid light parameters.";
                return false;
            }
        }

        lights_ = std::move(loaded);

        ambient_ = ambient;

        selectedLightIndex_ = -1;
        nextLightNumber_ =
            static_cast<std::uint64_t>(lights_.size()) + 1;

        dirty_ = false;
        ++revision_;

        return true;
    }
}
