// ============================================================================
// PopulationSystem.h - Population Management System
// Created: December 19, 2024 at 10:45 AM
// Location: include/game/population/PopulationSystem.h
// ============================================================================

#pragma once

#include "core/ECS/ComponentAccessManager.h"
#include "core/threading/ThreadSafeMessageBus.h"
#include "core/ECS/ISystem.h"
#include "core/threading/ThreadedSystemManager.h"
#include "game/population/PopulationTypes.h"
#include "game/population/PopulationEvents.h"
#include "game/population/PopulationComponents.h"
#include <memory>
#include <random>
#include <unordered_map>
#include <vector>
#include <string>
#include <chrono>

namespace game::population {

    // Forward declarations for classes
    class PopulationEventProcessor;
    class PopulationEventFormatter;
    class PopulationAnalyzer;
    class EnhancedPopulationFactory;

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

    class PopulationSystem : public game::core::ISystem {
    public:
        explicit PopulationSystem(::core::ecs::ComponentAccessManager& access_manager,
                                ::core::threading::ThreadSafeMessageBus& message_bus);
        
        virtual ~PopulationSystem() = default;

        // System lifecycle
        void Initialize() override;
        void Update(float delta_time) override;
        void Shutdown() override;
        
        // Threading configuration
        ::core::threading::ThreadingStrategy GetThreadingStrategy() const override;
        std::string GetThreadingRationale() const;

        // Serialization interface
        std::string GetSystemName() const override;
        Json::Value Serialize(int version) const override;
        bool Deserialize(const Json::Value& data, int version) override;

        // Population management interface
        void CreateInitialPopulation(game::types::EntityID province_id, const std::string& culture, 
                                   const std::string& religion, int base_population,
                                   double prosperity_level = 0.5, int year = 1200);
        
        void ProcessDemographicChanges(game::types::EntityID province_id, double yearly_fraction);
        void ProcessSocialMobility(game::types::EntityID province_id, double yearly_fraction);
        void ProcessSettlementEvolution(game::types::EntityID province_id, double yearly_fraction);
        void ProcessEmploymentShifts(game::types::EntityID province_id, double yearly_fraction);
        void ProcessCulturalChanges(game::types::EntityID province_id, double yearly_fraction);

        // Crisis management
        void ProcessPlague(game::types::EntityID province_id, const PlagueEvent& plague_data);
        void ProcessFamine(game::types::EntityID province_id, const FamineEvent& famine_data);
        void ProcessNaturalDisaster(game::types::EntityID province_id, const NaturalDisasterEvent& disaster_data);
        void ProcessSocialUnrest(game::types::EntityID province_id, const SocialUnrestEvent& unrest_data);

        // Military integration
        void ProcessMilitaryRecruitment(game::types::EntityID province_id, const MilitaryRecruitmentEvent& recruitment_data);
        void ProcessMilitaryService(game::types::EntityID province_id, const MilitaryServiceEvent& service_data);
        void UpdateMilitaryEligibility(game::types::EntityID province_id);

        // Administrative integration
        void ProcessTaxationChange(game::types::EntityID province_id, const TaxationChangeEvent& tax_data);
        void ProcessLegalCodeChange(game::types::EntityID province_id, const LegalCodeChangeEvent& legal_data);
        void ProcessAdministrativeReform(game::types::EntityID province_id, const AdministrativeReformEvent& reform_data);

        // Economic integration
        void UpdateEconomicImpact(game::types::EntityID province_id);
        void ProcessGuildFormation(game::types::EntityID province_id, const std::string& settlement_name);
        void ProcessEmploymentShift(game::types::EntityID province_id, const EmploymentShiftEvent& shift_data);

        // Analysis and reporting
        // TODO: Implement PopulationAnalyzer class
        // PopulationAnalyzer::PopulationReport GeneratePopulationReport(game::types::EntityID province_id);
        PopulationTrendAnalysis AnalyzeTrends(game::types::EntityID province_id, std::chrono::hours analysis_period);
        std::vector<game::types::EntityID> GetProvincesInCrisis();
        std::vector<std::string> GetPopulationWarnings(game::types::EntityID province_id);

        // Migration system
        void ProcessMigration(double yearly_fraction);
        void ProcessMigrationBetweenProvinces(game::types::EntityID from_province, game::types::EntityID to_province, 
                                            const MigrationEvent& migration_data);

        // Configuration
        void UpdateConfiguration(const PopulationSystemConfig& new_config);
        const PopulationSystemConfig& GetConfiguration() const;

    private:
        // Core dependencies
        ::core::ecs::ComponentAccessManager& m_access_manager;
        ::core::threading::ThreadSafeMessageBus& m_message_bus;

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
        std::unordered_map<game::types::EntityID, std::chrono::steady_clock::time_point> m_last_updates;
        std::unordered_map<game::types::EntityID, std::vector<std::string>> m_active_crises;

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
        void ProcessClassMobility(PopulationComponent& population, game::types::EntityID province_id, double yearly_fraction);
        void ProcessLegalStatusChanges(PopulationComponent& population, game::types::EntityID province_id, double yearly_fraction);
        void ProcessGuildAdvancement(PopulationComponent& population, SettlementComponent& settlements, 
                                   game::types::EntityID province_id, double yearly_fraction);

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
        void ProcessCulturalAssimilation(PopulationComponent& population, game::types::EntityID province_id, double yearly_fraction);
        void ProcessReligiousConversion(PopulationComponent& population, game::types::EntityID province_id, double yearly_fraction);
        void UpdateCulturalTensions(PopulationComponent& population, game::types::EntityID province_id);

        // Crisis processing
        void ApplyCrisisEffects(PopulationComponent& population, const std::string& crisis_type, double severity);
        void RecoverFromCrisis(PopulationComponent& population, const std::string& crisis_type, double recovery_rate);
        void UpdateCrisisState(game::types::EntityID province_id, const std::string& crisis_type, bool active);

        // Helper methods
        std::vector<game::types::EntityID> GetAllPopulatedProvinces();
        PopulationGroup* FindPopulationGroup(PopulationComponent& population, SocialClass social_class, 
                                           const std::string& culture, const std::string& religion);
        PopulationGroup* FindOrCreatePopulationGroup(PopulationComponent& population, SocialClass social_class, 
                                                    LegalStatus legal_status, const std::string& culture, 
                                                    const std::string& religion);
        
        void RecalculateSettlementSummary(SettlementComponent& settlements);
        void ValidatePopulationConsistency(game::types::EntityID province_id);
        
        double CalculateWealthDisparity(const PopulationComponent& population);
        double CalculateSocialStability(const PopulationComponent& population);
        double CalculateUrbanizationPressure(const PopulationComponent& population, 
                                           const SettlementComponent& settlements);

        // Random generation helpers
        double GenerateRandomDouble(double min, double max);
        int GenerateRandomInt(int min, int max);
        bool RandomChance(double probability);
        
        // ECS component helpers
        void RecalculatePopulationAggregates(PopulationComponent& population);
        
        // Event generation
        void SendPopulationUpdateEvent(game::types::EntityID province_id, const PopulationComponent& population);
        void SendDemographicChangeEvent(game::types::EntityID province_id, const PopulationGroup& group, 
                                      int births, int deaths, const std::string& reason);
        void SendSocialMobilityEvent(game::types::EntityID province_id, SocialClass from_class, SocialClass to_class,
                                   int population_affected, const std::string& reason);
        void SendSettlementEvolutionEvent(game::types::EntityID province_id, const Settlement& settlement, 
                                        SettlementType old_type, const std::string& reason);
        void SendEmploymentShiftEvent(game::types::EntityID province_id, EmploymentType from_employment, 
                                    EmploymentType to_employment, int workers_affected, 
                                    const std::string& reason);
        void SendCulturalAssimilationEvent(game::types::EntityID province_id, const std::string& from_culture, 
                                         const std::string& to_culture, int population_affected);
        void SendCrisisEvent(game::types::EntityID province_id, const std::string& crisis_type, double severity,
                           const std::vector<SocialClass>& affected_classes);

        // Integration helpers
        void NotifyMilitarySystem(game::types::EntityID province_id, const MilitaryRecruitmentEvent& data);
        void NotifyEconomicSystem(game::types::EntityID province_id, const EconomicUpdateEvent& data);
        void NotifyAdministrativeSystem(game::types::EntityID province_id, const TaxationChangeEvent& data);
        void NotifySettlementSystem(game::types::EntityID province_id, const SettlementUpdateEvent& data);
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
        // Settlement creation helpers (MISSING FROM ORIGINAL)
        void CreateSecondaryUrbanSettlements(SettlementComponent& settlements,
                                            const std::string& province_name,
                                            int remaining_population,
                                            double prosperity_level,
                                            const std::string& culture,
                                            const std::string& religion,
                                            int year,
                                            const std::vector<std::string>& strategic_resources);
        
        // Settlement type and characteristics
        SettlementType DetermineMainCityType(int urban_population, double prosperity_level);
        double CalculateUrbanizationRate(int total_population, double prosperity_level, int year);
        void SetEconomicSpecializations(Settlement& settlement,
                                        const std::vector<std::string>& strategic_resources,
                                        double prosperity_level);
        
        // Peasant distribution calculations
        double CalculateFreePeasantPercentage(int year, double prosperity_level);
        double CalculateVilleinPercentage(int year, double prosperity_level);
        
        // Settlement infrastructure helpers
        double GetSettlementInfrastructure(SettlementType type, double prosperity_level);
        double GetSettlementFortification(SettlementType type, double prosperity_level);
        double GetSettlementSanitation(SettlementType type, double prosperity_level);
        double GetSettlementWaterAccess(SettlementType type, double prosperity_level);
        double GetSettlementAutonomy(SettlementType type);
        double GetSettlementDiseaseRisk(SettlementType type, double prosperity_level);
        
        // Group characteristics (MISSING FROM ORIGINAL)
        void SetDemographicRates(PopulationGroup& group, SocialClass social_class, double prosperity_level);
        void SetCulturalFactors(PopulationGroup& group, SocialClass social_class, int year);
        
        // Legal system helpers (DECLARED BUT NOT IMPLEMENTED)
        std::vector<std::string> GetLegalPrivileges(LegalStatus status);
        std::vector<std::string> GetEconomicRights(LegalStatus status);
        std::vector<std::string> GetSocialRestrictions(LegalStatus status);
        
        // Military helpers (MISSING FROM ORIGINAL)
        int CalculateMilitaryEligible(const PopulationGroup& group);
        double CalculateMilitaryQuality(SocialClass social_class, double prosperity_level);
        
        // Class characteristics (MISSING FROM ORIGINAL)
        double GetClassHealthLevel(SocialClass social_class, double prosperity_level);
        double GetClassBaseHappiness(SocialClass social_class, double prosperity_level);
        
        // Foreign culture/religion determination (IMPLEMENTED BUT NOT DECLARED)
        std::string DetermineForeignCulture(const std::string& local_culture, int year);
        std::string DetermineForeignReligion(const std::string& local_religion, int year);
        
        // Random generation helper
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

        // Additional population creation methods
        void CreateLesserNoblePopulation(PopulationComponent& population, const std::string& culture, 
                                       const std::string& religion, int base_population, double prosperity_level, int year);
        void CreateScholarPopulation(PopulationComponent& population, const std::string& culture, 
                                   const std::string& religion, int base_population, double prosperity_level, int year);
        void CreateUrbanLaborerPopulation(PopulationComponent& population, const std::string& culture, 
                                        const std::string& religion, int base_population, double prosperity_level, int year);
        void CreateReligiousOrdersPopulation(PopulationComponent& population, const std::string& culture, 
                                           const std::string& religion, int base_population, double prosperity_level, int year);
        void CreateForeignerPopulation(PopulationComponent& population, const std::string& culture, 
                                     const std::string& religion, int base_population, double prosperity_level, int year);
        
        // Additional settlement creation methods
        void CreateAdministrativeSettlements(SettlementComponent& settlements, const std::string& province_name,
                                           double prosperity_level, const std::string& culture, const std::string& religion, int year);
        
        // Helper methods
        void SetGroupCharacteristics(PopulationGroup& group, SocialClass social_class, LegalStatus legal_status,
                                   double prosperity_level, int year);
        double GetClassBaseWealth(SocialClass social_class, double prosperity_level);
        double GetClassLiteracyRate(SocialClass social_class, int year);
        EmploymentType GetPrimaryEmployment(SocialClass social_class);
        Settlement CreateSettlement(const std::string& name, SettlementType type, 
                                   const std::string& province_name, double prosperity_level);
        
        // Population analysis and calculation methods
        double GetHistoricalPercentage(SocialClass social_class, int year, double prosperity_level);
        void SetEmploymentDistribution(PopulationGroup& group, SocialClass social_class);
        void RecalculateSettlementSummary(SettlementComponent& settlements);
    };

} // namespace game::population