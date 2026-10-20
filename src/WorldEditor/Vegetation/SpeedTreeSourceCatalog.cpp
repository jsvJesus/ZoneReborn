#include "WorldEditor/Vegetation/SpeedTreeSourceCatalog.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace world_editor::vegetation {

    namespace {

        [[nodiscard]] std::string lowercase(std::string value) {
            for (char& character : value) {
                character = static_cast<char>(std::tolower(
                    static_cast<unsigned char>(character)
                ));
            }

            return value;
        }

        [[nodiscard]] bool contains(
            std::string_view text,
            std::string_view token
        ) noexcept {
            return text.find(token) != std::string_view::npos;
        }

        [[nodiscard]] bool supported_header(
            const std::filesystem::path& path
        ) {
            std::array<unsigned char, 20> header{};
            std::ifstream file(path, std::ios::binary);

            if (!file.read(
                reinterpret_cast<char*>(header.data()),
                static_cast<std::streamsize>(header.size())
            )) {
                return false;
            }

            constexpr std::array<unsigned char, 4> version = {
                0xE8, 0x03, 0x00, 0x00
            };
            constexpr std::string_view signature = "__IdvSpt_02_";

            return std::equal(version.begin(), version.end(), header.begin()) &&
                std::equal(
                    signature.begin(),
                    signature.end(),
                    header.begin() + 8
                );
        }

        [[nodiscard]] SpeedTreeSource classify(
            const std::filesystem::path& relative,
            const std::filesystem::path& source
        ) {
            SpeedTreeSource item;
            item.relativePath = relative.generic_string();
            const std::string path = lowercase(item.relativePath);

            if (contains(path, "swamp") ||
                contains(path, "cane") ||
                contains(path, "reed")) {
                item.habitat = VegetationHabitat::Wetland;
            } else if (contains(path, "dry") ||
                contains(path, "yellow")) {
                item.habitat = VegetationHabitat::Sandy;
            } else {
                item.habitat = VegetationHabitat::Forest;
            }

            if (contains(path, "bush") ||
                contains(path, "shrub") ||
                contains(path, "thicket")) {
                item.kind = VegetationKind::Bush;
            } else if (contains(path, "grass") ||
                contains(path, "flower") ||
                contains(path, "cane") ||
                contains(path, "reed")) {
                item.kind = VegetationKind::Grass;
            }

            auto compiled = source;
            compiled.replace_extension(".ctree");
            item.compiledTree = std::filesystem::is_regular_file(compiled);

            auto mesh = source;
            mesh.replace_extension(".primitives");

            auto relativeMesh = relative;
            relativeMesh.replace_extension(".primitives");

            if (!std::filesystem::is_regular_file(mesh)) {
                const auto name = source.stem().string() +
                    "_optim.primitives";
                mesh = source.parent_path() / name;
                relativeMesh = relative.parent_path() / name;
            }

            item.renderableMesh = std::filesystem::is_regular_file(mesh);

            if (item.renderableMesh) {
                item.meshRelativePath = relativeMesh.generic_string();
            }

            return item;
        }

    }

    std::vector<SpeedTreeSource> scan_speedtree_sources(
        const std::filesystem::path& packsDirectory
    ) {
        const auto root = packsDirectory / "speedtree";

        if (!std::filesystem::is_directory(root)) {
            throw std::runtime_error(
                "SpeedTree directory not found: " + root.string()
            );
        }

        std::vector<SpeedTreeSource> sources;
        const auto options =
            std::filesystem::directory_options::skip_permission_denied;

        for (const auto& entry :
             std::filesystem::recursive_directory_iterator(root, options)) {
            if (!entry.is_regular_file() ||
                lowercase(entry.path().extension().string()) != ".spt") {
                continue;
            }

            if (!supported_header(entry.path())) {
                continue;
            }

            sources.push_back(classify(
                entry.path().lexically_relative(packsDirectory),
                entry.path()
            ));
        }

        std::sort(
            sources.begin(),
            sources.end(),
            [](const SpeedTreeSource& left, const SpeedTreeSource& right) {
                return left.relativePath < right.relativePath;
            }
        );

        return sources;
    }

}
