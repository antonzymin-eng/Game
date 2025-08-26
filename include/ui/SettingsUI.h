#pragma once
#include "ScreenAPI.h"
namespace ui {
    void ShowSettingsUI(AppState& app, bool* p_open = nullptr);
    void DrawSettings(AppState& app); // wrapper used by your main.cpp
}
