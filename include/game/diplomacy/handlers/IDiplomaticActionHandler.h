// ============================================================================
// IDiplomaticActionHandler.h - Interface for Diplomatic Action Handlers
// Created: 2025-10-28 - Refactoring DiplomacySystem
// Location: include/game/diplomacy/handlers/IDiplomaticActionHandler.h
// ============================================================================

#pragma once

#include "core/types/game_types.h"
#include <string>
#include <unordered_map>
#include <memory>

namespace game::diplomacy {

// Forward declarations
class DiplomacyRepository;
class DiplomaticCalculator;

/// Result of a diplomatic action attempt
struct DiplomaticActionResult {
    bool success = false;
    std::string message;
    int opinion_change = 0;
    double trust_change = 0.0;

    static DiplomaticActionResult Success(const std::string& msg = "Action successful") {
        return DiplomaticActionResult{true, msg, 0, 0.0};
    }

    static DiplomaticActionResult Failure(const std::string& msg) {
        return DiplomaticActionResult{false, msg, 0, 0.0};
    }
};

/// Base interface for all diplomatic action handlers
class IDiplomaticActionHandler {
public:
    virtual ~IDiplomaticActionHandler() = default;

    /// Execute the diplomatic action
    virtual DiplomaticActionResult Execute(
        types::EntityID initiator,
        types::EntityID target,
        const std::unordered_map<std::string, double>& parameters = {}
    ) = 0;

    /// Validate if the action can be performed
    virtual bool Validate(
        types::EntityID initiator,
        types::EntityID target
    ) const = 0;

    /// Get human-readable reason for validation failure
    virtual std::string GetValidationFailureReason(
        types::EntityID initiator,
        types::EntityID target
    ) const = 0;

    /// Get the name of this action type
    virtual std::string GetActionName() const = 0;
};

/// Base class providing common functionality for handlers
class BaseDiplomaticHandler : public IDiplomaticActionHandler {
protected:
    DiplomacyRepository& m_repository;
    DiplomaticCalculator& m_calculator;

public:
    BaseDiplomaticHandler(
        DiplomacyRepository& repository,
        DiplomaticCalculator& calculator
    ) : m_repository(repository), m_calculator(calculator) {}

    virtual ~BaseDiplomaticHandler() = default;

protected:
    /// Common validation checks
    bool ValidateBasicRequirements(
        types::EntityID initiator,
        types::EntityID target,
        std::string& failure_reason
    ) const;

    /// Log diplomatic event
    void LogEvent(
        types::EntityID initiator,
        types::EntityID target,
        const std::string& action,
        const std::string& details = ""
    ) const;
};

} // namespace game::diplomacy
