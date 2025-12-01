#include "ui/Toast.h"
#include "core/logging/Logger.h"
#include <algorithm>

namespace ui {
    // Toast display constants
    constexpr float TOAST_PADDING = 10.0f;
    constexpr float TOAST_WIDTH = 350.0f;
    constexpr float TOAST_SPACING = 10.0f;
    constexpr float TOAST_FADE_DURATION = 0.5f;
    constexpr float TOAST_SLIDE_DURATION = 0.3f;
    constexpr float TOAST_SLIDE_OFFSET = 50.0f;
    constexpr float TOAST_VERTICAL_PADDING = 20.0f;
    constexpr float TOAST_TEXT_WRAP_OFFSET = 70.0f;
    constexpr float TOAST_WINDOW_ROUNDING = 5.0f;
    constexpr float TOAST_WINDOW_PADDING = 10.0f;
    constexpr float TOAST_BACKGROUND_ALPHA = 0.95f;
    constexpr size_t MAX_ACTIVE_TOASTS = 5;

    void Toast::ShowTyped(const char* message, ToastType type, float duration) {
        auto& toasts = GetToasts();

        // Prevent unbounded toast queue growth
        if (toasts.size() >= MAX_ACTIVE_TOASTS) {
            toasts.erase(toasts.begin());  // Remove oldest toast
        }

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
        float y_offset = io.DisplaySize.y - TOAST_PADDING;

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

            // Calculate fade for last TOAST_FADE_DURATION seconds
            float fade_start = toast.duration_seconds - TOAST_FADE_DURATION;
            float alpha = 1.0f;
            if (elapsed > fade_start) {
                alpha = 1.0f - ((elapsed - fade_start) / TOAST_FADE_DURATION);
            }

            // Calculate slide-in animation for first TOAST_SLIDE_DURATION seconds
            float slide_progress = std::min(elapsed / TOAST_SLIDE_DURATION, 1.0f);
            float x_offset = (1.0f - slide_progress) * TOAST_SLIDE_OFFSET;

            // Get color and icon for this toast type
            ImVec4 color = GetColorForType(toast.type);
            color.w *= alpha; // Apply fade to alpha
            const char* icon = GetIconForType(toast.type);

            // Calculate toast height based on text (cached for performance)
            float toast_height;
            if (!toast.height_computed) {
                ImVec2 text_size = ImGui::CalcTextSize(toast.message.c_str(), nullptr, false,
                                                        TOAST_WIDTH - TOAST_TEXT_WRAP_OFFSET);
                toast.cached_height = text_size.y + TOAST_VERTICAL_PADDING;
                toast.height_computed = true;
                toast_height = toast.cached_height;
            } else {
                toast_height = toast.cached_height;
            }

            // Set toast position
            ImVec2 toast_pos(
                io.DisplaySize.x - TOAST_WIDTH - TOAST_PADDING + x_offset,
                y_offset - toast_height
            );

            // Set up window for this toast
            ImGui::SetNextWindowPos(toast_pos);
            ImGui::SetNextWindowSize(ImVec2(TOAST_WIDTH, toast_height));

            // Configure window style
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, TOAST_WINDOW_ROUNDING);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(TOAST_WINDOW_PADDING, TOAST_WINDOW_PADDING));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, TOAST_BACKGROUND_ALPHA * alpha));
            ImGui::PushStyleColor(ImGuiCol_Border, color);

            // Create unique window name using timestamp (safe from pointer invalidation)
            char window_name[64];
            auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                toast.creation_time.time_since_epoch()).count();
            snprintf(window_name, sizeof(window_name), "##toast_%lld", timestamp);

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
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + TOAST_WIDTH - TOAST_TEXT_WRAP_OFFSET);
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
