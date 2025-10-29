// ============================================================================
// BaseDiplomaticHandler.cpp - Base Handler Implementation
// Created: 2025-10-28
// ============================================================================

#include "game/diplomacy/handlers/IDiplomaticActionHandler.h"
#include "game/diplomacy/DiplomacyRepository.h"
#include "game/diplomacy/DiplomaticCalculator.h"
#include "core/logging/Logger.h"

namespace game::diplomacy {

bool BaseDiplomaticHandler::ValidateBasicRequirements(
    types::EntityID initiator,
    types::EntityID target,
    std::string& failure_reason) const {

    // Check if initiator and target are different
    if (initiator == target) {
        failure_reason = "Cannot perform diplomatic action with self";
        return false;
    }

    // Check if both realms have diplomacy components
    auto initiator_diplomacy = m_repository.Get(initiator);
    auto target_diplomacy = m_repository.Get(target);

    if (!initiator_diplomacy) {
        failure_reason = "Initiator realm has no diplomacy component";
        return false;
    }

    if (!target_diplomacy) {
        failure_reason = "Target realm has no diplomacy component";
        return false;
    }

    return true;
}

void BaseDiplomaticHandler::LogEvent(
    types::EntityID initiator,
    types::EntityID target,
    const std::string& action,
    const std::string& details) const {

    std::string message = "Diplomatic Action: " + action +
                         " | Initiator: " + std::to_string(initiator) +
                         " | Target: " + std::to_string(target);

    if (!details.empty()) {
        message += " | Details: " + details;
    }

    ::core::logging::LogInfo("DiplomaticHandler", message);
}

} // namespace game::diplomacy
