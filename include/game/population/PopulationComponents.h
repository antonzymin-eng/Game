// ============================================================================
// PopulationComponents.h - ECS Components for Population System
// Created: October 11, 2025 - ECS Integration Test
// Location: include/game/population/PopulationComponents.h
// ============================================================================

#pragma once

#include "core/ECS/IComponent.h"
#include "game/population/PopulationTypes.h"
#include "game/population/PopulationEvents.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <chrono>

namespace game::population {

    // ============================================================================
    // Population Component - Manages population groups for a province/settlement
    // ============================================================================

    struct PopulationComponent : public game::core::Component<PopulationComponent> {
        std::vector<PopulationGroup> population_groups;
        
        // Aggregate statistics (matches factory expectations)
        int total_population = 0;
        int total_children = 0;
        int total_adults = 0;
        int total_elderly = 0;
        int total_males = 0;
        int total_females = 0;

        double average_happiness = 0.5;
        double average_literacy = 0.1;
        double average_wealth = 100.0;
        double average_health = 0.7;
        double overall_employment_rate = 0.0;

        int total_military_eligible = 0;
        double average_military_quality = 0.5;
        int total_military_service_obligation = 0;

        std::unordered_map<std::string, int> culture_distribution;
        std::unordered_map<std::string, int> religion_distribution;
        std::unordered_map<SocialClass, int> class_distribution;
        std::unordered_map<LegalStatus, int> legal_status_distribution;
        std::unordered_map<EmploymentType, int> total_employment;

        int productive_workers = 0;
        int non_productive_income = 0;
        int unemployed_seeking = 0;
        int unemployable = 0;
        int dependents = 0;

        double total_tax_revenue_potential = 0.0;
        double total_feudal_service_days = 0.0;
        double guild_membership_percentage = 0.0;
        double social_mobility_average = 0.005;
        double cultural_assimilation_rate = 0.02;
        double religious_conversion_rate = 0.01;
        double inter_class_tension = 0.0;

        // Historical tracking
        std::chrono::steady_clock::time_point last_update;
        std::vector<PopulationUpdateEvent> historical_events;

        // Additional fields for ECS integration
        double population_density = 0.0;
        double growth_rate = 0.0;
        double birth_rate_average = 0.035;
        double death_rate_average = 0.030;
        double migration_net_rate = 0.0;

        // Component interface
        std::string GetComponentTypeName() const override;
        void Reset();
    };

    // ============================================================================
    // Settlement Component - Manages settlement data for urban areas
    // ============================================================================

    struct SettlementComponent : public game::core::Component<SettlementComponent> {
        // Core settlement data (matches factory expectations)
        std::vector<Settlement> settlements;
        std::unordered_map<SettlementType, int> settlement_counts;

        double total_production_value = 0.0;
        double total_consumption_value = 0.0;
        double trade_income_total = 0.0;
        double total_market_importance = 0.0;

        double urbanization_rate = 0.0;
        double average_infrastructure = 0.5;
        double average_fortification = 0.0;
        double average_sanitation = 0.3;
        double average_prosperity = 0.5;

        int total_garrison_size = 0;
        double cultural_diversity_index = 0.0;
        double religious_diversity_index = 0.0;
        double average_cultural_tolerance = 0.5;
        double average_religious_tolerance = 0.5;

        double average_administrative_efficiency = 0.5;
        double average_autonomy_level = 0.3;
        double average_tax_burden = 0.15;

        int military_settlements = 0;
        int economic_settlements = 0;
        int religious_settlements = 0;
        int administrative_settlements = 0;

                // Component interface
        std::string GetComponentTypeName() const override;
        void Reset();
    };

    // ============================================================================
    // Population Events Component - Manages population-related events
    // ============================================================================

    struct PopulationEventsComponent : public game::core::Component<PopulationEventsComponent> {
        std::vector<MigrationEvent> pending_migrations;
        std::vector<SocialMobilityEvent> pending_social_changes;
        std::vector<LegalStatusChangeEvent> pending_legal_changes;
        std::vector<EmploymentShiftEvent> pending_employment_changes;

        // Event processing state
        std::chrono::system_clock::time_point last_processed;
        int events_processed_this_cycle = 0;
        double event_processing_backlog = 0.0;

        // Component interface
        std::string GetComponentTypeName() const override;
        void Reset();
    };

} // namespace game::population