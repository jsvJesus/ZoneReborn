#include "Engine/Core/Engine.h"
#include "WorldEditor/WorldEditor.h"

#include "WorldEditor/Terrain/TerrainGenerator.h"
#include "WorldEditor/Terrain/TerrainExporter.h"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <iostream>

int main() {
    try {
        engine::initialize();
        world_editor::initialize();

        world_editor::terrain::Settings settings;

        const auto terrain =
            world_editor::terrain::generate(settings);

        const auto outputDir =
            std::filesystem::current_path() /
            "generated" /
            "terrain";

        std::filesystem::create_directories(
            outputDir
        );

        world_editor::terrain::save_obj(
            terrain,
            outputDir / "terrain.obj"
        );

        world_editor::terrain::save_pgm(
            terrain,
            outputDir / "heightmap.pgm"
        );

        const auto [minHeight, maxHeight] =
            std::minmax_element(
                terrain.heights.begin(),
                terrain.heights.end()
            );

        std::cout
            << "Terrain generated: "
            << terrain.samplesX
            << " x "
            << terrain.samplesZ
            << " samples\n"
            << "Height range: "
            << *minHeight
            << " .. "
            << *maxHeight
            << " m\n"
            << "Output: "
            << outputDir.string()
            << '\n';

        return 0;
    }
    catch (const std::exception& exception) {
        std::cerr
            << "Terrain generation failed: "
            << exception.what()
            << '\n';

        return 1;
    }
}
