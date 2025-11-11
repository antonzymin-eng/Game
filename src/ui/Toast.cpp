#include "ui/Toast.h"

#include "core/logging/Logger.h"

namespace ui {
    void Toast::Show(const char* message, float duration) {
        // Placeholder implementation - just print to console
        CORE_STREAM_INFO("Toast") << message << " (duration: " << duration << "s)";
    }
    
    void Toast::RenderAll() {
        // Placeholder implementation - no rendering yet
    }
}