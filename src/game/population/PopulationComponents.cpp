// ============================================================================
// PopulationComponents.cpp - ECS Component Method Implementations
// Created: October 13, 2025 - Following Architecture Database Patterns
// Location: src/game/population/PopulationComponents.cpp
// ============================================================================

#include "game/population/PopulationComponents.h"
#include "core/logging/Logger.h"

namespace game::population {

    // ============================================================================
    // PopulationComponent Methods
    // ============================================================================

    std::string PopulationComponent::GetComponentTypeName() const {
        return "PopulationComponent";
    }

    void PopulationComponent::Reset() {
        population_groups.clear();
        total_population = 0;
        total_children = 0;
        total_adults = 0;
        total_elderly = 0;
        total_males = 0;
        total_females = 0;
        average_happiness = 0.5;
        average_literacy = 0.1;
        average_wealth = 100.0;
        average_health = 0.7;
        overall_employment_rate = 0.0;
        total_military_eligible = 0;
        average_military_quality = 0.5;
        total_military_service_obligation = 0;
        culture_distribution.clear();
        religion_distribution.clear();
        class_distribution.clear();
        legal_status_distribution.clear();
        total_employment.clear();
        productive_workers = 0;
        non_productive_income = 0;
        unemployed_seeking = 0;
        unemployable = 0;
        dependents = 0;
        total_tax_revenue_potential = 0.0;
        total_feudal_service_days = 0.0;
        guild_membership_percentage = 0.0;
        social_mobility_average = 0.005;
        cultural_assimilation_rate = 0.02;
        religious_conversion_rate = 0.01;
        inter_class_tension = 0.0;
        last_update = std::chrono::steady_clock::now();
        historical_events.clear();
        population_density = 0.0;
        growth_rate = 0.0;
        birth_rate_average = 0.035;
        death_rate_average = 0.030;
        migration_net_rate = 0.0;
    }

    // ============================================================================
    // SettlementComponent Methods
    // ============================================================================

    std::string SettlementComponent::GetComponentTypeName() const {
        return "SettlementComponent";
    }

    void SettlementComponent::Reset() {
        settlements.clear();
        settlement_counts.clear();
        total_production_value = 0.0;
        total_consumption_value = 0.0;
        trade_income_total = 0.0;
        total_market_importance = 0.0;
        urbanization_rate = 0.0;
        average_infrastructure = 0.5;
        average_fortification = 0.0;
        average_sanitation = 0.3;
        average_prosperity = 0.5;
        total_garrison_size = 0;
        cultural_diversity_index = 0.0;
        religious_diversity_index = 0.0;
        average_cultural_tolerance = 0.5;
        average_religious_tolerance = 0.5;
        average_administrative_efficiency = 0.5;
        average_autonomy_level = 0.3;
        average_tax_burden = 0.15;
        military_settlements = 0;
        economic_settlements = 0;
        religious_settlements = 0;
        administrative_settlements = 0;
    }

    // ============================================================================
    // PopulationEventsComponent Methods
    // ============================================================================

    std::string PopulationEventsComponent::GetComponentTypeName() const {
        return "PopulationEventsComponent";
    }

    void PopulationEventsComponent::Reset() {
        pending_migrations.clear();
        pending_social_changes.clear();
        pending_legal_changes.clear();
        pending_employment_changes.clear();
        last_processed = std::chrono::system_clock::now();
        events_processed_this_cycle = 0;
        event_processing_backlog = 0.0;
    }

} // namespace game::population