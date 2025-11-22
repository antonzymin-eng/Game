// ============================================================================
// TechnologyEffectApplicator.cpp - Implementation
// Created: October 31, 2025
// Location: src/game/technology/TechnologyEffectApplicator.cpp
// ============================================================================

#include "game/technology/TechnologyEffectApplicator.h"
#include "game/economy/EconomicComponents.h"
#include <sstream>
#include <iostream>

namespace game::technology {

// ============================================================================
// Main Application Method
// ============================================================================

EffectApplicationResult TechnologyEffectApplicator::ApplyTechnologyEffects(
    core::ecs::EntityManager& entity_manager,
    core::ecs::MessageBus& message_bus,
    game::types::EntityID entity_id,
    TechnologyType technology,
    double implementation_level) {

    EffectApplicationResult result;
    result.success = true;

    // Get all effects for this technology
    auto effects = TechnologyEffectsDatabase::GetEffects(technology);

    if (effects.empty()) {
        result.success = false;
        result.message = "No effects defined for this technology";
        return result;
    }

    // Apply each effect
    for (const auto& effect : effects) {
        bool applied = ApplySpecificEffect(entity_manager, entity_id, effect, implementation_level);

        if (applied) {
            result.applied_effects.push_back(effect.description);

            // Track impact by system
            if (effect.affected_system == "economy") {
                result.total_economic_impact += effect.value * implementation_level;
            } else if (effect.affected_system == "military") {
                result.total_military_impact += effect.value * implementation_level;
            }

            // Send message about applied effect
            TechnologyEffectAppliedMessage msg;
            msg.entity_id = entity_id;
            msg.technology = technology;
            msg.effect_type = effect.type;
            msg.effect_value = effect.value;
            msg.implementation_level = implementation_level;
            msg.system_affected = effect.affected_system;
            message_bus.PublishMessage(msg);

        } else {
            result.failed_effects.push_back(effect.description);
        }
    }

    // Create result message
    std::ostringstream msg;
    msg << "Applied " << result.applied_effects.size() << " effects, "
        << result.failed_effects.size() << " failed";
    result.message = msg.str();

    return result;
}

// ============================================================================
// Specific Effect Application
// ============================================================================

bool TechnologyEffectApplicator::ApplySpecificEffect(
    core::ecs::EntityManager& entity_manager,
    game::types::EntityID entity_id,
    const TechnologyEffect& effect,
    double implementation_level) {

    // Route to appropriate system handler
    if (effect.affected_system == "economy") {
        return ApplyEconomicEffect(entity_manager, entity_id, effect, implementation_level);
    } else if (effect.affected_system == "military") {
        return ApplyMilitaryEffect(entity_manager, entity_id, effect, implementation_level);
    } else if (effect.affected_system == "technology") {
        return ApplyTechnologyEffect(entity_manager, entity_id, effect, implementation_level);
    } else if (effect.affected_system == "administration") {
        return ApplyAdministrativeEffect(entity_manager, entity_id, effect, implementation_level);
    } else if (effect.affected_system == "diplomacy") {
        return ApplyDiplomaticEffect(entity_manager, entity_id, effect, implementation_level);
    } else if (effect.affected_system == "population") {
        return ApplyPopulationEffect(entity_manager, entity_id, effect, implementation_level);
    }

    return false;
}

// ============================================================================
// System-Specific Application Methods
// ============================================================================

bool TechnologyEffectApplicator::ApplyEconomicEffect(
    core::ecs::EntityManager& entity_manager,
    game::types::EntityID entity_id,
    const TechnologyEffect& effect,
    double implementation_level) {

    auto econ_comp = entity_manager.GetComponent<game::economy::EconomicComponent>(
        core::ecs::EntityID(entity_id, 1));

    if (!econ_comp) {
        return false;
    }

    double scaled_value = ScaleEffectByImplementation(effect.value, implementation_level);

    switch (effect.type) {
        case EffectType::PRODUCTION_BONUS:
            // Applied through TechnologyEconomicBridge production_efficiency
            // This is tracked for UI display purposes
            break;

        case EffectType::TRADE_EFFICIENCY:
            // Applied through TechnologyEconomicBridge trade_efficiency
            break;

        case EffectType::TAX_EFFICIENCY:
            // Applied through TechnologyEconomicBridge tax_efficiency
            break;

        case EffectType::FOOD_PRODUCTION:
            // Specific food production bonus (additive with production_efficiency)
            // This would integrate with a food system when available
            break;

        case EffectType::INFRASTRUCTURE_QUALITY:
            // Applied through TechnologyEconomicBridge infrastructure_multiplier
            break;

        case EffectType::BUILDING_COST_REDUCTION:
            // Reduces building construction costs
            // Would integrate with building system when available
            break;

        case EffectType::MARKET_ACCESS:
            // Improves access to trade markets
            // Would integrate with trade system when available
            break;

        default:
            return false;
    }

    return true;
}

bool TechnologyEffectApplicator::ApplyMilitaryEffect(
    core::ecs::EntityManager& entity_manager,
    game::types::EntityID entity_id,
    const TechnologyEffect& effect,
    double implementation_level) {

    // Military system integration
    auto military_comp = entity_manager.GetComponent<game::military::MilitaryComponent>(
        core::ecs::EntityID(entity_id, 1));

    double scaled_value = ScaleEffectByImplementation(effect.value, implementation_level);

    switch (effect.type) {
        case EffectType::MILITARY_STRENGTH:
            // Increases unit combat effectiveness
            if (military_comp) {
                // Apply attack strength bonus to all units
                for (auto& [unit_id, unit] : military_comp->units) {
                    unit.attack_strength *= (1.0 + scaled_value);
                }
                return true;
            }
            break;

        case EffectType::MILITARY_DEFENSE:
            // Increases defensive capabilities
            if (military_comp) {
                // Apply defense strength bonus to all units
                for (auto& [unit_id, unit] : military_comp->units) {
                    unit.defense_strength *= (1.0 + scaled_value);
                }
                return true;
            }
            break;

        case EffectType::MILITARY_MAINTENANCE:
            // Applied through TechnologyEconomicBridge military_maintenance_efficiency
            // Also directly reduce maintenance costs for units
            if (military_comp) {
                for (auto& [unit_id, unit] : military_comp->units) {
                    unit.monthly_maintenance *= (1.0 - scaled_value);
                }
                return true;
            }
            break;

        case EffectType::FORTIFICATION_STRENGTH:
            // Applied through TechnologyEconomicBridge fortification_cost_modifier
            // Also boost fortification defense values
            if (military_comp) {
                // Apply fortification defense bonus
                military_comp->fortification_defense_bonus += scaled_value;
                return true;
            }
            break;

        case EffectType::NAVAL_STRENGTH:
            // Increases naval power
            if (military_comp) {
                // Apply naval combat bonus to naval units
                for (auto& [unit_id, unit] : military_comp->units) {
                    if (unit.unit_class == game::military::UnitClass::NAVAL) {
                        unit.attack_strength *= (1.0 + scaled_value);
                        unit.defense_strength *= (1.0 + scaled_value);
                    }
                }
                return true;
            }
            break;

        case EffectType::UNIT_COST_REDUCTION:
            // Reduces unit recruitment and maintenance costs
            if (military_comp) {
                for (auto& [unit_id, unit] : military_comp->units) {
                    unit.recruitment_cost *= (1.0 - scaled_value);
                    unit.monthly_maintenance *= (1.0 - scaled_value);
                }
                return true;
            }
            break;

        default:
            return false;
    }

    return false;
}

bool TechnologyEffectApplicator::ApplyTechnologyEffect(
    core::ecs::EntityManager& entity_manager,
    game::types::EntityID entity_id,
    const TechnologyEffect& effect,
    double implementation_level) {

    auto research_comp = entity_manager.GetComponent<ResearchComponent>(
        core::ecs::EntityID(entity_id, 1));
    auto innovation_comp = entity_manager.GetComponent<InnovationComponent>(
        core::ecs::EntityID(entity_id, 1));
    auto knowledge_comp = entity_manager.GetComponent<KnowledgeComponent>(
        core::ecs::EntityID(entity_id, 1));

    double scaled_value = ScaleEffectByImplementation(effect.value, implementation_level);

    switch (effect.type) {
        case EffectType::RESEARCH_SPEED:
            if (research_comp) {
                research_comp->base_research_efficiency *= (1.0 + scaled_value);
                return true;
            }
            break;

        case EffectType::INNOVATION_RATE:
            if (innovation_comp) {
                innovation_comp->innovation_rate *= (1.0 + scaled_value);
                return true;
            }
            break;

        case EffectType::KNOWLEDGE_TRANSMISSION:
            if (knowledge_comp) {
                knowledge_comp->knowledge_transmission_rate *= (1.0 + scaled_value);
                return true;
            }
            break;

        default:
            return false;
    }

    return false;
}

bool TechnologyEffectApplicator::ApplyAdministrativeEffect(
    core::ecs::EntityManager& entity_manager,
    game::types::EntityID entity_id,
    const TechnologyEffect& effect,
    double implementation_level) {

    // Administrative system integration
    // When administrative components are available, apply effects here

    double scaled_value = ScaleEffectByImplementation(effect.value, implementation_level);

    switch (effect.type) {
        case EffectType::ADMINISTRATIVE_CAPACITY:
            // Increases province management efficiency
            // Would integrate with administrative system
            break;

        case EffectType::TAX_EFFICIENCY:
            // Also affects administrative efficiency
            // Applied through economic system
            break;

        default:
            return false;
    }

    return true;
}

bool TechnologyEffectApplicator::ApplyDiplomaticEffect(
    core::ecs::EntityManager& entity_manager,
    game::types::EntityID entity_id,
    const TechnologyEffect& effect,
    double implementation_level) {

    // Diplomatic system integration
    // When diplomatic components are available, apply effects here

    double scaled_value = ScaleEffectByImplementation(effect.value, implementation_level);

    switch (effect.type) {
        case EffectType::DIPLOMATIC_REPUTATION:
            // Improves diplomatic standing with other entities
            // Would integrate with diplomacy system
            break;

        default:
            return false;
    }

    return true;
}

bool TechnologyEffectApplicator::ApplyPopulationEffect(
    core::ecs::EntityManager& entity_manager,
    game::types::EntityID entity_id,
    const TechnologyEffect& effect,
    double implementation_level) {

    // Population system integration
    auto population_comp = entity_manager.GetComponent<game::population::PopulationComponent>(
        core::ecs::EntityID(entity_id, 1));

    double scaled_value = ScaleEffectByImplementation(effect.value, implementation_level);

    switch (effect.type) {
        case EffectType::POPULATION_GROWTH:
            // Increases population growth rate
            if (population_comp) {
                // Apply growth rate bonus
                population_comp->growth_rate += scaled_value;
                // Also increase birth rate while reducing death rate
                population_comp->birth_rate_average *= (1.0 + scaled_value * 0.5);
                population_comp->death_rate_average *= (1.0 - scaled_value * 0.5);
                return true;
            }
            break;

        case EffectType::FOOD_PRODUCTION:
            // Improved agriculture supports larger populations
            if (population_comp) {
                // Reduce death rate from improved nutrition
                population_comp->death_rate_average *= (1.0 - scaled_value * 0.3);
                // Slight increase in growth capacity
                population_comp->growth_rate += scaled_value * 0.5;
                return true;
            }
            break;

        case EffectType::HEALTH_IMPROVEMENT:
            // Better health reduces mortality
            if (population_comp) {
                // Improve average health
                population_comp->average_health = std::min(1.0,
                    population_comp->average_health + scaled_value * 0.2);
                // Reduce death rate
                population_comp->death_rate_average *= (1.0 - scaled_value);
                return true;
            }
            break;

        case EffectType::EDUCATION_QUALITY:
            // Better education increases literacy
            if (population_comp) {
                // Improve average literacy
                population_comp->average_literacy = std::min(1.0,
                    population_comp->average_literacy + scaled_value * 0.1);
                return true;
            }
            break;

        default:
            return false;
    }

    return false;
}

// ============================================================================
// Calculation Methods
// ============================================================================

std::unordered_map<EffectType, double> TechnologyEffectApplicator::CalculateTotalEffects(
    core::ecs::EntityManager& entity_manager,
    game::types::EntityID entity_id) {

    std::unordered_map<EffectType, double> total_effects;

    auto research_comp = entity_manager.GetComponent<ResearchComponent>(
        core::ecs::EntityID(entity_id, 1));

    if (!research_comp) {
        return total_effects;
    }

    // Iterate through all implemented technologies
    for (const auto& [tech_type, state] : research_comp->technology_states) {
        if (state != ResearchState::IMPLEMENTED) {
            continue;
        }

        // Get implementation level
        double impl_level = 1.0;
        auto impl_it = research_comp->implementation_level.find(tech_type);
        if (impl_it != research_comp->implementation_level.end()) {
            impl_level = impl_it->second;
        }

        // Get effects for this technology
        auto effects = TechnologyEffectsDatabase::GetEffects(tech_type);

        // Accumulate effects
        for (const auto& effect : effects) {
            double scaled_value = ScaleEffectByImplementation(effect.value, impl_level);
            total_effects[effect.type] += scaled_value;
        }
    }

    return total_effects;
}

std::string TechnologyEffectApplicator::GetEffectSummary(
    core::ecs::EntityManager& entity_manager,
    game::types::EntityID entity_id) {

    auto total_effects = CalculateTotalEffects(entity_manager, entity_id);

    if (total_effects.empty()) {
        return "No technology effects active";
    }

    std::ostringstream summary;
    summary << "Active Technology Effects:\n";

    for (const auto& [effect_type, value] : total_effects) {
        summary << "  " << EffectTypeToString(effect_type) << ": ";

        // Format value as percentage or modifier
        if (value >= 0) {
            summary << "+";
        }
        summary << static_cast<int>(value * 100) << "%\n";
    }

    return summary.str();
}

} // namespace game::technology
