#pragma once

#include "Core/Math/BoundingBox.h"
#include "Core/Math/Transform3x4.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace core::assets
{
    struct VisualNode final
    {
        std::string identifier;
        math::Transform3x4 transform;
    };

    struct VisualMaterialProperty final
    {
        std::string name;
        std::vector<std::byte> binaryName;

        std::string textureReference;
        std::string textureLogicalPath;

        std::optional<std::array<float, 4>> vector4;
    };

    struct VisualMaterial final
    {
        std::string identifier;
        std::vector<std::byte> binaryIdentifier;

        std::string effect;

        std::int32_t collisionFlags = 0;
        std::int32_t materialKind = 0;

        std::vector<VisualMaterialProperty> properties;
    };

    struct VisualPrimitiveGroup final
    {
        std::int32_t index = 0;

        VisualMaterial material;
    };

    struct VisualGeometry final
    {
        std::string vertexSection;
        std::vector<std::byte> vertexDescriptor;

        std::vector<std::string> streams;

        std::string primitiveSection;

        std::vector<VisualPrimitiveGroup> primitiveGroups;
    };

    struct VisualRenderSet final
    {
        bool treatAsWorldSpaceObject = false;

        std::vector<std::string> nodes;
        std::vector<VisualGeometry> geometries;
    };

    struct VisualAsset final
    {
        std::string logicalPath;

        std::vector<VisualNode> nodes;
        std::vector<VisualRenderSet> renderSets;

        std::optional<math::BoundingBox> boundingBox;
    };
}