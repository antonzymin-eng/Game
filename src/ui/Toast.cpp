#include "ui/Toast.h"
#include "core/logging/Logger.h"
#include <algorithm>

/**
 * Toast Notification System Implementation
 *
 * DESIGN DECISIONS:
 *
 * 1. SINGLETON PATTERN (Static Storage):
 *    The toast queue uses static storage (function-local static) to provide
 *    a simple, globally-accessible API without requiring dependency injection.
 *
 *    Rationale:
 *    - Toasts are UI-level notifications needed everywhere
 *    - Passing a ToastManager reference to every UI window is verbose
 *    - Simple API: Toast::ShowSuccess("message") is more ergonomic
 *    - No multi-instance use case (only one toast queue needed)
 *
 *    Trade-offs:
 *    - Testing: Use Toast::ClearAll() to reset state between tests
 *    - Lifetime: Static lifetime (acceptable for UI singleton)
 *    - Thread safety: See below
 *
 * 2. THREAD SAFETY:
 *    ⚠️ WARNING: This implementation is NOT thread-safe.
 *
 *    Requirement: ALL Toast methods must be called from the UI thread only.
 *
 *    Rationale:
 *    - ImGui rendering is single-threaded
 *    - Game UI systems typically run on main thread
 *    - Adding mutex would add overhead for no benefit in single-threaded context
 *
 *    Future: If multi-threading is needed, add std::mutex to GetToasts()
 *
 * 3. PERFORMANCE OPTIMIZATIONS:
 *    - Text height cached on first render (avoids repeated CalcTextSize calls)
 *    - Max toast limit prevents unbounded memory growth
 *    - Compile-time constants allow compiler optimizations
 *
 * 4. PORTABILITY:
 *    - USE_ASCII_ICONS flag for systems without UTF-8 font support
 *    - Set to true by default for maximum compatibility
 */

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

    // Toast color constants
    constexpr ImVec4 TOAST_COLOR_SUCCESS{0.2f, 0.8f, 0.2f, 1.0f};  // Green
    constexpr ImVec4 TOAST_COLOR_ERROR{0.9f, 0.2f, 0.2f, 1.0f};    // Red
    constexpr ImVec4 TOAST_COLOR_WARNING{1.0f, 0.7f, 0.0f, 1.0f};  // Orange
    constexpr ImVec4 TOAST_COLOR_INFO{0.3f, 0.6f, 1.0f, 1.0f};     // Blue
    constexpr ImVec4 TOAST_COLOR_DEFAULT{1.0f, 1.0f, 1.0f, 1.0f};  // White
    constexpr ImVec4 TOAST_BG_COLOR{0.1f, 0.1f, 0.1f, 1.0f};       // Dark gray
    constexpr ImVec4 TOAST_TEXT_COLOR{1.0f, 1.0f, 1.0f, 1.0f};     // White

    // Use ASCII icons for maximum portability (UTF-8 icons may not render on all systems)
    // Set to 0 to use Unicode icons (✓, ✗, !, i) if font supports them
    constexpr bool USE_ASCII_ICONS = true;

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
            ImVec4 bg_color = TOAST_BG_COLOR;
            bg_color.w = TOAST_BACKGROUND_ALPHA * alpha;
            ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_color);
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
            ImVec4 text_color = TOAST_TEXT_COLOR;
            text_color.w = alpha;
            ImGui::PushStyleColor(ImGuiCol_Text, text_color);
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
                return TOAST_COLOR_SUCCESS;
            case ToastType::ERROR:
                return TOAST_COLOR_ERROR;
            case ToastType::WARNING:
                return TOAST_COLOR_WARNING;
            case ToastType::INFO:
                return TOAST_COLOR_INFO;
            default:
                return TOAST_COLOR_DEFAULT;
        }
    }

    const char* Toast::GetIconForType(ToastType type) {
        if (USE_ASCII_ICONS) {
            // ASCII fallback for maximum compatibility
            switch (type) {
                case ToastType::SUCCESS:
                    return "[+]"; // Plus for success
                case ToastType::ERROR:
                    return "[X]"; // X for error
                case ToastType::WARNING:
                    return "[!]"; // Exclamation for warning
                case ToastType::INFO:
                    return "[i]"; // i for info
                default:
                    return "[*]"; // Asterisk
            }
        } else {
            // UTF-8 icons (requires font support)
            switch (type) {
                case ToastType::SUCCESS:
                    return "[✓]"; // Unicode checkmark U+2713
                case ToastType::ERROR:
                    return "[✗]"; // Unicode X mark U+2717
                case ToastType::WARNING:
                    return "[!]"; // Exclamation
                case ToastType::INFO:
                    return "[i]"; // Info
                default:
                    return "[•]"; // Unicode bullet U+2022
            }
        }
    }
}
