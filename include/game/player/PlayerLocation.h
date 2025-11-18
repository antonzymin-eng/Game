// ============================================================================
// PlayerLocation.h - Player Location and Presence System
// Created: November 18, 2025
// Description: Tracks player character location for news delay calculations
//              Determines regent/council control when player is away
// ============================================================================

#pragma once

#include "utils/PlatformMacros.h"
#include "core/types/game_types.h"
#include "core/ECS/IComponent.h"
#include "map/ProvinceRenderComponent.h"
#include <cmath>

namespace game::player {

    // ========================================================================
    // PlayerLocationType - Where the player character is
    // ========================================================================
    enum class PlayerLocationType {
        IN_CAPITAL = 0,         // At the capital, running the nation
        WITH_ARMY,              // Commanding an army in the field
        WITH_NAVY,              // Commanding a navy at sea
        TRAVELING,              // Traveling between locations
        AT_PROVINCE,            // Visiting a specific province
        AT_DIPLOMATIC_MISSION,  // At foreign court for diplomacy
        IMPRISONED,             // Captured/imprisoned
        UNKNOWN,                // Location unknown
        COUNT
    };

    // ========================================================================
    // RegentType - Who manages the nation when player is away
    // ========================================================================
    enum class RegentType {
        NONE = 0,               // Player is present (in capital)
        SPOUSE,                 // Player's spouse
        HEIR,                   // Designated heir
        CHIEF_MINISTER,         // Chief minister/chancellor
        ROYAL_COUNCIL,          // Council of advisors
        MILITARY_JUNTA,         // Military council (emergency)
        FOREIGN_ADMINISTRATOR,  // Foreign occupation
        COUNT
    };

    // ========================================================================
    // PlayerLocationComponent - Tracks player position
    // ========================================================================
    struct PlayerLocationComponent : public game::core::Component<PlayerLocationComponent> {
        // Current location
        PlayerLocationType location_type = PlayerLocationType::IN_CAPITAL;
        game::types::EntityID current_province = 0;     // Province player is in
        game::types::EntityID current_army = 0;         // Army player is with
        game::types::EntityID current_navy = 0;         // Navy player is with

        map::Vector2 current_position;                  // Precise world position

        // Capital information
        game::types::EntityID capital_province = 0;
        map::Vector2 capital_position;

        // Travel information
        bool is_traveling = false;
        game::types::EntityID destination_province = 0;
        map::Vector2 destination_position;
        float travel_progress = 0.0f;                   // 0.0 to 1.0
        float travel_speed = 50.0f;                     // km per game hour

        // Distance to capital (for message delay)
        float distance_from_capital = 0.0f;

        // Last time at capital
        float last_at_capital_time = 0.0f;
        float time_away_from_capital = 0.0f;

        PlayerLocationComponent() = default;

        // Check if player is at capital
        bool IsAtCapital() const {
            return location_type == PlayerLocationType::IN_CAPITAL;
        }

        // Check if player is in the field (military/naval)
        bool IsInField() const {
            return location_type == PlayerLocationType::WITH_ARMY ||
                   location_type == PlayerLocationType::WITH_NAVY;
        }

        // Calculate distance from a position to player
        float GetDistanceFrom(const map::Vector2& position) const {
            float dx = current_position.x - position.x;
            float dy = current_position.y - position.y;
            return std::sqrt(dx * dx + dy * dy);
        }

        // Update distance from capital
        void UpdateDistanceFromCapital() {
            distance_from_capital = GetDistanceFrom(capital_position);
        }

        std::string GetComponentTypeName() const override {
            return "PlayerLocationComponent";
        }
    };

    // ========================================================================
    // RegentComponent - Regent managing nation when player is away
    // ========================================================================
    struct RegentComponent : public game::core::Component<RegentComponent> {
        RegentType regent_type = RegentType::NONE;
        game::types::EntityID regent_character = 0;     // Character serving as regent

        // Regent competencies (0.0 to 1.0)
        float administrative_skill = 0.5f;
        float diplomatic_skill = 0.5f;
        float military_skill = 0.5f;
        float economic_skill = 0.5f;

        // Regent authority and loyalty
        float authority = 0.7f;                         // How much power they have
        float loyalty = 0.8f;                           // Loyalty to player
        float popularity = 0.5f;                        // Popularity with nobility

        // Regency status
        bool is_active = false;                         // Currently acting as regent
        float time_as_regent = 0.0f;                    // Total time as regent
        float start_time = 0.0f;                        // When regency started

        // Decision making
        bool autonomous_decisions = false;              // Can make decisions without approval
        float decision_quality_modifier = 1.0f;         // Quality of decisions

        // Communication with player
        float report_frequency = 7.0f;                  // Send report every N game days
        float last_report_time = 0.0f;

        // Council members (if royal council)
        std::vector<game::types::EntityID> council_members;

        // Recent decisions made
        struct RegentDecision {
            std::string decision_type;
            std::string description;
            float game_time;
            bool successful = true;
        };
        std::vector<RegentDecision> recent_decisions;
        size_t max_decision_history = 50;

        RegentComponent() = default;

        // Check if regent can make autonomous decisions
        bool CanMakeAutonomousDecisions() const {
            return is_active && autonomous_decisions && authority > 0.5f;
        }

        // Get overall competency
        float GetOverallCompetency() const {
            return (administrative_skill + diplomatic_skill +
                    military_skill + economic_skill) / 4.0f;
        }

        // Check if time to send report
        bool ShouldSendReport(float current_time) const {
            return is_active && (current_time - last_report_time >= report_frequency);
        }

        std::string GetComponentTypeName() const override {
            return "RegentComponent";
        }
    };

    // ========================================================================
    // Helper Functions
    // ========================================================================

    inline const char* PlayerLocationTypeToString(PlayerLocationType type) {
        switch (type) {
            case PlayerLocationType::IN_CAPITAL: return "In Capital";
            case PlayerLocationType::WITH_ARMY: return "With Army";
            case PlayerLocationType::WITH_NAVY: return "With Navy";
            case PlayerLocationType::TRAVELING: return "Traveling";
            case PlayerLocationType::AT_PROVINCE: return "At Province";
            case PlayerLocationType::AT_DIPLOMATIC_MISSION: return "Diplomatic Mission";
            case PlayerLocationType::IMPRISONED: return "Imprisoned";
            case PlayerLocationType::UNKNOWN: return "Unknown";
            default: return "Unknown";
        }
    }

    inline const char* RegentTypeToString(RegentType type) {
        switch (type) {
            case RegentType::NONE: return "None";
            case RegentType::SPOUSE: return "Spouse";
            case RegentType::HEIR: return "Heir";
            case RegentType::CHIEF_MINISTER: return "Chief Minister";
            case RegentType::ROYAL_COUNCIL: return "Royal Council";
            case RegentType::MILITARY_JUNTA: return "Military Junta";
            case RegentType::FOREIGN_ADMINISTRATOR: return "Foreign Administrator";
            default: return "Unknown";
        }
    }

} // namespace game::player
