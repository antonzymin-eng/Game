#include "imgui.h"
#include "ScreenAPI.h"
#include "state/SimulationState.h"

// UI modules
#include "ui/MapViewport.h"     // DrawMapViewport(AppState&)
#include "ui/SaveLoadUI.h"
#include "ui/Toast.h"
#include "io/SaveLoad.h"

#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include <mutex>
#include <cstdarg>
#include <cstdio>
#include <cmath>

// Forward declare settings return setter and new reserve setter
namespace ui { void SettingsSetReturn(Screen s); }
namespace ui { void SetRightPanelReserve(float px); } // NEW

// (rest of file omitted for brevity - this is the same GameScreen you already have,
//  with one small change inside DrawRightLedger())

namespace ui {

extern float g_right_w; // if not available, you can remove this extern and use ImGui::GetWindowSize().x directly

static ImVec2 ClampToViewport(ImVec2 pos, ImVec2 size) {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImVec2 min = vp->Pos;
    ImVec2 max = ImVec2(vp->Pos.x + vp->Size.x, vp->Pos.y + vp->Size.y);
    if (pos.x < min.x) pos.x = min.x;
    if (pos.y < min.y) pos.y = min.y;
    if (pos.x + size.x > max.x) pos.x = max.x - size.x;
    if (pos.y + size.y > max.y) pos.y = max.y - size.y;
    return pos;
}

static bool g_right_collapsed_once = false;

static void DrawRightLedger() {
    const float margin = 12.0f;
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImVec2 rp_size = ImVec2(230.0f, vp->Size.y - (64.0f + margin * 2.0f)); // default width if not sticky here
    ImVec2 rp_pos  = ImVec2(vp->Pos.x + vp->Size.x - rp_size.x - margin, vp->Pos.y + 64.0f + margin);
    rp_pos = ClampToViewport(rp_pos, rp_size);

    ImGui::SetNextWindowPos(rp_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(rp_size, ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints(ImVec2(180, 200), ImVec2(420, rp_size.y));

    if (!g_right_collapsed_once) { ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once); g_right_collapsed_once = true; }

    bool open = ImGui::Begin("Details / Ledger");
    // Inform the map about the space this window currently occupies (avoid overlap)
    float reserve = 0.0f;
    if (open) {
        bool collapsed = ImGui::IsWindowCollapsed();
        reserve = collapsed ? 0.0f : ImGui::GetWindowSize().x + margin;
    }
    SetRightPanelReserve(reserve);

    if (open) {
        if (ImGui::BeginTabBar("RightTabs")) {
            if (ImGui::BeginTabItem("Details")) {
                ImGui::TextDisabled("Select a province or nation element to view details.");
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Ledger")) {
                if (ImGui::BeginTable("LedgerTbl", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
                    ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthStretch, 1.5f);
                    ImGui::TableSetupColumn("Value",    ImGuiTableColumnFlags_WidthStretch, 0.5f);
                    ImGui::TableHeadersRow();
                    auto row = [](const char* k, const char* v) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(k);
                        ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(v);
                    };
                    row("Tax Income",       "420");
                    row("Production",       "310");
                    row("Trade",            "220");
                    row("Gold & Tariffs",   "90");
                    row("—",                "");
                    row("Army Maintenance", "-380");
                    row("Navy Maintenance", "-120");
                    row("State Expenses",   "-140");
                    row("—",                "");
                    row("Net Balance",      "400");
                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

} // namespace ui