// ============================================================================
// DiplomaticCalculator.h - Opinion & Trust Calculation Logic
// Created: 2025-10-28 - Refactoring DiplomacySystem
// Location: include/game/diplomacy/DiplomaticCalculator.h
// ============================================================================

#pragma once

#include "game/diplomacy/DiplomacyComponents.h"
#include "core/types/game_types.h"
#include <string>

namespace game::diplomacy {

/// Enumeration of diplomatic actions that affect opinion
enum class DiplomaticAction {
    ALLIANCE_FORMED,
    ALLIANCE_BROKEN,
    WAR_DECLARED,
    PEACE_SIGNED,
    TRADE_AGREEMENT,
    MARRIAGE_ARRANGED,
    GIFT_SENT,
    EMBASSY_ESTABLISHED,
    EMBASSY_RECALLED,
    TREATY_HONORED,
    TREATY_VIOLATED,
    BORDER_INCIDENT,
    INSULT_GIVEN,
    PRAISE_GIVEN
};

/// Incident types that affect trust
enum class DiplomaticIncident {
    TREATY_BREACH,
    BETRAYAL,
    MILITARY_AGGRESSION,
    ESPIONAGE_DISCOVERED,
    HONORING_ALLIANCE,
    KEEPING_PROMISE,
    DIPLOMATIC_SUPPORT,
    TRADE_FULFILLED
};

/// Pure calculation functions for diplomatic relationships
class DiplomaticCalculator {
public:
    // ========================================================================
    // Opinion Calculations
    // ========================================================================

    /// Calculate opinion change from a diplomatic action
    static int CalculateOpinionChange(
        const DiplomaticState& current_state,
        DiplomaticAction action,
        double magnitude = 1.0
    );

    /// Calculate opinion decay over time
    static int CalculateOpinionDecay(
        int current_opinion,
        float time_delta,
        DiplomaticPersonality personality
    );

    /// Calculate base opinion between two realms
    static int CalculateBaseOpinion(
        const DiplomacyComponent& realm1,
        const DiplomacyComponent& realm2
    );

    /// Clamp opinion to valid range [-100, 100]
    static int ClampOpinion(int opinion);

    // ========================================================================
    // Trust Calculations
    // ========================================================================

    /// Calculate trust change from an incident
    static double CalculateTrustChange(
        double current_trust,
        DiplomaticIncident incident
    );

    /// Calculate trust decay over time
    static double CalculateTrustDecay(
        double current_trust,
        int diplomatic_incidents,
        float time_delta
    );

    /// Calculate trust restoration (when no incidents occur)
    static double CalculateTrustRestoration(
        double current_trust,
        float time_delta
    );

    /// Clamp trust to valid range [0.0, 1.0]
    static double ClampTrust(double trust);

    // ========================================================================
    // War Likelihood Calculations
    // ========================================================================

    /// Calculate likelihood of war between two realms [0.0, 1.0]
    static double CalculateWarLikelihood(
        const DiplomacyComponent& aggressor,
        const DiplomacyComponent& target,
        int opinion,
        double prestige_difference
    );

    /// Get base war likelihood from personality
    static double GetPersonalityWarLikelihood(DiplomaticPersonality personality);

    /// Get base trade preference from personality
    static double GetPersonalityTradePreference(DiplomaticPersonality personality);

    /// Get base alliance preference from personality
    static double GetPersonalityAlliancePreference(DiplomaticPersonality personality);

    // ========================================================================
    // Prestige Calculations
    // ========================================================================

    /// Calculate prestige from diplomatic factors
    static double CalculatePrestige(
        const DiplomacyComponent& diplomacy,
        int alliance_count,
        int marriage_count,
        int hostile_count
    );

    /// Calculate prestige impact on opinion
    static int CalculatePrestigeOpinionModifier(double prestige_difference);

    // ========================================================================
    // War Score Calculations
    // ========================================================================

    /// Calculate war score between combatants [0.0, 1.0]
    /// Higher value = first realm winning
    static double CalculateWarScore(
        types::EntityID realm1,
        types::EntityID realm2
    );

    // ========================================================================
    // Treaty Evaluation
    // ========================================================================

    /// Calculate value of an alliance to a realm [0.0, 1.0]
    static double CalculateAllianceValue(
        const DiplomacyComponent& evaluator,
        const DiplomacyComponent& potential_ally
    );

    /// Calculate acceptability of trade terms [0.0, 1.0]
    static double CalculateTradeTermsAcceptability(
        double trade_bonus,
        int current_opinion
    );

    // ========================================================================
    // Utility Functions
    // ========================================================================

    /// Convert diplomatic action to string
    static std::string ActionToString(DiplomaticAction action);

    /// Convert diplomatic incident to string
    static std::string IncidentToString(DiplomaticIncident incident);

    /// Get opinion description from value
    static std::string GetOpinionDescription(int opinion);

    /// Get trust description from value
    static std::string GetTrustDescription(double trust);
};

} // namespace game::diplomacy
