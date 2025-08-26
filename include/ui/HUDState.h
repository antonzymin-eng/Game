
#pragma once
#include <string>

namespace ui {

struct HUDState {
    bool showProvinces = false;
    bool showTrade     = false;
    bool showTerrain   = true;
    bool showBorders   = true;
    bool showRivers    = true;

    int         selectionProvinceId = -1;
    std::string selectionName;
    int         hoveredProvinceId = -1;
    std::string hoveredName;
};

HUDState& GetHUDState();
HUDState& MutableHUDState();

} // namespace ui

inline ui::HUDState& GetHUDState() { return ui::GetHUDState(); }
