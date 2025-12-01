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

    // Get real treaty data from DiplomacyComponent
    auto diplomacy_comp = diplomacy_system_.GetDiplomacyComponent(current_player_entity_);

    if (diplomacy_comp && !diplomacy_comp->active_treaties.empty()) {
        for (const auto& treaty : diplomacy_comp->active_treaties) {
            // Skip if treaty is not active
            if (!treaty.is_active) continue;

            // Determine partner (the other signatory)
            game::types::EntityID partner = (treaty.signatory_a == current_player_entity_)
                                           ? treaty.signatory_b
                                           : treaty.signatory_a;

            ImGui::PushID(treaty.treaty_id.c_str());

            // Display treaty type
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
            const char* treaty_type_str = "Unknown";
            switch (treaty.type) {
                case game::diplomacy::TreatyType::ALLIANCE: treaty_type_str = "Alliance"; break;
                case game::diplomacy::TreatyType::TRADE_AGREEMENT: treaty_type_str = "Trade Agreement"; break;
                case game::diplomacy::TreatyType::NON_AGGRESSION: treaty_type_str = "Non-Aggression Pact"; break;
                case game::diplomacy::TreatyType::MARRIAGE_PACT: treaty_type_str = "Marriage Pact"; break;
                case game::diplomacy::TreatyType::TRIBUTE: treaty_type_str = "Tribute"; break;
                case game::diplomacy::TreatyType::MILITARY_ACCESS: treaty_type_str = "Military Access"; break;
                case game::diplomacy::TreatyType::DEFENSIVE_LEAGUE: treaty_type_str = "Defensive League"; break;
                default: break;
            }
            ImGui::Text("%s", treaty_type_str);
            ImGui::PopStyleColor();

            // Display partner and treaty details
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
            ImGui::Text("  Partner: Nation %u | Compliance: %.0f%%", partner, treaty.GetOverallCompliance() * 100.0);
            ImGui::PopStyleColor();

            // Break treaty button with confirmation
            ImGui::SameLine(ImGui::GetWindowWidth() - 150);
            if (ImGui::Button("Break Treaty", ImVec2(130, 0))) {
                pending_treaty_id_to_break_ = treaty.treaty_id;
                show_treaty_break_confirmation_ = true;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Breaking a treaty will damage relations and reputation");
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::PopID();
        }
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
        ImGui::Text("No active treaties");
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }

    // Confirmation dialog for breaking treaties
    if (show_treaty_break_confirmation_) {
        ImGui::OpenPopup("Confirm Break Treaty");
    }

    if (ImGui::BeginPopupModal("Confirm Break Treaty", &show_treaty_break_confirmation_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Are you sure you want to break this treaty?");
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "This will:");
        ImGui::BulletText("Damage diplomatic relations");
        ImGui::BulletText("Reduce your reputation");
        ImGui::BulletText("May trigger defensive alliances");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Confirm Break Treaty", ImVec2(200, 0))) {
            // Find the treaty and extract partner info
            if (diplomacy_comp) {
                for (const auto& treaty : diplomacy_comp->active_treaties) {
                    if (treaty.treaty_id == pending_treaty_id_to_break_) {
                        game::types::EntityID partner = (treaty.signatory_a == current_player_entity_)
                                                       ? treaty.signatory_b
                                                       : treaty.signatory_a;
                        diplomacy_system_.BreakTreatyBidirectional(current_player_entity_, partner, treaty.type);
                        break;
                    }
                }
            }
            show_treaty_break_confirmation_ = false;
            pending_treaty_id_to_break_.clear();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) {
            show_treaty_break_confirmation_ = false;
            pending_treaty_id_to_break_.clear();
        }

        ImGui::EndPopup();
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

    // Nation selector - using static storage to avoid string lifetime issues
    static std::vector<std::string> treaty_nation_names;
    if (!available_nations.empty()) {
        ImGui::Text("Select Target Nation:");
        ImGui::SameLine();
        if (selected_treaty_target_ >= static_cast<int>(available_nations.size())) {
            selected_treaty_target_ = 0;
        }

        // Rebuild nation names (cached in static vector)
        treaty_nation_names.clear();
        for (auto nation_id : available_nations) {
            treaty_nation_names.push_back("Nation " + std::to_string(nation_id));
        }

        ImGui::SetNextItemWidth(200);
        // Use ImGui::ListBox pattern which is safer with string lifetimes
        if (ImGui::BeginCombo("##treaty_target_nation", treaty_nation_names[selected_treaty_target_].c_str())) {
            for (int i = 0; i < static_cast<int>(treaty_nation_names.size()); i++) {
                bool is_selected = (selected_treaty_target_ == i);
                if (ImGui::Selectable(treaty_nation_names[i].c_str(), is_selected)) {
                    selected_treaty_target_ = i;
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }

    ImGui::Spacing();

    if (ImGui::Button("Propose Alliance", ImVec2(150, 0))) {
        if (!available_nations.empty() && current_player_entity_ != 0) {
            game::types::EntityID target = available_nations[selected_treaty_target_];
            std::unordered_map<std::string, double> terms;
            terms["duration"] = 10.0; // 10 years
            terms["mutual_defense"] = 1.0;
            bool success = diplomacy_system_.ProposeAlliance(current_player_entity_, target, terms);
            // TODO: Show toast notification based on success
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Propose a military alliance (10-year duration)");
    }

    ImGui::SameLine();
    if (ImGui::Button("Propose Trade Deal", ImVec2(150, 0))) {
        if (!available_nations.empty() && current_player_entity_ != 0) {
            game::types::EntityID target = available_nations[selected_treaty_target_];
            bool success = diplomacy_system_.ProposeTradeAgreement(current_player_entity_, target, 0.15, 5);
            // TODO: Show toast notification based on success
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Propose trade agreement (+15% trade bonus for 5 years)");
    }

    ImGui::SameLine();
    if (ImGui::Button("Propose Peace", ImVec2(150, 0))) {
        if (!available_nations.empty() && current_player_entity_ != 0) {
            game::types::EntityID target = available_nations[selected_treaty_target_];
            std::unordered_map<std::string, double> peace_terms;
            peace_terms["war_reparations"] = 0.0;
            peace_terms["status_quo"] = 1.0;
            bool success = diplomacy_system_.SueForPeace(current_player_entity_, target, peace_terms);
            // TODO: Show toast notification based on success
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Propose peace with status quo terms");
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
    auto all_war_realms = diplomacy_system_.GetAllRealms();
    std::vector<game::types::EntityID> war_targets;
    for (auto realm : all_war_realms) {
        if (realm != current_player_entity_ && !diplomacy_system_.AreAtWar(current_player_entity_, realm)) {
            war_targets.push_back(realm);
        }
    }

    // Nation selector for war - using static storage
    static std::vector<std::string> war_nation_names;
    if (!war_targets.empty()) {
        ImGui::Text("Target Nation:");
        ImGui::SameLine();
        if (selected_war_target_ >= static_cast<int>(war_targets.size())) {
            selected_war_target_ = 0;
        }

        // Rebuild nation names
        war_nation_names.clear();
        for (auto nation_id : war_targets) {
            war_nation_names.push_back("Nation " + std::to_string(nation_id));
        }

        ImGui::SetNextItemWidth(200);
        if (ImGui::BeginCombo("##war_target", war_nation_names[selected_war_target_].c_str())) {
            for (int i = 0; i < static_cast<int>(war_nation_names.size()); i++) {
                bool is_selected = (selected_war_target_ == i);
                if (ImGui::Selectable(war_nation_names[i].c_str(), is_selected)) {
                    selected_war_target_ = i;
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Spacing();

        // Casus Belli selector
        const char* casus_belli_names[] = {
            "None (Select CB)",
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

    bool can_declare_war = !war_targets.empty() && current_player_entity_ != 0 && selected_casus_belli_ > 0;

    if (!can_declare_war) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Declare War", ImVec2(150, 0))) {
        if (can_declare_war) {
            pending_war_target_ = war_targets[selected_war_target_];
            pending_casus_belli_ = static_cast<game::diplomacy::CasusBelli>(selected_casus_belli_);
            show_war_confirmation_ = true;
        }
    }

    if (!can_declare_war) {
        ImGui::EndDisabled();
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        if (selected_casus_belli_ == 0) {
            ImGui::SetTooltip("Select a valid casus belli first");
        } else {
            ImGui::SetTooltip("This is a declaration of war. Choose wisely.");
        }
    }

    // War confirmation dialog
    if (show_war_confirmation_) {
        ImGui::OpenPopup("Confirm Declaration of War");
    }

    if (ImGui::BeginPopupModal("Confirm Declaration of War", &show_war_confirmation_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "DECLARE WAR");
        ImGui::Spacing();
        ImGui::Text("Are you sure you want to declare war?");
        ImGui::Spacing();

        // Show target and CB
        const char* cb_names[] = {"None", "Border Dispute", "Trade Interference", "Dynastic Claim",
                                 "Religious Conflict", "Insult to Honor", "Broken Treaty",
                                 "Protection of Ally", "Liberation War"};
        ImGui::BulletText("Target: Nation %u", pending_war_target_);
        ImGui::BulletText("Casus Belli: %s", cb_names[static_cast<int>(pending_casus_belli_)]);

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "This will:");
        ImGui::BulletText("Start an immediate war");
        ImGui::BulletText("May activate defensive alliances");
        ImGui::BulletText("Damage relations with allies of the target");
        ImGui::BulletText("Increase war weariness over time");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Confirm Declaration", ImVec2(180, 0))) {
            bool success = diplomacy_system_.DeclareWar(current_player_entity_, pending_war_target_, pending_casus_belli_);
            // TODO: Show toast notification based on success
            show_war_confirmation_ = false;
            pending_war_target_ = 0;
            pending_casus_belli_ = game::diplomacy::CasusBelli::NONE;
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) {
            show_war_confirmation_ = false;
            pending_war_target_ = 0;
            pending_casus_belli_ = game::diplomacy::CasusBelli::NONE;
        }

        ImGui::EndPopup();
    }
}

} // namespace ui
