#include "Editor/WorldGenerator/VegetationCatalogStorage.h"

#include <Windows.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <vector>

namespace editor::world_generator {

    namespace {

        constexpr const char* signature = "VEGETATION_CATALOG";
        constexpr unsigned formatVersion = 1;
        constexpr std::uintmax_t maxFileBytes = 8 * 1024 * 1024;
        constexpr std::size_t maxModels = 20000;

        [[nodiscard]] bool valid_preference(
            const VegetationModelPreference& model
        ) noexcept {
            return static_cast<unsigned>(model.source) <= 1 &&
                static_cast<unsigned>(model.kind) <= 3 &&
                (model.biomeMask & ~world_editor::vegetation::allVegetationBiomes) == 0 &&
                std::isfinite(model.weight) &&
                model.weight >= 0.0f &&
                model.weight <= 20.0f;
        }

    }

    void VegetationCatalogPreferences::remember(
        const world_editor::vegetation::VegetationCatalogEntry& entry
    ) {
        models[entry.relativePath] = {
            entry.source,
            entry.kind,
            entry.biomeMask,
            entry.weight,
            entry.enabled
        };
    }

    void VegetationCatalogPreferences::apply(
        world_editor::vegetation::VegetationAssetCatalog& catalog
    ) const {
        for (auto& entry : catalog.entries) {
            const auto found = models.find(entry.relativePath);

            if (found == models.end() ||
                found->second.source != entry.source) {
                continue;
            }

            entry.kind = found->second.kind;
            entry.biomeMask = found->second.biomeMask;
            entry.weight = found->second.weight;
            entry.enabled = found->second.enabled;
        }
    }

    std::optional<VegetationCatalogPreferences> load_vegetation_preferences(
        const std::filesystem::path& path
    ) {
        if (!std::filesystem::exists(path)) {
            return std::nullopt;
        }

        if (!std::filesystem::is_regular_file(path) ||
            std::filesystem::file_size(path) > maxFileBytes) {
            throw std::runtime_error("Invalid Vegetation catalog settings file");
        }

        std::ifstream stream(path, std::ios::binary);
        stream.imbue(std::locale::classic());

        std::string header;
        unsigned version = 0;
        unsigned source = 0;
        std::size_t modelCount = 0;

        if (!(stream >> header >> version >> source >> modelCount) ||
            header != signature ||
            version != formatVersion ||
            source > 1 ||
            modelCount > maxModels) {
            throw std::runtime_error("Invalid Vegetation catalog settings header");
        }

        VegetationCatalogPreferences preferences;
        preferences.source = static_cast<
            world_editor::vegetation::VegetationAssetSource
        >(source);

        for (std::size_t i = 0; i < modelCount; ++i) {
            std::string relativePath;
            unsigned modelSource = 0;
            unsigned kind = 0;
            unsigned mask = 0;
            unsigned enabled = 0;
            float weight = 0.0f;

            if (!(stream >> std::quoted(relativePath) >> modelSource >>
                kind >> mask >> enabled >> weight) ||
                relativePath.empty() ||
                relativePath.size() > 1024 ||
                enabled > 1 ||
                mask > world_editor::vegetation::allVegetationBiomes) {
                throw std::runtime_error("Invalid Vegetation model settings");
            }

            VegetationModelPreference model{
                static_cast<world_editor::vegetation::VegetationAssetSource>(
                    modelSource
                ),
                static_cast<world_editor::vegetation::VegetationKind>(kind),
                static_cast<std::uint8_t>(mask),
                weight,
                enabled != 0
            };

            if (!valid_preference(model) ||
                !preferences.models.emplace(
                    std::move(relativePath), model
                ).second) {
                throw std::runtime_error("Invalid Vegetation model settings");
            }
        }

        std::string trailing;

        if (stream >> trailing || !stream.eof()) {
            throw std::runtime_error("Unexpected Vegetation catalog data");
        }

        return preferences;
    }

    void save_vegetation_preferences(
        const VegetationCatalogPreferences& preferences,
        const std::filesystem::path& path
    ) {
        if (static_cast<unsigned>(preferences.source) > 1 ||
            preferences.models.size() > maxModels) {
            throw std::invalid_argument("Invalid Vegetation catalog settings");
        }

        std::vector<std::pair<std::string, VegetationModelPreference>> models(
            preferences.models.begin(), preferences.models.end()
        );

        std::sort(
            models.begin(), models.end(),
            [](const auto& left, const auto& right) {
                return left.first < right.first;
            }
        );

        auto temporary = path;
        temporary += L"." + std::to_wstring(GetCurrentProcessId()) + L".tmp";

        try {
            std::ofstream stream(
                temporary, std::ios::binary | std::ios::trunc
            );
            stream.imbue(std::locale::classic());

            if (!stream) {
                throw std::runtime_error("Cannot write Vegetation catalog settings");
            }

            stream << signature << ' ' << formatVersion << '\n';
            stream << static_cast<unsigned>(preferences.source) << ' '
                << models.size() << '\n';
            stream << std::setprecision(
                std::numeric_limits<float>::max_digits10
            );

            for (const auto& [relativePath, model] : models) {
                if (relativePath.empty() ||
                    relativePath.size() > 1024 ||
                    !valid_preference(model)) {
                    throw std::invalid_argument(
                        "Invalid Vegetation model settings"
                    );
                }

                stream << std::quoted(relativePath) << ' '
                    << static_cast<unsigned>(model.source) << ' '
                    << static_cast<unsigned>(model.kind) << ' '
                    << static_cast<unsigned>(model.biomeMask) << ' '
                    << static_cast<unsigned>(model.enabled) << ' '
                    << model.weight << '\n';
            }

            stream.flush();

            if (!stream) {
                throw std::runtime_error("Cannot write Vegetation catalog settings");
            }

            stream.close();

            if (!stream || !MoveFileExW(
                temporary.c_str(), path.c_str(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
            )) {
                throw std::runtime_error("Cannot replace Vegetation catalog settings");
            }
        } catch (...) {
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
            throw;
        }
    }

}
