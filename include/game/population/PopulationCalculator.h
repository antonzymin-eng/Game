// ============================================================================
// Mechanica Imperii - Population Calculator Header
// Pure Calculation Functions for Population System
// ============================================================================

#pragma once

#include "game/population/PopulationTypes.h"
#include <unordered_map>
#include <algorithm>

namespace game::population {

    /**
     * @brief Pure calculation functions for population demographics
     * All functions are static with no side effects
     */
    class PopulationCalculator {
    public:
        // ====================================================================
        // Historical Demographics
        // ====================================================================

        /**
         * @brief Get historical percentage for a social class based on year and prosperity
         */
        static double GetHistoricalPercentage(SocialClass social_class, int year, double prosperity_level);

        /**
         * @brief Calculate urbanization rate based on population and prosperity
         */
        static double CalculateUrbanizationRate(int total_population, double prosperity_level, int year);

        /**
         * @brief Calculate free peasant percentage based on historical period
         */
        static double CalculateFreePeasantPercentage(int year, double prosperity_level);

        /**
         * @brief Calculate villein percentage based on historical period
         */
        static double CalculateVilleinPercentage(int year, double prosperity_level);

        // ====================================================================
        // Wealth Calculations
        // ====================================================================

        /**
         * @brief Get base wealth for a social class
         */
        static double GetClassBaseWealth(SocialClass social_class, double prosperity_level);

        /**
         * @brief Calculate total wealth for a population group
         */
        static double CalculateGroupWealth(int population_count, double wealth_per_capita);

        // ====================================================================
        // Literacy and Education
        // ====================================================================

        /**
         * @brief Get literacy rate for a social class based on historical period
         */
        static double GetClassLiteracyRate(SocialClass social_class, int year);

        /**
         * @brief Calculate education access based on class and prosperity
         */
        static double CalculateEducationAccess(SocialClass social_class, double prosperity_level);

        // ====================================================================
        // Demographics (Age/Gender Distribution)
        // ====================================================================

        /**
         * @brief Calculate medieval age distribution
         * @return Tuple of (children_0_14, adults_15_64, elderly_65_plus)
         */
        static std::tuple<int, int, int> CalculateAgeDistribution(int total_population);

        /**
         * @brief Calculate gender distribution
         * @return Pair of (males, females)
         */
        static std::pair<int, int> CalculateGenderDistribution(int total_population);

        // ====================================================================
        // Health and Happiness
        // ====================================================================

        /**
         * @brief Get base happiness for a social class
         */
        static double GetClassBaseHappiness(SocialClass social_class, double prosperity_level);

        /**
         * @brief Get health level for a social class
         */
        static double GetClassHealthLevel(SocialClass social_class, double prosperity_level);

        // ====================================================================
        // Military Calculations
        // ====================================================================

        /**
         * @brief Calculate military eligible population
         */
        static int CalculateMilitaryEligible(int adult_males, SocialClass social_class);

        /**
         * @brief Calculate military quality based on class and prosperity
         */
        static double CalculateMilitaryQuality(SocialClass social_class, double prosperity_level);

        // ====================================================================
        // Settlement Calculations
        // ====================================================================

        /**
         * @brief Calculate number of settlements based on population
         */
        static int CalculateSettlementCount(int population, int avg_settlement_size);

        /**
         * @brief Get settlement infrastructure level
         */
        static double GetSettlementInfrastructure(SettlementType type, double prosperity_level);

        /**
         * @brief Get settlement fortification level
         */
        static double GetSettlementFortification(SettlementType type, double prosperity_level);

        /**
         * @brief Get settlement autonomy level
         */
        static double GetSettlementAutonomy(SettlementType type);

        /**
         * @brief Calculate disease risk for settlement
         */
        static double GetSettlementDiseaseRisk(SettlementType type, double prosperity_level);

        // ====================================================================
        // Utility Functions
        // ====================================================================

        /**
         * @brief Check if social class is wealthy
         */
        static bool IsWealthyClass(SocialClass social_class);

        /**
         * @brief Clamp value to range
         */
        static double Clamp(double value, double min_val, double max_val);

    private:
        // Historical data constants
        static constexpr double MEDIEVAL_URBANIZATION_BASE = 0.08; // 8% base urbanization
        static constexpr double MEDIEVAL_CHILDREN_PERCENTAGE = 0.35;
        static constexpr double MEDIEVAL_ADULT_PERCENTAGE = 0.55;
        static constexpr double MEDIEVAL_MALE_PERCENTAGE = 0.48;
    };

} // namespace game::population
