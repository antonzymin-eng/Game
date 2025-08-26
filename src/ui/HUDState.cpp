
#include "ui/HUDState.h"

namespace ui {
static HUDState gHUD;
HUDState& GetHUDState()       { return gHUD; }
HUDState& MutableHUDState()   { return gHUD; }
} // namespace ui
