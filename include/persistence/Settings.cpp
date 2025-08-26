#include "persistence/Settings.h"
#include <filesystem>
#include <fstream>
#include <string>

#include "imgui.h"
#ifdef __has_include
#  if __has_include(<SDL.h>)
#    include <SDL.h>
#    define HAVE_SDL 1
#  else
#    define HAVE_SDL 0
#  endif
#else
#  define HAVE_SDL 0
#endif

namespace fs = std::filesystem;

namespace persist {

static fs::path ConfigDir() { return fs::path("config"); }
static fs::path SettingsPath() { return ConfigDir() / "settings.json"; }

static bool ensureDir(const fs::path& p) {
    std::error_code ec;
    fs::create_directories(p, ec);
    return !ec;
}

// Extremely tiny JSON-ish writer/reader for 3 keys (avoids external deps).
static std::string to_json(const Settings& s) {
    char buf[256];
    std::snprintf(buf, sizeof(buf),
        "{\n"
        "  \"vsync\": %s,\n"
        "  \"masterVol\": %.2f,\n"
        "  \"uiScale\": %.2f\n"
        "}\n",
        s.vsync ? "true" : "false", s.masterVol, s.uiScale);
    return std::string(buf);
}

static bool find_bool(const std::string& js, const char* key, bool& out) {
    auto k = std::string("\"") + key + "\":";
    auto pos = js.find(k);
    if (pos == std::string::npos) return false;
    pos += k.size();
    while (pos < js.size() && (js[pos]==' '||js[pos]=='\t')) ++pos;
    if (js.compare(pos, 4, "true") == 0) { out = true; return true; }
    if (js.compare(pos, 5, "false")== 0) { out = false; return true; }
    return false;
}
static bool find_float(const std::string& js, const char* key, float& out) {
    auto k = std::string("\"") + key + "\":";
    auto pos = js.find(k);
    if (pos == std::string::npos) return false;
    pos = js.find_first_of("-0123456789", pos + k.size());
    if (pos == std::string::npos) return false;
    out = std::strtof(js.c_str() + pos, nullptr);
    return true;
}

bool LoadSettings(Settings& out) {
    auto p = SettingsPath();
    std::ifstream in(p, std::ios::binary);
    if (!in) return false;
    std::string js((std::istreambuf_iterator<char>(in)), std::istream_iterator<char>());
    (void)find_bool(js, "vsync", out.vsync);
    (void)find_float(js, "masterVol", out.masterVol);
    (void)find_float(js, "uiScale", out.uiScale);
    return true;
}

bool SaveSettings(const Settings& s) {
    ensureDir(ConfigDir());
    auto p = SettingsPath();
    std::ofstream out(p, std::ios::binary);
    if (!out) return false;
    auto js = to_json(s);
    out.write(js.data(), static_cast<std::streamsize>(js.size()));
    return static_cast<bool>(out);
}

void ApplySettings(const Settings& s) {
#if HAVE_SDL
    SDL_GL_SetSwapInterval(s.vsync ? 1 : 0);
#endif
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = (s.uiScale > 0.2f && s.uiScale < 3.0f) ? s.uiScale : 1.0f;
    // Master volume: wire to your audio mixer if/when available.
}

} // namespace persist
