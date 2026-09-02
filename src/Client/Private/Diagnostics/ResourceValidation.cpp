#include "Diagnostics/ResourceValidation.h"

#include "Core/Assets/MeshLoader.h"
#include "Core/Assets/ModelBundleLoader.h"
#include "Core/Log.h"

#include <cstddef>
#include <string>
#include <unordered_map>

namespace client::diagnostics
{
    bool RunResourceValidation(
        core::Runtime& runtime)
    {
        core::Log::Info(
            "Resource validation started");

        constexpr std::string_view ModelReference =
            "models/props/electric/"
            "stolb_02_set_light_lod1.model";

        core::assets::ModelBundleLoader bundleLoader;

        core::assets::ModelBundle bundle;

        std::string error;

        if (!bundleLoader.Load(
                runtime.Resources(),
                ModelReference,
                bundle,
                error))
        {
            core::Log::Error(
                std::string(
                    "Model bundle validation failed: ") +
                error);

            return false;
        }

        core::Log::Info(
            std::string("Model: ") +
            bundle.model.resource.logicalPath);

        core::Log::Info(
            std::string("Visual: ") +
            bundle.visual.logicalPath);

        core::Log::Info(
            std::string("Primitives: ") +
            bundle.primitives.logicalPath);

        core::Log::Info(
            std::string("Primitives sections: ") +
            std::to_string(
                bundle.primitives.sections.size()));

        for (const core::assets::PrimitivesSection& section :
             bundle.primitives.sections)
        {
            core::Log::Info(
                std::string("  ") +
                section.name +
                " : " +
                std::to_string(section.size) +
                " bytes");
        }

        core::assets::MeshLoader meshLoader;

        std::size_t geometryIndex = 0;

        std::size_t totalVertices = 0;
        std::size_t totalIndices = 0;
        std::size_t totalTriangles = 0;
        std::size_t totalPrimitiveGroups = 0;

        for (const core::assets::VisualRenderSet& renderSet :
             bundle.visual.renderSets)
        {
            for (const core::assets::VisualGeometry& geometry :
                 renderSet.geometries)
            {
                core::assets::MeshData mesh;

                if (!meshLoader.Load(
                        bundle.primitives,
                        geometry,
                        mesh,
                        error))
                {
                    core::Log::Error(
                        std::string(
                            "Mesh validation failed: ") +
                        error);

                    return false;
                }

                core::Log::Info(
                    std::string("Geometry ") +
                    std::to_string(geometryIndex) +
                    ": format=" +
                    mesh.vertexFormat +
                    ", vertices=" +
                    std::to_string(mesh.vertices.size()) +
                    ", indices=" +
                    std::to_string(mesh.indices.size()) +
                    ", triangles=" +
                    std::to_string(mesh.TriangleCount()) +
                    ", groups=" +
                    std::to_string(
                        mesh.primitiveGroups.size()));

                totalVertices +=
                    mesh.vertices.size();

                totalIndices +=
                    mesh.indices.size();

                totalTriangles +=
                    mesh.TriangleCount();

                totalPrimitiveGroups +=
                    mesh.primitiveGroups.size();

                ++geometryIndex;
            }
        }

        if (geometryIndex == 0)
        {
            core::Log::Error(
                "Visual contains no geometry");

            return false;
        }

        std::unordered_map<
            std::string,
            core::assets::TextureResource>
            textures;

        for (const core::assets::VisualRenderSet& renderSet :
             bundle.visual.renderSets)
        {
            for (const core::assets::VisualGeometry& geometry :
                 renderSet.geometries)
            {
                for (const core::assets::VisualPrimitiveGroup& group :
                     geometry.primitiveGroups)
                {
                    for (const core::assets::VisualMaterialProperty& property :
                         group.material.properties)
                    {
                        if (!property.texture.has_value())
                        {
                            continue;
                        }

                        const core::assets::TextureResource& texture =
                            *property.texture;

                        textures.insert_or_assign(
                            texture.logicalPath,
                            texture);
                    }
                }
            }
        }

        std::size_t foundTextures = 0;

        for (const auto& [path, texture] :
             textures)
        {
            if (texture.exists)
            {
                ++foundTextures;

                if (texture.sourceLogicalPath !=
                    texture.logicalPath)
                {
                    core::Log::Info(
                        std::string("Texture resolved: ") +
                        texture.sourceLogicalPath +
                        " -> " +
                        texture.logicalPath);
                }
                else
                {
                    core::Log::Info(
                        std::string("Texture: ") +
                        texture.logicalPath);
                }
            }
            else
            {
                core::Log::Warning(
                    std::string("Texture not found: ") +
                    texture.sourceLogicalPath +
                    " -> tried " +
                    texture.logicalPath);
            }
        }

        core::Log::Info(
            std::string("Geometry count: ") +
            std::to_string(
                geometryIndex));

        core::Log::Info(
            std::string("Total vertices: ") +
            std::to_string(
                totalVertices));

        core::Log::Info(
            std::string("Total indices: ") +
            std::to_string(
                totalIndices));

        core::Log::Info(
            std::string("Total triangles: ") +
            std::to_string(
                totalTriangles));

        core::Log::Info(
            std::string("Primitive groups: ") +
            std::to_string(
                totalPrimitiveGroups));

        core::Log::Info(
            std::string("Textures found: ") +
            std::to_string(foundTextures) +
            "/" +
            std::to_string(
                textures.size()));

        core::Log::Info(
            "Resource validation succeeded");

        return true;
    }
}