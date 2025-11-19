// ============================================================================
// PopulationAggregator.h - Centralized Population Statistics Calculator
// Created: October 12, 2025 - Extensive Refactoring
// Location: include/game/population/PopulationAggregator.h
// Purpose: Single-responsibility class for calculating population aggregates
// ============================================================================

#pragma once

#include "game/population/PopulationTypes.h"

// Forward declaration
namespace game::population {
    struct PopulationComponent;
}
#include <chrono>

namespace game::population {

    /**
     * @brief High-performance population statistics aggregator
     * 
     * Centralizes all population aggregate calculations to eliminate duplication
     * between PopulationSystem and PopulationFactory. Uses modern C++ patterns
     * for optimal performance and memory efficiency.
     */
    class PopulationAggregator {
    public:
        /**
         * @brief Calculate all aggregate statistics for a population component
         * @param population The population component to update
         * @param update_timestamp Whether to update the last_update timestamp
         */
        static void RecalculateAllAggregates(PopulationComponent& population, bool update_timestamp = true);

        /**
         * @brief Calculate only basic demographic totals (optimized for frequent updates)
         * @param population The population component to update
         */
        static void RecalculateBasicTotals(PopulationComponent& population);

        /**
         * @brief Calculate weighted averages (optimized for performance-critical updates)
         * @param population The population component to update
         */
        static void RecalculateWeightedAverages(PopulationComponent& population);

        /**
         * @brief Calculate employment and economic statistics
         * @param population The population component to update
         */
        static void RecalculateEconomicData(PopulationComponent& population);

        /**
         * @brief Calculate military and recruitment statistics
         * @param population The population component to update
         */
        static void RecalculateMilitaryData(PopulationComponent& population);

        /**
         * @brief Validate data consistency and log warnings for anomalies
         * @param population The population component to validate
         * @return true if data is consistent, false if anomalies detected
         */
        static bool ValidateDataConsistency(const PopulationComponent& population);

    private:
        // Performance optimization structures
        struct AggregationContext {
            int total_population = 0;
            double happiness_sum = 0.0;
            double literacy_sum = 0.0;
            double wealth_sum = 0.0;
            double health_sum = 0.0;
            double birth_rate_sum = 0.0;
            double death_rate_sum = 0.0;
            double military_quality_sum = 0.0;
            int productive_workers = 0;
            int unemployed_seeking = 0;
            int military_eligible = 0;

            // Demographics
            int total_children = 0;
            int total_adults = 0;
            int total_elderly = 0;
            int total_males = 0;
            int total_females = 0;

            // Distributions
            std::unordered_map<std::string, int> culture_distribution;
            std::unordered_map<std::string, int> religion_distribution;
            std::unordered_map<SocialClass, int> class_distribution;
            std::unordered_map<LegalStatus, int> legal_status_distribution;
            std::unordered_map<EmploymentType, int> total_employment;
        };

        static void ResetAggregates(PopulationComponent& population);
        static AggregationContext ProcessPopulationGroups(const PopulationComponent& population);
        static void ApplyAggregationResults(PopulationComponent& population, const AggregationContext& context);
    };

} // namespace game::population