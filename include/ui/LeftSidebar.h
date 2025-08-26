
#pragma once
#include "state/SimulationState.h"
struct AppState;
namespace ui {
void DrawLeftSidebar(AppState& app, core::SimulationState& sim, float width);
void DrawPanelDetails(AppState& app, core::SimulationState& sim);
} // namespace ui
