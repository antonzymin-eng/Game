#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include "imgui.h"

namespace ui {
    /**
     * WindowManager - Manages all game windows with pin/unpin, close, and tabbing support
     * EU4-style window management system
     */
    class WindowManager {
    public:
        enum class WindowType {
            NATION_OVERVIEW,
            ECONOMY,
            MILITARY,
            DIPLOMACY,
            TECHNOLOGY,
            POPULATION,
            TRADE,
            REALM,
            PROVINCE_INFO,
            SETTINGS,
            PERFORMANCE,
            ADMINISTRATION
        };

        struct WindowState {
            bool is_open = false;
            bool is_pinned = false;
            ImVec2 position = ImVec2(0, 0);
            ImVec2 size = ImVec2(400, 500);
            int active_tab = 0;

            // For docked windows
            bool is_docked_to_sidebar = false;
        };

        WindowManager();
        ~WindowManager() = default;

        // Window state management
        bool IsWindowOpen(WindowType type) const;
        bool IsWindowPinned(WindowType type) const;
        void SetWindowOpen(WindowType type, bool open);
        void SetWindowPinned(WindowType type, bool pinned);
        void ToggleWindow(WindowType type);
        void CloseWindow(WindowType type);

        // Get window state
        WindowState& GetWindowState(WindowType type);
        const WindowState& GetWindowState(WindowType type) const;

        // Helper for rendering with pin/close buttons
        bool BeginManagedWindow(WindowType type, const char* title, ImGuiWindowFlags extra_flags = 0);
        void EndManagedWindow();

        // Reset all windows to default state
        void ResetAllWindows();

    private:
        std::unordered_map<WindowType, WindowState> window_states_;

        void InitializeDefaultStates();
        ImGuiWindowFlags GetWindowFlags(WindowType type) const;
    };
}
