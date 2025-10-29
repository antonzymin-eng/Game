// ============================================================================
// Mechanica Imperii - Gameplay Calculator Implementation
// ============================================================================

#include "game/gameplay/GameplayCalculator.h"
#include <cmath>

namespace game::gameplay {

    double GameplayCalculator::CalculateBaseDecisionQuality(double system_performance) {
        double base_quality = 0.6;
        double performance_bonus = CalculatePerformanceBonus(system_performance);
        return base_quality + performance_bonus;
    }

    double GameplayCalculator::ApplyUrgencyPenalty(double base_quality, bool is_urgent) {
        return is_urgent ? base_quality - 0.1 : base_quality;
    }

    double GameplayCalculator::ApplyImportanceBonus(double base_quality, double importance_weight) {
        double bonus = (importance_weight - 1.0) * 0.1;
        return base_quality + bonus;
    }

    double GameplayCalculator::ClampQuality(double quality) {
        return Clamp(quality, 0.0, 1.0);
    }

    double GameplayCalculator::CalculateEscalationFactor(
        double system_performance,
        bool is_urgent,
        double importance_weight,
        DecisionScope scope,
        double performance_threshold) {

        double base_factor = 1.0;

        // Performance penalty
        if (system_performance < performance_threshold) {
            base_factor += (performance_threshold - system_performance) * 2.0;
        }

        // Urgency amplification
        if (is_urgent) {
            base_factor *= 1.3;
        }

        // Importance amplification
        base_factor *= importance_weight;

        // Scope amplification
        switch (scope) {
        case DecisionScope::REGIONAL: base_factor *= 1.2; break;
        case DecisionScope::NATIONAL: base_factor *= 1.5; break;
        case DecisionScope::HISTORIC: base_factor *= 2.0; break;
        default: break;
        }

        return Clamp(base_factor, 1.0, 5.0);
    }

    bool GameplayCalculator::ShouldEscalate(
        ConsequenceSeverity severity,
        double system_performance,
        bool is_urgent,
        double importance_weight,
        DecisionScope scope,
        double performance_threshold) {

        bool performance_based = system_performance < performance_threshold;
        bool urgency_based = is_urgent && severity >= ConsequenceSeverity::MODERATE;
        bool importance_based = importance_weight > 1.5 && severity >= ConsequenceSeverity::MAJOR;
        bool scope_based = scope >= DecisionScope::NATIONAL && severity >= ConsequenceSeverity::MAJOR;

        return performance_based || urgency_based || importance_based || scope_based;
    }

    ConsequenceSeverity GameplayCalculator::DetermineSeverity(double quality) {
        if (quality >= 0.8) return ConsequenceSeverity::MINOR;
        if (quality >= 0.5) return ConsequenceSeverity::MODERATE;
        if (quality >= 0.3) return ConsequenceSeverity::MAJOR;
        return ConsequenceSeverity::CRITICAL;
    }

    ConsequenceSeverity GameplayCalculator::EscalateSeverity(ConsequenceSeverity current) {
        switch (current) {
        case ConsequenceSeverity::MINOR: return ConsequenceSeverity::MODERATE;
        case ConsequenceSeverity::MODERATE: return ConsequenceSeverity::MAJOR;
        case ConsequenceSeverity::MAJOR: return ConsequenceSeverity::CRITICAL;
        case ConsequenceSeverity::CRITICAL: return ConsequenceSeverity::CRITICAL;
        default: return current;
        }
    }

    double GameplayCalculator::CalculateWeightedAverage(double current, double new_value, double learning_rate) {
        return (1.0 - learning_rate) * current + learning_rate * new_value;
    }

    double GameplayCalculator::CalculatePerformanceBonus(double system_performance) {
        return (system_performance - 0.5) * 0.4; // ±0.2 based on performance
    }

    double GameplayCalculator::Clamp(double value, double min_val, double max_val) {
        return std::max(min_val, std::min(value, max_val));
    }

} // namespace game::gameplay
