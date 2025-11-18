#pragma once

#include "ui/WindowManager.h"
#include "imgui.h"

namespace ui {
    /**
     * LeftSidebar - EU4-style left sidebar with system icons
     * Clicking icons opens corresponding windows
     */
    class LeftSidebar {
    public:
        LeftSidebar(WindowManager& window_manager);
        ~LeftSidebar() = default;

        void Render();

    private:
        WindowManager& window_manager_;

        struct SystemIcon {
            const char* label;
            const char* tooltip;
            WindowManager::WindowType window_type;
            ImU32 color;
            const char* icon_text; // Simple text icon for now (can be replaced with textures)
        };

        std::vector<SystemIcon> icons_;

        void InitializeIcons();
        void RenderIcon(const SystemIcon& icon, const ImVec2& pos, const ImVec2& size, bool is_active);
    };
}
