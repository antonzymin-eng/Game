#include "ui/WindowManager.h"
#include <stdexcept>

namespace ui {

WindowManager::WindowManager() {
    InitializeDefaultStates();
}

void WindowManager::InitializeDefaultStates() {
    // Initialize all window types with default states
    window_states_[WindowType::NATION_OVERVIEW] = WindowState{false, false, ImVec2(100, 100), ImVec2(500, 600), 0, false};
    window_states_[WindowType::ECONOMY] = WindowState{false, false, ImVec2(100, 100), ImVec2(600, 500), 0, false};
    window_states_[WindowType::MILITARY] = WindowState{false, false, ImVec2(100, 100), ImVec2(600, 500), 0, false};
    window_states_[WindowType::DIPLOMACY] = WindowState{false, false, ImVec2(100, 100), ImVec2(600, 500), 0, false};
    window_states_[WindowType::TECHNOLOGY] = WindowState{false, false, ImVec2(100, 100), ImVec2(700, 600), 0, false};
    window_states_[WindowType::POPULATION] = WindowState{false, false, ImVec2(100, 100), ImVec2(500, 500), 0, false};
    window_states_[WindowType::TRADE] = WindowState{false, false, ImVec2(100, 100), ImVec2(700, 600), 0, false};
    window_states_[WindowType::REALM] = WindowState{false, false, ImVec2(100, 100), ImVec2(500, 500), 0, false};
    window_states_[WindowType::ADMINISTRATION] = WindowState{false, false, ImVec2(100, 100), ImVec2(600, 500), 0, false};
    window_states_[WindowType::PROVINCE_INFO] = WindowState{false, true, ImVec2(10, 10), ImVec2(350, 400), 0, false};
    window_states_[WindowType::PERFORMANCE] = WindowState{false, true, ImVec2(10, 10), ImVec2(400, 300), 0, false};
}

bool WindowManager::IsWindowOpen(WindowType type) const {
    auto it = window_states_.find(type);
    return it != window_states_.end() && it->second.is_open;
}

bool WindowManager::IsWindowPinned(WindowType type) const {
    auto it = window_states_.find(type);
    return it != window_states_.end() && it->second.is_pinned;
}

void WindowManager::SetWindowOpen(WindowType type, bool open) {
    window_states_[type].is_open = open;
}

void WindowManager::SetWindowPinned(WindowType type, bool pinned) {
    window_states_[type].is_pinned = pinned;
}

void WindowManager::ToggleWindow(WindowType type) {
    bool will_be_open = !window_states_[type].is_open;

    // If opening a window, close all other unpinned windows
    if (will_be_open) {
        for (auto& [window_type, state] : window_states_) {
            if (window_type != type && !state.is_pinned && state.is_open) {
                state.is_open = false;
            }
        }
    }

    window_states_[type].is_open = will_be_open;
}

void WindowManager::CloseWindow(WindowType type) {
    window_states_[type].is_open = false;
}

WindowManager::WindowState& WindowManager::GetWindowState(WindowType type) {
    auto it = window_states_.find(type);
    if (it == window_states_.end()) {
        throw std::runtime_error("Window type not found in WindowManager");
    }
    return it->second;
}

const WindowManager::WindowState& WindowManager::GetWindowState(WindowType type) const {
    auto it = window_states_.find(type);
    if (it == window_states_.end()) {
        throw std::runtime_error("Window type not found in WindowManager");
    }
    return it->second;
}

ImGuiWindowFlags WindowManager::GetWindowFlags(WindowType type) const {
    ImGuiWindowFlags flags = 0;

    const auto& state = GetWindowState(type);

    if (!state.is_pinned) {
        // Unpinned windows can be moved and resized
        flags |= ImGuiWindowFlags_NoCollapse;
    } else {
        // Pinned windows are fixed in place
        flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    }

    return flags;
}

bool WindowManager::BeginManagedWindow(WindowType type, const char* title, ImGuiWindowFlags extra_flags) {
    auto& state = GetWindowState(type);

    if (!state.is_open) {
        return false;
    }

    // Set window position and size if pinned
    if (state.is_pinned) {
        ImGui::SetNextWindowPos(state.position, ImGuiCond_Always);
        ImGui::SetNextWindowSize(state.size, ImGuiCond_Always);
    } else {
        ImGui::SetNextWindowPos(state.position, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(state.size, ImGuiCond_FirstUseEver);
    }

    ImGuiWindowFlags flags = GetWindowFlags(type) | extra_flags;

    bool is_open = state.is_open;
    bool result = ImGui::Begin(title, &is_open, flags);

    // Update open state if close button was clicked
    if (!is_open) {
        state.is_open = false;
    }

    if (result) {
        // Store current position and size for unpinned windows
        if (!state.is_pinned) {
            state.position = ImGui::GetWindowPos();
            state.size = ImGui::GetWindowSize();
        }

        // Render pin/unpin button in title bar
        const char* pin_label = state.is_pinned ? "Unpin" : "Pin";
        float button_width = ImGui::CalcTextSize(pin_label).x + ImGui::GetStyle().FramePadding.x * 2.0f;
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - button_width - ImGui::GetStyle().WindowPadding.x);
        ImGui::SetCursorPosY(ImGui::GetStyle().WindowPadding.y);
        if (ImGui::SmallButton(pin_label)) {
            state.is_pinned = !state.is_pinned;
        }

        // Show tooltip
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s window to %s it",
                state.is_pinned ? "Unpin" : "Pin",
                state.is_pinned ? "allow moving/resizing" : "lock in place");
        }
    }

    return result;
}

void WindowManager::EndManagedWindow() {
    ImGui::End();
}

void WindowManager::ResetAllWindows() {
    InitializeDefaultStates();
}

} // namespace ui
