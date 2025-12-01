#include "ui/Toast.h"
#include "core/logging/Logger.h"
#include <algorithm>

namespace ui {
    void Toast::ShowTyped(const char* message, ToastType type, float duration) {
        auto& toasts = GetToasts();
        toasts.emplace_back(message, type, duration);

        // Also log to console for debugging
        switch (type) {
            case ToastType::SUCCESS:
                CORE_STREAM_INFO("Toast") << "[SUCCESS] " << message;
                break;
            case ToastType::ERROR:
                CORE_STREAM_ERROR("Toast") << "[ERROR] " << message;
                break;
            case ToastType::WARNING:
                CORE_STREAM_WARN("Toast") << "[WARNING] " << message;
                break;
            case ToastType::INFO:
                CORE_STREAM_INFO("Toast") << "[INFO] " << message;
                break;
        }
    }

    void Toast::RenderAll() {
        auto& toasts = GetToasts();
        if (toasts.empty()) {
            return;
        }

        // Get current time
        auto now = std::chrono::steady_clock::now();

        // Position toasts in bottom-right corner
        ImGuiIO& io = ImGui::GetIO();
        const float PADDING = 10.0f;
        const float TOAST_WIDTH = 350.0f;
        const float TOAST_SPACING = 10.0f;

        float y_offset = io.DisplaySize.y - PADDING;

        // Render toasts from bottom to top
        for (auto it = toasts.begin(); it != toasts.end(); ) {
            auto& toast = *it;

            // Calculate elapsed time
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - toast.creation_time).count() / 1000.0f;

            // Check if toast should be removed
            if (elapsed > toast.duration_seconds) {
                it = toasts.erase(it);
                continue;
            }

            // Calculate fade for last 0.5 seconds
            float fade_start = toast.duration_seconds - 0.5f;
            float alpha = 1.0f;
            if (elapsed > fade_start) {
                alpha = 1.0f - ((elapsed - fade_start) / 0.5f);
            }

            // Calculate slide-in animation for first 0.3 seconds
            float slide_progress = std::min(elapsed / 0.3f, 1.0f);
            float x_offset = (1.0f - slide_progress) * 50.0f; // Slide in from right

            // Get color and icon for this toast type
            ImVec4 color = GetColorForType(toast.type);
            color.w *= alpha; // Apply fade to alpha
            const char* icon = GetIconForType(toast.type);

            // Calculate toast height based on text
            ImVec2 text_size = ImGui::CalcTextSize(toast.message.c_str(), nullptr, false, TOAST_WIDTH - 60.0f);
            float toast_height = text_size.y + 20.0f; // Padding

            // Set toast position
            ImVec2 toast_pos(
                io.DisplaySize.x - TOAST_WIDTH - PADDING + x_offset,
                y_offset - toast_height
            );

            // Set up window for this toast
            ImGui::SetNextWindowPos(toast_pos);
            ImGui::SetNextWindowSize(ImVec2(TOAST_WIDTH, toast_height));

            // Configure window style
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.95f * alpha));
            ImGui::PushStyleColor(ImGuiCol_Border, color);

            // Create unique window name
            char window_name[64];
            snprintf(window_name, sizeof(window_name), "##toast_%p", (void*)&toast);

            // Render toast window
            ImGui::Begin(window_name, nullptr,
                ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoFocusOnAppearing |
                ImGuiWindowFlags_NoNav);

            // Render icon and message
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::Text("%s", icon);
            ImGui::PopStyleColor();

            ImGui::SameLine();
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + TOAST_WIDTH - 70.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, alpha));
            ImGui::TextWrapped("%s", toast.message.c_str());
            ImGui::PopStyleColor();
            ImGui::PopTextWrapPos();

            ImGui::End();

            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(2);

            // Move y_offset up for next toast
            y_offset -= (toast_height + TOAST_SPACING);

            ++it;
        }
    }

    void Toast::ClearAll() {
        GetToasts().clear();
    }

    std::vector<ToastMessage>& Toast::GetToasts() {
        static std::vector<ToastMessage> toasts;
        return toasts;
    }

    ImVec4 Toast::GetColorForType(ToastType type) {
        switch (type) {
            case ToastType::SUCCESS:
                return ImVec4(0.2f, 0.8f, 0.2f, 1.0f); // Green
            case ToastType::ERROR:
                return ImVec4(0.9f, 0.2f, 0.2f, 1.0f); // Red
            case ToastType::WARNING:
                return ImVec4(1.0f, 0.7f, 0.0f, 1.0f); // Orange
            case ToastType::INFO:
                return ImVec4(0.3f, 0.6f, 1.0f, 1.0f); // Blue
            default:
                return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White
        }
    }

    const char* Toast::GetIconForType(ToastType type) {
        switch (type) {
            case ToastType::SUCCESS:
                return "[✓]"; // Checkmark
            case ToastType::ERROR:
                return "[✗]"; // X mark
            case ToastType::WARNING:
                return "[!]"; // Exclamation
            case ToastType::INFO:
                return "[i]"; // Info
            default:
                return "[•]"; // Bullet
        }
    }
}
