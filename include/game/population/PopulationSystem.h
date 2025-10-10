// ============================================================================
// PopulationSystem.h - Population Management System
// Created: December 19, 2024 at 10:45 AM
// Location: include/game/population/PopulationSystem.h
// ============================================================================

#pragma once

#include "core/ECS/ComponentAccessManager.h"
#include "core/ECS/MessageBus.h"
#include "core/threading/ThreadedSystemManager.h"
#include "game/population/PopulationTypes.h"
#include "game/population/PopulationEvents.h"
#include <memory>
#include <random>
#include <unordered_map>
#include <vector>
#include <string>
#include <chrono>

namespace game::population {

    // Forward declarations
    struct PopulationComponent;
    struct SettlementComponent;
    class PopulationEventProcessor;
    class PopulationEventFormatter;
    class PopulationAnalyzer;
    class EnhancedPopulationFactory;

    // ============================================================================
    // Core Population Components (ECS)
    // ============================================================================

    struct PopulationComponent {
        std::vector<PopulationGroup> population_groups;
        
        // Aggregate statistics
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

        // Component interface
        static constexpr const char* GetTypeName() { return "PopulationComponent"; }
    };

    struct SettlementComponent {
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
        int total_militia_potential = 0;
        double total_military_importance = 0.0;
        std::vector<std::string> strategic_chokepoints;

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
        static constexpr const char* GetTypeName() { return "SettlementComponent"; }
    };

    // ============================================================================
    // Population System Configuration
    // ============================================================================

    struct PopulationSystemConfig {
        // Update frequencies
        double demographic_update_interval = 0.1; // 10 Hz
        double mobility_update_interval = 1.0;     // 1 Hz
        double settlement_update_interval = 2.0;   // 0.5 Hz

        // Demographic parameters
        double base_birth_rate = 0.035;
        double base_death_rate = 0.030;
        double base_infant_mortality = 0.25;
        double base_maternal_mortality = 0.02;

        // Crisis multipliers
        double plague_death_multiplier = 3.0;
        double famine_death_multiplier = 2.0;
        double war_death_multiplier = 1.5;

        // Social mobility rates
        double base_upward_mobility = 0.005;
        double base_downward_mobility = 0.003;
        double exceptional_mobility_rate = 0.0001;

        // Cultural change rates
        double cultural_assimilation_rate = 0.02;
        double religious_conversion_rate = 0.01;
        double literacy_spread_rate = 0.05;

        // Employment transition rates
        double agricultural_to_craft_transition = 0.02;
        double craft_to_trade_transition = 0.015;
        double seasonal_employment_rate = 0.3;

        // Settlement evolution thresholds
        double settlement_growth_threshold = 1.02;
        double settlement_decline_threshold = 0.98;
        double urbanization_growth_rate = 0.01;

        // Economic parameters
        double guild_formation_threshold = 0.05;
        double trade_specialization_bonus = 0.15;
        double resource_specialization_bonus = 0.2;
    };

    // ============================================================================
    // Main Population System
    // ============================================================================

    class PopulationSystem : public core::ecs::ThreadSafeSystem {
    public:
        explicit PopulationSystem(core::ecs::ComponentAccessManager& access_manager,
                                core::ecs::MessageBus& message_bus);
        
        virtual ~PopulationSystem() = default;

        // System lifecycle
        void Initialize() override;
        void Update(float delta_time) override;
        void Shutdown() override;
        
        // Threading configuration
        core::threading::ThreadingStrategy GetThreadingStrategy() const override;
        std::string GetThreadingRationale() const override;

        // Population management interface
        void CreateInitialPopulation(EntityID province_id, const std::string& culture, 
                                   const std::string& religion, int base_population,
                                   double prosperity_level = 0.5, int year = 1200);
        
        void ProcessDemographicChanges(EntityID province_id, double yearly_fraction);
        void ProcessSocialMobility(EntityID province_id, double yearly_fraction);
        void ProcessSettlementEvolution(EntityID province_id, double yearly_fraction);
        void ProcessEmploymentShifts(EntityID province_id, double yearly_fraction);
        void ProcessCulturalChanges(EntityID province_id, double yearly_fraction);

        // Crisis management
        void ProcessPlague(EntityID province_id, const PlagueEvent& plague_data);
        void ProcessFamine(EntityID province_id, const FamineEvent& famine_data);
        void ProcessNaturalDisaster(EntityID province_id, const NaturalDisasterEvent& disaster_data);
        void ProcessSocialUnrest(EntityID province_id, const SocialUnrestEvent& unrest_data);

        // Military integration
        void ProcessMilitaryRecruitment(EntityID province_id, const MilitaryRecruitmentEvent& recruitment_data);
        void ProcessMilitaryService(EntityID province_id, const MilitaryServiceEvent& service_data);
        void UpdateMilitaryEligibility(EntityID province_id);

        // Administrative integration
        void ProcessTaxationChange(EntityID province_id, const TaxationChangeEvent& tax_data);
        void ProcessLegalCodeChange(EntityID province_id, const LegalCodeChangeEvent& legal_data);
        void ProcessAdministrativeReform(EntityID province_id, const AdministrativeReformEvent& reform_data);

        // Economic integration
        void UpdateEconomicImpact(EntityID province_id);
        void ProcessGuildFormation(EntityID province_id, const std::string& settlement_name);
        void ProcessEmploymentShift(EntityID province_id, const EmploymentShiftEvent& shift_data);

        // Analysis and reporting
        PopulationAnalyzer::PopulationReport GeneratePopulationReport(EntityID province_id);
        PopulationTrendAnalysis AnalyzeTrends(EntityID province_id, std::chrono::hours analysis_period);
        std::vector<EntityID> GetProvincesInCrisis();
        std::vector<std::string> GetPopulationWarnings(EntityID province_id);

        // Migration system
        void ProcessMigration(double yearly_fraction);
        void ProcessMigrationBetweenProvinces(EntityID from_province, EntityID to_province, 
                                            const MigrationEvent& migration_data);

        // Configuration
        void UpdateConfiguration(const PopulationSystemConfig& new_config);
        const PopulationSystemConfig& GetConfiguration() const;

    private:
        // Core dependencies
        core::ecs::ComponentAccessManager& m_access_manager;
        core::ecs::MessageBus& m_message_bus;

        // System state
        bool m_initialized = false;
        PopulationSystemConfig m_config;
        std::unique_ptr<EnhancedPopulationFactory> m_factory;
        std::unique_ptr<PopulationEventProcessor> m_event_processor;
        std::unique_ptr<PopulationEventFormatter> m_event_formatter;

        // Timing and updates
        float m_accumulated_time = 0.0f;
        float m_demographic_timer = 0.0f;
        float m_mobility_timer = 0.0f;
        float m_settlement_timer = 0.0f;

        // Random generation
        std::random_device m_random_device;
        std::mt19937 m_random_generator;

        // Population tracking
        std::unordered_map<EntityID, std::chrono::steady_clock::time_point> m_last_updates;
        std::unordered_map<EntityID, std::vector<std::string>> m_active_crises;

        // System initialization
        void InitializeEventProcessor();
        void InitializeFactory();
        void SubscribeToEvents();
        void LoadConfiguration();

        // Update processing
        void ProcessRegularUpdates(float delta_time);
        void ProcessDemographicUpdates(float delta_time);
        void ProcessMobilityUpdates(float delta_time);
        void ProcessSettlementUpdates(float delta_time);

        // Core demographic calculations
        void UpdatePopulationGrowth(PopulationComponent& population, double yearly_fraction);
        void UpdateAgeStructure(PopulationComponent& population, double yearly_fraction);
        void UpdateHealthAndMortality(PopulationComponent& population, double yearly_fraction);
        void UpdateLiteracyAndEducation(PopulationComponent& population, double yearly_fraction);

        // Social mobility implementation
        void ProcessClassMobility(PopulationComponent& population, EntityID province_id, double yearly_fraction);
        void ProcessLegalStatusChanges(PopulationComponent& population, EntityID province_id, double yearly_fraction);
        void ProcessGuildAdvancement(PopulationComponent& population, SettlementComponent& settlements, 
                                   EntityID province_id, double yearly_fraction);

        // Settlement management
        void UpdateSettlementGrowth(SettlementComponent& settlements, const PopulationComponent& population, 
                                  double yearly_fraction);
        void UpdateSettlementSpecialization(SettlementComponent& settlements, const PopulationComponent& population);
        void UpdateUrbanization(SettlementComponent& settlements, PopulationComponent& population, 
                               double yearly_fraction);

        // Employment management
        void UpdateEmploymentDistribution(PopulationComponent& population, const SettlementComponent& settlements);
        void ProcessJobCreation(PopulationComponent& population, const SettlementComponent& settlements);
        void ProcessJobLoss(PopulationComponent& population, const std::string& reason);

        // Cultural and religious changes
        void ProcessCulturalAssimilation(PopulationComponent& population, EntityID province_id, double yearly_fraction);
        void ProcessReligiousConversion(PopulationComponent& population, EntityID province_id, double yearly_fraction);
        void UpdateCulturalTensions(PopulationComponent& population, EntityID province_id);

        // Crisis processing
        void ApplyCrisisEffects(PopulationComponent& population, const std::string& crisis_type, double severity);
        void RecoverFromCrisis(PopulationComponent& population, const std::string& crisis_type, double recovery_rate);
        void UpdateCrisisState(EntityID province_id, const std::string& crisis_type, bool active);

        // Helper methods
        std::vector<EntityID> GetAllPopulatedProvinces();
        PopulationGroup* FindPopulationGroup(PopulationComponent& population, SocialClass social_class, 
                                           const std::string& culture, const std::string& religion);
        PopulationGroup* FindOrCreatePopulationGroup(PopulationComponent& population, SocialClass social_class, 
                                                    LegalStatus legal_status, const std::string& culture, 
                                                    const std::string& religion);
        
        void RecalculatePopulationSummary(PopulationComponent& population);
        void RecalculateSettlementSummary(SettlementComponent& settlements);
        void ValidatePopulationConsistency(EntityID province_id);
        
        double CalculateWealthDisparity(const PopulationComponent& population);
        double CalculateSocialStability(const PopulationComponent& population);
        double CalculateUrbanizationPressure(const PopulationComponent& population, 
                                           const SettlementComponent& settlements);

        // Random generation helpers
        double GenerateRandomDouble(double min, double max);
        int GenerateRandomInt(int min, int max);
        bool RandomChance(double probability);
        
        // Event generation
        void SendPopulationUpdateEvent(EntityID province_id, const PopulationComponent& population);
        void SendDemographicChangeEvent(EntityID province_id, const PopulationGroup& group, 
                                      int births, int deaths, const std::string& reason);
        void SendSocialMobilityEvent(EntityID province_id, SocialClass from_class, SocialClass to_class,
                                   int population_affected, const std::string& reason);
        void SendSettlementEvolutionEvent(EntityID province_id, const Settlement& settlement, 
                                        SettlementType old_type, const std::string& reason);
        void SendEmploymentShiftEvent(EntityID province_id, EmploymentType from_employment, 
                                    EmploymentType to_employment, int workers_affected, 
                                    const std::string& reason);
        void SendCulturalAssimilationEvent(EntityID province_id, const std::string& from_culture, 
                                         const std::string& to_culture, int population_affected);
        void SendCrisisEvent(EntityID province_id, const std::string& crisis_type, double severity,
                           const std::vector<SocialClass>& affected_classes);

        // Integration helpers
        void NotifyMilitarySystem(EntityID province_id, const MilitaryRecruitmentEvent& data);
        void NotifyEconomicSystem(EntityID province_id, const EconomicUpdateEvent& data);
        void NotifyAdministrativeSystem(EntityID province_id, const TaxationChangeEvent& data);
        void NotifySettlementSystem(EntityID province_id, const SettlementUpdateEvent& data);
    };

    // ============================================================================
    // Population Factory for Initial Creation
    // ============================================================================

    class EnhancedPopulationFactory {
    public:
        EnhancedPopulationFactory();
        
        PopulationComponent CreateMedievalPopulation(const std::string& culture, const std::string& religion,
                                                    int base_population, double prosperity_level = 0.5, 
                                                    int year = 1200);
        
        SettlementComponent CreateMedievalSettlements(const std::string& province_name, int total_population,
                                                     double prosperity_level = 0.5, const std::string& culture = "english",
                                                     const std::string& religion = "catholic", int year = 1200,
                                                     const std::vector<std::string>& strategic_resources = {});

    private:
        std::random_device m_random_device;
        std::mt19937 m_random_generator;

        // Population creation methods
        void CreateNoblePopulation(PopulationComponent& population, const std::string& culture, 
                                 const std::string& religion, int base_population, double prosperity_level, int year);
        void CreateClergyPopulation(PopulationComponent& population, const std::string& culture, 
                                  const std::string& religion, int base_population, double prosperity_level, int year);
        void CreateMerchantPopulation(PopulationComponent& population, const std::string& culture, 
                                    const std::string& religion, int base_population, double prosperity_level, int year);
        void CreateCraftsmanPopulation(PopulationComponent& population, const std::string& culture, 
                                     const std::string& religion, int base_population, double prosperity_level, int year);
        void CreatePeasantPopulation(PopulationComponent& population, const std::string& culture, 
                                   const std::string& religion, int base_population, double prosperity_level, int year);

        // Settlement creation methods
        void CreateUrbanSettlements(SettlementComponent& settlements, const std::string& province_name,
                                   int urban_population, double prosperity_level, const std::string& culture,
                                   const std::string& religion, int year, const std::vector<std::string>& resources);
        void CreateRuralSettlements(SettlementComponent& settlements, const std::string& province_name,
                                   int rural_population, double prosperity_level, const std::string& culture,
                                   const std::string& religion, int year, const std::vector<std::string>& resources);
        void CreateMilitarySettlements(SettlementComponent& settlements, const std::string& province_name,
                                      double prosperity_level, const std::string& culture, const std::string& religion, 
                                      int year, const std::vector<std::string>& resources);
        void CreateReligiousSettlements(SettlementComponent& settlements, const std::string& province_name,
                                       double prosperity_level, const std::string& culture, const std::string& religion, 
                                       int year);

        // Helper methods
        void SetGroupCharacteristics(PopulationGroup& group, SocialClass social_class, LegalStatus legal_status,
                                   double prosperity_level, int year);
        std::vector<std::string> GetLegalPrivileges(LegalStatus status);
        std::vector<std::string> GetEconomicRights(LegalStatus status);
        double GetClassBaseWealth(SocialClass social_class, double prosperity_level);
        double GetClassLiteracyRate(SocialClass social_class, int year);
        EmploymentType GetPrimaryEmployment(SocialClass social_class);
        Settlement CreateSettlement(const std::string& name, SettlementType type, 
                                   const std::string& province_name, double prosperity_level);
    };

} // namespace game::population