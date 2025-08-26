#pragma once
#include "ScreenAPI.h"

namespace ui {

void DrawMainMenu(AppState& app);
void DrawNewGame(AppState& app);
void DrawSettings(AppState& app);
void DrawCredits(AppState& app);

void CenterNextWindow(float w, float h, float y_offset = 0.0f);
void FillViewportBackground(float alpha = 1.0f);

} // namespace ui
