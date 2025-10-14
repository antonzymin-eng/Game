// ============================================================================
// MilitaryComponents.h - ECS Components for Military System
// Created: October 11, 2025 - ECS Integration Implementation
// Location: include/game/military/MilitaryComponents.h
// ============================================================================

#pragma once

#include "core/ECS/IComponent.h"
#include "core/types/game_types.h"
#include "game/population/PopulationTypes.h"
#include <vector>
#include <unordered_map>
#include <string>

namespace game::military {

    // ============================================================================
    // Military Type Definitions (moved from MilitarySystem.h)
    // ============================================================================

    enum class UnitType {
        // Infantry
        LEVIES = 0, SPEARMEN, CROSSBOWMEN, LONGBOWMEN, MEN_AT_ARMS, PIKEMEN, ARQUEBUSIERS, MUSKETEERS,
        // Cavalry  
        LIGHT_CAVALRY, HEAVY_CAVALRY, MOUNTED_ARCHERS, DRAGOONS,
        // Siege Equipment
        CATAPULTS, TREBUCHETS, CANNONS, SIEGE_TOWERS,
        // Naval Units
        GALLEYS, COGS, CARRACKS, GALLEONS, SHIPS_OF_THE_LINE,
        COUNT
    };

    enum class UnitClass {
        INFANTRY = 0, CAVALRY, SIEGE, NAVAL, COUNT
    };

    enum class MilitaryRank {
        SOLDIER = 0, SERGEANT, LIEUTENANT, CAPTAIN, MAJOR, COLONEL, GENERAL, MARSHAL, COUNT
    };

    enum class CombatRole {
        MELEE = 0, RANGED, SIEGE, SUPPORT, CAVALRY_CHARGE, SKIRMISH, COUNT
    };

    enum class MoraleState {
        ROUTING = 0, BROKEN, WAVERING, STEADY, CONFIDENT, FANATICAL, COUNT
    };

    // ============================================================================
    // Military Unit Data Structure
    // ============================================================================

    struct MilitaryUnit {
        UnitType type = UnitType::LEVIES;
        UnitClass unit_class = UnitClass::INFANTRY;
        CombatRole primary_role = CombatRole::MELEE;

        // Unit composition
        uint32_t max_strength = 1000;
        uint32_t current_strength = 1000;
        double experience = 0.0;
        double training = 0.5;

        // Equipment and supplies
        double equipment_quality = 0.5;
        double supply_level = 1.0;
        double ammunition = 1.0;

        // Morale and cohesion
        MoraleState morale = MoraleState::STEADY;
        double cohesion = 0.8;
        double loyalty = 0.8;

        // Combat statistics
        double attack_strength = 10.0;
        double defense_strength = 8.0;
        double movement_speed = 1.0;
        double range = 0.0;

        // Cost and maintenance
        double recruitment_cost = 100.0;
        double monthly_maintenance = 10.0;

        // Social composition
        game::population::SocialClass primary_class = game::population::SocialClass::FREE_PEASANTS;
        std::unordered_map<game::population::SocialClass, double> class_composition;

        MilitaryUnit() = default;
        explicit MilitaryUnit(UnitType unit_type);

        void ApplyLosses(uint32_t casualties);
        void RestoreStrength(uint32_t reinforcements);
        void UpdateMorale(double morale_change);
        void ConsumeSupplies(double consumption_rate);
        bool IsEffective() const;
        double GetCombatEffectiveness() const;
    };

    // ============================================================================
    // Commander Data Structure
    // ============================================================================

    struct Commander {
        std::string name;
        game::types::EntityID character_id = 0;
        MilitaryRank rank = MilitaryRank::CAPTAIN;

        // Command attributes
        double martial_skill = 0.5;
        double tactical_skill = 0.5;
        double strategic_skill = 0.5;
        double logistics_skill = 0.5;

        // Leadership qualities
        double charisma = 0.5;
        double loyalty = 0.8;
        double ambition = 0.5;

        // Experience and specialization
        double experience = 0.0;
        UnitClass specialty = UnitClass::INFANTRY;
        std::vector<std::string> traits;

        // Command capacity
        uint32_t command_limit = 5000;
        game::types::EntityID current_army = 0;

        Commander() = default;
        explicit Commander(const std::string& commander_name);

        double GetCommandEffectiveness(uint32_t army_size) const;
        double GetMoraleBonus() const;
        bool CanCommand(UnitType unit_type) const;
    };

    // ============================================================================
    // Military Component - Provincial military forces and infrastructure
    // ============================================================================

    struct MilitaryComponent : public game::core::Component<MilitaryComponent> {
        // Garrison forces
        std::vector<MilitaryUnit> garrison_units;
        
        // Active armies from this province
        std::vector<game::types::EntityID> active_armies;
        
        // Military infrastructure
        uint32_t recruitment_capacity = 500;
        double training_facilities = 0.3;
        double supply_infrastructure = 0.5;
        double barracks_level = 1.0;
        
        // Recruitment system
        std::unordered_map<game::population::SocialClass, uint32_t> available_recruits;
        std::unordered_map<UnitType, uint32_t> recruitment_quotas;
        std::unordered_map<UnitType, bool> unit_type_available;
        
        // Military spending
        double military_budget = 100.0;
        double recruitment_spending = 0.0;
        double maintenance_spending = 0.0;
        double equipment_spending = 0.0;
        double infrastructure_spending = 0.0;
        
        // Technology and equipment
        std::unordered_map<UnitType, double> equipment_quality_modifiers;
        std::vector<std::string> available_technologies;
        
        // Military effectiveness
        double overall_military_efficiency = 1.0;
        double recruitment_rate_modifier = 1.0;
        double training_effectiveness = 1.0;
        
        // Component methods
        bool CanRecruit(UnitType unit_type, uint32_t quantity) const;
        MilitaryUnit* CreateUnit(UnitType unit_type, uint32_t initial_strength);
        void DisbandUnit(size_t unit_index);
        double CalculateTotalMaintenance() const;
        uint32_t GetTotalGarrisonStrength() const;

        std::string GetComponentTypeName() const override {
            return "MilitaryComponent";
        }
    };

    // ============================================================================
    // Army Component - Mobile military forces
    // ============================================================================

    struct ArmyComponent : public game::core::Component<ArmyComponent> {
        std::string army_name;
        game::types::EntityID home_province = 0;
        game::types::EntityID current_location = 0;
        game::types::EntityID commander_id = 0;

        // Army composition
        std::vector<MilitaryUnit> units;
        uint32_t total_strength = 0;
        UnitClass dominant_unit_class = UnitClass::INFANTRY;
        
        // Army status
        double supply_level = 1.0;
        double movement_points = 100.0;
        double max_movement_points = 100.0;
        bool is_active = true;
        
        // Combat status
        bool is_in_battle = false;
        bool is_besieging = false;
        game::types::EntityID siege_target = 0;
        game::types::EntityID battle_id = 0;
        
        // Army characteristics
        double army_morale = 0.8;
        double organization = 0.8;
        double fatigue = 0.0;
        double cohesion = 0.8;
        
        // Supply and logistics
        double supply_consumption_rate = 1.0;
        double supply_range = 5.0;
        game::types::EntityID supply_source = 0;
        
        // Experience and training
        double collective_experience = 0.0;
        double battle_experience = 0.0;
        std::vector<std::string> battle_history;
        
        // Component methods
        explicit ArmyComponent(const std::string& name);
        void AddUnit(const MilitaryUnit& unit);
        void RemoveUnit(size_t unit_index);
        void RecalculateStrength();
        double GetCombatStrength() const;
        bool CanMove() const;
        
        std::string GetComponentTypeName() const override {
            return "ArmyComponent";
        }
    };

    // ============================================================================
    // Fortification Component - Defensive structures and capabilities
    // ============================================================================

    struct FortificationComponent : public game::core::Component<FortificationComponent> {
        // Fortification levels
        uint32_t walls_level = 1;
        uint32_t towers_level = 0;
        uint32_t gates_level = 1;
        uint32_t citadel_level = 0;
        uint32_t moat_level = 0;
        
        // Defensive capabilities
        double structural_integrity = 1.0;
        double siege_resistance = 1.0;
        double garrison_capacity_multiplier = 1.0;
        uint32_t garrison_capacity = 500;
        
        // Artillery and defensive equipment
        std::unordered_map<UnitType, uint32_t> defensive_artillery;
        double artillery_effectiveness = 1.0;
        
        // Supply and logistics
        double supply_storage_capacity = 1000.0;
        double current_supply_storage = 1000.0;
        double siege_endurance_days = 30.0;
        
        // Fortification status
        bool under_siege = false;
        game::types::EntityID besieging_army = 0;
        double siege_progress = 0.0;
        
        // Construction and maintenance
        double construction_progress = 0.0;
        double maintenance_level = 1.0;
        double upgrade_cost_modifier = 1.0;
        
        std::string GetComponentTypeName() const override {
            return "FortificationComponent";
        }
    };

    // ============================================================================
    // Combat Component - Active battle state and combat statistics
    // ============================================================================

    struct CombatComponent : public game::core::Component<CombatComponent> {
        // Battle identification
        game::types::EntityID battle_id = 0;
        std::string battle_name;
        game::types::EntityID location = 0;
        
        // Combatants
        game::types::EntityID attacker_army = 0;
        game::types::EntityID defender_army = 0;
        std::vector<game::types::EntityID> reinforcing_armies;
        
        // Battle state
        bool battle_active = false;
        double battle_duration = 0.0;
        double battle_intensity = 1.0;
        
        // Combat statistics
        uint32_t attacker_initial_strength = 0;
        uint32_t defender_initial_strength = 0;
        uint32_t attacker_casualties = 0;
        uint32_t defender_casualties = 0;
        
        // Battle factors
        double terrain_modifier = 1.0;
        double weather_modifier = 1.0;
        double fortification_bonus = 0.0;
        double commander_skill_difference = 0.0;
        
        // Battle phases
        std::string current_phase = "engagement"; // engagement, melee, pursuit
        double phase_progress = 0.0;
        
        // Morale tracking
        double attacker_morale = 1.0;
        double defender_morale = 1.0;
        
        // Battle outcome prediction
        double attacker_victory_chance = 0.5;
        std::string decisive_factors;
        
        std::string GetComponentTypeName() const override {
            return "CombatComponent";
        }
    };

    // ============================================================================
    // Military Events Component - Military events, campaigns, and battle history
    // ============================================================================

    struct MilitaryEventsComponent : public game::core::Component<MilitaryEventsComponent> {
        // Active military events
        std::vector<std::string> active_campaigns;
        std::vector<std::string> ongoing_sieges;
        std::vector<std::string> recruitment_drives;
        
        // Battle history
        std::vector<std::string> battle_history;
        std::vector<std::string> siege_history;
        std::vector<std::string> campaign_history;
        
        // Event frequency and timing
        double event_frequency_modifier = 1.0;
        int months_since_last_battle = 0;
        int months_since_last_recruitment = 0;
        
        // Military reputation and prestige  
        double military_reputation = 0.5;
        double battle_prestige = 0.0;
        double siege_expertise = 0.0;
        
        // Officer development
        std::vector<game::types::EntityID> promoted_officers;
        std::vector<game::types::EntityID> veteran_units;
        
        // Logistics events
        std::vector<std::string> supply_disruptions;
        std::vector<std::string> equipment_shortages;
        
        // Maximum history tracking
        int max_history_size = 100;
        
        std::string GetComponentTypeName() const override {
            return "MilitaryEventsComponent";
        }
    };

} // namespace game::military