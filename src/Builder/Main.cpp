#include "Engine/Assets/ResourceIndex.h"

#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    const auto packs = argc > 1
        ? std::filesystem::path(argv[1])
        : std::filesystem::current_path() / "packs";

    engine::assets::ResourceIndex index;
    std::string error;

    if (!index.scan(packs, error)) {
        std::cerr << "Resource scan failed: " << error << '\n';
        return 1;
    }

    std::cout << "Packs: " << packs << '\n'
              << "Files: " << index.files().size() << '\n'
              << "Total bytes: " << index.total_size() << "\n\n";

    for (const auto& [extension, info] : index.types()) {
        std::cout << extension << ": " << info.count
                  << " files, " << info.bytes << " bytes\n";

        for (const auto& sample : info.samples) {
            std::cout << "    " << sample.generic_string() << '\n';
        }
    }

    return 0;
}
