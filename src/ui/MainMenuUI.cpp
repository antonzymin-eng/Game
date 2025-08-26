#include "imgui.h"
#include "ScreenAPI.h"
#include "ui/SaveLoadUI.h"
#include "ui/Toast.h"
#include "io/SaveLoad.h"

#include <optional>
#include <string>

// Forward decls to link with other TUs
namespace ui { void HUDLog(const char* fmt, ...); }
namespace ui { void SettingsSetReturn(Screen s); }

namespace ui {

static bool g_request_open_start = false; // request flag (one-shot)
static int  g_start_ix   = 0;
static const int kStartYears[] = { 1444, 1492, 1618, 1701, 1776, 1836 };

static void BeginCenteredMainMenuWindow() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImVec2 center = ImVec2(vp->Pos.x + vp->Size.x * 0.5f, vp->Pos.y + vp->Size.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(540, 460), ImGuiCond_Appearing);
    ImGui::Begin("Main Menu", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
}

void DrawMainMenu(AppState& app) {
    BeginCenteredMainMenuWindow();

    ImGui::Dummy(ImVec2(0,8));
    ImGui::TextColored(ImVec4(0.88f,0.78f,0.45f,1), "Mechanica Imperii");
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0,8));

    if (ImGui::Button("Continue", ImVec2(-1, 0))) {
        std::optional<io::SaveEntry> latest = io::SaveLoadManager::I().latestSave();
        if (latest.has_value()) {
            if (io::SaveLoadManager::I().loadByName(latest->name)) {
                ui::Toast("Loaded latest save", 2.0f);
            }
        }
        app.screen = ui::Screen::Game;
    }

    if (ImGui::Button("Load Game", ImVec2(-1, 0))) {
        SaveLoadUI::OpenLoadModal("MainMenu");
    }

    // "New Game" lives here (initial main menu)
    if (ImGui::Button("New Game", ImVec2(-1, 0))) {
        g_request_open_start = true; // mark request; we'll OpenPopup after End()
    }

    if (ImGui::Button("Settings", ImVec2(-1, 0))) {
        SettingsSetReturn(Screen::MainMenu);
        app.screen = ui::Screen::Settings;
    }

    if (ImGui::Button("Credits", ImVec2(-1, 0))) {
        app.screen = ui::Screen::Credits;
    }

    if (ImGui::Button("Quit", ImVec2(-1, 0))) {
        app.screen = ui::Screen::Quit;
        app.requestExit = true;
    }

    ImGui::End(); // Main Menu

    // Handle opening the Start Campaign modal cleanly after window ends
    if (g_request_open_start) {
        ImGui::OpenPopup("Start Campaign");
        g_request_open_start = false;
    }

    // Center the modal and render it every frame while open
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImVec2 center = ImVec2(vp->Pos.x + vp->Size.x * 0.5f, vp->Pos.y + vp->Size.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(420, 200), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Start Campaign", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextDisabled("Choose starting year:");
        ImGui::Separator();

        // Year combo
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", kStartYears[g_start_ix]);
        if (ImGui::BeginCombo("Start Year", buf)) {
            for (int i = 0; i < (int)(sizeof(kStartYears)/sizeof(kStartYears[0])); ++i) {
                bool selected = (g_start_ix == i);
                char item[16];
                snprintf(item, sizeof(item), "%d", kStartYears[i]);
                if (ImGui::Selectable(item, selected)) {
                    g_start_ix = i;
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::Spacing();
        if (ImGui::Button("Start", ImVec2(140, 0))) {
            const int year = kStartYears[g_start_ix];
            ui::HUDLog("Starting new campaign (%d)", year);
            // TODO: wire to sim: core::SimulationState::I().SetStartYear(year);
            app.screen = ui::Screen::Game;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(140, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Modals + toasts (load/save) render on main menu, too.
    SaveLoadUI::RenderModals();
    if (SaveLoadUI::ConsumeLoadedFlag()) {
        app.screen = ui::Screen::Game;
    }
    ToastManager::I().Render();
}

} // namespace ui