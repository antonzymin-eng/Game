// ============================================================================
// Date/Time Created: Wednesday, September 24, 2025 - 2:45 PM PST
// Intended Folder Location: include/game/military/MilitarySystem.h
// MilitarySystem.h - Core Military System (Declarations Only)
// Dependencies: PopulationSystem, ProvinceSystem, TechnologySystem, ECS Foundation
// ============================================================================

#pragma once

#include "core/ECS/ISystem.h"
#include "core/ECS/ComponentAccessManager.h"
#include "core/ECS/MessageBus.h"
#include "core/Types/game_types.h"
#include "game/population/PopulationComponents.h"
#include "game/province/ProvinceComponents.h"
#include "game/technology/TechnologyComponents.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>

namespace game::military {

    // ============================================================================
    // Military Type Definitions
    // ============================================================================

    enum class UnitType {
        // Infantry
        LEVIES = 0,          // Peasant militia
        SPEARMEN,            // Professional spearmen
        CROSSBOWMEN,         // Crossbow infantry
        LONGBOWMEN,          // Longbow archers
        MEN_AT_ARMS,         // Elite infantry
        PIKEMEN,             // Anti-cavalry specialists
        ARQUEBUSIERS,        // Early firearms
        MUSKETEERS,          // Advanced firearms

        // Cavalry
        LIGHT_CAVALRY,       // Scouting and harassment
        HEAVY_CAVALRY,       // Knights and cataphracts
        MOUNTED_ARCHERS,     // Horse archers
        DRAGOONS,            // Mounted firearms

        // Siege Equipment
        CATAPULTS,           // Basic siege engines
        TREBUCHETS,          // Advanced siege engines
        CANNONS,             // Gunpowder artillery
        SIEGE_TOWERS,        // Assault equipment

        // Naval Units
        GALLEYS,             // Mediterranean warships
        COGS,                // Northern European ships
        CARRACKS,            // Ocean-going vessels
        GALLEONS,            // Advanced warships
        SHIPS_OF_THE_LINE,   // Naval artillery platforms

        COUNT
    };

    enum class UnitClass {
        INFANTRY = 0,
        CAVALRY,
        SIEGE,
        NAVAL,
        COUNT
    };

    enum class MilitaryRank {
        SOLDIER = 0,
        SERGEANT,
        LIEUTENANT,
        CAPTAIN,
        MAJOR,
        COLONEL,
        GENERAL,
        MARSHAL,
        COUNT
    };

    enum class CombatRole {
        MELEE = 0,           // Close combat
        RANGED,              // Archery and firearms
        SIEGE,               // Siege warfare
        SUPPORT,             // Logistics and engineering
        CAVALRY_CHARGE,      // Heavy cavalry shock
        SKIRMISH,            // Light troops harassment
        COUNT
    };

    enum class MoraleState {
        ROUTING = 0,         // Fleeing in panic
        BROKEN,              // Very low morale
        WAVERING,            // Unstable morale
        STEADY,              // Normal morale
        CONFIDENT,           // High morale
        FANATICAL,           // Maximum morale
        COUNT
    };

    // ============================================================================
    // Military Unit Structures
    // ============================================================================

    struct MilitaryUnit {
        UnitType type = UnitType::LEVIES;
        UnitClass unit_class = UnitClass::INFANTRY;
        CombatRole primary_role = CombatRole::MELEE;

        // Unit composition
        uint32_t max_strength = 1000;        // Maximum unit size
        uint32_t current_strength = 1000;    // Current personnel
        double experience = 0.0;             // 0.0-1.0 combat experience
        double training = 0.5;               // 0.0-1.0 training level

        // Equipment and supplies
        double equipment_quality = 0.5;      // 0.0-1.0 equipment condition
        double supply_level = 1.0;           // 0.0-1.0 supply state
        double ammunition = 1.0;             // 0.0-1.0 ammunition level

        // Morale and cohesion
        MoraleState morale = MoraleState::STEADY;
        double cohesion = 0.8;               // Unit discipline and organization
        double loyalty = 0.8;                // Loyalty to commander/realm

        // Combat statistics
        double attack_strength = 10.0;       // Base attack power
        double defense_strength = 8.0;       // Base defense power
        double movement_speed = 1.0;         // Tactical movement speed
        double range = 0.0;                  // Ranged attack range (0 for melee)

        // Cost and maintenance
        double recruitment_cost = 100.0;     // Initial recruitment cost
        double monthly_maintenance = 10.0;   // Monthly upkeep cost

        // Social composition
        types::SocialClass primary_class = types::SocialClass::FREE_PEASANT;
        std::unordered_map<types::SocialClass, double> class_composition;

        MilitaryUnit();
        explicit MilitaryUnit(UnitType unit_type);

        // Unit management methods
        void ApplyLosses(uint32_t casualties);
        void RestoreStrength(uint32_t reinforcements);
        void UpdateMorale(double morale_change);
        void ConsumeSupplies(double consumption_rate);
        bool IsEffective() const;
        double GetCombatEffectiveness() const;
    };

    struct Commander {
        std::string name;
        types::EntityID character_id = 0;
        MilitaryRank rank = MilitaryRank::CAPTAIN;

        // Command attributes
        double martial_skill = 0.5;          // Combat leadership ability
        double tactical_skill = 0.5;         // Battlefield tactics
        double strategic_skill = 0.5;        // Campaign planning
        double logistics_skill = 0.5;        // Supply management

        // Leadership qualities
        double charisma = 0.5;               // Morale impact on troops
        double loyalty = 0.8;                // Loyalty to realm
        double ambition = 0.5;               // Personal ambition level

        // Experience and specialization
        double experience = 0.0;             // Combat experience
        UnitClass specialty = UnitClass::INFANTRY; // Preferred unit type
        std::vector<std::string> traits;     // Special commander traits

        // Command capacity
        uint32_t command_limit = 5000;       // Maximum troops commanded effectively
        types::EntityID current_army = 0;    // Currently commanding army

        Commander();
        explicit Commander(const std::string& commander_name);

        // Command effectiveness methods
        double GetCommandEffectiveness(uint32_t army_size) const;
        double GetMoraleBonus() const;
        bool CanCommand(UnitType unit_type) const;
    };

    // ============================================================================
    // Military Components
    // ============================================================================

    struct MilitaryComponent : public core::ecs::IComponent<MilitaryComponent> {
        // Available units in province
        std::vector<MilitaryUnit> garrison_units;
        std::vector<types::EntityID> active_armies;

        // Military infrastructure
        uint32_t recruitment_capacity = 500;     // Monthly recruitment limit
        double training_facilities = 0.3;       // 0.0-1.0 training infrastructure
        double supply_infrastructure = 0.5;     // 0.0-1.0 logistics capability

        // Recruitment pools
        std::unordered_map<types::SocialClass, uint32_t> available_recruits;
        std::unordered_map<UnitType, uint32_t> recruitment_quotas;

        // Military spending
        double military_budget = 100.0;         // Monthly military budget
        double recruitment_spending = 0.0;      // Spending on new units
        double maintenance_spending = 0.0;      // Unit upkeep costs
        double equipment_spending = 0.0;        // Equipment and supplies

        // Fortifications
        uint32_t fortification_level = 1;       // Defensive structures level
        double garrison_effectiveness = 1.0;    // Defensive bonus
        bool under_siege = false;

        MilitaryComponent() = default;

        // Military management methods
        bool CanRecruit(UnitType unit_type, uint32_t quantity) const;
        MilitaryUnit* CreateUnit(UnitType unit_type, uint32_t initial_strength);
        void DisbandUnit(size_t unit_index);
        double CalculateTotalMaintenance() const;
        uint32_t GetTotalGarrisonStrength() const;
    };

    struct ArmyComponent : public core::ecs::IComponent<ArmyComponent> {
        std::string army_name;
        types::EntityID home_province = 0;
        types::EntityID current_location = 0;
        types::EntityID commander_id = 0;

        // Army composition
        std::vector<MilitaryUnit> units;
        uint32_t total_strength = 0;

        // Army status
        double supply_level = 1.0;              // Overall supply state
        double movement_points = 100.0;         // Movement capability
        bool is_besieging = false;
        types::EntityID siege_target = 0;

        // Combat readiness
        double army_morale = 0.8;               // Overall army morale
        double organization = 0.8;              // Command structure efficiency
        double fatigue = 0.0;                   // Exhaustion level

        ArmyComponent() = default;
        explicit ArmyComponent(const std::string& name);

        // Army management methods
        void AddUnit(const MilitaryUnit& unit);
        void RemoveUnit(size_t unit_index);
        void RecalculateStrength();
        double GetCombatStrength() const;
        bool CanMove() const;
        void Move(types::EntityID destination);
    };

    struct FortificationComponent : public core::ecs::IComponent<FortificationComponent> {
        uint32_t walls_level = 1;               // Wall fortification level
        uint32_t towers_level = 0;              // Tower defenses
        uint32_t gates_level = 1;               // Gate strength
        uint32_t citadel_level = 0;             // Inner keep

        // Defensive capabilities
        double structural_integrity = 1.0;      // Current condition
        double siege_resistance = 1.0;          // Resistance to siege
        uint32_t garrison_capacity = 500;       // Maximum garrison size

        // Supply and logistics
        double supply_storage = 1000.0;         // Stored supplies
        double siege_endurance = 30.0;          // Days of siege supplies

        FortificationComponent() = default;

        // Fortification management methods
        double GetDefensiveBonus() const;
        bool CanWithstandSiege() const;
        void TakeSiegeDamage(double damage);
        void RepairDamage(double repair_amount);
    };

    // ============================================================================
    // Military Events and Messages
    // ============================================================================

    namespace messages {

        struct UnitRecruited {
            types::EntityID province;
            UnitType unit_type;
            uint32_t unit_strength;
            types::SocialClass recruited_from;
            double recruitment_cost;
            std::string recruitment_details;
        };

        struct ArmyCreated {
            types::EntityID army_id;
            types::EntityID home_province;
            types::EntityID commander_id;
            std::string army_name;
            uint32_t initial_strength;
            std::vector<UnitType> unit_composition;
        };

        struct BattleResult {
            types::EntityID attacker_army;
            types::EntityID defender_army;
            types::EntityID battle_location;

            // Battle outcome
            std::string battle_name;
            bool attacker_victory;
            uint32_t attacker_casualties;
            uint32_t defender_casualties;

            // Battle details
            double battle_duration;              // Hours of combat
            std::string decisive_factors;        // What determined outcome
            std::vector<std::string> unit_performance; // Individual unit results
        };

        struct SiegeUpdate {
            types::EntityID besieging_army;
            types::EntityID besieged_province;

            // Siege progress
            double siege_progress;               // 0.0-1.0 progress to capture
            uint32_t siege_casualties;
            double fortification_damage;

            // Siege status
            std::string siege_phase;             // "approach", "investment", "assault"
            double estimated_duration;           // Days to completion
            bool siege_broken = false;
        };

        struct MoraleChange {
            types::EntityID unit_army;
            std::vector<size_t> affected_units;
            MoraleState old_morale;
            MoraleState new_morale;
            std::string morale_cause;
            double morale_impact;
        };

        struct MilitaryUpgrade {
            types::EntityID province;
            std::string upgrade_type;            // "training", "equipment", "fortification"
            double upgrade_cost;
            double effectiveness_improvement;
            std::string upgrade_description;
        };

    } // namespace messages

    // ============================================================================
    // Military System - Main Class Declaration
    // ============================================================================

    class MilitarySystem : public core::ecs::ISystem {
    public:
        MilitarySystem(core::ecs::ComponentAccessManager& access_manager,
            core::ecs::MessageBus& message_bus);
        virtual ~MilitarySystem() = default;

        // ISystem interface
        void Initialize() override;
        void Update(float delta_time) override;
        void Shutdown() override;

        // Unit management
        bool RecruitUnit(types::EntityID province_id, UnitType unit_type, uint32_t quantity);
        void DisbandUnit(types::EntityID province_id, size_t unit_index);
        void MergeUnits(types::EntityID province_id, size_t unit_a, size_t unit_b);
        void SplitUnit(types::EntityID province_id, size_t unit_index, uint32_t split_size);

        // Army management
        types::EntityID CreateArmy(types::EntityID home_province, const std::string& army_name,
            const std::vector<size_t>& unit_indices);
        void DisbandArmy(types::EntityID army_id);
        void MoveArmy(types::EntityID army_id, types::EntityID destination);
        void AssignCommander(types::EntityID army_id, types::EntityID commander_id);

        // Combat operations
        void InitiateBattle(types::EntityID attacker_army, types::EntityID defender_army);
        void ProcessBattle(types::EntityID battle_id, float battle_duration);
        void ResolveBattle(types::EntityID battle_id);

        // Siege operations
        void BeginSiege(types::EntityID besieging_army, types::EntityID target_province);
        void ProcessSiege(types::EntityID siege_id, float time_delta);
        void ResolveSiege(types::EntityID siege_id, bool attacker_success);

        // Military development
        void UpgradeTrainingFacilities(types::EntityID province_id, double investment);
        void ImproveEquipment(types::EntityID province_id, UnitType unit_type, double investment);
        void ConstructFortifications(types::EntityID province_id, uint32_t fortification_type, double cost);

        // Technology integration
        void ApplyMilitaryTechnology(types::EntityID province_id, technology::TechnologyType tech);
        void UpdateTechnologyEffects(types::EntityID province_id);
        std::vector<UnitType> GetAvailableUnitTypes(types::EntityID province_id) const;

        // Morale and leadership
        void UpdateUnitMorale(types::EntityID army_id, double morale_change, const std::string& cause);
        void ProcessCommanderEffects(types::EntityID army_id);
        void HandleDesertion(types::EntityID army_id);

        // Supply and logistics
        void UpdateSupplyLines(types::EntityID army_id);
        void ProcessSupplyConsumption(types::EntityID army_id, float time_delta);
        void ResupplyArmy(types::EntityID army_id, types::EntityID supply_source);

        // Query methods
        std::vector<types::EntityID> GetAllArmies() const;
        std::vector<types::EntityID> GetProvincialGarrisons() const;
        uint32_t GetTotalMilitaryStrength(types::EntityID province_id) const;
        double GetMilitaryMaintenance(types::EntityID province_id) const;

        // Integration with other systems
        void ProcessMilitaryRecruitment(types::EntityID province_id);
        void UpdateMilitaryBudget(types::EntityID province_id, double budget_allocation);
        void IntegrateWithPopulation(types::EntityID province_id);
        void IntegrateWithEconomy(types::EntityID province_id);

        // Configuration
        void SetCombatResolutionSpeed(double speed_multiplier);
        void SetMoraleFactors(double base_morale, double leadership_impact);
        void SetSupplyConsumptionRate(double consumption_multiplier);

        // Data access
        MilitaryComponent* GetMilitaryComponent(types::EntityID province_id);
        const MilitaryComponent* GetMilitaryComponent(types::EntityID province_id) const;
        ArmyComponent* GetArmyComponent(types::EntityID army_id);
        const ArmyComponent* GetArmyComponent(types::EntityID army_id) const;

    private:
        core::ecs::ComponentAccessManager& m_access_manager;
        core::ecs::MessageBus& m_message_bus;

        // System state
        bool m_initialized = false;
        float m_accumulated_time = 0.0f;
        float m_update_interval = 1.0f;      // Update every second
        float m_monthly_timer = 0.0f;        // Monthly military processing

        // Military configuration
        std::unordered_map<UnitType, MilitaryUnit> m_unit_templates;
        std::unordered_map<technology::TechnologyType, std::vector<UnitType>> m_tech_unlocks;

        // Combat resolution
        double m_combat_speed_multiplier = 1.0;
        double m_base_morale_impact = 1.0;
        double m_supply_consumption_rate = 1.0;

        // Active military operations
        std::vector<types::EntityID> m_active_battles;
        std::vector<types::EntityID> m_active_sieges;

        // Internal helper methods (declarations only)
        void InitializeUnitTemplates();
        void InitializeTechnologyUnlocks();
        void SubscribeToEvents();

        // Recruitment calculations
        bool CanRecruitFromClass(types::EntityID province_id, types::SocialClass social_class,
            UnitType unit_type, uint32_t quantity);
        double CalculateRecruitmentCost(UnitType unit_type, uint32_t quantity,
            types::SocialClass social_class);

        // Combat calculations
        double CalculateUnitEffectiveness(const MilitaryUnit& unit, const Commander* commander);
        double CalculateTerrainModifier(types::EntityID battle_location, UnitType unit_type);
        double CalculateMoraleImpact(MoraleState attacker_morale, MoraleState defender_morale);

        // Siege calculations
        double CalculateSiegeProgress(const ArmyComponent& besieging_army,
            const FortificationComponent& fortifications);
        double CalculateSiegeAttrition(const ArmyComponent& army, double siege_duration);

        // Supply calculations
        double CalculateSupplyRequirement(const ArmyComponent& army);
        double CalculateSupplyDistance(types::EntityID army_location, types::EntityID supply_source);
        bool IsSupplyRouteSecure(types::EntityID source, types::EntityID destination);

        // Technology effects
        void ApplyTechnologyToUnit(MilitaryUnit& unit, technology::TechnologyType tech);
        std::vector<technology::TechnologyType> GetRelevantTechnologies(UnitType unit_type);

        // Event handlers
        void OnPopulationChanged(const core::ecs::Message& message);
        void OnTechnologyDiscovered(const core::ecs::Message& message);
        void OnEconomicUpdate(const core::ecs::Message& message);
        void OnBuildingConstructed(const core::ecs::Message& message);

        // Utility methods
        void LogMilitaryEvent(types::EntityID province_id, const std::string& event_description);
        void ValidateMilitaryState(types::EntityID province_id);
    };

    // ============================================================================
    // Military Database and Utility Functions (Declarations Only)
    // ============================================================================

    namespace database {
        // Unit template creation
        MilitaryUnit CreateInfantryUnit(UnitType type, types::SocialClass recruitment_class);
        MilitaryUnit CreateCavalryUnit(UnitType type, types::SocialClass recruitment_class);
        MilitaryUnit CreateSiegeUnit(UnitType type, types::SocialClass recruitment_class);
        MilitaryUnit CreateNavalUnit(UnitType type, types::SocialClass recruitment_class);

        // Commander generation
        Commander GenerateCommander(const std::string& name, types::SocialClass social_class,
            MilitaryRank rank);

        // Technology effects on military units
        namespace effects {
            constexpr const char* UNIT_ATTACK = "unit_attack";
            constexpr const char* UNIT_DEFENSE = "unit_defense";
            constexpr const char* UNIT_MORALE = "unit_morale";
            constexpr const char* RECRUITMENT_COST = "recruitment_cost";
            constexpr const char* MAINTENANCE_COST = "maintenance_cost";
            constexpr const char* SIEGE_EFFICIENCY = "siege_efficiency";
            constexpr const char* FORTIFICATION_STRENGTH = "fortification_strength";
        }
    }

    namespace utils {
        // String conversion utilities
        std::string UnitTypeToString(UnitType type);
        std::string UnitClassToString(UnitClass unit_class);
        std::string MilitaryRankToString(MilitaryRank rank);
        std::string CombatRoleToString(CombatRole role);
        std::string MoraleStateToString(MoraleState morale);

        // Military analysis utilities
        bool IsUnitTypeAvailable(UnitType type, int current_year);
        bool RequiresTechnology(UnitType type, technology::TechnologyType tech);
        UnitClass GetUnitClass(UnitType type);
        CombatRole GetPrimaryCombatRole(UnitType type);

        // Combat effectiveness calculations
        double CalculateUnitMatchup(UnitType attacker, UnitType defender);
        double GetTerrainAdvantage(UnitType unit_type, const std::string& terrain_type);
        double GetSeasonalModifier(UnitType unit_type, int current_month);

        // Resource requirements
        std::vector<types::ResourceType> GetUnitResourceRequirements(UnitType type);
        double GetUnitResourceConsumption(UnitType type, types::ResourceType resource);

        // Historical availability
        int GetHistoricalIntroductionYear(UnitType type);
        int GetHistoricalObsolescenceYear(UnitType type);
        bool IsHistoricallyAccurate(UnitType type, int year);
    }

} // namespace game::military