// ============================================================================
// Mechanica Imperii - AI Attention Calculator Header
// Pure Calculation Functions for Attention System
// ============================================================================

#pragma once

#include "game/ai/AIAttentionManager.h"
#include <vector>
#include <cstdint>

namespace AI {

    /**
     * @brief Pure calculation functions for AI attention system
     * All functions are static with no side effects
     */
    class AIAttentionCalculator {
    public:
        // ====================================================================
        // Attention Score Calculations
        // ====================================================================

        /**
         * @brief Calculate overall attention score for an information packet
         * Components:
         * - Type weight: 40%
         * - Severity: 30%
         * - Accuracy: 20%
         * - Relevance: 10%
         * @return Score in range [0.0, 1.0]
         */
        static float CalculateAttentionScore(
            float typeWeight,
            float severity,
            float accuracy,
            InformationRelevance baseRelevance,
            float globalMultiplier = 1.0f
        );

        /**
         * @brief Calculate type weight component of attention score
         */
        static float CalculateTypeWeightComponent(float typeWeight);

        /**
         * @brief Calculate severity component of attention score
         */
        static float CalculateSeverityComponent(float severity);

        /**
         * @brief Calculate accuracy component of attention score
         */
        static float CalculateAccuracyComponent(float accuracy);

        /**
         * @brief Calculate relevance component of attention score
         */
        static float CalculateRelevanceComponent(InformationRelevance relevance);

        /**
         * @brief Convert information relevance to numeric score
         * CRITICAL: 1.0
         * HIGH: 0.7
         * MEDIUM: 0.4
         * LOW: 0.2
         */
        static float RelevanceToScore(InformationRelevance relevance);

        // ====================================================================
        // Distance and Type Filtering
        // ====================================================================

        /**
         * @brief Calculate estimated distance from hop count
         * Uses 200 units per hop as estimation
         */
        static float CalculateEstimatedDistance(uint32_t hopCount);

        /**
         * @brief Check if information passes distance filter
         */
        static bool PassesDistanceFilter(
            uint32_t hopCount,
            float maxAttentionDistance
        );

        /**
         * @brief Check if information type passes relevance threshold
         */
        static bool PassesTypeFilter(
            float typeWeight,
            float minTypeWeight = 0.1f
        );

        /**
         * @brief Check if entity is in a list (rival, ally, watched)
         */
        static bool IsInList(
            uint32_t entityId,
            const std::vector<uint32_t>& list
        );

        /**
         * @brief Determine if information is special interest
         * (from rival, ally, or watched province)
         */
        static bool IsSpecialInterest(
            uint32_t originatorId,
            uint32_t provinceId,
            const std::vector<uint32_t>& rivalNations,
            const std::vector<uint32_t>& alliedNations,
            const std::vector<uint32_t>& watchedProvinces
        );

        // ====================================================================
        // Relevance Adjustment
        // ====================================================================

        /**
         * @brief Adjust information relevance based on attention score
         */
        static InformationRelevance AdjustRelevanceByScore(
            InformationRelevance baseRelevance,
            float attentionScore,
            float criticalThreshold,
            float highThreshold,
            float mediumThreshold,
            float lowThreshold
        );

        /**
         * @brief Upgrade relevance to higher level
         */
        static InformationRelevance UpgradeRelevance(
            InformationRelevance current,
            InformationRelevance minimum
        );

        // ====================================================================
        // Processing Delay Calculations
        // ====================================================================

        /**
         * @brief Calculate processing delay in days based on attention score
         * Critical (>= criticalThreshold): 0 days
         * High (>= highThreshold): 1 day
         * Medium (>= mediumThreshold): 3 days
         * Low (default): 7 days
         */
        static float CalculateProcessingDelay(
            float attentionScore,
            float criticalThreshold,
            float highThreshold,
            float mediumThreshold
        );

        // ====================================================================
        // Personality and Archetype Mapping
        // ====================================================================

        /**
         * @brief Map character archetype to nation personality
         */
        static NationPersonality ArchetypeToPersonality(
            CharacterArchetype archetype
        );

        /**
         * @brief Map nation personality to character archetype
         */
        static CharacterArchetype PersonalityToArchetype(
            NationPersonality personality
        );

        // ====================================================================
        // Threshold Classification
        // ====================================================================

        /**
         * @brief Classify attention score into priority tier
         */
        enum class AttentionTier {
            CRITICAL,
            HIGH,
            MEDIUM,
            LOW,
            BELOW_THRESHOLD
        };

        static AttentionTier ClassifyAttentionTier(
            float attentionScore,
            float criticalThreshold,
            float highThreshold,
            float mediumThreshold,
            float lowThreshold
        );

        // ====================================================================
        // Component Weight Calculations
        // ====================================================================

        /**
         * @brief Get standard component weights for attention calculation
         * Returns {typeWeight%, severity%, accuracy%, relevance%}
         */
        static constexpr float TYPE_WEIGHT_PERCENTAGE = 0.4f;      // 40%
        static constexpr float SEVERITY_PERCENTAGE = 0.3f;         // 30%
        static constexpr float ACCURACY_PERCENTAGE = 0.2f;         // 20%
        static constexpr float RELEVANCE_PERCENTAGE = 0.1f;        // 10%

        /**
         * @brief Distance estimation constant (units per hop)
         */
        static constexpr float DISTANCE_PER_HOP = 200.0f;

        /**
         * @brief Minimum type weight for filtering
         */
        static constexpr float MIN_TYPE_WEIGHT = 0.1f;

        // ====================================================================
        // Utility Functions
        // ====================================================================

        /**
         * @brief Clamp value to range [0.0, 1.0]
         */
        static float Clamp01(float value);

        /**
         * @brief Clamp value to range
         */
        static float Clamp(float value, float min_val, float max_val);
    };

} // namespace AI
