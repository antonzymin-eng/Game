#include "ui/DiplomacyWindow.h"
#include "ui/WindowManager.h"
#include "ui/PortraitGenerator.h"
#include "game/components/CharacterComponent.h"

namespace ui {

DiplomacyWindow::DiplomacyWindow(core::ecs::EntityManager& entity_manager,
                                 game::diplomacy::DiplomacySystem& diplomacy_system)
    : entity_manager_(entity_manager)
    , diplomacy_system_(diplomacy_system)
    , portrait_generator_(nullptr) {
}

void DiplomacyWindow::Render(WindowManager& window_manager, game::types::EntityID player_entity) {
    current_player_entity_ = player_entity;

    if (!window_manager.BeginManagedWindow(WindowManager::WindowType::DIPLOMACY, "Diplomacy")) {
        return;
    }

    if (ImGui::BeginTabBar("DiplomacyTabs")) {
        if (ImGui::BeginTabItem("Overview")) {
            RenderOverviewTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Relations")) {
            RenderRelationsTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Treaties")) {
            RenderTreatiesTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Wars")) {
            RenderWarTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    window_manager.EndManagedWindow();
}

void DiplomacyWindow::RenderOverviewTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("DIPLOMATIC OVERVIEW");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Text("Diplomatic relations and status");
}

void DiplomacyWindow::RenderRelationsTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("RELATIONS");
    ImGui::PopStyleColor();
    ImGui::Separator();

    // Placeholder text explaining portrait integration
    ImGui::TextWrapped("Relation standings with other nations");
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f),
                       "Portrait system ready - portraits will appear here when character data is available");

    // Example of how portraits would be displayed
    if (portrait_generator_) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("When integrated with character system, each nation's ruler portrait will appear here:");
        ImGui::Spacing();

        // Demonstration layout for future character portraits
        ImGui::BeginChild("RelationsList", ImVec2(0, 0), true);

        ImGui::Text("Example: [64x64 Portrait] Nation Name - Opinion: +50 (Friendly)");
        ImGui::Text("Future: Portraits will be generated procedurally for each ruler");

        ImGui::EndChild();
    }
}

void DiplomacyWindow::RenderTreatiesTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("TREATIES & AGREEMENTS");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    // Active treaties
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
    ImGui::Text("Active Treaties:");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    struct Treaty {
        const char* type;
        const char* partner;
        int years_remaining;
    };

    Treaty treaties[] = {
        {"Alliance", "Francia", 10},
        {"Trade Agreement", "Byzantine Empire", 5},
        {"Non-Aggression Pact", "Kingdom of England", 8}
    };

    for (const auto& treaty : treaties) {
        ImGui::PushID(treaty.partner);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
        ImGui::Text("%s", treaty.type);
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
        ImGui::Text("  Partner: %s | Years Remaining: %d", treaty.partner, treaty.years_remaining);
        ImGui::PopStyleColor();

        ImGui::SameLine(ImGui::GetWindowWidth() - 150);
        if (ImGui::Button("Break Treaty", ImVec2(130, 0))) {
            // Determine treaty type and partner based on treaty
            game::diplomacy::TreatyType treaty_type = game::diplomacy::TreatyType::ALLIANCE;
            game::types::EntityID partner = 0;

            if (treaty.type == "Alliance") {
                treaty_type = game::diplomacy::TreatyType::ALLIANCE;
            } else if (treaty.type == "Trade Agreement") {
                treaty_type = game::diplomacy::TreatyType::TRADE_AGREEMENT;
            } else if (treaty.type == "Non-Aggression Pact") {
                treaty_type = game::diplomacy::TreatyType::NON_AGGRESSION;
            }

            // Get partner nation ID (simplified - in real implementation, map partner name to ID)
            // For now, using a placeholder
            if (current_player_entity_ != 0) {
                diplomacy_system_.BreakTreatyBidirectional(current_player_entity_, partner, treaty_type);
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Breaking a treaty will damage relations and reputation");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PopID();
    }

    // Propose new treaty
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("PROPOSE NEW TREATY");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    // Get available nations for proposals
    auto all_realms = diplomacy_system_.GetAllRealms();
    std::vector<game::types::EntityID> available_nations;
    for (auto realm : all_realms) {
        if (realm != current_player_entity_) {
            available_nations.push_back(realm);
        }
    }

    // Nation selector
    if (!available_nations.empty()) {
        ImGui::Text("Select Target Nation:");
        ImGui::SameLine();
        if (selected_nation_index_ >= static_cast<int>(available_nations.size())) {
            selected_nation_index_ = 0;
        }

        std::vector<const char*> nation_names;
        std::vector<std::string> nation_name_strings;
        for (auto nation_id : available_nations) {
            nation_name_strings.push_back("Nation " + std::to_string(nation_id));
        }
        for (const auto& name : nation_name_strings) {
            nation_names.push_back(name.c_str());
        }

        ImGui::SetNextItemWidth(200);
        ImGui::Combo("##target_nation", &selected_nation_index_, nation_names.data(), static_cast<int>(nation_names.size()));
    }

    ImGui::Spacing();

    if (ImGui::Button("Propose Alliance", ImVec2(150, 0))) {
        if (!available_nations.empty() && current_player_entity_ != 0) {
            game::types::EntityID target = available_nations[selected_nation_index_];
            std::unordered_map<std::string, double> terms;
            terms["duration"] = 10.0; // 10 years
            terms["mutual_defense"] = 1.0;
            diplomacy_system_.ProposeAlliance(current_player_entity_, target, terms);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Propose Trade Deal", ImVec2(150, 0))) {
        if (!available_nations.empty() && current_player_entity_ != 0) {
            game::types::EntityID target = available_nations[selected_nation_index_];
            diplomacy_system_.ProposeTradeAgreement(current_player_entity_, target, 0.15, 5);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Propose Peace", ImVec2(150, 0))) {
        if (!available_nations.empty() && current_player_entity_ != 0) {
            game::types::EntityID target = available_nations[selected_nation_index_];
            std::unordered_map<std::string, double> peace_terms;
            peace_terms["war_reparations"] = 0.0;
            peace_terms["status_quo"] = 1.0;
            diplomacy_system_.SueForPeace(current_player_entity_, target, peace_terms);
        }
    }
}

void DiplomacyWindow::RenderWarTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("WARS");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    // Active wars
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
    ImGui::Text("Active Wars:");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
    ImGui::Text("Currently at peace");
    ImGui::PopStyleColor();

    // TODO: Display active wars with:
    // - Enemy nation
    // - War score
    // - Battles won/lost
    // - Sue for peace button

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Declare war
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("DECLARE WAR");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
    ImGui::TextWrapped("Select a nation and casus belli (justification) to declare war:");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    // Get available nations for war
    auto all_realms = diplomacy_system_.GetAllRealms();
    std::vector<game::types::EntityID> war_targets;
    for (auto realm : all_realms) {
        if (realm != current_player_entity_ && !diplomacy_system_.AreAtWar(current_player_entity_, realm)) {
            war_targets.push_back(realm);
        }
    }

    // Nation selector for war
    if (!war_targets.empty()) {
        ImGui::Text("Target Nation:");
        ImGui::SameLine();
        if (selected_nation_index_ >= static_cast<int>(war_targets.size())) {
            selected_nation_index_ = 0;
        }

        std::vector<const char*> war_target_names;
        std::vector<std::string> war_target_name_strings;
        for (auto nation_id : war_targets) {
            war_target_name_strings.push_back("Nation " + std::to_string(nation_id));
        }
        for (const auto& name : war_target_name_strings) {
            war_target_names.push_back(name.c_str());
        }

        ImGui::SetNextItemWidth(200);
        ImGui::Combo("##war_target", &selected_nation_index_, war_target_names.data(), static_cast<int>(war_target_names.size()));

        ImGui::Spacing();

        // Casus Belli selector
        const char* casus_belli_names[] = {
            "None",
            "Border Dispute",
            "Trade Interference",
            "Dynastic Claim",
            "Religious Conflict",
            "Insult to Honor",
            "Broken Treaty",
            "Protection of Ally",
            "Liberation War"
        };

        ImGui::Text("Casus Belli:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        ImGui::Combo("##casus_belli", &selected_casus_belli_, casus_belli_names, 9);

        ImGui::Spacing();
    }

    if (ImGui::Button("Declare War", ImVec2(150, 0))) {
        if (!war_targets.empty() && current_player_entity_ != 0 && selected_casus_belli_ > 0) {
            game::types::EntityID target = war_targets[selected_nation_index_];
            auto cb = static_cast<game::diplomacy::CasusBelli>(selected_casus_belli_);
            diplomacy_system_.DeclareWar(current_player_entity_, target, cb);
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Requires a valid casus belli to avoid diplomatic penalties");
    }
}

} // namespace ui
