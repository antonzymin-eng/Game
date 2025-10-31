#pragma once

#include "core/ecs/EntityManager.h"
#include "game/military/MilitarySystem.h"
#include "game/military/MilitaryComponents.h"
#include "game/military/BattleResolutionCalculator.h"
#include "game/types/Types.h"
#include <string>
#include <vector>

namespace ui {

/**
 * @brief Battle Viewer Window
 *
 * Provides a comprehensive interface for viewing active and historical battles,
 * including battle statistics, unit compositions, casualties, and outcomes.
 */
class BattleViewerWindow {
public:
    /**
     * @brief Construct a new Battle Viewer Window
     *
     * @param entity_manager Reference to the ECS entity manager
     * @param military_system Reference to the military system
     */
    BattleViewerWindow(
        ::core::ecs::EntityManager& entity_manager,
        game::military::MilitarySystem& military_system
    );

    ~BattleViewerWindow() = default;

    // Non-copyable
    BattleViewerWindow(const BattleViewerWindow&) = delete;
    BattleViewerWindow& operator=(const BattleViewerWindow&) = delete;

    /**
     * @brief Render the battle viewer window
     *
     * Called every frame to display the UI. Handles all ImGui rendering.
     */
    void Render();

    /**
     * @brief Set the visibility of the window
     *
     * @param visible True to show the window, false to hide it
     */
    void SetVisible(bool visible);

    /**
     * @brief Check if the window is visible
     *
     * @return true if visible, false otherwise
     */
    bool IsVisible() const;

    /**
     * @brief Toggle the window visibility
     */
    void ToggleVisibility();

    /**
     * @brief Set the selected battle to view
     *
     * @param battle_id The entity ID of the battle to view
     */
    void SetSelectedBattle(game::types::EntityID battle_id);

    /**
     * @brief Clear the battle selection
     */
    void ClearSelection();

private:
    // Dependencies
    ::core::ecs::EntityManager& entity_manager_;
    game::military::MilitarySystem& military_system_;

    // UI State
    bool visible_ = true;
    int current_tab_ = 0;
    game::types::EntityID selected_battle_ = 0;

    // Battle list state
    bool show_only_active_ = true;
    bool show_player_battles_only_ = false;
    char search_buffer_[256] = {0};

    // Helper methods for rendering tabs
    void RenderHeader();
    void RenderActiveBattlesTab();
    void RenderBattleHistoryTab();
    void RenderBattleDetailsTab();

    // Battle rendering helpers
    void RenderBattleListItem(game::types::EntityID battle_id, const game::military::CombatComponent* combat);
    void RenderBattleDetails(game::types::EntityID battle_id);
    void RenderArmyComposition(const game::military::ArmyComponent* army, const char* title);
    void RenderCasualtyReport(const game::military::CombatComponent* combat);
    void RenderBattleFactors(const game::military::CombatComponent* combat);
    void RenderBattleProgress(const game::military::CombatComponent* combat);

    // Utility methods
    std::string GetBattleLocationName(game::types::EntityID location_id) const;
    std::string GetArmyName(game::types::EntityID army_id) const;
    std::string GetNationName(game::types::EntityID nation_id) const;
    std::string GetBattlePhaseName(const std::string& phase) const;
    std::string GetUnitTypeName(game::military::UnitType type) const;
    std::string GetUnitClassName(game::military::UnitClass unit_class) const;
    const char* GetUnitClassIcon(game::military::UnitClass unit_class) const;

    // Color helpers
    struct ImVec4 GetMoraleColor(double morale) const;
    struct ImVec4 GetCasualtyColor(double casualty_rate) const;
    struct ImVec4 GetVictoryChanceColor(double chance) const;
    struct ImVec4 GetUnitClassColor(game::military::UnitClass unit_class) const;

    // Battle state helpers
    bool IsBattleActive(const game::military::CombatComponent* combat) const;
    double GetBattleProgress(const game::military::CombatComponent* combat) const;
    std::string GetBattleOutcomePrediction(const game::military::CombatComponent* combat) const;
};

} // namespace ui
