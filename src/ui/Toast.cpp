#include "ui/Toast.h"
#include <iostream>

namespace ui {
    void Toast::Show(const char* message, float duration) {
        // Placeholder implementation - just print to console
        std::cout << "[Toast] " << message << " (duration: " << duration << "s)" << std::endl;
    }
    
    void Toast::RenderAll() {
        // Placeholder implementation - no rendering yet
    }
}