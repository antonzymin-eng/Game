// ============================================================================
// MilitaryCampaignManager.h - Central Manager for Military Campaign Systems
// Created: November 18, 2025
// Description: Integrates fog of war, LOS, orders, delays, and news systems
//              Coordinates all military campaign mechanics
// ============================================================================

#pragma once

#include "game/military/MilitaryOrders.h"
#include "game/military/CommandDelay.h"
#include "game/news/NewsDelaySystem.h"
#include "game/player/PlayerLocation.h"
#include "map/FogOfWar.h"
#include "map/LineOfSight.h"
#include "core/ECS/EntityManager.h"
#include <memory>

namespace game::military {

    // ========================================================================
    // MilitaryCampaignManager - Coordinates all campaign systems
    // ========================================================================
    class MilitaryCampaignManager {
    public:
        MilitaryCampaignManager(::core::ecs::EntityManager& entity_manager);
        ~MilitaryCampaignManager() = default;

        // Initialize all subsystems
        bool Initialize();

        // Main update loop - call every game tick
        void Update(float delta_time, float current_game_time);

        // Update fog of war based on army positions
        void UpdateFogOfWar(game::types::EntityID player_id, float current_game_time);

        // Issue order to army
        void IssueOrderToArmy(
            game::types::EntityID army_id,
            const MilitaryOrder& order,
            float current_game_time
        );

        // Handle player location changes
        void SetPlayerLocation(
            game::types::EntityID player_id,
            player::PlayerLocationType location_type,
            game::types::EntityID location_entity,
            const map::Vector2& position,
            float current_game_time
        );

        // Activate/deactivate regent
        void ActivateRegent(
            game::types::EntityID player_id,
            player::RegentType regent_type,
            game::types::EntityID regent_character,
            float current_game_time
        );

        void DeactivateRegent(
            game::types::EntityID player_id,
            float current_game_time
        );

        // Send news about military events
        void SendBattleReport(
            game::types::EntityID player_id,
            const std::string& battle_name,
            const map::Vector2& battle_location,
            bool victory,
            int casualties,
            float event_time
        );

        void SendArmyStatusUpdate(
            game::types::EntityID player_id,
            game::types::EntityID army_id,
            const std::string& status_message,
            const map::Vector2& army_position,
            float event_time
        );

        // Get subsystems
        map::FogOfWarManager* GetFogOfWarManager() { return fog_of_war_manager_.get(); }
        map::LineOfSightCalculator* GetLOSCalculator() { return los_calculator_.get(); }
        CommandDelaySystem* GetCommandDelaySystem() { return command_delay_system_.get(); }
        news::NewsDelaySystem* GetNewsDelaySystem() { return news_delay_system_.get(); }

        // Settings
        void SetFogOfWarEnabled(bool enabled) { fog_of_war_enabled_ = enabled; }
        void SetNewsDelayEnabled(bool enabled) { news_delay_enabled_ = enabled; }
        void SetCommandDelayEnabled(bool enabled) { command_delay_enabled_ = enabled; }

        bool IsFogOfWarEnabled() const { return fog_of_war_enabled_; }
        bool IsNewsDelayEnabled() const { return news_delay_enabled_; }
        bool IsCommandDelayEnabled() const { return command_delay_enabled_; }

    private:
        ::core::ecs::EntityManager& entity_manager_;

        // Subsystems
        std::unique_ptr<map::FogOfWarManager> fog_of_war_manager_;
        std::unique_ptr<map::LineOfSightCalculator> los_calculator_;
        std::unique_ptr<CommandDelaySystem> command_delay_system_;
        std::unique_ptr<news::NewsDelaySystem> news_delay_system_;

        // Settings
        bool fog_of_war_enabled_ = true;
        bool news_delay_enabled_ = true;
        bool command_delay_enabled_ = true;

        // Helper methods
        void UpdateArmyOrders(float delta_time, float current_game_time);
        void UpdateArmyVisibility(game::types::EntityID player_id, float current_game_time);
        void ProcessPendingCommands(float current_game_time);
        void GenerateAutomaticReports(float current_game_time);

        // Get army position
        map::Vector2 GetArmyPosition(game::types::EntityID army_id) const;

        // Calculate vision range for army
        float GetArmyVisionRange(game::types::EntityID army_id) const;
    };

} // namespace game::military
