#include "ui/DiplomacyPanel.h"
#include "imgui.h"
#include "ui/PanelCommon.h"
#include <vector>
#include <string>
#include <algorithm>

using namespace ui;

namespace ui::panels {

static int   gSort_diplo = 0;
static char  gSearch_diplo[64] = {0};
static int   gSelected_diplo = -1;

void DiplomacyPanel::Draw(core::SimulationState& sim, float /*width*/) {
    ImGui::SetNextItemWidth(160);
    ImGui::InputTextWithHint("##search_diplo", "Search", gSearch_diplo, sizeof(gSearch_diplo));
    ImGui::SameLine();
    if (ImGui::BeginCombo("Sort##diplo", gSort_diplo==0 ? "Value" : "Name")) {
        bool s0 = (gSort_diplo==0);
        bool s1 = (gSort_diplo==1);
        if (ImGui::Selectable("Value", s0)) gSort_diplo=0;
        if (ImGui::Selectable("Name", s1))  gSort_diplo=1;
        ImGui::EndCombo();
    }

    const auto& rel = sim.diplomacy();
    std::vector<int> idx;
    idx.reserve((int)rel.size());
    for (int i=0;i<(int)rel.size();++i) {
        const auto& r = rel[i];
        std::string pair = sim.factionName(r.aId) + " ↔ " + sim.factionName(r.bId);
        if (matchesSearch(pair.c_str(), gSearch_diplo)) idx.push_back(i);
    }
    std::sort(idx.begin(), idx.end(), [&](int a, int b){
        if (gSort_diplo==0) return rel[a].value > rel[b].value;
        std::string A = sim.factionName(rel[a].aId);
        std::string B = sim.factionName(rel[b].aId);
        return A < B;
    });

    ImGui::Separator();
    ImGuiListClipper clip;
    clip.Begin((int)idx.size());
    while (clip.Step()) {
        for (int _i = clip.DisplayStart; _i < clip.DisplayEnd; ++_i) {
            int i = idx[_i];
            const auto& r = rel[i];
            std::string label = sim.factionName(r.aId) + " ↔ " + sim.factionName(r.bId);
            bool selected = (gSelected_diplo==i);
            if (ImGui::Selectable((label + "##diplo-" + std::to_string(i)).c_str(), selected)) {
                gSelected_diplo = i;
            }
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - 90);
            ImGui::Text("%d", r.value);

            ImGui::SameLine();
            if (r.war)           ImGui::TextColored(ImVec4(0.85f,0.2f,0.2f,1.0f), "WAR");
            else if (r.alliance) ImGui::TextColored(ImVec4(0.2f,0.5f,0.9f,1.0f), "ALLIANCE");
            else if (r.truce)    ImGui::TextColored(ImVec4(0.8f,0.6f,0.2f,1.0f), "TRUCE");
            else                 ImGui::TextUnformatted("Neutral");
        }
    }
}

void DiplomacyPanel::DrawDetails(core::SimulationState& sim) {
    int i = gSelected_diplo;
    if (i < 0) { ImGui::TextUnformatted("Select a row to see details."); return; }
    const auto& r = sim.diplomacy()[i];
    ImGui::Text("Relation: %s ↔ %s", sim.factionName(r.aId).c_str(), sim.factionName(r.bId).c_str());
    ImGui::Separator();
    ImGui::Text("Value: %d", r.value);
    ImGui::Text("War: %s  Alliance: %s  Truce: %s", r.war ? "Yes":"No", r.alliance ? "Yes":"No", r.truce ? "Yes":"No");
}

} // namespace ui::panels
