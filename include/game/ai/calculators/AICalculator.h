// ============================================================================
// Mechanica Imperii - AI Calculator Header
// Pure Calculation Functions for AI Systems
// ============================================================================

#pragma once

#include "game/ai/CharacterAI.h"
#include <algorithm>

namespace AI {

    /**
     * @brief Pure calculation functions for AI decision-making
     * All functions are static with no side effects
     */
    class AICalculator {
    public:
        // ====================================================================
        // Plot Calculations
        // ====================================================================

        /**
         * @brief Calculate plot success chance based on traits
         */
        static float CalculatePlotSuccessChance(
            PlotDecision::PlotType type,
            float boldness,
            float honor,
            float loyalty
        );

        /**
         * @brief Calculate plot desirability
         */
        static float CalculatePlotDesirability(
            PlotDecision::PlotType type,
            float ambition,
            float greed,
            float riskLevel,
            float successChance
        );

        /**
         * @brief Calculate plot risk level
         */
        static float CalculatePlotRisk(PlotDecision::PlotType type);

        /**
         * @brief Determine if plot should be executed
         */
        static bool ShouldExecutePlot(
            float desirability,
            float successChance,
            float boldness,
            float riskLevel
        );

        // ====================================================================
        // Proposal Calculations
        // ====================================================================

        /**
         * @brief Calculate proposal acceptance chance
         */
        static float CalculateProposalAcceptance(
            ProposalDecision::ProposalType type,
            float loyalty,
            float compassion
        );

        /**
         * @brief Adjust acceptance chance by personality
         */
        static float AdjustAcceptanceByPersonality(
            float baseAcceptance,
            float compassion
        );

        // ====================================================================
        // Relationship Calculations
        // ====================================================================

        /**
         * @brief Calculate opinion decay rate
         */
        static float CalculateOpinionDecay(float currentOpinion);

        /**
         * @brief Determine relationship type from opinion
         */
        static std::string DetermineRelationshipType(
            float opinion,
            bool isRival,
            bool isFriend
        );

        /**
         * @brief Calculate relationship action desirability
         */
        static float CalculateRelationshipDesirability(
            float opinion,
            float ambition,
            float boldness,
            float honor
        );

        // ====================================================================
        // Ambition Calculations
        // ====================================================================

        /**
         * @brief Score ambition desirability based on personality
         */
        static float ScoreAmbitionDesirability(
            CharacterAmbition ambition,
            float ambitionTrait,
            float greed,
            float compassion
        );

        /**
         * @brief Calculate ambition achievement progress
         */
        static float CalculateAmbitionProgress(
            CharacterAmbition ambition,
            int currentValue,
            int targetValue
        );

        // ====================================================================
        // Mood Calculations
        // ====================================================================

        /**
         * @brief Determine character mood from recent events
         */
        static CharacterMood DetermineMood(
            float averageEventSeverity,
            float recentOpinionChange,
            float ambitionProgress
        );

        /**
         * @brief Calculate mood modifier for decisions
         */
        static float CalculateMoodModifier(CharacterMood mood);

        // ====================================================================
        // Personal Action Calculations
        // ====================================================================

        /**
         * @brief Calculate expected benefit of personal action
         */
        static float CalculateExpectedBenefit(
            float successChance,
            float potentialGain,
            float riskLevel
        );

        /**
         * @brief Calculate action cost
         */
        static float CalculateActionCost(
            float baseCost,
            float complexity,
            float riskLevel
        );

        // ====================================================================
        // Decision Scoring
        // ====================================================================

        /**
         * @brief Calculate overall decision score
         */
        static float CalculateDecisionScore(
            float desirability,
            float successChance,
            float riskTolerance,
            float mood Modifier
        );

        /**
         * @brief Normalize score to 0-1 range
         */
        static float NormalizeScore(float score);

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
         * @brief Check if value is in range
         */
        static bool InRange(float value, float min_val, float max_val);
    };

} // namespace AI
