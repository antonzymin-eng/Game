// ============================================================================
// MilitarySystem.cpp - Military System Implementation (REBUILT FROM WORKING TEMPLATE)
// Created: October 12, 2025
// Based on: PopulationSystem.cpp (proven working ECS integration)
// Location: src/game/military/MilitarySystem.cpp
// ============================================================================

#include "game/military/MilitarySystem.h"
#include "game/military/MilitaryComponents.h"
#include "core/logging/Logger.h"
#include "game/config/GameConfig.h"
#include <json/json.h>
#include <iostream>
#include <algorithm>

namespace game::military {

    // ============================================================================
    // Constructor & Destructor
    // ============================================================================

    MilitarySystem::MilitarySystem(::core::ecs::ComponentAccessManager& access_manager,
                                 ::core::ecs::MessageBus& message_bus)
        : m_access_manager(access_manager)
        , m_message_bus(message_bus) {
        ::core::logging::LogInfo("MilitarySystem", "MilitarySystem constructor called");
    }

    // ============================================================================
    // System Lifecycle (ISystem interface)
    // ============================================================================

    void MilitarySystem::Initialize() {
        if (m_initialized) {
            ::core::logging::LogWarning("MilitarySystem", "System already initialized");
            return;
        }

        ::core::logging::LogInfo("MilitarySystem", "Initializing Military System...");
        
        InitializeUnitTemplates();
        InitializeTechnologyUnlocks();
        SubscribeToEvents();
        
        m_initialized = true;
        ::core::logging::LogInfo("MilitarySystem", "Military System initialized successfully");
    }

    void MilitarySystem::Update(float delta_time) {
        if (!m_initialized) {
            return;
        }

        m_accumulated_time += delta_time;
        m_monthly_timer += delta_time;
        
        // Regular updates every second
        if (m_accumulated_time >= m_update_interval) {
            ::core::logging::LogDebug("MilitarySystem", "Processing military updates");
            
            // Reset timer
            m_accumulated_time = 0.0f;
        }

        // Monthly processing (30 seconds = 1 month in-game)
        if (m_monthly_timer >= 30.0f) {
            ::core::logging::LogInfo("MilitarySystem", "Processing monthly military updates");
            m_monthly_timer = 0.0f;
        }
    }

    void MilitarySystem::Shutdown() {
        if (!m_initialized) {
            ::core::logging::LogWarning("MilitarySystem", "System not initialized");
            return;
        }

        ::core::logging::LogInfo("MilitarySystem", "Shutting down Military System...");
        m_initialized = false;
        ::core::logging::LogInfo("MilitarySystem", "Military System shutdown complete");
    }

    // ============================================================================
    // ECS Component Creation (Following PopulationSystem Pattern)
    // ============================================================================

    void MilitarySystem::CreateMilitaryComponents(game::types::EntityID entity_id) {
        if (!m_initialized) {
            ::core::logging::LogError("MilitarySystem", "System not initialized");
            return;
        }

        // Get EntityManager from ComponentAccessManager (PopulationSystem pattern)
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            ::core::logging::LogError("MilitarySystem", "EntityManager not available");
            return;
        }

        // Create EntityID handle for the entity (PopulationSystem pattern)
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);

        // Try to get existing MilitaryComponent or add a new one
        auto military_component = entity_manager->GetComponent<MilitaryComponent>(entity_handle);
        if (!military_component) {
            // Create new military component
            military_component = entity_manager->AddComponent<MilitaryComponent>(entity_handle);
            ::core::logging::LogInfo("MilitarySystem", "Created new MilitaryComponent for entity " + std::to_string(static_cast<int>(entity_id)));
        }

        if (military_component) {
            // Initialize military data
            military_component->military_budget = 1000.0; // Default budget
            military_component->recruitment_capacity = 100; // Default capacity
            
            // Enable basic unit types
            military_component->unit_type_available[UnitType::LEVIES] = true;
            military_component->unit_type_available[UnitType::SPEARMEN] = true;
            
            ::core::logging::LogInfo("MilitarySystem", "Initialized MilitaryComponent for entity " + std::to_string(static_cast<int>(entity_id)));
        }

        // Create FortificationComponent
        auto fortification_component = entity_manager->GetComponent<FortificationComponent>(entity_handle);
        if (!fortification_component) {
            fortification_component = entity_manager->AddComponent<FortificationComponent>(entity_handle);
            if (fortification_component) {
                // Initialize fortification data
                fortification_component->walls_level = 1;
                fortification_component->structural_integrity = 1.0;
                fortification_component->siege_resistance = 1.0;
                
                ::core::logging::LogInfo("MilitarySystem", "Created FortificationComponent for entity " + std::to_string(static_cast<int>(entity_id)));
            }
        }

        // Create MilitaryEventsComponent
        auto events_component = entity_manager->GetComponent<MilitaryEventsComponent>(entity_handle);
        if (!events_component) {
            events_component = entity_manager->AddComponent<MilitaryEventsComponent>(entity_handle);
            if (events_component) {
                ::core::logging::LogInfo("MilitarySystem", "Created MilitaryEventsComponent for entity " + std::to_string(static_cast<int>(entity_id)));
            }
        }
    }

    void MilitarySystem::CreateArmyComponents(game::types::EntityID army_id, const std::string& army_name) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            ::core::logging::LogError("MilitarySystem", "EntityManager not available");
            return;
        }

        ::core::ecs::EntityID army_handle(static_cast<uint64_t>(army_id), 1);

        // Create ArmyComponent  
        auto army_component = entity_manager->AddComponent<ArmyComponent>(army_handle, army_name);
        if (army_component) {
            army_component->army_name = army_name;
            army_component->home_province = army_id; // Temp: assuming army starts at home
            army_component->current_location = army_id;
            army_component->movement_points = 3.0; // Default movement
            army_component->organization = 1.0; // Full organization
            army_component->army_morale = 0.8; // Good morale
            
            ::core::logging::LogInfo("MilitarySystem", "Created ArmyComponent: " + army_name);
        }
    }

    // ============================================================================
    // Component Access Methods (Following Working Pattern)
    // ============================================================================

    MilitaryComponent* MilitarySystem::GetMilitaryComponent(types::EntityID province_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return nullptr;
        
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(province_id), 1);
        auto component = entity_manager->GetComponent<MilitaryComponent>(entity_handle);
        return component.get();
    }

    const MilitaryComponent* MilitarySystem::GetMilitaryComponent(types::EntityID province_id) const {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return nullptr;
        
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(province_id), 1);
        auto component = entity_manager->GetComponent<MilitaryComponent>(entity_handle);
        return component.get();
    }

    // ============================================================================
    // Military Operations (Simplified Working Implementations)
    // ============================================================================

    bool MilitarySystem::RecruitUnit(game::types::EntityID province_id, UnitType unit_type, uint32_t quantity) {
        if (!m_initialized) {
            ::core::logging::LogError("MilitarySystem", "System not initialized");
            return false;
        }

        auto* military_comp = GetMilitaryComponent(province_id);
        if (!military_comp) {
            CreateMilitaryComponents(province_id);
            military_comp = GetMilitaryComponent(province_id);
        }

        if (!military_comp) {
            ::core::logging::LogError("MilitarySystem", "Failed to create military component");
            return false;
        }

        // Simple recruitment logic
        if (military_comp->military_budget >= (quantity * 50.0)) { // 50 gold per unit
            MilitaryUnit new_unit;
            new_unit.type = unit_type;
            new_unit.current_strength = quantity;
            new_unit.max_strength = quantity;
            new_unit.recruitment_cost = quantity * 50.0;
            
            military_comp->garrison_units.push_back(new_unit);
            military_comp->military_budget -= new_unit.recruitment_cost;
            
            ::core::logging::LogInfo("MilitarySystem", "Recruited " + std::to_string(quantity) + " units for province " + std::to_string(static_cast<int>(province_id)));
            return true;
        }

        return false;
    }

    uint32_t MilitarySystem::GetTotalMilitaryStrength(game::types::EntityID province_id) const {
        auto* military_comp = GetMilitaryComponent(province_id);
        return military_comp ? military_comp->GetTotalGarrisonStrength() : 0;
    }

    double MilitarySystem::GetMilitaryMaintenance(game::types::EntityID province_id) const {
        auto* military_comp = GetMilitaryComponent(province_id);
        if (!military_comp) return 0.0;

        double total_maintenance = 0.0;
        for (const auto& unit : military_comp->garrison_units) {
            total_maintenance += unit.monthly_maintenance;
        }
        return total_maintenance;
    }

    std::vector<game::types::EntityID> MilitarySystem::GetAllArmies() const {
        // TODO: Implement when entity iteration API is available
        return {};
    }

    // ============================================================================
    // Stub Methods (To Be Implemented Later)
    // ============================================================================

    void MilitarySystem::DisbandUnit(game::types::EntityID province_id, size_t unit_index) {
        // TODO: Implement
    }

    void MilitarySystem::MergeUnits(game::types::EntityID province_id, size_t unit_a, size_t unit_b) {
        // TODO: Implement
    }

    void MilitarySystem::SplitUnit(game::types::EntityID province_id, size_t unit_index, uint32_t split_size) {
        // TODO: Implement
    }

    game::types::EntityID MilitarySystem::CreateArmy(game::types::EntityID home_province, const std::string& army_name) {
        // TODO: Implement
        return home_province; // Temporary
    }

    // ============================================================================
    // Internal Helper Methods
    // ============================================================================

    void MilitarySystem::InitializeUnitTemplates() {
        ::core::logging::LogDebug("MilitarySystem", "Initializing unit templates");
        // TODO: Load from configuration
    }

    void MilitarySystem::InitializeTechnologyUnlocks() {
        ::core::logging::LogDebug("MilitarySystem", "Initializing technology unlocks");
        // TODO: Load from configuration
    }

    void MilitarySystem::SubscribeToEvents() {
        ::core::logging::LogDebug("MilitarySystem", "Subscribing to events");
        // TODO: Subscribe to relevant events
    }

    MilitaryUnit MilitarySystem::CreateUnitTemplate(UnitType unit_type) const {
        MilitaryUnit unit;
        unit.type = unit_type;
        
        // Default values based on unit type
        switch (unit_type) {
            case UnitType::LEVIES:
                unit.max_strength = 1000;
                unit.attack_strength = 6.0;
                unit.defense_strength = 5.0;
                unit.recruitment_cost = 50.0;
                unit.monthly_maintenance = 5.0;
                break;
            case UnitType::SPEARMEN:
                unit.max_strength = 800;
                unit.attack_strength = 8.0;
                unit.defense_strength = 10.0;
                unit.recruitment_cost = 80.0;
                unit.monthly_maintenance = 8.0;
                break;
            default:
                unit.max_strength = 500;
                unit.attack_strength = 5.0;
                unit.defense_strength = 5.0;
                unit.recruitment_cost = 100.0;
                unit.monthly_maintenance = 10.0;
                break;
        }
        
        return unit;
    }

    // ============================================================================
    // Serialization Methods (ISerializable Interface)
    // ============================================================================

    Json::Value MilitarySystem::Serialize(int version) const {
        Json::Value root;
        root["version"] = version;
        root["system_name"] = "MilitarySystem";
        root["initialized"] = m_initialized;
        
        // Serialize basic system state
        root["recruitment_active"] = true; // Default state
        root["total_units_created"] = 0;   // Could track statistics
        
        // Note: Detailed component data is serialized by the ECS system itself
        // This method only saves system-specific configuration and state
        
        ::core::logging::LogInfo("MilitarySystem", "Serialization completed for version " + std::to_string(version));
        return root;
    }

    bool MilitarySystem::Deserialize(const Json::Value& data, int version) {
        if (!data.isObject()) {
            ::core::logging::LogError("MilitarySystem", "Invalid serialization data - not an object");
            return false;
        }
        
        if (!data.isMember("version") || !data.isMember("system_name")) {
            ::core::logging::LogError("MilitarySystem", "Missing required serialization fields");
            return false;
        }
        
        if (data["system_name"].asString() != "MilitarySystem") {
            ::core::logging::LogError("MilitarySystem", "Serialization data is for wrong system type");
            return false;
        }
        
        // Restore system state
        if (data.isMember("initialized")) {
            m_initialized = data["initialized"].asBool();
        }
        
        // Note: Detailed component data is deserialized by the ECS system itself
        // This method only restores system-specific configuration and state
        
        ::core::logging::LogInfo("MilitarySystem", "Deserialization completed for version " + std::to_string(version));
        return true;
    }

    // ============================================================================
    // Threading Strategy
    // ============================================================================

    ::core::threading::ThreadingStrategy MilitarySystem::GetThreadingStrategy() const {
        return ::core::threading::ThreadingStrategy::THREAD_POOL;
    }

} // namespace game::military} // namespace game::military