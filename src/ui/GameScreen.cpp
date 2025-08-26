
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

// Forward declare settings return setter
namespace ui { void SettingsSetReturn(Screen s); }

// -----------------------------------------------------------------------------
// FUSED link helpers so we don't rely on other cpp files being in the build
// -----------------------------------------------------------------------------
namespace io {
    std::string FindAsset(const std::string& name) {
        namespace fs = std::filesystem;
        fs::path p(name);
        if (p.is_absolute() && fs::exists(p)) return p.string();
        if (const char* env = std::getenv("MECH_ASSETS_DIR")) {
            fs::path base(env);
            fs::path candidate = base / name;
            if (fs::exists(candidate)) return candidate.string();
        }
        const char* bases[] = {"assets","./assets","../assets","../../assets","data","./data","../data","../../data"};
        for (const char* b : bases) {
            fs::path candidate = std::filesystem::path(b) / name;
            if (std::filesystem::exists(candidate)) return candidate.string();
        }
        return name;
    }
}

namespace ui {
    static std::mutex g_log_mtx;
    static std::vector<std::string> g_log;
    static constexpr size_t kMaxLog = 256;
    void HUDLog(const char* fmt, ...) {
        if (!fmt) return;
        char buf[1024];
        va_list args;
        va_start(args, fmt);
    #if defined(_MSC_VER)
        _vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, args);
    #else
        vsnprintf(buf, sizeof(buf), fmt, args);
    #endif
        va_end(args);
        std::lock_guard<std::mutex> lock(g_log_mtx);
        g_log.emplace_back(buf);
        if (g_log.size() > kMaxLog) g_log.erase(g_log.begin(), g_log.begin() + (g_log.size() - kMaxLog));
    }
}

// -----------------------------------------------------------------------------
// KPI helpers (replace placeholders with real SimulationState fields later)
// -----------------------------------------------------------------------------
static void DrawKPI(const char* label, const char* value, const char* delta = nullptr) {
    ImGui::BeginGroup();
    ImGui::TextDisabled("%s", label);
    ImGui::TextUnformatted(value);
    if (delta && delta[0]) { ImGui::SameLine(); ImGui::TextColored(ImVec4(0.65f,0.86f,0.65f,1), "%s", delta); }
    ImGui::EndGroup();
}
static void FetchNationKPIs(core::SimulationState& /*sim*/,
                            std::string& treasury, std::string& incomeDelta,
                            std::string& manpower, std::string& stability,
                            std::string& legitimacy, std::string& prestige) {
    treasury="1,245"; incomeDelta="+12"; manpower="18.2k"; stability="+1"; legitimacy="78"; prestige="42";
}

namespace ui {

// ---------------- UI state ----------------
static int    g_speed_ix = 1;      // 0=Pause,1=1x,2=2x,3=3x
static bool   g_show_ingame_menu=false;

// System Details accordion
struct PanelAnim { bool target_open=false; float t=0.0f; };
static int    g_active_panel=-1;       // which subpanel is open (exclusive)
static PanelAnim g_sys_anim[12];

static float  g_left_w  = 240.0f;      // sticky width for left panels
static float  g_right_w = 230.0f;      // sticky width for right ledger

static const char* kPanels[] = {
    "Overview","Economy","Military","Espionage","Diplomacy","Technology",
    "Religion","Trade","Culture","Administration","Population","Laws"
};

// ---------------- Animation util ----------------
static void StepAnim(PanelAnim& a) {
    const float dt = ImGui::GetIO().DeltaTime;
    float target = a.target_open ? 1.0f : 0.0f;
    a.t += (target - a.t) * (1.0f - powf(0.0001f, dt * 60.0f));
    if (a.t < 0.001f) a.t = 0.0f;
    if (a.t > 0.999f) a.t = 1.0f;
}

// ---------------- Top HUD (KPIs left; ☰ Menu button top-right; NO title text) ----------------
static void DrawTopHUD(AppState& app) {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(vp->Pos.x, vp->Pos.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(vp->Size.x, 64.0f), ImGuiCond_Always);
    if (ImGui::Begin("TopHUD", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus)) {
        // KPIs (top-left)
        core::SimulationState& sim = core::SimulationState::I();
        std::string treasury,incomeDelta,manpower,stability,legitimacy,prestige;
        FetchNationKPIs(sim,treasury,incomeDelta,manpower,stability,legitimacy,prestige);

        DrawKPI("Treasury", treasury.c_str(), incomeDelta.c_str()); ImGui::SameLine(0, 16);
        DrawKPI("Manpower", manpower.c_str()); ImGui::SameLine(0, 16);
        DrawKPI("Stability", stability.c_str()); ImGui::SameLine(0, 16);
        DrawKPI("Legitimacy", legitimacy.c_str()); ImGui::SameLine(0, 16);
        DrawKPI("Prestige", prestige.c_str()); ImGui::SameLine(0, 16);

        // Speed controls
        ImGui::TextDisabled("Speed"); ImGui::SameLine();
        if (ImGui::RadioButton("⏸", g_speed_ix==0)) { g_speed_ix=0; HUDLog("Speed: Pause"); }
        ImGui::SameLine();
        if (ImGui::RadioButton("1x", g_speed_ix==1)) { g_speed_ix=1; HUDLog("Speed: 1x"); }
        ImGui::SameLine();
        if (ImGui::RadioButton("2x", g_speed_ix==2)) { g_speed_ix=2; HUDLog("Speed: 2x"); }
        ImGui::SameLine();
        if (ImGui::RadioButton("3x", g_speed_ix==3)) { g_speed_ix=3; HUDLog("Speed: 3x"); }

        // Right aligned ☰ Menu button (top-right)
        float btn_w = ImGui::CalcTextSize("☰ Menu").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        float right_x = ImGui::GetWindowContentRegionMax().x - btn_w;
        ImGui::SameLine();
        ImGui::SetCursorPosX(right_x);
        if (ImGui::Button("☰ Menu")) { g_show_ingame_menu = true; ImGui::OpenPopup("Main Menu"); }
    }
    ImGui::End();

    // In-game Main Menu (centered modal) — NO "New Game" here
    if (g_show_ingame_menu) ImGui::OpenPopup("Main Menu");
    if (ImGui::BeginPopupModal("Main Menu", &g_show_ingame_menu, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (ImGui::Button("Continue", ImVec2(280, 0))) {
            g_show_ingame_menu = false;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button("Load Game", ImVec2(280, 0))) { SaveLoadUI::OpenLoadModal("InGameMenu"); }
        if (ImGui::Button("Settings", ImVec2(280, 0))) { SettingsSetReturn(Screen::Game); app.screen = Screen::Settings; }
        if (ImGui::Button("Credits", ImVec2(280, 0)))  { app.screen = Screen::Credits; }
        if (ImGui::Button("Quit to Main Menu", ImVec2(280, 0))) { app.screen = Screen::MainMenu; g_show_ingame_menu=false; ImGui::CloseCurrentPopup(); }
        if (ImGui::Button("Quit to Desktop", ImVec2(280, 0)))   { app.screen = Screen::Quit; }
        ImGui::EndPopup();
    }
}

// ---------------- Left Independent Panels (National Overview + System Details) ----------------
static void DrawLeft_NationalOverviewPanel() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    const float top_h = 64.0f;
    const float margin = 12.0f;
    const float avail_h = vp->Size.y - (top_h + margin * 2.0f);
    const float panel_h = (avail_h - margin) * 0.5f;

    ImVec2 pos = ImVec2(vp->Pos.x + margin, vp->Pos.y + top_h + margin);
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(g_left_w, panel_h), ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints(ImVec2(160, 200), ImVec2(480, panel_h));
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once); // collapsed by default

    if (ImGui::Begin("National Overview")) {
        // Remember adjusted width ("sticking" size)
        g_left_w = ImGui::GetWindowSize().x;

        ImGui::TextDisabled("National Overview");
        ImGui::Separator();
        ImGui::TextUnformatted("— Hook up real nation-level data here —");
    }
    ImGui::End();
}

static void DrawLeft_SystemDetailsPanel() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    const float top_h = 64.0f;
    const float margin = 12.0f;
    const float avail_h = vp->Size.y - (top_h + margin * 2.0f);
    const float panel_h = (avail_h - margin) * 0.5f;

    // Stack directly under National Overview (approximate height; clean and simple)
    ImVec2 pos = ImVec2(vp->Pos.x + margin, vp->Pos.y + top_h + margin + panel_h + margin);
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(g_left_w, panel_h), ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints(ImVec2(160, 200), ImVec2(480, panel_h));
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once); // collapsed by default

    if (ImGui::Begin("System Details")) {
        // Remember adjusted width ("sticking" size)
        g_left_w = ImGui::GetWindowSize().x;

        // Exclusive open panels with animation
        for (int i = 0; i < (int)(sizeof(kPanels)/sizeof(kPanels[0])); ++i) {
            ImGui::PushID(i);
            bool is_open = (g_active_panel == i);
            const char* arr = is_open ? "▾ " : "▸ ";
            bool clicked = ImGui::Selectable((std::string(arr)+kPanels[i]).c_str(), is_open, ImGuiSelectableFlags_SpanAllColumns);
            if (clicked) g_active_panel = is_open ? -1 : i;

            g_sys_anim[i].target_open = (g_active_panel == i); StepAnim(g_sys_anim[i]);
            float t = g_sys_anim[i].t;
            if (t > 0.0f) {
                const float max_h = 220.0f;
                float h = max_h * t * t;
                ImGui::BeginChild("PanelBody", ImVec2(0, h), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                ImGui::TextDisabled("%s", kPanels[i]);
                ImGui::Separator();
                ImGui::Text("Content stub for %s.\nWire to your SystemsPanel code.", kPanels[i]);
                ImGui::EndChild();
            }
            ImGui::PopID();
            ImGui::Separator();
        }
    }
    ImGui::End();
}

// ---------------- Right Ledger (sticky width, collapsible window) ----------------
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
    ImVec2 rp_size = ImVec2(g_right_w, vp->Size.y - (64.0f + margin * 2.0f));
    ImVec2 rp_pos  = ImVec2(vp->Pos.x + vp->Size.x - g_right_w - margin, vp->Pos.y + 64.0f + margin);
    rp_pos = ClampToViewport(rp_pos, rp_size);

    ImGui::SetNextWindowPos(rp_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(g_right_w, rp_size.y), ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints(ImVec2(180, 200), ImVec2(420, rp_size.y));

    if (!g_right_collapsed_once) { ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once); g_right_collapsed_once = true; }

    if (ImGui::Begin("Details / Ledger")) {
        // Remember adjusted width ("sticking" size)
        g_right_w = ImGui::GetWindowSize().x;

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

// ---------------- Game Screen ----------------
void DrawGameScreen(AppState& app) {
    DrawTopHUD(app);
    DrawMapViewport(app);

    // Left: two independent collapsing windows (collapsed by default)
    DrawLeft_NationalOverviewPanel();
    DrawLeft_SystemDetailsPanel();

    // Right: details/ledger (collapsed by default)
    DrawRightLedger();

    // Modals + toasts
    SaveLoadUI::RenderModals();
    (void)SaveLoadUI::ConsumeLoadedFlag();
    ToastManager::I().Render();
}

} // namespace ui
