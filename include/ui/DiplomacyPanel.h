
#pragma once
#include "state/SimulationState.h"
namespace ui::panels {
struct DiplomacyPanel {
    static void Draw(core::SimulationState& sim, float width);
    static void DrawDetails(core::SimulationState& sim);
};
} // namespace ui::panels
