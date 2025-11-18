#include "ui/SaveLoadDialog.h"
#include <cstring>
#include <algorithm>

namespace ui {

SaveLoadDialog::SaveLoadDialog()
    : current_mode_(Mode::SAVE)
    , is_open_(false)
    , has_pending_operation_(false)
    , selected_index_(-1) {
    std::memset(new_save_name_, 0, sizeof(new_save_name_));
    std::strcpy(new_save_name_, "New Save");
}

void SaveLoadDialog::Show(Mode mode) {
    current_mode_ = mode;
    is_open_ = true;
    has_pending_operation_ = false;
    selected_index_ = -1;
    RefreshSaveFileList();
}

void SaveLoadDialog::Close() {
    is_open_ = false;
}

void SaveLoadDialog::Render() {
    if (!is_open_) return;

    // Modal dialog
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(700, 500), ImGuiCond_Appearing);

    const char* title = (current_mode_ == Mode::SAVE) ? "Save Game" : "Load Game";

    if (ImGui::BeginPopupModal(title, &is_open_, ImGuiWindowFlags_NoResize)) {
        if (current_mode_ == Mode::SAVE) {
            RenderSaveMode();
        } else {
            RenderLoadMode();
        }

        ImGui::EndPopup();
    }

    // Open the modal if not already open
    if (is_open_ && !ImGui::IsPopupOpen(title)) {
        ImGui::OpenPopup(title);
    }
}

void SaveLoadDialog::RenderSaveMode() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("SAVE GAME");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    // Save name input
    ImGui::Text("Save Name:");
    ImGui::SetNextItemWidth(400);
    ImGui::InputText("##savename", new_save_name_, sizeof(new_save_name_));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Existing saves list
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
    ImGui::Text("Existing Saves (click to overwrite):");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    // Save file list
    ImGui::BeginChild("SaveList", ImVec2(0, 300), true);

    for (size_t i = 0; i < save_files_.size(); ++i) {
        const auto& save = save_files_[i];

        bool is_selected = (static_cast<int>(i) == selected_index_);
        if (ImGui::Selectable(save.display_name.c_str(), is_selected)) {
            selected_index_ = static_cast<int>(i);
            std::strcpy(new_save_name_, save.display_name.c_str());
        }

        ImGui::SameLine(500);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
        ImGui::Text("%s", save.date_string.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::EndChild();

    // Action buttons
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Save", ImVec2(120, 0))) {
        if (std::strlen(new_save_name_) > 0) {
            selected_save_file_ = std::string(new_save_name_);
            has_pending_operation_ = true;
            is_open_ = false;
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        is_open_ = false;
    }
}

void SaveLoadDialog::RenderLoadMode() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("LOAD GAME");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
    ImGui::Text("Select a save file to load:");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    // Save file list with details
    ImGui::BeginChild("LoadList", ImVec2(0, 350), true);

    for (size_t i = 0; i < save_files_.size(); ++i) {
        const auto& save = save_files_[i];

        bool is_selected = (static_cast<int>(i) == selected_index_);

        ImGui::PushID(static_cast<int>(i));

        if (ImGui::Selectable("##select", is_selected, 0, ImVec2(0, 60))) {
            selected_index_ = static_cast<int>(i);
        }

        ImGui::SameLine();
        ImGui::BeginGroup();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
        ImGui::Text("%s", save.display_name.c_str());
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
        ImGui::Text("  Nation: %s | Turn: %d", save.nation_name.c_str(), save.turn_number);
        ImGui::Text("  Saved: %s", save.date_string.c_str());
        ImGui::PopStyleColor();

        ImGui::EndGroup();

        ImGui::PopID();
        ImGui::Spacing();
    }

    ImGui::EndChild();

    // Action buttons
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    bool has_selection = (selected_index_ >= 0 && selected_index_ < static_cast<int>(save_files_.size()));

    if (!has_selection) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Load", ImVec2(120, 0))) {
        if (has_selection) {
            selected_save_file_ = save_files_[selected_index_].filename;
            has_pending_operation_ = true;
            is_open_ = false;
        }
    }

    if (!has_selection) {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        is_open_ = false;
    }

    if (!has_selection) {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::Text("  Please select a save file");
        ImGui::PopStyleColor();
    }
}

void SaveLoadDialog::RefreshSaveFileList() {
    save_files_.clear();

    // TODO: Scan save directory for .sav files
    // For now, add some example save files

    SaveFileInfo save1;
    save1.filename = "autosave_001.sav";
    save1.display_name = "Autosave 001";
    save1.timestamp = std::time(nullptr) - 3600; // 1 hour ago
    save1.date_string = FormatTimestamp(save1.timestamp);
    save1.turn_number = 145;
    save1.nation_name = "Kingdom of Francia";
    save_files_.push_back(save1);

    SaveFileInfo save2;
    save2.filename = "quicksave.sav";
    save2.display_name = "Quick Save";
    save2.timestamp = std::time(nullptr) - 7200; // 2 hours ago
    save2.date_string = FormatTimestamp(save2.timestamp);
    save2.turn_number = 132;
    save2.nation_name = "Kingdom of Francia";
    save_files_.push_back(save2);

    SaveFileInfo save3;
    save3.filename = "campaign_start.sav";
    save3.display_name = "Campaign Start";
    save3.timestamp = std::time(nullptr) - 86400; // 1 day ago
    save3.date_string = FormatTimestamp(save3.timestamp);
    save3.turn_number = 1;
    save3.nation_name = "Kingdom of Francia";
    save_files_.push_back(save3);

    // Sort by timestamp (newest first)
    std::sort(save_files_.begin(), save_files_.end(),
              [](const SaveFileInfo& a, const SaveFileInfo& b) {
                  return a.timestamp > b.timestamp;
              });
}

std::string SaveLoadDialog::FormatTimestamp(std::time_t timestamp) const {
    char buffer[64];
    std::tm* tm_info = std::localtime(&timestamp);
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", tm_info);
    return std::string(buffer);
}

} // namespace ui
