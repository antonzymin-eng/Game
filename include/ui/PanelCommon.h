
#pragma once
#include <string>
#include <algorithm>
#include "imgui.h"

// ImGui compatibility: older code calls GetContentRegionMax(), which was removed.
// Provide a thin wrapper mapping to GetContentRegionAvail().
namespace ImGui {
    inline ImVec2 GetContentRegionMax() { return GetContentRegionAvail(); }
}


namespace ui {

inline std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char)std::tolower(c); });
    return s;
}
inline bool matchesSearch(const std::string& hay, const char* needle) {
    if (!needle || !*needle) return true;
    auto h = toLower(hay);
    auto n = toLower(std::string(needle));
    return h.find(n) != std::string::npos;
}

} // namespace ui
