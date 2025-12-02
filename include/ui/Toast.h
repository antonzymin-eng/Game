#pragma once

#include <string>
#include <vector>
#include <chrono>
#include "imgui.h"

/**
 * Toast Notification System
 *
 * Provides a simple, globally-accessible API for showing temporary notifications
 * in the bottom-right corner of the screen.
 *
 * USAGE:
 *   Toast::ShowSuccess("Operation completed!");
 *   Toast::ShowError("Failed to save file");
 *   Toast::ShowWarning("Low disk space");
 *   Toast::ShowInfo("Loading...");
 *
 * FEATURES:
 *   - 4 notification types with color coding (Success, Error, Warning, Info)
 *   - Auto-dismiss with configurable duration
 *   - Smooth slide-in and fade-out animations
 *   - Stack multiple toasts vertically
 *   - Automatic queue management (max 5 toasts)
 *
 * THREAD SAFETY:
 *   ⚠️ NOT THREAD-SAFE. Must be called from UI thread only.
 *   All Toast:: methods must be invoked from the same thread that calls RenderAll().
 *
 * TESTING:
 *   Use Toast::ClearAll() to reset state between unit tests.
 *
 * INTEGRATION:
 *   Call Toast::RenderAll() once per frame in your main UI render loop,
 *   typically at the end so toasts appear on top of other UI elements.
 */

namespace ui {
    enum class ToastType {
        SUCCESS,
        ERROR,
        WARNING,
        INFO
    };

    struct ToastMessage {
        std::string message;
        ToastType type;
        std::chrono::steady_clock::time_point creation_time;
        float duration_seconds;
        mutable float cached_height; // Cached height, computed on first render
        mutable bool height_computed;

        ToastMessage(const std::string& msg, ToastType t, float duration)
            : message(msg), type(t), duration_seconds(duration),
              cached_height(0.0f), height_computed(false) {
            creation_time = std::chrono::steady_clock::now();
        }
    };

    class Toast {
    public:
        // Show a toast notification
        static void Show(const char* message, float duration = 3.0f) {
            ShowTyped(message, ToastType::INFO, duration);
        }

        // Show typed toast notifications
        static void ShowSuccess(const char* message, float duration = 3.0f) {
            ShowTyped(message, ToastType::SUCCESS, duration);
        }

        static void ShowError(const char* message, float duration = 5.0f) {
            ShowTyped(message, ToastType::ERROR, duration);
        }

        static void ShowWarning(const char* message, float duration = 4.0f) {
            ShowTyped(message, ToastType::WARNING, duration);
        }

        static void ShowInfo(const char* message, float duration = 3.0f) {
            ShowTyped(message, ToastType::INFO, duration);
        }

        // Render all active toasts (call once per frame in main UI loop)
        static void RenderAll();

        // Clear all toasts (useful for testing to reset state between test cases)
        static void ClearAll();

    private:
        static void ShowTyped(const char* message, ToastType type, float duration);
        static std::vector<ToastMessage>& GetToasts();
        static ImVec4 GetColorForType(ToastType type);
        static const char* GetIconForType(ToastType type);

        Toast() = default;
        ~Toast() = default;
    };
}