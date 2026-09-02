#include "Core/Resources/ResourceFormatRegistry.h"

#include <array>
#include <string>

namespace
{
    struct ExtensionMapping final
    {
        std::string_view extension;
        core::resources::ResourceType type;
    };

    constexpr std::array ExtensionMappings
    {
        ExtensionMapping{".xml", core::resources::ResourceType::Xml},
        ExtensionMapping{".def", core::resources::ResourceType::Definition},

        ExtensionMapping{".pyc", core::resources::ResourceType::PythonBytecode},
        ExtensionMapping{".pyc_dis", core::resources::ResourceType::PythonDecompiled},
        ExtensionMapping{".py", core::resources::ResourceType::PythonSource},

        ExtensionMapping{".animation", core::resources::ResourceType::Animation},
        ExtensionMapping{".animationsettings", core::resources::ResourceType::AnimationSettings},
        ExtensionMapping{".visualsettings", core::resources::ResourceType::VisualSettings},

        ExtensionMapping{".model", core::resources::ResourceType::Model},
        ExtensionMapping{".visual", core::resources::ResourceType::Visual},
        ExtensionMapping{".primitives", core::resources::ResourceType::Primitives},

        ExtensionMapping{".dds", core::resources::ResourceType::Texture},
        ExtensionMapping{".tga", core::resources::ResourceType::Texture},
        ExtensionMapping{".png", core::resources::ResourceType::Texture},
        ExtensionMapping{".bmp", core::resources::ResourceType::Texture},
        ExtensionMapping{".jpg", core::resources::ResourceType::Texture},
        ExtensionMapping{".jpeg", core::resources::ResourceType::Texture},

        ExtensionMapping{".wav", core::resources::ResourceType::Sound},
        ExtensionMapping{".ogg", core::resources::ResourceType::Sound},
        ExtensionMapping{".mp3", core::resources::ResourceType::Sound},

        ExtensionMapping{".space", core::resources::ResourceType::Space},
        ExtensionMapping{".chunk", core::resources::ResourceType::Chunk},
        ExtensionMapping{".terrain", core::resources::ResourceType::Terrain},
        ExtensionMapping{".cdata", core::resources::ResourceType::CollisionData},

        ExtensionMapping{".fx", core::resources::ResourceType::Shader},
        ExtensionMapping{".fxh", core::resources::ResourceType::Shader},
        ExtensionMapping{".hlsl", core::resources::ResourceType::Shader},
        ExtensionMapping{".hlsli", core::resources::ResourceType::Shader},

        ExtensionMapping{".ttf", core::resources::ResourceType::Font},
        ExtensionMapping{".otf", core::resources::ResourceType::Font}
    };

    char ToLowerAscii(const char value) noexcept
    {
        if (value >= 'A' && value <= 'Z')
        {
            return static_cast<char>(value - 'A' + 'a');
        }

        return value;
    }

    std::string NormalizeExtension(
        const std::filesystem::path& path)
    {
        std::string extension = path.extension().string();

        for (char& character : extension)
        {
            character = ToLowerAscii(character);
        }

        return extension;
    }
}

namespace core::resources
{
    ResourceType ResourceFormatRegistry::Detect(
        const std::filesystem::path& path)
    {
        const std::string extension =
            NormalizeExtension(path);

        if (extension.empty())
        {
            return ResourceType::Unknown;
        }

        for (const ExtensionMapping& mapping : ExtensionMappings)
        {
            if (extension == mapping.extension)
            {
                return mapping.type;
            }
        }

        return ResourceType::Unknown;
    }

    std::string_view ResourceFormatRegistry::Name(
        const ResourceType type) noexcept
    {
        switch (type)
        {
            case ResourceType::Unknown:
                return "Unknown";

            case ResourceType::Xml:
                return "XML";

            case ResourceType::Definition:
                return "Definition";

            case ResourceType::PythonBytecode:
                return "Python Bytecode";

            case ResourceType::PythonDecompiled:
                return "Decompiled Python";

            case ResourceType::PythonSource:
                return "Python Source";

            case ResourceType::Animation:
                return "Animation";

            case ResourceType::AnimationSettings:
                return "Animation Settings";

            case ResourceType::VisualSettings:
                return "Visual Settings";

            case ResourceType::Model:
                return "Model";

            case ResourceType::Visual:
                return "Visual";

            case ResourceType::Primitives:
                return "Primitives";

            case ResourceType::Texture:
                return "Texture";

            case ResourceType::Sound:
                return "Sound";

            case ResourceType::Space:
                return "Space";

            case ResourceType::Chunk:
                return "Chunk";

            case ResourceType::Terrain:
                return "Terrain";

            case ResourceType::CollisionData:
                return "Collision Data";

            case ResourceType::Shader:
                return "Shader";

            case ResourceType::Font:
                return "Font";
        }

        return "Unknown";
    }
}