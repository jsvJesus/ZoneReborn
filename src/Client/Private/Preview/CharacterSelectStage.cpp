#include "Preview/CharacterSelectStage.h"

#include "Core/Log.h"
#include "Core/Resources/DataSection.h"
#include "Core/Resources/PackedSectionReader.h"
#include "Core/Resources/ResourceType.h"

#include <span>
#include <string>
#include <vector>

namespace
{
    bool ReadString(
        const core::resources::DataSection& section,
        const std::string_view name,
        std::string& output)
    {
        const auto* child =
            section.FindChild(
                name);

        if (child ==
            nullptr)
        {
            return false;
        }

        const std::string* value =
            child->AsString();

        if (value ==
            nullptr)
        {
            return false;
        }

        output =
            *value;

        return true;
    }

    bool ReadFloat(
        const core::resources::DataSection& section,
        const std::string_view name,
        float& output)
    {
        const auto* child =
            section.FindChild(
                name);

        if (child ==
            nullptr)
        {
            return false;
        }

        return child->TryGetFloat(
            output);
    }

    bool ReadTransform(
        const core::resources::DataSection& section,
        core::math::Transform3x4& output)
    {
        const auto* values =
            section.AsFloats();

        if (values ==
                nullptr ||
            values->size() !=
                12)
        {
            return false;
        }

        for (std::size_t index = 0;
             index < 12;
             ++index)
        {
            output.values[index] =
                (*values)[index];
        }

        return true;
    }

    bool ReadTransform(
        const core::resources::DataSection& section,
        const std::string_view name,
        core::math::Transform3x4& output)
    {
        const auto* child =
            section.FindChild(
                name);

        if (child ==
            nullptr)
        {
            return false;
        }

        return ReadTransform(
            *child,
            output);
    }

    bool IsChunkInsideSpace(
        const std::string& logicalPath,
        const std::string& prefix)
    {
        return logicalPath.starts_with(
            prefix);
    }
}

namespace client::preview
{
    bool LoadCharacterSelectStage(
        const core::resources::ResourceFileSystem& resources,
        const std::string_view spaceName,
        CharacterSelectStageData& output,
        std::string& error)
    {
        output =
            {};

        error.clear();

        const std::string prefix =
            "res/spaces/" +
            std::string(
                spaceName) +
            "/";

        const auto chunkEntries =
            resources.FindByType(
                core::resources::
                    ResourceType::Chunk);

        core::resources::
            PackedSectionReader
                reader;

        for (const auto* entry :
             chunkEntries)
        {
            if (entry ==
                nullptr)
            {
                continue;
            }

            if (!IsChunkInsideSpace(
                    entry->logicalPath,
                    prefix))
            {
                continue;
            }

            std::vector<std::byte>
                bytes;

            if (!resources.ReadBinary(
                    entry->logicalPath,
                    bytes))
            {
                continue;
            }

            if (!core::resources::
                    PackedSectionReader::
                    HasSignature(
                        std::span<
                            const std::byte>(
                                bytes.data(),
                                bytes.size())))
            {
                continue;
            }

            core::resources::
                DataSection
                    root;

            std::string
                parseError;

            if (!reader.Read(
                    std::span<
                        const std::byte>(
                            bytes.data(),
                            bytes.size()),
                    root,
                    parseError))
            {
                core::Log::Warning(
                    std::string(
                        "CharacterSelect: unable to parse ") +
                    entry->logicalPath +
                    ": " +
                    parseError);

                continue;
            }

            core::math::Transform3x4
                chunkTransform =
                    core::math::
                        Transform3x4::
                        Identity();

            if (const auto* transform =
                    root.FindChild(
                        "transform"))
            {
                ReadTransform(
                    *transform,
                    chunkTransform);
            }

            for (const auto& section :
                 root.children)
            {
                //
                // cs_camera
                //
                if (section.name ==
                    "UserDataObject")
                {
                    std::string
                        type;

                    if (!ReadString(
                            section,
                            "type",
                            type))
                    {
                        continue;
                    }

                    if (type !=
                        "CameraNode")
                    {
                        continue;
                    }

                    const auto* properties =
                        section.FindChild(
                            "properties");

                    if (properties ==
                        nullptr)
                    {
                        continue;
                    }

                    std::string
                        cameraName;

                    if (!ReadString(
                            *properties,
                            "name",
                            cameraName))
                    {
                        continue;
                    }

                    if (cameraName !=
                        "cs_camera")
                    {
                        continue;
                    }

                    core::math::Transform3x4
                        localTransform;

                    if (!ReadTransform(
                            section,
                            "transform",
                            localTransform))
                    {
                        continue;
                    }

                    const auto worldTransform =
                        core::math::
                            Transform3x4::
                            Multiply(
                                localTransform,
                                chunkTransform);

                    output.camera.position =
                        worldTransform.
                            Translation();

                    //
                    // BigWorld transform:
                    //
                    // row 1 = Up
                    // row 2 = Forward
                    //
                    output.camera.up =
                    {
                        worldTransform.values[3],
                        worldTransform.values[4],
                        worldTransform.values[5]
                    };

                    output.camera.forward =
                    {
                        worldTransform.values[6],
                        worldTransform.values[7],
                        worldTransform.values[8]
                    };

                    float fov =
                        60.0f;

                    if (ReadFloat(
                            *properties,
                            "fov",
                            fov))
                    {
                        output.camera.
                            fieldOfViewDegrees =
                                fov;
                    }

                    output.hasCamera =
                        true;

                    core::Log::Info(
                        std::string(
                            "CharacterSelect cs_camera found in ") +
                        entry->logicalPath);

                    core::Log::Info(
                        std::string(
                            "CharacterSelect cs_camera FOV: ") +
                        std::to_string(
                            output.camera.
                                fieldOfViewDegrees));

                    continue;
                }

                //
                // AvatarDummy spawn
                //
                if (section.name ==
                    "entity")
                {
                    std::string
                        type;

                    if (!ReadString(
                            section,
                            "type",
                            type))
                    {
                        continue;
                    }

                    if (type !=
                        "AvatarDummy")
                    {
                        continue;
                    }

                    core::math::Transform3x4
                        localTransform;

                    if (!ReadTransform(
                            section,
                            "transform",
                            localTransform))
                    {
                        continue;
                    }

                    output.characterTransform =
                        core::math::
                            Transform3x4::
                            Multiply(
                                localTransform,
                                chunkTransform);

                    output.hasCharacterAnchor =
                        true;

                    const auto position =
                        output.
                            characterTransform.
                            Translation();

                    core::Log::Info(
                        std::string(
                            "Character anchor found at: ") +
                        std::to_string(
                            position.x) +
                        ", " +
                        std::to_string(
                            position.y) +
                        ", " +
                        std::to_string(
                            position.z));
                }
            }
        }

        if (!output.hasCamera)
        {
            error =
                "CharacterSelect: cs_camera was not found in space " +
                std::string(
                    spaceName);

            return false;
        }

        if (!output.hasCharacterAnchor)
        {
            error =
                "Character anchor was not found in space " +
                std::string(
                    spaceName);

            return false;
        }

        return true;
    }
}