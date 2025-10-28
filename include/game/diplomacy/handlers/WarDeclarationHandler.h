// ============================================================================
// WarDeclarationHandler.h - War Declaration Handler
// Created: 2025-10-28
// ============================================================================

#pragma once

#include "game/diplomacy/handlers/IDiplomaticActionHandler.h"

namespace game::diplomacy {

class WarDeclarationHandler : public BaseDiplomaticHandler {
public:
    using BaseDiplomaticHandler::BaseDiplomaticHandler;

    DiplomaticActionResult Execute(
        types::EntityID initiator,
        types::EntityID target,
        const std::unordered_map<std::string, double>& parameters = {}
    ) override;

    bool Validate(types::EntityID initiator, types::EntityID target) const override;
    std::string GetValidationFailureReason(types::EntityID initiator, types::EntityID target) const override;
    std::string GetActionName() const override { return "War Declaration"; }
};

} // namespace game::diplomacy
