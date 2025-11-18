// ============================================================================
// CommandDelay.h - Command Delay and Communication System
// Created: November 18, 2025
// Description: Realistic command propagation delays based on distance,
//              terrain, technology, and infrastructure
// ============================================================================

#pragma once

#include "utils/PlatformMacros.h"
#include "core/types/game_types.h"
#include "map/ProvinceRenderComponent.h"
#include "map/TerrainData.h"
#include <vector>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <string>

namespace game::military {

    // ========================================================================
    // CommunicationType - Methods of command transmission
    // ========================================================================
    enum class CommunicationType {
        MESSENGER = 0,      // Mounted messenger (realistic for medieval/early modern)
        COURIER,            // Fast courier service
        SIGNAL_FIRE,        // Signal fires (line of sight)
        SEMAPHORE,          // Semaphore system (requires infrastructure)
        TELEGRAPH,          // Telegraph (industrial era)
        RADIO,              // Radio communication (modern)
        INSTANT,            // Instant (for testing/fantasy)
        COUNT
    };

    // ========================================================================
    // CommunicationSpeed - Base speeds for different communication types
    // ========================================================================
    struct CommunicationSpeed {
        // Speed in km/h (kilometers per hour of game time)
        static constexpr float MESSENGER_SPEED = 15.0f;         // Mounted messenger
        static constexpr float COURIER_SPEED = 25.0f;           // Fast courier
        static constexpr float SIGNAL_FIRE_SPEED = 1000.0f;     // Near-instant (line of sight)
        static constexpr float SEMAPHORE_SPEED = 500.0f;        // Very fast (infrastructure)
        static constexpr float TELEGRAPH_SPEED = 10000.0f;      // Near-instant
        static constexpr float RADIO_SPEED = 100000.0f;         // Effectively instant

        // Minimum delays (in hours) regardless of distance
        static constexpr float MESSENGER_MIN_DELAY = 0.5f;      // 30 minutes
        static constexpr float COURIER_MIN_DELAY = 0.25f;       // 15 minutes
        static constexpr float SIGNAL_FIRE_MIN_DELAY = 0.05f;   // 3 minutes
        static constexpr float SEMAPHORE_MIN_DELAY = 0.05f;     // 3 minutes
        static constexpr float TELEGRAPH_MIN_DELAY = 0.01f;     // <1 minute
        static constexpr float RADIO_MIN_DELAY = 0.001f;        // Seconds
    };

    // ========================================================================
    // TerrainDelayModifier - Terrain affects communication speed
    // ========================================================================
    struct TerrainDelayModifier {
        // Delay multipliers for different terrain types
        static constexpr float PLAINS_MODIFIER = 1.0f;          // Normal speed
        static constexpr float FOREST_MODIFIER = 1.3f;          // 30% slower
        static constexpr float HILLS_MODIFIER = 1.2f;           // 20% slower
        static constexpr float MOUNTAIN_MODIFIER = 1.8f;        // 80% slower
        static constexpr float MARSH_MODIFIER = 1.5f;           // 50% slower
        static constexpr float DESERT_MODIFIER = 1.1f;          // 10% slower
        static constexpr float WATER_MODIFIER = 2.0f;           // Sea crossing
        static constexpr float SNOW_MODIFIER = 1.4f;            // 40% slower

        // Weather modifiers
        static constexpr float CLEAR_WEATHER = 1.0f;
        static constexpr float RAIN_MODIFIER = 1.2f;
        static constexpr float STORM_MODIFIER = 1.5f;
        static constexpr float SNOW_WEATHER_MODIFIER = 1.6f;
        static constexpr float FOG_MODIFIER = 1.3f;
    };

    // ========================================================================
    // CommandDelayCalculator - Calculates command propagation delays
    // ========================================================================
    class CommandDelayCalculator {
    public:
        CommandDelayCalculator();
        ~CommandDelayCalculator() = default;

        // Calculate delay for command to reach destination
        float CalculateDelay(
            const map::Vector2& from_position,
            const map::Vector2& to_position,
            CommunicationType comm_type,
            const map::TerrainGrid* terrain = nullptr,
            float weather_modifier = 1.0f
        ) const;

        // Calculate delay based on province-to-province
        float CalculateDelayBetweenProvinces(
            game::types::EntityID from_province,
            game::types::EntityID to_province,
            CommunicationType comm_type,
            float weather_modifier = 1.0f
        ) const;

        // Get average terrain modifier along path
        float CalculateTerrainModifier(
            const map::Vector2& from,
            const map::Vector2& to,
            const map::TerrainGrid* terrain
        ) const;

        // Get communication speed for type
        float GetCommunicationSpeed(CommunicationType type) const;

        // Get minimum delay for type
        float GetMinimumDelay(CommunicationType type) const;

        // Settings
        void SetDefaultCommunicationType(CommunicationType type) {
            default_comm_type_ = type;
        }

        void SetDelayMultiplier(float multiplier) {
            delay_multiplier_ = multiplier;
        }

        CommunicationType GetDefaultCommunicationType() const {
            return default_comm_type_;
        }

        float GetDelayMultiplier() const {
            return delay_multiplier_;
        }

    private:
        CommunicationType default_comm_type_ = CommunicationType::MESSENGER;
        float delay_multiplier_ = 1.0f;  // Global delay multiplier (difficulty setting)

        // Helper to get distance between points
        float GetDistance(const map::Vector2& from, const map::Vector2& to) const;
    };

    // ========================================================================
    // PendingCommand - Command in transit (delayed)
    // ========================================================================
    struct PendingCommand {
        game::types::EntityID target_army = 0;         // Army that will receive command
        game::types::EntityID order_id = 0;            // Order identifier
        float send_time = 0.0f;                        // When command was sent
        float arrival_time = 0.0f;                     // When command will arrive
        float total_delay = 0.0f;                      // Total delay in hours
        CommunicationType comm_type = CommunicationType::MESSENGER;

        map::Vector2 origin_position;                  // Where command originated
        map::Vector2 destination_position;             // Where command is going

        std::string order_description;                 // Description of the order

        PendingCommand() = default;

        // Check if command has arrived
        bool HasArrived(float current_game_time) const {
            return current_game_time >= arrival_time;
        }

        // Get progress (0.0 to 1.0)
        float GetProgress(float current_game_time) const {
            if (total_delay <= 0.0f) return 1.0f;
            float elapsed = current_game_time - send_time;
            return std::min(1.0f, elapsed / total_delay);
        }

        // Get remaining time
        float GetRemainingTime(float current_game_time) const {
            return std::max(0.0f, arrival_time - current_game_time);
        }
    };

    // ========================================================================
    // CommandDelaySystem - Manages pending commands and delays
    // ========================================================================
    class CommandDelaySystem {
    public:
        CommandDelaySystem();
        ~CommandDelaySystem() = default;

        // Send command with delay
        void SendCommand(
            game::types::EntityID target_army,
            game::types::EntityID order_id,
            const map::Vector2& from,
            const map::Vector2& to,
            CommunicationType comm_type,
            float current_game_time,
            const std::string& description = ""
        );

        // Update pending commands (check for arrivals)
        void Update(float current_game_time);

        // Get pending commands for an army
        std::vector<PendingCommand*> GetPendingCommandsForArmy(game::types::EntityID army_id);

        // Get all pending commands
        const std::vector<PendingCommand>& GetAllPendingCommands() const {
            return pending_commands_;
        }

        // Cancel pending commands for an army
        void CancelPendingCommandsForArmy(game::types::EntityID army_id);

        // Clear all pending commands
        void ClearAll() { pending_commands_.clear(); }

        // Get calculator
        CommandDelayCalculator& GetCalculator() { return calculator_; }
        const CommandDelayCalculator& GetCalculator() const { return calculator_; }

        // Statistics
        int GetPendingCommandCount() const {
            return static_cast<int>(pending_commands_.size());
        }

        int GetPendingCountForArmy(game::types::EntityID army_id) const;

    private:
        CommandDelayCalculator calculator_;
        std::vector<PendingCommand> pending_commands_;

        // Callbacks for when commands arrive (can be extended)
        void OnCommandArrived(const PendingCommand& command);
    };

    // ========================================================================
    // CommandDelayComponent - ECS Component for tracking command delays
    // ========================================================================
    struct CommandDelayComponent : public game::core::Component<CommandDelayComponent> {
        // Current communication technology available
        CommunicationType available_comm_type = CommunicationType::MESSENGER;

        // Infrastructure bonuses
        float road_network_bonus = 0.0f;           // Roads speed up messengers
        float signal_network_bonus = 0.0f;         // Signal fire/semaphore network
        float communication_infrastructure = 0.0f; // General infrastructure level

        // Command center location (HQ)
        game::types::EntityID command_center = 0;
        map::Vector2 command_position;

        // Recent command delays (for UI display)
        std::vector<PendingCommand> recent_commands;
        size_t max_recent_commands = 20;

        CommandDelayComponent() = default;

        // Calculate effective communication type (with infrastructure)
        CommunicationType GetEffectiveCommunicationType() const {
            // Infrastructure can upgrade communication type
            if (signal_network_bonus > 0.8f && available_comm_type == CommunicationType::MESSENGER) {
                return CommunicationType::SIGNAL_FIRE;
            }
            return available_comm_type;
        }

        // Get speed bonus from infrastructure
        float GetSpeedBonus() const {
            return 1.0f + (road_network_bonus * 0.3f) + (communication_infrastructure * 0.2f);
        }

        std::string GetComponentTypeName() const override {
            return "CommandDelayComponent";
        }
    };

    // ========================================================================
    // Inline implementations
    // ========================================================================

    inline float CommandDelayCalculator::GetDistance(
        const map::Vector2& from,
        const map::Vector2& to
    ) const {
        float dx = to.x - from.x;
        float dy = to.y - from.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    inline float CommandDelayCalculator::GetCommunicationSpeed(CommunicationType type) const {
        switch (type) {
            case CommunicationType::MESSENGER:
                return CommunicationSpeed::MESSENGER_SPEED;
            case CommunicationType::COURIER:
                return CommunicationSpeed::COURIER_SPEED;
            case CommunicationType::SIGNAL_FIRE:
                return CommunicationSpeed::SIGNAL_FIRE_SPEED;
            case CommunicationType::SEMAPHORE:
                return CommunicationSpeed::SEMAPHORE_SPEED;
            case CommunicationType::TELEGRAPH:
                return CommunicationSpeed::TELEGRAPH_SPEED;
            case CommunicationType::RADIO:
                return CommunicationSpeed::RADIO_SPEED;
            case CommunicationType::INSTANT:
                return 1000000.0f; // Effectively instant
            default:
                return CommunicationSpeed::MESSENGER_SPEED;
        }
    }

    inline float CommandDelayCalculator::GetMinimumDelay(CommunicationType type) const {
        switch (type) {
            case CommunicationType::MESSENGER:
                return CommunicationSpeed::MESSENGER_MIN_DELAY;
            case CommunicationType::COURIER:
                return CommunicationSpeed::COURIER_MIN_DELAY;
            case CommunicationType::SIGNAL_FIRE:
                return CommunicationSpeed::SIGNAL_FIRE_MIN_DELAY;
            case CommunicationType::SEMAPHORE:
                return CommunicationSpeed::SEMAPHORE_MIN_DELAY;
            case CommunicationType::TELEGRAPH:
                return CommunicationSpeed::TELEGRAPH_MIN_DELAY;
            case CommunicationType::RADIO:
                return CommunicationSpeed::RADIO_MIN_DELAY;
            case CommunicationType::INSTANT:
                return 0.0f;
            default:
                return CommunicationSpeed::MESSENGER_MIN_DELAY;
        }
    }

} // namespace game::military
