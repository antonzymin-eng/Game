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
#include "core/types/game_types.h"
#include "game/population/PopulationComponents.h"
#include "game/population/PopulationTypes.h"
#include "game/military/MilitaryComponents.h"

#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>

namespace game::military {

    // ============================================================================
    // Recruitment-Specific Enums and Types
    // ============================================================================

    enum class RecruitmentType : uint8_t {
        VOLUNTARY = 0,     // Professional soldiers
        CONSCRIPTION = 1,  // Forced service
        FEUDAL_LEVY = 2,   // Feudal obligations
        MERCENARY = 3,     // Hired troops
        MILITIA = 4        // Local defense forces
    };

    // ============================================================================
    // Recruitment-Specific Components
    // ============================================================================

    struct RecruitmentPoolComponent : public game::core::Component<RecruitmentPoolComponent> {
        // Available recruits by social class
        std::unordered_map<population::SocialClass, int> available_recruits;
        std::unordered_map<population::SocialClass, int> recruitment_potential;

        // Recruitment rates and effectiveness
        double base_recruitment_rate = 0.05;  // 5% of population per month
        double recruitment_efficiency = 1.0;   // Modifier based on infrastructure
        double population_willingness = 0.7;   // How willing population is to serve

        // Economic factors
        double local_wealth_modifier = 1.0;    // Affects equipment quality
        double recruitment_cost_modifier = 1.0; // Local cost adjustments

        // Class-specific data
        struct ClassRecruitmentData {
            double recruitment_rate = 0.05;
            double quality_modifier = 1.0;
            double cost_modifier = 1.0;
            double willingness = 0.7;
            int max_recruits_per_month = 100;
        };
        std::unordered_map<population::SocialClass, ClassRecruitmentData> class_data;

        // Recruitment events
        struct MilutaryEvent {
            std::string event_type;
            double magnitude;
            int duration_months;
            std::vector<population::SocialClass> affected_classes;
        };
        std::vector<MilutaryEvent> active_events;

        // Monthly recruitment tracking
        struct MonthlyRecruitmentData {
            std::unordered_map<population::SocialClass, int> recruited_count;
            std::unordered_map<UnitType, int> units_created;
            double total_cost = 0.0;
            double total_upkeep_added = 0.0;
        };
        MonthlyRecruitmentData current_month;
        std::vector<MonthlyRecruitmentData> recruitment_history;

        RecruitmentPoolComponent() = default;
    };

    // ============================================================================
    // Message Types for Recruitment Events
    // ============================================================================

    namespace messages {

        struct RecruitmentCrisis {
            types::EntityID province_id;
            std::string crisis_type;
            double severity; // 0.0-1.0
            std::vector<population::SocialClass> affected_classes;
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

    class MilitaryRecruitmentSystem : public game::core::ISystem {
    public:
        explicit MilitaryRecruitmentSystem(game::core::ComponentAccessManager& component_manager,
                                                       game::core::MessageBus& message_bus);
        ~MilitaryRecruitmentSystem() = default;

        // Core ISystem interface
        void Initialize() override;
        void Update(float delta_time) override;
        void Shutdown() override;
        std::string GetSystemName() const override;

        // ========================================================================
        // Core Recruitment Interface
        // ========================================================================

        // Primary recruitment methods
        bool RecruitUnit(types::EntityID province_id, UnitType unit_type,
            population::SocialClass preferred_class = population::SocialClass::FREE_PEASANTS);
        bool RecruitMultipleUnits(types::EntityID province_id, UnitType unit_type, int count,
            population::SocialClass preferred_class = population::SocialClass::FREE_PEASANTS);

        // Unit management
        bool DisbandUnit(types::EntityID unit_id);
        bool DisbandAllUnits(types::EntityID province_id);

        // ========================================================================
        // Recruitment Information & Analysis
        // ========================================================================

        // Population-based recruitment queries  
        int GetAvailableRecruits(types::EntityID province_id, population::SocialClass social_class);
        int GetTotalRecruitmentCapacity(types::EntityID province_id);

        // Unit viability analysis
        std::vector<UnitType> GetViableUnitTypes(population::SocialClass social_class);
        population::SocialClass GetOptimalRecruitmentClass(UnitType unit_type);
        double GetClassRecruitmentRate(types::EntityID province_id, population::SocialClass social_class);

        // ========================================================================
        // Quality & Cost Calculations  
        // ========================================================================

        // Quality determination
        UnitQuality CalculateBaseQuality(types::EntityID province_id);
        UnitQuality CalculateRecruitQuality(types::EntityID province_id, population::SocialClass social_class);

        // Economic calculations
        double CalculateRecruitmentCost(UnitType unit_type, population::SocialClass social_class);
        double CalculateMonthlyUpkeep(UnitType unit_type, UnitQuality quality);
        double GetTotalMilitaryUpkeep(types::EntityID province_id);

        // ========================================================================
        // Recruitment Feasibility & Constraints
        // ========================================================================

        // Recruitment validation
        bool CanRecruit(types::EntityID province_id, UnitType unit_type, population::SocialClass social_class);
        bool HasSufficientPopulation(types::EntityID province_id, population::SocialClass social_class, int needed);
        bool HasSufficientFunds(types::EntityID province_id, double cost);

        // ========================================================================
        // Configuration & Optimization
        // ========================================================================

        // System configuration
        void SetGlobalRecruitmentModifier(double modifier);
        void SetGlobalUpkeepModifier(double modifier);
        void SetMilitaryBudgetPercentage(double percentage);

    private:
        // Core system dependencies
        game::core::ComponentAccessManager& m_component_manager;
        game::core::MessageBus& m_message_bus;

        // Unit tracking
        std::unordered_map<types::EntityID, MilitaryUnit> m_all_units;
        types::EntityID m_next_unit_id{ 1 };

        // Update timing
        float m_accumulated_time = 0.0f;
        float m_recruitment_update_interval = 10.0f;  // Update recruitment every 10 seconds
        float m_training_update_interval = 5.0f;      // Update training every 5 seconds
        float m_training_accumulated_time = 0.0f;
        float m_monthly_timer = 0.0f;

        // System state
        bool m_initialized = false;

        // Configuration
        double m_global_recruitment_modifier = 1.0;
        double m_global_upkeep_modifier = 1.0;
        double m_global_training_modifier = 1.0;
        double m_military_budget_percentage = 0.3; // 30% of income for military

        // Unit type definitions
        struct UnitTypeDefinition {
            std::string name;
            std::vector<population::SocialClass> viable_classes;
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
        // Internal Implementation Methods
        // ========================================================================

        // Recruitment implementation
        MilitaryUnit CreateMilitaryUnit(types::EntityID province_id, UnitType unit_type,
            population::SocialClass social_class, int size);

        // Population interaction
        bool RecruitFromPopulation(types::EntityID province_id, UnitType unit_type,
            population::SocialClass social_class);
        bool CheckPopulationAvailability(types::EntityID province_id, population::SocialClass social_class, int needed);

        // Economic transactions
        void RemovePopulationForRecruitment(types::EntityID province_id, population::SocialClass social_class, int count);
        void ApplyRecruitmentEffectsToPopulation(types::EntityID province_id, population::SocialClass social_class, int recruited);

        // System maintenance
        void UpdateRecruitmentPools();
        void InitializeUnitDefinitions();
        void LoadMilitaryConfiguration();
        void SetupDefaultRecruitmentPools();
    };

} // namespace game::military