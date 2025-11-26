// ============================================================================
// CommandDelay.cpp - Command Delay System Implementation
// Created: November 18, 2025
// ============================================================================

#include "game/military/CommandDelay.h"
#include "core/logging/Logger.h"
#include <cmath>
#include <algorithm>

namespace game::military {

    // ========================================================================
    // CommandDelayCalculator Implementation
    // ========================================================================

    CommandDelayCalculator::CommandDelayCalculator() {
        CORE_STREAM_INFO("CommandDelay") << "CommandDelayCalculator initialized";
    }

    float CommandDelayCalculator::CalculateDelay(
        const map::Vector2& from_position,
        const map::Vector2& to_position,
        CommunicationType comm_type,
        const map::TerrainGrid* terrain,
        float weather_modifier
    ) const {
        // Calculate distance in world units (assuming 1 unit = 1 km)
        float distance_km = GetDistance(from_position, to_position);

        // Get base speed and minimum delay
        float speed_kmh = GetCommunicationSpeed(comm_type);
        float min_delay = GetMinimumDelay(comm_type);

        // Calculate terrain modifier if terrain data available
        float terrain_modifier = 1.0f;
        if (terrain) {
            terrain_modifier = CalculateTerrainModifier(from_position, to_position, terrain);
        }

        // Calculate delay in hours
        // delay = distance / speed * terrain_modifier * weather_modifier * global_multiplier
        float delay = (distance_km / speed_kmh) * terrain_modifier * weather_modifier * delay_multiplier_;

        // Apply minimum delay
        delay = std::max(delay, min_delay);

        return delay;
    }

    float CommandDelayCalculator::CalculateDelayBetweenProvinces(
        game::types::EntityID from_province,
        game::types::EntityID to_province,
        CommunicationType comm_type,
        float weather_modifier
    ) const {
        // Placeholder: In a real implementation, would get province positions
        // from a province manager or similar system
        // For now, just use minimum delay
        return GetMinimumDelay(comm_type) * weather_modifier * delay_multiplier_;
    }

    float CommandDelayCalculator::CalculateTerrainModifier(
        const map::Vector2& from,
        const map::Vector2& to,
        const map::TerrainGrid* terrain
    ) const {
        if (!terrain) return 1.0f;

        // Sample terrain along the path
        int num_samples = 10;
        float total_modifier = 0.0f;
        int valid_samples = 0;

        for (int i = 0; i <= num_samples; ++i) {
            float t = static_cast<float>(i) / num_samples;
            float x = from.x + (to.x - from.x) * t;
            float y = from.y + (to.y - from.y) * t;

            const map::TerrainCell* cell = terrain->GetCellAtPosition(x, y);
            if (cell) {
                float modifier = 1.0f;

                switch (cell->type) {
                    case map::TerrainCellType::PLAINS:
                        modifier = TerrainDelayModifier::PLAINS_MODIFIER;
                        break;
                    case map::TerrainCellType::FOREST:
                        modifier = TerrainDelayModifier::FOREST_MODIFIER;
                        break;
                    case map::TerrainCellType::HILLS:
                        modifier = TerrainDelayModifier::HILLS_MODIFIER;
                        break;
                    case map::TerrainCellType::MOUNTAIN:
                        modifier = TerrainDelayModifier::MOUNTAIN_MODIFIER;
                        break;
                    case map::TerrainCellType::MARSH:
                        modifier = TerrainDelayModifier::MARSH_MODIFIER;
                        break;
                    case map::TerrainCellType::DESERT:
                        modifier = TerrainDelayModifier::DESERT_MODIFIER;
                        break;
                    case map::TerrainCellType::WATER:
                        modifier = TerrainDelayModifier::WATER_MODIFIER;
                        break;
                    case map::TerrainCellType::SNOW:
                        modifier = TerrainDelayModifier::SNOW_MODIFIER;
                        break;
                    default:
                        modifier = 1.0f;
                        break;
                }

                total_modifier += modifier;
                valid_samples++;
            }
        }

        return valid_samples > 0 ? total_modifier / valid_samples : 1.0f;
    }

    // ========================================================================
    // CommandDelaySystem Implementation
    // ========================================================================

    CommandDelaySystem::CommandDelaySystem() {
        CORE_STREAM_INFO("CommandDelay") << "CommandDelaySystem initialized";
    }

    void CommandDelaySystem::SendCommand(
        game::types::EntityID target_army,
        game::types::EntityID order_id,
        const map::Vector2& from,
        const map::Vector2& to,
        CommunicationType comm_type,
        float current_game_time,
        const std::string& description
    ) {
        PendingCommand command;
        command.target_army = target_army;
        command.order_id = order_id;
        command.send_time = current_game_time;
        command.comm_type = comm_type;
        command.origin_position = from;
        command.destination_position = to;
        command.order_description = description;

        // Calculate delay
        command.total_delay = calculator_.CalculateDelay(from, to, comm_type, nullptr, 1.0f);
        command.arrival_time = current_game_time + command.total_delay;

        pending_commands_.push_back(command);

        CORE_STREAM_INFO("CommandDelay") << "Sent command to army " << target_army
                                         << " (delay: " << command.total_delay << " hours)";
    }

    void CommandDelaySystem::Update(float current_game_time) {
        // Check for arrived commands
        std::vector<PendingCommand> arrived_commands;

        auto it = pending_commands_.begin();
        while (it != pending_commands_.end()) {
            if (it->HasArrived(current_game_time)) {
                arrived_commands.push_back(*it);
                it = pending_commands_.erase(it);
            } else {
                ++it;
            }
        }

        // Process arrived commands
        for (const auto& command : arrived_commands) {
            OnCommandArrived(command);
        }
    }

    std::vector<PendingCommand*> CommandDelaySystem::GetPendingCommandsForArmy(
        game::types::EntityID army_id
    ) {
        std::vector<PendingCommand*> result;
        for (auto& command : pending_commands_) {
            if (command.target_army == army_id) {
                result.push_back(&command);
            }
        }
        return result;
    }

    void CommandDelaySystem::CancelPendingCommandsForArmy(game::types::EntityID army_id) {
        pending_commands_.erase(
            std::remove_if(pending_commands_.begin(), pending_commands_.end(),
                [army_id](const PendingCommand& cmd) {
                    return cmd.target_army == army_id;
                }),
            pending_commands_.end()
        );
    }

    int CommandDelaySystem::GetPendingCountForArmy(game::types::EntityID army_id) const {
        int count = 0;
        for (const auto& command : pending_commands_) {
            if (command.target_army == army_id) {
                count++;
            }
        }
        return count;
    }

    void CommandDelaySystem::OnCommandArrived(const PendingCommand& command) {
        CORE_STREAM_INFO("CommandDelay") << "Command arrived for army " << command.target_army
                                         << ": " << command.order_description;

        // In a real implementation, this would trigger order execution
        // in the military system or notify the appropriate components
    }

} // namespace game::military
