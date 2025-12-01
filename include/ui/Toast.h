#pragma once

#include <string>
#include <vector>
#include <chrono>
#include "imgui.h"

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
        float fade_progress; // 0.0 = fully visible, 1.0 = fully faded

        ToastMessage(const std::string& msg, ToastType t, float duration)
            : message(msg), type(t), duration_seconds(duration), fade_progress(0.0f) {
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

        // Render all active toasts
        static void RenderAll();

        // Clear all toasts
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