#include "Engine/Assets/VisualMaterialLoader.h"

#include "Engine/Assets/PackedSection.h"

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>

namespace engine::assets {

    namespace {

        [[nodiscard]] std::filesystem::path resource_path(
            std::string value
        ) {
            std::replace(
                value.begin(),
                value.end(),
                '\\',
                '/'
            );

            const std::filesystem::path path =
                std::filesystem::path(value).lexically_normal();

            if (path.empty() ||
                path.is_absolute() ||
                path.has_root_name()) {
                throw std::runtime_error(
                    "Invalid resource path"
                );
            }

            for (const auto& component : path) {
                if (component == "..") {
                    throw std::runtime_error(
                        "Resource path escapes packs"
                    );
                }
            }

            return path;
        }

        [[nodiscard]] const PackedNode& required(
            const PackedNode& node,
            const char* name
        ) {
            const PackedNode* child = node.find(name);

            if (!child) {
                throw std::runtime_error(
                    std::string("Missing resource field: ") +
                    name
                );
            }

            return *child;
        }

        [[nodiscard]] std::uint32_t integer_property(
            const PackedNode& property
        ) {
            return required(
                property,
                "Int"
            ).unsigned_value();
        }

        [[nodiscard]] bool boolean_property(
            const PackedNode& property
        ) {
            return required(
                property,
                "Bool"
            ).boolean();
        }

        [[nodiscard]] VisualMaterial parse_material(
            const PackedNode& group
        ) {
            VisualMaterial result;

            result.primitiveGroup =
                group.unsigned_value();

            const PackedNode& material =
                required(group, "material");

            if (const auto* identifier =
                material.find("identifier")) {
                result.identifier = identifier->text();
            }

            bool hasDiffuse = false;

            for (const auto& property : material.children) {
                if (property.name != "property") {
                    continue;
                }

                const std::string name = property.text();

                if (name == "diffuseMap") {
                    const PackedNode& texture =
                        required(property, "Texture");

                    result.diffusePath =
                        resource_path(texture.text());

                    hasDiffuse = true;
                } else if (name == "alphaTestEnable") {
                    result.alphaTest =
                        boolean_property(property);
                } else if (name == "alphaReference") {
                    const std::uint32_t value =
                        integer_property(property);

                    if (value > 255) {
                        throw std::runtime_error(
                            "Invalid alpha reference"
                        );
                    }

                    result.alphaReference =
                        static_cast<std::uint8_t>(value);
                } else if (name == "doubleSided") {
                    result.doubleSided =
                        boolean_property(property);
                }
            }

            if (!hasDiffuse ||
                result.diffusePath.extension() != ".dds") {
                throw std::runtime_error(
                    "Visual material has no supported diffuse DDS: " +
                    result.identifier
                );
            }

            return result;
        }

    }

    VisualMaterialSet load_visual_materials(
        const std::filesystem::path& packsDirectory,
        const std::filesystem::path& primitivesRelativePath
    ) {
        const auto primitivesPath =
            resource_path(
                primitivesRelativePath.generic_string()
            );

        if (!primitivesPath.generic_string().starts_with(
                "models/flora/"
            ) || primitivesPath.extension() != ".primitives") {
            throw std::invalid_argument(
                "Expected models/flora/*.primitives"
            );
        }

        VisualMaterialSet result;

        result.modelPath = primitivesPath;
        result.modelPath.replace_extension(".model");

        const PackedNode model = load_packed_section(
            packsDirectory / result.modelPath
        );

        const PackedNode* visualReference =
            model.find("nodelessVisual");

        if (!visualReference) {
            visualReference = model.find("nodefullVisual");
        }

        if (!visualReference) {
            throw std::runtime_error(
                "Model has no supported visual reference: " +
                result.modelPath.string()
            );
        }

        result.visualPath =
            resource_path(visualReference->text());

        if (result.visualPath.extension().empty()) {
            result.visualPath += ".visual";
        }

        if (result.visualPath.extension() != ".visual" ||
            !result.visualPath.generic_string().starts_with(
                "models/flora/"
            )) {
            throw std::runtime_error(
                "Unsupported visual resource path"
            );
        }

        const PackedNode visual = load_packed_section(
            packsDirectory / result.visualPath
        );

        bool foundGeometry = false;

        for (const auto& renderSet : visual.children) {
            if (renderSet.name != "renderSet") {
                continue;
            }

            for (const auto& geometry : renderSet.children) {
                if (geometry.name != "geometry") {
                    continue;
                }

                if (foundGeometry) {
                    throw std::runtime_error(
                        "Multiple visual geometries "
                        "are not supported yet"
                    );
                }

                foundGeometry = true;

                if (required(
                    geometry,
                    "primitive"
                ).text() != "indices") {
                    throw std::runtime_error(
                        "Unsupported visual index section"
                    );
                }

                for (const auto& group : geometry.children) {
                    if (group.name != "primitiveGroup") {
                        continue;
                    }

                    result.materials.push_back(
                        parse_material(group)
                    );
                }
            }
        }

        if (!foundGeometry || result.materials.empty()) {
            throw std::runtime_error(
                "Visual has no supported material groups"
            );
        }

        std::sort(
            result.materials.begin(),
            result.materials.end(),
            [](const VisualMaterial& a,
               const VisualMaterial& b) {
                return a.primitiveGroup < b.primitiveGroup;
            }
        );

        for (std::size_t i = 0;
             i < result.materials.size();
             ++i) {
            if (result.materials[i].primitiveGroup != i) {
                throw std::runtime_error(
                    "Visual material groups are incomplete"
                );
            }
        }

        return result;
    }

}
