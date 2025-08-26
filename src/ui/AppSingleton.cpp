// src/ui/AppSingleton.cpp
// Global UI AppState accessor implementation.
// Declared in ScreenAPI.h as: namespace ui { AppState& App(); }

#include "ScreenAPI.h"

namespace ui {

AppState& App() {
    static AppState g_app;
    return g_app;
}

} // namespace ui
