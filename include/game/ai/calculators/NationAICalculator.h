// ============================================================================
// Mechanica Imperii - Nation AI Calculator Header
// Pure Calculation Functions for Nation-Level AI
// ============================================================================

#pragma once

#include "game/ai/NationAI.h"
#include <algorithm>

namespace game {
namespace ai {

    /**
     * @brief Pure calculation functions for nation-level AI decision-making
     * All functions are static with no side effects
     */
    class NationAICalculator {
    public:
        // ====================================================================
        // Strategic Goal Calculations
        // ====================================================================

        /**
         * @brief Score strategic goal desirability based on current situation
         */
        static float ScoreGoalDesirability(
            StrategicGoal goal,
            float aggressiveness,
            float economicHealth,
            float militaryStrength,
            float stability
        );

        /**
         * @brief Calculate goal achievement progress
         */
        static float CalculateGoalProgress(
            StrategicGoal goal,
            int provinceCount,
            double treasury,
            float stability
        );

        /**
         * @brief Determine if goal is achieved
         */
        static bool IsGoalAchieved(StrategicGoal goal, float progress);

        // ====================================================================
        // War Decision Calculations
        // ====================================================================

        /**
         * @brief Calculate expected war success chance
         */
        static float CalculateWarSuccessChance(
            float relativeStrength,
            float aggressiveness,
            float riskTolerance
        );

        /**
         * @brief Calculate relative military strength
         */
        static float CalculateRelativeStrength(
            int ourForces,
            int theirForces,
            float ourStability,
            float theirStability
        );

        /**
         * @brief Calculate war desirability score
         */
        static float CalculateWarDesirability(
            float expectedSuccess,
            float aggressiveness,
            StrategicGoal primaryGoal,
            float threatLevel
        );

        /**
         * @brief Determine if war should be declared
         */
        static bool ShouldDeclareWar(
            float warDesirability,
            float expectedSuccess,
            float minSuccessThreshold
        );

        // ====================================================================
        // Threat Assessment
        // ====================================================================

        /**
         * @brief Assess threat level from another realm
         */
        static ThreatLevel AssessThreat(
            float militaryDifference,
            float opinion,
            float proximity,
            bool atWar
        );

        /**
         * @brief Calculate threat score (0-1)
         */
        static float CalculateThreatScore(
            int theirForces,
            int ourForces,
            float opinion,
            bool hasCommonBorder
        );

        // ====================================================================
        // Economic Calculations
        // ====================================================================

        /**
         * @brief Calculate economic health score
         */
        static float CalculateEconomicHealth(
            double treasury,
            double monthlyIncome,
            double monthlyExpenses
        );

        /**
         * @brief Determine economic policy action
         */
        static EconomicDecision::ActionType DetermineEconomicAction(
            float economicHealth,
            StrategicGoal primaryGoal
        );

        /**
         * @brief Calculate tax adjustment value
         */
        static float CalculateTaxAdjustment(
            float currentHealth,
            float targetHealth
        );

        // ====================================================================
        // Military Calculations
        // ====================================================================

        /**
         * @brief Calculate military readiness
         */
        static float CalculateMilitaryReadiness(
            int totalForces,
            int provinceCount,
            int recommendedForcesPerProvince
        );

        /**
         * @brief Calculate required military size
         */
        static int CalculateRequiredForces(
            StrategicGoal goal,
            int provinceCount,
            float threatLevel
        );

        /**
         * @brief Determine military action priority
         */
        static MilitaryDecision::MilitaryAction DetermineMilitaryAction(
            float readiness,
            StrategicGoal primaryGoal,
            float treasury
        );

        // ====================================================================
        // Diplomatic Calculations
        // ====================================================================

        /**
         * @brief Calculate relationship score
         */
        static float CalculateRelationshipScore(
            float opinion,
            bool hasAlliance,
            bool atWar,
            bool hasTradeAgreement
        );

        /**
         * @brief Determine diplomatic action
         */
        static DiplomaticDecision::DiplomaticAction DetermineDiplomaticAction(
            float relationshipScore,
            ThreatLevel threat,
            StrategicGoal primaryGoal
        );

        /**
         * @brief Calculate alliance value
         */
        static float CalculateAllianceValue(
            float theirMilitaryStrength,
            float relationshipScore,
            float sharedThreatLevel
        );

        // ====================================================================
        // Personality Adjustments
        // ====================================================================

        /**
         * @brief Adjust aggressiveness based on recent events
         */
        static float AdjustAggressiveness(
            float currentAggressiveness,
            float stability,
            float economicHealth,
            int recentWars
        );

        /**
         * @brief Adjust risk tolerance based on situation
         */
        static float AdjustRiskTolerance(
            float currentRiskTolerance,
            float treasury,
            int threatCount
        );

        // ====================================================================
        // Decision Scoring
        // ====================================================================

        /**
         * @brief Calculate overall decision priority
         */
        static float CalculateDecisionPriority(
            float desirability,
            float urgency,
            float expectedBenefit,
            float cost
        );

        /**
         * @brief Normalize priority score
         */
        static float NormalizePriority(float priority);

        // ====================================================================
        // Utility Functions
        // ====================================================================

        /**
         * @brief Clamp value to range
         */
        static float Clamp(float value, float min_val, float max_val);

        /**
         * @brief Linear interpolation
         */
        static float Lerp(float a, float b, float t);

        /**
         * @brief Check if value is critical (very low/high)
         */
        static bool IsCritical(float value, float threshold);
    };

} // namespace ai
} // namespace game
