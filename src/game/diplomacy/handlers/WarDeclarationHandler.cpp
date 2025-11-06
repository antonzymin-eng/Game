// ============================================================================
// WarDeclarationHandler.cpp - Implementation
// Created: 2025-10-28
// ============================================================================

#include "game/diplomacy/handlers/WarDeclarationHandler.h"
#include "core/logging/Logger.h"

namespace game::diplomacy {

DiplomaticActionResult WarDeclarationHandler::Execute(
    types::EntityID initiator,
    types::EntityID target,
    const std::unordered_map<std::string, double>& parameters) {

    if (!Validate(initiator, target)) {
        return DiplomaticActionResult::Failure(GetValidationFailureReason(initiator, target));
    }

    auto pair = m_repository.GetPair(initiator, target);
    if (!pair.both_valid()) {
        return DiplomaticActionResult::Failure("Failed to access diplomacy components");
    }

    // Set war relationship
    pair.first->SetRelation(target, DiplomaticRelation::AT_WAR);
    pair.second->SetRelation(initiator, DiplomaticRelation::AT_WAR);

    // Calculate opinion changes
    int opinion_change = m_calculator.CalculateOpinionChange(
        *pair.first->GetRelationship(target),
        DiplomaticAction::WAR_DECLARED
    );

    // Apply opinion changes
    pair.first->ModifyOpinion(target, opinion_change, "War declaration");
    pair.second->ModifyOpinion(initiator, opinion_change, "War declared on us");

    // Set cooldown to prevent repeated war declarations
    auto* state1 = pair.first->GetRelationship(target);
    auto* state2 = pair.second->GetRelationship(initiator);
    if (state1) {
        state1->SetActionCooldown(DiplomaticAction::DECLARE_WAR);
    }
    if (state2) {
        state2->SetActionCooldown(DiplomaticAction::DECLARE_WAR);
    }

    // Increase war weariness
    pair.first->war_weariness += 0.1;
    pair.second->war_weariness += 0.05;

    LogEvent(initiator, target, "War Declared", "Opinion impact: " + std::to_string(opinion_change));

    auto result = DiplomaticActionResult::Success("War declared successfully");
    result.opinion_change = opinion_change;
    return result;
}

bool WarDeclarationHandler::Validate(types::EntityID initiator, types::EntityID target) const {
    std::string reason;
    if (!ValidateBasicRequirements(initiator, target, reason)) {
        return false;
    }

    // Use const GetPair overload for read-only validation
    auto pair = m_repository.GetPair(initiator, target);
    if (!pair.both_valid()) {
        return false;
    }

    // Cannot declare war if already at war
    if (pair.first->IsAtWarWith(target)) {
        return false;
    }

    // Check cooldown to prevent war spam
    auto* state = pair.first->GetRelationship(target);
    if (state && state->IsActionOnCooldown(DiplomaticAction::DECLARE_WAR)) {
        return false;
    }

    return true;
}

std::string WarDeclarationHandler::GetValidationFailureReason(
    types::EntityID initiator,
    types::EntityID target) const {

    std::string reason;
    if (!ValidateBasicRequirements(initiator, target, reason)) {
        return reason;
    }

    auto pair = m_repository.GetPair(initiator, target);
    if (!pair.both_valid()) {
        return "Cannot access diplomacy components";
    }

    if (pair.first->IsAtWarWith(target)) {
        return "Already at war with target";
    }

    // Check cooldown
    auto* state = pair.first->GetRelationship(target);
    if (state && state->IsActionOnCooldown(DiplomaticAction::DECLARE_WAR)) {
        int days_remaining = state->GetRemainingCooldownDays(DiplomaticAction::DECLARE_WAR);
        return "War declaration on cooldown (" + std::to_string(days_remaining) + " days remaining)";
    }

    return "Unknown validation failure";
}

} // namespace game::diplomacy
