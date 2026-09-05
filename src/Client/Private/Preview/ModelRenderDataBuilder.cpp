#include "Preview/ModelRenderDataBuilder.h"

#include "Core/Images/DdsDecoder.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    std::string ToLower(
        std::string value)
    {
        for (char& character :
             value)
        {
            if (character >= 'A' &&
                character <= 'Z')
            {
                character =
                    static_cast<char>(
                        character -
                        'A' +
                        'a');
            }
        }

        return value;
    }

    std::string PropertyName(
        const core::assets::VisualMaterialProperty& property)
    {
        if (!property.name.empty())
        {
            return
                ToLower(
                    property.name);
        }

        std::string decoded;

        decoded.reserve(
            property.binaryName.size());

        for (const std::byte value :
             property.binaryName)
        {
            const auto character =
                std::to_integer<unsigned char>(
                    value);

            if (character == 0)
            {
                break;
            }

            if (character < 32 ||
                character > 126)
            {
                decoded.clear();
                break;
            }

            decoded.push_back(
                static_cast<char>(
                    character));
        }

        return
            ToLower(
                std::move(decoded));
    }

    bool Contains(
        const std::string_view text,
        const std::string_view value) noexcept
    {
        return
            text.find(
                value) !=
            std::string_view::npos;
    }

    bool IsNonDiffuseTexture(
        const std::string_view propertyName,
        const std::string_view path) noexcept
    {
        return
            Contains(
                propertyName,
                "normal") ||

            Contains(
                propertyName,
                "bump") ||

            Contains(
                propertyName,
                "specular") ||

            Contains(
                propertyName,
                "specmap") ||

            Contains(
                propertyName,
                "gloss") ||

            Contains(
                propertyName,
                "reflection") ||

            Contains(
                propertyName,
                "environment") ||

            Contains(
                propertyName,
                "cubemap") ||

            Contains(
                propertyName,
                "height") ||

            Contains(
                propertyName,
                "lightmap") ||

            Contains(
                path,
                "normal_map") ||

            Contains(
                path,
                "normalmap") ||

            Contains(
                path,
                "_normal.") ||

            Contains(
                path,
                "_nm.");
    }

    int DiffuseScore(
        const std::string_view propertyName,
        const std::string_view path) noexcept
    {
        if (IsNonDiffuseTexture(
                propertyName,
                path))
        {
            return -1;
        }

        int score = 1;

        if (Contains(
                propertyName,
                "diffuse"))
        {
            score += 100;
        }

        if (Contains(
                propertyName,
                "albedo"))
        {
            score += 100;
        }

        if (Contains(
                propertyName,
                "colour") ||
            Contains(
                propertyName,
                "color"))
        {
            score += 80;
        }

        if (Contains(
                propertyName,
                "base"))
        {
            score += 60;
        }

        if (Contains(
                propertyName,
                "texture"))
        {
            score += 20;
        }

        if (Contains(
                propertyName,
                "map"))
        {
            score += 10;
        }

        return score;
    }

    bool LooksLikeBlendMaterial(
        const core::assets::VisualMaterial& material,
        const std::string_view texturePath)
    {
        const std::string combined =
            ToLower(
                material.effect +
                " " +
                material.identifier +
                " " +
                std::string(
                    texturePath));

        return
            Contains(
                combined,
                "glass") ||

            Contains(
                combined,
                "window") ||

            Contains(
                combined,
                "transparent") ||

            Contains(
                combined,
                "transparency") ||

            Contains(
                combined,
                "alphablend") ||

            Contains(
                combined,
                "alpha_blend") ||

            Contains(
                combined,
                "blendalpha");
    }

    bool LooksLikeCutoutMaterial(
        const core::assets::VisualMaterial& material,
        const std::string_view texturePath)
    {
        const std::string combined =
            ToLower(
                material.effect +
                " " +
                material.identifier +
                " " +
                std::string(
                    texturePath));

        return
            Contains(
                combined,
                "alphatest") ||

            Contains(
                combined,
                "alpha_test") ||

            Contains(
                combined,
                "cutout") ||

            Contains(
                combined,
                "fence") ||

            Contains(
                combined,
                "wire") ||

            Contains(
                combined,
                "grid") ||

            Contains(
                combined,
                "leaf") ||

            Contains(
                combined,
                "leaves") ||

            Contains(
                combined,
                "grass") ||

            Contains(
                combined,
                "flora");
    }

    void AnalyzeAlpha(
        client::graphics::SceneTextureData& texture)
    {
        texture.hasTransparentPixels =
            false;

        texture.hasZeroAlphaPixels =
            false;

        texture.hasPartialAlphaPixels =
            false;

        if (texture.image.pixels.empty())
        {
            return;
        }

        for (std::size_t offset = 3;
             offset <
                texture.image.pixels.size();
             offset += 4)
        {
            const std::uint8_t alpha =
                std::to_integer<std::uint8_t>(
                    texture.image.pixels[
                        offset]);

            if (alpha == 255)
            {
                continue;
            }

            texture.hasTransparentPixels =
                true;

            if (alpha == 0)
            {
                texture.hasZeroAlphaPixels =
                    true;
            }
            else
            {
                texture.hasPartialAlphaPixels =
                    true;
            }

            if (texture.hasZeroAlphaPixels &&
                texture.hasPartialAlphaPixels)
            {
                break;
            }
        }
    }

    client::graphics::SceneAlphaMode
    ResolveAlphaMode(
        const core::assets::VisualMaterial& material,
        const client::graphics::SceneTextureData& texture)
    {
        if (!texture.hasTransparentPixels)
        {
            return
                client::graphics::SceneAlphaMode::Opaque;
        }

        if (LooksLikeBlendMaterial(
                material,
                texture.logicalPath))
        {
            return
                client::graphics::SceneAlphaMode::Blend;
        }

        if (LooksLikeCutoutMaterial(
                material,
                texture.logicalPath))
        {
            return
                client::graphics::SceneAlphaMode::Cutout;
        }

        if (texture.hasZeroAlphaPixels)
        {
            return
                client::graphics::SceneAlphaMode::Cutout;
        }

        return
            client::graphics::SceneAlphaMode::Opaque;
    }
}

namespace client::preview
{
    const core::assets::TextureResource*
    ModelRenderDataBuilder::FindDiffuseTexture(
        const core::assets::VisualMaterial& material) const noexcept
    {
        const core::assets::TextureResource*
            bestTexture =
                nullptr;

        int bestScore =
            -1;

        for (const core::assets::VisualMaterialProperty& property :
             material.properties)
        {
            if (!property.texture.has_value())
            {
                continue;
            }

            const core::assets::TextureResource& texture =
                *property.texture;

            if (!texture.exists)
            {
                continue;
            }

            const std::string propertyName =
                PropertyName(
                    property);

            const std::string path =
                ToLower(
                    texture.logicalPath);

            const int score =
                DiffuseScore(
                    propertyName,
                    path);

            if (score >
                bestScore)
            {
                bestScore =
                    score;

                bestTexture =
                    &texture;
            }
        }

        return bestTexture;
    }

    bool ModelRenderDataBuilder::ResolveTexture(
        const core::resources::ResourceFileSystem& resources,
        const core::assets::TextureResource& texture,
        graphics::SceneRenderData& scene,
        std::size_t& outputTextureIndex,
        std::string& error)
    {
        error.clear();

        if (!texture.exists ||
            texture.logicalPath.empty())
        {
            error =
                "Model texture does not exist.";

            return false;
        }

        const auto cached =
            textureCache_.find(
                texture.logicalPath);

        if (cached !=
            textureCache_.end())
        {
            outputTextureIndex =
                cached->second;

            return true;
        }

        std::vector<std::byte>
            encoded;

        if (!resources.ReadBinary(
                texture.logicalPath,
                encoded))
        {
            error =
                "Unable to read model texture: " +
                texture.logicalPath;

            return false;
        }

        core::images::DdsDecoder
            decoder;

        core::images::RgbaImage
            image;

        if (!decoder.Decode(
                std::span<const std::byte>(
                    encoded.data(),
                    encoded.size()),
                image,
                error))
        {
            error =
                texture.logicalPath +
                ": " +
                error;

            return false;
        }

        graphics::SceneTextureData
            sceneTexture;

        sceneTexture.logicalPath =
            texture.logicalPath;

        sceneTexture.image =
            std::move(image);

        AnalyzeAlpha(
            sceneTexture);

        outputTextureIndex =
            scene.textures.size();

        scene.textures.push_back(
            std::move(sceneTexture));

        textureCache_.emplace(
            texture.logicalPath,
            outputTextureIndex);

        return true;
    }

    bool ModelRenderDataBuilder::Build(
        const core::resources::ResourceFileSystem& resources,
        const core::assets::VisualGeometry& geometry,
        graphics::SceneRenderData& scene,
        graphics::SceneMesh& sceneMesh,
        std::size_t& outputTexturedGroups,
        std::string& error)
    {
        outputTexturedGroups =
            0;

        error.clear();

        sceneMesh.modelMaterials.clear();

        sceneMesh.modelMaterials.resize(
            sceneMesh.geometry.primitiveGroups.size());

        for (const core::assets::VisualPrimitiveGroup& visualGroup :
             geometry.primitiveGroups)
        {
            if (visualGroup.index < 0)
            {
                error =
                    "Model material contains negative primitive group index.";

                return false;
            }

            const std::size_t groupIndex =
                static_cast<std::size_t>(
                    visualGroup.index);

            if (groupIndex >=
                sceneMesh.modelMaterials.size())
            {
                error =
                    "Model material primitive group index exceeds mesh.";

                return false;
            }

            const core::assets::TextureResource* diffuse =
                FindDiffuseTexture(
                    visualGroup.material);

            if (diffuse ==
                nullptr)
            {
                continue;
            }

            std::size_t textureIndex =
                0;

            std::string textureError;

            if (!ResolveTexture(
                    resources,
                    *diffuse,
                    scene,
                    textureIndex,
                    textureError))
            {
                continue;
            }

            graphics::SceneModelMaterial& material =
                sceneMesh.modelMaterials[
                    groupIndex];

            material.diffuseTextureIndex =
                static_cast<std::int32_t>(
                    textureIndex);

            material.alphaMode =
                ResolveAlphaMode(
                    visualGroup.material,
                    scene.textures[
                        textureIndex]);

            material.alphaCutoff =
                0.45f;

            ++outputTexturedGroups;
        }

        return true;
    }
}