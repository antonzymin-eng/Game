// ============================================================================
// DiplomaticCalculator.cpp - Implementation
// Created: 2025-10-28
// ============================================================================

#include "game/diplomacy/DiplomaticCalculator.h"
#include <algorithm>
#include <cmath>

namespace game::diplomacy {

// ============================================================================
// Opinion Calculations
// ============================================================================

int DiplomaticCalculator::CalculateOpinionChange(
    const DiplomaticState& current_state,
    DiplomaticAction action,
    double magnitude) {

    int base_change = 0;

    switch (action) {
        case DiplomaticAction::ALLIANCE_FORMED:
            base_change = 20;
            break;
        case DiplomaticAction::ALLIANCE_BROKEN:
            base_change = -30;
            break;
        case DiplomaticAction::WAR_DECLARED:
            base_change = -50;
            break;
        case DiplomaticAction::PEACE_SIGNED:
            base_change = 10;
            break;
        case DiplomaticAction::TRADE_AGREEMENT:
            base_change = 5;
            break;
        case DiplomaticAction::MARRIAGE_ARRANGED:
            base_change = 20;
            break;
        case DiplomaticAction::GIFT_SENT:
            base_change = static_cast<int>(5.0 + magnitude * 2.5); // 5-30 based on value
            break;
        case DiplomaticAction::EMBASSY_ESTABLISHED:
            base_change = 10;
            break;
        case DiplomaticAction::EMBASSY_RECALLED:
            base_change = -15;
            break;
        case DiplomaticAction::TREATY_HONORED:
            base_change = 1;
            break;
        case DiplomaticAction::TREATY_VIOLATED:
            base_change = -30;
            break;
        case DiplomaticAction::BORDER_INCIDENT:
            base_change = -5;
            break;
        case DiplomaticAction::INSULT_GIVEN:
            base_change = -10;
            break;
        case DiplomaticAction::PRAISE_GIVEN:
            base_change = 5;
            break;
    }

    // Apply magnitude multiplier
    base_change = static_cast<int>(base_change * magnitude);

    // Trust affects opinion changes (high trust = more positive, low trust = more negative)
    if (base_change > 0) {
        base_change = static_cast<int>(base_change * (0.5 + current_state.trust * 0.5));
    } else if (base_change < 0) {
        base_change = static_cast<int>(base_change * (1.5 - current_state.trust * 0.5));
    }

    return base_change;
}

int DiplomaticCalculator::CalculateOpinionDecay(
    int current_opinion,
    float time_delta,
    DiplomaticPersonality personality) {

    // Base decay rate per time unit
    double decay_rate = 0.1 * time_delta;

    // Personality affects decay rate
    switch (personality) {
        case DiplomaticPersonality::FORGIVING:
            decay_rate *= 1.5; // Forgives faster
            break;
        case DiplomaticPersonality::VENGEFUL:
            decay_rate *= 0.5; // Holds grudges
            break;
        case DiplomaticPersonality::PRAGMATIC:
            decay_rate *= 1.2; // Slightly faster decay
            break;
        default:
            break;
    }

    // Opinions decay toward neutral (0)
    int decay = 0;
    if (current_opinion > 0) {
        decay = -static_cast<int>(std::ceil(decay_rate));
    } else if (current_opinion < 0) {
        decay = static_cast<int>(std::ceil(decay_rate));
    }

    return decay;
}

int DiplomaticCalculator::CalculateBaseOpinion(
    const DiplomacyComponent& realm1,
    const DiplomacyComponent& realm2) {

    int base_opinion = 0;

    // Personality compatibility
    if (realm1.personality == realm2.personality) {
        base_opinion += 10; // Similar personalities like each other
    }

    // Specific personality interactions
    if (realm1.personality == DiplomaticPersonality::AGGRESSIVE &&
        realm2.personality == DiplomaticPersonality::PEACEFUL) {
        base_opinion -= 10; // Aggressive dislikes peaceful
    }

    if (realm1.personality == DiplomaticPersonality::DIPLOMATIC &&
        realm2.personality == DiplomaticPersonality::DIPLOMATIC) {
        base_opinion += 15; // Diplomats like diplomats
    }

    // Prestige difference affects opinion
    double prestige_diff = realm1.prestige - realm2.prestige;
    if (prestige_diff > 50.0) {
        base_opinion += 5; // High prestige respected
    } else if (prestige_diff < -50.0) {
        base_opinion -= 5; // Low prestige looked down upon
    }

    return ClampOpinion(base_opinion);
}

int DiplomaticCalculator::ClampOpinion(int opinion) {
    return std::clamp(opinion, -100, 100);
}

// ============================================================================
// Trust Calculations
// ============================================================================

double DiplomaticCalculator::CalculateTrustChange(
    double current_trust,
    DiplomaticIncident incident) {

    double change = 0.0;

    switch (incident) {
        case DiplomaticIncident::TREATY_BREACH:
            change = -0.3;
            break;
        case DiplomaticIncident::BETRAYAL:
            change = -0.5;
            break;
        case DiplomaticIncident::MILITARY_AGGRESSION:
            change = -0.2;
            break;
        case DiplomaticIncident::ESPIONAGE_DISCOVERED:
            change = -0.15;
            break;
        case DiplomaticIncident::HONORING_ALLIANCE:
            change = 0.1;
            break;
        case DiplomaticIncident::KEEPING_PROMISE:
            change = 0.05;
            break;
        case DiplomaticIncident::DIPLOMATIC_SUPPORT:
            change = 0.08;
            break;
        case DiplomaticIncident::TRADE_FULFILLED:
            change = 0.02;
            break;
    }

    // Trust is harder to gain than lose
    if (change > 0) {
        change *= 0.7;
    }

    return change;
}

double DiplomaticCalculator::CalculateTrustDecay(
    double current_trust,
    int diplomatic_incidents,
    float time_delta) {

    if (diplomatic_incidents > 5) {
        return -0.01 * time_delta;
    }

    return 0.0;
}

double DiplomaticCalculator::CalculateTrustRestoration(
    double current_trust,
    float time_delta) {

    if (current_trust < 1.0) {
        return 0.005 * time_delta; // Slowly rebuild trust
    }

    return 0.0;
}

double DiplomaticCalculator::ClampTrust(double trust) {
    return std::clamp(trust, 0.0, 1.0);
}

// ============================================================================
// War Likelihood Calculations
// ============================================================================

double DiplomaticCalculator::CalculateWarLikelihood(
    const DiplomacyComponent& aggressor,
    const DiplomacyComponent& target,
    int opinion,
    double prestige_difference) {

    double likelihood = 0.0;

    // Base likelihood from personality
    likelihood += GetPersonalityWarLikelihood(aggressor.personality);

    // Opinion strongly affects war likelihood
    if (opinion < -50) {
        likelihood += 0.4; // Very hostile = high war chance
    } else if (opinion < -25) {
        likelihood += 0.2;
    } else if (opinion < 0) {
        likelihood += 0.1;
    } else {
        likelihood -= 0.2; // Positive opinion reduces war chance
    }

    // War weariness reduces likelihood
    likelihood -= aggressor.war_weariness * 0.5;

    // Prestige difference affects likelihood
    if (prestige_difference > 50.0) {
        likelihood += 0.1; // Stronger realms more likely to attack
    } else if (prestige_difference < -50.0) {
        likelihood -= 0.2; // Weaker realms less likely to attack
    }

    return std::clamp(likelihood, 0.0, 1.0);
}

double DiplomaticCalculator::GetPersonalityWarLikelihood(DiplomaticPersonality personality) {
    switch (personality) {
        case DiplomaticPersonality::AGGRESSIVE:
            return 0.6;
        case DiplomaticPersonality::EXPANSIONIST:
            return 0.5;
        case DiplomaticPersonality::MILITARISTIC:
            return 0.55;
        case DiplomaticPersonality::DEFENSIVE:
            return 0.2;
        case DiplomaticPersonality::PEACEFUL:
            return 0.1;
        case DiplomaticPersonality::DIPLOMATIC:
            return 0.15;
        case DiplomaticPersonality::PRAGMATIC:
            return 0.3;
        default:
            return 0.25;
    }
}

double DiplomaticCalculator::GetPersonalityTradePreference(DiplomaticPersonality personality) {
    switch (personality) {
        case DiplomaticPersonality::MERCANTILE:
            return 0.9;
        case DiplomaticPersonality::PRAGMATIC:
            return 0.7;
        case DiplomaticPersonality::DIPLOMATIC:
            return 0.6;
        case DiplomaticPersonality::ISOLATIONIST:
            return 0.2;
        case DiplomaticPersonality::AGGRESSIVE:
            return 0.3;
        default:
            return 0.5;
    }
}

double DiplomaticCalculator::GetPersonalityAlliancePreference(DiplomaticPersonality personality) {
    switch (personality) {
        case DiplomaticPersonality::DIPLOMATIC:
            return 0.9;
        case DiplomaticPersonality::DEFENSIVE:
            return 0.8;
        case DiplomaticPersonality::PRAGMATIC:
            return 0.7;
        case DiplomaticPersonality::ISOLATIONIST:
            return 0.2;
        case DiplomaticPersonality::TREACHEROUS:
            return 0.4;
        default:
            return 0.6;
    }
}

// ============================================================================
// Prestige Calculations
// ============================================================================

double DiplomaticCalculator::CalculatePrestige(
    const DiplomacyComponent& diplomacy,
    int alliance_count,
    int marriage_count,
    int hostile_count) {

    double prestige = diplomacy.diplomatic_reputation * 50.0;

    prestige += alliance_count * 10.0;
    prestige += marriage_count * 15.0;
    prestige -= hostile_count * 5.0;
    prestige -= diplomacy.war_weariness * 25.0;

    return std::max(0.0, prestige);
}

int DiplomaticCalculator::CalculatePrestigeOpinionModifier(double prestige_difference) {
    if (prestige_difference > 50.0) {
        return static_cast<int>(std::min(5.0, prestige_difference / 20.0));
    } else if (prestige_difference < -50.0) {
        return static_cast<int>(std::max(-5.0, prestige_difference / 20.0));
    }
    return 0;
}

// ============================================================================
// War Score Calculations
// ============================================================================

double DiplomaticCalculator::CalculateWarScore(
    types::EntityID realm1,
    types::EntityID realm2) {

    // Simplified war score calculation
    // In a full implementation, this would check:
    // - Territory occupied
    // - Battles won/lost
    // - War goal progress
    // - Time since war started

    // For now, return neutral score
    return 0.5;
}

// ============================================================================
// Treaty Evaluation
// ============================================================================

double DiplomaticCalculator::CalculateAllianceValue(
    const DiplomacyComponent& evaluator,
    const DiplomacyComponent& potential_ally) {

    double value = 0.5; // Base value

    // Prestige of potential ally adds value
    value += (potential_ally.prestige / 200.0);

    // Diplomatic reputation adds value
    value += (potential_ally.diplomatic_reputation - 1.0) * 0.3;

    // War weariness reduces value
    value -= potential_ally.war_weariness * 0.3;

    return std::clamp(value, 0.0, 1.0);
}

double DiplomaticCalculator::CalculateTradeTermsAcceptability(
    double trade_bonus,
    int current_opinion) {

    double acceptability = 0.5;

    // Higher trade bonus = more acceptable
    acceptability += (trade_bonus / 100.0);

    // Positive opinion increases acceptability
    if (current_opinion > 0) {
        acceptability += (current_opinion / 200.0);
    } else {
        acceptability += (current_opinion / 400.0);
    }

    return std::clamp(acceptability, 0.0, 1.0);
}

// ============================================================================
// Utility Functions
// ============================================================================

std::string DiplomaticCalculator::ActionToString(DiplomaticAction action) {
    switch (action) {
        case DiplomaticAction::ALLIANCE_FORMED: return "Alliance Formed";
        case DiplomaticAction::ALLIANCE_BROKEN: return "Alliance Broken";
        case DiplomaticAction::WAR_DECLARED: return "War Declared";
        case DiplomaticAction::PEACE_SIGNED: return "Peace Signed";
        case DiplomaticAction::TRADE_AGREEMENT: return "Trade Agreement";
        case DiplomaticAction::MARRIAGE_ARRANGED: return "Marriage Arranged";
        case DiplomaticAction::GIFT_SENT: return "Gift Sent";
        case DiplomaticAction::EMBASSY_ESTABLISHED: return "Embassy Established";
        case DiplomaticAction::EMBASSY_RECALLED: return "Embassy Recalled";
        case DiplomaticAction::TREATY_HONORED: return "Treaty Honored";
        case DiplomaticAction::TREATY_VIOLATED: return "Treaty Violated";
        case DiplomaticAction::BORDER_INCIDENT: return "Border Incident";
        case DiplomaticAction::INSULT_GIVEN: return "Insult Given";
        case DiplomaticAction::PRAISE_GIVEN: return "Praise Given";
        default: return "Unknown Action";
    }
}

std::string DiplomaticCalculator::IncidentToString(DiplomaticIncident incident) {
    switch (incident) {
        case DiplomaticIncident::TREATY_BREACH: return "Treaty Breach";
        case DiplomaticIncident::BETRAYAL: return "Betrayal";
        case DiplomaticIncident::MILITARY_AGGRESSION: return "Military Aggression";
        case DiplomaticIncident::ESPIONAGE_DISCOVERED: return "Espionage Discovered";
        case DiplomaticIncident::HONORING_ALLIANCE: return "Honoring Alliance";
        case DiplomaticIncident::KEEPING_PROMISE: return "Keeping Promise";
        case DiplomaticIncident::DIPLOMATIC_SUPPORT: return "Diplomatic Support";
        case DiplomaticIncident::TRADE_FULFILLED: return "Trade Fulfilled";
        default: return "Unknown Incident";
    }
}

std::string DiplomaticCalculator::GetOpinionDescription(int opinion) {
    if (opinion >= 75) return "Excellent";
    if (opinion >= 50) return "Very Good";
    if (opinion >= 25) return "Good";
    if (opinion >= 0) return "Neutral";
    if (opinion >= -25) return "Poor";
    if (opinion >= -50) return "Bad";
    if (opinion >= -75) return "Very Bad";
    return "Terrible";
}

std::string DiplomaticCalculator::GetTrustDescription(double trust) {
    if (trust >= 0.9) return "Absolute";
    if (trust >= 0.75) return "High";
    if (trust >= 0.6) return "Moderate";
    if (trust >= 0.4) return "Low";
    if (trust >= 0.2) return "Very Low";
    return "None";
}

} // namespace game::diplomacy
