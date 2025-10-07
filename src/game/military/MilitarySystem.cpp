// ============================================================================
// Date/Time Created: Wednesday, September 24, 2025 - 3:30 PM PST
// Intended Folder Location: src/game/military/MilitarySystem.cpp
// MilitarySystem.cpp - Core Military System Implementation
// ============================================================================

#include "game/military/MilitarySystem.h"
#include "game/population/PopulationComponents.h"
#include "game/province/ProvinceComponents.h"
#include "game/technology/TechnologyComponents.h"
#include "core/config/GameConfig.h"
#include <iostream>
#include <algorithm>
#include <random>
#include <cmath>

namespace game::military {

    // ============================================================================
    // MilitaryUnit Implementation
    // ============================================================================

    MilitaryUnit::MilitaryUnit() = default;

    MilitaryUnit::MilitaryUnit(UnitType unit_type) : type(unit_type) {
        unit_class = utils::GetUnitClass(unit_type);
        primary_role = utils::GetPrimaryCombatRole(unit_type);
        
        // Set default stats based on unit type
        switch (unit_type) {
            case UnitType::LEVIES:
                max_strength = 1000;
                attack_strength = 6.0;
                defense_strength = 5.0;
                movement_speed = 1.0;
                recruitment_cost = 50.0;
                monthly_maintenance = 5.0;
                primary_class = types::SocialClass::PEASANTS;
                break;
                
            case UnitType::SPEARMEN:
                max_strength = 800;
                attack_strength = 8.0;
                defense_strength = 10.0;
                movement_speed = 0.9;
                recruitment_cost = 80.0;
                monthly_maintenance = 8.0;
                primary_class = types::SocialClass::FREE_PEASANT;
                break;
                
            case UnitType::CROSSBOWMEN:
                max_strength = 600;
                attack_strength = 12.0;
                defense_strength = 7.0;
                movement_speed = 0.8;
                range = 200.0;
                recruitment_cost = 120.0;
                monthly_maintenance = 12.0;
                primary_class = types::SocialClass::CRAFTSMEN;
                break;
                
            case UnitType::HEAVY_CAVALRY:
                max_strength = 300;
                attack_strength = 18.0;
                defense_strength = 15.0;
                movement_speed = 2.0;
                recruitment_cost = 300.0;
                monthly_maintenance = 25.0;
                primary_class = types::SocialClass::NOBILITY;
                break;
                
            case UnitType::CANNONS:
                max_strength = 100;
                attack_strength = 25.0;
                defense_strength = 5.0;
                movement_speed = 0.3;
                range = 500.0;
                recruitment_cost = 800.0;
                monthly_maintenance = 40.0;
                primary_class = types::SocialClass::CRAFTSMEN;
                break;
                
            default:
                max_strength = 500;
                attack_strength = 10.0;
                defense_strength = 8.0;
                movement_speed = 1.0;
                recruitment_cost = 100.0;
                monthly_maintenance = 10.0;
                primary_class = types::SocialClass::FREE_PEASANT;
                break;
        }
        
        current_strength = max_strength;
        
        // Initialize class composition
        class_composition[primary_class] = 1.0;
    }

    void MilitaryUnit::ApplyLosses(uint32_t casualties) {
        casualties = std::min(casualties, current_strength);
        current_strength -= casualties;
        
        // Morale impact from casualties
        double casualty_ratio = static_cast<double>(casualties) / max_strength;
        UpdateMorale(-casualty_ratio * 0.3);
        
        // Cohesion degradation
        cohesion = std::max(0.1, cohesion - casualty_ratio * 0.2);
    }

    void MilitaryUnit::RestoreStrength(uint32_t reinforcements) {
        current_strength = std::min(max_strength, current_strength + reinforcements);
        
        // Slight morale boost from reinforcements
        double reinforcement_ratio = static_cast<double>(reinforcements) / max_strength;
        UpdateMorale(reinforcement_ratio * 0.1);
    }

    void MilitaryUnit::UpdateMorale(double morale_change) {
        int old_morale_state = static_cast<int>(morale);
        
        switch (morale) {
            case MoraleState::ROUTING:
                if (morale_change > 0.3) morale = MoraleState::BROKEN;
                break;
            case MoraleState::BROKEN:
                if (morale_change > 0.2) morale = MoraleState::WAVERING;
                else if (morale_change < -0.1) morale = MoraleState::ROUTING;
                break;
            case MoraleState::WAVERING:
                if (morale_change > 0.2) morale = MoraleState::STEADY;
                else if (morale_change < -0.2) morale = MoraleState::BROKEN;
                break;
            case MoraleState::STEADY:
                if (morale_change > 0.3) morale = MoraleState::CONFIDENT;
                else if (morale_change < -0.2) morale = MoraleState::WAVERING;
                break;
            case MoraleState::CONFIDENT:
                if (morale_change > 0.4) morale = MoraleState::FANATICAL;
                else if (morale_change < -0.3) morale = MoraleState::STEADY;
                break;
            case MoraleState::FANATICAL:
                if (morale_change < -0.4) morale = MoraleState::CONFIDENT;
                break;
        }
    }

    void MilitaryUnit::ConsumeSupplies(double consumption_rate) {
        supply_level = std::max(0.0, supply_level - consumption_rate);
        
        // Low supplies affect performance
        if (supply_level < 0.3) {
            UpdateMorale(-0.1);
        }
    }

    bool MilitaryUnit::IsEffective() const {
        return current_strength > max_strength * 0.1 && 
               morale != MoraleState::ROUTING && 
               morale != MoraleState::BROKEN;
    }

    double MilitaryUnit::GetCombatEffectiveness() const {
        double effectiveness = 1.0;
        
        // Strength factor
        double strength_ratio = static_cast<double>(current_strength) / max_strength;
        effectiveness *= (0.3 + 0.7 * strength_ratio);
        
        // Experience factor
        effectiveness *= (0.5 + 0.5 * experience);
        
        // Training factor
        effectiveness *= (0.6 + 0.4 * training);
        
        // Equipment factor
        effectiveness *= (0.4 + 0.6 * equipment_quality);
        
        // Supply factor
        effectiveness *= (0.5 + 0.5 * supply_level);
        
        // Morale factor
        double morale_multiplier = 1.0;
        switch (morale) {
            case MoraleState::ROUTING: morale_multiplier = 0.1; break;
            case MoraleState::BROKEN: morale_multiplier = 0.3; break;
            case MoraleState::WAVERING: morale_multiplier = 0.7; break;
            case MoraleState::STEADY: morale_multiplier = 1.0; break;
            case MoraleState::CONFIDENT: morale_multiplier = 1.3; break;
            case MoraleState::FANATICAL: morale_multiplier = 1.6; break;
        }
        effectiveness *= morale_multiplier;
        
        return effectiveness;
    }

    // ============================================================================
    // Commander Implementation
    // ============================================================================

    Commander::Commander() = default;

    Commander::Commander(const std::string& commander_name) : name(commander_name) {
        // Generate random skills (0.3-0.8 range for realism)
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> skill_dist(0.3, 0.8);
        
        martial_skill = skill_dist(gen);
        tactical_skill = skill_dist(gen);
        strategic_skill = skill_dist(gen);
        logistics_skill = skill_dist(gen);
        charisma = skill_dist(gen);
        
        // Command limit based on rank
        switch (rank) {
            case MilitaryRank::CAPTAIN: command_limit = 1000; break;
            case MilitaryRank::MAJOR: command_limit = 2500; break;
            case MilitaryRank::COLONEL: command_limit = 5000; break;
            case MilitaryRank::GENERAL: command_limit = 10000; break;
            case MilitaryRank::MARSHAL: command_limit = 25000; break;
            default: command_limit = 500; break;
        }
    }

    double Commander::GetCommandEffectiveness(uint32_t army_size) const {
        double effectiveness = 1.0;
        
        // Command capacity factor
        if (army_size > command_limit) {
            double overflow_ratio = static_cast<double>(army_size) / command_limit;
            effectiveness *= (1.0 / overflow_ratio);
        }
        
        // Skill factor (average of relevant skills)
        double skill_average = (martial_skill + tactical_skill + strategic_skill + logistics_skill) / 4.0;
        effectiveness *= (0.5 + 0.5 * skill_average);
        
        return std::max(0.1, effectiveness);
    }

    double Commander::GetMoraleBonus() const {
        return charisma * 0.2; // Up to 20% morale bonus
    }

    bool Commander::CanCommand(UnitType unit_type) const {
        UnitClass unit_class = utils::GetUnitClass(unit_type);
        
        // Specialty bonus
        if (unit_class == specialty) {
            return true;
        }
        
        // High-ranking commanders can command any unit type
        return rank >= MilitaryRank::COLONEL;
    }

    // ============================================================================
    // MilitaryComponent Implementation
    // ============================================================================

    bool MilitaryComponent::CanRecruit(UnitType unit_type, uint32_t quantity) const {
        // Check recruitment capacity
        uint32_t MilitaryComponent::GetTotalGarrisonStrength() const {
        uint32_t total = 0;
        for (const auto& unit : garrison_units) {
            total += unit.current_strength;
        }
        return total;
    }

    // ============================================================================
    // ArmyComponent Implementation
    // ============================================================================

    ArmyComponent::ArmyComponent(const std::string& name) : army_name(name) {
        RecalculateStrength();
    }

    void ArmyComponent::AddUnit(const MilitaryUnit& unit) {
        units.push_back(unit);
        RecalculateStrength();
    }

    void ArmyComponent::RemoveUnit(size_t unit_index) {
        if (unit_index >= units.size()) {
            return;
        }
        
        units.erase(units.begin() + unit_index);
        RecalculateStrength();
    }

    void ArmyComponent::RecalculateStrength() {
        total_strength = 0;
        for (const auto& unit : units) {
            total_strength += unit.current_strength;
        }
    }

    double ArmyComponent::GetCombatStrength() const {
        double total_combat_strength = 0.0;
        
        for (const auto& unit : units) {
            total_combat_strength += unit.GetCombatEffectiveness() * unit.current_strength;
        }
        
        // Apply army-level modifiers
        total_combat_strength *= army_morale;
        total_combat_strength *= organization;
        total_combat_strength *= (1.0 - fatigue);
        
        return total_combat_strength;
    }

    bool ArmyComponent::CanMove() const {
        return movement_points > 0 && 
               organization > 0.3 && 
               !is_besieging &&
               army_morale != MoraleState::ROUTING &&
               army_morale != MoraleState::BROKEN;
    }

    void ArmyComponent::Move(types::EntityID destination) {
        if (!CanMove()) {
            return;
        }
        
        current_location = destination;
        movement_points = std::max(0.0, movement_points - 10.0); // Base movement cost
        fatigue += 0.1; // Movement causes fatigue
    }

    // ============================================================================
    // FortificationComponent Implementation
    // ============================================================================

    double FortificationComponent::GetDefensiveBonus() const {
        double bonus = 1.0;
        
        bonus += walls_level * 0.2;
        bonus += towers_level * 0.15;
        bonus += gates_level * 0.1;
        bonus += citadel_level * 0.3;
        
        // Structural integrity affects bonus
        bonus *= structural_integrity;
        
        return bonus;
    }

    bool FortificationComponent::CanWithstandSiege() const {
        return structural_integrity > 0.3 && 
               supply_storage > 0 && 
               siege_endurance > 0;
    }

    void FortificationComponent::TakeSiegeDamage(double damage) {
        structural_integrity = std::max(0.0, structural_integrity - damage);
        
        // Damage affects various levels
        if (structural_integrity < 0.8) {
            walls_level = std::max(0u, walls_level - 1);
        }
        if (structural_integrity < 0.6) {
            towers_level = std::max(0u, towers_level - 1);
        }
        if (structural_integrity < 0.4) {
            gates_level = std::max(0u, gates_level - 1);
        }
    }

    void FortificationComponent::RepairDamage(double repair_amount) {
        structural_integrity = std::min(1.0, structural_integrity + repair_amount);
    }

    // ============================================================================
    // MilitarySystem Constructor & Core Interface
    // ============================================================================

    MilitarySystem::MilitarySystem(core::ecs::ComponentAccessManager& access_manager,
                                 core::ecs::MessageBus& message_bus)
        : m_access_manager(access_manager)
        , m_message_bus(message_bus) {
    }

    void MilitarySystem::Initialize() {
        std::cout << "Initializing Military System..." << std::endl;
        
        InitializeUnitTemplates();
        InitializeTechnologyUnlocks();
        SubscribeToEvents();
        
        m_initialized = true;
        std::cout << "Military System initialized successfully" << std::endl;
    }

    void MilitarySystem::Update(float delta_time) {
        if (!m_initialized) {
            return;
        }
        
        m_accumulated_time += delta_time;
        m_monthly_timer += delta_time;
        
        // Regular updates every second
        if (m_accumulated_time >= m_update_interval) {
            // Update all armies
            auto army_entities = m_access_manager.GetEntitiesWithComponent<ArmyComponent>();
            for (auto entity : army_entities) {
                UpdateSupplyLines(entity);
                ProcessSupplyConsumption(entity, m_accumulated_time);
                ProcessCommanderEffects(entity);
            }
            
            // Process active battles
            for (auto battle_id : m_active_battles) {
                ProcessBattle(battle_id, m_accumulated_time);
            }
            
            // Process active sieges
            for (auto siege_id : m_active_sieges) {
                ProcessSiege(siege_id, m_accumulated_time);
            }
            
            m_accumulated_time = 0.0f;
        }
        
        // Monthly military processing
        if (m_monthly_timer >= 30.0f) { // Simulate 30 seconds = 1 month
            auto military_entities = m_access_manager.GetEntitiesWithComponent<MilitaryComponent>();
            for (auto entity : military_entities) {
                ProcessMilitaryRecruitment(entity);
                IntegrateWithPopulation(entity);
                IntegrateWithEconomy(entity);
            }
            
            m_monthly_timer = 0.0f;
        }
    }

    void MilitarySystem::Shutdown() {
        std::cout << "Shutting down Military System..." << std::endl;
        m_unit_templates.clear();
        m_tech_unlocks.clear();
        m_active_battles.clear();
        m_active_sieges.clear();
        m_initialized = false;
    }

    // ============================================================================
    // Unit Management Implementation
    // ============================================================================

    bool MilitarySystem::RecruitUnit(types::EntityID province_id, UnitType unit_type, uint32_t quantity) {
        auto* military_comp = GetMilitaryComponent(province_id);
        if (!military_comp) {
            std::cerr << "No military component found for province " << province_id << std::endl;
            return false;
        }
        
        // Check if recruitment is possible
        if (!military_comp->CanRecruit(unit_type, quantity)) {
            std::cerr << "Cannot recruit " << quantity << " units of type " << 
                utils::UnitTypeToString(unit_type) << " in province " << province_id << std::endl;
            return false;
        }
        
        // Get population component to check available recruits
        auto* population_comp = m_access_manager.GetComponent<population::PopulationComponent>(province_id);
        if (!population_comp) {
            std::cerr << "No population component found for recruitment" << std::endl;
            return false;
        }
        
        // Determine recruitment social class
        auto unit_template = m_unit_templates[unit_type];
        auto recruitment_class = unit_template.primary_class;
        
        // Check if enough population is available
        auto available_population = population_comp->GetPopulationByClass(recruitment_class);
        if (available_population < quantity) {
            std::cerr << "Insufficient population for recruitment" << std::endl;
            return false;
        }
        
        // Calculate recruitment cost
        double total_cost = CalculateRecruitmentCost(unit_type, quantity, recruitment_class);
        
        // Create the unit
        auto* new_unit = military_comp->CreateUnit(unit_type, quantity);
        if (!new_unit) {
            std::cerr << "Failed to create military unit" << std::endl;
            return false;
        }
        
        // Remove population
        population_comp->RemovePopulation(recruitment_class, quantity);
        
        // Publish recruitment event
        messages::UnitRecruited recruitment_msg;
        recruitment_msg.province = province_id;
        recruitment_msg.unit_type = unit_type;
        recruitment_msg.unit_strength = quantity;
        recruitment_msg.recruited_from = recruitment_class;
        recruitment_msg.recruitment_cost = total_cost;
        recruitment_msg.recruitment_details = "Standard recruitment from " + 
            utils::UnitTypeToString(unit_type);
        
        m_message_bus.Publish(recruitment_msg);
        
        LogMilitaryEvent(province_id, "Recruited " + std::to_string(quantity) + " " + 
                        utils::UnitTypeToString(unit_type));
        
        return true;
    }

    void MilitarySystem::DisbandUnit(types::EntityID province_id, size_t unit_index) {
        auto* military_comp = GetMilitaryComponent(province_id);
        if (!military_comp) {
            return;
        }
        
        if (unit_index >= military_comp->garrison_units.size()) {
            std::cerr << "Invalid unit index for disbandment" << std::endl;
            return;
        }
        
        const auto& unit = military_comp->garrison_units[unit_index];
        
        // Return population (some casualties expected)
        auto* population_comp = m_access_manager.GetComponent<population::PopulationComponent>(province_id);
        if (population_comp) {
            uint32_t returning_population = static_cast<uint32_t>(unit.current_strength * 0.9);
            population_comp->AddPopulation(unit.primary_class, returning_population);
        }
        
        LogMilitaryEvent(province_id, "Disbanded " + utils::UnitTypeToString(unit.type) + 
                        " unit with " + std::to_string(unit.current_strength) + " soldiers");
        
        military_comp->DisbandUnit(unit_index);
    }

    void MilitarySystem::MergeUnits(types::EntityID province_id, size_t unit_a, size_t unit_b) {
        auto* military_comp = GetMilitaryComponent(province_id);
        if (!military_comp) {
            return;
        }
        
        auto& units = military_comp->garrison_units;
        if (unit_a >= units.size() || unit_b >= units.size() || unit_a == unit_b) {
            std::cerr << "Invalid unit indices for merging" << std::endl;
            return;
        }
        
        auto& unit_primary = units[unit_a];
        auto& unit_secondary = units[unit_b];
        
        // Can only merge units of same type
        if (unit_primary.type != unit_secondary.type) {
            std::cerr << "Cannot merge units of different types" << std::endl;
            return;
        }
        
        // Merge soldiers
        uint32_t total_strength = unit_primary.current_strength + unit_secondary.current_strength;
        uint32_t merged_strength = std::min(total_strength, unit_primary.max_strength);
        
        // Calculate weighted averages for quality metrics
        double total_current = unit_primary.current_strength + unit_secondary.current_strength;
        
        unit_primary.experience = (unit_primary.experience * unit_primary.current_strength + 
                                  unit_secondary.experience * unit_secondary.current_strength) / total_current;
        
        unit_primary.training = (unit_primary.training * unit_primary.current_strength + 
                               unit_secondary.training * unit_secondary.current_strength) / total_current;
        
        unit_primary.equipment_quality = (unit_primary.equipment_quality * unit_primary.current_strength + 
                                        unit_secondary.equipment_quality * unit_secondary.current_strength) / total_current;
        
        unit_primary.current_strength = merged_strength;
        
        // Remove the secondary unit
        units.erase(units.begin() + unit_b);
        
        LogMilitaryEvent(province_id, "Merged two " + utils::UnitTypeToString(unit_primary.type) + 
                        " units into single unit of " + std::to_string(merged_strength) + " soldiers");
    }

    void MilitarySystem::SplitUnit(types::EntityID province_id, size_t unit_index, uint32_t split_size) {
        auto* military_comp = GetMilitaryComponent(province_id);
        if (!military_comp) {
            return;
        }
        
        auto& units = military_comp->garrison_units;
        if (unit_index >= units.size()) {
            std::cerr << "Invalid unit index for splitting" << std::endl;
            return;
        }
        
        auto& original_unit = units[unit_index];
        
        if (split_size >= original_unit.current_strength) {
            std::cerr << "Cannot split unit - split size too large" << std::endl;
            return;
        }
        
        // Create new unit with same properties
        MilitaryUnit new_unit = original_unit;
        new_unit.current_strength = split_size;
        
        // Reduce original unit size
        original_unit.current_strength -= split_size;
        
        // Add new unit to garrison
        units.push_back(new_unit);
        
        LogMilitaryEvent(province_id, "Split " + utils::UnitTypeToString(original_unit.type) + 
                        " unit - created new unit with " + std::to_string(split_size) + " soldiers");
    }

    // ============================================================================
    // Stub Implementations for Required Methods
    // ============================================================================

    types::EntityID MilitarySystem::CreateArmy(types::EntityID home_province, const std::string& army_name,
                                              const std::vector<size_t>& unit_indices) {
        // Implementation would create new army entity and transfer units
        LogMilitaryEvent(home_province, "Created army: " + army_name);
        return 0; // Return new army ID
    }

    void MilitarySystem::DisbandArmy(types::EntityID army_id) {
        LogMilitaryEvent(0, "Disbanded army: " + std::to_string(army_id));
    }

    void MilitarySystem::MoveArmy(types::EntityID army_id, types::EntityID destination) {
        LogMilitaryEvent(destination, "Army " + std::to_string(army_id) + " moved to province");
    }

    void MilitarySystem::AssignCommander(types::EntityID army_id, types::EntityID commander_id) {
        LogMilitaryEvent(0, "Assigned commander " + std::to_string(commander_id) + " to army " + std::to_string(army_id));
    }

    // Combat and siege operations (stubs)
    void MilitarySystem::InitiateBattle(types::EntityID attacker_army, types::EntityID defender_army) {}
    void MilitarySystem::ProcessBattle(types::EntityID battle_id, float battle_duration) {}
    void MilitarySystem::ResolveBattle(types::EntityID battle_id) {}
    void MilitarySystem::BeginSiege(types::EntityID besieging_army, types::EntityID target_province) {}
    void MilitarySystem::ProcessSiege(types::EntityID siege_id, float time_delta) {}
    void MilitarySystem::ResolveSiege(types::EntityID siege_id, bool attacker_success) {}

    // Military development (stubs)
    void MilitarySystem::UpgradeTrainingFacilities(types::EntityID province_id, double investment) {}
    void MilitarySystem::ImproveEquipment(types::EntityID province_id, UnitType unit_type, double investment) {}
    void MilitarySystem::ConstructFortifications(types::EntityID province_id, uint32_t fortification_type, double cost) {}

    // Technology integration (stubs)
    void MilitarySystem::ApplyMilitaryTechnology(types::EntityID province_id, technology::TechnologyType tech) {}
    void MilitarySystem::UpdateTechnologyEffects(types::EntityID province_id) {}
    std::vector<UnitType> MilitarySystem::GetAvailableUnitTypes(types::EntityID province_id) const { return {}; }

    // Morale and leadership (stubs)
    void MilitarySystem::UpdateUnitMorale(types::EntityID army_id, double morale_change, const std::string& cause) {}
    void MilitarySystem::ProcessCommanderEffects(types::EntityID army_id) {}
    void MilitarySystem::HandleDesertion(types::EntityID army_id) {}

    // Supply and logistics (stubs)
    void MilitarySystem::UpdateSupplyLines(types::EntityID army_id) {}
    void MilitarySystem::ProcessSupplyConsumption(types::EntityID army_id, float time_delta) {}
    void MilitarySystem::ResupplyArmy(types::EntityID army_id, types::EntityID supply_source) {}

    // Query methods
    std::vector<types::EntityID> MilitarySystem::GetAllArmies() const { return {}; }
    std::vector<types::EntityID> MilitarySystem::GetProvincialGarrisons() const { return {}; }

    uint32_t MilitarySystem::GetTotalMilitaryStrength(types::EntityID province_id) const {
        auto* military_comp = GetMilitaryComponent(province_id);
        return military_comp ? military_comp->GetTotalGarrisonStrength() : 0;
    }

    double MilitarySystem::GetMilitaryMaintenance(types::EntityID province_id) const {
        auto* military_comp = GetMilitaryComponent(province_id);
        return military_comp ? military_comp->CalculateTotalMaintenance() : 0.0;
    }

    // Integration with other systems (stubs)
    void MilitarySystem::ProcessMilitaryRecruitment(types::EntityID province_id) {}
    void MilitarySystem::UpdateMilitaryBudget(types::EntityID province_id, double budget_allocation) {}
    void MilitarySystem::IntegrateWithPopulation(types::EntityID province_id) {}
    void MilitarySystem::IntegrateWithEconomy(types::EntityID province_id) {}

    // Configuration (stubs)
    void MilitarySystem::SetCombatResolutionSpeed(double speed_multiplier) { m_combat_speed_multiplier = speed_multiplier; }
    void MilitarySystem::SetMoraleFactors(double base_morale, double leadership_impact) { m_base_morale_impact = base_morale; }
    void MilitarySystem::SetSupplyConsumptionRate(double consumption_multiplier) { m_supply_consumption_rate = consumption_multiplier; }

    // ============================================================================
    // Helper Method Implementations
    // ============================================================================

    double MilitarySystem::CalculateRecruitmentCost(UnitType unit_type, uint32_t quantity, 
                                                   types::SocialClass social_class) {
        auto it = m_unit_templates.find(unit_type);
        if (it == m_unit_templates.end()) {
            return 100.0 * quantity; // Default cost
        }
        
        double base_cost = it->second.recruitment_cost;
        
        // Social class modifiers
        switch (social_class) {
            case types::SocialClass::NOBILITY:
                base_cost *= 2.0; // Expensive to recruit nobles
                break;
            case types::SocialClass::MERCHANTS:
                base_cost *= 1.5;
                break;
            case types::SocialClass::CRAFTSMEN:
                base_cost *= 1.2;
                break;
            case types::SocialClass::FREE_PEASANT:
                base_cost *= 1.0;
                break;
            case types::SocialClass::PEASANTS:
                base_cost *= 0.8;
                break;
            default:
                break;
        }
        
        return base_cost * quantity;
    }

    void MilitarySystem::InitializeUnitTemplates() {
        // Initialize templates for all unit types
        for (int i = 0; i < static_cast<int>(UnitType::COUNT); ++i) {
            UnitType type = static_cast<UnitType>(i);
            m_unit_templates[type] = MilitaryUnit(type);
        }
    }

    void MilitarySystem::InitializeTechnologyUnlocks() {
        // Technology unlocks would be loaded from configuration
    }

    void MilitarySystem::SubscribeToEvents() {
        // Subscribe to relevant events from other systems
    }

    void MilitarySystem::LogMilitaryEvent(types::EntityID province_id, const std::string& event_description) {
        std::cout << "[Military] Province " << province_id << ": " << event_description << std::endl;
    }

    void MilitarySystem::ValidateMilitaryState(types::EntityID province_id) {
        auto* military_comp = GetMilitaryComponent(province_id);
        if (!military_comp) {
            return;
        }
        
        // Validate unit counts
        for (auto& unit : military_comp->garrison_units) {
            if (unit.current_strength > unit.max_strength) {
                unit.current_strength = unit.max_strength;
            }
            
            if (unit.current_strength == 0) {
                // Mark for removal
                unit.morale = MoraleState::ROUTING;
            }
        }
        
        // Remove destroyed units
        military_comp->garrison_units.erase(
            std::remove_if(military_comp->garrison_units.begin(), 
                          military_comp->garrison_units.end(),
                          [](const MilitaryUnit& unit) { 
                              return unit.current_strength == 0; 
                          }),
            military_comp->garrison_units.end());
    }

    MilitaryComponent* MilitarySystem::GetMilitaryComponent(types::EntityID province_id) {
        return m_access_manager.GetComponent<MilitaryComponent>(province_id);
    }

    const MilitaryComponent* MilitarySystem::GetMilitaryComponent(types::EntityID province_id) const {
        return m_access_manager.GetComponent<MilitaryComponent>(province_id);
    }

    ArmyComponent* MilitarySystem::GetArmyComponent(types::EntityID army_id) {
        return m_access_manager.GetComponent<ArmyComponent>(army_id);
    }

    const ArmyComponent* MilitarySystem::GetArmyComponent(types::EntityID army_id) const {
        return m_access_manager.GetComponent<ArmyComponent>(army_id);
    }

} // namespace game::military_t current_recruitment = 0;
        for (const auto& unit : garrison_units) {
            current_recruitment += unit.current_strength;
        }
        
        if (current_recruitment + quantity > recruitment_capacity) {
            return false;
        }
        
        // Check budget
        auto unit_template = MilitaryUnit(unit_type);
        double total_cost = unit_template.recruitment_cost * quantity;
        
        return military_budget >= total_cost;
    }

    MilitaryUnit* MilitaryComponent::CreateUnit(UnitType unit_type, uint32_t initial_strength) {
        garrison_units.emplace_back(unit_type);
        MilitaryUnit& new_unit = garrison_units.back();
        
        if (initial_strength > 0 && initial_strength < new_unit.max_strength) {
            new_unit.current_strength = initial_strength;
        }
        
        // Update recruitment spending
        recruitment_spending += new_unit.recruitment_cost;
        military_budget -= new_unit.recruitment_cost;
        
        return &new_unit;
    }

    void MilitaryComponent::DisbandUnit(size_t unit_index) {
        if (unit_index >= garrison_units.size()) {
            return;
        }
        
        const auto& unit = garrison_units[unit_index];
        
        // Return some budget (partial refund)
        military_budget += unit.recruitment_cost * 0.3;
        
        garrison_units.erase(garrison_units.begin() + unit_index);
    }

    double MilitaryComponent::CalculateTotalMaintenance() const {
        double total = 0.0;
        for (const auto& unit : garrison_units) {
            total += unit.monthly_maintenance;
        }
        return total;
    }

    uint32
