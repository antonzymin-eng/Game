#include "ui/AdministrativeWindow.h"
#include "ui/WindowManager.h"
#include "game/administration/AdministrativeComponents.h"

namespace ui {

AdministrativeWindow::AdministrativeWindow(core::ecs::EntityManager& entity_manager,
                                          game::administration::AdministrativeSystem& administrative_system)
    : entity_manager_(entity_manager)
    , administrative_system_(administrative_system) {
}

void AdministrativeWindow::Render(WindowManager& window_manager, game::types::EntityID player_entity) {
    current_player_entity_ = player_entity;

    if (!window_manager.BeginManagedWindow(WindowManager::WindowType::ADMINISTRATION, "Administration")) {
        return;
    }

    // Tab bar
    if (ImGui::BeginTabBar("AdministrationTabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Overview")) {
            RenderOverviewTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Officials")) {
            RenderOfficialsTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Bureaucracy")) {
            RenderBureaucracyTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Law & Order")) {
            RenderLawOrderTab();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    window_manager.EndManagedWindow();
}

void AdministrativeWindow::RenderOverviewTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("ADMINISTRATIVE OVERVIEW");
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::Spacing();

    // Get real data from administrative system
    double admin_efficiency = administrative_system_.GetAdministrativeEfficiency(current_player_entity_);
    double tax_collection_rate = administrative_system_.GetTaxCollectionRate(current_player_entity_);
    double bureaucratic_efficiency = administrative_system_.GetBureaucraticEfficiency(current_player_entity_);

    // Get governance component for additional data
    auto entity_handle = entity_manager_.GetEntityHandle(current_player_entity_);
    auto governance = entity_manager_.GetComponent<game::administration::GovernanceComponent>(entity_handle);

    ImGui::Columns(2, "admin_overview", false);
    ImGui::SetColumnWidth(0, 250);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));

    // Efficiency metrics
    ImGui::Text("Administrative Efficiency:");
    ImGui::NextColumn();
    ImGui::TextColored(GetEfficiencyColor(admin_efficiency), "%.1f%%", admin_efficiency * 100.0);
    ImGui::NextColumn();

    ImGui::Text("Tax Collection Efficiency:");
    ImGui::NextColumn();
    ImGui::TextColored(GetEfficiencyColor(tax_collection_rate), "%.1f%%", tax_collection_rate * 100.0);
    ImGui::NextColumn();

    ImGui::Text("Bureaucratic Efficiency:");
    ImGui::NextColumn();
    ImGui::TextColored(GetEfficiencyColor(bureaucratic_efficiency), "%.1f%%", bureaucratic_efficiency * 100.0);
    ImGui::NextColumn();

    if (governance) {
        ImGui::Separator();

        // Governance info
        ImGui::Text("Governance Type:");
        ImGui::NextColumn();
        ImGui::Text("%s", GetGovernanceTypeName(governance->governance_type));
        ImGui::NextColumn();

        ImGui::Text("Governance Stability:");
        ImGui::NextColumn();
        ImGui::TextColored(GetEfficiencyColor(governance->governance_stability), "%.1f%%", governance->governance_stability * 100.0);
        ImGui::NextColumn();

        ImGui::Separator();

        // Costs
        ImGui::Text("Monthly Admin Costs:");
        ImGui::NextColumn();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f)); // Red
        ImGui::Text("$%.0f", governance->monthly_administrative_costs);
        ImGui::PopStyleColor();
        ImGui::NextColumn();

        ImGui::Text("Official Salaries:");
        ImGui::NextColumn();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f)); // Red
        ImGui::Text("$%.0f", governance->official_salaries);
        ImGui::PopStyleColor();
        ImGui::NextColumn();

        ImGui::Text("Infrastructure Costs:");
        ImGui::NextColumn();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f)); // Red
        ImGui::Text("$%.0f", governance->infrastructure_costs);
        ImGui::PopStyleColor();
        ImGui::NextColumn();

        ImGui::Separator();

        // Officials count
        size_t officials_count = governance->appointed_officials.size();
        ImGui::Text("Appointed Officials:");
        ImGui::NextColumn();
        ImGui::Text("%zu", officials_count);
        ImGui::NextColumn();

        ImGui::Text("Bureaucratic Capacity:");
        ImGui::NextColumn();
        ImGui::Text("%.0f", governance->bureaucratic_capacity);
        ImGui::NextColumn();
    }

    ImGui::PopStyleColor();
    ImGui::Columns(1);

    // Governance type change
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("GOVERNANCE REFORMS");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    if (governance) {
        new_governance_type_ = static_cast<int>(governance->governance_type);
    }

    const char* governance_types[] = {"Feudal", "Centralized", "Bureaucratic", "Merchant Republic", "Theocracy", "Tribal"};
    if (ImGui::Combo("Governance Type", &new_governance_type_, governance_types, 6)) {
        // Update governance type
        administrative_system_.UpdateGovernanceType(current_player_entity_,
                                                   static_cast<game::administration::GovernanceType>(new_governance_type_));
    }

    ImGui::Spacing();
    if (ImGui::Button("Enact Administrative Reforms", ImVec2(250, 0))) {
        administrative_system_.ProcessAdministrativeReforms(current_player_entity_);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Improve administrative efficiency (costs money)");
    }
}

void AdministrativeWindow::RenderOfficialsTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("APPOINTED OFFICIALS");
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::Spacing();

    // Get governance component for officials list
    auto entity_handle = entity_manager_.GetEntityHandle(current_player_entity_);
    auto governance = entity_manager_.GetComponent<game::administration::GovernanceComponent>(entity_handle);

    if (governance && !governance->appointed_officials.empty()) {
        // Officials table
        ImGui::Columns(7, "officials", true);
        ImGui::SetColumnWidth(0, 150);
        ImGui::SetColumnWidth(1, 120);
        ImGui::SetColumnWidth(2, 80);
        ImGui::SetColumnWidth(3, 80);
        ImGui::SetColumnWidth(4, 80);
        ImGui::SetColumnWidth(5, 120);
        ImGui::SetColumnWidth(6, 100);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
        ImGui::Text("Name");
        ImGui::NextColumn();
        ImGui::Text("Position");
        ImGui::NextColumn();
        ImGui::Text("Competence");
        ImGui::NextColumn();
        ImGui::Text("Loyalty");
        ImGui::NextColumn();
        ImGui::Text("Efficiency");
        ImGui::NextColumn();
        ImGui::Text("Traits");
        ImGui::NextColumn();
        ImGui::Text("Actions");
        ImGui::NextColumn();
        ImGui::Separator();
        ImGui::PopStyleColor();

        // List officials
        for (const auto& official : governance->appointed_officials) {
            ImGui::PushID(official.official_id);

            ImGui::Text("%s", official.name.c_str());
            ImGui::NextColumn();

            ImGui::Text("%s", GetOfficialTypeName(official.type));
            ImGui::NextColumn();

            ImGui::TextColored(GetEfficiencyColor(official.competence), "%.0f%%", official.competence * 100.0);
            ImGui::NextColumn();

            ImGui::TextColored(GetEfficiencyColor(official.loyalty), "%.0f%%", official.loyalty * 100.0);
            ImGui::NextColumn();

            ImGui::TextColored(GetEfficiencyColor(official.efficiency), "%.0f%%", official.efficiency * 100.0);
            ImGui::NextColumn();

            // Display traits
            if (!official.traits.empty()) {
                std::string traits_str;
                for (size_t i = 0; i < official.traits.size() && i < 2; ++i) {
                    if (i > 0) traits_str += ", ";
                    traits_str += GetOfficialTraitName(official.traits[i]);
                }
                if (official.traits.size() > 2) {
                    traits_str += "...";
                }
                ImGui::Text("%s", traits_str.c_str());
            } else {
                ImGui::Text("None");
            }
            ImGui::NextColumn();

            if (ImGui::Button("Dismiss", ImVec2(80, 0))) {
                administrative_system_.DismissOfficial(current_player_entity_, official.official_id);
            }
            ImGui::NextColumn();

            ImGui::PopID();
        }

        ImGui::Columns(1);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
        ImGui::Text("No officials appointed");
        ImGui::PopStyleColor();
    }

    // Appointment section
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("APPOINT NEW OFFICIAL");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    const char* official_types[] = {
        "Tax Collector", "Trade Minister", "Military Governor",
        "Court Advisor", "Provincial Governor", "Judge", "Scribe", "Customs Officer"
    };

    ImGui::Text("Official Name:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    ImGui::InputText("##official_name", new_official_name_, 64);

    ImGui::Text("Position:     ");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    ImGui::Combo("##official_type", &selected_official_type_, official_types, 8);

    ImGui::Spacing();
    if (ImGui::Button("Appoint Official", ImVec2(150, 0))) {
        if (strlen(new_official_name_) > 0) {
            administrative_system_.AppointOfficial(
                current_player_entity_,
                static_cast<game::administration::OfficialType>(selected_official_type_),
                std::string(new_official_name_)
            );
            // Clear the input
            new_official_name_[0] = '\0';
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Appoint a new official to this position");
    }
}

void AdministrativeWindow::RenderBureaucracyTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("BUREAUCRACY");
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::Spacing();

    // Get bureaucracy component
    auto entity_handle = entity_manager_.GetEntityHandle(current_player_entity_);
    auto bureaucracy = entity_manager_.GetComponent<game::administration::BureaucracyComponent>(entity_handle);

    if (bureaucracy) {
        ImGui::Columns(2, "bureaucracy", false);
        ImGui::SetColumnWidth(0, 250);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));

        ImGui::Text("Bureaucracy Level:");
        ImGui::NextColumn();
        ImGui::Text("%u", bureaucracy->bureaucracy_level);
        ImGui::NextColumn();

        ImGui::Text("Scribes Employed:");
        ImGui::NextColumn();
        ImGui::Text("%u", bureaucracy->scribes_employed);
        ImGui::NextColumn();

        ImGui::Text("Clerks Employed:");
        ImGui::NextColumn();
        ImGui::Text("%u", bureaucracy->clerks_employed);
        ImGui::NextColumn();

        ImGui::Text("Administrators:");
        ImGui::NextColumn();
        ImGui::Text("%u", bureaucracy->administrators_employed);
        ImGui::NextColumn();

        ImGui::Separator();

        ImGui::Text("Record Keeping Quality:");
        ImGui::NextColumn();
        ImGui::TextColored(GetEfficiencyColor(bureaucracy->record_keeping_quality), "%.1f%%", bureaucracy->record_keeping_quality * 100.0);
        ImGui::NextColumn();

        ImGui::Text("Document Accuracy:");
        ImGui::NextColumn();
        ImGui::TextColored(GetEfficiencyColor(bureaucracy->document_accuracy), "%.1f%%", bureaucracy->document_accuracy * 100.0);
        ImGui::NextColumn();

        ImGui::Text("Administrative Speed:");
        ImGui::NextColumn();
        ImGui::TextColored(GetEfficiencyColor(bureaucracy->administrative_speed), "%.1f%%", bureaucracy->administrative_speed * 100.0);
        ImGui::NextColumn();

        ImGui::Separator();

        ImGui::Text("Corruption Level:");
        ImGui::NextColumn();
        // Invert color for corruption (low is good)
        ImGui::TextColored(GetEfficiencyColor(1.0 - bureaucracy->corruption_level), "%.1f%%", bureaucracy->corruption_level * 100.0);
        ImGui::NextColumn();

        ImGui::Text("Oversight Effectiveness:");
        ImGui::NextColumn();
        ImGui::TextColored(GetEfficiencyColor(bureaucracy->oversight_effectiveness), "%.1f%%", bureaucracy->oversight_effectiveness * 100.0);
        ImGui::NextColumn();

        ImGui::Text("Documents/Month:");
        ImGui::NextColumn();
        ImGui::Text("%u", bureaucracy->documents_processed_monthly);
        ImGui::NextColumn();

        ImGui::PopStyleColor();
        ImGui::Columns(1);

        // Actions
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
        ImGui::Text("BUREAUCRATIC IMPROVEMENTS");
        ImGui::PopStyleColor();
        ImGui::Spacing();

        if (ImGui::Button("Expand Bureaucracy (+5 Clerks)", ImVec2(250, 0))) {
            administrative_system_.ExpandBureaucracy(current_player_entity_, 5);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Hire additional clerks to increase capacity");
        }

        ImGui::Spacing();
        if (ImGui::Button("Improve Record Keeping", ImVec2(250, 0))) {
            administrative_system_.ImproveRecordKeeping(current_player_entity_, 1000.0);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Invest in better record keeping systems");
        }

        // Recent reforms
        if (!bureaucracy->recent_reforms.empty()) {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
            ImGui::Text("RECENT REFORMS");
            ImGui::PopStyleColor();
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
            for (const auto& reform : bureaucracy->recent_reforms) {
                ImGui::BulletText("%s", reform.c_str());
            }
            ImGui::PopStyleColor();
        }
    } else {
        ImGui::Text("No bureaucracy data available");
    }
}

void AdministrativeWindow::RenderLawOrderTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("LAW & ORDER");
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::Spacing();

    // Get law component
    auto entity_handle = entity_manager_.GetEntityHandle(current_player_entity_);
    auto law = entity_manager_.GetComponent<game::administration::LawComponent>(entity_handle);

    if (law) {
        ImGui::Columns(2, "law_order", false);
        ImGui::SetColumnWidth(0, 250);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));

        ImGui::Text("Law Enforcement:");
        ImGui::NextColumn();
        ImGui::TextColored(GetEfficiencyColor(law->law_enforcement_effectiveness), "%.1f%%", law->law_enforcement_effectiveness * 100.0);
        ImGui::NextColumn();

        ImGui::Text("Public Order:");
        ImGui::NextColumn();
        ImGui::TextColored(GetEfficiencyColor(law->public_order), "%.1f%%", law->public_order * 100.0);
        ImGui::NextColumn();

        ImGui::Text("Crime Rate:");
        ImGui::NextColumn();
        // Invert color for crime (low is good)
        ImGui::TextColored(GetEfficiencyColor(1.0 - law->crime_rate), "%.1f%%", law->crime_rate * 100.0);
        ImGui::NextColumn();

        ImGui::Separator();

        ImGui::Text("Courts Established:");
        ImGui::NextColumn();
        ImGui::Text("%u", law->courts_established);
        ImGui::NextColumn();

        ImGui::Text("Judges Appointed:");
        ImGui::NextColumn();
        ImGui::Text("%u", law->judges_appointed);
        ImGui::NextColumn();

        ImGui::Text("Bailiffs Employed:");
        ImGui::NextColumn();
        ImGui::Text("%u", law->bailiffs_employed);
        ImGui::NextColumn();

        ImGui::Separator();

        ImGui::Text("Legal Process Speed:");
        ImGui::NextColumn();
        ImGui::TextColored(GetEfficiencyColor(law->legal_process_speed), "%.1f%%", law->legal_process_speed * 100.0);
        ImGui::NextColumn();

        ImGui::Text("Justice Fairness:");
        ImGui::NextColumn();
        ImGui::TextColored(GetEfficiencyColor(law->justice_fairness), "%.1f%%", law->justice_fairness * 100.0);
        ImGui::NextColumn();

        ImGui::Text("Legal Accessibility:");
        ImGui::NextColumn();
        ImGui::TextColored(GetEfficiencyColor(law->legal_accessibility), "%.1f%%", law->legal_accessibility * 100.0);
        ImGui::NextColumn();

        ImGui::Separator();

        ImGui::Text("Cases Pending:");
        ImGui::NextColumn();
        ImGui::Text("%u", law->cases_pending);
        ImGui::NextColumn();

        ImGui::Text("Cases Resolved/Month:");
        ImGui::NextColumn();
        ImGui::Text("%u", law->cases_resolved_monthly);
        ImGui::NextColumn();

        ImGui::PopStyleColor();
        ImGui::Columns(1);

        // Active laws
        if (!law->active_laws.empty()) {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
            ImGui::Text("ACTIVE LAWS");
            ImGui::PopStyleColor();
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
            for (const auto& law_text : law->active_laws) {
                ImGui::BulletText("%s", law_text.c_str());
            }
            ImGui::PopStyleColor();
        }

        // Actions
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
        ImGui::Text("LEGAL ACTIONS");
        ImGui::PopStyleColor();
        ImGui::Spacing();

        if (ImGui::Button("Establish Court", ImVec2(200, 0))) {
            administrative_system_.EstablishCourt(current_player_entity_);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Build a new court to improve legal system");
        }

        ImGui::SameLine();
        if (ImGui::Button("Appoint Judge", ImVec2(200, 0))) {
            administrative_system_.AppointJudge(current_player_entity_, "Judge " + std::to_string(law->judges_appointed + 1));
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Appoint a new judge to handle more cases");
        }
    } else {
        ImGui::Text("No law system data available");
    }
}

// Helper functions
const char* AdministrativeWindow::GetOfficialTypeName(game::administration::OfficialType type) const {
    switch (type) {
        case game::administration::OfficialType::TAX_COLLECTOR: return "Tax Collector";
        case game::administration::OfficialType::TRADE_MINISTER: return "Trade Minister";
        case game::administration::OfficialType::MILITARY_GOVERNOR: return "Military Governor";
        case game::administration::OfficialType::COURT_ADVISOR: return "Court Advisor";
        case game::administration::OfficialType::PROVINCIAL_GOVERNOR: return "Provincial Governor";
        case game::administration::OfficialType::JUDGE: return "Judge";
        case game::administration::OfficialType::SCRIBE: return "Scribe";
        case game::administration::OfficialType::CUSTOMS_OFFICER: return "Customs Officer";
        default: return "Unknown";
    }
}

const char* AdministrativeWindow::GetGovernanceTypeName(game::administration::GovernanceType type) const {
    switch (type) {
        case game::administration::GovernanceType::FEUDAL: return "Feudal";
        case game::administration::GovernanceType::CENTRALIZED: return "Centralized";
        case game::administration::GovernanceType::BUREAUCRATIC: return "Bureaucratic";
        case game::administration::GovernanceType::MERCHANT_REPUBLIC: return "Merchant Republic";
        case game::administration::GovernanceType::THEOCRACY: return "Theocracy";
        case game::administration::GovernanceType::TRIBAL: return "Tribal";
        default: return "Unknown";
    }
}

const char* AdministrativeWindow::GetOfficialTraitName(game::administration::OfficialTrait trait) const {
    switch (trait) {
        case game::administration::OfficialTrait::CORRUPT: return "Corrupt";
        case game::administration::OfficialTrait::EFFICIENT: return "Efficient";
        case game::administration::OfficialTrait::LOYAL: return "Loyal";
        case game::administration::OfficialTrait::AMBITIOUS: return "Ambitious";
        case game::administration::OfficialTrait::EXPERIENCED: return "Experienced";
        case game::administration::OfficialTrait::YOUNG_TALENT: return "Young Talent";
        case game::administration::OfficialTrait::WELL_CONNECTED: return "Well Connected";
        case game::administration::OfficialTrait::STUBBORN: return "Stubborn";
        case game::administration::OfficialTrait::SCHOLARLY: return "Scholarly";
        default: return "None";
    }
}

ImVec4 AdministrativeWindow::GetEfficiencyColor(double efficiency) const {
    if (efficiency >= 0.75) {
        return ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green - Excellent
    } else if (efficiency >= 0.5) {
        return ImVec4(0.8f, 0.8f, 0.0f, 1.0f); // Yellow - Good
    } else if (efficiency >= 0.25) {
        return ImVec4(1.0f, 0.6f, 0.0f, 1.0f); // Orange - Poor
    } else {
        return ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // Red - Very Poor
    }
}

} // namespace ui
