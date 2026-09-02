#pragma once

#include "Core/Assets/PrimitivesContainer.h"
#include "Core/Resources/ResourceFileSystem.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace core::assets
{
    class PrimitivesLoader final
    {
    public:
        static constexpr std::uint32_t Signature =
            0x42A14E65u;

        [[nodiscard]]
        bool Load(
            const resources::ResourceFileSystem& resources,
            std::string_view primitivesReference,
            PrimitivesContainer& output,
            std::string& error) const;
    };
}