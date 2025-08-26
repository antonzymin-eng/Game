#pragma once
#include <string>

namespace io {
    // Resolve a relative asset path like "assets/maps/europe_1000/map.png"
    // Searches: working dir, MECH_ASSETS_DIR override, and one dir up ("../")
    std::string FindAsset(const std::string& rel);
}
