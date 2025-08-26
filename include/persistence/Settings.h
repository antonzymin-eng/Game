#pragma once
namespace persist {

struct Settings {
    bool  vsync     = true;
    float masterVol = 0.8f;
    float uiScale   = 1.0f; // 0.75 .. 1.5
};

// Load/Save from ./config/settings.json
bool LoadSettings(Settings& out);
bool SaveSettings(const Settings& s);

// Apply to runtime (SDL/ImGui). Safe to call every frame if needed.
void ApplySettings(const Settings& s);

} // namespace persist
