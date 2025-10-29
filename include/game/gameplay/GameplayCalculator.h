// ============================================================================
// Mechanica Imperii - Gameplay Calculator Header
// Pure Calculation Functions for Core Gameplay
// ============================================================================

#pragma once

#include "game/gameplay/CoreGameplaySystem.h"
#include <algorithm>

namespace game::gameplay {

    /**
     * @brief Pure calculation functions for gameplay mechanics
     */
    class GameplayCalculator {
    public:
        // Decision quality calculations
        static double CalculateBaseDecisionQuality(double system_performance);
        static double ApplyUrgencyPenalty(double base_quality, bool is_urgent);
        static double ApplyImportanceBonus(double base_quality, double importance_weight);
        static double ClampQuality(double quality);

        // Escalation calculations
        static double CalculateEscalationFactor(
            double system_performance,
            bool is_urgent,
            double importance_weight,
            DecisionScope scope,
            double performance_threshold
        );

        static bool ShouldEscalate(
            ConsequenceSeverity severity,
            double system_performance,
            bool is_urgent,
            double importance_weight,
            DecisionScope scope,
            double performance_threshold
        );

        // Severity calculations
        static ConsequenceSeverity DetermineSeverity(double quality);
        static ConsequenceSeverity EscalateSeverity(ConsequenceSeverity current);

        // Performance calculations
        static double CalculateWeightedAverage(double current, double new_value, double learning_rate);
        static double CalculatePerformanceBonus(double system_performance);

        // Utility
        static double Clamp(double value, double min_val, double max_val);
    };

} // namespace game::gameplay
