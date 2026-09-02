#include "Preview/ModelPreviewLoader.h"

#include "Core/Assets/MeshLoader.h"
#include "Core/Assets/ModelBundleLoader.h"
#include "Core/Log.h"

#include <string>
#include <string_view>
#include <utility>

namespace client::preview
{
    bool LoadModelPreview(
        core::Runtime& runtime,
        core::assets::MeshData& output,
        std::string& error)
    {
        output = {};
        error.clear();

        constexpr std::string_view ModelReference =
            "models/props/electric/"
            "stolb_02_set_light_lod1.model";

        core::assets::ModelBundleLoader bundleLoader;

        core::assets::ModelBundle bundle;

        if (!bundleLoader.Load(
                runtime.Resources(),
                ModelReference,
                bundle,
                error))
        {
            return false;
        }

        const core::assets::VisualGeometry*
            selectedGeometry =
                nullptr;

        for (const core::assets::VisualRenderSet& renderSet :
             bundle.visual.renderSets)
        {
            if (!renderSet.geometries.empty())
            {
                selectedGeometry =
                    &renderSet.geometries.front();

                break;
            }
        }

        if (selectedGeometry == nullptr)
        {
            error =
                "Model visual contains no geometry.";

            return false;
        }

        core::assets::MeshLoader meshLoader;

        core::assets::MeshData mesh;

        if (!meshLoader.Load(
                bundle.primitives,
                *selectedGeometry,
                mesh,
                error))
        {
            return false;
        }

        core::Log::Info(
            std::string("Preview model: ") +
            bundle.model.resource.logicalPath);

        core::Log::Info(
            std::string("Preview visual: ") +
            bundle.visual.logicalPath);

        core::Log::Info(
            std::string("Preview primitives: ") +
            bundle.primitives.logicalPath);

        core::Log::Info(
            std::string("Preview mesh: vertices=") +
            std::to_string(
                mesh.vertices.size()) +
            ", indices=" +
            std::to_string(
                mesh.indices.size()) +
            ", triangles=" +
            std::to_string(
                mesh.TriangleCount()));

        output =
            std::move(mesh);

        return true;
    }
}