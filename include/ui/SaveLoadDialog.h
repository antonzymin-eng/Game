#pragma once

#include "imgui.h"
#include <string>
#include <vector>
#include <ctime>

namespace ui {
    /**
     * SaveLoadDialog - Modal dialog for saving and loading games
     * Displays list of save files with timestamps and metadata
     */
    class SaveLoadDialog {
    public:
        enum class Mode {
            SAVE,
            LOAD
        };

        SaveLoadDialog();
        ~SaveLoadDialog() = default;

        // Show the dialog
        void Show(Mode mode);
        void Close();

        // Render the dialog (call every frame)
        void Render();

        // Check if a save/load operation was requested
        bool HasPendingOperation() const { return has_pending_operation_; }
        std::string GetSelectedSaveFile() const { return selected_save_file_; }
        std::string GetNewSaveName() const { return new_save_name_; }
        Mode GetMode() const { return current_mode_; }
        void ClearPendingOperation() { has_pending_operation_ = false; }

    private:
        struct SaveFileInfo {
            std::string filename;
            std::string display_name;
            std::time_t timestamp;
            std::string date_string;
            int turn_number;
            std::string nation_name;
        };

        Mode current_mode_;
        bool is_open_;
        bool has_pending_operation_;
        std::string selected_save_file_;
        char new_save_name_[256];

        std::vector<SaveFileInfo> save_files_;
        int selected_index_;

        void RefreshSaveFileList();
        void RenderSaveMode();
        void RenderLoadMode();
        std::string FormatTimestamp(std::time_t timestamp) const;
    };
}
