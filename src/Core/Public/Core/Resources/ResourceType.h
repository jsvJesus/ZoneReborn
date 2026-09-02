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

        SpaceSettings,
        Chunk,
        ChunkData,
        LargeObject,
        LargeObjectData,
        Terrain,

        Shader,
        Font
    };
}