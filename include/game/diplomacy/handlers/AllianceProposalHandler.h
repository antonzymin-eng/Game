// ============================================================================
// AllianceProposalHandler.h - Alliance Proposal Handler
// Created: 2025-10-28
// ============================================================================

#pragma once

#include "game/diplomacy/handlers/IDiplomaticActionHandler.h"
#include "game/diplomacy/DiplomacyRepository.h"
#include "game/diplomacy/DiplomaticCalculator.h"

namespace game::diplomacy {

/// Handler for alliance proposals and formation
class AllianceProposalHandler : public BaseDiplomaticHandler {
public:
    AllianceProposalHandler(
        DiplomacyRepository& repository,
        DiplomaticCalculator& calculator
    ) : BaseDiplomaticHandler(repository, calculator) {}

    DiplomaticActionResult Execute(
        types::EntityID initiator,
        types::EntityID target,
        const std::unordered_map<std::string, double>& parameters = {}
    ) override;

    bool Validate(
        types::EntityID initiator,
        types::EntityID target
    ) const override;

    std::string GetValidationFailureReason(
        types::EntityID initiator,
        types::EntityID target
    ) const override;

    std::string GetActionName() const override { return "Alliance Proposal"; }

private:
    /// Establish alliance between two realms
    bool EstablishAlliance(types::EntityID realm1, types::EntityID realm2);

    /// Check if alliance is beneficial
    bool IsAllianceBeneficial(
        const DiplomacyComponent& proposer,
        const DiplomacyComponent& target
    ) const;
};

} // namespace game::diplomacy
