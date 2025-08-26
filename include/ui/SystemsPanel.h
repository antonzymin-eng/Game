#pragma once
#include <cstdint>

// Simple, non-invasive systems dock for Mechanica Imperii.
// Drop-in: include this header where you render HUD panels, then call ui::DrawSystemsPanels().
// It draws a bottom "Systems Dock" window with tabs for all major systems.
// The state is internal and persists across frames; no changes to AppState are required.

namespace ui {

// Call once per frame from Game Screen (after top bar and before end-of-frame ImGui).
void DrawSystemsPanels();

} // namespace ui