// ============================================================================
// Mechanica Imperii - AI Calculator Implementation
// ============================================================================

#include "game/ai/calculators/AICalculator.h"
#include <cmath>
#include <algorithm>

namespace AI {

    // ========================================================================
    // Plot Calculations
    // ========================================================================

    float AICalculator::CalculatePlotSuccessChance(
        PlotDecision::PlotType type,
        float boldness,
        float honor,
        float loyalty) {

        float baseChance = 0.3f;

        switch (type) {
        case PlotDecision::ASSASSINATION:
            baseChance = 0.3f * (1.0f - honor) * boldness;
            break;
        case PlotDecision::COUP:
            baseChance = 0.2f * boldness * (1.0f - loyalty);
            break;
        case PlotDecision::BLACKMAIL:
            baseChance = 0.5f * boldness;
            break;
        case PlotDecision::FABRICATE_CLAIM:
            baseChance = 0.6f;
            break;
        default:
            baseChance = 0.4f;
            break;
        }

        return Clamp(baseChance, 0.0f, 1.0f);
    }

    float AICalculator::CalculatePlotDesirability(
        PlotDecision::PlotType type,
        float ambition,
        float greed,
        float riskLevel,
        float successChance) {

        float baseDesirability = 0.5f;

        // Ambition drives dangerous plots
        baseDesirability += ambition * 0.3f;

        // Greed influences material plots
        if (type == PlotDecision::BLACKMAIL || type == PlotDecision::SEIZE_ASSETS) {
            baseDesirability += greed * 0.4f;
        }

        // Success chance matters
        baseDesirability += successChance * 0.5f;

        // Risk reduces desirability
        baseDesirability -= riskLevel * 0.3f;

        return Clamp(baseDesirability, 0.0f, 1.0f);
    }

    float AICalculator::CalculatePlotRisk(PlotDecision::PlotType type) {
        switch (type) {
        case PlotDecision::ASSASSINATION: return 0.9f;
        case PlotDecision::COUP: return 0.95f;
        case PlotDecision::BLACKMAIL: return 0.6f;
        case PlotDecision::FABRICATE_CLAIM: return 0.4f;
        case PlotDecision::SEIZE_ASSETS: return 0.7f;
        default: return 0.5f;
        }
    }

    bool AICalculator::ShouldExecutePlot(
        float desirability,
        float successChance,
        float boldness,
        float riskLevel) {

        return desirability > 0.6f &&
               successChance > 0.3f &&
               boldness > riskLevel * 0.5f;
    }

    // ========================================================================
    // Proposal Calculations
    // ========================================================================

    float AICalculator::CalculateProposalAcceptance(
        ProposalDecision::ProposalType type,
        float loyalty,
        float compassion) {

        float baseAcceptance = 0.3f;

        switch (type) {
        case ProposalDecision::REQUEST_TITLE:
            baseAcceptance = 0.3f + (loyalty * 0.4f);
            break;
        case ProposalDecision::REQUEST_GOLD:
            baseAcceptance = 0.4f + (loyalty * 0.3f);
            break;
        case ProposalDecision::REQUEST_MARRIAGE:
            baseAcceptance = 0.5f;
            break;
        case ProposalDecision::REQUEST_COUNCIL_POSITION:
            baseAcceptance = 0.2f + (loyalty * 0.5f);
            break;
        case ProposalDecision::SUGGEST_WAR:
            baseAcceptance = 0.3f;
            break;
        default:
            baseAcceptance = 0.4f;
            break;
        }

        return AdjustAcceptanceByPersonality(baseAcceptance, compassion);
    }

    float AICalculator::AdjustAcceptanceByPersonality(
        float baseAcceptance,
        float compassion) {

        float adjusted = baseAcceptance * (1.0f + compassion * 0.2f);
        return Clamp(adjusted, 0.0f, 1.0f);
    }

    // ========================================================================
    // Relationship Calculations
    // ========================================================================

    float AICalculator::CalculateOpinionDecay(float currentOpinion) {
        if (currentOpinion > 0.0f) {
            return -0.5f; // Positive opinions decay
        } else if (currentOpinion < 0.0f) {
            return +0.3f; // Negative opinions recover slowly
        }
        return 0.0f;
    }

    std::string AICalculator::DetermineRelationshipType(
        float opinion,
        bool isRival,
        bool isFriend) {

        if (opinion < -50.0f && !isRival) {
            return "rival";
        } else if (opinion > 70.0f && !isFriend) {
            return "friend";
        } else if (opinion > 50.0f) {
            return "ally";
        } else if (opinion < -30.0f) {
            return "enemy";
        }
        return "neutral";
    }

    float AICalculator::CalculateRelationshipDesirability(
        float opinion,
        float ambition,
        float boldness,
        float honor) {

        float desirability = 0.5f;

        // High opinion increases friendship desirability
        if (opinion > 70.0f) {
            desirability += 0.3f;
        }

        // Low opinion + low honor = blackmail opportunity
        if (opinion < -50.0f && honor < 0.4f) {
            desirability += boldness * 0.4f;
        }

        // Ambition drives relationship building
        desirability += ambition * 0.2f;

        return Clamp(desirability, 0.0f, 1.0f);
    }

    // ========================================================================
    // Ambition Calculations
    // ========================================================================

    float AICalculator::ScoreAmbitionDesirability(
        CharacterAmbition ambition,
        float ambitionTrait,
        float greed,
        float compassion) {

        float score = 0.5f;

        switch (ambition) {
        case CharacterAmbition::ACCUMULATE_WEALTH:
            score = greed * 0.8f + ambitionTrait * 0.2f;
            break;
        case CharacterAmbition::POWER:
            score = ambitionTrait * 0.9f;
            break;
        case CharacterAmbition::GAIN_LAND:
            score = ambitionTrait * 0.7f + (1.0f - compassion) * 0.3f;
            break;
        case CharacterAmbition::PIETY:
            score = compassion * 0.6f;
            break;
        case CharacterAmbition::LEGACY:
            score = ambitionTrait * 0.5f + compassion * 0.3f;
            break;
        case CharacterAmbition::KNOWLEDGE:
            score = compassion * 0.4f;
            break;
        default:
            score = ambitionTrait * 0.5f;
            break;
        }

        return Clamp(score, 0.0f, 1.0f);
    }

    float AICalculator::CalculateAmbitionProgress(
        CharacterAmbition ambition,
        int currentValue,
        int targetValue) {

        if (targetValue <= 0) return 0.0f;
        return Clamp(static_cast<float>(currentValue) / targetValue, 0.0f, 1.0f);
    }

    // ========================================================================
    // Mood Calculations
    // ========================================================================

    CharacterMood AICalculator::DetermineMood(
        float averageEventSeverity,
        float recentOpinionChange,
        float ambitionProgress) {

        // Calculate mood score (-1.0 to 1.0)
        float moodScore = 0.0f;

        // Event severity influences mood
        moodScore -= averageEventSeverity * 0.5f;

        // Opinion changes affect mood
        moodScore += recentOpinionChange * 0.01f; // Scale down opinion

        // Ambition progress improves mood
        moodScore += ambitionProgress * 0.5f;

        // Determine mood from score
        if (moodScore > 0.5f) return CharacterMood::ELATED;
        if (moodScore > 0.2f) return CharacterMood::HAPPY;
        if (moodScore > -0.2f) return CharacterMood::CONTENT;
        if (moodScore > -0.5f) return CharacterMood::UNHAPPY;
        if (moodScore > -0.7f) return CharacterMood::ANGRY;
        return CharacterMood::AFRAID;
    }

    float AICalculator::CalculateMoodModifier(CharacterMood mood) {
        switch (mood) {
        case CharacterMood::ELATED: return 1.3f;
        case CharacterMood::HAPPY: return 1.15f;
        case CharacterMood::CONTENT: return 1.0f;
        case CharacterMood::UNHAPPY: return 0.85f;
        case CharacterMood::ANGRY: return 0.7f;
        case CharacterMood::AFRAID: return 0.5f;
        default: return 1.0f;
        }
    }

    // ========================================================================
    // Personal Action Calculations
    // ========================================================================

    float AICalculator::CalculateExpectedBenefit(
        float successChance,
        float potentialGain,
        float riskLevel) {

        float expectedValue = successChance * potentialGain;
        float riskPenalty = riskLevel * potentialGain * 0.3f;
        return std::max(0.0f, expectedValue - riskPenalty);
    }

    float AICalculator::CalculateActionCost(
        float baseCost,
        float complexity,
        float riskLevel) {

        return baseCost * (1.0f + complexity * 0.5f) * (1.0f + riskLevel * 0.3f);
    }

    // ========================================================================
    // Decision Scoring
    // ========================================================================

    float AICalculator::CalculateDecisionScore(
        float desirability,
        float successChance,
        float riskTolerance,
        float moodModifier) {

        float score = desirability * 0.5f +
                     successChance * 0.3f +
                     riskTolerance * 0.2f;

        score *= moodModifier;

        return NormalizeScore(score);
    }

    float AICalculator::NormalizeScore(float score) {
        return Clamp(score, 0.0f, 1.0f);
    }

    // ========================================================================
    // Utility Functions
    // ========================================================================

    float AICalculator::Clamp(float value, float min_val, float max_val) {
        return std::max(min_val, std::min(value, max_val));
    }

    float AICalculator::Lerp(float a, float b, float t) {
        return a + (b - a) * Clamp(t, 0.0f, 1.0f);
    }

    bool AICalculator::InRange(float value, float min_val, float max_val) {
        return value >= min_val && value <= max_val;
    }

} // namespace AI
