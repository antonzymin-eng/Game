// ============================================================================
// NavalCombatLogger.cpp - Logging and Debugging Implementation
// Created: 2025-11-18 - Naval Combat Logging System
// Location: src/game/military/NavalCombatLogger.cpp
// ============================================================================

#include "game/military/NavalCombatLogger.h"
#include "game/military/NavalOperationsSystem.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>

namespace game::military {

    // Static member initialization
    std::ofstream NavalCombatLogger::log_file_;
    bool NavalCombatLogger::is_initialized_ = false;
    bool NavalCombatLogger::is_enabled_ = false;
    NavalPerformanceMetrics NavalCombatLogger::metrics_;

    std::string NavalPerformanceMetrics::ToString() const {
        std::ostringstream oss;
        oss << "=== Naval Combat Performance Metrics ===\n";
        oss << "Battles:\n";
        oss << "  Total: " << total_battles << "\n";
        oss << "  Total Time: " << total_combat_time_ms << "ms\n";
        oss << "  Average Time: " << avg_combat_time_ms << "ms\n\n";

        oss << "Pathfinding:\n";
        oss << "  Total Calls: " << total_pathfinding_calls << "\n";
        oss << "  Total Time: " << total_pathfinding_time_ms << "ms\n";
        oss << "  Average Time: " << avg_pathfinding_time_ms << "ms\n";
        oss << "  Cache Hits: " << pathfinding_cache_hits << "\n";
        oss << "  Cache Misses: " << pathfinding_cache_misses << "\n";

        if (pathfinding_cache_hits + pathfinding_cache_misses > 0) {
            double hit_rate = 100.0 * pathfinding_cache_hits /
                              (pathfinding_cache_hits + pathfinding_cache_misses);
            oss << "  Cache Hit Rate: " << std::fixed << std::setprecision(1) << hit_rate << "%\n";
        }

        oss << "\n";
        oss << "Fleet Composition:\n";
        oss << "  Cache Hits: " << composition_cache_hits << "\n";
        oss << "  Cache Misses: " << composition_cache_misses << "\n";

        if (composition_cache_hits + composition_cache_misses > 0) {
            double hit_rate = 100.0 * composition_cache_hits /
                              (composition_cache_hits + composition_cache_misses);
            oss << "  Cache Hit Rate: " << std::fixed << std::setprecision(1) << hit_rate << "%\n";
        }

        return oss.str();
    }

    void NavalCombatLogger::Initialize(const std::string& log_file) {
        if (is_initialized_) {
            return;
        }

        log_file_.open(log_file, std::ios::out | std::ios::app);
        if (log_file_.is_open()) {
            is_initialized_ = true;
            is_enabled_ = true;

            log_file_ << "\n" << std::string(80, '=') << "\n";
            log_file_ << "Naval Combat Logger Initialized - " << GetTimestamp() << "\n";
            log_file_ << std::string(80, '=') << "\n\n";
            log_file_.flush();
        }
    }

    void NavalCombatLogger::Shutdown() {
        if (is_initialized_ && log_file_.is_open()) {
            log_file_ << "\n" << std::string(80, '=') << "\n";
            log_file_ << "Naval Combat Logger Shutdown - " << GetTimestamp() << "\n";
            log_file_ << std::string(80, '=') << "\n\n";
            log_file_.close();
            is_initialized_ = false;
        }
    }

    void NavalCombatLogger::Log(LogLevel level, const std::string& message) {
        if (!is_enabled_ || !is_initialized_) {
            return;
        }

        std::ostringstream oss;
        oss << "[" << GetTimestamp() << "] ";
        oss << "[" << LogLevelToString(level) << "] ";
        oss << message << "\n";

        std::string log_entry = oss.str();

        if (log_file_.is_open()) {
            log_file_ << log_entry;
            log_file_.flush();
        }

        // Also output to console for important messages
        if (level == LogLevel::WARNING || level == LogLevel::ERROR) {
            std::cerr << log_entry;
        }
    }

    void NavalCombatLogger::LogBattle(
        const NavalBattleResult& result,
        const ArmyComponent& attacker_fleet,
        const ArmyComponent& defender_fleet,
        const std::string& location
    ) {
        if (!is_enabled_) return;

        std::ostringstream oss;
        oss << "=== NAVAL BATTLE ===\n";
        oss << "Location: " << location << "\n";
        oss << "Attacker: " << attacker_fleet.army_name << " (" << attacker_fleet.units.size() << " ships)\n";
        oss << "Defender: " << defender_fleet.army_name << " (" << defender_fleet.units.size() << " ships)\n";
        oss << "Outcome: " << BattleResolutionCalculator::OutcomeToString(result.outcome) << "\n";
        oss << "Casualties: A=" << result.attacker_casualties << " D=" << result.defender_casualties << "\n";
        oss << "Ships Sunk: A=" << result.ships_sunk_attacker << " D=" << result.ships_sunk_defender << "\n";
        oss << "Ships Captured: " << result.ships_captured_by_attacker << "\n";
        oss << "Broadsides: " << result.casualties_from_broadsides << "\n";
        oss << "Boarding: " << result.casualties_from_boarding << "\n";
        oss << "Ramming: " << result.casualties_from_ramming << "\n";
        oss << "Fire: " << result.casualties_from_fire << "\n";
        oss << "Naval Tradition: +" << result.naval_tradition_gained << "\n";
        oss << "==================\n";

        Log(LogLevel::INFO, oss.str());
    }

    void NavalCombatLogger::LogBlockade(
        const BlockadeStatus& blockade,
        const ArmyComponent& fleet,
        const std::string& target_name
    ) {
        if (!is_enabled_) return;

        std::ostringstream oss;
        oss << "=== NAVAL BLOCKADE ===\n";
        oss << "Fleet: " << fleet.army_name << " (" << fleet.units.size() << " ships)\n";
        oss << "Target: " << target_name << "\n";
        oss << "Effectiveness: " << NavalOperationsSystem::BlockadeEffectivenessToString(blockade.effectiveness) << "\n";
        oss << "Trade Disruption: " << (blockade.trade_disruption_percent * 100) << "%\n";
        oss << "Days Active: " << blockade.days_active << "\n";
        oss << "Ships Intercepted: " << blockade.ships_intercepted << "\n";
        oss << "=====================\n";

        Log(LogLevel::INFO, oss.str());
    }

    void NavalCombatLogger::LogPathfinding(
        const std::vector<game::types::EntityID>& path,
        uint32_t start_province,
        uint32_t goal_province,
        uint64_t computation_time_ms
    ) {
        if (!is_enabled_) return;

        std::ostringstream oss;
        oss << "Pathfinding: " << start_province << " -> " << goal_province;
        oss << " | Path Length: " << path.size();
        oss << " | Time: " << computation_time_ms << "ms";

        Log(LogLevel::DEBUG, oss.str());
    }

    void NavalCombatLogger::LogFleetComposition(
        const ArmyComponent& fleet,
        const FleetComposition& composition
    ) {
        if (!is_enabled_) return;

        std::ostringstream oss;
        oss << "Fleet Composition - " << fleet.army_name << ":\n";
        oss << "  Total Ships: " << composition.total_ships << "\n";
        oss << "  Ships of Line: " << composition.ships_of_the_line << "\n";
        oss << "  Frigates: " << composition.frigates << "\n";
        oss << "  Corvettes: " << composition.corvettes << "\n";
        oss << "  Light Ships: " << composition.light_ships << "\n";
        oss << "  Galleys: " << composition.galleys << "\n";
        oss << "  Total Firepower: " << composition.total_firepower;

        Log(LogLevel::DEBUG, oss.str());
    }

    void NavalCombatLogger::LogPerformanceMetrics(const NavalPerformanceMetrics& metrics) {
        if (!is_enabled_) return;

        Log(LogLevel::INFO, metrics.ToString());
    }

    const NavalPerformanceMetrics& NavalCombatLogger::GetMetrics() {
        return metrics_;
    }

    void NavalCombatLogger::RecordBattleTime(uint64_t time_ms) {
        metrics_.total_battles++;
        metrics_.total_combat_time_ms += time_ms;
        metrics_.avg_combat_time_ms = static_cast<double>(metrics_.total_combat_time_ms) / metrics_.total_battles;
    }

    void NavalCombatLogger::RecordPathfindingTime(uint64_t time_ms) {
        metrics_.total_pathfinding_calls++;
        metrics_.total_pathfinding_time_ms += time_ms;
        metrics_.avg_pathfinding_time_ms = static_cast<double>(metrics_.total_pathfinding_time_ms) /
                                            metrics_.total_pathfinding_calls;
    }

    void NavalCombatLogger::RecordCacheAccess(bool is_hit, bool is_pathfinding) {
        if (is_pathfinding) {
            if (is_hit) {
                metrics_.pathfinding_cache_hits++;
            } else {
                metrics_.pathfinding_cache_misses++;
            }
        } else {
            if (is_hit) {
                metrics_.composition_cache_hits++;
            } else {
                metrics_.composition_cache_misses++;
            }
        }
    }

    void NavalCombatLogger::SetEnabled(bool enabled) {
        is_enabled_ = enabled;
    }

    bool NavalCombatLogger::IsEnabled() {
        return is_enabled_;
    }

    std::string NavalCombatLogger::GetTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);

        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    std::string NavalCombatLogger::LogLevelToString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARNING: return "WARN";
            case LogLevel::ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }

} // namespace game::military
