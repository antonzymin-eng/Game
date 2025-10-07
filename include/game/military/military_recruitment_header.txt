// ============================================================================
// Date/Time Created: Wednesday, September 24, 2025 - 2:30 PM PST
// Intended Folder Location: include/game/military/MilitaryRecruitmentSystem.h
// MilitaryRecruitmentSystem.h - Population-Based Military Recruitment (Declarations Only)
// Dependencies: PopulationSystem, EconomicSystem, TradeSystem, ECS Foundation
// ============================================================================

#pragma once

#include "core/ECS/ISystem.h"
#include "core/ECS/ComponentAccessManager.h"
#include "core/ECS/MessageBus.h"
#include "core/Types/game_types.h"
#include "game/population/PopulationComponents.h"

#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>

namespace game::military {

    // ============================================================================
    // Military Unit Types & Classifications
    // ============================================================================

    enum class UnitType : uint8_t {
        // Infantry Units
        LEVY_SPEARMEN = 0,        // Peasant militia with spears
        PROFESSIONAL_INFANTRY = 1, // Trained foot soldiers
        CROSSBOWMEN = 2,          // Ranged infantry
        PIKEMEN = 3,              // Anti-cavalry specialists
        HEAVY_INFANTRY = 4,       // Elite armored foot soldiers

        // Cavalry Units
        LIGHT_CAVALRY = 10,       // Fast scouts and raiders
        HEAVY_CAVALRY = 11,       // Armored knights
        HORSE_ARCHERS = 12,       // Mounted ranged units

        // Specialist Units
        ENGINEERS = 20,           // Siege specialists
        ARTILLERY_CREW = 21,      // Cannon operators
        MARINES = 22,             // Naval infantry

        // Nobles and Officers
        KNIGHTS = 30,             // Noble heavy cavalry
        RETINUE = 31,             // Noble household troops
        COMMANDERS = 32,          // Military leaders

        INVALID = 255
    };

    enum class UnitQuality : uint8_t {
        GREEN = 0,         // Untrained recruits
        TRAINED = 1,       // Basic military training
        EXPERIENCED = 2,   // Combat veterans
        ELITE = 3,         // Best of the best
        LEGENDARY = 4      // Heroic units
    };

    enum class RecruitmentType : uint8_t {
        VOLUNTARY = 0,     // Professional soldiers
        CONSCRIPTION = 1,  // Forced service
        FEUDAL_LEVY = 2,   // Feudal obligations
        MERCENARY = 3,     // Hired troops
        MILITIA = 4        // Local defense forces
    };

    // ============================================================================
    // Military Components
    // ============================================================================

    struct MilitaryUnit {
        types::EntityID unit_id;
        UnitType type = UnitType::INVALID;
        UnitQuality quality = UnitQuality::GREEN;
        RecruitmentType recruitment_type = RecruitmentType::VOLUNTARY;

        // Unit composition
        int current_strength = 0;      // Current number of soldiers
        int maximum_strength = 0;      // Full strength capacity
        double experience = 0.0;       // Combat experience (0.0-1.0)
        double morale = 0.7;          // Unit morale (0.0-1.0)
        double equipment_quality = 0.5; // Equipment condition (0.0-1.0)

        // Recruitment data
        types::EntityID source_province;
        types::SocialClass recruited_from = types::SocialClass::FREE_PEASANT;
        std::chrono::steady_clock::time_point recruitment_date;

        // Maintenance costs
        double monthly_upkeep = 0.0;   // Base maintenance cost
        double equipment_upkeep = 0.0; // Equipment maintenance

        // Combat capabilities
        double melee_attack = 1.0;
        double ranged_attack = 0.0;
        double defense = 1.0;
        double mobility = 1.0;

        MilitaryUnit();
        MilitaryUnit(UnitType unit_type, types::EntityID province, types::SocialClass social_class);
    };

    struct MilitaryComponent : public core::ecs::IComponent<MilitaryComponent> {
        // Active military units
        std::vector<MilitaryUnit> active_units;
        std::unordered_map<UnitType, int> unit_counts;

        // Military infrastructure
        int barracks_capacity = 0;     // Maximum unit housing
        int training_facilities = 0;   // Training efficiency bonus
        int armory_level = 0;         // Equipment storage and quality

        // Recruitment settings
        RecruitmentType default_recruitment = RecruitmentType::VOLUNTARY;
        double recruitment_rate = 0.1; // Percentage of eligible population recruited per month
        bool active_recruitment = false;

        // Military readiness
        double overall_readiness = 0.0; // 0.0-1.0 combat readiness
        double supply_level = 1.0;     // Supply and logistics status
        int total_active_soldiers = 0;

        // Financial data
        double monthly_military_expenses = 0.0;
        double equipment_costs = 0.0;
        double recruitment_costs = 0.0;

        MilitaryComponent() = default;
    };

    struct RecruitmentPoolComponent : public core::ecs::IComponent<RecruitmentPoolComponent> {
        // Available recruits by social class
        std::unordered_map<types::SocialClass, int> available_recruits;
        std::unordered_map<types::SocialClass, int> recruitment_potential;

        // Recruitment modifiers
        double recruitment_enthusiasm = 0.5; // Population willingness to serve
        double military_tradition = 0.3;     // Cultural military heritage
        bool wartime_recruitment = false;    // Emergency recruitment active

        // Class-specific recruitment data
        struct ClassRecruitmentData {
            double base_recruitment_rate = 0.05;  // 5% can be recruited per year
            double current_recruitment_rate = 0.05;
            int currently_serving = 0;             // How many from this class are serving
            int lifetime_recruited = 0;           // Total recruited historically
            double recruitment_resistance = 0.0;   // Resistance to recruitment (0.0-1.0)
        };

        std::unordered_map<types::SocialClass, ClassRecruitmentData> class_data;

        RecruitmentPoolComponent() = default;
    };

    // ============================================================================
    // Military Events & Messages
    // ============================================================================

    namespace messages {

        struct UnitRecruited {
            MilitaryUnit unit;
            types::EntityID province_id;
            int recruitment_cost;
            std::string recruitment_reason;
            std::chrono::steady_clock::time_point timestamp;
        };

        struct UnitDisbanded {
            types::EntityID unit_id;
            types::EntityID province_id;
            UnitType unit_type;
            int returned_population;
            std::string disbandment_reason;
        };

        struct RecruitmentCrisis {
            types::EntityID province_id;
            std::string crisis_type;
            double severity; // 0.0-1.0
            std::vector<types::SocialClass> affected_classes;
            std::string description;
        };

        struct MilitaryUpkeepShortfall {
            types::EntityID province_id;
            double shortfall_amount;
            std::vector<types::EntityID> affected_units;
            std::string consequences;
        };

        struct UnitPromoted {
            types::EntityID unit_id;
            UnitQuality old_quality;
            UnitQuality new_quality;
            std::string promotion_reason;
        };

    } // namespace messages

    // ============================================================================
    // Military Recruitment System - Main Class Declaration
    // ============================================================================

    class MilitaryRecruitmentSystem : public core::ecs::ISystem {
    public:
        MilitaryRecruitmentSystem(core::ecs::ComponentAccessManager& access_manager,
            core::ecs::MessageBus& message_bus);
        ~MilitaryRecruitmentSystem() = default;

        // ISystem interface
        void Initialize() override;
        void Update(float delta_time) override;
        void Shutdown() override;
        std::string GetSystemName() const override;

        // ========================================================================
        // Core Recruitment Interface
        // ========================================================================

        // Primary recruitment methods
        bool RecruitUnit(types::EntityID province_id, UnitType unit_type,
            types::SocialClass preferred_class = types::SocialClass::FREE_PEASANT);
        bool RecruitMultipleUnits(types::EntityID province_id, UnitType unit_type, int count,
            types::SocialClass preferred_class = types::SocialClass::FREE_PEASANT);

        // Batch recruitment for emergencies
        std::vector<MilitaryUnit> EmergencyRecruitment(types::EntityID province_id, int target_strength,
            RecruitmentType recruitment_type = RecruitmentType::CONSCRIPTION);

        // Unit management
        bool DisbandUnit(types::EntityID unit_id);
        bool DisbandAllUnits(types::EntityID province_id);
        void ReturnPopulationFromUnit(const MilitaryUnit& unit);

        // ========================================================================
        // Recruitment Pool Management
        // ========================================================================

        void UpdateRecruitmentPools();
        void CalculateRecruitmentPotential(types::EntityID province_id);
        int GetAvailableRecruits(types::EntityID province_id, types::SocialClass social_class);
        int GetTotalRecruitmentCapacity(types::EntityID province_id);

        // Social class recruitment specifics
        std::vector<UnitType> GetViableUnitTypes(types::SocialClass social_class);
        types::SocialClass GetOptimalRecruitmentClass(UnitType unit_type);
        double GetClassRecruitmentRate(types::EntityID province_id, types::SocialClass social_class);

        // ========================================================================
        // Equipment & Supply Integration
        // ========================================================================

        // Equipment requirements
        std::unordered_map<types::ResourceType, int> GetEquipmentRequirements(UnitType unit_type, int unit_size);
        bool CheckEquipmentAvailability(types::EntityID province_id, UnitType unit_type, int unit_count);
        void ConsumeEquipmentForRecruitment(types::EntityID province_id, UnitType unit_type, int unit_count);

        // Ongoing supply needs
        std::unordered_map<types::ResourceType, double> GetMonthlySupplyNeeds(const MilitaryUnit& unit);
        void ProcessMonthlySupplyConsumption(types::EntityID province_id);
        bool CanSupplyUnits(types::EntityID province_id);

        // ========================================================================
        // Unit Quality & Training
        // ========================================================================

        void ProcessUnitTraining(types::EntityID province_id, float time_delta);
        void ImproveUnitQuality(MilitaryUnit& unit, double experience_gain);
        UnitQuality CalculateRecruitQuality(types::EntityID province_id, types::SocialClass social_class);

        // Training infrastructure effects
        double GetTrainingEfficiencyBonus(types::EntityID province_id);
        void UpdateUnitExperience(MilitaryUnit& unit, double base_gain, float time_delta);

        // ========================================================================
        // Economic Integration
        // ========================================================================

        // Cost calculations
        double CalculateRecruitmentCost(UnitType unit_type, types::SocialClass social_class);
        double CalculateMonthlyUpkeep(const MilitaryUnit& unit);
        double CalculateEquipmentCost(UnitType unit_type, UnitQuality quality);

        // Economic impact
        void ProcessMilitaryExpenses(types::EntityID province_id);
        double GetTotalMilitaryBurden(types::EntityID province_id);
        void ApplyMilitaryEconomicEffects(types::EntityID province_id);

        // ========================================================================
        // Query Interface
        // ========================================================================

        // Unit information
        std::vector<MilitaryUnit> GetAllUnits(types::EntityID province_id);
        std::vector<MilitaryUnit> GetUnitsByType(types::EntityID province_id, UnitType unit_type);
        MilitaryUnit* GetUnit(types::EntityID unit_id);
        int GetTotalMilitaryStrength(types::EntityID province_id);

        // Recruitment information
        bool CanRecruit(types::EntityID province_id, UnitType unit_type, types::SocialClass social_class);
        std::string GetRecruitmentLimitationReason(types::EntityID province_id, UnitType unit_type);

        // Military readiness
        double CalculateMilitaryReadiness(types::EntityID province_id);
        double CalculateUnitEffectiveness(const MilitaryUnit& unit);

        // Statistics
        int GetActiveUnitCount(types::EntityID province_id);
        int GetTotalSoldierCount(types::EntityID province_id);
        double GetAverageUnitQuality(types::EntityID province_id);

        // ========================================================================
        // Configuration & Settings
        // ========================================================================

        void SetRecruitmentPolicy(types::EntityID province_id, RecruitmentType policy);
        void SetRecruitmentRate(types::EntityID province_id, double rate);
        void EnableEmergencyRecruitment(types::EntityID province_id, bool enable);

        void SetGlobalMilitaryBudget(double budget_percentage);
        void SetUnitUpkeepModifier(double modifier);
        void SetTrainingEfficiencyModifier(double modifier);

    private:
        // ========================================================================
        // Internal Data Management
        // ========================================================================

        core::ecs::ComponentAccessManager& m_access_manager;
        core::ecs::MessageBus& m_message_bus;

        // Unit tracking
        std::unordered_map<types::EntityID, MilitaryUnit> m_all_units;
        types::EntityID m_next_unit_id{ 1 };

        // Update timing
        float m_accumulated_time = 0.0f;
        float m_recruitment_update_interval = 10.0f;  // Update recruitment every 10 seconds
        float m_training_update_interval = 5.0f;      // Update training every 5 seconds
        float m_training_accumulated_time = 0.0f;

        // Configuration
        double m_global_recruitment_modifier = 1.0;
        double m_global_upkeep_modifier = 1.0;
        double m_global_training_modifier = 1.0;
        double m_military_budget_percentage = 0.3; // 30% of income for military

        // Unit type definitions
        struct UnitTypeDefinition {
            std::string name;
            std::vector<types::SocialClass> viable_classes;
            std::unordered_map<types::ResourceType, int> equipment_requirements;
            std::unordered_map<types::ResourceType, double> monthly_supply_needs;
            double base_upkeep_cost;
            double base_recruitment_cost;
            int default_unit_size;
            UnitQuality min_quality;
            UnitQuality max_quality;
        };

        std::unordered_map<UnitType, UnitTypeDefinition> m_unit_definitions;

        // ========================================================================
        // Internal Implementation Methods (Declarations Only)
        // ========================================================================

        // Unit creation helpers
        MilitaryUnit CreateUnit(UnitType unit_type, types::EntityID province_id,
            types::SocialClass social_class, int size);
        void InitializeUnitStats(MilitaryUnit& unit);
        void ApplyQualityModifiers(MilitaryUnit& unit);

        // Recruitment validation
        bool ValidateRecruitmentRequirements(types::EntityID province_id, UnitType unit_type,
            types::SocialClass social_class);
        bool CheckPopulationAvailability(types::EntityID province_id, types::SocialClass social_class, int needed);
        bool CheckFinancialCapacity(types::EntityID province_id, UnitType unit_type);

        // Population integration
        void RemovePopulationForRecruitment(types::EntityID province_id, types::SocialClass social_class, int count);
        void ApplyRecruitmentEffectsToPopulation(types::EntityID province_id, types::SocialClass social_class, int recruited);

        // Economic integration helpers
        void ChargeRecruitmentCosts(types::EntityID province_id, double cost);
        void ProcessUpkeepPayments(types::EntityID province_id);
        void HandleInsufficientFunds(types::EntityID province_id, double shortfall);

        // Training and experience
        void ProcessBasicTraining(MilitaryUnit& unit, double training_efficiency, float time_delta);
        void ApplyExperienceGains(MilitaryUnit& unit, double base_experience, double time_factor);
        void CheckForPromotions(MilitaryUnit& unit);

        // Message publishing
        void PublishUnitRecruited(const MilitaryUnit& unit, types::EntityID province_id, int cost);
        void PublishUnitDisbanded(types::EntityID unit_id, types::EntityID province_id,
            UnitType unit_type, int returned_pop);
        void PublishRecruitmentCrisis(types::EntityID province_id, const std::string& crisis_type,
            double severity);

        // Initialization helpers
        void InitializeUnitDefinitions();
        void LoadMilitaryConfiguration();
        void SetupDefaultRecruitmentPools();

        // Utility methods
        std::string GetUnitTypeName(UnitType unit_type);
        std::string GetQualityName(UnitQuality quality);
        void LogMilitaryActivity(const std::string& message);
    };

} // namespace game::military