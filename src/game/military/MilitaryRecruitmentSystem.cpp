// ============================================================================
// MilitaryRecruitmentSystem.cpp - Military Recruitment System (REBUILT FROM WORKING TEMPLATE)
// Created: October 12, 2025
// Based on: PopulationSystem.cpp and MilitarySystem.cpp (proven working ECS integration)
// Location: src/game/military/MilitaryRecruitmentSystem.cpp
// ============================================================================

#include "game/military/MilitaryRecruitmentSystem.h"
#include "game/military/MilitaryComponents.h"
#include "game/population/PopulationComponents.h"
#include "core/logging/Logger.h"
#include "game/config/GameConfig.h"
#include <json/json.h>
#include <iostream>
#include <algorithm>

namespace game::military {

    // ============================================================================
    // Constructor & Destructor
    // ============================================================================

    MilitaryRecruitmentSystem::MilitaryRecruitmentSystem(::core::ecs::ComponentAccessManager& access_manager,
                                                       ::core::ecs::MessageBus& message_bus)
        : m_access_manager(access_manager)
        , m_message_bus(message_bus) {
        ::core::logging::LogInfo("MilitaryRecruitmentSystem", "MilitaryRecruitmentSystem constructor called");
    }

    // ============================================================================
    // System Lifecycle (ISystem interface)
    // ============================================================================

    void MilitaryRecruitmentSystem::Initialize() {
        if (m_initialized) {
            ::core::logging::LogWarning("MilitaryRecruitmentSystem", "System already initialized");
            return;
        }

        ::core::logging::LogInfo("MilitaryRecruitmentSystem", "Initializing Military Recruitment System...");
        
        InitializeUnitDefinitions();
        LoadMilitaryConfiguration();
        SetupDefaultRecruitmentPools();
        
        m_initialized = true;
        ::core::logging::LogInfo("MilitaryRecruitmentSystem", "Military Recruitment System initialized successfully");
    }

    void MilitaryRecruitmentSystem::Update(float delta_time) {
        if (!m_initialized) {
            return;
        }

        m_accumulated_time += delta_time;
        m_monthly_timer += delta_time;
        
        // Daily recruitment updates
        if (m_accumulated_time >= 1.0f) { // 1 second = 1 day
            ::core::logging::LogDebug("MilitaryRecruitmentSystem", "Processing daily recruitment updates");
            
            // Reset timer
            m_accumulated_time = 0.0f;
        }

        // Monthly recruitment processing
        if (m_monthly_timer >= 30.0f) { // 30 seconds = 1 month
            ::core::logging::LogInfo("MilitaryRecruitmentSystem", "Processing monthly recruitment cycle");
            UpdateRecruitmentPools();
            m_monthly_timer = 0.0f;
        }
    }

    void MilitaryRecruitmentSystem::Shutdown() {
        if (!m_initialized) {
            ::core::logging::LogWarning("MilitaryRecruitmentSystem", "System not initialized");
            return;
        }

        ::core::logging::LogInfo("MilitaryRecruitmentSystem", "Shutting down Military Recruitment System...");
        m_initialized = false;
        ::core::logging::LogInfo("MilitaryRecruitmentSystem", "Military Recruitment System shutdown complete");
    }

    std::string MilitaryRecruitmentSystem::GetSystemName() const {
        return "MilitaryRecruitmentSystem";
    }

    // ============================================================================
    // Core Recruitment Interface (Simplified Working Implementations)
    // ============================================================================

        std::vector<MilitaryUnit> MilitaryRecruitmentSystem::EmergencyRecruitment(game::types::EntityID province_id,
                                                                            int target_strength) {
        std::vector<MilitaryUnit> recruited_units;
        
        if (!m_initialized) {
            ::core::logging::LogError("MilitaryRecruitmentSystem", "System not initialized");
            return recruited_units;
        }

        // Get EntityManager from ComponentAccessManager (PopulationSystem pattern)
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            ::core::logging::LogError("MilitaryRecruitmentSystem", "EntityManager not available");
            return recruited_units;
        }

        // Create EntityID handle for the province
        ::core::ecs::EntityID province_handle(static_cast<uint64_t>(province_id), 1);

        // Get MilitaryComponent
        auto military_component = entity_manager->GetComponent<game::military::MilitaryComponent>(province_handle);
        if (!military_component) {
            ::core::logging::LogError("MilitaryRecruitmentSystem", "No MilitaryComponent found for province " + std::to_string(static_cast<int>(province_id)));
            return recruited_units;
        }

        // Get PopulationComponent for recruitment source
        auto population_component = entity_manager->GetComponent<game::population::PopulationComponent>(province_handle);
        if (!population_component) {
            ::core::logging::LogError("MilitaryRecruitmentSystem", "No PopulationComponent found for recruitment");
            return recruited_units;
        }

        // Simple recruitment logic - create LEVIES (basic units) for emergency
        UnitType unit_type = UnitType::LEVIES;
        double recruitment_cost = CalculateRecruitmentCost(unit_type, game::population::SocialClass::FREE_PEASANTS);
        
        if (military_component->military_budget >= recruitment_cost && population_component->total_population > target_strength) {
            
            // Create and add the unit
            MilitaryUnit new_unit;
            new_unit.type = unit_type;
            new_unit.current_strength = target_strength;
            new_unit.max_strength = target_strength;
            new_unit.recruitment_cost = recruitment_cost;
            
            recruited_units.push_back(new_unit);
            
            ::core::logging::LogInfo("MilitaryRecruitmentSystem", 
                "Successfully recruited " + std::to_string(target_strength) + 
                " emergency units for province " + std::to_string(static_cast<int>(province_id)));
        }

        return recruited_units;
    }

    bool MilitaryRecruitmentSystem::DisbandUnit(game::types::EntityID unit_id) {
        // TODO: Implement when unit tracking is available
        ::core::logging::LogInfo("MilitaryRecruitmentSystem", "DisbandUnit called - implementation pending");
        return true;
    }

    bool MilitaryRecruitmentSystem::DisbandAllUnits(game::types::EntityID province_id) {
        // TODO: Implement
        ::core::logging::LogInfo("MilitaryRecruitmentSystem", "DisbandAllUnits called - implementation pending");
        return true;
    }

    // ============================================================================
    // Query Methods
    // ============================================================================

    int MilitaryRecruitmentSystem::GetAvailableRecruits(game::types::EntityID province_id, 
                                                       game::population::SocialClass social_class) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return 0;

        ::core::ecs::EntityID province_handle(static_cast<uint64_t>(province_id), 1);
        auto population_component = entity_manager->GetComponent<game::population::PopulationComponent>(province_handle);
        
        if (population_component) {
            // Simple calculation: 10% of population can be recruited
            return static_cast<int>(population_component->total_population * 0.1);
        }
        return 0;
    }

    int MilitaryRecruitmentSystem::GetTotalRecruitmentCapacity(game::types::EntityID province_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return 0;

        ::core::ecs::EntityID province_handle(static_cast<uint64_t>(province_id), 1);
        auto military_component = entity_manager->GetComponent<MilitaryComponent>(province_handle);
        
        return military_component ? military_component->recruitment_capacity : 0;
    }

    std::vector<UnitType> MilitaryRecruitmentSystem::GetViableUnitTypes(game::population::SocialClass social_class) {
        std::vector<UnitType> viable_types;
        
        // Simple mapping based on social class
        switch (social_class) {
            case game::population::SocialClass::SERFS:
                viable_types.push_back(UnitType::LEVIES);
                break;
            case game::population::SocialClass::FREE_PEASANTS:
                viable_types.push_back(UnitType::LEVIES);
                viable_types.push_back(UnitType::SPEARMEN);
                break;
            case game::population::SocialClass::CRAFTSMEN:
                viable_types.push_back(UnitType::CROSSBOWMEN);
                viable_types.push_back(UnitType::CATAPULTS);
                break;
            case game::population::SocialClass::WEALTHY_MERCHANTS:
                viable_types.push_back(UnitType::LIGHT_CAVALRY);
                break;
            case game::population::SocialClass::LESSER_NOBILITY:
            case game::population::SocialClass::HIGH_NOBILITY:
                viable_types.push_back(UnitType::HEAVY_CAVALRY);
                viable_types.push_back(UnitType::MEN_AT_ARMS);
                break;
            default:
                viable_types.push_back(UnitType::LEVIES);
                break;
        }
        
        return viable_types;
    }

    game::population::SocialClass MilitaryRecruitmentSystem::GetOptimalRecruitmentClass(UnitType unit_type) {
        // Simple mapping
        switch (unit_type) {
            case UnitType::LEVIES:
                return game::population::SocialClass::SERFS;
            case UnitType::SPEARMEN:
            case UnitType::PIKEMEN:
                return game::population::SocialClass::FREE_PEASANTS;
            case UnitType::CROSSBOWMEN:
            case UnitType::CATAPULTS:
                return game::population::SocialClass::CRAFTSMEN;
            case UnitType::LIGHT_CAVALRY:
                return game::population::SocialClass::WEALTHY_MERCHANTS;
            case UnitType::HEAVY_CAVALRY:
            case UnitType::MEN_AT_ARMS:
                return game::population::SocialClass::LESSER_NOBILITY;
            default:
                return game::population::SocialClass::FREE_PEASANTS;
        }
    }

    // ============================================================================
    // Cost Calculation Methods
    // ============================================================================

    double MilitaryRecruitmentSystem::CalculateRecruitmentCost(UnitType unit_type, 
                                                              game::population::SocialClass social_class) {
        double base_cost = 50.0; // Base recruitment cost
        
        // Unit type modifiers
        switch (unit_type) {
            case UnitType::LEVIES:
                base_cost *= 0.5;
                break;
            case UnitType::SPEARMEN:
                base_cost *= 1.0;
                break;
            case UnitType::CROSSBOWMEN:
                base_cost *= 1.5;
                break;
            case UnitType::LIGHT_CAVALRY:
                base_cost *= 2.0;
                break;
            case UnitType::HEAVY_CAVALRY:
            case UnitType::MEN_AT_ARMS:
                base_cost *= 5.0;
                break;
            default:
                base_cost *= 1.0;
                break;
        }
        
        // Social class modifiers
        switch (social_class) {
            case game::population::SocialClass::SERFS:
                base_cost *= 0.8;
                break;
            case game::population::SocialClass::LESSER_NOBILITY:
                base_cost *= 2.0;
                break;
            case game::population::SocialClass::HIGH_NOBILITY:
                base_cost *= 3.0;
                break;
            default:
                base_cost *= 1.0;
                break;
        }
        
        return base_cost;
    }

    // ============================================================================
    // Internal Helper Methods (Stubs for now)
    // ============================================================================

    void MilitaryRecruitmentSystem::UpdateRecruitmentPools() {
        ::core::logging::LogDebug("MilitaryRecruitmentSystem", "Updating recruitment pools");
        // TODO: Implement recruitment pool updates
    }

    void MilitaryRecruitmentSystem::InitializeUnitDefinitions() {
        ::core::logging::LogDebug("MilitaryRecruitmentSystem", "Initializing unit definitions");
        // TODO: Load unit definitions from configuration
    }

    void MilitaryRecruitmentSystem::LoadMilitaryConfiguration() {
        ::core::logging::LogDebug("MilitaryRecruitmentSystem", "Loading military configuration");
        // TODO: Load configuration from files
    }

    void MilitaryRecruitmentSystem::SetupDefaultRecruitmentPools() {
        ::core::logging::LogDebug("MilitaryRecruitmentSystem", "Setting up default recruitment pools");
        // TODO: Initialize default recruitment pools
    }

    // ============================================================================
    // ISystem Interface Implementation
    // ============================================================================

    ::core::threading::ThreadingStrategy MilitaryRecruitmentSystem::GetThreadingStrategy() const {
        return ::core::threading::ThreadingStrategy::THREAD_POOL;
    }

    Json::Value MilitaryRecruitmentSystem::Serialize(int version) const {
        Json::Value data;
        data["system_name"] = "MilitaryRecruitmentSystem";
        data["version"] = version;
        data["initialized"] = m_initialized;
        // TODO: Serialize recruitment state
        return data;
    }

    bool MilitaryRecruitmentSystem::Deserialize(const Json::Value& data, int version) {
        if (data["system_name"].asString() != "MilitaryRecruitmentSystem") {
            return false;
        }
        m_initialized = data["initialized"].asBool();
        // TODO: Deserialize recruitment state
        return true;
    }

} // namespace game::military} // namespace game::military