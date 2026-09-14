#pragma once

#include "Core/Assets/VisualAsset.h"
#include "Core/Resources/DataSection.h"
#include "Core/Resources/ResourceFileSystem.h"

#include <cstddef>
#include <string>
#include <string_view>

namespace core::assets
{
    class MaterialLoader final
    {
    public:
        [[nodiscard]]
        bool Load(
            const resources::ResourceFileSystem& resources,
            const resources::DataSection& section,
            VisualMaterial& output,
            std::string& error) const;

    private:
        [[nodiscard]]
        bool LoadSection(
            const resources::ResourceFileSystem& resources,
            const resources::DataSection& section,
            std::size_t depth,
            VisualMaterial& output,
            std::string& error) const;

        [[nodiscard]]
        bool LoadReference(
            const resources::ResourceFileSystem& resources,
            std::string_view reference,
            std::size_t depth,
            VisualMaterial& output,
            std::string& error) const;
    };
}