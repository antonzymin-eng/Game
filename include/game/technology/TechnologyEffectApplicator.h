// ============================================================================
// TechnologyEffectApplicator.h - Applies Technology Effects to Game Systems
// Created: October 31, 2025
// Location: include/game/technology/TechnologyEffectApplicator.h
// ============================================================================

#pragma once

#include "game/technology/TechnologyEffects.h"
#include "game/technology/TechnologyComponents.h"
#include "game/military/MilitaryComponents.h"
#include "game/population/PopulationComponents.h"
#include "core/ECS/EntityManager.h"
#include "core/ECS/MessageBus.h"
#include "core/types/game_types.h"

namespace game::technology {

// ============================================================================
// Technology Effect Application Messages
// ============================================================================

struct TechnologyEffectAppliedMessage : public core::ecs::IMessage {
    game::types::EntityID entity_id;
    TechnologyType technology;
    EffectType effect_type;
    double effect_value;
    double implementation_level;
    std::string system_affected;

    std::type_index GetTypeIndex() const override {
        return typeid(TechnologyEffectAppliedMessage);
    }
};

// ============================================================================
// Effect Application Results
// ============================================================================

struct EffectApplicationResult {
    bool success = false;
    std::string message;
    std::vector<std::string> applied_effects;
    std::vector<std::string> failed_effects;
    double total_economic_impact = 0.0;
    double total_military_impact = 0.0;
};

// ============================================================================
// Technology Effect Applicator
// ============================================================================

class TechnologyEffectApplicator {
public:
    TechnologyEffectApplicator() = default;
    ~TechnologyEffectApplicator() = default;

    // Apply all effects from a technology based on implementation level
    static EffectApplicationResult ApplyTechnologyEffects(
        core::ecs::EntityManager& entity_manager,
        core::ecs::MessageBus& message_bus,
        game::types::EntityID entity_id,
        TechnologyType technology,
        double implementation_level);

    // Apply a specific effect
    static bool ApplySpecificEffect(
        core::ecs::EntityManager& entity_manager,
        game::types::EntityID entity_id,
        const TechnologyEffect& effect,
        double implementation_level);

    // Calculate total effects from all implemented technologies
    static std::unordered_map<EffectType, double> CalculateTotalEffects(
        core::ecs::EntityManager& entity_manager,
        game::types::EntityID entity_id);

    // Get effect summary for UI display
    static std::string GetEffectSummary(
        core::ecs::EntityManager& entity_manager,
        game::types::EntityID entity_id);

private:
    // Apply effects by system
    static bool ApplyEconomicEffect(
        core::ecs::EntityManager& entity_manager,
        game::types::EntityID entity_id,
        const TechnologyEffect& effect,
        double implementation_level);

    static bool ApplyMilitaryEffect(
        core::ecs::EntityManager& entity_manager,
        game::types::EntityID entity_id,
        const TechnologyEffect& effect,
        double implementation_level);

    static bool ApplyTechnologyEffect(
        core::ecs::EntityManager& entity_manager,
        game::types::EntityID entity_id,
        const TechnologyEffect& effect,
        double implementation_level);

    static bool ApplyAdministrativeEffect(
        core::ecs::EntityManager& entity_manager,
        game::types::EntityID entity_id,
        const TechnologyEffect& effect,
        double implementation_level);

    static bool ApplyDiplomaticEffect(
        core::ecs::EntityManager& entity_manager,
        game::types::EntityID entity_id,
        const TechnologyEffect& effect,
        double implementation_level);

    static bool ApplyPopulationEffect(
        core::ecs::EntityManager& entity_manager,
        game::types::EntityID entity_id,
        const TechnologyEffect& effect,
        double implementation_level);

    // Helper functions
    static double ScaleEffectByImplementation(double base_value, double implementation_level);
    static std::string EffectTypeToString(EffectType type);
};

// ============================================================================
// Inline Implementations
// ============================================================================

inline double TechnologyEffectApplicator::ScaleEffectByImplementation(
    double base_value, double implementation_level) {
    // Scale effect value by implementation level (0.0 to 1.0)
    return base_value * implementation_level;
}

inline std::string TechnologyEffectApplicator::EffectTypeToString(EffectType type) {
    switch (type) {
        case EffectType::PRODUCTION_BONUS: return "Production Bonus";
        case EffectType::TRADE_EFFICIENCY: return "Trade Efficiency";
        case EffectType::TAX_EFFICIENCY: return "Tax Efficiency";
        case EffectType::MILITARY_STRENGTH: return "Military Strength";
        case EffectType::MILITARY_DEFENSE: return "Military Defense";
        case EffectType::RESEARCH_SPEED: return "Research Speed";
        case EffectType::POPULATION_GROWTH: return "Population Growth";
        case EffectType::BUILDING_COST_REDUCTION: return "Building Cost Reduction";
        case EffectType::UNIT_COST_REDUCTION: return "Unit Cost Reduction";
        case EffectType::INFRASTRUCTURE_QUALITY: return "Infrastructure Quality";
        case EffectType::DIPLOMATIC_REPUTATION: return "Diplomatic Reputation";
        case EffectType::KNOWLEDGE_TRANSMISSION: return "Knowledge Transmission";
        case EffectType::INNOVATION_RATE: return "Innovation Rate";
        case EffectType::FOOD_PRODUCTION: return "Food Production";
        case EffectType::MILITARY_MAINTENANCE: return "Military Maintenance";
        case EffectType::FORTIFICATION_STRENGTH: return "Fortification Strength";
        case EffectType::NAVAL_STRENGTH: return "Naval Strength";
        case EffectType::MARKET_ACCESS: return "Market Access";
        case EffectType::ADMINISTRATIVE_CAPACITY: return "Administrative Capacity";
        default: return "Unknown Effect";
    }
}

} // namespace game::technology
