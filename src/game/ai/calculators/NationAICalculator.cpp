// ============================================================================
// Mechanica Imperii - Nation AI Calculator Implementation
// ============================================================================

#include "game/ai/calculators/NationAICalculator.h"
#include <cmath>
#include <algorithm>

namespace game {
namespace ai {

    // ========================================================================
    // Strategic Goal Calculations
    // ========================================================================

    float NationAICalculator::ScoreGoalDesirability(
        StrategicGoal goal,
        float aggressiveness,
        float economicHealth,
        float militaryStrength,
        float stability) {

        float score = 0.5f;

        switch (goal) {
        case StrategicGoal::EXPANSION:
            score = aggressiveness * 0.6f + militaryStrength * 0.3f + economicHealth * 0.1f;
            break;
        case StrategicGoal::CONSOLIDATION:
            score = (1.0f - aggressiveness) * 0.5f + stability * 0.5f;
            break;
        case StrategicGoal::ECONOMIC_GROWTH:
            score = economicHealth * 0.7f + (1.0f - aggressiveness) * 0.3f;
            break;
        case StrategicGoal::DIPLOMATIC_DOMINANCE:
            score = (1.0f - aggressiveness) * 0.6f + economicHealth * 0.4f;
            break;
        case StrategicGoal::TECHNOLOGICAL_ADVANCEMENT:
            score = economicHealth * 0.5f + stability * 0.5f;
            break;
        case StrategicGoal::CULTURAL_SUPREMACY:
            score = economicHealth * 0.4f + stability * 0.6f;
            break;
        case StrategicGoal::SURVIVAL:
            score = stability < 0.3f ? 1.0f : 0.1f; // Survival is critical when stability low
            break;
        default:
            score = 0.5f;
            break;
        }

        return Clamp(score, 0.0f, 1.0f);
    }

    float NationAICalculator::CalculateGoalProgress(
        StrategicGoal goal,
        int provinceCount,
        double treasury,
        float stability) {

        switch (goal) {
        case StrategicGoal::EXPANSION:
            return Clamp(provinceCount / 50.0f, 0.0f, 1.0f); // Target: 50 provinces
        case StrategicGoal::CONSOLIDATION:
            return stability;
        case StrategicGoal::ECONOMIC_GROWTH:
            return Clamp(static_cast<float>(treasury / 20000.0), 0.0f, 1.0f); // Target: 20k treasury
        case StrategicGoal::SURVIVAL:
            return stability > 0.3f ? 1.0f : stability / 0.3f;
        default:
            return 0.5f;
        }
    }

    bool NationAICalculator::IsGoalAchieved(StrategicGoal goal, float progress) {
        return progress >= 0.8f; // 80% progress = achieved
    }

    // ========================================================================
    // War Decision Calculations
    // ========================================================================

    float NationAICalculator::CalculateWarSuccessChance(
        float relativeStrength,
        float aggressiveness,
        float riskTolerance) {

        float baseChance = std::min(1.0f, relativeStrength * 0.7f);

        // Aggressive nations overestimate
        baseChance += aggressiveness * 0.1f;

        // Risk tolerant nations more optimistic
        baseChance += riskTolerance * 0.05f;

        return Clamp(baseChance, 0.0f, 1.0f);
    }

    float NationAICalculator::CalculateRelativeStrength(
        int ourForces,
        int theirForces,
        float ourStability,
        float theirStability) {

        if (theirForces == 0) return 2.0f; // Maximum advantage

        float militaryRatio = static_cast<float>(ourForces) / theirForces;
        float stabilityAdvantage = ourStability / std::max(0.1f, theirStability);

        return (militaryRatio * 0.7f + stabilityAdvantage * 0.3f);
    }

    float NationAICalculator::CalculateWarDesirability(
        float expectedSuccess,
        float aggressiveness,
        StrategicGoal primaryGoal,
        float threatLevel) {

        float desirability = 0.3f;

        // Success chance influences desirability
        desirability += expectedSuccess * 0.4f;

        // Aggressiveness increases war appetite
        desirability += aggressiveness * 0.3f;

        // Expansion goal increases war desirability
        if (primaryGoal == StrategicGoal::EXPANSION) {
            desirability += 0.2f;
        }

        // High threat increases defensive war desirability
        desirability += threatLevel * 0.3f;

        return Clamp(desirability, 0.0f, 1.0f);
    }

    bool NationAICalculator::ShouldDeclareWar(
        float warDesirability,
        float expectedSuccess,
        float minSuccessThreshold) {

        return warDesirability > 0.6f && expectedSuccess > minSuccessThreshold;
    }

    // ========================================================================
    // Threat Assessment
    // ========================================================================

    ThreatLevel NationAICalculator::AssessThreat(
        float militaryDifference,
        float opinion,
        float proximity,
        bool atWar) {

        if (atWar) return ThreatLevel::CRITICAL;

        float threatScore = 0.0f;

        // Military advantage of enemy
        if (militaryDifference < -0.5f) threatScore += 0.4f; // They're much stronger

        // Hostile opinion
        if (opinion < -50.0f) threatScore += 0.3f;

        // Proximity matters
        threatScore += proximity * 0.3f;

        // Determine threat level
        if (threatScore > 0.8f) return ThreatLevel::CRITICAL;
        if (threatScore > 0.6f) return ThreatLevel::SEVERE;
        if (threatScore > 0.4f) return ThreatLevel::MODERATE;
        if (threatScore > 0.2f) return ThreatLevel::MINOR;
        return ThreatLevel::NONE;
    }

    float NationAICalculator::CalculateThreatScore(
        int theirForces,
        int ourForces,
        float opinion,
        bool hasCommonBorder) {

        float threatScore = 0.0f;

        // Military threat
        if (ourForces > 0) {
            float militaryRatio = static_cast<float>(theirForces) / ourForces;
            if (militaryRatio > 1.5f) threatScore += 0.5f;
            else if (militaryRatio > 1.0f) threatScore += 0.3f;
        }

        // Opinion-based threat
        if (opinion < -50.0f) threatScore += 0.3f;
        else if (opinion < 0.0f) threatScore += 0.1f;

        // Proximity threat
        if (hasCommonBorder) threatScore += 0.2f;

        return Clamp(threatScore, 0.0f, 1.0f);
    }

    // ========================================================================
    // Economic Calculations
    // ========================================================================

    float NationAICalculator::CalculateEconomicHealth(
        double treasury,
        double monthlyIncome,
        double monthlyExpenses) {

        if (monthlyExpenses <= 0.0) return 1.0f;

        // How many months can we survive?
        float monthsOfReserve = static_cast<float>(treasury / std::max(1.0, monthlyExpenses));

        // Income/expense ratio
        float incomeRatio = static_cast<float>(monthlyIncome / std::max(1.0, monthlyExpenses));

        // Combined health score
        float reserveScore = std::min(1.0f, monthsOfReserve / 12.0f); // 12 months = excellent
        float incomeScore = std::min(1.0f, incomeRatio);

        return (reserveScore * 0.4f + incomeScore * 0.6f);
    }

    EconomicDecision::ActionType NationAICalculator::DetermineEconomicAction(
        float economicHealth,
        StrategicGoal primaryGoal) {

        if (economicHealth < 0.3f) {
            return EconomicDecision::ADJUST_TAXES; // Emergency
        } else if (economicHealth > 0.7f) {
            if (primaryGoal == StrategicGoal::ECONOMIC_GROWTH ||
                primaryGoal == StrategicGoal::TECHNOLOGICAL_ADVANCEMENT) {
                return EconomicDecision::BUILD_INFRASTRUCTURE;
            } else {
                return EconomicDecision::DEVELOP_TRADE;
            }
        }
        return EconomicDecision::DEVELOP_TRADE; // Default balanced approach
    }

    float NationAICalculator::CalculateTaxAdjustment(
        float currentHealth,
        float targetHealth) {

        float difference = targetHealth - currentHealth;
        return Clamp(difference * 0.3f, -0.2f, 0.2f); // Max ±20% adjustment
    }

    // ========================================================================
    // Military Calculations
    // ========================================================================

    float NationAICalculator::CalculateMilitaryReadiness(
        int totalForces,
        int provinceCount,
        int recommendedForcesPerProvince) {

        int recommendedForces = provinceCount * recommendedForcesPerProvince;
        if (recommendedForces == 0) return 1.0f;

        return Clamp(static_cast<float>(totalForces) / recommendedForces, 0.0f, 2.0f);
    }

    int NationAICalculator::CalculateRequiredForces(
        StrategicGoal goal,
        int provinceCount,
        float threatLevel) {

        int baseForcesPerProvince = 200;

        switch (goal) {
        case StrategicGoal::EXPANSION:
            baseForcesPerProvince = 300; // Offensive needs more
            break;
        case StrategicGoal::SURVIVAL:
            baseForcesPerProvince = 250; // Defensive
            break;
        case StrategicGoal::CONSOLIDATION:
            baseForcesPerProvince = 150; // Peacetime
            break;
        default:
            baseForcesPerProvince = 200;
            break;
        }

        // Adjust for threat
        baseForcesPerProvince += static_cast<int>(threatLevel * 100);

        return provinceCount * baseForcesPerProvince;
    }

    MilitaryDecision::MilitaryAction NationAICalculator::DetermineMilitaryAction(
        float readiness,
        StrategicGoal primaryGoal,
        float treasury) {

        if (readiness < 0.5f) {
            // Insufficient forces
            if (treasury > 5000.0f) {
                return MilitaryDecision::HIRE_MERCENARIES;
            } else {
                return MilitaryDecision::RAISE_LEVIES;
            }
        } else if (readiness > 1.5f && primaryGoal != StrategicGoal::EXPANSION) {
            // Overstaffed - reduce costs
            return MilitaryDecision::DISMISS_TROOPS;
        } else if (primaryGoal == StrategicGoal::CONSOLIDATION) {
            return MilitaryDecision::BUILD_FORTIFICATIONS;
        }

        return MilitaryDecision::TRAIN_TROOPS; // Default improvement
    }

    // ========================================================================
    // Diplomatic Calculations
    // ========================================================================

    float NationAICalculator::CalculateRelationshipScore(
        float opinion,
        bool hasAlliance,
        bool atWar,
        bool hasTradeAgreement) {

        float score = opinion / 100.0f; // Normalize to -1 to 1

        if (hasAlliance) score += 0.5f;
        if (atWar) score -= 1.0f;
        if (hasTradeAgreement) score += 0.2f;

        return Clamp(score, -2.0f, 2.0f);
    }

    DiplomaticDecision::DiplomaticAction NationAICalculator::DetermineDiplomaticAction(
        float relationshipScore,
        ThreatLevel threat,
        StrategicGoal primaryGoal) {

        if (relationshipScore > 0.7f) {
            return DiplomaticDecision::FORM_ALLIANCE;
        } else if (relationshipScore < -0.5f) {
            if (threat >= ThreatLevel::MODERATE) {
                return DiplomaticDecision::DENOUNCE;
            }
        } else if (relationshipScore > 0.3f) {
            if (primaryGoal == StrategicGoal::ECONOMIC_GROWTH ||
                primaryGoal == StrategicGoal::DIPLOMATIC_DOMINANCE) {
                return DiplomaticDecision::TRADE_AGREEMENT;
            }
        }

        return DiplomaticDecision::IMPROVE_RELATIONS;
    }

    float NationAICalculator::CalculateAllianceValue(
        float theirMilitaryStrength,
        float relationshipScore,
        float sharedThreatLevel) {

        float value = 0.3f;

        // Strong allies are valuable
        value += theirMilitaryStrength * 0.4f;

        // Good relations make alliance reliable
        value += relationshipScore * 0.3f;

        // Shared threats make alliances urgent
        value += sharedThreatLevel * 0.3f;

        return Clamp(value, 0.0f, 1.0f);
    }

    // ========================================================================
    // Personality Adjustments
    // ========================================================================

    float NationAICalculator::AdjustAggressiveness(
        float currentAggressiveness,
        float stability,
        float economicHealth,
        int recentWars) {

        float adjusted = currentAggressiveness;

        // Low stability reduces aggression
        if (stability < 0.4f) adjusted *= 0.7f;

        // Poor economy reduces aggression
        if (economicHealth < 0.3f) adjusted *= 0.6f;

        // Recent wars fatigue reduces aggression
        adjusted *= (1.0f - recentWars * 0.1f);

        return Clamp(adjusted, 0.1f, 1.0f);
    }

    float NationAICalculator::AdjustRiskTolerance(
        float currentRiskTolerance,
        float treasury,
        int threatCount) {

        float adjusted = currentRiskTolerance;

        // Wealthy nations can afford risks
        if (treasury > 10000.0f) adjusted *= 1.2f;
        else if (treasury < 1000.0f) adjusted *= 0.6f;

        // Many threats reduce risk tolerance
        adjusted *= (1.0f - threatCount * 0.15f);

        return Clamp(adjusted, 0.1f, 1.0f);
    }

    // ========================================================================
    // Decision Scoring
    // ========================================================================

    float NationAICalculator::CalculateDecisionPriority(
        float desirability,
        float urgency,
        float expectedBenefit,
        float cost) {

        float priority = desirability * 0.3f +
                        urgency * 0.3f +
                        expectedBenefit * 0.3f;

        // Cost reduces priority
        if (cost > 0.0f) {
            priority *= (1.0f - cost * 0.1f);
        }

        return NormalizePriority(priority);
    }

    float NationAICalculator::NormalizePriority(float priority) {
        return Clamp(priority, 0.0f, 1.0f);
    }

    // ========================================================================
    // Utility Functions
    // ========================================================================

    float NationAICalculator::Clamp(float value, float min_val, float max_val) {
        return std::max(min_val, std::min(value, max_val));
    }

    float NationAICalculator::Lerp(float a, float b, float t) {
        return a + (b - a) * Clamp(t, 0.0f, 1.0f);
    }

    bool NationAICalculator::IsCritical(float value, float threshold) {
        return value < threshold;
    }

} // namespace ai
} // namespace game
