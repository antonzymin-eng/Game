// ============================================================================
// DiplomaticAI.h - AI Decision Making for Diplomacy
// Created: 2025-10-28
// ============================================================================

#pragma once

#include "game/diplomacy/DiplomacyRepository.h"
#include "game/diplomacy/DiplomaticCalculator.h"
#include "core/types/game_types.h"
#include <vector>
#include <string>

namespace game::diplomacy {

// Forward declarations
class IDiplomaticActionHandler;

/// AI decision for diplomatic action
struct AIDecision {
    enum class ActionType {
        PROPOSE_ALLIANCE,
        DECLARE_WAR,
        PROPOSE_TRADE,
        ARRANGE_MARRIAGE,
        ESTABLISH_EMBASSY,
        SEND_GIFT,
        SUE_FOR_PEACE
    };

    ActionType action;
    types::EntityID target;
    double priority;          // 0.0 - 1.0, higher = more urgent
    std::string reasoning;
    std::unordered_map<std::string, double> parameters;
};

/// Diplomatic AI decision-making system
class DiplomaticAI {
public:
    DiplomaticAI(
        DiplomacyRepository& repository,
        DiplomaticCalculator& calculator
    );

    /// Evaluate all diplomatic options for a realm
    std::vector<AIDecision> EvaluateDiplomaticOptions(types::EntityID realm_id);

    /// Check if should propose alliance with candidate
    bool ShouldProposeAlliance(
        types::EntityID realm_id,
        types::EntityID candidate
    ) const;

    /// Check if should declare war on target
    bool ShouldDeclareWar(
        types::EntityID realm_id,
        types::EntityID target
    ) const;

    /// Check if should propose trade agreement
    bool ShouldProposeTrade(
        types::EntityID realm_id,
        types::EntityID candidate
    ) const;

    /// Get best alliance candidates for a realm
    std::vector<types::EntityID> GetAllianceCandidates(
        types::EntityID realm_id,
        int max_count = 3
    ) const;

    /// Get potential war targets
    std::vector<types::EntityID> GetWarTargets(
        types::EntityID realm_id,
        int max_count = 3
    ) const;

private:
    DiplomacyRepository& m_repository;
    DiplomaticCalculator& m_calculator;

    /// Calculate value of alliance with candidate
    double CalculateAllianceDesirability(
        const DiplomacyComponent& evaluator,
        const DiplomacyComponent& candidate
    ) const;

    /// Calculate desirability of war with target
    double CalculateWarDesirability(
        const DiplomacyComponent& aggressor,
        const DiplomacyComponent& target
    ) const;

    /// Calculate trade relationship value
    double CalculateTradeValue(
        const DiplomacyComponent& evaluator,
        const DiplomacyComponent& partner
    ) const;

    /// Check if realm needs more alliances
    bool NeedsAlliances(const DiplomacyComponent& diplomacy) const;

    /// Check if realm is overextended in wars
    bool IsOverextendedInWar(const DiplomacyComponent& diplomacy) const;
};

} // namespace game::diplomacy
