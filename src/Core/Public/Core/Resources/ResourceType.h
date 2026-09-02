#pragma once

#include <cstdint>

namespace core::resources
{
    enum class ResourceType : std::uint8_t
    {
        Unknown = 0,

        Xml,
        Definition,

        PythonBytecode,
        PythonDecompiled,
        PythonSource,

        Animation,
        AnimationSettings,
        VisualSettings,

        Model,
        Visual,
        Primitives,

        Texture,
        Sound,

        Space,
        Chunk,
        Terrain,
        CollisionData,

        Shader,
        Font
    };
}