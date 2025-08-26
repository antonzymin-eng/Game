#pragma once
#include <string>
namespace ui {
struct SelectionContext {
    std::string province;        // current province scope (optional)
    std::string selectedGood;    // last selected market good
    int selectedArmyId = -1;     // last selected army
    int selectedAgentId = -1;    // last selected agent
};

SelectionContext& UX(); // singleton accessor

// Helpers
inline void ScopeProvince(const std::string& name) { UX().province = name; }
inline void ClearProvinceScope() { UX().province.clear(); }
} // namespace ui
