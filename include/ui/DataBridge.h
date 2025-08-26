
#pragma once
#include <string>

namespace ui {

struct EconomyData { double treasury=0, income=0, expenses=0, inflation=0; };
struct MilitaryData { int armies=0, navies=0, manpower=0, forceLimit=0; };
struct DiplomacyData { int relations=0, truces=0, wars=0; };
struct EspionageData { int activeAgents=0, networks=0, alerts=0; };
struct TechData { int levelAdmin=0, levelDip=0, levelMil=0; };
struct RealmData { std::string date, rulerName, stateName; };

struct UIData {
    RealmData realm; EconomyData economy; MilitaryData military;
    DiplomacyData diplomacy; EspionageData espionage; TechData tech;
};

// ---- Header-only implementation ----
// These inline functions provide the single storage and solve linker errors.
inline UIData& Data() {
    static UIData gData;
    return gData;
}
inline const UIData& ReadOnlyData() {
    return Data();
}

} // namespace ui
