// ============================================================================
// MilitarySystem.cpp - Military System Implementation (REBUILT FROM WORKING TEMPLATE)
// Created: October 12, 2025
// Based on: PopulationSystem.cpp (proven working ECS integration)
// Location: src/game/military/MilitarySystem.cpp
// ============================================================================

#include "game/military/MilitarySystem.h"
#include "game/military/MilitaryComponents.h"
#include "game/military/BattleResolutionCalculator.h"
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
                                 ::core::threading::ThreadSafeMessageBus& message_bus)
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
    // Combat Operations
    // ============================================================================

    void MilitarySystem::InitiateBattle(game::types::EntityID attacker_army, game::types::EntityID defender_army) {
        if (!m_initialized) {
            ::core::logging::LogError("MilitarySystem", "System not initialized");
            return;
        }

        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            ::core::logging::LogError("MilitarySystem", "EntityManager not available");
            return;
        }

        // Get army components
        ::core::ecs::EntityID attacker_handle(static_cast<uint64_t>(attacker_army), 1);
        ::core::ecs::EntityID defender_handle(static_cast<uint64_t>(defender_army), 1);

        auto attacker_comp = entity_manager->GetComponent<ArmyComponent>(attacker_handle);
        auto defender_comp = entity_manager->GetComponent<ArmyComponent>(defender_handle);

        if (!attacker_comp || !defender_comp) {
            ::core::logging::LogError("MilitarySystem", "Failed to get army components for battle");
            return;
        }

        // Mark armies as in battle
        attacker_comp->is_in_battle = true;
        defender_comp->is_in_battle = true;

        // Create a battle entity and combat component
        auto battle_entity = entity_manager->CreateEntity();
        auto combat_comp = entity_manager->AddComponent<CombatComponent>(battle_entity);

        if (combat_comp) {
            combat_comp->battle_id = battle_entity.id;
            combat_comp->attacker_army = attacker_army;
            combat_comp->defender_army = defender_army;
            combat_comp->location = attacker_comp->current_location;
            combat_comp->battle_active = true;
            combat_comp->attacker_initial_strength = attacker_comp->total_strength;
            combat_comp->defender_initial_strength = defender_comp->total_strength;
            combat_comp->attacker_morale = attacker_comp->army_morale;
            combat_comp->defender_morale = defender_comp->army_morale;

            // Set battle IDs in army components
            attacker_comp->battle_id = combat_comp->battle_id;
            defender_comp->battle_id = combat_comp->battle_id;

            // Add to active battles list
            m_active_battles.push_back(combat_comp->battle_id);

            ::core::logging::LogInfo("MilitarySystem",
                "Battle initiated between armies " + std::to_string(static_cast<int>(attacker_army)) +
                " and " + std::to_string(static_cast<int>(defender_army)));
        }
    }

    void MilitarySystem::ProcessBattle(game::types::EntityID battle_id, float battle_duration) {
        if (!m_initialized) {
            return;
        }

        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return;
        }

        ::core::ecs::EntityID battle_handle(static_cast<uint64_t>(battle_id), 1);
        auto combat_comp = entity_manager->GetComponent<CombatComponent>(battle_handle);

        if (!combat_comp || !combat_comp->battle_active) {
            return;
        }

        // Update battle duration
        combat_comp->battle_duration += battle_duration;

        ::core::logging::LogDebug("MilitarySystem",
            "Processing battle " + std::to_string(static_cast<int>(battle_id)) +
            " duration: " + std::to_string(combat_comp->battle_duration));

        // After sufficient duration, resolve the battle
        if (combat_comp->battle_duration >= 1.0) {
            ResolveBattle(battle_id);
        }
    }

    void MilitarySystem::ResolveBattle(game::types::EntityID battle_id) {
        if (!m_initialized) {
            ::core::logging::LogError("MilitarySystem", "System not initialized");
            return;
        }

        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            ::core::logging::LogError("MilitarySystem", "EntityManager not available");
            return;
        }

        ::core::ecs::EntityID battle_handle(static_cast<uint64_t>(battle_id), 1);
        auto combat_comp = entity_manager->GetComponent<CombatComponent>(battle_handle);

        if (!combat_comp || !combat_comp->battle_active) {
            ::core::logging::LogError("MilitarySystem", "Invalid battle or battle not active");
            return;
        }

        // Get army components
        ::core::ecs::EntityID attacker_handle(static_cast<uint64_t>(combat_comp->attacker_army), 1);
        ::core::ecs::EntityID defender_handle(static_cast<uint64_t>(combat_comp->defender_army), 1);

        auto attacker_comp = entity_manager->GetComponent<ArmyComponent>(attacker_handle);
        auto defender_comp = entity_manager->GetComponent<ArmyComponent>(defender_handle);

        if (!attacker_comp || !defender_comp) {
            ::core::logging::LogError("MilitarySystem", "Failed to get army components for battle resolution");
            return;
        }

        // Get commanders (if any)
        Commander* attacker_commander = nullptr;
        Commander* defender_commander = nullptr;
        // TODO: Retrieve actual commander objects when commander system is implemented

        // Get fortification component for defender's location
        ::core::ecs::EntityID location_handle(static_cast<uint64_t>(combat_comp->location), 1);
        auto fortification_comp = entity_manager->GetComponent<FortificationComponent>(location_handle);

        // Load battle configuration
        BattleConfig config = BattleResolutionCalculator::GetDefaultConfig();

        // Resolve the battle using the calculator
        BattleResult result = BattleResolutionCalculator::ResolveBattle(
            *attacker_comp,
            *defender_comp,
            *combat_comp,
            attacker_commander,
            defender_commander,
            fortification_comp.get(),
            config
        );

        // Apply casualties to armies
        ApplyCasualties(*attacker_comp, result.attacker_casualties);
        ApplyCasualties(*defender_comp, result.defender_casualties);

        // Update morale
        attacker_comp->army_morale = std::clamp(
            attacker_comp->army_morale + result.attacker_morale_change,
            0.0, 1.0
        );
        defender_comp->army_morale = std::clamp(
            defender_comp->army_morale + result.defender_morale_change,
            0.0, 1.0
        );

        // Update experience
        attacker_comp->battle_experience += result.attacker_experience_gain;
        defender_comp->battle_experience += result.defender_experience_gain;

        // Apply experience to individual units
        for (auto& unit : attacker_comp->units) {
            unit.experience += result.attacker_experience_gain * 0.1;
        }
        for (auto& unit : defender_comp->units) {
            unit.experience += result.defender_experience_gain * 0.1;
        }

        // Mark armies as no longer in battle
        attacker_comp->is_in_battle = false;
        defender_comp->is_in_battle = false;
        attacker_comp->battle_id = 0;
        defender_comp->battle_id = 0;

        // Mark battle as complete
        combat_comp->battle_active = false;
        combat_comp->attacker_casualties = result.attacker_casualties;
        combat_comp->defender_casualties = result.defender_casualties;

        // Log battle result
        std::string outcome_str = BattleResolutionCalculator::OutcomeToString(result.outcome);
        ::core::logging::LogInfo("MilitarySystem",
            "Battle resolved: " + outcome_str +
            " | Attacker casualties: " + std::to_string(result.attacker_casualties) +
            " | Defender casualties: " + std::to_string(result.defender_casualties));

        // Generate and log battle summary
        std::string summary = BattleResolutionCalculator::GenerateBattleSummary(
            result,
            attacker_comp->army_name.empty() ? "Attacker" : attacker_comp->army_name,
            defender_comp->army_name.empty() ? "Defender" : defender_comp->army_name,
            "Province " + std::to_string(static_cast<int>(combat_comp->location))
        );
        ::core::logging::LogInfo("MilitarySystem", "Battle Summary:\n" + summary);

        // Remove from active battles
        m_active_battles.erase(
            std::remove(m_active_battles.begin(), m_active_battles.end(), battle_id),
            m_active_battles.end()
        );

        // TODO: Publish battle result event
        // m_message_bus.Publish("BattleResolved",
        //     "Battle at location " + std::to_string(static_cast<int>(combat_comp->location)) +
        //     " resolved: " + outcome_str);
    }

    void MilitarySystem::ApplyCasualties(ArmyComponent& army, uint32_t total_casualties) {
        if (total_casualties == 0) return;

        uint32_t remaining_casualties = total_casualties;

        // Distribute casualties across units proportionally
        for (auto& unit : army.units) {
            if (remaining_casualties == 0) break;

            // Calculate this unit's share of casualties
            double unit_ratio = static_cast<double>(unit.current_strength) /
                               std::max(army.total_strength, 1u);
            uint32_t unit_casualties = std::min(
                static_cast<uint32_t>(remaining_casualties * unit_ratio),
                unit.current_strength
            );

            unit.ApplyLosses(unit_casualties);
            remaining_casualties -= unit_casualties;
        }

        // Recalculate army strength
        army.RecalculateStrength();
    }

    // ============================================================================
    // Army Management
    // ============================================================================

    void MilitarySystem::DisbandArmy(game::types::EntityID army_id) {
        // TODO: Implement
        ::core::logging::LogInfo("MilitarySystem", "DisbandArmy called for army " + std::to_string(static_cast<int>(army_id)));
    }

    void MilitarySystem::MoveArmy(game::types::EntityID army_id, game::types::EntityID destination) {
        // TODO: Implement
        ::core::logging::LogInfo("MilitarySystem", "MoveArmy called for army " + std::to_string(static_cast<int>(army_id)));
    }

    void MilitarySystem::AssignCommander(game::types::EntityID army_id, game::types::EntityID commander_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        ::core::ecs::EntityID army_handle(static_cast<uint64_t>(army_id), 1);
        auto army_comp = entity_manager->GetComponent<ArmyComponent>(army_handle);

        if (army_comp) {
            army_comp->commander_id = commander_id;
            ::core::logging::LogInfo("MilitarySystem",
                "Assigned commander " + std::to_string(static_cast<int>(commander_id)) +
                " to army " + std::to_string(static_cast<int>(army_id)));
        }
    }

    // ============================================================================
    // Siege Operations (Stubs)
    // ============================================================================

    void MilitarySystem::BeginSiege(game::types::EntityID besieging_army, game::types::EntityID target_province) {
        // TODO: Implement
        ::core::logging::LogInfo("MilitarySystem", "BeginSiege called");
    }

    void MilitarySystem::ProcessSiege(game::types::EntityID siege_id, float time_delta) {
        // TODO: Implement
    }

    void MilitarySystem::ResolveSiege(game::types::EntityID siege_id, bool attacker_success) {
        // TODO: Implement
        ::core::logging::LogInfo("MilitarySystem", "ResolveSiege called");
    }

    // ============================================================================
    // Military Development (Stubs)
    // ============================================================================

    void MilitarySystem::UpgradeTrainingFacilities(game::types::EntityID province_id, double investment) {
        // TODO: Implement
    }

    void MilitarySystem::ImproveEquipment(game::types::EntityID province_id, UnitType unit_type, double investment) {
        // TODO: Implement
    }

    void MilitarySystem::ConstructFortifications(game::types::EntityID province_id, uint32_t fortification_type, double cost) {
        // TODO: Implement
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