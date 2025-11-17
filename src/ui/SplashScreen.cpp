#include "ui/SplashScreen.h"
#include "imgui.h"
#include <cmath>

namespace ui {

SplashScreen::SplashScreen()
    : advance_to_menu_(false)
    , fade_alpha_(0.0f)
    , start_time_(std::chrono::steady_clock::now()) {
}

void SplashScreen::Reset() {
    advance_to_menu_ = false;
    fade_alpha_ = 0.0f;
    start_time_ = std::chrono::steady_clock::now();
}

void SplashScreen::Render() {
    // Calculate fade-in effect
    auto current_time = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(current_time - start_time_).count();
    fade_alpha_ = std::min(1.0f, elapsed / 2.0f); // Fade in over 2 seconds

    // Full-screen window
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
                           | ImGuiWindowFlags_NoSavedSettings
                           | ImGuiWindowFlags_NoFocusOnAppearing
                           | ImGuiWindowFlags_NoNav
                           | ImGuiWindowFlags_NoMove
                           | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.13f, 0.14f, 0.06f, 1.0f)); // Dark brown background

    if (ImGui::Begin("##SplashScreen", nullptr, flags)) {
        RenderBackground();
        RenderTitle();
        RenderPrompt();

        // Check for any key press or mouse click
        if (elapsed > 1.0f) { // Prevent skipping too early
            // Check for common keys that should advance
            bool any_key_pressed = false;
            for (ImGuiKey key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key = (ImGuiKey)(key + 1)) {
                if (ImGui::IsKeyPressed(key)) {
                    any_key_pressed = true;
                    break;
                }
            }

            if (any_key_pressed || ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1)) {
                advance_to_menu_ = true;
            }
        }
    }
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void SplashScreen::RenderBackground() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 screen_size = viewport->Size;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Vignette effect - darken edges
    ImU32 vignette_center = IM_COL32(34, 24, 16, 0);
    ImU32 vignette_edge = IM_COL32(0, 0, 0, 180);

    draw_list->AddRectFilledMultiColor(
        viewport->Pos,
        ImVec2(viewport->Pos.x + screen_size.x, viewport->Pos.y + screen_size.y),
        vignette_center, vignette_center, vignette_edge, vignette_edge
    );
}

void SplashScreen::RenderTitle() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 screen_size = viewport->Size;

    // Center the title vertically (slightly above center)
    float title_y = screen_size.y * 0.35f;

    ImGui::SetCursorPos(ImVec2(0, title_y));

    // Main title
    const char* title = "MECHANICA IMPERII";
    ImGui::PushFont(ImGui::GetFont()); // Use default font for now

    // Calculate text size for centering
    ImVec2 title_size = ImGui::CalcTextSize(title);
    float title_scale = 3.5f;
    title_size.x *= title_scale;
    title_size.y *= title_scale;

    float title_x = (screen_size.x - title_size.x) * 0.5f;

    ImGui::SetCursorPos(ImVec2(title_x, title_y));

    // Golden color with fade
    ImU32 gold_color = IM_COL32(
        212, 175, 55,
        static_cast<int>(255 * fade_alpha_)
    );

    // Draw title with shadow effect
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 text_pos = ImVec2(viewport->Pos.x + title_x, viewport->Pos.y + title_y);

    // Shadow
    draw_list->AddText(
        ImGui::GetFont(),
        ImGui::GetFontSize() * title_scale,
        ImVec2(text_pos.x + 4, text_pos.y + 4),
        IM_COL32(0, 0, 0, static_cast<int>(180 * fade_alpha_)),
        title
    );

    // Main text
    draw_list->AddText(
        ImGui::GetFont(),
        ImGui::GetFontSize() * title_scale,
        text_pos,
        gold_color,
        title
    );

    // Subtitle
    const char* subtitle = "1000 - 1900 AD";
    ImVec2 subtitle_size = ImGui::CalcTextSize(subtitle);
    float subtitle_scale = 1.2f;
    subtitle_size.x *= subtitle_scale;

    float subtitle_x = (screen_size.x - subtitle_size.x) * 0.5f;
    float subtitle_y = title_y + title_size.y + 30.0f;

    ImU32 subtitle_color = IM_COL32(
        201, 169, 97,
        static_cast<int>(255 * fade_alpha_)
    );

    draw_list->AddText(
        ImGui::GetFont(),
        ImGui::GetFontSize() * subtitle_scale,
        ImVec2(viewport->Pos.x + subtitle_x, viewport->Pos.y + subtitle_y),
        subtitle_color,
        subtitle
    );

    // Version info
    const char* version = "Grand Strategy Simulation";
    ImVec2 version_size = ImGui::CalcTextSize(version);
    float version_x = (screen_size.x - version_size.x) * 0.5f;
    float version_y = subtitle_y + 35.0f;

    ImU32 version_color = IM_COL32(
        139, 115, 85,
        static_cast<int>(200 * fade_alpha_)
    );

    draw_list->AddText(
        ImVec2(viewport->Pos.x + version_x, viewport->Pos.y + version_y),
        version_color,
        version
    );

    ImGui::PopFont();
}

void SplashScreen::RenderPrompt() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 screen_size = viewport->Size;

    // Pulsing "Press any key" text
    auto current_time = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(current_time - start_time_).count();
    float pulse = (std::sin(elapsed * 2.0f) + 1.0f) * 0.5f; // Oscillate between 0 and 1
    pulse = 0.4f + (pulse * 0.6f); // Map to 0.4 - 1.0 range

    const char* prompt = "PRESS ANY KEY TO BEGIN";
    ImVec2 prompt_size = ImGui::CalcTextSize(prompt);
    float prompt_scale = 1.0f;
    prompt_size.x *= prompt_scale;

    float prompt_x = (screen_size.x - prompt_size.x) * 0.5f;
    float prompt_y = screen_size.y * 0.65f;

    ImU32 prompt_color = IM_COL32(
        212, 175, 55,
        static_cast<int>(255 * fade_alpha_ * pulse)
    );

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddText(
        ImVec2(viewport->Pos.x + prompt_x, viewport->Pos.y + prompt_y),
        prompt_color,
        prompt
    );

    // Copyright notice at bottom
    const char* copyright = "2025 MECHANICA IMPERII - ALL RIGHTS RESERVED";
    ImVec2 copyright_size = ImGui::CalcTextSize(copyright);
    float copyright_scale = 0.8f;
    copyright_size.x *= copyright_scale;
    copyright_size.y *= copyright_scale;

    float copyright_x = (screen_size.x - copyright_size.x) * 0.5f;
    float copyright_y = screen_size.y - 40.0f;

    ImU32 copyright_color = IM_COL32(
        107, 93, 79,
        static_cast<int>(180 * fade_alpha_)
    );

    draw_list->AddText(
        ImGui::GetFont(),
        ImGui::GetFontSize() * copyright_scale,
        ImVec2(viewport->Pos.x + copyright_x, viewport->Pos.y + copyright_y),
        copyright_color,
        copyright
    );
}

} // namespace ui
