#include "ui/BuildingsUI.h"
#include "ui/MapAPI.h"
#include "imgui.h"
#include <unordered_map>
#include <algorithm>

namespace ui{
static std::vector<BuildingType> g={
    {"market","Market","Improves local trade and tax flows.",100,+0.10f,+0.00f},
    {"workshop","Workshop","Boosts production and artisans.",120,+0.12f,+0.01f},
    {"barracks","Barracks","Raises training capacity.",150,+0.00f,+0.00f},
    {"church","Church","Religious cohesion, reduces unrest.",80,+0.00f,+0.00f}
};
static std::unordered_map<int,std::vector<std::string>> plan;

const std::vector<BuildingType>& Buildings_List(){return g;}
void Buildings_PlanBuildProvince(int pid, const char* k){
    auto& v=plan[pid]; if(std::find(v.begin(),v.end(),k)==v.end()) v.emplace_back(k);
}
void Buildings_PlanBuildNation(const std::vector<int>& ids, const char* k){
    for(int id:ids) Buildings_PlanBuildProvince(id,k);
}
std::vector<std::string> Buildings_GetPlanned(int pid){
    auto it=plan.find(pid); if(it==plan.end()) return {}; return it->second;
}

static void Card(const BuildingType& t, int pid=-1){
    ImGui::BeginGroup();
    ImGui::TextUnformatted(t.name);
    ImGui::TextDisabled("%s",t.desc);
    ImGui::Text("Cost: %d",t.cost);
    if(t.econDelta) ImGui::BulletText("Economy %+0.0f%%", t.econDelta*100.0f);
    if(t.popDelta)  ImGui::BulletText("Population %+0.1f%%", t.popDelta*100.0f);
    if(pid>=0){ if(ImGui::Button("Queue Here")) Buildings_PlanBuildProvince(pid,t.key); }
    ImGui::EndGroup();
}

void BuildingsUI_ShowForProvince(int pid, const char* pname, bool* open){
    if(!*open) return;
    ImGui::SetNextWindowSize(ImVec2(460,520),ImGuiCond_Appearing);
    if(ImGui::Begin("Province Buildings", open, ImGuiWindowFlags_NoSavedSettings)){
        ImGui::TextDisabled("%s", pname?pname:"—");
        ImGui::Separator();
        ImGui::TextUnformatted("Available Buildings");
        ImGui::Separator();
        for(auto& t:g){ ImGui::PushID(t.key); Card(t,pid); ImGui::Separator(); ImGui::PopID(); }
        auto p=Buildings_GetPlanned(pid);
        if(!p.empty()){
            ImGui::Separator();
            ImGui::TextUnformatted("Planned here:");
            for(auto& k:p) ImGui::BulletText("%s",k.c_str());
        }
    }
    ImGui::End();
}

void BuildingsUI_Show(bool* open){
    if(!*open) return;
    ImGui::SetNextWindowSize(ImVec2(560,560),ImGuiCond_Appearing);
    if(ImGui::Begin("Nation Building", open, ImGuiWindowFlags_NoSavedSettings)){
        static int sel=-1; static char filt[64]="";
        ImGui::TextUnformatted("Select Provinces");
        ImGui::InputTextWithHint("##f","filter name/owner",filt,sizeof(filt));
        ImGui::BeginChild("##prov",ImVec2(220,420),true);
        int n=MapGetProvinceListSize();
        for(int i=0;i<n;++i){
            int id=MapGetNthProvinceId(i); ProvinceSummary ps{};
            if(MapGetProvinceSummary(id,&ps)){
                if(filt[0]){
                    std::string f=filt, nm=ps.name?ps.name:"", ow=ps.owner?ps.owner:"";
                    for(char& c:f) c=(char)tolower(c);
                    std::string h=nm+" "+ow; for(char& c:h) c=(char)tolower(c);
                    if(h.find(f)==std::string::npos) continue;
                }
                bool on=(sel==id);
                if(ImGui::Selectable(ps.name?ps.name:"—",on)) sel=id;
                if(ImGui::IsItemHovered()) ImGui::SetTooltip("Owner: %s", ps.owner?ps.owner:"—");
            }
        }
        ImGui::EndChild(); ImGui::SameLine();
        ImGui::BeginChild("##opts",ImVec2(0,420),true);
        static int choice=0; for(int i=0;i<(int)g.size();++i){ if(ImGui::RadioButton(g[i].name, choice==i)) choice=i; }
        ImGui::Dummy(ImVec2(1,8));
        if(ImGui::Button("Build in Selected") && sel>=0) Buildings_PlanBuildProvince(sel,g[choice].key);
        ImGui::SameLine();
        if(ImGui::Button("Build in All Listed")){
            std::vector<int> ids;
            int n=MapGetProvinceListSize();
            for(int i=0;i<n;++i){
                int id=MapGetNthProvinceId(i); ProvinceSummary ps{};
                if(MapGetProvinceSummary(id,&ps)){
                    if(filt[0]){
                        std::string f=filt, nm=ps.name?ps.name:"", ow=ps.owner?ps.owner:"";
                        for(char& c:f) c=(char)tolower(c);
                        std::string h=nm+" "+ow; for(char& c:h) c=(char)tolower(c);
                        if(h.find(f)==std::string::npos) continue;
                    }
                    ids.push_back(id);
                }
            }
            Buildings_PlanBuildNation(ids,g[choice].key);
        }
        ImGui::EndChild();
    }
    ImGui::End();
}
}
