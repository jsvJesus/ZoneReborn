#pragma once

#include "Core/Math/Vector3.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace core::assets::speedtree
{
    struct CTreeMaterial final
    {
        std::string diffuseReference;
        std::string normalReference;

        std::string diffuseLogicalPath;
        std::string normalLogicalPath;
    };

    struct CTreeIndexedVertex final
    {
        math::Vector3 position;
        math::Vector3 normal;

        float u = 0.0f;
        float v = 0.0f;

        std::array<std::byte, 32>
            extra{};
    };

    struct CTreeLeafVertex final
    {
        math::Vector3 position;
        math::Vector3 normal;

        std::array<std::byte, 76>
            extra{};
    };

    struct CTreeBillboardVertex final
    {
        math::Vector3 position;
        math::Vector3 normal;

        std::array<std::byte, 44>
            extra{};
    };

    struct CTreeLod final
    {
        std::vector<std::uint32_t>
            indices;
    };

    struct CTreeIndexedGeometry final
    {
        std::vector<CTreeIndexedVertex>
            vertices;

        std::vector<CTreeLod>
            lods;

        CTreeMaterial
            material;
    };

    struct CTreeLeafGeometry final
    {
        std::vector<CTreeLeafVertex>
            vertices;

        std::vector<CTreeLod>
            lods;

        CTreeMaterial
            material;
    };

    struct CTreeBillboardGroup final
    {
        std::vector<CTreeBillboardVertex>
            vertices;

        std::vector<std::uint32_t>
            indices;
    };

    struct CTreeBillboardGeometry final
    {
        std::vector<CTreeBillboardGroup>
            groups;

        CTreeMaterial
            material;
    };

    struct CTreeAsset final
    {
        static constexpr std::uint32_t
            SupportedVersion =
                103;

        std::string sptLogicalPath;
        std::string ctreeLogicalPath;

        std::uint32_t version = 0;

        math::Vector3 boundsMinimum;
        math::Vector3 boundsMaximum;

        float parameter0 = 0.0f;
        float parameter1 = 0.0f;

        CTreeIndexedGeometry
            branches;

        CTreeIndexedGeometry
            fronds;

        CTreeLeafGeometry
            leaves;

        CTreeBillboardGeometry
            billboard;
    };
}