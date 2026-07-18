#include <editor/editor.hpp>
#include <logger.hpp>
#include <ctime>

namespace vke_editor
{
    static ImVec4 LogLevelColor(spdlog::level::level_enum level)
    {
        switch (level)
        {
        case spdlog::level::warn:
            return ImVec4(1.0f, 0.75f, 0.2f, 1.0f);
        case spdlog::level::err:
        case spdlog::level::critical:
            return ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
        case spdlog::level::debug:
        case spdlog::level::trace:
            return ImVec4(0.6f, 0.7f, 0.8f, 1.0f);
        default:
            return ImGui::GetStyleColorVec4(ImGuiCol_Text);
        }
    }

    void Editor::showLog()
    {
        ImGui::Begin("Log");

        static ImGuiTextFilter filter;
        static bool autoScroll = true;
        static vke_common::LogID lastSeenLogID = 0;

        if (ImGui::Button("Clear"))
            vke_common::Logger::ClearEntries();
        ImGui::SameLine();
        filter.Draw("Filter", 240.0f);
        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &autoScroll);
        ImGui::Separator();

        const vke_common::LogID latestLogID = vke_common::Logger::GetLatestEntryID();
        const bool hasNewEntries = latestLogID > lastSeenLogID;

        ImGui::BeginChild("LogEntries", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None,
                          ImGuiWindowFlags_HorizontalScrollbar);
        vke_common::Logger::IterateEntries(0, latestLogID, [&](const vke_common::LogEntry &entry)
        {
            if (!filter.PassFilter(entry.message.c_str()))
                return;

            const std::time_t timestamp = std::chrono::system_clock::to_time_t(entry.timestamp);
            std::tm localTime{};
            localtime_s(&localTime, &timestamp);
            char timeBuffer[16]{};
            std::strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", &localTime);

            ImGui::PushStyleColor(ImGuiCol_Text, LogLevelColor(entry.level));
            ImGui::TextUnformatted(timeBuffer);
            ImGui::SameLine();
            const spdlog::string_view_t levelName = spdlog::level::to_string_view(entry.level);
            ImGui::TextUnformatted(levelName.data(), levelName.data() + levelName.size());
            ImGui::SameLine();
            ImGui::TextUnformatted(entry.message.c_str());
            ImGui::PopStyleColor();
        });
        lastSeenLogID = latestLogID;

        if (autoScroll && hasNewEntries)
            ImGui::SetScrollHereY(1.0f);
        ImGui::EndChild();
        ImGui::End();
    }
}
