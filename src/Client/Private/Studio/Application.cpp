#include "Studio/Application.h"

#include "Preview/XRayLevelRenderDataBuilder.h"

#include "Core/Log.h"
#include "Core/World/XRay/XRayLevelLoader.h"

#include <filesystem>
#include <utility>

namespace studio
{
    Application::Application(
        std::string levelName)
        :
        levelName_(
            std::move(
                levelName))
    {
    }

    int Application::Run()
    {
        if (!Initialize())
        {
            Shutdown();

            return 1;
        }

        while (window_.ProcessMessages())
        {
            if (!Update())
            {
                Shutdown();

                return 2;
            }
        }

        Shutdown();

        return 0;
    }

    bool Application::Initialize()
    {
        if (!runtime_.Initialize())
        {
            return false;
        }

        runtimeInitialized_ =
            true;

        std::string error;

        if (!window_.Initialize(
                1600,
                900,
                L"Studio",
                error))
        {
            core::Log::Error(
                error);

            return false;
        }

        windowInitialized_ =
            true;

        const std::filesystem::path levelComponent(
            levelName_);

        if (levelName_.empty() ||
            levelComponent.is_absolute() ||
            levelComponent.has_parent_path() ||
            levelComponent ==
                "." ||
            levelComponent ==
                "..")
        {
            core::Log::Error(
                "X-Ray level name must be a single directory name.");

            return false;
        }

        const std::filesystem::path levelDirectory =
            runtime_.GameRoot() /
            "gamedata" /
            "levels" /
            levelComponent;

        core::Log::Info(
            std::string(
                "Studio loading X-Ray level: ") +
            levelDirectory.string());

        core::world::xray::LevelData
            level;

        core::world::xray::LevelLoader
            levelLoader;

        if (!levelLoader.Load(
                levelDirectory,
                level,
                error))
        {
            core::Log::Error(
                std::string(
                    "Unable to load X-Ray level: ") +
                error);

            return false;
        }

        core::Log::Info(
            std::string(
                "X-Ray metadata ready: visuals=") +
            std::to_string(
                level.visuals.size()) +
            ", vertex buffers=" +
            std::to_string(
                level.vertexBuffers.size()) +
            ", index buffers=" +
            std::to_string(
                level.indexBuffers.size()));

        if (!renderer_.Initialize(
                window_.NativeHandle(),
                window_.Width(),
                window_.Height(),
                error))
        {
            core::Log::Error(
                error);

            return false;
        }

        rendererInitialized_ =
            true;

        client::preview::XRayLevelRenderStatistics
            statistics;

        if (!client::preview::StreamXRayLevelRenderData(
                level,
                renderer_,
                statistics,
                error))
        {
            core::Log::Error(
                std::string(
                    "Unable to build X-Ray DX11 scene: ") +
                error);

            return false;
        }

        core::Log::Info(
            std::string(
                "X-Ray level version: ") +
            std::to_string(
                level.version));

        core::Log::Info(
            std::string(
                "X-Ray vertex buffers: ") +
            std::to_string(
                level.vertexBuffers.size()));

        core::Log::Info(
            std::string(
                "X-Ray index buffers: ") +
            std::to_string(
                level.indexBuffers.size()));

        core::Log::Info(
            std::string(
                "X-Ray visuals: ") +
            std::to_string(
                level.visuals.size()));

        core::Log::Info(
            std::string(
                "X-Ray shaders: ") +
            std::to_string(
                level.shaders.size()));

        core::Log::Info(
            std::string(
                "X-Ray static visuals loaded: ") +
            std::to_string(
                statistics.staticVisualCount));

        core::Log::Info(
            std::string(
                "X-Ray hierarchy visuals: ") +
            std::to_string(
                statistics.hierarchyVisualCount));

        core::Log::Info(
            std::string(
                "X-Ray unsupported visuals skipped: ") +
            std::to_string(
                statistics.skippedVisualCount));

        core::Log::Info(
            std::string(
                "X-Ray DX11 meshes: ") +
            std::to_string(
                statistics.meshCount));

        core::Log::Info(
            std::string(
                "X-Ray DX11 vertices: ") +
            std::to_string(
                statistics.vertexCount));

        core::Log::Info(
            std::string(
                "X-Ray DX11 triangles: ") +
            std::to_string(
                statistics.triangleCount));

        core::Log::Info(
            std::string(
                "X-Ray material groups: ") +
            std::to_string(
                statistics.materialGroupCount));

        core::Log::Info(
            std::string(
                "X-Ray textured material groups: ") +
            std::to_string(
                statistics.texturedMaterialCount));

        core::Log::Info(
            std::string(
                "X-Ray lightmapped material groups: ") +
            std::to_string(
                statistics.lightmappedMaterialCount));

        core::Log::Info(
            std::string(
                "X-Ray DDS textures loaded: ") +
            std::to_string(
                statistics.loadedTextureCount));

        core::Log::Info(
            std::string(
                "X-Ray DDS textures missing: ") +
            std::to_string(
                statistics.missingTextureCount));

        camera_.Reset(
            renderer_.SceneCenter(),
            renderer_.SceneRadius());

        renderer_.SetCamera(
            camera_.View());

        previousFrame_ =
            std::chrono::steady_clock::now();

        core::Log::Info(
            "Studio scene ready.");

        return true;
    }

    bool Application::Update()
    {
        const auto now =
            std::chrono::steady_clock::now();

        const float deltaSeconds =
            std::chrono::duration<float>(
                now -
                previousFrame_).
                count();

        previousFrame_ =
            now;

        camera_.Update(
            window_.NativeHandle(),
            window_.ConsumeMouseWheelDelta(),
            deltaSeconds);

        renderer_.SetCamera(
            camera_.View());

        std::string error;

        if (!renderer_.Render(
                error))
        {
            core::Log::Error(
                error);

            return false;
        }

        return true;
    }

    void Application::Shutdown()
    {
        if (rendererInitialized_)
        {
            renderer_.Shutdown();

            rendererInitialized_ =
                false;
        }

        collision_.Clear();

        collisionReady_ =
            false;

        if (windowInitialized_)
        {
            window_.Shutdown();

            windowInitialized_ =
                false;
        }

        if (runtimeInitialized_)
        {
            runtime_.Shutdown();

            runtimeInitialized_ =
                false;
        }
    }
}
