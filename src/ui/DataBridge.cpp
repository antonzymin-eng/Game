
#include "ui/DataBridge.h"
namespace ui { static UIData gData; UIData& Data(){return gData;} const UIData& ReadOnlyData(){return gData;} }
