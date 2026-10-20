#include "Editor/UI/OutputLog.h"

#include "imgui.h"

#include <cstddef>
#include <string_view>

namespace editor::ui {

    void OutputLog::add(std::string_view message) {
        constexpr std::size_t maxEntries = 512;

        if (entries_.size() == maxEntries) {
            entries_.erase(entries_.begin());
        }

        entries_.emplace_back(message);
    }

    void OutputLog::draw() {
        if (!ImGui::Begin("Output Log")) {
            ImGui::End();
            return;
        }

        if (ImGui::Button("Clear")) {
            entries_.clear();
        }

        ImGui::Separator();

        for (const auto& entry : entries_) {
            ImGui::TextUnformatted(entry.c_str());
        }

        ImGui::End();
    }

}
