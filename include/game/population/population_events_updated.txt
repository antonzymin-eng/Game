// ============================================================================
// PopulationEvents.h - Population System Event Definitions
// Created: December 19, 2024 at 11:25 AM
// Location: include/game/population/PopulationEvents.h
// ============================================================================

#pragma once

#include "core/ECS/EntityManager.h"
#include "core/ECS/MessageBus.h"
#include "game/population/PopulationTypes.h"
#include <string>
#include <vector>
#include <chrono>
#include <unordered_map>

namespace game::population {

    // ============================================================================
    // Core Population Events
    // ============================================================================

    struct PopulationUpdateEvent {
        EntityID entity_id;
        int total_population;
        double population_growth_rate;
        double average_wealth;
        double average_happiness;
        double average_literacy;
        double average_health;
        int military_eligible;
        double military_quality;

        static std::string GetTypeName() { return "PopulationUpdateEvent"; }
    };

    struct DemographicChangeEvent {
        EntityID entity_id;
        int births;
        int deaths;
        int infant_deaths;
        int maternal_deaths;
        double birth_rate;
        double death_rate;
        SocialClass affected_class;
        int population_affected;

        static std::string GetTypeName() { return "DemographicChangeEvent"; }
    };

    struct HealthCrisisEvent {
        EntityID entity_id;
        std::string crisis_type; // "plague", "famine", "epidemic"
        SocialClass most_affected_class;
        double mortality_increase;
        int population_affected;
        int duration_months;
        double economic_impact;

        static std::string GetTypeName() { return "HealthCrisisEvent"; }
    };

    // ============================================================================
    // Social Mobility Events
    // ============================================================================

    struct SocialMobilityEvent {
        EntityID entity_id;
        SocialClass from_class;
        SocialClass to_class;
        int population_affected;
        bool upward_mobility;
        std::string reason; // "wealth", "education", "political", "marriage", "crisis"
        double wealth_change;
        double happiness_change;

        static std::string GetTypeName() { return "SocialMobilityEvent"; }
    };

    struct LegalStatusChangeEvent {
        EntityID entity_id;
        LegalStatus from_status;
        LegalStatus to_status;
        int population_affected;
        std::string reason; // "manumission", "purchase", "reward", "punishment", "law_change"
        double economic_impact;

        static std::string GetTypeName() { return "LegalStatusChangeEvent"; }
    };

    struct GuildAdvancementEvent {
        EntityID entity_id;
        SocialClass from_class;
        SocialClass to_class;
        int population_affected;
        std::string guild_type; // "craftsmen", "merchants", "scholars"
        double skill_requirement;
        double wealth_requirement;

        static std::string GetTypeName() { return "GuildAdvancementEvent"; }
    };

    // ============================================================================
    // Cultural and Religious Events
    // ============================================================================

    struct CulturalAssimilationEvent {
        EntityID entity_id;
        std::string from_culture;
        std::string to_culture;
        int population_affected;
        double assimilation_rate;
        std::string mechanism; // "intermarriage", "education", "economic", "political"
        double resistance_level;

        static std::string GetTypeName() { return "CulturalAssimilationEvent"; }
    };

    struct ReligiousConversionEvent {
        EntityID entity_id;
        std::string from_religion;
        std::string to_religion;
        int population_affected;
        double conversion_rate;
        std::string mechanism; // "missionary", "political", "economic", "marriage", "persecution"
        bool voluntary;
        double social_tension_change;

        static std::string GetTypeName() { return "ReligiousConversionEvent"; }
    };

    struct CulturalTensionEvent {
        EntityID entity_id;
        std::string primary_culture;
        std::string secondary_culture;
        double tension_level; // 0.0 to 1.0
        std::string cause; // "discrimination", "competition", "persecution", "favoritism"
        int affected_population;
        double stability_impact;
        double economic_impact;

        static std::string GetTypeName() { return "CulturalTensionEvent"; }
    };

    // ============================================================================
    // Economic and Employment Events
    // ============================================================================

    struct EconomicUpdateEvent {
        EntityID entity_id;
        double tax_revenue_potential;
        int productive_workers;
        double unemployment_rate;
        double trade_income;
        double economic_complexity;
        std::string dominant_industry;
        double guild_influence;

        static std::string GetTypeName() { return "EconomicUpdateEvent"; }
    };

    struct EmploymentShiftEvent {
        EntityID entity_id;
        EmploymentType from_employment;
        EmploymentType to_employment;
        int workers_affected;
        SocialClass social_class;
        std::string reason; // "technology", "demand", "crisis", "opportunity", "training"
        double wage_change;
        double productivity_change;

        static std::string GetTypeName() { return "EmploymentShiftEvent"; }
    };

    struct UnemploymentCrisisEvent {
        EntityID entity_id;
        double unemployment_rate;
        int unemployed_count;
        std::vector<SocialClass> most_affected_classes;
        std::string primary_cause; // "economic_downturn", "technological_change", "war", "plague"
        double social_unrest_risk;
        double crime_rate_change;

        static std::string GetTypeName() { return "UnemploymentCrisisEvent"; }
    };

    struct GuildFormationEvent {
        EntityID entity_id;
        std::string guild_name;
        EmploymentType primary_profession;
        int founding_members;
        SocialClass member_class;
        double initial_influence;
        std::vector<std::string> guild_privileges;
        double economic_impact;

        static std::string GetTypeName() { return "GuildFormationEvent"; }
    };

    // ============================================================================
    // Settlement Events
    // ============================================================================

    struct SettlementUpdateEvent {
        EntityID entity_id;
        int total_settlements;
        double urbanization_rate;
        double average_prosperity;
        double total_economic_output;
        int military_settlements;
        int economic_settlements;
        int religious_settlements;
        int administrative_settlements;

        static std::string GetTypeName() { return "SettlementUpdateEvent"; }
    };

    struct SettlementGrowthEvent {
        EntityID entity_id;
        std::string settlement_name;
        SettlementType from_type;
        SettlementType to_type;
        int population_growth;
        double prosperity_increase;
        std::string growth_driver; // "trade", "industry", "military", "religious", "administrative"
        double infrastructure_demand;

        static std::string GetTypeName() { return "SettlementGrowthEvent"; }
    };

    struct UrbanizationEvent {
        EntityID entity_id;
        double old_urbanization_rate;
        double new_urbanization_rate;
        int rural_to_urban_migrants;
        std::vector<SocialClass> migrant_classes;
        std::string migration_driver; // "economic_opportunity", "rural_crisis", "political", "disaster"
        double urban_strain;
        double rural_labor_shortage;

        static std::string GetTypeName() { return "UrbanizationEvent"; }
    };

    struct InfrastructureDevelopmentEvent {
        EntityID entity_id;
        std::string settlement_name;
        std::string infrastructure_type; // "roads", "sanitation", "fortification", "markets"
        double improvement_level;
        double cost;
        int workers_employed;
        double economic_benefit;
        double population_capacity_increase;

        static std::string GetTypeName() { return "InfrastructureDevelopmentEvent"; }
    };

    // ============================================================================
    // Military Events
    // ============================================================================

    struct MilitaryRecruitmentEvent {
        EntityID entity_id;
        int recruits_needed;
        int recruits_available;
        double recruitment_rate;
        std::vector<SocialClass> recruitment_sources;
        double average_quality;
        double recruitment_cost;
        double social_disruption;

        static std::string GetTypeName() { return "MilitaryRecruitmentEvent"; }
    };

    struct MilitaryServiceEvent {
        EntityID entity_id;
        int soldiers_called;
        int days_of_service;
        std::vector<SocialClass> serving_classes;
        double service_burden;
        double economic_impact; // lost productivity
        double veteran_benefits;
        double military_experience_gained;

        static std::string GetTypeName() { return "MilitaryServiceEvent"; }
    };

    struct MilitaryTechnologyEvent {
        EntityID entity_id;
        std::string technology_name;
        double military_effectiveness_change;
        std::vector<SocialClass> equipment_suppliers;
        double equipment_cost;
        int skilled_workers_needed;
        double strategic_advantage;

        static std::string GetTypeName() { return "MilitaryTechnologyEvent"; }
    };

    // ============================================================================
    // Crisis Events
    // ============================================================================

    struct FamineEvent {
        EntityID entity_id;
        double severity; // 0.0 to 1.0
        int duration_months;
        std::vector<SocialClass> most_affected;
        double mortality_increase;
        double health_decrease;
        double social_unrest_increase;
        double economic_disruption;
        int migration_pressure;

        static std::string GetTypeName() { return "FamineEvent"; }
    };

    struct PlagueEvent {
        EntityID entity_id;
        std::string plague_type;
        double infection_rate;
        double mortality_rate;
        int initial_cases;
        std::vector<SocialClass> most_vulnerable;
        double economic_disruption;
        double trade_impact;
        int quarantine_effectiveness;
        double social_change_potential;
        double severity; // Added for compatibility with PopulationSystem

        static std::string GetTypeName() { return "PlagueEvent"; }
    };

    struct SocialUnrestEvent {
        EntityID entity_id;
        std::vector<SocialClass> participating_classes;
        std::string primary_grievance; // "taxation", "food_shortage", "oppression", "religious"
        double unrest_intensity;
        int participants;
        double violence_level;
        double property_damage;
        double authority_response;
        std::vector<std::string> demands;

        static std::string GetTypeName() { return "SocialUnrestEvent"; }
    };

    struct NaturalDisasterEvent {
        EntityID entity_id;
        std::string disaster_type; // "earthquake", "flood", "fire", "drought", "storm"
        double severity;
        int population_affected;
        int casualties;
        double infrastructure_damage;
        double economic_loss;
        int displaced_population;
        double recovery_time_estimate;

        static std::string GetTypeName() { return "NaturalDisasterEvent"; }
    };

    // ============================================================================
    // Administrative Events
    // ============================================================================

    struct TaxationChangeEvent {
        EntityID entity_id;
        std::vector<SocialClass> affected_classes;
        double old_tax_rate;
        double new_tax_rate;
        double revenue_change;
        double happiness_impact;
        std::string taxation_reason; // "war_funding", "infrastructure", "luxury", "crisis"
        double compliance_rate;

        static std::string GetTypeName() { return "TaxationChangeEvent"; }
    };

    struct LegalCodeChangeEvent {
        EntityID entity_id;
        std::string legal_change_type; // "rights_expansion", "restrictions_added", "enforcement_change"
        std::vector<LegalStatus> affected_statuses;
        std::vector<std::string> new_privileges;
        std::vector<std::string> removed_privileges;
        double social_stability_impact;
        double economic_impact;

        static std::string GetTypeName() { return "LegalCodeChangeEvent"; }
    };

    struct AdministrativeReformEvent {
        EntityID entity_id;
        std::string reform_type; // "centralization", "bureaucracy_expansion", "local_autonomy"
        double efficiency_change;
        double cost_change;
        std::vector<SocialClass> benefiting_classes;
        std::vector<SocialClass> disadvantaged_classes;
        double implementation_time_months;

        static std::string GetTypeName() { return "AdministrativeReformEvent"; }
    };

    // ============================================================================
    // Specialized Event Categories
    // ============================================================================

    struct MigrationEvent {
        EntityID from_entity;
        EntityID to_entity;
        int migrant_population;
        std::vector<SocialClass> migrant_classes;
        std::string migration_type; // "economic", "refugee", "political", "religious", "seasonal"
        double integration_difficulty;
        double economic_opportunity;
        double cultural_adaptation_time;

        static std::string GetTypeName() { return "MigrationEvent"; }
    };

    struct PopulationGrowthEvent {
        EntityID entity_id;
        double old_growth_rate;
        double new_growth_rate;
        int net_population_change;
        std::string growth_driver; // "prosperity", "medical_advances", "peace", "resources"
        std::vector<SocialClass> fastest_growing_classes;
        double carrying_capacity_impact;

        static std::string GetTypeName() { return "PopulationGrowthEvent"; }
    };

    struct EducationalAdvancementEvent {
        EntityID entity_id;
        std::string advancement_type; // "school_founding", "university_establishment", "literacy_campaign"
        double literacy_improvement;
        std::vector<SocialClass> benefiting_classes;
        int students_affected;
        double long_term_economic_impact;
        double cultural_impact;

        static std::string GetTypeName() { return "EducationalAdvancementEvent"; }
    };

    struct TechnologicalAdoptionEvent {
        EntityID entity_id;
        std::string technology_name;
        std::vector<EmploymentType> affected_employments;
        int jobs_created;
        int jobs_eliminated;
        double productivity_change;
        std::vector<SocialClass> adopting_classes;
        double social_resistance_level;

        static std::string GetTypeName() { return "TechnologicalAdoptionEvent"; }
    };

    // ============================================================================
    // Event Aggregation and Analysis
    // ============================================================================

    struct PopulationTrendAnalysis {
        EntityID entity_id;
        std::chrono::steady_clock::time_point analysis_period_start;
        std::chrono::steady_clock::time_point analysis_period_end;

        // Growth and demographic trends
        double population_growth_trend;
        double urbanization_trend;
        double literacy_trend;
        double wealth_distribution_trend;

        // Social mobility trends
        std::unordered_map<SocialClass, double> class_mobility_rates;
        double overall_social_mobility;
        double upward_mobility_rate;
        double downward_mobility_rate;

        // Economic trends
        std::unordered_map<EmploymentType, double> employment_growth_rates;
        double economic_diversification_index;
        double guild_influence_trend;

        // Cultural and social trends
        double cultural_assimilation_rate;
        double religious_conversion_rate;
        double social_stability_trend;
        double inter_class_tension_trend;

        // Warnings and predictions
        std::vector<std::string> demographic_warnings;
        std::vector<std::string> economic_warnings;
        std::vector<std::string> social_warnings;
        std::vector<std::string> predicted_developments;

        static std::string GetTypeName() { return "PopulationTrendAnalysis"; }
    };

    // ============================================================================
    // Event Processing and Handling
    // ============================================================================

    class PopulationEventProcessor {
    public:
        PopulationEventProcessor();
        ~PopulationEventProcessor() = default;

        // Core event processing
        void ProcessEvent(const PopulationUpdateEvent& event);
        void ProcessEvent(const SocialMobilityEvent& event);
        void ProcessEvent(const LegalStatusChangeEvent& event);
        void ProcessEvent(const EmploymentShiftEvent& event);
        void ProcessEvent(const SettlementUpdateEvent& event);
        void ProcessEvent(const EconomicUpdateEvent& event);
        void ProcessEvent(const MilitaryRecruitmentEvent& event);
        void ProcessEvent(const CulturalAssimilationEvent& event);
        void ProcessEvent(const ReligiousConversionEvent& event);
        void ProcessEvent(const GuildAdvancementEvent& event);

        // Crisis event processing
        void ProcessEvent(const FamineEvent& event);
        void ProcessEvent(const PlagueEvent& event);
        void ProcessEvent(const SocialUnrestEvent& event);
        void ProcessEvent(const NaturalDisasterEvent& event);

        // Settlement event processing
        void ProcessEvent(const SettlementGrowthEvent& event);
        void ProcessEvent(const UrbanizationEvent& event);
        void ProcessEvent(const InfrastructureDevelopmentEvent& event);

        // Administrative event processing
        void ProcessEvent(const TaxationChangeEvent& event);
        void ProcessEvent(const LegalCodeChangeEvent& event);
        void ProcessEvent(const AdministrativeReformEvent& event);

        // Migration and population change processing
        void ProcessEvent(const MigrationEvent& event);
        void ProcessEvent(const PopulationGrowthEvent& event);
        void ProcessEvent(const EducationalAdvancementEvent& event);
        void ProcessEvent(const TechnologicalAdoptionEvent& event);

        // Aggregate trend analysis
        PopulationTrendAnalysis AnalyzeTrends(EntityID entity_id,
            const std::vector<PopulationUpdateEvent>& historical_events,
            std::chrono::hours analysis_period);

        // Event validation
        bool ValidateEvent(const PopulationUpdateEvent& event);
        bool ValidateEvent(const SocialMobilityEvent& event);
        bool ValidateEvent(const CrisisEvent& event); // Base crisis validation

        // Event chain processing
        void ProcessEventChain(EntityID entity_id, const std::string& trigger_event);
        std::vector<EntityID> GetAffectedProvinces(const std::string& event_type);

    private:
        // Internal processing helpers
        void LogEvent(const std::string& event_type, EntityID entity_id, const std::string& description);
        void UpdateGameState(EntityID entity_id, const std::string& state_change);
        void TriggerConsequentialEvents(EntityID entity_id, const std::string& trigger_event);
        void CalculateSecondaryEffects(EntityID entity_id, const std::string& primary_event);
        void ValidateEventConsistency(EntityID entity_id);

        // Event storage and tracking
        std::unordered_map<EntityID, std::vector<std::string>> m_active_events;
        std::unordered_map<EntityID, std::chrono::steady_clock::time_point> m_last_processed;
        
        // Processing statistics
        std::unordered_map<std::string, int> m_event_processing_counts;
        std::chrono::steady_clock::time_point m_processor_start_time;
    };

    // ============================================================================
    // Event Formatting and Display
    // ============================================================================

    class PopulationEventFormatter {
    public:
        PopulationEventFormatter() = default;
        ~PopulationEventFormatter() = default;

        // Event formatting methods
        std::string FormatEvent(const PopulationUpdateEvent& event);
        std::string FormatEvent(const SocialMobilityEvent& event);
        std::string FormatEvent(const LegalStatusChangeEvent& event);
        std::string FormatEvent(const EmploymentShiftEvent& event);
        std::string FormatEvent(const SettlementUpdateEvent& event);
        std::string FormatEvent(const EconomicUpdateEvent& event);
        std::string FormatEvent(const CulturalAssimilationEvent& event);
        std::string FormatEvent(const ReligiousConversionEvent& event);
        std::string FormatEvent(const GuildAdvancementEvent& event);

        // Crisis event formatting
        std::string FormatEvent(const FamineEvent& event);
        std::string FormatEvent(const PlagueEvent& event);
        std::string FormatEvent(const SocialUnrestEvent& event);
        std::string FormatEvent(const NaturalDisasterEvent& event);

        // Administrative event formatting
        std::string FormatEvent(const TaxationChangeEvent& event);
        std::string FormatEvent(const AdministrativeReformEvent& event);

        // Migration and growth event formatting
        std::string FormatEvent(const MigrationEvent& event);
        std::string FormatEvent(const PopulationGrowthEvent& event);
        std::string FormatEvent(const EducationalAdvancementEvent& event);

        // Trend analysis formatting
        std::string FormatTrendAnalysis(const PopulationTrendAnalysis& analysis);

        // Utility formatting methods
        std::string FormatPopulationSummary(EntityID entity_id);
        std::string FormatCrisisSummary(EntityID entity_id);
        std::string FormatEventHistory(EntityID entity_id, int max_events = 10);

    private:
        // Helper formatting methods
        std::string FormatSocialClassName(SocialClass social_class);
        std::string FormatLegalStatusName(LegalStatus legal_status);
        std::string FormatEmploymentTypeName(EmploymentType employment_type);
        std::string FormatSettlementTypeName(SettlementType settlement_type);
        std::string FormatPopulationNumber(int population);
        std::string FormatPercentage(double percentage);
        std::string FormatCurrency(double amount);
        std::string GetEventSeverityColor(double severity);
        std::string GetEventIcon(const std::string& event_type);
        std::string FormatTimeStamp(std::chrono::steady_clock::time_point timestamp);
    };

    // ============================================================================
    // Message Bus Integration
    // ============================================================================

    namespace messages {
        
        // Population crisis notifications
        struct PopulationCrisis {
            EntityID province;
            std::string crisis_type;
            double severity;
            int population_affected;
            std::vector<SocialClass> affected_classes;
            
            static std::string GetTypeName() { return "PopulationCrisis"; }
        };

        // Economic impact notifications
        struct PopulationEconomicUpdate {
            EntityID province_id;
            double tax_revenue_potential;
            int productive_workers;
            double unemployment_rate;
            double trade_income;
            
            static std::string GetTypeName() { return "PopulationEconomicUpdate"; }
        };

        // Military recruitment results
        struct MilitaryRecruitmentResult {
            EntityID province_id;
            int requested_recruits;
            int actual_recruits;
            double average_quality;
            double recruitment_cost;
            
            static std::string GetTypeName() { return "MilitaryRecruitmentResult"; }
        };

        // Administrative policy updates
        struct TaxationPolicyUpdate {
            EntityID province_id;
            std::vector<SocialClass> affected_classes;
            double new_tax_rate;
            double expected_revenue;
            double compliance_rate;
            
            static std::string GetTypeName() { return "TaxationPolicyUpdate"; }
        };

        // Settlement evolution notifications
        struct SettlementEvolution {
            EntityID province_id;
            std::string settlement_name;
            SettlementType old_type;
            SettlementType new_type;
            std::string evolution_reason;
            
            static std::string GetTypeName() { return "SettlementEvolution"; }
        };

    } // namespace messages

} // namespace game::population