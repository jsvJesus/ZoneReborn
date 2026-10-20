#pragma once

#include "WorldEditor/Vegetation/VegetationGenerator.h"

#include <cstdint>
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace world_editor::vegetation {

    enum class VegetationHabitat : std::uint8_t {
        Any,
        Grassland,
        Forest,
        Sandy,
        Rocky,
        Wetland
    };

    constexpr std::uint8_t allVegetationBiomes = 0x1f;

    enum class VegetationAssetSource : std::uint8_t {
        Flora,
        SpeedTree
    };

    [[nodiscard]] constexpr std::uint8_t vegetation_biome_mask(
        VegetationHabitat habitat
    ) noexcept {
        if (habitat == VegetationHabitat::Any) {
            return allVegetationBiomes;
        }

        return static_cast<std::uint8_t>(
            1u << (static_cast<unsigned>(habitat) - 1u)
        );
    }

    struct VegetationCatalogEntry {
        // Path relative to bin/packs.
        std::string relativePath;

        VegetationKind kind =
            VegetationKind::Unclassified;

        // Grassland, Forest, Sandy, Rocky, Wetland in bit order.
        std::uint8_t biomeMask = allVegetationBiomes;

        float weight = 1.0f;

        bool enabled = false;
        bool lodVariant = false;
        VegetationAssetSource source = VegetationAssetSource::Flora;
    };

    struct VegetationAssetCatalog {
        std::vector<VegetationCatalogEntry> entries;

        [[nodiscard]] std::size_t enabled_count() const noexcept;
        [[nodiscard]] std::size_t enabled_count(
            VegetationAssetSource source
        ) const noexcept;

        [[nodiscard]] VegetationAssetCatalog for_source(
            VegetationAssetSource source
        ) const;
    };

    [[nodiscard]] VegetationAssetCatalog scan_vegetation_catalog(
        const std::filesystem::path& packsDirectory
    );

}
