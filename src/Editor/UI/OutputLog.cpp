#include "Editor/UI/OutputLog.h"

#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <string>
#include <string_view>

namespace editor::ui {

    void OutputLog::add(std::string_view message, const LogLevel level) {
        constexpr std::size_t maxEntries = 1000;

        if (entries_.size() == maxEntries) {
            entries_.erase(entries_.begin());
        }

        const std::time_t now = std::time(nullptr);
        std::tm localTime{};
        localtime_s(&localTime, &now);

        char time[16]{};
        std::strftime(time, sizeof(time), "%H:%M:%S", &localTime);
        entries_.push_back({time, std::string(message), level});
        scrollToBottom_ = true;
    }

    void OutputLog::draw() {
        if (!ImGui::Begin("Output Log")) {
            ImGui::End();
            return;
        }

        const auto matches = [this](const Entry& entry) {
            const bool levelVisible =
                (entry.level == LogLevel::Info && showInfo_) ||
                (entry.level == LogLevel::Warning && showWarnings_) ||
                (entry.level == LogLevel::Error && showErrors_);

            if (!levelVisible || filter_[0] == '\0') {
                return levelVisible;
            }

            std::string message = entry.message;
            std::string filter = filter_;
            const auto lower = [](unsigned char character) {
                return static_cast<char>(std::tolower(character));
            };
            std::transform(message.begin(), message.end(), message.begin(), lower);
            std::transform(filter.begin(), filter.end(), filter.begin(), lower);
            return message.find(filter) != std::string::npos;
        };

        const auto levelName = [](const LogLevel level) {
            return level == LogLevel::Error ? "Error"
                : level == LogLevel::Warning ? "Warning" : "Info";
        };

        if (ImGui::Button("Clear")) {
            entries_.clear();
        }

        ImGui::SameLine();

        if (ImGui::Button("Copy")) {
            std::string content;

            for (const auto& entry : entries_) {
                if (matches(entry)) {
                    content += "[" + entry.time + "] [" +
                        levelName(entry.level) + "] " + entry.message + "\n";
                }
            }

            ImGui::SetClipboardText(content.c_str());
        }

        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &autoScroll_);
        ImGui::SameLine();
        ImGui::TextDisabled("%zu messages", entries_.size());

        ImGui::Checkbox("Info", &showInfo_);
        ImGui::SameLine();
        ImGui::Checkbox("Warnings", &showWarnings_);
        ImGui::SameLine();
        ImGui::Checkbox("Errors", &showErrors_);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(220.0f);
        ImGui::InputTextWithHint("##log_filter", "Search log", filter_, sizeof(filter_));

        ImGui::Separator();

        ImGui::BeginChild("##log_entries");

        for (const auto& entry : entries_) {
            if (!matches(entry)) {
                continue;
            }

            const ImVec4 color = entry.level == LogLevel::Error
                ? ImVec4(1.0f, 0.42f, 0.42f, 1.0f)
                : entry.level == LogLevel::Warning
                    ? ImVec4(1.0f, 0.75f, 0.35f, 1.0f)
                    : ImVec4(0.80f, 0.85f, 0.90f, 1.0f);

            ImGui::TextColored(color, "[%s] [%s]",
                entry.time.c_str(), levelName(entry.level));
            ImGui::SameLine();
            ImGui::TextUnformatted(entry.message.c_str());
        }

        if (scrollToBottom_ && autoScroll_) {
            ImGui::SetScrollHereY(1.0f);
        }

        scrollToBottom_ = false;
        ImGui::EndChild();

        ImGui::End();
    }

}
