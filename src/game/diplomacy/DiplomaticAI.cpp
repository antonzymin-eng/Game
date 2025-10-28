// ============================================================================
// DiplomaticAI.cpp - Implementation
// Created: 2025-10-28
// ============================================================================

#include "game/diplomacy/DiplomaticAI.h"
#include "core/logging/Logger.h"
#include <algorithm>

namespace game::diplomacy {

DiplomaticAI::DiplomaticAI(
    DiplomacyRepository& repository,
    DiplomaticCalculator& calculator)
    : m_repository(repository)
    , m_calculator(calculator) {}

std::vector<AIDecision> DiplomaticAI::EvaluateDiplomaticOptions(types::EntityID realm_id) {
    std::vector<AIDecision> decisions;

    auto diplomacy = m_repository.Get(realm_id);
    if (!diplomacy) {
        return decisions;
    }

    // Get all other realms
    auto all_realms = m_repository.GetAllRealms();

    // Evaluate alliance opportunities
    if (NeedsAlliances(*diplomacy)) {
        auto alliance_candidates = GetAllianceCandidates(realm_id, 3);
        for (auto candidate : alliance_candidates) {
            if (ShouldProposeAlliance(realm_id, candidate)) {
                AIDecision decision;
                decision.action = AIDecision::ActionType::PROPOSE_ALLIANCE;
                decision.target = candidate;
                decision.priority = 0.7;
                decision.reasoning = "Strategic alliance opportunity";
                decisions.push_back(decision);
            }
        }
    }

    // Evaluate war opportunities (if not overextended)
    if (!IsOverextendedInWar(*diplomacy)) {
        auto war_targets = GetWarTargets(realm_id, 2);
        for (auto target : war_targets) {
            if (ShouldDeclareWar(realm_id, target)) {
                AIDecision decision;
                decision.action = AIDecision::ActionType::DECLARE_WAR;
                decision.target = target;
                decision.priority = 0.6;
                decision.reasoning = "Favorable war opportunity";
                decisions.push_back(decision);
            }
        }
    }

    // Sort by priority
    std::sort(decisions.begin(), decisions.end(),
        [](const AIDecision& a, const AIDecision& b) {
            return a.priority > b.priority;
        });

    return decisions;
}

bool DiplomaticAI::ShouldProposeAlliance(
    types::EntityID realm_id,
    types::EntityID candidate) const {

    auto realm_diplomacy = m_repository.Get(realm_id);
    auto candidate_diplomacy = m_repository.Get(candidate);

    if (!realm_diplomacy || !candidate_diplomacy) {
        return false;
    }

    // Already allied
    if (realm_diplomacy->IsAlliedWith(candidate)) {
        return false;
    }

    // At war
    if (realm_diplomacy->IsAtWarWith(candidate)) {
        return false;
    }

    // Calculate desirability
    double desirability = CalculateAllianceDesirability(*realm_diplomacy, *candidate_diplomacy);

    // Personality affects threshold
    double threshold = 0.5;
    double alliance_preference = m_calculator.GetPersonalityAlliancePreference(realm_diplomacy->personality);
    threshold = 0.7 - (alliance_preference * 0.4);

    return desirability > threshold;
}

bool DiplomaticAI::ShouldDeclareWar(
    types::EntityID realm_id,
    types::EntityID target) const {

    auto realm_diplomacy = m_repository.Get(realm_id);
    auto target_diplomacy = m_repository.Get(target);

    if (!realm_diplomacy || !target_diplomacy) {
        return false;
    }

    // Already at war
    if (realm_diplomacy->IsAtWarWith(target)) {
        return false;
    }

    // Allied
    if (realm_diplomacy->IsAlliedWith(target)) {
        return false;
    }

    // Get current opinion
    auto relationship = realm_diplomacy->GetRelationship(target);
    int opinion = relationship ? relationship->opinion : 0;

    // Calculate war likelihood
    double prestige_diff = realm_diplomacy->prestige - target_diplomacy->prestige;
    double war_likelihood = m_calculator.CalculateWarLikelihood(
        *realm_diplomacy, *target_diplomacy, opinion, prestige_diff);

    // High war weariness prevents war
    if (realm_diplomacy->war_weariness > 0.7) {
        return false;
    }

    // Threshold varies by personality
    double threshold = 0.7;
    double personality_war = m_calculator.GetPersonalityWarLikelihood(realm_diplomacy->personality);
    threshold = 0.9 - (personality_war * 0.4);

    return war_likelihood > threshold;
}

bool DiplomaticAI::ShouldProposeTrade(
    types::EntityID realm_id,
    types::EntityID candidate) const {

    auto realm_diplomacy = m_repository.Get(realm_id);
    auto candidate_diplomacy = m_repository.Get(candidate);

    if (!realm_diplomacy || !candidate_diplomacy) {
        return false;
    }

    // At war - no trade
    if (realm_diplomacy->IsAtWarWith(candidate)) {
        return false;
    }

    double trade_value = CalculateTradeValue(*realm_diplomacy, *candidate_diplomacy);
    double trade_preference = m_calculator.GetPersonalityTradePreference(realm_diplomacy->personality);

    return trade_value > (0.6 - trade_preference * 0.2);
}

std::vector<types::EntityID> DiplomaticAI::GetAllianceCandidates(
    types::EntityID realm_id,
    int max_count) const {

    auto realm_diplomacy = m_repository.Get(realm_id);
    if (!realm_diplomacy) {
        return {};
    }

    auto all_realms = m_repository.GetAllRealms();

    std::vector<std::pair<types::EntityID, double>> candidates;

    for (auto other_id : all_realms) {
        if (other_id == realm_id) continue;

        auto other_diplomacy = m_repository.Get(other_id);
        if (!other_diplomacy) continue;

        // Skip if already allied or at war
        if (realm_diplomacy->IsAlliedWith(other_id) || realm_diplomacy->IsAtWarWith(other_id)) {
            continue;
        }

        double desirability = CalculateAllianceDesirability(*realm_diplomacy, *other_diplomacy);
        candidates.emplace_back(other_id, desirability);
    }

    // Sort by desirability
    std::sort(candidates.begin(), candidates.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    std::vector<types::EntityID> result;
    for (int i = 0; i < max_count && i < candidates.size(); ++i) {
        result.push_back(candidates[i].first);
    }

    return result;
}

std::vector<types::EntityID> DiplomaticAI::GetWarTargets(
    types::EntityID realm_id,
    int max_count) const {

    auto realm_diplomacy = m_repository.Get(realm_id);
    if (!realm_diplomacy) {
        return {};
    }

    auto all_realms = m_repository.GetAllRealms();

    std::vector<std::pair<types::EntityID, double>> targets;

    for (auto other_id : all_realms) {
        if (other_id == realm_id) continue;

        auto other_diplomacy = m_repository.Get(other_id);
        if (!other_diplomacy) continue;

        // Skip if already at war or allied
        if (realm_diplomacy->IsAtWarWith(other_id) || realm_diplomacy->IsAlliedWith(other_id)) {
            continue;
        }

        double desirability = CalculateWarDesirability(*realm_diplomacy, *other_diplomacy);
        targets.emplace_back(other_id, desirability);
    }

    std::sort(targets.begin(), targets.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    std::vector<types::EntityID> result;
    for (int i = 0; i < max_count && i < targets.size(); ++i) {
        result.push_back(targets[i].first);
    }

    return result;
}

// Private helper methods

double DiplomaticAI::CalculateAllianceDesirability(
    const DiplomacyComponent& evaluator,
    const DiplomacyComponent& candidate) const {

    return m_calculator.CalculateAllianceValue(evaluator, candidate);
}

double DiplomaticAI::CalculateWarDesirability(
    const DiplomacyComponent& aggressor,
    const DiplomacyComponent& target) const {

    double desirability = 0.5;

    // Prestige difference affects desirability
    double prestige_diff = aggressor.prestige - target.prestige;
    if (prestige_diff > 0) {
        desirability += 0.2; // Stronger realms more likely to attack
    } else {
        desirability -= 0.2;
    }

    // War weariness reduces desirability
    desirability -= aggressor.war_weariness * 0.5;

    // Personality affects base desirability
    double personality_factor = m_calculator.GetPersonalityWarLikelihood(aggressor.personality);
    desirability = (desirability + personality_factor) / 2.0;

    return std::clamp(desirability, 0.0, 1.0);
}

double DiplomaticAI::CalculateTradeValue(
    const DiplomacyComponent& evaluator,
    const DiplomacyComponent& partner) const {

    double value = 0.5;

    // Positive opinion increases trade value
    auto relationship = evaluator.GetRelationship(partner.realm_id);
    if (relationship && relationship->opinion > 0) {
        value += relationship->opinion / 200.0;
    }

    // High trust increases value
    if (relationship) {
        value += relationship->trust * 0.2;
    }

    return std::clamp(value, 0.0, 1.0);
}

bool DiplomaticAI::NeedsAlliances(const DiplomacyComponent& diplomacy) const {
    // Count current alliances
    int alliance_count = 0;
    for (const auto& treaty : diplomacy.active_treaties) {
        if (treaty.type == TreatyType::ALLIANCE && treaty.is_active) {
            alliance_count++;
        }
    }

    // Needs alliances if has fewer than 3
    return alliance_count < 3;
}

bool DiplomaticAI::IsOverextendedInWar(const DiplomacyComponent& diplomacy) const {
    // Count active wars
    int war_count = 0;
    for (const auto& [other_id, state] : diplomacy.relationships) {
        if (state.relation == DiplomaticRelation::AT_WAR) {
            war_count++;
        }
    }

    // Overextended if in more than 2 wars or high war weariness
    return war_count > 2 || diplomacy.war_weariness > 0.6;
}

} // namespace game::diplomacy
