#include <string>
#include <cstdlib>
#include <array>
#include <filesystem>

namespace fs = std::filesystem;

namespace io {

// Robust but conservative asset resolver. Tries MECH_ASSETS_DIR env, then common relative paths.
// Returns the first existing path; otherwise returns the input unchanged (so caller can still try).
std::string FindAsset(const std::string& name) {
    // If absolute and exists, just return it
    fs::path p(name);
    if (p.is_absolute() && fs::exists(p)) return p.string();

    // 1) MECH_ASSETS_DIR/name
    if (const char* env = std::getenv("MECH_ASSETS_DIR")) {
        fs::path base(env);
        fs::path candidate = base / name;
        if (fs::exists(candidate)) return candidate.string();
    }

    // 2) try common relative locations
    const std::array<const char*, 8> bases = {
        "assets", "./assets", "../assets", "../../assets",
        "data", "./data", "../data", "../../data"
    };
    for (auto b : bases) {
        fs::path candidate = fs::path(b) / name;
        if (fs::exists(candidate)) return candidate.string();
    }

    // 3) If no variant exists, return original (caller may handle missing gracefully)
    return name;
}

} // namespace io