#include "World/WorldCollision.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

namespace
{
    constexpr float CellSize =
        8.0f;

    constexpr float Epsilon =
        0.000001f;

    using Vector3 =
        core::math::Vector3;

    Vector3 Add(
        const Vector3& a,
        const Vector3& b) noexcept
    {
        return
        {
            a.x + b.x,
            a.y + b.y,
            a.z + b.z
        };
    }

    Vector3 Subtract(
        const Vector3& a,
        const Vector3& b) noexcept
    {
        return
        {
            a.x - b.x,
            a.y - b.y,
            a.z - b.z
        };
    }

    Vector3 Multiply(
        const Vector3& value,
        const float scalar) noexcept
    {
        return
        {
            value.x * scalar,
            value.y * scalar,
            value.z * scalar
        };
    }

    float Dot(
        const Vector3& a,
        const Vector3& b) noexcept
    {
        return
            a.x * b.x +
            a.y * b.y +
            a.z * b.z;
    }

    Vector3 Cross(
        const Vector3& a,
        const Vector3& b) noexcept
    {
        return
        {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }

    float LengthSquared(
        const Vector3& value) noexcept
    {
        return
            Dot(
                value,
                value);
    }

    Vector3 Normalize(
        const Vector3& value) noexcept
    {
        const float lengthSquared =
            LengthSquared(
                value);

        if (lengthSquared <=
            Epsilon)
        {
            return {};
        }

        const float inverseLength =
            1.0f /
            std::sqrt(
                lengthSquared);

        return
            Multiply(
                value,
                inverseLength);
    }

    Vector3 TransformPoint(
        const Vector3& value,
        const core::math::Transform3x4& transform) noexcept
    {
        return
        {
            value.x * transform.values[0] +
            value.y * transform.values[3] +
            value.z * transform.values[6] +
            transform.values[9],

            value.x * transform.values[1] +
            value.y * transform.values[4] +
            value.z * transform.values[7] +
            transform.values[10],

            value.x * transform.values[2] +
            value.y * transform.values[5] +
            value.z * transform.values[8] +
            transform.values[11]
        };
    }

    Vector3 ClosestPointOnTriangle(
        const Vector3& point,
        const Vector3& a,
        const Vector3& b,
        const Vector3& c) noexcept
    {
        const Vector3 ab =
            Subtract(
                b,
                a);

        const Vector3 ac =
            Subtract(
                c,
                a);

        const Vector3 ap =
            Subtract(
                point,
                a);

        const float d1 =
            Dot(
                ab,
                ap);

        const float d2 =
            Dot(
                ac,
                ap);

        if (d1 <= 0.0f &&
            d2 <= 0.0f)
        {
            return a;
        }

        const Vector3 bp =
            Subtract(
                point,
                b);

        const float d3 =
            Dot(
                ab,
                bp);

        const float d4 =
            Dot(
                ac,
                bp);

        if (d3 >= 0.0f &&
            d4 <= d3)
        {
            return b;
        }

        const float vc =
            d1 * d4 -
            d3 * d2;

        if (vc <= 0.0f &&
            d1 >= 0.0f &&
            d3 <= 0.0f)
        {
            const float v =
                d1 /
                (d1 - d3);

            return
                Add(
                    a,
                    Multiply(
                        ab,
                        v));
        }

        const Vector3 cp =
            Subtract(
                point,
                c);

        const float d5 =
            Dot(
                ab,
                cp);

        const float d6 =
            Dot(
                ac,
                cp);

        if (d6 >= 0.0f &&
            d5 <= d6)
        {
            return c;
        }

        const float vb =
            d5 * d2 -
            d1 * d6;

        if (vb <= 0.0f &&
            d2 >= 0.0f &&
            d6 <= 0.0f)
        {
            const float w =
                d2 /
                (d2 - d6);

            return
                Add(
                    a,
                    Multiply(
                        ac,
                        w));
        }

        const float va =
            d3 * d6 -
            d5 * d4;

        if (va <= 0.0f &&
            (d4 - d3) >= 0.0f &&
            (d5 - d6) >= 0.0f)
        {
            const Vector3 bc =
                Subtract(
                    c,
                    b);

            const float w =
                (d4 - d3) /
                (
                    (d4 - d3) +
                    (d5 - d6)
                );

            return
                Add(
                    b,
                    Multiply(
                        bc,
                        w));
        }

        const float denominator =
            1.0f /
            (
                va +
                vb +
                vc
            );

        const float v =
            vb *
            denominator;

        const float w =
            vc *
            denominator;

        return
            Add(
                a,
                Add(
                    Multiply(
                        ab,
                        v),
                    Multiply(
                        ac,
                        w)));
    }

    bool IntersectSegmentTriangle(
        const Vector3& start,
        const Vector3& end,
        const Vector3& a,
        const Vector3& b,
        const Vector3& c,
        float& fraction) noexcept
    {
        const Vector3 direction =
            Subtract(
                end,
                start);

        const Vector3 edge1 =
            Subtract(
                b,
                a);

        const Vector3 edge2 =
            Subtract(
                c,
                a);

        const Vector3 p =
            Cross(
                direction,
                edge2);

        const float determinant =
            Dot(
                edge1,
                p);

        if (std::abs(
                determinant) <=
            Epsilon)
        {
            return false;
        }

        const float inverseDeterminant =
            1.0f /
            determinant;

        const Vector3 t =
            Subtract(
                start,
                a);

        const float u =
            Dot(
                t,
                p) *
            inverseDeterminant;

        if (u < 0.0f ||
            u > 1.0f)
        {
            return false;
        }

        const Vector3 q =
            Cross(
                t,
                edge1);

        const float v =
            Dot(
                direction,
                q) *
            inverseDeterminant;

        if (v < 0.0f ||
            u + v > 1.0f)
        {
            return false;
        }

        const float result =
            Dot(
                edge2,
                q) *
            inverseDeterminant;

        if (result < 0.0f ||
            result > 1.0f)
        {
            return false;
        }

        fraction =
            result;

        return true;
    }
}

namespace client::world
{
    std::int32_t Collision::CellCoordinate(
        const float value) noexcept
    {
        return
            static_cast<std::int32_t>(
                std::floor(
                    value /
                    CellSize));
    }

    std::uint64_t Collision::CellKey(
        const std::int32_t x,
        const std::int32_t z) noexcept
    {
        return
            (
                static_cast<std::uint64_t>(
                    static_cast<std::uint32_t>(
                        x)) <<
                32u
            ) |
            static_cast<std::uint32_t>(
                z);
    }

    bool Collision::Build(
        const graphics::SceneRenderData& scene,
        std::string& error)
    {
        Clear();

        error.clear();

        auto appendInstance =
            [this, &scene](
                const std::size_t meshIndex,
                const core::math::Transform3x4& transform)
            {
                if (meshIndex >=
                    scene.meshes.size())
                {
                    return;
                }

                const graphics::SceneMesh& sceneMesh =
                    scene.meshes[
                        meshIndex];

                if (sceneMesh.waterMaterialIndex >=
                    0)
                {
                    return;
                }

                const core::assets::MeshData& mesh =
                    sceneMesh.geometry;

                const std::size_t triangleCount =
                    mesh.TriangleCount();

                for (std::size_t triangleIndex = 0;
                     triangleIndex < triangleCount;
                     ++triangleIndex)
                {
                    const std::size_t indexOffset =
                        triangleIndex *
                        3;

                    const std::uint32_t ia =
                        mesh.IndexAt(
                            indexOffset + 0);

                    const std::uint32_t ib =
                        mesh.IndexAt(
                            indexOffset + 1);

                    const std::uint32_t ic =
                        mesh.IndexAt(
                            indexOffset + 2);

                    if (ia >= mesh.vertices.size() ||
                        ib >= mesh.vertices.size() ||
                        ic >= mesh.vertices.size())
                    {
                        continue;
                    }

                    Triangle triangle;

                    triangle.a =
                        TransformPoint(
                            mesh.vertices[ia].position,
                            transform);

                    triangle.b =
                        TransformPoint(
                            mesh.vertices[ib].position,
                            transform);

                    triangle.c =
                        TransformPoint(
                            mesh.vertices[ic].position,
                            transform);

                    const Vector3 edge1 =
                        Subtract(
                            triangle.b,
                            triangle.a);

                    const Vector3 edge2 =
                        Subtract(
                            triangle.c,
                            triangle.a);

                    triangle.normal =
                        Normalize(
                            Cross(
                                edge1,
                                edge2));

                    if (LengthSquared(
                            triangle.normal) <=
                        Epsilon)
                    {
                        continue;
                    }

                    triangle.minX =
                        std::min(
                            {
                                triangle.a.x,
                                triangle.b.x,
                                triangle.c.x
                            });

                    triangle.maxX =
                        std::max(
                            {
                                triangle.a.x,
                                triangle.b.x,
                                triangle.c.x
                            });

                    triangle.minY =
                        std::min(
                            {
                                triangle.a.y,
                                triangle.b.y,
                                triangle.c.y
                            });

                    triangle.maxY =
                        std::max(
                            {
                                triangle.a.y,
                                triangle.b.y,
                                triangle.c.y
                            });

                    triangle.minZ =
                        std::min(
                            {
                                triangle.a.z,
                                triangle.b.z,
                                triangle.c.z
                            });

                    triangle.maxZ =
                        std::max(
                            {
                                triangle.a.z,
                                triangle.b.z,
                                triangle.c.z
                            });

                    triangle.terrain =
                        sceneMesh.terrainMaterialIndex >=
                        0;

                    const std::uint32_t storedIndex =
                        static_cast<std::uint32_t>(
                            triangles_.size());

                    triangles_.push_back(
                        triangle);

                    if (!hasBounds_)
                    {
                        minimumX_ = triangle.minX;
                        maximumX_ = triangle.maxX;

                        minimumY_ = triangle.minY;
                        maximumY_ = triangle.maxY;

                        minimumZ_ = triangle.minZ;
                        maximumZ_ = triangle.maxZ;

                        hasBounds_ =
                            true;
                    }
                    else
                    {
                        minimumX_ =
                            std::min(
                                minimumX_,
                                triangle.minX);

                        maximumX_ =
                            std::max(
                                maximumX_,
                                triangle.maxX);

                        minimumY_ =
                            std::min(
                                minimumY_,
                                triangle.minY);

                        maximumY_ =
                            std::max(
                                maximumY_,
                                triangle.maxY);

                        minimumZ_ =
                            std::min(
                                minimumZ_,
                                triangle.minZ);

                        maximumZ_ =
                            std::max(
                                maximumZ_,
                                triangle.maxZ);
                    }

                    if (triangle.terrain)
                    {
                        if (!hasTerrainBounds_)
                        {
                            terrainMinimumX_ =
                                triangle.minX;

                            terrainMaximumX_ =
                                triangle.maxX;

                            terrainMinimumZ_ =
                                triangle.minZ;

                            terrainMaximumZ_ =
                                triangle.maxZ;

                            hasTerrainBounds_ =
                                true;
                        }
                        else
                        {
                            terrainMinimumX_ =
                                std::min(
                                    terrainMinimumX_,
                                    triangle.minX);

                            terrainMaximumX_ =
                                std::max(
                                    terrainMaximumX_,
                                    triangle.maxX);

                            terrainMinimumZ_ =
                                std::min(
                                    terrainMinimumZ_,
                                    triangle.minZ);

                            terrainMaximumZ_ =
                                std::max(
                                    terrainMaximumZ_,
                                    triangle.maxZ);
                        }
                    }

                    const std::int32_t minimumCellX =
                        CellCoordinate(
                            triangle.minX);

                    const std::int32_t maximumCellX =
                        CellCoordinate(
                            triangle.maxX);

                    const std::int32_t minimumCellZ =
                        CellCoordinate(
                            triangle.minZ);

                    const std::int32_t maximumCellZ =
                        CellCoordinate(
                            triangle.maxZ);

                    for (std::int32_t cellZ = minimumCellZ;
                         cellZ <= maximumCellZ;
                         ++cellZ)
                    {
                        for (std::int32_t cellX = minimumCellX;
                             cellX <= maximumCellX;
                             ++cellX)
                        {
                            cells_[
                                CellKey(
                                    cellX,
                                    cellZ)].
                                push_back(
                                    storedIndex);
                        }
                    }
                }
            };

        for (const graphics::SceneInstance& instance :
             scene.instances)
        {
            appendInstance(
                instance.meshIndex,
                instance.transform);
        }

        for (const graphics::SceneLodInstance& instance :
             scene.lodInstances)
        {
            if (instance.levelCount ==
                0)
            {
                continue;
            }

            for (const std::size_t meshIndex :
                 instance.levels[0].meshIndices)
            {
                appendInstance(
                    meshIndex,
                    instance.transform);
            }
        }

        if (triangles_.empty())
        {
            error =
                "World collision contains no triangles.";

            return false;
        }

        return true;
    }

    void Collision::Clear() noexcept
    {
        triangles_.clear();
        cells_.clear();

        minimumX_ = 0.0f;
        maximumX_ = 0.0f;

        minimumY_ = 0.0f;
        maximumY_ = 0.0f;

        minimumZ_ = 0.0f;
        maximumZ_ = 0.0f;

        terrainMinimumX_ = 0.0f;
        terrainMaximumX_ = 0.0f;

        terrainMinimumZ_ = 0.0f;
        terrainMaximumZ_ = 0.0f;

        hasBounds_ = false;
        hasTerrainBounds_ = false;
    }

    bool Collision::GroundAt(
        const float x,
        const float z,
        const float minimumY,
        const float maximumY,
        const bool terrainOnly,
        float& height) const noexcept
    {
        const auto iterator =
            cells_.find(
                CellKey(
                    CellCoordinate(
                        x),
                    CellCoordinate(
                        z)));

        if (iterator ==
            cells_.end())
        {
            return false;
        }

        bool found =
            false;

        float bestHeight =
            -std::numeric_limits<float>::infinity();

        for (const std::uint32_t triangleIndex :
             iterator->second)
        {
            if (triangleIndex >=
                triangles_.size())
            {
                continue;
            }

            const Triangle& triangle =
                triangles_[
                    triangleIndex];

            if (terrainOnly &&
                !triangle.terrain)
            {
                continue;
            }

            if (std::abs(
                    triangle.normal.y) <
                0.25f)
            {
                continue;
            }

            if (x < triangle.minX - Epsilon ||
                x > triangle.maxX + Epsilon ||
                z < triangle.minZ - Epsilon ||
                z > triangle.maxZ + Epsilon)
            {
                continue;
            }

            const float denominator =
                (
                    triangle.b.z -
                    triangle.c.z
                ) *
                (
                    triangle.a.x -
                    triangle.c.x
                ) +
                (
                    triangle.c.x -
                    triangle.b.x
                ) *
                (
                    triangle.a.z -
                    triangle.c.z
                );

            if (std::abs(
                    denominator) <=
                Epsilon)
            {
                continue;
            }

            const float first =
                (
                    (
                        triangle.b.z -
                        triangle.c.z
                    ) *
                    (
                        x -
                        triangle.c.x
                    ) +
                    (
                        triangle.c.x -
                        triangle.b.x
                    ) *
                    (
                        z -
                        triangle.c.z
                    )
                ) /
                denominator;

            const float second =
                (
                    (
                        triangle.c.z -
                        triangle.a.z
                    ) *
                    (
                        x -
                        triangle.c.x
                    ) +
                    (
                        triangle.a.x -
                        triangle.c.x
                    ) *
                    (
                        z -
                        triangle.c.z
                    )
                ) /
                denominator;

            const float third =
                1.0f -
                first -
                second;

            if (first < -Epsilon ||
                second < -Epsilon ||
                third < -Epsilon)
            {
                continue;
            }

            const float candidate =
                triangle.a.y * first +
                triangle.b.y * second +
                triangle.c.y * third;

            if (candidate <
                    minimumY ||
                candidate >
                    maximumY)
            {
                continue;
            }

            if (!found ||
                candidate >
                    bestHeight)
            {
                bestHeight =
                    candidate;

                found =
                    true;
            }
        }

        if (!found)
        {
            return false;
        }

        height =
            bestHeight;

        return true;
    }

    bool Collision::FindSpawn(
        core::math::Vector3& position) const noexcept
    {
        if (!hasBounds_)
        {
            return false;
        }

        const bool terrainPreferred =
            hasTerrainBounds_;

        const float centerX =
            terrainPreferred
                ? (
                    terrainMinimumX_ +
                    terrainMaximumX_
                  ) *
                  0.5f
                : (
                    minimumX_ +
                    maximumX_
                  ) *
                  0.5f;

        const float centerZ =
            terrainPreferred
                ? (
                    terrainMinimumZ_ +
                    terrainMaximumZ_
                  ) *
                  0.5f
                : (
                    minimumZ_ +
                    maximumZ_
                  ) *
                  0.5f;

        constexpr float SearchStep =
            4.0f;

        constexpr int SearchRadius =
            32;

        for (int ring = 0;
             ring <= SearchRadius;
             ++ring)
        {
            for (int z = -ring;
                 z <= ring;
                 ++z)
            {
                for (int x = -ring;
                     x <= ring;
                     ++x)
                {
                    if (ring != 0 &&
                        std::abs(x) != ring &&
                        std::abs(z) != ring)
                    {
                        continue;
                    }

                    const float sampleX =
                        centerX +
                        static_cast<float>(
                            x) *
                        SearchStep;

                    const float sampleZ =
                        centerZ +
                        static_cast<float>(
                            z) *
                        SearchStep;

                    float ground =
                        0.0f;

                    if (GroundAt(
                            sampleX,
                            sampleZ,
                            minimumY_ - 32.0f,
                            maximumY_ + 32.0f,
                            terrainPreferred,
                            ground))
                    {
                        position =
                        {
                            sampleX,
                            ground + 0.02f,
                            sampleZ
                        };

                        return true;
                    }
                }
            }
        }

        float ground =
            0.0f;

        if (!GroundAt(
                centerX,
                centerZ,
                minimumY_ - 32.0f,
                maximumY_ + 32.0f,
                false,
                ground))
        {
            return false;
        }

        position =
        {
            centerX,
            ground + 0.02f,
            centerZ
        };

        return true;
    }

    bool Collision::FindGround(
        const core::math::Vector3& position,
        const float stepUp,
        const float probeDown,
        float& groundHeight) const noexcept
    {
        return
            GroundAt(
                position.x,
                position.z,
                position.y -
                    probeDown,
                position.y +
                    stepUp,
                false,
                groundHeight);
    }

    bool Collision::BlocksCapsule(
        const core::math::Vector3& position,
        const float radius,
        const float height) const noexcept
    {
        const std::int32_t minimumCellX =
            CellCoordinate(
                position.x -
                radius);

        const std::int32_t maximumCellX =
            CellCoordinate(
                position.x +
                radius);

        const std::int32_t minimumCellZ =
            CellCoordinate(
                position.z -
                radius);

        const std::int32_t maximumCellZ =
            CellCoordinate(
                position.z +
                radius);

        const float radiusSquared =
            radius *
            radius;

        std::unordered_set<std::uint32_t>
            visited;

        const Vector3 samples[3]
        {
            {
                position.x,
                position.y + radius,
                position.z
            },
            {
                position.x,
                position.y + height * 0.5f,
                position.z
            },
            {
                position.x,
                position.y + height - radius,
                position.z
            }
        };

        for (std::int32_t cellZ = minimumCellZ;
             cellZ <= maximumCellZ;
             ++cellZ)
        {
            for (std::int32_t cellX = minimumCellX;
                 cellX <= maximumCellX;
                 ++cellX)
            {
                const auto iterator =
                    cells_.find(
                        CellKey(
                            cellX,
                            cellZ));

                if (iterator ==
                    cells_.end())
                {
                    continue;
                }

                for (const std::uint32_t triangleIndex :
                     iterator->second)
                {
                    if (!visited.insert(
                            triangleIndex).
                            second)
                    {
                        continue;
                    }

                    if (triangleIndex >=
                        triangles_.size())
                    {
                        continue;
                    }

                    const Triangle& triangle =
                        triangles_[
                            triangleIndex];

                    if (std::abs(
                            triangle.normal.y) >
                        0.72f)
                    {
                        continue;
                    }

                    if (position.y + height <
                            triangle.minY ||
                        position.y >
                            triangle.maxY)
                    {
                        continue;
                    }

                    for (const Vector3& sample :
                         samples)
                    {
                        const Vector3 closest =
                            ClosestPointOnTriangle(
                                sample,
                                triangle.a,
                                triangle.b,
                                triangle.c);

                        if (LengthSquared(
                                Subtract(
                                    sample,
                                    closest)) <
                            radiusSquared)
                        {
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    bool Collision::Raycast(
        const core::math::Vector3& start,
        const core::math::Vector3& end,
        float& fraction) const noexcept
    {
        core::math::Vector3 normal{};

        return
            Raycast(
                start,
                end,
                fraction,
                normal);
    }

    bool Collision::Raycast(
        const core::math::Vector3& start,
        const core::math::Vector3& end,
        float& fraction,
        core::math::Vector3& normal) const noexcept
    {
        fraction =
            1.0f;

        normal =
            {};

        const float minimumX =
            std::min(
                start.x,
                end.x);

        const float maximumX =
            std::max(
                start.x,
                end.x);

        const float minimumZ =
            std::min(
                start.z,
                end.z);

        const float maximumZ =
            std::max(
                start.z,
                end.z);

        const std::int32_t minimumCellX =
            CellCoordinate(
                minimumX);

        const std::int32_t maximumCellX =
            CellCoordinate(
                maximumX);

        const std::int32_t minimumCellZ =
            CellCoordinate(
                minimumZ);

        const std::int32_t maximumCellZ =
            CellCoordinate(
                maximumZ);

        std::unordered_set<std::uint32_t>
            visited;

        bool hit =
            false;

        for (std::int32_t cellZ = minimumCellZ;
             cellZ <= maximumCellZ;
             ++cellZ)
        {
            for (std::int32_t cellX = minimumCellX;
                 cellX <= maximumCellX;
                 ++cellX)
            {
                const auto iterator =
                    cells_.find(
                        CellKey(
                            cellX,
                            cellZ));

                if (iterator ==
                    cells_.end())
                {
                    continue;
                }

                for (const std::uint32_t triangleIndex :
                     iterator->second)
                {
                    if (!visited.insert(
                            triangleIndex).
                            second)
                    {
                        continue;
                    }

                    if (triangleIndex >=
                        triangles_.size())
                    {
                        continue;
                    }

                    const Triangle& triangle =
                        triangles_[
                            triangleIndex];

                    float candidate =
                        1.0f;

                    if (!IntersectSegmentTriangle(
                            start,
                            end,
                            triangle.a,
                            triangle.b,
                            triangle.c,
                            candidate))
                    {
                        continue;
                    }

                    if (!hit ||
                        candidate <
                            fraction)
                    {
                        fraction =
                            candidate;

                        normal =
                            triangle.normal;

                        hit =
                            true;
                    }
                }
            }
        }

        return hit;
    }
}
