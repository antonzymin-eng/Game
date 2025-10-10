#pragma once

namespace ui {
    class Toast {
    public:
        static void Show(const char* message, float duration);
        static void RenderAll();
        
    private:
        Toast() = default;
        ~Toast() = default;
    };
}