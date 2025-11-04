#include "ui/BattleViewerWindow.h"
#include "imgui.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstring>  // For memset

namespace ui {

BattleViewerWindow::BattleViewerWindow(
    ::core::ecs::EntityManager& entity_manager,
    game::military::MilitarySystem& military_system
)
    : entity_manager_(entity_manager),
      military_system_(military_system),
      visible_(true),
      current_tab_(0),
      selected_battle_(0),
      show_only_active_(true),
      show_player_battles_only_(false) {
    std::memset(search_buffer_, 0, sizeof(search_buffer_));
}

void BattleViewerWindow::Render() {
    if (!visible_) {
        return;
    }

    // Set window position and size
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos;
    ImVec2 work_size = viewport->WorkSize;

    // Position window in center-right
    ImVec2 window_pos = ImVec2(work_pos.x + work_size.x * 0.25f, work_pos.y + work_size.y * 0.15f);
    ImVec2 window_size = ImVec2(work_size.x * 0.5f, work_size.y * 0.7f);

    ImGui::SetNextWindowPos(window_pos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(window_size, ImGuiCond_FirstUseEver);

    // Window flags
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;

    if (ImGui::Begin("Battle Viewer", &visible_, window_flags)) {
        RenderHeader();

        ImGui::Separator();

        // Tab bar for different views
        if (ImGui::BeginTabBar("BattleViewerTabs", ImGuiTabBarFlags_None)) {
            if (ImGui::BeginTabItem("Active Battles")) {
                RenderActiveBattlesTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Battle History")) {
                RenderBattleHistoryTab();
                ImGui::EndTabItem();
            }

            if (selected_battle_ != 0) {
                if (ImGui::BeginTabItem("Battle Details")) {
                    RenderBattleDetailsTab();
                    ImGui::EndTabItem();
                }
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void BattleViewerWindow::SetVisible(bool visible) {
    visible_ = visible;
}

bool BattleViewerWindow::IsVisible() const {
    return visible_;
}

void BattleViewerWindow::ToggleVisibility() {
    visible_ = !visible_;
}

void BattleViewerWindow::SetSelectedBattle(game::types::EntityID battle_id) {
    selected_battle_ = battle_id;
}

void BattleViewerWindow::ClearSelection() {
    selected_battle_ = 0;
}

void BattleViewerWindow::RenderHeader() {
    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Battle Viewer");

    // Display statistics
    auto all_armies = military_system_.GetAllArmies();
    int active_battles = 0;
    int total_combatants = 0;

    // Count active battles (simplified - would need proper ECS query)
    // This is a placeholder implementation
    ImGui::Spacing();
    ImGui::Text("Active Battles: %d", active_battles);
    ImGui::SameLine(200);
    ImGui::Text("Total Combatants: %d", total_combatants);

    ImGui::Spacing();

    // Filters
    ImGui::Checkbox("Show Active Only", &show_only_active_);
    ImGui::SameLine();
    ImGui::Checkbox("Player Battles Only", &show_player_battles_only_);
}

void BattleViewerWindow::RenderActiveBattlesTab() {
    ImGui::BeginChild("ActiveBattles", ImVec2(0, 0), true);

    // Get all active battles (placeholder - would need proper ECS query)
    // For now, display a message
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No active battles at the moment");

    // Example of how battle list would look:
    /*
    for (auto battle_id : active_battles) {
        auto* combat = GetCombatComponent(battle_id);
        if (combat && IsBattleActive(combat)) {
            RenderBattleListItem(battle_id, combat);
        }
    }
    */

    ImGui::EndChild();
}

void BattleViewerWindow::RenderBattleHistoryTab() {
    ImGui::BeginChild("BattleHistory", ImVec2(0, 0), true);

    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Recent Battles");
    ImGui::Separator();

    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No battle history available");

    // Example of how battle history would look:
    /*
    for (const auto& battle_record : battle_history) {
        ImGui::Separator();
        ImGui::Text("Battle of %s", battle_record.location.c_str());
        ImGui::Text("Date: %s", battle_record.date.c_str());
        ImGui::Text("Outcome: %s", battle_record.outcome.c_str());

        if (ImGui::Button(("View Details##" + std::to_string(battle_record.id)).c_str())) {
            selected_battle_ = battle_record.id;
        }
    }
    */

    ImGui::EndChild();
}

void BattleViewerWindow::RenderBattleDetailsTab() {
    if (selected_battle_ == 0) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "No battle selected");
        return;
    }

    ImGui::BeginChild("BattleDetails", ImVec2(0, 0), true);

    RenderBattleDetails(selected_battle_);

    ImGui::EndChild();
}

void BattleViewerWindow::RenderBattleListItem(
    game::types::EntityID battle_id,
    const game::military::CombatComponent* combat
) {
    if (!combat) return;

    ImGui::PushID(battle_id);

    // Battle header
    std::string location_name = GetBattleLocationName(combat->location);
    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Battle of %s", location_name.c_str());

    // Combatants
    std::string attacker_name = GetArmyName(combat->attacker_army);
    std::string defender_name = GetArmyName(combat->defender_army);

    ImGui::Text("%s vs %s", attacker_name.c_str(), defender_name.c_str());

    // Battle progress
    if (combat->battle_active) {
        float progress = static_cast<float>(GetBattleProgress(combat));
        ImGui::ProgressBar(progress, ImVec2(-1, 0), "");

        // Battle phase
        std::string phase = GetBattlePhaseName(combat->current_phase);
        ImGui::Text("Phase: %s", phase.c_str());
    }

    // View details button
    if (ImGui::Button("View Details")) {
        selected_battle_ = battle_id;
        current_tab_ = 2; // Switch to details tab
    }

    ImGui::Separator();
    ImGui::PopID();
}

void BattleViewerWindow::RenderBattleDetails(game::types::EntityID battle_id) {
    // Get combat component
    // This is a placeholder - would need proper component access
    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Battle Details");
    ImGui::Separator();

    ImGui::Text("Battle ID: %u", battle_id);

    // Example layout for battle details
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "Combatants");
    ImGui::Separator();

    // Split view for attacker and defender
    float available_width = ImGui::GetContentRegionAvail().x;
    float column_width = available_width * 0.48f;

    ImGui::BeginGroup();
    {
        ImGui::BeginChild("AttackerPanel", ImVec2(column_width, 300), true);
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Attacker");
        ImGui::Separator();
        // Would show army composition here
        ImGui::Text("Army: [Name]");
        ImGui::Text("Strength: [X,XXX]");
        ImGui::Text("Morale: [XX%%]");
        ImGui::EndChild();
    }
    ImGui::EndGroup();

    ImGui::SameLine();

    ImGui::BeginGroup();
    {
        ImGui::BeginChild("DefenderPanel", ImVec2(column_width, 300), true);
        ImGui::TextColored(ImVec4(0.3f, 0.3f, 1.0f, 1.0f), "Defender");
        ImGui::Separator();
        // Would show army composition here
        ImGui::Text("Army: [Name]");
        ImGui::Text("Strength: [X,XXX]");
        ImGui::Text("Morale: [XX%%]");
        ImGui::EndChild();
    }
    ImGui::EndGroup();

    ImGui::Spacing();
    ImGui::Spacing();

    // Battle factors
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "Battle Factors");
    ImGui::Separator();

    ImGui::Text("Terrain: [Type]");
    ImGui::Text("Weather: [Condition]");
    ImGui::Text("Fortification: [Level]");

    ImGui::Spacing();
    ImGui::Spacing();

    // Casualty report
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "Casualties");
    ImGui::Separator();

    ImGui::Text("Attacker: [X,XXX]");
    ImGui::Text("Defender: [X,XXX]");
}

void BattleViewerWindow::RenderArmyComposition(
    const game::military::ArmyComponent* army,
    const char* title
) {
    if (!army) return;

    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "%s", title);
    ImGui::Separator();

    ImGui::Text("Total Strength: %u", army->total_strength);
    ImGui::Text("Morale: %.1f%%", army->army_morale * 100.0);
    ImGui::Text("Organization: %.1f%%", army->organization * 100.0);

    ImGui::Spacing();

    // Unit breakdown
    ImGui::Text("Units:");
    for (const auto& unit : army->units) {
        std::string unit_type = GetUnitTypeName(unit.type);
        ImGui::BulletText("%s: %u/%u", unit_type.c_str(), unit.current_strength, unit.max_strength);
    }
}

void BattleViewerWindow::RenderCasualtyReport(const game::military::CombatComponent* combat) {
    if (!combat) return;

    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Casualty Report");
    ImGui::Separator();

    // Attacker casualties
    ImGui::Text("Attacker Casualties: %u", combat->attacker_casualties);
    if (combat->attacker_initial_strength > 0) {
        float casualty_rate = static_cast<float>(combat->attacker_casualties) /
                              static_cast<float>(combat->attacker_initial_strength);
        ImVec4 color = GetCasualtyColor(casualty_rate);
        ImGui::SameLine();
        ImGui::TextColored(color, "(%.1f%%)", casualty_rate * 100.0f);
    }

    // Defender casualties
    ImGui::Text("Defender Casualties: %u", combat->defender_casualties);
    if (combat->defender_initial_strength > 0) {
        float casualty_rate = static_cast<float>(combat->defender_casualties) /
                              static_cast<float>(combat->defender_initial_strength);
        ImVec4 color = GetCasualtyColor(casualty_rate);
        ImGui::SameLine();
        ImGui::TextColored(color, "(%.1f%%)", casualty_rate * 100.0f);
    }

    // Casualty ratio
    if (combat->defender_casualties > 0) {
        float ratio = static_cast<float>(combat->attacker_casualties) /
                      static_cast<float>(combat->defender_casualties);
        ImGui::Text("Casualty Ratio: %.2f:1", ratio);
    }
}

void BattleViewerWindow::RenderBattleFactors(const game::military::CombatComponent* combat) {
    if (!combat) return;

    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Battle Factors");
    ImGui::Separator();

    ImGui::Text("Terrain Modifier: %.2fx", combat->terrain_modifier);
    ImGui::Text("Weather Modifier: %.2fx", combat->weather_modifier);
    ImGui::Text("Fortification Bonus: %.2f", combat->fortification_bonus);
    ImGui::Text("Commander Skill Difference: %.2f", combat->commander_skill_difference);
}

void BattleViewerWindow::RenderBattleProgress(const game::military::CombatComponent* combat) {
    if (!combat) return;

    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Battle Progress");
    ImGui::Separator();

    if (combat->battle_active) {
        // Current phase
        std::string phase = GetBattlePhaseName(combat->current_phase);
        ImGui::Text("Current Phase: %s", phase.c_str());

        // Phase progress
        float phase_progress = static_cast<float>(combat->phase_progress);
        ImGui::ProgressBar(phase_progress, ImVec2(-1, 0), "");

        // Battle duration
        ImGui::Text("Battle Duration: %.1f minutes", combat->battle_duration);

        // Morale status
        ImGui::Spacing();
        ImGui::Text("Attacker Morale:");
        ImGui::SameLine();
        ImVec4 attacker_morale_color = GetMoraleColor(combat->attacker_morale);
        ImGui::TextColored(attacker_morale_color, "%.1f%%", combat->attacker_morale * 100.0);

        ImGui::Text("Defender Morale:");
        ImGui::SameLine();
        ImVec4 defender_morale_color = GetMoraleColor(combat->defender_morale);
        ImGui::TextColored(defender_morale_color, "%.1f%%", combat->defender_morale * 100.0);

        // Victory prediction
        ImGui::Spacing();
        std::string prediction = GetBattleOutcomePrediction(combat);
        ImGui::Text("Predicted Outcome: %s", prediction.c_str());
    } else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Battle is not active");
    }
}

// Utility method implementations

std::string BattleViewerWindow::GetBattleLocationName(game::types::EntityID location_id) const {
    // Placeholder - would need to query province name
    return "Unknown Location";
}

std::string BattleViewerWindow::GetArmyName(game::types::EntityID army_id) const {
    // Placeholder - would need to query army component
    return "Unknown Army";
}

std::string BattleViewerWindow::GetNationName(game::types::EntityID nation_id) const {
    // Placeholder - would need to query nation data
    return "Unknown Nation";
}

std::string BattleViewerWindow::GetBattlePhaseName(const std::string& phase) const {
    if (phase == "engagement") return "Engagement";
    if (phase == "melee") return "Melee Combat";
    if (phase == "pursuit") return "Pursuit";
    return "Unknown Phase";
}

std::string BattleViewerWindow::GetUnitTypeName(game::military::UnitType type) const {
    switch (type) {
        case game::military::UnitType::LEVIES: return "Levies";
        case game::military::UnitType::SPEARMEN: return "Spearmen";
        case game::military::UnitType::CROSSBOWMEN: return "Crossbowmen";
        case game::military::UnitType::LONGBOWMEN: return "Longbowmen";
        case game::military::UnitType::MEN_AT_ARMS: return "Men-at-Arms";
        case game::military::UnitType::PIKEMEN: return "Pikemen";
        case game::military::UnitType::ARQUEBUSIERS: return "Arquebusiers";
        case game::military::UnitType::MUSKETEERS: return "Musketeers";
        case game::military::UnitType::LIGHT_CAVALRY: return "Light Cavalry";
        case game::military::UnitType::HEAVY_CAVALRY: return "Heavy Cavalry";
        case game::military::UnitType::MOUNTED_ARCHERS: return "Mounted Archers";
        case game::military::UnitType::DRAGOONS: return "Dragoons";
        case game::military::UnitType::CATAPULTS: return "Catapults";
        case game::military::UnitType::TREBUCHETS: return "Trebuchets";
        case game::military::UnitType::CANNONS: return "Cannons";
        case game::military::UnitType::SIEGE_TOWERS: return "Siege Towers";
        default: return "Unknown Unit";
    }
}

std::string BattleViewerWindow::GetUnitClassName(game::military::UnitClass unit_class) const {
    switch (unit_class) {
        case game::military::UnitClass::INFANTRY: return "Infantry";
        case game::military::UnitClass::CAVALRY: return "Cavalry";
        case game::military::UnitClass::SIEGE: return "Siege";
        case game::military::UnitClass::NAVAL: return "Naval";
        default: return "Unknown";
    }
}

const char* BattleViewerWindow::GetUnitClassIcon(game::military::UnitClass unit_class) const {
    switch (unit_class) {
        case game::military::UnitClass::INFANTRY: return "[I]";
        case game::military::UnitClass::CAVALRY: return "[C]";
        case game::military::UnitClass::SIEGE: return "[S]";
        case game::military::UnitClass::NAVAL: return "[N]";
        default: return "[?]";
    }
}

ImVec4 BattleViewerWindow::GetMoraleColor(double morale) const {
    if (morale < 0.3) {
        return ImVec4(1.0f, 0.2f, 0.2f, 1.0f); // Red - routing
    } else if (morale < 0.5) {
        return ImVec4(1.0f, 0.6f, 0.2f, 1.0f); // Orange - wavering
    } else if (morale < 0.8) {
        return ImVec4(1.0f, 1.0f, 0.3f, 1.0f); // Yellow - steady
    } else {
        return ImVec4(0.3f, 1.0f, 0.3f, 1.0f); // Green - confident
    }
}

ImVec4 BattleViewerWindow::GetCasualtyColor(double casualty_rate) const {
    if (casualty_rate < 0.1) {
        return ImVec4(0.3f, 1.0f, 0.3f, 1.0f); // Green - light
    } else if (casualty_rate < 0.3) {
        return ImVec4(1.0f, 1.0f, 0.3f, 1.0f); // Yellow - moderate
    } else if (casualty_rate < 0.5) {
        return ImVec4(1.0f, 0.6f, 0.2f, 1.0f); // Orange - heavy
    } else {
        return ImVec4(1.0f, 0.2f, 0.2f, 1.0f); // Red - devastating
    }
}

ImVec4 BattleViewerWindow::GetVictoryChanceColor(double chance) const {
    if (chance < 0.3) {
        return ImVec4(1.0f, 0.2f, 0.2f, 1.0f); // Red - unlikely
    } else if (chance < 0.5) {
        return ImVec4(1.0f, 0.6f, 0.2f, 1.0f); // Orange - uncertain
    } else if (chance < 0.7) {
        return ImVec4(1.0f, 1.0f, 0.3f, 1.0f); // Yellow - likely
    } else {
        return ImVec4(0.3f, 1.0f, 0.3f, 1.0f); // Green - very likely
    }
}

ImVec4 BattleViewerWindow::GetUnitClassColor(game::military::UnitClass unit_class) const {
    switch (unit_class) {
        case game::military::UnitClass::INFANTRY:
            return ImVec4(0.7f, 0.5f, 0.3f, 1.0f); // Brown
        case game::military::UnitClass::CAVALRY:
            return ImVec4(0.8f, 0.7f, 0.3f, 1.0f); // Gold
        case game::military::UnitClass::SIEGE:
            return ImVec4(0.5f, 0.5f, 0.5f, 1.0f); // Gray
        case game::military::UnitClass::NAVAL:
            return ImVec4(0.3f, 0.5f, 0.8f, 1.0f); // Blue
        default:
            return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    }
}

bool BattleViewerWindow::IsBattleActive(const game::military::CombatComponent* combat) const {
    return combat && combat->battle_active;
}

double BattleViewerWindow::GetBattleProgress(const game::military::CombatComponent* combat) const {
    if (!combat) return 0.0;

    // Simple progress based on casualty rate
    double total_initial = combat->attacker_initial_strength + combat->defender_initial_strength;
    double total_casualties = combat->attacker_casualties + combat->defender_casualties;

    if (total_initial > 0) {
        return std::min(1.0, total_casualties / (total_initial * 0.5));
    }

    return 0.0;
}

std::string BattleViewerWindow::GetBattleOutcomePrediction(const game::military::CombatComponent* combat) const {
    if (!combat) return "Unknown";

    double chance = combat->attacker_victory_chance;

    if (chance < 0.2) return "Defender Victory Likely";
    if (chance < 0.4) return "Defender Advantage";
    if (chance < 0.6) return "Even Match";
    if (chance < 0.8) return "Attacker Advantage";
    return "Attacker Victory Likely";
}

} // namespace ui
