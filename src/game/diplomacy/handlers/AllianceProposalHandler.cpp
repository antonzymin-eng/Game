// ============================================================================
// AllianceProposalHandler.cpp - Implementation
// Created: 2025-10-28
// ============================================================================

#include "game/diplomacy/handlers/AllianceProposalHandler.h"
#include "core/logging/Logger.h"

namespace game::diplomacy {

DiplomaticActionResult AllianceProposalHandler::Execute(
    types::EntityID initiator,
    types::EntityID target,
    const std::unordered_map<std::string, double>& parameters) {

    if (!Validate(initiator, target)) {
        return DiplomaticActionResult::Failure(
            GetValidationFailureReason(initiator, target)
        );
    }

    // Get both diplomacy components
    auto pair = m_repository.GetPair(initiator, target);
    if (!pair.both_valid()) {
        return DiplomaticActionResult::Failure("Failed to access diplomacy components");
    }

    // Check if alliance would be beneficial
    if (!IsAllianceBeneficial(*pair.first, *pair.second)) {
        LogEvent(initiator, target, "Alliance Proposal", "Rejected - not beneficial");
        return DiplomaticActionResult::Failure("Alliance proposal rejected - insufficient benefit");
    }

    // Establish the alliance
    if (!EstablishAlliance(initiator, target)) {
        return DiplomaticActionResult::Failure("Failed to establish alliance");
    }

    // Calculate opinion and trust changes
    int opinion_change = m_calculator.CalculateOpinionChange(
        *pair.first->GetRelationship(target),
        DiplomaticAction::ALLIANCE_FORMED
    );

    double trust_change = m_calculator.CalculateTrustChange(
        pair.first->GetRelationship(target)->trust,
        DiplomaticIncident::DIPLOMATIC_SUPPORT
    );

    LogEvent(initiator, target, "Alliance Formed",
             "Opinion: +" + std::to_string(opinion_change) +
             ", Trust: +" + std::to_string(trust_change));

    auto result = DiplomaticActionResult::Success("Alliance successfully formed");
    result.opinion_change = opinion_change;
    result.trust_change = trust_change;

    return result;
}

bool AllianceProposalHandler::Validate(
    types::EntityID initiator,
    types::EntityID target) const {

    std::string failure_reason;
    if (!ValidateBasicRequirements(initiator, target, failure_reason)) {
        return false;
    }

    auto pair = m_repository.GetPair(initiator, target);
    if (!pair.both_valid()) {
        return false;
    }

    // Check if already allied
    if (pair.first->IsAlliedWith(target)) {
        return false;
    }

    // Check if at war
    if (pair.first->IsAtWarWith(target)) {
        return false;
    }

    return true;
}

std::string AllianceProposalHandler::GetValidationFailureReason(
    types::EntityID initiator,
    types::EntityID target) const {

    std::string failure_reason;
    if (!ValidateBasicRequirements(initiator, target, failure_reason)) {
        return failure_reason;
    }

    auto pair = m_repository.GetPair(initiator, target);
    if (!pair.both_valid()) {
        return "Cannot access diplomacy components";
    }

    if (pair.first->IsAlliedWith(target)) {
        return "Realms are already allied";
    }

    if (pair.first->IsAtWarWith(target)) {
        return "Cannot ally with realm at war";
    }

    return "Unknown validation failure";
}

bool AllianceProposalHandler::EstablishAlliance(types::EntityID realm1, types::EntityID realm2) {
    auto pair = m_repository.GetPair(realm1, realm2);
    if (!pair.both_valid()) {
        return false;
    }

    // Set alliance relation
    pair.first->SetRelation(realm2, DiplomaticRelation::ALLIED);
    pair.second->SetRelation(realm1, DiplomaticRelation::ALLIED);

    // Modify opinions positively
    pair.first->ModifyOpinion(realm2, 20, "Alliance formed");
    pair.second->ModifyOpinion(realm1, 20, "Alliance formed");

    // Create alliance treaty
    Treaty alliance_treaty(TreatyType::ALLIANCE, realm1, realm2);
    alliance_treaty.signed_date = std::chrono::system_clock::now();
    alliance_treaty.expiry_date = alliance_treaty.signed_date + std::chrono::hours(8760 * 10); // 10 years
    alliance_treaty.is_active = true;

    pair.first->AddTreaty(alliance_treaty);
    pair.second->AddTreaty(alliance_treaty);

    ::core::logging::LogInfo("AllianceHandler",
        "Alliance established between " + std::to_string(realm1) +
        " and " + std::to_string(realm2));

    return true;
}

bool AllianceProposalHandler::IsAllianceBeneficial(
    const DiplomacyComponent& proposer,
    const DiplomacyComponent& target) const {

    // Calculate alliance value
    double alliance_value = m_calculator.CalculateAllianceValue(proposer, target);

    // Alliance is beneficial if value > 0.4 (out of 1.0)
    return alliance_value > 0.4;
}

} // namespace game::diplomacy
