// ============================================================================
// Date/Time Created: Wednesday, September 24, 2025 - 3:00 PM PST
// Intended Folder Location: src/game/military/MilitaryRecruitmentSystem.cpp
// MilitaryRecruitmentSystem.cpp - Population-Based Military Recruitment Implementation
// ============================================================================

#include "game/military/MilitaryRecruitmentSystem.h"
#include "game/population/PopulationComponents.h"
#include "game/economics/EconomicComponents.h"
#include "game/resources/ResourceComponents.h"
#include "core/config/GameConfig.h"
#include <iostream>
#include <algorithm>
#include <numeric>
#include <random>

namespace game::military {

    // ============================================================================
    // MilitaryUnit Implementation
    // ============================================================================

    MilitaryUnit::MilitaryUnit() = default;

    MilitaryUnit::MilitaryUnit(UnitType unit_type, types::EntityID province, types::SocialClass social_class) 
        : type(unit_type)
        , source_province(province)
        , recruited_from(social_class)
        , recruitment_date(std::chrono::steady_clock::now()) {
        
        unit_id = 0; // Will be assigned by system
        
        // Set default values based on unit type
        switch (unit_type) {
            case UnitType::LEVY_SPEARMEN:
                maximum_strength = 800;
                current_strength = 800;
                quality = UnitQuality::GREEN;
                recruitment_type = RecruitmentType::FEUDAL_LEVY;
                monthly_upkeep = 5.0;
                melee_attack = 0.8;
                defense = 1.0;
                mobility = 1.2;
                break;
                
            case UnitType::PROFESSIONAL_INFANTRY:
                maximum_strength = 600;
                current_strength = 600;
                quality = UnitQuality::TRAINED;
                recruitment_type = RecruitmentType::VOLUNTARY;
                monthly_upkeep = 15.0;
                melee_attack = 1.2;
                defense = 1.3;
                mobility = 1.0;
                break;
                
            case UnitType::CROSSBOWMEN:
                maximum_strength = 400;
                current_strength = 400;
                quality = UnitQuality::TRAINED;
                recruitment_type = RecruitmentType::VOLUNTARY;
                monthly_upkeep = 20.0;
                melee_attack = 0.7;
                ranged_attack = 1.5;
                defense = 0.9;
                mobility = 0.9;
                break;
                
            case UnitType::HEAVY_CAVALRY:
                maximum_strength = 200;
                current_strength = 200;
                quality = UnitQuality::EXPERIENCED;
                recruitment_type = RecruitmentType::VOLUNTARY;
                monthly_upkeep = 50.0;
                melee_attack = 2.0;
                defense = 1.8;
                mobility = 2.5;
                break;
                
            case UnitType::KNIGHTS:
                maximum_strength = 100;
                current_strength = 100;
                quality = UnitQuality::ELITE;
                recruitment_type = RecruitmentType::FEUDAL_LEVY;
                monthly_upkeep = 100.0;
                melee_attack = 2.8;
                defense = 2.5;
                mobility = 2.8;
                break;
                
            default:
                maximum_strength = 500;
                current_strength = 500;
                quality = UnitQuality::GREEN;
                recruitment_type = RecruitmentType::VOLUNTARY;
                monthly_upkeep = 10.0;
                melee_attack = 1.0;
                defense = 1.0;
                mobility = 1.0;
                break;
        }
        
        // Adjust stats based on social class
        switch (social_class) {
            case types::SocialClass::NOBILITY:
                experience += 0.3;
                morale += 0.2;
                equipment_quality += 0.3;
                monthly_upkeep *= 1.5;
                break;
                
            case types::SocialClass::MERCHANTS:
                equipment_quality += 0.2;
                monthly_upkeep *= 1.2;
                break;
                
            case types::SocialClass::CRAFTSMEN:
                equipment_quality += 0.1;
                monthly_upkeep *= 1.1;
                break;
                
            case types::SocialClass::PEASANTS:
                experience -= 0.1;
                equipment_quality -= 0.1;
                monthly_upkeep *= 0.8;
                break;
                
            default:
                break;
        }
        
        // Calculate equipment upkeep
        equipment_upkeep = monthly_upkeep * 0.3;
    }

    // ============================================================================
    // MilitaryRecruitmentSystem Constructor & Core Interface
    // ============================================================================

    MilitaryRecruitmentSystem::MilitaryRecruitmentSystem(
        core::ecs::ComponentAccessManager& access_manager,
        core::ecs::MessageBus& message_bus)
        : m_access_manager(access_manager)
        , m_message_bus(message_bus) {
    }

    void MilitaryRecruitmentSystem::Initialize() {
        std::cout << "Initializing Military Recruitment System..." << std::endl;
        
        InitializeUnitDefinitions();
        LoadMilitaryConfiguration();
        SetupDefaultRecruitmentPools();
        
        // Subscribe to relevant events
        m_message_bus.Subscribe<population::messages::PopulationChanged>(
            [this](const auto& msg) { 
                // Update recruitment pools when population changes
                CalculateRecruitmentPotential(msg.province_id);
            }
        );
        
        m_message_bus.Subscribe<economics::messages::EconomicUpdate>(
            [this](const auto& msg) {
                // Process military expenses when economy updates
                ProcessMilitaryExpenses(msg.province_id);
            }
        );
        
        std::cout << "Military Recruitment System initialized successfully" << std::endl;
    }

    void MilitaryRecruitmentSystem::Update(float delta_time) {
        m_accumulated_time += delta_time;
        m_training_accumulated_time += delta_time;
        
        // Update recruitment pools periodically
        if (m_accumulated_time >= m_recruitment_update_interval) {
            UpdateRecruitmentPools();
            m_accumulated_time = 0.0f;
        }
        
        // Update training periodically
        if (m_training_accumulated_time >= m_training_update_interval) {
            // Process training for all provinces with military components
            auto military_entities = m_access_manager.GetEntitiesWithComponent<MilitaryComponent>();
            for (auto entity : military_entities) {
                ProcessUnitTraining(entity, m_training_accumulated_time);
            }
            m_training_accumulated_time = 0.0f;
        }
    }

    void MilitaryRecruitmentSystem::Shutdown() {
        std::cout << "Shutting down Military Recruitment System..." << std::endl;
        m_all_units.clear();
        m_unit_definitions.clear();
    }

    std::string MilitaryRecruitmentSystem::GetSystemName() const {
        return "MilitaryRecruitmentSystem";
    }

    // ============================================================================
    // Core Recruitment Interface Implementation
    // ============================================================================

    bool MilitaryRecruitmentSystem::RecruitUnit(types::EntityID province_id, UnitType unit_type, 
                                               types::SocialClass preferred_class) {
        
        LogMilitaryActivity("Attempting to recruit " + GetUnitTypeName(unit_type) + 
                           " from " + std::to_string(static_cast<int>(preferred_class)) + 
                           " in province " + std::to_string(province_id));
        
        // Validate recruitment requirements
        if (!ValidateRecruitmentRequirements(province_id, unit_type, preferred_class)) {
            return false;
        }
        
        auto* military_comp = m_access_manager.GetComponent<MilitaryComponent>(province_id);
        auto* recruitment_comp = m_access_manager.GetComponent<RecruitmentPoolComponent>(province_id);
        
        if (!military_comp || !recruitment_comp) {
            std::cerr << "Missing required components for recruitment in province " << province_id << std::endl;
            return false;
        }
        
        // Check population availability
        int required_population = m_unit_definitions[unit_type].default_unit_size;
        if (!CheckPopulationAvailability(province_id, preferred_class, required_population)) {
            return false;
        }
        
        // Check financial capacity
        if (!CheckFinancialCapacity(province_id, unit_type)) {
            return false;
        }
        
        // Check equipment availability
        if (!CheckEquipmentAvailability(province_id, unit_type, 1)) {
            return false;
        }
        
        // Create the unit
        MilitaryUnit new_unit = CreateUnit(unit_type, province_id, preferred_class, required_population);
        
        // Remove population from recruitment pool
        RemovePopulationForRecruitment(province_id, preferred_class, required_population);
        
        // Consume equipment
        ConsumeEquipmentForRecruitment(province_id, unit_type, 1);
        
        // Charge recruitment costs
        double cost = CalculateRecruitmentCost(unit_type, preferred_class);
        ChargeRecruitmentCosts(province_id, cost);
        
        // Add unit to military component
        new_unit.unit_id = m_next_unit_id++;
        military_comp->active_units.push_back(new_unit);
        military_comp->unit_counts[unit_type]++;
        military_comp->total_active_soldiers += new_unit.current_strength;
        
        // Track unit globally
        m_all_units[new_unit.unit_id] = new_unit;
        
        // Apply recruitment effects to population
        ApplyRecruitmentEffectsToPopulation(province_id, preferred_class, required_population);
        
        // Update military expenses
        military_comp->monthly_military_expenses += new_unit.monthly_upkeep;
        military_comp->equipment_costs += new_unit.equipment_upkeep;
        
        // Publish recruitment event
        PublishUnitRecruited(new_unit, province_id, static_cast<int>(cost));
        
        LogMilitaryActivity("Successfully recruited " + GetUnitTypeName(unit_type) + 
                           " (ID: " + std::to_string(new_unit.unit_id) + ")");
        
        return true;
    }

    bool MilitaryRecruitmentSystem::RecruitMultipleUnits(types::EntityID province_id, UnitType unit_type, 
                                                        int count, types::SocialClass preferred_class) {
        
        if (count <= 0) return false;
        
        LogMilitaryActivity("Attempting to recruit " + std::to_string(count) + " units of " + 
                           GetUnitTypeName(unit_type) + " in province " + std::to_string(province_id));
        
        // Check if we can recruit all requested units
        for (int i = 0; i < count; ++i) {
            if (!ValidateRecruitmentRequirements(province_id, unit_type, preferred_class)) {
                std::cerr << "Cannot recruit " << count << " units - failed validation at unit " << (i + 1) << std::endl;
                return false;
            }
        }
        
        // Recruit units one by one
        int successful_recruitments = 0;
        for (int i = 0; i < count; ++i) {
            if (RecruitUnit(province_id, unit_type, preferred_class)) {
                successful_recruitments++;
            } else {
                std::cerr << "Failed to recruit unit " << (i + 1) << " of " << count << std::endl;
                break;
            }
        }
        
        LogMilitaryActivity("Successfully recruited " + std::to_string(successful_recruitments) + 
                           " out of " + std::to_string(count) + " requested units");
        
        return successful_recruitments == count;
    }

    std::vector<MilitaryUnit> MilitaryRecruitmentSystem::EmergencyRecruitment(
        types::EntityID province_id, int target_strength, RecruitmentType recruitment_type) {
        
        std::vector<MilitaryUnit> emergency_units;
        
        LogMilitaryActivity("Emergency recruitment activated in province " + std::to_string(province_id) + 
                           " - target strength: " + std::to_string(target_strength));
        
        auto* recruitment_comp = m_access_manager.GetComponent<RecruitmentPoolComponent>(province_id);
        if (!recruitment_comp) {
            std::cerr << "No recruitment component found for emergency recruitment" << std::endl;
            return emergency_units;
        }
        
        // Set emergency recruitment flag
        recruitment_comp->wartime_recruitment = true;
        
        int current_strength = 0;
        std::vector<UnitType> emergency_unit_types = { 
            UnitType::LEVY_SPEARMEN, 
            UnitType::PROFESSIONAL_INFANTRY, 
            UnitType::CROSSBOWMEN 
        };
        
        // Priority order for recruitment classes during emergency
        std::vector<types::SocialClass> recruitment_classes = {
            types::SocialClass::FREE_PEASANT,
            types::SocialClass::CRAFTSMEN,
            types::SocialClass::MERCHANTS,
            types::SocialClass::PEASANTS
        };
        
        for (auto unit_type : emergency_unit_types) {
            if (current_strength >= target_strength) break;
            
            for (auto social_class : recruitment_classes) {
                if (current_strength >= target_strength) break;
                
                // Try to recruit as many units as possible of this type/class combination
                while (current_strength < target_strength) {
                    if (ValidateRecruitmentRequirements(province_id, unit_type, social_class)) {
                        MilitaryUnit unit = CreateUnit(unit_type, province_id, social_class, 
                                                     m_unit_definitions[unit_type].default_unit_size);
                        
                        // Emergency recruitment uses conscription rules
                        unit.recruitment_type = recruitment_type;
                        unit.morale *= 0.8; // Lower morale for emergency recruits
                        unit.experience *= 0.7; // Less experienced
                        
                        emergency_units.push_back(unit);
                        current_strength += unit.current_strength;
                        
                        // Apply recruitment effects immediately
                        RemovePopulationForRecruitment(province_id, social_class, unit.current_strength);
                        ApplyRecruitmentEffectsToPopulation(province_id, social_class, unit.current_strength);
                        
                        LogMilitaryActivity("Emergency recruited " + GetUnitTypeName(unit_type) + 
                                           " with " + std::to_string(unit.current_strength) + " soldiers");
                    } else {
                        break; // Can't recruit more of this type/class
                    }
                }
            }
        }
        
        LogMilitaryActivity("Emergency recruitment completed - recruited " + 
                           std::to_string(emergency_units.size()) + " units with total strength " + 
                           std::to_string(current_strength));
        
        return emergency_units;
    }

    // ============================================================================
    // Unit Management Implementation
    // ============================================================================

    bool MilitaryRecruitmentSystem::DisbandUnit(types::EntityID unit_id) {
        auto unit_it = m_all_units.find(unit_id);
        if (unit_it == m_all_units.end()) {
            std::cerr << "Unit " << unit_id << " not found for disbandment" << std::endl;
            return false;
        }
        
        const MilitaryUnit& unit = unit_it->second;
        types::EntityID province_id = unit.source_province;
        
        LogMilitaryActivity("Disbanding unit " + std::to_string(unit_id) + " (" + 
                           GetUnitTypeName(unit.type) + ") from province " + std::to_string(province_id));
        
        auto* military_comp = m_access_manager.GetComponent<MilitaryComponent>(province_id);
        if (!military_comp) {
            std::cerr << "No military component found for unit disbandment" << std::endl;
            return false;
        }
        
        // Find and remove unit from military component
        auto& units = military_comp->active_units;
        auto unit_iter = std::find_if(units.begin(), units.end(),
            [unit_id](const MilitaryUnit& u) { return u.unit_id == unit_id; });
        
        if (unit_iter != units.end()) {
            // Return population to recruitment pool
            ReturnPopulationFromUnit(*unit_iter);
            
            // Update military component statistics
            military_comp->unit_counts[unit_iter->type]--;
            military_comp->total_active_soldiers -= unit_iter->current_strength;
            military_comp->monthly_military_expenses -= unit_iter->monthly_upkeep;
            military_comp->equipment_costs -= unit_iter->equipment_upkeep;
            
            // Publish disbandment event
            PublishUnitDisbanded(unit_id, province_id, unit_iter->type, unit_iter->current_strength);
            
            // Remove from containers
            units.erase(unit_iter);
            m_all_units.erase(unit_it);
            
            LogMilitaryActivity("Successfully disbanded unit " + std::to_string(unit_id));
            return true;
        }
        
        std::cerr << "Unit " << unit_id << " not found in military component" << std::endl;
        return false;
    }

    bool MilitaryRecruitmentSystem::DisbandAllUnits(types::EntityID province_id) {
        auto* military_comp = m_access_manager.GetComponent<MilitaryComponent>(province_id);
        if (!military_comp) {
            std::cerr << "No military component found for province " << province_id << std::endl;
            return false;
        }
        
        LogMilitaryActivity("Disbanding all units in province " + std::to_string(province_id));
        
        std::vector<types::EntityID> units_to_disband;
        for (const auto& unit : military_comp->active_units) {
            units_to_disband.push_back(unit.unit_id);
        }
        
        bool all_successful = true;
        for (auto unit_id : units_to_disband) {
            if (!DisbandUnit(unit_id)) {
                all_successful = false;
            }
        }
        
        LogMilitaryActivity("Disbandment complete - " + std::to_string(units_to_disband.size()) + 
                           " units processed");
        
        return all_successful;
    }

    void MilitaryRecruitmentSystem::ReturnPopulationFromUnit(const MilitaryUnit& unit) {
        auto* recruitment_comp = m_access_manager.GetComponent<RecruitmentPoolComponent>(unit.source_province);
        if (!recruitment_comp) {
            std::cerr << "No recruitment component found for returning population" << std::endl;
            return;
        }
        
        // Return soldiers to recruitment pool (some may have died or deserted)
        double return_rate = 0.8; // 80% of soldiers return to population
        int returning_population = static_cast<int>(unit.current_strength * return_rate);
        
        recruitment_comp->available_recruits[unit.recruited_from] += returning_population;
        recruitment_comp->class_data[unit.recruited_from].currently_serving -= unit.current_strength;
        
        LogMilitaryActivity("Returned " + std::to_string(returning_population) + " soldiers to " +
                           "social class " + std::to_string(static_cast<int>(unit.recruited_from)) + 
                           " recruitment pool");
    }

    // ============================================================================
    // Recruitment Pool Management Implementation
    // ============================================================================

    void MilitaryRecruitmentSystem::UpdateRecruitmentPools() {
        auto recruitment_entities = m_access_manager.GetEntitiesWithComponent<RecruitmentPoolComponent>();
        
        for (auto entity : recruitment_entities) {
            CalculateRecruitmentPotential(entity);
        }
    }

    void MilitaryRecruitmentSystem::CalculateRecruitmentPotential(types::EntityID province_id) {
        auto* recruitment_comp = m_access_manager.GetComponent<RecruitmentPoolComponent>(province_id);
        auto* population_comp = m_access_manager.GetComponent<population::PopulationComponent>(province_id);
        
        if (!recruitment_comp || !population_comp) {
            return;
        }
        
        // Clear existing recruitment potential
        recruitment_comp->recruitment_potential.clear();
        
        // Calculate recruitment potential for each social class
        for (const auto& [social_class, pop_data] : population_comp->demographics.population_by_class) {
            auto& class_data = recruitment_comp->class_data[social_class];
            
            // Base recruitment rate affected by various factors
            double effective_rate = class_data.base_recruitment_rate;
            
            // Apply military tradition bonus
            effective_rate *= (1.0 + recruitment_comp->military_tradition);
            
            // Apply recruitment enthusiasm
            effective_rate *= recruitment_comp->recruitment_enthusiasm;
            
            // Reduce rate if already heavily recruited from this class
            double recruitment_ratio = static_cast<double>(class_data.currently_serving) / 
                                     std::max(1.0, static_cast<double>(pop_data.total_population));
            if (recruitment_ratio > 0.1) { // If more than 10% are serving
                effective_rate *= (1.0 - recruitment_ratio);
            }
            
            // Apply resistance factor
            effective_rate *= (1.0 - class_data.recruitment_resistance);
            
            // Calculate potential recruits
            int potential = static_cast<int>(pop_data.total_population * effective_rate);
            recruitment_comp->recruitment_potential[social_class] = potential;
            
            // Update available recruits (some may become available each update)
            int new_available = static_cast<int>(potential * 0.1); // 10% become available each period
            recruitment_comp->available_recruits[social_class] = 
                std::min(recruitment_comp->available_recruits[social_class] + new_available, potential);
        }
    }

    int MilitaryRecruitmentSystem::GetAvailableRecruits(types::EntityID province_id, types::SocialClass social_class) {
        auto* recruitment_comp = m_access_manager.GetComponent<RecruitmentPoolComponent>(province_id);
        if (!recruitment_comp) {
            return 0;
        }
        
        auto it = recruitment_comp->available_recruits.find(social_class);
        return (it != recruitment_comp->available_recruits.end()) ? it->second : 0;
    }

    int MilitaryRecruitmentSystem::GetTotalRecruitmentCapacity(types::EntityID province_id) {
        auto* recruitment_comp = m_access_manager.GetComponent<RecruitmentPoolComponent>(province_id);
        if (!recruitment_comp) {
            return 0;
        }
        
        int total = 0;
        for (const auto& [social_class, count] : recruitment_comp->recruitment_potential) {
            total += count;
        }
        
        return total;
    }

    // ============================================================================
    // Social Class Recruitment Specifics Implementation
    // ============================================================================

    std::vector<UnitType> MilitaryRecruitmentSystem::GetViableUnitTypes(types::SocialClass social_class) {
        std::vector<UnitType> viable_types;
        
        switch (social_class) {
            case types::SocialClass::NOBILITY:
                viable_types = { 
                    UnitType::KNIGHTS, 
                    UnitType::HEAVY_CAVALRY, 
                    UnitType::RETINUE,
                    UnitType::COMMANDERS 
                };
                break;
                
            case types::SocialClass::MERCHANTS:
                viable_types = { 
                    UnitType::CROSSBOWMEN, 
                    UnitType::PROFESSIONAL_INFANTRY,
                    UnitType::LIGHT_CAVALRY,
                    UnitType::MARINES 
                };
                break;
                
            case types::SocialClass::CRAFTSMEN:
                viable_types = { 
                    UnitType::ENGINEERS, 
                    UnitType::ARTILLERY_CREW,
                    UnitType::CROSSBOWMEN,
                    UnitType::PROFESSIONAL_INFANTRY 
                };
                break;
                
            case types::SocialClass::FREE_PEASANT:
                viable_types = { 
                    UnitType::LEVY_SPEARMEN, 
                    UnitType::PROFESSIONAL_INFANTRY,
                    UnitType::PIKEMEN,
                    UnitType::LIGHT_CAVALRY 
                };
                break;
                
            case types::SocialClass::PEASANTS:
                viable_types = { 
                    UnitType::LEVY_SPEARMEN,
                    UnitType::PIKEMEN 
                };
                break;
                
            default:
                viable_types = { UnitType::LEVY_SPEARMEN };
                break;
        }
        
        return viable_types;
    }

    types::SocialClass MilitaryRecruitmentSystem::GetOptimalRecruitmentClass(UnitType unit_type) {
        switch (unit_type) {
            case UnitType::KNIGHTS:
            case UnitType::RETINUE:
            case UnitType::COMMANDERS:
                return types::SocialClass::NOBILITY;
                
            case UnitType::HEAVY_CAVALRY:
                return types::SocialClass::NOBILITY; // Secondary: MERCHANTS
                
            case UnitType::CROSSBOWMEN:
            case UnitType::MARINES:
                return types::SocialClass::MERCHANTS;
                
            case UnitType::ENGINEERS:
            case UnitType::ARTILLERY_CREW:
                return types::SocialClass::CRAFTSMEN;
                
            case UnitType::PROFESSIONAL_INFANTRY:
            case UnitType::HEAVY_INFANTRY:
                return types::SocialClass::FREE_PEASANT;
                
            case UnitType::LEVY_SPEARMEN:
            case UnitType::PIKEMEN:
                return types::SocialClass::PEASANTS;
                
            case UnitType::LIGHT_CAVALRY:
            case UnitType::HORSE_ARCHERS:
                return types::SocialClass::FREE_PEASANT;
                
            default:
                return types::SocialClass::FREE_PEASANT;
        }
    }

    double MilitaryRecruitmentSystem::GetClassRecruitmentRate(types::EntityID province_id, types::SocialClass social_class) {
        auto* recruitment_comp = m_access_manager.GetComponent<RecruitmentPoolComponent>(province_id);
        if (!recruitment_comp) {
            return 0.0;
        }
        
        auto it = recruitment_comp->class_data.find(social_class);
        if (it != recruitment_comp->class_data.end()) {
            return it->second.current_recruitment_rate;
        }
        
        return 0.05; // Default 5% recruitment rate
    }

    // ============================================================================
    // Equipment & Supply Integration Implementation
    // ============================================================================

    std::unordered_map<types::ResourceType, int> MilitaryRecruitmentSystem::GetEquipmentRequirements(
        UnitType unit_type, int unit_size) {
        
        std::unordered_map<types::ResourceType, int> requirements;
        
        auto it = m_unit_definitions.find(unit_type);
        if (it == m_unit_definitions.end()) {
            return requirements;
        }
        
        const auto& definition = it->second;
        
        // Scale requirements by unit size
        double size_factor = static_cast<double>(unit_size) / definition.default_unit_size;
        
        for (const auto& [resource, base_amount] : definition.equipment_requirements) {
            requirements[resource] = static_cast<int>(base_amount * size_factor);
        }
        
        return requirements;
    }

    bool MilitaryRecruitmentSystem::CheckEquipmentAvailability(types::EntityID province_id, 
                                                             UnitType unit_type, int unit_count) {
        auto* resource_comp = m_access_manager.GetComponent<resources::ResourceComponent>(province_id);
        if (!resource_comp) {
            return false;
        }
        
        auto requirements = GetEquipmentRequirements(unit_type, 
                                                   m_unit_definitions[unit_type].default_unit_size * unit_count);
        
        for (const auto& [resource, needed] : requirements) {
            auto available = resource_comp->GetResourceAmount(resource);
            if (available < needed) {
                LogMilitaryActivity("Insufficient " + std::to_string(static_cast<int>(resource)) + 
                                   " for recruitment - need " + std::to_string(needed) + 
                                   ", have " + std::to_string(available));
                return false;
            }
        }
        
        return true;
    }

    void MilitaryRecruitmentSystem::ConsumeEquipmentForRecruitment(types::EntityID province_id, 
                                                                 UnitType unit_type, int unit_count) {
        auto* resource_comp = m_access_manager.GetComponent<resources::ResourceComponent>(province_id);
        if (!resource_comp) {
            std::cerr << "No resource component found for equipment consumption" << std::endl;
            return;
        }
        
        auto requirements = GetEquipmentRequirements(unit_type, 
                                                   m_unit_definitions[unit_type].default_unit_size * unit_count);
        
        for (const auto& [resource, needed] : requirements) {
            resource_comp->ConsumeResource(resource, needed);
            LogMilitaryActivity("Consumed " + std::to_string(needed) + " units of " + 
                               std::to_string(static_cast<int>(resource)) + " for recruitment");
        }
    }

    std::unordered_map<types::ResourceType, double> MilitaryRecruitmentSystem::GetMonthlySupplyNeeds(
        const MilitaryUnit& unit) {
        
        std::unordered_map<types::ResourceType, double> supply_needs;
        
        auto it = m_unit_definitions.find(unit.type);
        if (it == m_unit_definitions.end()) {
            return supply_needs;
        }
        
        const auto& definition = it->second;
        double strength_factor = static_cast<double>(unit.current_strength) / unit.maximum_strength;
        
        for (const auto& [resource, base_consumption] : definition.monthly_supply_needs) {
            supply_needs[resource] = base_consumption * strength_factor;
        }
        
        return supply_needs;
    }

// ============================================================================
    // Supply and Logistics Implementation (Continued)
    // ============================================================================

    void MilitaryRecruitmentSystem::ProcessMonthlySupplyConsumption(types::EntityID province_id) {
        auto* military_comp = m_access_manager.GetComponent<MilitaryComponent>(province_id);
        auto* resource_comp = m_access_manager.GetComponent<resources::ResourceComponent>(province_id);
        
        if (!military_comp || !resource_comp) {
            return;
        }
        
        std::unordered_map<types::ResourceType, double> total_needs;
        
        // Calculate total supply needs for all units
        for (const auto& unit : military_comp->active_units) {
            auto unit_needs = GetMonthlySupplyNeeds(unit);
            for (const auto& [resource, amount] : unit_needs) {
                total_needs[resource] += amount;
            }
        }
        
        // Consume supplies
        for (const auto& [resource, needed] : total_needs) {
            auto available = resource_comp->GetResourceAmount(resource);
            if (available >= needed) {
                resource_comp->ConsumeResource(resource, static_cast<int>(needed));
            } else {
                // Handle supply shortage
                double shortage_ratio = available / needed;
                resource_comp->ConsumeResource(resource, static_cast<int>(available));
                
                // Apply supply shortage effects to units
                for (auto& unit : military_comp->active_units) {
                    unit.supply_level *= shortage_ratio;
                    unit.morale *= (0.8 + 0.2 * shortage_ratio); // Morale drops with poor supply
                }
                
                LogMilitaryActivity("Supply shortage in province " + std::to_string(province_id) + 
                                   " - only " + std::to_string(shortage_ratio * 100) + "% of needs met");
            }
        }
    }

    bool MilitaryRecruitmentSystem::CanSupplyUnits(types::EntityID province_id) {
        auto* military_comp = m_access_manager.GetComponent<MilitaryComponent>(province_id);
        auto* resource_comp = m_access_manager.GetComponent<resources::ResourceComponent>(province_id);
        
        if (!military_comp || !resource_comp) {
            return false;
        }
        
        std::unordered_map<types::ResourceType, double> total_needs;
        
        for (const auto& unit : military_comp->active_units) {
            auto unit_needs = GetMonthlySupplyNeeds(unit);
            for (const auto& [resource, amount] : unit_needs) {
                total_needs[resource] += amount;
            }
        }
        
        for (const auto& [resource, needed] : total_needs) {
            auto available = resource_comp->GetResourceAmount(resource);
            if (available < needed) {
                return false;
            }
        }
        
        return true;
    }

    // ============================================================================
    // Unit Quality & Training Implementation
    // ============================================================================

    void MilitaryRecruitmentSystem::ProcessUnitTraining(types::EntityID province_id, float time_delta) {
        auto* military_comp = m_access_manager.GetComponent<MilitaryComponent>(province_id);
        if (!military_comp) {
            return;
        }
        
        double training_efficiency = GetTrainingEfficiencyBonus(province_id);
        
        for (auto& unit : military_comp->active_units) {
            ProcessBasicTraining(unit, training_efficiency, time_delta);
            UpdateUnitExperience(unit, 0.01, time_delta); // Base experience gain
            CheckForPromotions(unit);
        }
    }

    void MilitaryRecruitmentSystem::ImproveUnitQuality(MilitaryUnit& unit, double experience_gain) {
        unit.experience += experience_gain;
        unit.experience = std::min(1.0, unit.experience);
        
        // Check for quality improvements
        UnitQuality old_quality = unit.quality;
        
        if (unit.experience >= 0.9 && unit.quality < UnitQuality::LEGENDARY) {
            unit.quality = UnitQuality::LEGENDARY;
        } else if (unit.experience >= 0.7 && unit.quality < UnitQuality::ELITE) {
            unit.quality = UnitQuality::ELITE;
        } else if (unit.experience >= 0.5 && unit.quality < UnitQuality::EXPERIENCED) {
            unit.quality = UnitQuality::EXPERIENCED;
        } else if (unit.experience >= 0.3 && unit.quality < UnitQuality::TRAINED) {
            unit.quality = UnitQuality::TRAINED;
        }
        
        if (unit.quality != old_quality) {
            ApplyQualityModifiers(unit);
            
            // Publish promotion event
            messages::UnitPromoted promotion_msg;
            promotion_msg.unit_id = unit.unit_id;
            promotion_msg.old_quality = old_quality;
            promotion_msg.new_quality = unit.quality;
            promotion_msg.promotion_reason = "Combat experience and training";
            
            m_message_bus.Publish(promotion_msg);
            
            LogMilitaryActivity("Unit " + std::to_string(unit.unit_id) + " promoted from " + 
                               GetQualityName(old_quality) + " to " + GetQualityName(unit.quality));
        }
    }

    UnitQuality MilitaryRecruitmentSystem::CalculateRecruitQuality(types::EntityID province_id, 
                                                                  types::SocialClass social_class) {
        auto* military_comp = m_access_manager.GetComponent<MilitaryComponent>(province_id);
        if (!military_comp) {
            return UnitQuality::GREEN;
        }
        
        // Base quality depends on social class
        UnitQuality base_quality = UnitQuality::GREEN;
        
        switch (social_class) {
            case types::SocialClass::NOBILITY:
                base_quality = UnitQuality::EXPERIENCED;
                break;
            case types::SocialClass::MERCHANTS:
                base_quality = UnitQuality::TRAINED;
                break;
            case types::SocialClass::CRAFTSMEN:
                base_quality = UnitQuality::TRAINED;
                break;
            case types::SocialClass::FREE_PEASANT:
                base_quality = UnitQuality::GREEN;
                break;
            case types::SocialClass::PEASANTS:
                base_quality = UnitQuality::GREEN;
                break;
            default:
                base_quality = UnitQuality::GREEN;
                break;
        }
        
        // Modify based on training facilities
        if (military_comp->training_facilities >= 3 && base_quality < UnitQuality::TRAINED) {
            base_quality = UnitQuality::TRAINED;
        }
        
        return base_quality;
    }

    double MilitaryRecruitmentSystem::GetTrainingEfficiencyBonus(types::EntityID province_id) {
        auto* military_comp = m_access_manager.GetComponent<MilitaryComponent>(province_id);
        if (!military_comp) {
            return 1.0;
        }
        
        double efficiency = 1.0;
        
        // Training facilities bonus
        efficiency += military_comp->training_facilities * 0.2;
        
        // Global training modifier
        efficiency *= m_global_training_modifier;
        
        return efficiency;
    }

    void MilitaryRecruitmentSystem::UpdateUnitExperience(MilitaryUnit& unit, double base_gain, float time_delta) {
        double time_factor = time_delta / 30.0; // Normalize to monthly gains
        double experience_gain = base_gain * time_factor;
        
        // Quality affects experience gain rate
        switch (unit.quality) {
            case UnitQuality::GREEN:
                experience_gain *= 1.5; // Learn faster when inexperienced
                break;
            case UnitQuality::TRAINED:
                experience_gain *= 1.2;
                break;
            case UnitQuality::EXPERIENCED:
                experience_gain *= 1.0;
                break;
            case UnitQuality::ELITE:
                experience_gain *= 0.8;
                break;
            case UnitQuality::LEGENDARY:
                experience_gain *= 0.5; // Very slow improvement at max level
                break;
        }
        
        // Apply morale bonus to experience gain
        experience_gain *= unit.morale;
        
        ImproveUnitQuality(unit, experience_gain);
    }

    // ============================================================================
    // Economic Integration Implementation
    // ============================================================================

    double MilitaryRecruitmentSystem::CalculateRecruitmentCost(UnitType unit_type, types::SocialClass social_class) {
        auto it = m_unit_definitions.find(unit_type);
        if (it == m_unit_definitions.end()) {
            return 100.0; // Default cost
        }
        
        double base_cost = it->second.base_recruitment_cost;
        
        // Social class modifiers
        switch (social_class) {
            case types::SocialClass::NOBILITY:
                base_cost *= 3.0; // Expensive to recruit nobles
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
        
        // Apply global recruitment modifier
        base_cost *= m_global_recruitment_modifier;
        
        return base_cost;
    }

    double MilitaryRecruitmentSystem::CalculateMonthlyUpkeep(const MilitaryUnit& unit) {
        double base_upkeep = unit.monthly_upkeep;
        
        // Quality affects upkeep
        switch (unit.quality) {
            case UnitQuality::GREEN:
                base_upkeep *= 0.8;
                break;
            case UnitQuality::TRAINED:
                base_upkeep *= 1.0;
                break;
            case UnitQuality::EXPERIENCED:
                base_upkeep *= 1.2;
                break;
            case UnitQuality::ELITE:
                base_upkeep *= 1.5;
                break;
            case UnitQuality::LEGENDARY:
                base_upkeep *= 2.0;
                break;
        }
        
        // Strength affects upkeep
        double strength_ratio = static_cast<double>(unit.current_strength) / unit.maximum_strength;
        base_upkeep *= strength_ratio;
        
        // Apply global upkeep modifier
        base_upkeep *= m_global_upkeep_modifier;
        
        return base_upkeep;
    }

    double MilitaryRecruitmentSystem::CalculateEquipmentCost(UnitType unit_type, UnitQuality quality) {
        auto it = m_unit_definitions.find(unit_type);
        if (it == m_unit_definitions.end()) {
            return 50.0;
        }
        
        double base_cost = it->second.base_recruitment_cost * 0.3; // Equipment is 30% of recruitment cost
        
        // Quality affects equipment cost
        switch (quality) {
            case UnitQuality::GREEN:
                base_cost *= 0.7;
                break;
            case UnitQuality::TRAINED:
                base_cost *= 1.0;
                break;
            case UnitQuality::EXPERIENCED:
                base_cost *= 1.3;
                break;
            case UnitQuality::ELITE:
                base_cost *= 1.7;
                break;
            case UnitQuality::LEGENDARY:
                base_cost *= 2.5;
                break;
        }
        
        return base_cost;
    }

    void MilitaryRecruitmentSystem::ProcessMilitaryExpenses(types::EntityID province_id) {
        auto* military_comp = m_access_manager.GetComponent<MilitaryComponent>(province_id);
        auto* economic_comp = m_access_manager.GetComponent<economics::EconomicComponent>(province_id);
        
        if (!military_comp || !economic_comp) {
            return;
        }
        
        double total_upkeep = 0.0;
        double total_equipment = 0.0;
        
        // Calculate total expenses
        for (const auto& unit : military_comp->active_units) {
            total_upkeep += CalculateMonthlyUpkeep(unit);
            total_equipment += unit.equipment_upkeep;
        }
        
        // Update military component
        military_comp->maintenance_spending = total_upkeep;
        military_comp->equipment_spending = total_equipment;
        military_comp->monthly_military_expenses = total_upkeep + total_equipment;
        
        // Check if province can afford military
        double available_funds = economic_comp->GetTreasuryAmount();
        double total_expense = military_comp->monthly_military_expenses;
        
        if (available_funds < total_expense) {
            // Handle insufficient funds
            HandleInsufficientFunds(province_id, total_expense - available_funds);
        } else {
            // Charge expenses
            economic_comp->SpendFromTreasury(total_expense);
        }
    }

    double MilitaryRecruitmentSystem::GetTotalMilitaryBurden(types::EntityID province_id) {
        auto* military_comp = m_access_manager.GetComponent<MilitaryComponent>(province_id);
        auto* economic_comp = m_access_manager.GetComponent<economics::EconomicComponent>(province_id);
        
        if (!military_comp || !economic_comp) {
            return 0.0;
        }
        
        double total_income = economic_comp->GetMonthlyIncome();
        if (total_income <= 0.0) {
            return 1.0; // 100% burden if no income
        }
        
        return military_comp->monthly_military_expenses / total_income;
    }

    void MilitaryRecruitmentSystem::ApplyMilitaryEconomicEffects(types::EntityID province_id) {
        auto* economic_comp = m_access_manager.GetComponent<economics::EconomicComponent>(province_id);
        auto* population_comp = m_access_manager.GetComponent<population::PopulationComponent>(province_id);
        
        if (!economic_comp || !population_comp) {
            return;
        }
        
        double military_burden = GetTotalMilitaryBurden(province_id);
        
        // High military burden affects economy
        if (military_burden > 0.4) { // More than 40% of income
            // Reduce economic growth
            double growth_penalty = (military_burden - 0.4) * 0.5;
            economic_comp->ApplyGrowthModifier(-growth_penalty);
            
            // Increase tax burden feeling
            population_comp->ModifyHappiness(-growth_penalty * 10);
            
            LogMilitaryActivity("High military burden (" + std::to_string(military_burden * 100) + 
                               "%) affecting economy in province " + std::to_string(province_id));
        }
    }

    // ============================================================================
    // Query Interface Implementation
    // ============================================================================

    std::vector<MilitaryUnit> MilitaryRecruitmentSystem::GetAllUnits(types::EntityID province_id) {
        auto* military_comp = m_access_manager.GetComponent<MilitaryComponent>(province_id);
        if (!military_comp) {
            return {};
        }
        
        return military_comp->active_units;
    }

    std::vector<MilitaryUnit> MilitaryRecruitmentSystem::GetUnitsByType(types::EntityID province_id, UnitType unit_type) {
        auto* military_comp = m_access_manager.GetComponent<MilitaryComponent>(province_id);
        if (!military_comp) {
            return {};
        }
        
        std::vector<MilitaryUnit> units_of_type;
        for (const auto& unit : military_comp->active_units) {
            if (unit.type == unit_type) {
                units_of_type.push_back(unit);
            }
        }
        
        return units_of_type;
    }

    MilitaryUnit* MilitaryRecruitmentSystem::GetUnit(types::EntityID unit_id) {
        auto it = m_all_units.find(unit_id);
        return (it != m_all_units.end()) ? &it->second : nullptr;
    }

    int MilitaryRecruitmentSystem::GetTotalMilitaryStrength(types::EntityID province_id) {
        auto* military_comp = m_access_manager.GetComponent<MilitaryComponent>(province_id);
        if (!military_comp) {
            return 0;
        }
        
        return military_comp->total_active_soldiers;
    }

    bool MilitaryRecruitmentSystem::CanRecruit(types::EntityID province_id, UnitType unit_type, types::SocialClass social_class) {
        return ValidateRecruitmentRequirements(province_id, unit_type, social_class);
    }

    std::string MilitaryRecruitmentSystem::GetRecruitmentLimitationReason(types::EntityID province_id, UnitType unit_type) {
        auto* military_comp = m_access_manager.GetComponent<MilitaryComponent>(province_id);
        auto* recruitment_comp = m_access_manager.GetComponent<RecruitmentPoolComponent>(province_id);
        
        if (!military_comp || !recruitment_comp) {
            return "Missing required components";
        }
        
        // Check various limitations
        types::SocialClass optimal_class = GetOptimalRecruitmentClass(unit_type);
        
        if (!CheckPopulationAvailability(province_id, optimal_class, m_unit_definitions[unit_type].default_unit_size)) {
            return "Insufficient population of " + std::to_string(static_cast<int>(optimal_class)) + " class";
        }
        
        if (!CheckFinancialCapacity(province_id, unit_type)) {
            return "Insufficient funds for recruitment";
        }
        
        if (!CheckEquipmentAvailability(province_id, unit_type, 1)) {
            return "Insufficient equipment and resources";
        }
        
        return "No limitations found";
    }

    double MilitaryRecruitmentSystem::CalculateMilitaryReadiness(types::EntityID province_id) {
        auto* military_comp = m_access_manager.GetComponent<MilitaryComponent>(province_id);
        if (!military_comp) {
            return 0.0;
        }
        
        double total_readiness = 0.0;
        int unit_count = 0;
        
        for (const auto& unit : military_comp->active_units) {
            double unit_readiness = CalculateUnitEffectiveness(unit);
            total_readiness += unit_readiness;
            unit_count++;
        }
        
        return (unit_count > 0) ? (total_readiness / unit_count) : 0.0;
    }

    double MilitaryRecruitmentSystem::CalculateUnitEffectiveness(const MilitaryUnit& unit) {
        double effectiveness = 1.0;
        
        // Strength factor
        double strength_ratio = static_cast<double>(unit.current_strength) / unit.maximum_strength;
        effectiveness *= (0.3 + 0.7 * strength_ratio);
        
        // Experience factor
        effectiveness *= (0.5 + 0.5 * unit.experience);
        
        // Equipment factor
        effectiveness *= (0.4 + 0.6 * unit.equipment_quality);
        
        // Morale factor
        effectiveness *= unit.morale;
        
        return std::max(0.1, effectiveness);
    }

    int MilitaryRecruitmentSystem::GetActiveUnitCount(types::EntityID province_id) {
        auto* military_comp = m_access_manager.GetComponent<MilitaryComponent>(province_id);
        if (!military_comp) {
            return 0;
        }
        
        return static_cast<int>(military_comp->active_units.size());
    }

    int MilitaryRecruitmentSystem::GetTotalSoldierCount(types::EntityID province_id) {
        return GetTotalMilitaryStrength(province_id);
    }

    double MilitaryRecruitmentSystem::GetAverageUnitQuality(types::EntityID province_id) {
        auto* military_comp = m_access_manager.GetComponent<MilitaryComponent>(province_id);
        if (!military_comp || military_comp->active_units.empty()) {
            return 0.0;
        }
        
        double total_quality = 0.0;
        for (const auto& unit : military_comp->active_units) {
            total_quality += static_cast<double>(unit.quality);
        }
        
        return total_quality / military_comp->active_units.size();
    }

    // ============================================================================
    // Configuration & Settings Implementation
    // ============================================================================

    void MilitaryRecruitmentSystem::SetRecruitmentPolicy(types::EntityID province_id, RecruitmentType policy) {
        auto* military_comp = m_access_manager.GetComponent<MilitaryComponent>(province_id);
        if (!military_comp) {
            return;
        }
        
        military_comp->default_recruitment = policy;
    }

    void MilitaryRecruitmentSystem::SetRecruitmentRate(types::EntityID province_id, double rate) {
        auto* military_comp = m_access_manager.GetComponent<MilitaryComponent>(province_id);
        if (!military_comp) {
            return;
        }
        
        military_comp->recruitment_rate = std::max(0.0, std::min(1.0, rate));
    }

    void MilitaryRecruitmentSystem::EnableEmergencyRecruitment(types::EntityID province_id, bool enable) {
        auto* recruitment_comp = m_access_manager.GetComponent<RecruitmentPoolComponent>(province_id);
        if (!recruitment_comp) {
            return;
        }
        
        recruitment_comp->wartime_recruitment = enable;
    }

    void MilitaryRecruitmentSystem::SetGlobalMilitaryBudget(double budget_percentage) {
        m_military_budget_percentage = std::max(0.0, std::min(1.0, budget_percentage));
    }

    void MilitaryRecruitmentSystem::SetUnitUpkeepModifier(double modifier) {
        m_global_upkeep_modifier = std::max(0.1, modifier);
    }

    void MilitaryRecruitmentSystem::SetTrainingEfficiencyModifier(double modifier) {
        m_global_training_modifier = std::max(0.1, modifier);
    }

    // ============================================================================
    // Internal Helper Method Implementations
    // ============================================================================

    MilitaryUnit MilitaryRecruitmentSystem::CreateUnit(UnitType unit_type, types::EntityID province_id,
                                                      types::SocialClass social_class, int size) {
        MilitaryUnit unit(unit_type, province_id, social_class);
        unit.current_strength = std::min(size, unit.maximum_strength);
        
        InitializeUnitStats(unit);
        ApplyQualityModifiers(unit);
        
        return unit;
    }

    void MilitaryRecruitmentSystem::InitializeUnitStats(MilitaryUnit& unit) {
        // Stats are already initialized in constructor
        // This method can be used for additional customization
    }

    void MilitaryRecruitmentSystem::ApplyQualityModifiers(MilitaryUnit& unit) {
        double quality_multiplier = 1.0;
        
        switch (unit.quality) {
            case UnitQuality::GREEN:
                quality_multiplier = 0.8;
                break;
            case UnitQuality::TRAINED:
                quality_multiplier = 1.0;
                break;
            case UnitQuality::EXPERIENCED:
                quality_multiplier = 1.2;
                break;
            case UnitQuality::ELITE:
                quality_multiplier = 1.5;
                break;
            case UnitQuality::LEGENDARY:
                quality_multiplier = 2.0;
                break;
        }
        
        unit.melee_attack *= quality_multiplier;
        unit.ranged_attack *= quality_multiplier;
        unit.defense *= quality_multiplier;
    }

    bool MilitaryRecruitmentSystem::ValidateRecruitmentRequirements(types::EntityID province_id, UnitType unit_type,
                                                                   types::SocialClass social_class) {
        // Check if unit type is viable for social class
        auto viable_types = GetViableUnitTypes(social_class);
        if (std::find(viable_types.begin(), viable_types.end(), unit_type) == viable_types.end()) {
            LogMilitaryActivity("Unit type " + GetUnitTypeName(unit_type) + " not viable for social class");
            return false;
        }
        
        // Check population availability
        if (!CheckPopulationAvailability(province_id, social_class, m_unit_definitions[unit_type].default_unit_size)) {
            return false;
        }
        
        // Check financial capacity
        if (!CheckFinancialCapacity(province_id, unit_type)) {
            return false;
        }
        
        // Check equipment availability
        if (!CheckEquipmentAvailability(province_id, unit_type, 1)) {
            return false;
        }
        
        return true;
    }

    bool MilitaryRecruitmentSystem::CheckPopulationAvailability(types::EntityID province_id, 
                                                               types::SocialClass social_class, int needed) {
        auto* recruitment_comp = m_access_manager.GetComponent<RecruitmentPoolComponent>(province_id);
        if (!recruitment_comp) {
            return false;
        }
        
        int available = GetAvailableRecruits(province_id, social_class);
        return available >= needed;
    }

    bool MilitaryRecruitmentSystem::CheckFinancialCapacity(types::EntityID province_id, UnitType unit_type) {
        auto* economic_comp = m_access_manager.GetComponent<economics::EconomicComponent>(province_id);
        if (!economic_comp) {
            return false;
        }
        
        double recruitment_cost = CalculateRecruitmentCost(unit_type, GetOptimalRecruitmentClass(unit_type));
        double available_funds = economic_comp->GetTreasuryAmount();
        
        return available_funds >= recruitment_cost;
    }

    void MilitaryRecruitmentSystem::RemovePopulationForRecruitment(types::EntityID province_id, 
                                                                  types::SocialClass social_class, int count) {
        auto* recruitment_comp = m_access_manager.GetComponent<RecruitmentPoolComponent>(province_id);
        if (!recruitment_comp) {
            return;
        }
        
        recruitment_comp->available_recruits[social_class] -= count;
        recruitment_comp->class_data[social_class].currently_serving += count;
        recruitment_comp->class_data[social_class].lifetime_recruited += count;
    }

    void MilitaryRecruitmentSystem::ApplyRecruitmentEffectsToPopulation(types::EntityID province_id, 
                                                                       types::SocialClass social_class, int recruited) {
        auto* population_comp = m_access_manager.GetComponent<population::PopulationComponent>(province_id);
        if (!population_comp) {
            return;
        }
        
        // Remove population from civilian pool
        population_comp->RemovePopulation(social_class, recruited);
        
        // Slight happiness decrease due to family members leaving
        double happiness_impact = -static_cast<double>(recruited) / 1000.0; // Small negative impact
        population_comp->ModifyHappiness(happiness_impact);
    }

    void MilitaryRecruitmentSystem::ChargeRecruitmentCosts(types::EntityID province_id, double cost) {
        auto* economic_comp = m_access_manager.GetComponent<economics::EconomicComponent>(province_id);
        if (!economic_comp) {
            return;
        }
        
        economic_comp->SpendFromTreasury(cost);
    }

    void MilitaryRecruitmentSystem::ProcessUpkeepPayments(types::EntityID province_id) {
        ProcessMilitaryExpenses(province_id);
    }

    void MilitaryRecruitmentSystem::HandleInsufficientFunds(types::EntityID province_id, double shortfall) {
        LogMilitaryActivity("Insufficient funds in province " + std::to_string(province_id) + 
                           " - shortfall: " + std::to_string(shortfall));
        
        // Reduce unit morale and effectiveness due to lack of pay
        auto* military_comp = m_access_manager.GetComponent<MilitaryComponent>(province_id);
        if (military_comp) {
            for (auto& unit : military_comp->active_units) {
                unit.morale *= 0.9; // 10% morale reduction
                unit.equipment_quality *= 0.95; // Equipment degrades faster
            }
            
            // Publish upkeep shortfall event
            messages::MilitaryUpkeepShortfall shortfall_msg;
            shortfall_msg.province_id = province_id;
            shortfall_msg.shortfall_amount = shortfall;
            shortfall_msg.consequences = "Reduced morale and equipment quality";
            
            for (const auto& unit : military_comp->active_units) {
                shortfall_msg.affected_units.push_back(unit.unit_id);
            }
            
            m_message_bus.Publish(shortfall_msg);
        }
    }

    void MilitaryRecruitmentSystem::ProcessBasicTraining(MilitaryUnit& unit, double training_efficiency, float time_delta) {
        double training_gain = 0.01 * training_efficiency * (time_delta / 30.0); // Monthly base
        unit.experience += training_gain;
        unit.experience = std::min(1.0, unit.experience);
    }

    void MilitaryRecruitmentSystem::ApplyExperienceGains(MilitaryUnit& unit, double base_experience, double time_factor) {
        UpdateUnitExperience(unit, base_experience, static_cast<float>(time_factor));
    }

    void MilitaryRecruitmentSystem::CheckForPromotions(MilitaryUnit& unit) {
        // Promotions are handled in ImproveUnitQuality
    }

    void MilitaryRecruitmentSystem::PublishUnitRecruited(const MilitaryUnit& unit, types::EntityID province_id, int cost) {
        messages::UnitRecruited recruitment_msg;
        recruitment_msg.unit = unit;
        recruitment_msg.province_id = province_id;
        recruitment_msg.recruitment_cost = cost;
        recruitment_msg.recruitment_reason = "Standard recruitment";
        recruitment_msg.timestamp = std::chrono::steady_clock::now();
        
        m_message_bus.Publish(recruitment_msg);
    }

    void MilitaryRecruitmentSystem::PublishUnitDisbanded(types::EntityID unit_id, types::EntityID province_id,
                                                        UnitType unit_type, int returned_pop) {
        messages::UnitDisbanded disbandment_msg;
        disbandment_msg.unit_id = unit_id;
        disbandment_msg.province_id = province_id;
        disbandment_msg.unit_type = unit_type;
        disbandment_msg.returned_population = returned_pop;
        disbandment_msg.disbandment_reason = "Administrative disbandment";
        
        m_message_bus.Publish(disbandment_msg);
    }

    void MilitaryRecruitmentSystem::PublishRecruitmentCrisis(types::EntityID province_id, 
                                                            const std::string& crisis_type, double severity) {
        messages::RecruitmentCrisis crisis_msg;
        crisis_msg.province_id = province_id;
        crisis_msg.crisis_type = crisis_type;
        crisis_msg.severity = severity;
        crisis_msg.description = "Military recruitment crisis in province " + std::to_string(province_id);
        
        m_message_bus.Publish(crisis_msg);
    }

    // ============================================================================
    // Initialization and Configuration Methods
    // ============================================================================

    void MilitaryRecruitmentSystem::InitializeUnitDefinitions() {
        // Initialize unit definitions with proper resource requirements and costs
        m_unit_definitions[UnitType::LEVY_SPEARMEN] = {
            "Levy Spearmen",
            {types::SocialClass::PEASANTS, types::SocialClass::FREE_PEASANT},
            {{types::ResourceType::IRON, 50}, {types::ResourceType::WOOD, 100}},
            {{types::ResourceType::FOOD, 30.0}},
            5.0, 50.0, 800, UnitQuality::GREEN, UnitQuality::TRAINED
        };
        
        m_unit_definitions[UnitType::PROFESSIONAL_INFANTRY] = {
            "Professional Infantry",
            {types::SocialClass::FREE_PEASANT, types::SocialClass::CRAFTSMEN},
            {{types::ResourceType::IRON, 100}, {types::ResourceType::LEATHER, 50}},
            {{types::ResourceType::FOOD, 40.0}},
            15.0, 120.0, 600, UnitQuality::GREEN, UnitQuality::ELITE
        };
        
        // Add more unit definitions as needed
    }

    void MilitaryRecruitmentSystem::LoadMilitaryConfiguration() {
        // Load from GameConfig if available
        auto& config = core::config::GameConfig::Instance();
        
        m_global_recruitment_modifier = config.GetFloat("military.recruitment_modifier", 1.0);
        m_global_upkeep_modifier = config.GetFloat("military.upkeep_modifier", 1.0);
        m_global_training_modifier = config.GetFloat("military.training_modifier", 1.0);
        m_military_budget_percentage = config.GetFloat("military.budget_percentage", 0.3);
    }

    void MilitaryRecruitmentSystem::SetupDefaultRecruitmentPools() {
        // Setup will be done when provinces are created
    }

    // ============================================================================
    // Utility Method Implementations
    // ============================================================================

    std::string MilitaryRecruitmentSystem::GetUnitTypeName(UnitType unit_type) {
        switch (unit_type) {
            case UnitType::LEVY_SPEARMEN: return "Levy Spearmen";
            case UnitType::PROFESSIONAL_INFANTRY: return "Professional Infantry";
            case UnitType::CROSSBOWMEN: return "Crossbowmen";
            case UnitType::PIKEMEN: return "Pikemen";
            case UnitType::HEAVY_INFANTRY: return "Heavy Infantry";
            case UnitType::LIGHT_CAVALRY: return "Light Cavalry";
            case UnitType::HEAVY_CAVALRY: return "Heavy Cavalry";
            case UnitType::HORSE_ARCHERS: return "Horse Archers";
            case UnitType::ENGINEERS: return "Engineers";
            case UnitType::ARTILLERY_CREW: return "Artillery Crew";
            case UnitType::MARINES: return "Marines";
            case UnitType::KNIGHTS: return "Knights";
            case UnitType::RETINUE: return "Retinue";
            case UnitType::COMMANDERS: return "Commanders";
            default: return "Unknown Unit";
        }
    }

    std::string MilitaryRecruitmentSystem::GetQualityName(UnitQuality quality) {
        switch (quality) {
            case UnitQuality::GREEN: return "Green";
            case UnitQuality::TRAINED: return "Trained";
            case UnitQuality::EXPERIENCED: return "Experienced";
            case UnitQuality::ELITE: return "Elite";
            case UnitQuality::LEGENDARY: return "Legendary";
            default: return "Unknown";
        }
    }

    void MilitaryRecruitmentSystem::LogMilitaryActivity(const std::string& message) {
        std::cout << "[MilitaryRecruitment] " << message << std::endl;
    }

} // namespace game::military
