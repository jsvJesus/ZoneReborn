#pragma once

#include "Core/Resources/PackedSectionReader.h"
#include "Core/Resources/ResourceFileSystem.h"
#include "Core/World/Chunk.h"

#include <string>
#include <string_view>

namespace core::world
{
    class ChunkLoader final
    {
    public:
        [[nodiscard]]
        bool Load(
            const resources::ResourceFileSystem& resources,
            std::string_view spaceName,
            std::string_view chunkId,
            Chunk& output,
            std::string& error) const;

    private:
        resources::PackedSectionReader reader_;
    };
}