#include "Core/Assets/ModelBundleLoader.h"

#include "Core/Assets/ModelLoader.h"
#include "Core/Assets/PrimitivesLoader.h"
#include "Core/Assets/VisualLoader.h"

#include <string>
#include <utility>

namespace core::assets
{
    bool ModelBundleLoader::Load(
        const resources::ResourceFileSystem& resources,
        const std::string_view modelReference,
        ModelBundle& output,
        std::string& error) const
    {
        output = {};
        error.clear();

        ModelBundle bundle;

        ModelLoader modelLoader;

        if (!modelLoader.Load(
                resources,
                modelReference,
                bundle.model,
                error))
        {
            return false;
        }

        if (!bundle.model.visualExists)
        {
            error =
                "Visual resource was not found: " +
                bundle.model.visualLogicalPath;

            return false;
        }

        if (!bundle.model.primitivesExists)
        {
            error =
                "Primitives resource was not found: " +
                bundle.model.primitivesLogicalPath;

            return false;
        }

        VisualLoader visualLoader;

        if (!visualLoader.Load(
                resources,
                bundle.model.visualLogicalPath,
                bundle.visual,
                error))
        {
            return false;
        }

        PrimitivesLoader primitivesLoader;

        if (!primitivesLoader.Load(
                resources,
                bundle.model.primitivesLogicalPath,
                bundle.primitives,
                error))
        {
            return false;
        }

        for (const VisualRenderSet& renderSet :
             bundle.visual.renderSets)
        {
            for (const VisualGeometry& geometry :
                 renderSet.geometries)
            {
                if (bundle.primitives.FindSection(
                        geometry.vertexSection) == nullptr)
                {
                    error =
                        "Vertex section was not found in primitives: " +
                        geometry.vertexSection;

                    return false;
                }

                for (const std::string& stream :
                     geometry.streams)
                {
                    if (bundle.primitives.FindSection(
                            stream) == nullptr)
                    {
                        error =
                            "Vertex stream was not found in primitives: " +
                            stream;

                        return false;
                    }
                }

                if (bundle.primitives.FindSection(
                        geometry.primitiveSection) == nullptr)
                {
                    error =
                        "Index section was not found in primitives: " +
                        geometry.primitiveSection;

                    return false;
                }
            }
        }

        output =
            std::move(bundle);

        return true;
    }
}