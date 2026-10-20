#include "WorldEditor/Vegetation/VegetationAssetCatalog.h"
#include "WorldEditor/Vegetation/SpeedTreeSourceCatalog.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace world_editor::vegetation {

    namespace {

        [[nodiscard]] std::string lowercase(
            std::string value
        ) {
            for (char& ch : value) {
                ch = static_cast<char>(
                    std::tolower(
                        static_cast<unsigned char>(ch)
                    )
                );
            }

            return value;
        }

        [[nodiscard]] bool contains(
            const std::string_view text,
            const std::string_view token
        ) noexcept {
            return text.find(token) !=
                std::string_view::npos;
        }

        [[nodiscard]] bool is_lod(
            const std::string& name
        ) noexcept {
            return contains(name, "_lod") ||
                name.starts_with("lod_") ||
                name.ends_with("_lod");
        }

        [[nodiscard]] VegetationKind classify_kind(
            const std::string& folder,
            const std::string& name
        ) noexcept {
            if (folder == "trees" ||
                folder == "trees_side" ||
                folder == "swamp_trees") {
                return VegetationKind::Tree;
            }

            if (folder == "grass" ||
                folder == "grass_custom" ||
                folder == "plants" ||
                folder == "ground") {
                if (contains(name, "bush") ||
                    contains(name, "shrub") ||
                    contains(name, "kust")) {
                    return VegetationKind::Bush;
                }

                return VegetationKind::Grass;
            }

            if (folder == "swamp") {
                if (contains(name, "tree")) {
                    return VegetationKind::Tree;
                }

                if (contains(name, "bush")) {
                    return VegetationKind::Bush;
                }

                return VegetationKind::Grass;
            }

            // Biomass, stones, mushrooms and other unknown
            // objects require explicit classification.
            return VegetationKind::Unclassified;
        }

        [[nodiscard]] VegetationHabitat classify_habitat(
            const std::string& path,
            const VegetationKind kind
        ) noexcept {
            if (contains(path, "swamp") ||
                contains(path, "kamysh") ||
                contains(path, "reed") ||
                contains(path, "tina")) {
                return VegetationHabitat::Wetland;
            }

            if (contains(path, "dry") ||
                contains(path, "steppe") ||
                contains(path, "withered") ||
                contains(path, "yellow")) {
                return VegetationHabitat::Sandy;
            }

            if (kind == VegetationKind::Tree) {
                return VegetationHabitat::Forest;
            }

            return VegetationHabitat::Any;
        }

    }

    std::size_t VegetationAssetCatalog::enabled_count()
        const noexcept {
        return static_cast<std::size_t>(
            std::count_if(
                entries.begin(),
                entries.end(),
                [](const VegetationCatalogEntry& entry) {
                    return entry.enabled &&
                        entry.biomeMask != 0 &&
                        entry.kind !=
                            VegetationKind::Unclassified &&
                        entry.weight > 0.0f;
                }
            )
        );
    }

    VegetationAssetCatalog scan_vegetation_catalog(
        const std::filesystem::path& packsDirectory
    ) {
        const auto root =
            packsDirectory / "models" / "flora";

        if (!std::filesystem::is_directory(root)) {
            throw std::runtime_error(
                "Vegetation asset directory not found: " +
                root.string()
            );
        }

        VegetationAssetCatalog catalog;

        const auto options =
            std::filesystem::directory_options::
                skip_permission_denied;

        for (const auto& file :
             std::filesystem::recursive_directory_iterator(
                 root,
                 options
             )) {
            if (!file.is_regular_file()) {
                continue;
            }

            const std::string extension =
                lowercase(
                    file.path().extension().string()
                );

            if (extension != ".primitives") {
                continue;
            }

            const auto relativeFlora =
                file.path().lexically_relative(root);

            const auto relativePacks =
                file.path().lexically_relative(
                    packsDirectory
                );

            if (relativeFlora.empty() ||
                relativePacks.empty()) {
                continue;
            }

            const std::string name = lowercase(
                file.path().stem().string()
            );

            const std::string folder = lowercase(
                relativeFlora.begin()->string()
            );

            VegetationCatalogEntry entry;

            entry.relativePath =
                relativePacks.generic_string();

            entry.lodVariant = is_lod(name);

            entry.kind = classify_kind(
                folder,
                name
            );

            entry.biomeMask = vegetation_biome_mask(
                classify_habitat(
                    lowercase(entry.relativePath),
                    entry.kind
                )
            );

            entry.enabled =
                !entry.lodVariant &&
                entry.kind != VegetationKind::Unclassified;

            catalog.entries.push_back(
                std::move(entry)
            );
        }

        if (std::filesystem::is_directory(packsDirectory / "speedtree")) {
            for (const auto& source :
                 scan_speedtree_sources(packsDirectory)) {
                if (!source.renderableMesh) {
                    continue;
                }

                VegetationCatalogEntry entry;
                entry.relativePath = source.meshRelativePath;
                entry.kind = source.kind;
                entry.biomeMask = vegetation_biome_mask(source.habitat);
                entry.enabled = true;
                catalog.entries.push_back(std::move(entry));
            }
        }

        // Stable ordering is essential for deterministic
        // asset IDs and reproducible VegetationSeed results.
        std::sort(
            catalog.entries.begin(),
            catalog.entries.end(),
            [](const VegetationCatalogEntry& a,
               const VegetationCatalogEntry& b) {
                const auto left = lowercase(a.relativePath);
                const auto right = lowercase(b.relativePath);

                if (left != right) {
                    return left < right;
                }

                return a.relativePath < b.relativePath;
            }
        );

        if (catalog.entries.empty()) {
            throw std::runtime_error(
                "No .primitives files found in " +
                root.string()
            );
        }

        return catalog;
    }

}
