// ============================================================================
// Mechanica Imperii - AI Attention Calculator Implementation
// ============================================================================

#include "game/ai/calculators/AIAttentionCalculator.h"
#include <algorithm>
#include <cmath>

namespace AI {

    // ========================================================================
    // Attention Score Calculations
    // ========================================================================

    float AIAttentionCalculator::CalculateAttentionScore(
        float typeWeight,
        float severity,
        float accuracy,
        InformationRelevance baseRelevance,
        float globalMultiplier) {

        float score = 0.0f;

        // Type weight component (40%)
        score += CalculateTypeWeightComponent(typeWeight);

        // Severity component (30%)
        score += CalculateSeverityComponent(severity);

        // Accuracy component (20%)
        score += CalculateAccuracyComponent(accuracy);

        // Relevance component (10%)
        score += CalculateRelevanceComponent(baseRelevance);

        // Apply global multiplier
        score *= globalMultiplier;

        return Clamp01(score);
    }

    float AIAttentionCalculator::CalculateTypeWeightComponent(float typeWeight) {
        return typeWeight * TYPE_WEIGHT_PERCENTAGE;
    }

    float AIAttentionCalculator::CalculateSeverityComponent(float severity) {
        return severity * SEVERITY_PERCENTAGE;
    }

    float AIAttentionCalculator::CalculateAccuracyComponent(float accuracy) {
        return accuracy * ACCURACY_PERCENTAGE;
    }

    float AIAttentionCalculator::CalculateRelevanceComponent(
        InformationRelevance relevance) {

        float relevanceScore = RelevanceToScore(relevance);
        return relevanceScore * RELEVANCE_PERCENTAGE;
    }

    float AIAttentionCalculator::RelevanceToScore(InformationRelevance relevance) {
        switch (relevance) {
        case InformationRelevance::CRITICAL:
            return 1.0f;
        case InformationRelevance::HIGH:
            return 0.7f;
        case InformationRelevance::MEDIUM:
            return 0.4f;
        case InformationRelevance::LOW:
            return 0.2f;
        default:
            return 0.0f;
        }
    }

    // ========================================================================
    // Distance and Type Filtering
    // ========================================================================

    float AIAttentionCalculator::CalculateEstimatedDistance(uint32_t hopCount) {
        return static_cast<float>(hopCount) * DISTANCE_PER_HOP;
    }

    bool AIAttentionCalculator::PassesDistanceFilter(
        uint32_t hopCount,
        float maxAttentionDistance) {

        float estimatedDistance = CalculateEstimatedDistance(hopCount);
        return estimatedDistance <= maxAttentionDistance;
    }

    bool AIAttentionCalculator::PassesTypeFilter(
        float typeWeight,
        float minTypeWeight) {

        return typeWeight > minTypeWeight;
    }

    bool AIAttentionCalculator::IsInList(
        uint32_t entityId,
        const std::vector<uint32_t>& list) {

        return std::find(list.begin(), list.end(), entityId) != list.end();
    }

    bool AIAttentionCalculator::IsSpecialInterest(
        uint32_t originatorId,
        uint32_t provinceId,
        const std::vector<uint32_t>& rivalNations,
        const std::vector<uint32_t>& alliedNations,
        const std::vector<uint32_t>& watchedProvinces) {

        // Check if from rival nation
        if (originatorId != 0) {
            if (IsInList(originatorId, rivalNations)) {
                return true;
            }

            // Check if from allied nation
            if (IsInList(originatorId, alliedNations)) {
                return true;
            }
        }

        // Check if from watched province
        if (IsInList(provinceId, watchedProvinces)) {
            return true;
        }

        return false;
    }

    // ========================================================================
    // Relevance Adjustment
    // ========================================================================

    InformationRelevance AIAttentionCalculator::AdjustRelevanceByScore(
        InformationRelevance baseRelevance,
        float attentionScore,
        float criticalThreshold,
        float highThreshold,
        float mediumThreshold,
        float lowThreshold) {

        // Upgrade relevance based on attention score
        if (attentionScore >= criticalThreshold) {
            return InformationRelevance::CRITICAL;
        } else if (attentionScore >= highThreshold) {
            return UpgradeRelevance(baseRelevance, InformationRelevance::HIGH);
        } else if (attentionScore >= mediumThreshold) {
            return UpgradeRelevance(baseRelevance, InformationRelevance::MEDIUM);
        } else if (attentionScore >= lowThreshold) {
            return UpgradeRelevance(baseRelevance, InformationRelevance::LOW);
        }

        return baseRelevance;
    }

    InformationRelevance AIAttentionCalculator::UpgradeRelevance(
        InformationRelevance current,
        InformationRelevance minimum) {

        return std::max(current, minimum);
    }

    // ========================================================================
    // Processing Delay Calculations
    // ========================================================================

    float AIAttentionCalculator::CalculateProcessingDelay(
        float attentionScore,
        float criticalThreshold,
        float highThreshold,
        float mediumThreshold) {

        if (attentionScore >= criticalThreshold) {
            return 0.0f; // Immediate
        } else if (attentionScore >= highThreshold) {
            return 1.0f; // 1 day
        } else if (attentionScore >= mediumThreshold) {
            return 3.0f; // 3 days
        } else {
            return 7.0f; // 7 days (low priority)
        }
    }

    // ========================================================================
    // Personality and Archetype Mapping
    // ========================================================================

    NationPersonality AIAttentionCalculator::ArchetypeToPersonality(
        CharacterArchetype archetype) {

        switch (archetype) {
        case CharacterArchetype::THE_CONQUEROR:
        case CharacterArchetype::WARRIOR_KING:
            return NationPersonality::EXPANSIONIST;

        case CharacterArchetype::THE_DIPLOMAT:
            return NationPersonality::DIPLOMATIC;

        case CharacterArchetype::THE_MERCHANT:
            return NationPersonality::ECONOMIC;

        case CharacterArchetype::THE_SCHOLAR:
            return NationPersonality::TECHNOLOGICAL;

        case CharacterArchetype::THE_ZEALOT:
            return NationPersonality::RELIGIOUS;

        case CharacterArchetype::THE_BUILDER:
        case CharacterArchetype::THE_ADMINISTRATOR:
            return NationPersonality::DEVELOPMENTAL;

        case CharacterArchetype::THE_TYRANT:
            return NationPersonality::AGGRESSIVE;

        case CharacterArchetype::THE_REFORMER:
            return NationPersonality::PROGRESSIVE;

        default:
            return NationPersonality::BALANCED;
        }
    }

    CharacterArchetype AIAttentionCalculator::PersonalityToArchetype(
        NationPersonality personality) {

        switch (personality) {
        case NationPersonality::EXPANSIONIST:
            return CharacterArchetype::THE_CONQUEROR;

        case NationPersonality::DIPLOMATIC:
            return CharacterArchetype::THE_DIPLOMAT;

        case NationPersonality::ECONOMIC:
            return CharacterArchetype::THE_MERCHANT;

        case NationPersonality::TECHNOLOGICAL:
            return CharacterArchetype::THE_SCHOLAR;

        case NationPersonality::RELIGIOUS:
            return CharacterArchetype::THE_ZEALOT;

        case NationPersonality::DEVELOPMENTAL:
            return CharacterArchetype::THE_BUILDER;

        case NationPersonality::AGGRESSIVE:
            return CharacterArchetype::THE_TYRANT;

        case NationPersonality::PROGRESSIVE:
            return CharacterArchetype::THE_REFORMER;

        case NationPersonality::BALANCED:
        default:
            return CharacterArchetype::THE_ADMINISTRATOR;
        }
    }

    // ========================================================================
    // Threshold Classification
    // ========================================================================

    AIAttentionCalculator::AttentionTier AIAttentionCalculator::ClassifyAttentionTier(
        float attentionScore,
        float criticalThreshold,
        float highThreshold,
        float mediumThreshold,
        float lowThreshold) {

        if (attentionScore >= criticalThreshold) {
            return AttentionTier::CRITICAL;
        } else if (attentionScore >= highThreshold) {
            return AttentionTier::HIGH;
        } else if (attentionScore >= mediumThreshold) {
            return AttentionTier::MEDIUM;
        } else if (attentionScore >= lowThreshold) {
            return AttentionTier::LOW;
        } else {
            return AttentionTier::BELOW_THRESHOLD;
        }
    }

    // ========================================================================
    // Utility Functions
    // ========================================================================

    float AIAttentionCalculator::Clamp01(float value) {
        return Clamp(value, 0.0f, 1.0f);
    }

    float AIAttentionCalculator::Clamp(float value, float min_val, float max_val) {
        return std::max(min_val, std::min(value, max_val));
    }

} // namespace AI
