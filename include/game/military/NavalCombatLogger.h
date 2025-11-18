// ============================================================================
// NavalCombatLogger.h - Logging and Debugging for Naval Combat
// Created: 2025-11-18 - Naval Combat Logging System
// Location: include/game/military/NavalCombatLogger.h
// ============================================================================

#pragma once

#include "game/military/NavalCombatCalculator.h"
#include "game/military/FleetManagementSystem.h"
#include <string>
#include <vector>
#include <chrono>
#include <fstream>

namespace game::military {

    // Forward declarations
    struct BlockadeStatus;

    /// Log level enumeration
    enum class LogLevel {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };

    /// Performance metrics for naval operations
    struct NavalPerformanceMetrics {
        // Combat metrics
        uint32_t total_battles = 0;
        uint64_t total_combat_time_ms = 0;
        double avg_combat_time_ms = 0.0;

        // Pathfinding metrics
        uint32_t total_pathfinding_calls = 0;
        uint64_t total_pathfinding_time_ms = 0;
        double avg_pathfinding_time_ms = 0.0;
        uint32_t pathfinding_cache_hits = 0;
        uint32_t pathfinding_cache_misses = 0;

        // Fleet composition metrics
        uint32_t composition_cache_hits = 0;
        uint32_t composition_cache_misses = 0;

        void Reset() {
            total_battles = 0;
            total_combat_time_ms = 0;
            avg_combat_time_ms = 0.0;
            total_pathfinding_calls = 0;
            total_pathfinding_time_ms = 0;
            avg_pathfinding_time_ms = 0.0;
            pathfinding_cache_hits = 0;
            pathfinding_cache_misses = 0;
            composition_cache_hits = 0;
            composition_cache_misses = 0;
        }

        std::string ToString() const;
    };

    /// Naval combat logger
    class NavalCombatLogger {
    public:
        /// Initialize logger
        static void Initialize(const std::string& log_file = "naval_combat.log");

        /// Shutdown logger
        static void Shutdown();

        /// Log message
        static void Log(LogLevel level, const std::string& message);

        /// Log naval battle
        static void LogBattle(
            const NavalBattleResult& result,
            const ArmyComponent& attacker_fleet,
            const ArmyComponent& defender_fleet,
            const std::string& location
        );

        /// Log blockade operation
        static void LogBlockade(
            const BlockadeStatus& blockade,
            const ArmyComponent& fleet,
            const std::string& target_name
        );

        /// Log pathfinding operation
        static void LogPathfinding(
            const std::vector<game::types::EntityID>& path,
            uint32_t start_province,
            uint32_t goal_province,
            uint64_t computation_time_ms
        );

        /// Log fleet composition
        static void LogFleetComposition(
            const ArmyComponent& fleet,
            const FleetComposition& composition
        );

        /// Log performance metrics
        static void LogPerformanceMetrics(const NavalPerformanceMetrics& metrics);

        /// Get current performance metrics
        static const NavalPerformanceMetrics& GetMetrics();

        /// Record battle timing
        static void RecordBattleTime(uint64_t time_ms);

        /// Record pathfinding timing
        static void RecordPathfindingTime(uint64_t time_ms);

        /// Record cache hit/miss
        static void RecordCacheAccess(bool is_hit, bool is_pathfinding);

        /// Enable/disable logging
        static void SetEnabled(bool enabled);

        /// Check if logging is enabled
        static bool IsEnabled();

    private:
        static std::ofstream log_file_;
        static bool is_initialized_;
        static bool is_enabled_;
        static NavalPerformanceMetrics metrics_;

        static std::string GetTimestamp();
        static std::string LogLevelToString(LogLevel level);
    };

    /// RAII timer for measuring performance
    class NavalPerformanceTimer {
    public:
        enum class Operation {
            COMBAT,
            PATHFINDING,
            COMPOSITION_ANALYSIS
        };

        explicit NavalPerformanceTimer(Operation op)
            : operation_(op)
            , start_time_(std::chrono::high_resolution_clock::now()) {}

        ~NavalPerformanceTimer() {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                end_time - start_time_
            ).count();

            switch (operation_) {
                case Operation::COMBAT:
                    NavalCombatLogger::RecordBattleTime(duration);
                    break;
                case Operation::PATHFINDING:
                    NavalCombatLogger::RecordPathfindingTime(duration);
                    break;
                case Operation::COMPOSITION_ANALYSIS:
                    // Could add separate timing if needed
                    break;
            }
        }

    private:
        Operation operation_;
        std::chrono::high_resolution_clock::time_point start_time_;
    };

    /// Convenience macros for logging
    #define NAVAL_LOG_DEBUG(msg) NavalCombatLogger::Log(LogLevel::DEBUG, msg)
    #define NAVAL_LOG_INFO(msg) NavalCombatLogger::Log(LogLevel::INFO, msg)
    #define NAVAL_LOG_WARNING(msg) NavalCombatLogger::Log(LogLevel::WARNING, msg)
    #define NAVAL_LOG_ERROR(msg) NavalCombatLogger::Log(LogLevel::ERROR, msg)

    #define NAVAL_TIMER_COMBAT() NavalPerformanceTimer _timer(NavalPerformanceTimer::Operation::COMBAT)
    #define NAVAL_TIMER_PATHFINDING() NavalPerformanceTimer _timer(NavalPerformanceTimer::Operation::PATHFINDING)

} // namespace game::military
