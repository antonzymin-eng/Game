#pragma once
#include <string>
#include <vector>

namespace ui {

// Center-screen toast manager with timed fade-out.
class ToastManager {
public:
    static ToastManager& I();
    void Add(const std::string& text, float duration_sec = 3.0f);
    void Render(); // call once per frame (after main content)

private:
    ToastManager() = default;
    struct Item {
        std::string text;
        double start_time = 0.0;
        float duration = 3.0f;
    };
    std::vector<Item> items_;
};

// Compatibility helpers: keep legacy ui::Toast(...) calls working
void Toast(const std::string& text, float duration_sec = 3.0f);
void Toast(const char* text, float duration_sec = 3.0f);

} // namespace ui