#include "Studio/UI/EditorUI.h"
#include "Studio/EditorScene.h"
#include "Core/World/XRay/XRaySpawnData.h"
#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <cstdio>

namespace
{
    // Native spawn strings may be CP1251. Keep bytes intact in Core and convert
    // for Dear ImGui only; already-valid UTF-8 is left unchanged.
    std::string DisplayText(const std::string& native)
    {
        if (native.empty()) return {};
        const int length = static_cast<int>(native.size());
        if (MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,native.data(),length,nullptr,0) > 0)
            return native;
        const int wideLength = MultiByteToWideChar(1251,0,native.data(),length,nullptr,0);
        if (wideLength <= 0) return "[unreadable string]";
        std::wstring wide(static_cast<std::size_t>(wideLength),L'\0');
        MultiByteToWideChar(1251,0,native.data(),length,wide.data(),wideLength);
        const int utf8Length = WideCharToMultiByte(CP_UTF8,0,wide.data(),wideLength,nullptr,0,nullptr,nullptr);
        if (utf8Length <= 0) return "[unreadable string]";
        std::string result(static_cast<std::size_t>(utf8Length),'\0');
        WideCharToMultiByte(CP_UTF8,0,wide.data(),wideLength,result.data(),utf8Length,nullptr,nullptr);
        return result;
    }

    bool Contains(const std::string& text, const std::string& filter)
    {
        return std::search(text.begin(),text.end(),filter.begin(),filter.end(),
            [](unsigned char a, unsigned char b) { return std::tolower(a) == std::tolower(b); }) != text.end();
    }

    void Field(const char* name, const std::string& value)
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(name);
        ImGui::TableSetColumnIndex(1);
        const auto display = DisplayText(value);
        ImGui::TextWrapped("%s",display.c_str());
    }

    const char* StateName(core::world::xray::SpawnStateDecoding state)
    {
        using core::world::xray::SpawnStateDecoding;
        switch (state)
        {
        case SpawnStateDecoding::Complete: return "Decoded";
        case SpawnStateDecoding::Partial: return "Partially decoded; remaining bytes preserved";
        default: return "Opaque; original bytes preserved";
        }
    }
}

namespace studio::ui
{
    void EditorUI::BuildSpawnOutliner(EditorScene& scene)
    {
        const auto* spawn = scene.Spawn();
        if (!spawn)
        {
            if (!scene.IsEmpty()) ImGui::TextDisabled("No level.spawn.");
            return;
        }
        ImGui::SetNextItemOpen(true,ImGuiCond_Once);
        if (!ImGui::TreeNode("Game objects (level.spawn)")) return;
        ImGui::Text("Objects: %zu | Sections: %zu",spawn->objects.size(),spawn->sectionCount);
        ImGui::Checkbox("Spawn markers",&showSpawnMarkers_);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Position overlays, not game models. The selected object is always marked.");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##SpawnSearch","Search name or section...",spawnSearch_,sizeof(spawnSearch_));
        if (spawnFilterRevision_ != scene.Revision() || spawnFilterText_ != spawnSearch_)
        {
            spawnFilterRevision_ = scene.Revision();
            spawnFilterText_ = spawnSearch_;
            filteredSpawnObjects_.clear();
            for (std::size_t i = 0; i < spawn->objects.size(); ++i)
            {
                const auto& object = spawn->objects[i];
                if (spawnFilterText_.empty() || Contains(DisplayText(object.name),spawnFilterText_) ||
                    Contains(DisplayText(object.section),spawnFilterText_))
                    filteredSpawnObjects_.push_back(static_cast<int>(i));
            }
        }
        ImGui::TextDisabled("Matches: %zu",filteredSpawnObjects_.size());
        if (ImGui::BeginChild("SpawnList",ImVec2(0,240.0f*uiScale_),ImGuiChildFlags_Borders))
        {
            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(filteredSpawnObjects_.size()),ImGui::GetTextLineHeightWithSpacing());
            while (clipper.Step())
            {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
                {
                    const int index = filteredSpawnObjects_[static_cast<std::size_t>(row)];
                    const auto& object = spawn->objects[static_cast<std::size_t>(index)];
                    const auto label = DisplayText(object.name.empty() ? object.section : object.name);
                    ImGui::PushID(index);
                    const auto position = ImGui::GetCursorScreenPos();
                    if (ImGui::Selectable("##SpawnObject",scene.SelectedSpawnIndex() == index)) scene.SelectSpawn(index);
                    ImGui::GetWindowDrawList()->AddText(position,ImGui::GetColorU32(ImGuiCol_Text),label.c_str());
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();
                        ImGui::TextUnformatted(DisplayText(object.section).c_str());
                        ImGui::Text("Chunk: %u | Object ID: %u",object.chunkId,unsigned(object.id));
                        ImGui::EndTooltip();
                    }
                    ImGui::PopID();
                }
            }
        }
        ImGui::EndChild();
        if (scene.SelectedSpawnObject() && ImGui::Button("Clear object selection")) scene.SelectSpawn(-1);
        ImGui::TreePop();
    }

    bool EditorUI::BuildSpawnDetails(const EditorScene& scene)
    {
        const auto* object = scene.SelectedSpawnObject();
        if (!object) return false;
        ImGui::TextUnformatted("Game object (level.spawn)");
        ImGui::TextDisabled("Read-only native import. Scripts are not executed.");
        ImGui::Separator();
        if (ImGui::BeginTable("SpawnFields",2,ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp))
        {
            Field("Name",object->name);
            Field("Section",object->section);
            Field("Chunk",std::to_string(object->chunkId));
            Field("Object ID",std::to_string(object->id));
            Field("Parent ID",std::to_string(object->parentId));
            Field("Phantom ID",std::to_string(object->phantomId));
            Field("Spawn ID",std::to_string(object->spawnId));
            Field("Version",std::to_string(object->version));
            Field("Script version",std::to_string(object->scriptVersion));
            Field("Flags",std::to_string(object->flags));
            Field("Game ID",std::to_string(object->gameId));
            Field("Game type mask",std::to_string(object->gameTypeMask));
            Field("Respawn point",std::to_string(object->respawnPoint));
            Field("Respawn time",std::to_string(object->respawnTime));
            Field("Visual (OGF reference)",object->visual.empty() ? "Not decoded / not set" : object->visual);
            Field("STATE decoding",StateName(object->stateDecoding));
            Field("STATE bytes",std::to_string(object->stateSize));
            Field("Decoded STATE bytes",std::to_string(object->decodedStateBytes));
            Field("Client data bytes",std::to_string(object->clientDataSize));
            Field("Trailing update bytes",std::to_string(object->packet.size()-object->stateOffset-object->stateSize));
            for (const auto& parameter : object->parameters) Field(parameter.first.c_str(),parameter.second);
            ImGui::EndTable();
        }
        ImGui::Text("Position: %.3f %.3f %.3f",object->position.x,object->position.y,object->position.z);
        ImGui::Text("Angles (radians): %.4f %.4f %.4f",object->rotation.x,object->rotation.y,object->rotation.z);
        if (!object->customData.empty() && ImGui::CollapsingHeader("Custom data (ini)"))
        {
            if (ImGui::BeginChild("SpawnIni",ImVec2(0,160.0f*uiScale_),ImGuiChildFlags_Borders,
                ImGuiWindowFlags_HorizontalScrollbar))
                ImGui::TextUnformatted(DisplayText(object->customData).c_str());
            ImGui::EndChild();
        }
        if (ImGui::CollapsingHeader("Original M_SPAWN packet (hex)"))
        {
            if (ImGui::BeginChild("SpawnHex",ImVec2(0,180.0f*uiScale_),ImGuiChildFlags_Borders,
                ImGuiWindowFlags_HorizontalScrollbar))
            {
                ImGuiListClipper clipper;
                clipper.Begin(static_cast<int>((object->packet.size()+15)/16));
                while (clipper.Step())
                {
                    for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
                    {
                        char line[96]{};
                        std::size_t length = static_cast<std::size_t>(std::snprintf(line,sizeof(line),"%04X: ",row*16));
                        for (std::size_t offset = static_cast<std::size_t>(row)*16;
                            offset < object->packet.size() && offset < static_cast<std::size_t>(row+1)*16; ++offset)
                            length += static_cast<std::size_t>(std::snprintf(line+length,sizeof(line)-length,"%02X ",
                                std::to_integer<unsigned>(object->packet[offset])));
                        ImGui::TextUnformatted(line);
                    }
                }
            }
            ImGui::EndChild();
        }
        return true;
    }
}
