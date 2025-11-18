// ============================================================================
// NewsDelaySystem.h - News Delay and Propagation Manager
// Created: November 18, 2025
// Description: Manages delayed news delivery based on player location,
//              integrates with command delay system, handles regent reports
// ============================================================================

#pragma once

#include "game/news/NewsSystem.h"
#include "game/player/PlayerLocation.h"
#include "game/military/CommandDelay.h"
#include "core/ECS/EntityManager.h"
#include <memory>

namespace game::news {

    // ========================================================================
    // NewsDelaySystem - Manages news propagation and delays
    // ========================================================================
    class NewsDelaySystem {
    public:
        NewsDelaySystem();
        ~NewsDelaySystem() = default;

        // Initialize with entity manager and command delay calculator
        void Initialize(
            ::core::ecs::EntityManager& entity_manager,
            military::CommandDelayCalculator* delay_calculator
        );

        // Send news to player with automatic delay calculation
        void SendNewsToPlayer(
            game::types::EntityID player_entity,
            const NewsMessage& message,
            float current_game_time
        );

        // Update system - check for arrived messages
        void Update(float current_game_time);

        // Generate regent report
        RegentReport GenerateRegentReport(
            game::types::EntityID player_entity,
            float report_start_time,
            float report_end_time
        );

        // Send regent report to player
        void SendRegentReport(
            game::types::EntityID player_entity,
            const RegentReport& report,
            float current_game_time
        );

        // Calculate delay for news based on player location
        float CalculateNewsDelay(
            game::types::EntityID player_entity,
            const map::Vector2& news_origin,
            NewsPriority priority,
            float current_game_time
        ) const;

        // Check if regent should send report
        bool ShouldSendRegentReport(
            game::types::EntityID player_entity,
            float current_game_time
        ) const;

        // Activate regent when player leaves capital
        void ActivateRegent(
            game::types::EntityID player_entity,
            player::RegentType regent_type,
            game::types::EntityID regent_character,
            float current_game_time
        );

        // Deactivate regent when player returns to capital
        void DeactivateRegent(
            game::types::EntityID player_entity,
            float current_game_time
        );

        // Get regent decision quality modifier
        float GetRegentDecisionQuality(game::types::EntityID player_entity) const;

        // Settings
        void SetBaseDelayMultiplier(float multiplier) { base_delay_multiplier_ = multiplier; }
        void SetMinimumDelay(float hours) { minimum_delay_hours_ = hours; }
        void SetMaximumDelay(float hours) { maximum_delay_hours_ = hours; }

        float GetBaseDelayMultiplier() const { return base_delay_multiplier_; }
        float GetMinimumDelay() const { return minimum_delay_hours_; }
        float GetMaximumDelay() const { return maximum_delay_hours_; }

        // Statistics
        int GetPendingMessageCount(game::types::EntityID player_entity) const;
        int GetUnreadMessageCount(game::types::EntityID player_entity) const;

    private:
        ::core::ecs::EntityManager* entity_manager_ = nullptr;
        military::CommandDelayCalculator* delay_calculator_ = nullptr;

        // Settings
        float base_delay_multiplier_ = 1.0f;    // Global delay multiplier
        float minimum_delay_hours_ = 0.1f;      // Minimum delay (6 minutes)
        float maximum_delay_hours_ = 720.0f;    // Maximum delay (30 days)

        // Helper methods
        player::PlayerLocationComponent* GetPlayerLocation(game::types::EntityID player_entity);
        const player::PlayerLocationComponent* GetPlayerLocation(game::types::EntityID player_entity) const;

        player::RegentComponent* GetRegent(game::types::EntityID player_entity);
        const player::RegentComponent* GetRegent(game::types::EntityID player_entity) const;

        MessageInboxComponent* GetInbox(game::types::EntityID player_entity);
        const MessageInboxComponent* GetInbox(game::types::EntityID player_entity) const;

        // Process arrived messages
        void ProcessArrivedMessages(
            game::types::EntityID player_entity,
            MessageInboxComponent* inbox,
            float current_game_time
        );

        // Create news message from regent report
        NewsMessage CreateRegentReportMessage(
            const RegentReport& report,
            const map::Vector2& capital_position,
            float current_game_time
        );
    };

    // ========================================================================
    // NewsFactory - Helper class to create common news messages
    // ========================================================================
    class NewsFactory {
    public:
        // Battle report
        static NewsMessage CreateBattleReport(
            const std::string& battle_name,
            const map::Vector2& battle_location,
            game::types::EntityID attacker_army,
            game::types::EntityID defender_army,
            bool victory,
            int casualties,
            float event_time
        );

        // Economic report
        static NewsMessage CreateEconomicReport(
            game::types::EntityID province,
            const std::string& report_content,
            double income,
            double expenses,
            float event_time
        );

        // Diplomatic message
        static NewsMessage CreateDiplomaticMessage(
            game::types::EntityID sender_nation,
            const std::string& sender_name,
            const std::string& message_content,
            bool requires_response,
            float event_time
        );

        // Province event
        static NewsMessage CreateProvinceEvent(
            game::types::EntityID province,
            const map::Vector2& province_position,
            const std::string& event_description,
            NewsPriority priority,
            float event_time
        );

        // Emergency alert
        static NewsMessage CreateEmergencyAlert(
            const std::string& alert_title,
            const std::string& alert_content,
            const map::Vector2& location,
            game::types::EntityID related_entity,
            float event_time
        );

        // Technology breakthrough
        static NewsMessage CreateTechnologyNews(
            const std::string& technology_name,
            const std::string& description,
            float event_time
        );

        // Character event
        static NewsMessage CreateCharacterNews(
            game::types::EntityID character,
            const std::string& character_name,
            const std::string& event_description,
            NewsPriority priority,
            float event_time
        );

        // Naval battle report
        static NewsMessage CreateNavalBattleReport(
            const std::string& battle_name,
            const map::Vector2& battle_location,
            game::types::EntityID attacker_fleet,
            game::types::EntityID defender_fleet,
            bool victory,
            int ships_lost,
            float event_time
        );
    };

} // namespace game::news
