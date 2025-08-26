
#include "ui/AssetPath.h"
#include <cstdlib>
#include <filesystem>
namespace ui {
namespace fs = std::filesystem;
std::string ResolveAssetsRoot() {
    if (const char* env = std::getenv("MECH_ASSETS_DIR")) {
        fs::path p(env);
        std::error_code ec;
        if (fs::exists(p, ec)) return p.string();
    }
    fs::path cur = fs::current_path();
    for (int i = 0; i < 6; ++i) {
        fs::path candidate = cur / "assets";
        std::error_code ec;
        if (fs::exists(candidate, ec) && fs::is_directory(candidate, ec)) {
            return candidate.string();
        }
        if (cur.has_parent_path()) cur = cur.parent_path();
        else break;
    }
    return (fs::current_path() / "assets").string();
}
} // namespace ui
