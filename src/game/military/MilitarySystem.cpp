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
#include <fstream>

namespace game::military {

    // ============================================================================
    // Configuration Constants
    // ============================================================================

    namespace config {
        // Default initialization values
        constexpr double DEFAULT_MILITARY_BUDGET = 1000.0;
        constexpr uint32_t DEFAULT_RECRUITMENT_CAPACITY = 100;
        constexpr double DEFAULT_TRAINING_FACILITIES = 0.3;
        constexpr double DEFAULT_SUPPLY_INFRASTRUCTURE = 0.5;
        constexpr double DEFAULT_BARRACKS_LEVEL = 1.0;

        // Recruitment costs
        constexpr double UNIT_BASE_COST = 50.0;
        constexpr double DISBANDMENT_REFUND_RATIO = 0.2; // 20% refund on disbanding

        // Military development costs
        constexpr double TRAINING_FACILITY_COST_DIVISOR = 1000.0; // $1000 = +0.1 training
        constexpr double EQUIPMENT_UPGRADE_COST_DIVISOR = 500.0;  // $500 = +0.1 quality

        // Fortification construction
        constexpr double FORTIFICATION_SIEGE_RESISTANCE_BONUS = 0.1;
        constexpr double FORTIFICATION_ARTILLERY_BONUS = 0.1;
        constexpr double FORTIFICATION_INTEGRITY_BONUS = 0.05;
        constexpr uint32_t FORTIFICATION_GARRISON_CAPACITY_BONUS = 100;

        // Siege damage
        constexpr double SIEGE_SUCCESS_DAMAGE = 0.3; // 30% structural damage on successful siege

        // Update intervals (in seconds)
        constexpr float MONTHLY_UPDATE_INTERVAL = 30.0f; // 30 seconds = 1 month
    }


    // ============================================================================
    // Constructor & Destructor
    // ============================================================================

    MilitarySystem::MilitarySystem(::core::ecs::ComponentAccessManager& access_manager,
                                 ::core::threading::ThreadSafeMessageBus& message_bus)
        : m_access_manager(access_manager)
        , m_message_bus(message_bus) {
        CORE_LOG_INFO("MilitarySystem", "MilitarySystem constructor called");
    }

    // ============================================================================
    // System Lifecycle (ISystem interface)
    // ============================================================================

    void MilitarySystem::Initialize() {
        if (m_initialized) {
            CORE_LOG_WARN("MilitarySystem", "System already initialized");
            return;
        }

        CORE_LOG_INFO("MilitarySystem", "Initializing Military System...");
        
        InitializeUnitTemplates();
        InitializeTechnologyUnlocks();
        SubscribeToEvents();
        
        m_initialized = true;
        CORE_LOG_INFO("MilitarySystem", "Military System initialized successfully");
    }

    void MilitarySystem::Update(float delta_time) {
        if (!m_initialized) {
            return;
        }

        m_accumulated_time += delta_time;
        m_monthly_timer += delta_time;
        
        // Regular updates every second
        if (m_accumulated_time >= m_update_interval) {
            CORE_LOG_DEBUG("MilitarySystem", "Processing military updates");
            
            // Reset timer
            m_accumulated_time = 0.0f;
        }

        // Monthly processing
        if (m_monthly_timer >= config::MONTHLY_UPDATE_INTERVAL) {
            CORE_LOG_INFO("MilitarySystem", "Processing monthly military updates");
            m_monthly_timer = 0.0f;
        }

        // Periodic army registry cleanup (every 100 updates)
        m_update_counter++;
        if (m_update_counter >= 100) {
            PerformArmyRegistryCleanup();
            m_update_counter = 0;
        }
    }

    void MilitarySystem::Shutdown() {
        if (!m_initialized) {
            CORE_LOG_WARN("MilitarySystem", "System not initialized");
            return;
        }

        CORE_LOG_INFO("MilitarySystem", "Shutting down Military System...");
        m_initialized = false;
        CORE_LOG_INFO("MilitarySystem", "Military System shutdown complete");
    }

    // ============================================================================
    // ECS Component Creation (Following PopulationSystem Pattern)
    // ============================================================================

    void MilitarySystem::CreateMilitaryComponents(game::types::EntityID entity_id) {
        if (!m_initialized) {
            CORE_LOG_ERROR("MilitarySystem", "System not initialized");
            return;
        }

        // Get EntityManager from ComponentAccessManager (PopulationSystem pattern)
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            CORE_LOG_ERROR("MilitarySystem", "EntityManager not available");
            return;
        }

        // Create EntityID handle for the entity (PopulationSystem pattern)
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);

        // Try to get existing MilitaryComponent or add a new one
        auto military_component = entity_manager->GetComponent<MilitaryComponent>(entity_handle);
        if (!military_component) {
            // Create new military component
            military_component = entity_manager->AddComponent<MilitaryComponent>(entity_handle);
            CORE_LOG_INFO("MilitarySystem", "Created new MilitaryComponent for entity " + std::to_string(static_cast<int>(entity_id)));
        }

        if (military_component) {
            // Initialize military data with config constants
            military_component->military_budget = config::DEFAULT_MILITARY_BUDGET;
            military_component->recruitment_capacity = config::DEFAULT_RECRUITMENT_CAPACITY;
            military_component->training_facilities = config::DEFAULT_TRAINING_FACILITIES;
            military_component->supply_infrastructure = config::DEFAULT_SUPPLY_INFRASTRUCTURE;
            military_component->barracks_level = config::DEFAULT_BARRACKS_LEVEL;

            // Enable basic unit types
            military_component->unit_type_available[UnitType::LEVIES] = true;
            military_component->unit_type_available[UnitType::SPEARMEN] = true;

            CORE_LOG_INFO("MilitarySystem", "Initialized MilitaryComponent for entity " + std::to_string(static_cast<int>(entity_id)));
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
                
                CORE_LOG_INFO("MilitarySystem", "Created FortificationComponent for entity " + std::to_string(static_cast<int>(entity_id)));
            }
        }

        // Create MilitaryEventsComponent
        auto events_component = entity_manager->GetComponent<MilitaryEventsComponent>(entity_handle);
        if (!events_component) {
            events_component = entity_manager->AddComponent<MilitaryEventsComponent>(entity_handle);
            if (events_component) {
                CORE_LOG_INFO("MilitarySystem", "Created MilitaryEventsComponent for entity " + std::to_string(static_cast<int>(entity_id)));
            }
        }
    }

    void MilitarySystem::CreateArmyComponents(game::types::EntityID army_id, const std::string& army_name) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            CORE_LOG_ERROR("MilitarySystem", "EntityManager not available");
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
            
            CORE_LOG_INFO("MilitarySystem", "Created ArmyComponent: " + army_name);
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
            CORE_LOG_ERROR("MilitarySystem", "System not initialized");
            return false;
        }

        auto* military_comp = GetMilitaryComponent(province_id);
        if (!military_comp) {
            CreateMilitaryComponents(province_id);
            military_comp = GetMilitaryComponent(province_id);
        }

        if (!military_comp) {
            CORE_LOG_ERROR("MilitarySystem", "Failed to create military component");
            return false;
        }

        // Simple recruitment logic
        double total_cost = quantity * config::UNIT_BASE_COST;
        if (military_comp->military_budget >= total_cost) {
            MilitaryUnit new_unit;
            new_unit.type = unit_type;
            new_unit.current_strength = quantity;
            new_unit.max_strength = quantity;
            new_unit.recruitment_cost = total_cost;

            std::lock_guard<std::mutex> lock(military_comp->garrison_mutex);
            military_comp->garrison_units.push_back(new_unit);
            military_comp->military_budget -= new_unit.recruitment_cost;

            CORE_LOG_INFO("MilitarySystem", "Recruited " + std::to_string(quantity) + " units for province " + std::to_string(static_cast<int>(province_id)));

            // Publish recruitment event
            Json::Value recruitment_event;
            recruitment_event["event_type"] = "UnitRecruited";
            recruitment_event["province_id"] = static_cast<int>(province_id);
            recruitment_event["unit_type"] = static_cast<int>(unit_type);
            recruitment_event["quantity"] = quantity;
            recruitment_event["cost"] = new_unit.recruitment_cost;
            // TODO: Update to use typed message instead of deprecated string topic API
            // m_message_bus.Publish("military.unit_recruited", recruitment_event);

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

        std::lock_guard<std::mutex> lock(military_comp->garrison_mutex);
        double total_maintenance = 0.0;
        for (const auto& unit : military_comp->garrison_units) {
            total_maintenance += unit.monthly_maintenance;
        }
        return total_maintenance;
    }

    std::vector<game::types::EntityID> MilitarySystem::GetAllArmies() const {
        std::lock_guard<std::mutex> lock(m_armies_registry_mutex);

        // Return a copy of the army registry
        // Filter out disbanded/inactive armies
        std::vector<game::types::EntityID> active_armies;

        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            CORE_LOG_WARN("MilitarySystem", "EntityManager not available for GetAllArmies");
            return active_armies;
        }

        for (const auto& army_id : m_all_armies) {
            ::core::ecs::EntityID army_handle(static_cast<uint64_t>(army_id), 1);
            auto army_comp = entity_manager->GetComponent<ArmyComponent>(army_handle);

            // Only return active armies
            if (army_comp && army_comp->is_active) {
                active_armies.push_back(army_id);
            }
        }

        return active_armies;
    }

    // ============================================================================
    // Combat Operations
    // ============================================================================

    void MilitarySystem::InitiateBattle(game::types::EntityID attacker_army, game::types::EntityID defender_army) {
        if (!m_initialized) {
            CORE_LOG_ERROR("MilitarySystem", "System not initialized");
            return;
        }

        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            CORE_LOG_ERROR("MilitarySystem", "EntityManager not available");
            return;
        }

        // Get army components
        ::core::ecs::EntityID attacker_handle(static_cast<uint64_t>(attacker_army), 1);
        ::core::ecs::EntityID defender_handle(static_cast<uint64_t>(defender_army), 1);

        auto attacker_comp = entity_manager->GetComponent<ArmyComponent>(attacker_handle);
        auto defender_comp = entity_manager->GetComponent<ArmyComponent>(defender_handle);

        if (!attacker_comp || !defender_comp) {
            CORE_LOG_ERROR("MilitarySystem", "Failed to get army components for battle");
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
            {
                std::lock_guard<std::mutex> lock(m_active_battles_mutex);
                m_active_battles.push_back(combat_comp->battle_id);
            }

            CORE_LOG_INFO("MilitarySystem",
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

        CORE_LOG_DEBUG("MilitarySystem",
            "Processing battle " + std::to_string(static_cast<int>(battle_id)) +
            " duration: " + std::to_string(combat_comp->battle_duration));

        // After sufficient duration, resolve the battle
        if (combat_comp->battle_duration >= 1.0) {
            ResolveBattle(battle_id);
        }
    }

    void MilitarySystem::ResolveBattle(game::types::EntityID battle_id) {
        if (!m_initialized) {
            CORE_LOG_ERROR("MilitarySystem", "System not initialized");
            return;
        }

        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            CORE_LOG_ERROR("MilitarySystem", "EntityManager not available");
            return;
        }

        ::core::ecs::EntityID battle_handle(static_cast<uint64_t>(battle_id), 1);
        auto combat_comp = entity_manager->GetComponent<CombatComponent>(battle_handle);

        if (!combat_comp || !combat_comp->battle_active) {
            CORE_LOG_ERROR("MilitarySystem", "Invalid battle or battle not active");
            return;
        }

        // Get army components
        ::core::ecs::EntityID attacker_handle(static_cast<uint64_t>(combat_comp->attacker_army), 1);
        ::core::ecs::EntityID defender_handle(static_cast<uint64_t>(combat_comp->defender_army), 1);

        auto attacker_comp = entity_manager->GetComponent<ArmyComponent>(attacker_handle);
        auto defender_comp = entity_manager->GetComponent<ArmyComponent>(defender_handle);

        if (!attacker_comp || !defender_comp) {
            CORE_LOG_ERROR("MilitarySystem", "Failed to get army components for battle resolution");
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
        {
            std::lock_guard<std::mutex> lock(attacker_comp->units_mutex);
            for (auto& unit : attacker_comp->units) {
                unit.experience += result.attacker_experience_gain * 0.1;
            }
        }
        {
            std::lock_guard<std::mutex> lock(defender_comp->units_mutex);
            for (auto& unit : defender_comp->units) {
                unit.experience += result.defender_experience_gain * 0.1;
            }
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
        CORE_LOG_INFO("MilitarySystem",
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
        CORE_LOG_INFO("MilitarySystem", "Battle Summary:\n" + summary);

        // Remove from active battles
        {
            std::lock_guard<std::mutex> lock(m_active_battles_mutex);
            m_active_battles.erase(
                std::remove(m_active_battles.begin(), m_active_battles.end(), battle_id),
                m_active_battles.end()
            );
        }

        // Publish battle result event
        Json::Value battle_event;
        battle_event["event_type"] = "BattleResolved";
        battle_event["battle_id"] = static_cast<int>(battle_id);
        battle_event["location"] = static_cast<int>(combat_comp->location);
        battle_event["outcome"] = outcome_str;
        battle_event["attacker_army"] = static_cast<int>(combat_comp->attacker_army);
        battle_event["defender_army"] = static_cast<int>(combat_comp->defender_army);
        battle_event["attacker_casualties"] = result.attacker_casualties;
        battle_event["defender_casualties"] = result.defender_casualties;
        battle_event["attacker_name"] = attacker_comp->army_name.empty() ? "Attacker" : attacker_comp->army_name;
        battle_event["defender_name"] = defender_comp->army_name.empty() ? "Defender" : defender_comp->army_name;
        battle_event["summary"] = summary;

        // TODO: Update to use typed message instead of deprecated string topic API
        // m_message_bus.Publish("military.battle_resolved", battle_event);
    }

    void MilitarySystem::ApplyCasualties(ArmyComponent& army, uint32_t total_casualties) {
        if (total_casualties == 0) return;

        std::lock_guard<std::mutex> lock(army.units_mutex);

        // Validate army has strength
        if (army.total_strength == 0) {
            CORE_LOG_WARN("MilitarySystem", "Cannot apply casualties to army with zero strength");
            return;
        }

        uint32_t remaining_casualties = total_casualties;

        // Distribute casualties across units proportionally
        for (auto& unit : army.units) {
            if (remaining_casualties == 0) break;
            if (unit.current_strength == 0) continue; // Skip depleted units

            // Calculate this unit's share of casualties
            double unit_ratio = static_cast<double>(unit.current_strength) / static_cast<double>(army.total_strength);
            uint32_t unit_casualties = std::min(
                static_cast<uint32_t>(remaining_casualties * unit_ratio),
                unit.current_strength
            );

            unit.ApplyLosses(unit_casualties);
            remaining_casualties -= unit_casualties;
        }

        // Recalculate army strength (mutex already locked by calling RecalculateStrengthLocked)
        army.RecalculateStrengthLocked();
    }

    // ============================================================================
    // Army Management
    // ============================================================================

    void MilitarySystem::DisbandArmy(game::types::EntityID army_id) {
        if (!m_initialized) {
            CORE_LOG_ERROR("MilitarySystem", "System not initialized");
            return;
        }

        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            CORE_LOG_ERROR("MilitarySystem", "EntityManager not available");
            return;
        }

        ::core::ecs::EntityID army_handle(static_cast<uint64_t>(army_id), 1);
        auto army_comp = entity_manager->GetComponent<ArmyComponent>(army_handle);

        if (army_comp) {
            // Return units to home province garrison (optional enhancement)
            // For now, just mark as inactive
            army_comp->is_active = false;
            CORE_LOG_INFO("MilitarySystem",
                "Disbanded army " + army_comp->army_name +
                " (ID: " + std::to_string(static_cast<int>(army_id)) + ")");

            // Could remove component entirely, but marking inactive is safer
            // entity_manager->RemoveComponent<ArmyComponent>(army_handle);
        } else {
            CORE_LOG_WARN("MilitarySystem",
                "Army " + std::to_string(static_cast<int>(army_id)) + " not found");
        }
    }

    void MilitarySystem::MoveArmy(game::types::EntityID army_id, game::types::EntityID destination) {
        if (!m_initialized) {
            CORE_LOG_ERROR("MilitarySystem", "System not initialized");
            return;
        }

        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            CORE_LOG_ERROR("MilitarySystem", "EntityManager not available");
            return;
        }

        ::core::ecs::EntityID army_handle(static_cast<uint64_t>(army_id), 1);
        auto army_comp = entity_manager->GetComponent<ArmyComponent>(army_handle);

        if (!army_comp) {
            CORE_LOG_ERROR("MilitarySystem", "Army not found");
            return;
        }

        if (!army_comp->CanMove()) {
            CORE_LOG_WARN("MilitarySystem",
                "Army " + army_comp->army_name + " cannot move (insufficient movement points or in combat)");
            return;
        }

        // Update location
        army_comp->current_location = destination;

        // Consume movement points (simplified - could calculate based on distance/terrain)
        army_comp->movement_points = std::max(0.0, army_comp->movement_points - 1.0);

        CORE_LOG_INFO("MilitarySystem",
            "Army " + army_comp->army_name + " moved to province " +
            std::to_string(static_cast<int>(destination)));
    }

    void MilitarySystem::AssignCommander(game::types::EntityID army_id, game::types::EntityID commander_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        ::core::ecs::EntityID army_handle(static_cast<uint64_t>(army_id), 1);
        auto army_comp = entity_manager->GetComponent<ArmyComponent>(army_handle);

        if (army_comp) {
            army_comp->commander_id = commander_id;
            CORE_LOG_INFO("MilitarySystem",
                "Assigned commander " + std::to_string(static_cast<int>(commander_id)) +
                " to army " + std::to_string(static_cast<int>(army_id)));
        }
    }

    // ============================================================================
    // Siege Operations (Stubs)
    // ============================================================================

    void MilitarySystem::BeginSiege(game::types::EntityID besieging_army, game::types::EntityID target_province) {
        if (!m_initialized) {
            CORE_LOG_ERROR("MilitarySystem", "System not initialized");
            return;
        }

        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            CORE_LOG_ERROR("MilitarySystem", "EntityManager not available");
            return;
        }

        ::core::ecs::EntityID army_handle(static_cast<uint64_t>(besieging_army), 1);
        ::core::ecs::EntityID province_handle(static_cast<uint64_t>(target_province), 1);

        auto army_comp = entity_manager->GetComponent<ArmyComponent>(army_handle);
        auto fort_comp = entity_manager->GetComponent<FortificationComponent>(province_handle);

        if (!army_comp) {
            CORE_LOG_ERROR("MilitarySystem", "Army not found for siege");
            return;
        }

        if (!fort_comp) {
            CORE_LOG_WARN("MilitarySystem", "No fortifications at target province");
            return;
        }

        // Mark army as besieging
        army_comp->is_besieging = true;
        army_comp->siege_target = target_province;
        army_comp->current_location = target_province;

        // Mark fortification as under siege
        fort_comp->under_siege = true;
        fort_comp->besieging_army = besieging_army;
        fort_comp->siege_progress = 0.0;

        CORE_LOG_INFO("MilitarySystem",
            "Army " + army_comp->army_name + " began siege of province " +
            std::to_string(static_cast<int>(target_province)));

        // Publish siege start event
        Json::Value siege_event;
        siege_event["event_type"] = "SiegeStarted";
        siege_event["besieging_army"] = static_cast<int>(besieging_army);
        siege_event["target_province"] = static_cast<int>(target_province);
        siege_event["army_name"] = army_comp->army_name;
        siege_event["fortification_level"] = fort_comp->walls_level;
        // TODO: Update to use typed message instead of deprecated string topic API
        // m_message_bus.Publish("military.siege_started", siege_event);
    }

    void MilitarySystem::ProcessSiege(game::types::EntityID siege_id, float time_delta) {
        if (!m_initialized) return;

        // Simplified siege processing
        // In full implementation, would:
        // - Calculate siege progress based on attacker strength
        // - Apply attrition to besieging army
        // - Check supply levels
        // - Determine if fortification breached

        CORE_LOG_DEBUG("MilitarySystem",
            "Processing siege " + std::to_string(static_cast<int>(siege_id)) +
            " (delta: " + std::to_string(time_delta) + ")");
    }

    void MilitarySystem::ResolveSiege(game::types::EntityID siege_id, bool attacker_success) {
        if (!m_initialized) {
            CORE_LOG_ERROR("MilitarySystem", "System not initialized");
            return;
        }

        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        ::core::ecs::EntityID province_handle(static_cast<uint64_t>(siege_id), 1);
        auto fort_comp = entity_manager->GetComponent<FortificationComponent>(province_handle);

        if (!fort_comp || !fort_comp->under_siege) {
            CORE_LOG_WARN("MilitarySystem", "No active siege found");
            return;
        }

        ::core::ecs::EntityID army_handle(static_cast<uint64_t>(fort_comp->besieging_army), 1);
        auto army_comp = entity_manager->GetComponent<ArmyComponent>(army_handle);

        if (army_comp) {
            army_comp->is_besieging = false;
            army_comp->siege_target = 0;
        }

        fort_comp->under_siege = false;
        fort_comp->besieging_army = 0;

        if (attacker_success) {
            // Damage fortifications
            fort_comp->structural_integrity = std::max(0.0, fort_comp->structural_integrity - config::SIEGE_SUCCESS_DAMAGE);
            CORE_LOG_INFO("MilitarySystem", "Siege successful - fortifications breached");
        } else {
            CORE_LOG_INFO("MilitarySystem", "Siege failed - defenders held");
        }

        // Publish siege resolution event
        Json::Value siege_resolution_event;
        siege_resolution_event["event_type"] = "SiegeResolved";
        siege_resolution_event["province_id"] = static_cast<int>(siege_id);
        siege_resolution_event["success"] = attacker_success;
        siege_resolution_event["structural_damage"] = attacker_success ? config::SIEGE_SUCCESS_DAMAGE : 0.0;
        siege_resolution_event["remaining_integrity"] = fort_comp->structural_integrity;
        if (army_comp) {
            siege_resolution_event["army_name"] = army_comp->army_name;
        }
        // TODO: Update to use typed message instead of deprecated string topic API
        // m_message_bus.Publish("military.siege_resolved", siege_resolution_event);
    }

    // ============================================================================
    // Military Development (Stubs)
    // ============================================================================

    void MilitarySystem::UpgradeTrainingFacilities(game::types::EntityID province_id, double investment) {
        if (!m_initialized) {
            CORE_LOG_ERROR("MilitarySystem", "System not initialized");
            return;
        }

        auto* military_comp = GetMilitaryComponent(province_id);
        if (!military_comp) {
            CORE_LOG_ERROR("MilitarySystem", "Province military component not found");
            return;
        }

        if (military_comp->military_budget < investment) {
            CORE_LOG_WARN("MilitarySystem", "Insufficient budget for training facility upgrade");
            return;
        }

        // Improve training facilities
        double improvement = investment / config::TRAINING_FACILITY_COST_DIVISOR;
        military_comp->training_facilities = std::min(1.0, military_comp->training_facilities + improvement);
        military_comp->military_budget -= investment;

        CORE_LOG_INFO("MilitarySystem",
            "Training facilities upgraded to " + std::to_string(military_comp->training_facilities) +
            " for province " + std::to_string(static_cast<int>(province_id)));
    }

    void MilitarySystem::ImproveEquipment(game::types::EntityID province_id, UnitType unit_type, double investment) {
        if (!m_initialized) {
            CORE_LOG_ERROR("MilitarySystem", "System not initialized");
            return;
        }

        auto* military_comp = GetMilitaryComponent(province_id);
        if (!military_comp) {
            CORE_LOG_ERROR("MilitarySystem", "Province military component not found");
            return;
        }

        if (military_comp->military_budget < investment) {
            CORE_LOG_WARN("MilitarySystem", "Insufficient budget for equipment improvement");
            return;
        }

        // Improve equipment quality for this unit type
        double improvement = investment / config::EQUIPMENT_UPGRADE_COST_DIVISOR;
        military_comp->equipment_quality_modifiers[unit_type] += improvement;
        military_comp->military_budget -= investment;

        CORE_LOG_INFO("MilitarySystem",
            "Equipment improved for unit type " + std::to_string(static_cast<int>(unit_type)) +
            " in province " + std::to_string(static_cast<int>(province_id)));
    }

    void MilitarySystem::ConstructFortifications(game::types::EntityID province_id, uint32_t fortification_type, double cost) {
        if (!m_initialized) {
            CORE_LOG_ERROR("MilitarySystem", "System not initialized");
            return;
        }

        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            CORE_LOG_ERROR("MilitarySystem", "EntityManager not available");
            return;
        }

        auto* military_comp = GetMilitaryComponent(province_id);
        if (!military_comp) {
            CORE_LOG_ERROR("MilitarySystem", "Province military component not found");
            return;
        }

        if (military_comp->military_budget < cost) {
            CORE_LOG_WARN("MilitarySystem", "Insufficient budget for fortification construction");
            return;
        }

        ::core::ecs::EntityID province_handle(static_cast<uint64_t>(province_id), 1);
        auto fort_comp = entity_manager->GetComponent<FortificationComponent>(province_handle);

        if (!fort_comp) {
            CORE_LOG_ERROR("MilitarySystem", "Fortification component not found");
            return;
        }

        // Upgrade fortifications based on type
        // 0 = walls, 1 = towers, 2 = gates, 3 = citadel
        switch (fortification_type) {
            case 0: // Walls
                fort_comp->walls_level++;
                fort_comp->siege_resistance += config::FORTIFICATION_SIEGE_RESISTANCE_BONUS;
                break;
            case 1: // Towers
                fort_comp->towers_level++;
                fort_comp->artillery_effectiveness += config::FORTIFICATION_ARTILLERY_BONUS;
                break;
            case 2: // Gates
                fort_comp->gates_level++;
                fort_comp->structural_integrity = std::min(1.0, fort_comp->structural_integrity + config::FORTIFICATION_INTEGRITY_BONUS);
                break;
            case 3: // Citadel
                fort_comp->citadel_level++;
                fort_comp->garrison_capacity += config::FORTIFICATION_GARRISON_CAPACITY_BONUS;
                break;
            default:
                CORE_LOG_WARN("MilitarySystem", "Unknown fortification type");
                return;
        }

        military_comp->military_budget -= cost;

        CORE_LOG_INFO("MilitarySystem",
            "Constructed fortification type " + std::to_string(fortification_type) +
            " in province " + std::to_string(static_cast<int>(province_id)));
    }

    // ============================================================================
    // Stub Methods (To Be Implemented Later)
    // ============================================================================

    void MilitarySystem::DisbandUnit(game::types::EntityID province_id, size_t unit_index) {
        if (!m_initialized) {
            CORE_LOG_ERROR("MilitarySystem", "System not initialized");
            return;
        }

        auto* military_comp = GetMilitaryComponent(province_id);
        if (!military_comp) {
            CORE_LOG_ERROR("MilitarySystem", "Province military component not found");
            return;
        }

        std::lock_guard<std::mutex> lock(military_comp->garrison_mutex);
        if (unit_index >= military_comp->garrison_units.size()) {
            CORE_LOG_WARN("MilitarySystem", "Unit index out of range");
            return;
        }

        // Return a portion of recruitment cost (disbanding refund)
        double refund = military_comp->garrison_units[unit_index].recruitment_cost * config::DISBANDMENT_REFUND_RATIO;
        military_comp->military_budget += refund;

        // Remove the unit
        military_comp->garrison_units.erase(military_comp->garrison_units.begin() + unit_index);

        CORE_LOG_INFO("MilitarySystem",
            "Disbanded unit at index " + std::to_string(unit_index) +
            " from province " + std::to_string(static_cast<int>(province_id)));
    }

    void MilitarySystem::MergeUnits(game::types::EntityID province_id, size_t unit_a, size_t unit_b) {
        if (!m_initialized) {
            CORE_LOG_ERROR("MilitarySystem", "System not initialized");
            return;
        }

        auto* military_comp = GetMilitaryComponent(province_id);
        if (!military_comp) {
            CORE_LOG_ERROR("MilitarySystem", "Province military component not found");
            return;
        }

        std::lock_guard<std::mutex> lock(military_comp->garrison_mutex);
        if (unit_a >= military_comp->garrison_units.size() ||
            unit_b >= military_comp->garrison_units.size()) {
            CORE_LOG_WARN("MilitarySystem", "Unit index out of range");
            return;
        }

        auto& first_unit = military_comp->garrison_units[unit_a];
        auto& second_unit = military_comp->garrison_units[unit_b];

        // Can only merge units of the same type
        if (first_unit.type != second_unit.type) {
            CORE_LOG_WARN("MilitarySystem", "Cannot merge units of different types");
            return;
        }

        // Merge second into first
        uint32_t combined_strength = first_unit.current_strength + second_unit.current_strength;
        first_unit.current_strength = std::min(combined_strength, first_unit.max_strength);

        // Average experience and equipment quality
        first_unit.experience = (first_unit.experience + second_unit.experience) / 2.0;
        first_unit.equipment_quality = (first_unit.equipment_quality + second_unit.equipment_quality) / 2.0;

        // Remove the second unit
        military_comp->garrison_units.erase(military_comp->garrison_units.begin() + unit_b);

        CORE_LOG_INFO("MilitarySystem",
            "Merged units " + std::to_string(unit_a) + " and " + std::to_string(unit_b) +
            " in province " + std::to_string(static_cast<int>(province_id)));
    }

    void MilitarySystem::SplitUnit(game::types::EntityID province_id, size_t unit_index, uint32_t split_size) {
        if (!m_initialized) {
            CORE_LOG_ERROR("MilitarySystem", "System not initialized");
            return;
        }

        auto* military_comp = GetMilitaryComponent(province_id);
        if (!military_comp) {
            CORE_LOG_ERROR("MilitarySystem", "Province military component not found");
            return;
        }

        std::lock_guard<std::mutex> lock(military_comp->garrison_mutex);
        if (unit_index >= military_comp->garrison_units.size()) {
            CORE_LOG_WARN("MilitarySystem", "Unit index out of range");
            return;
        }

        auto& original_unit = military_comp->garrison_units[unit_index];

        if (split_size >= original_unit.current_strength) {
            CORE_LOG_WARN("MilitarySystem", "Split size must be less than current unit strength");
            return;
        }

        // Create new unit with split size
        MilitaryUnit new_unit = original_unit; // Copy all properties
        new_unit.current_strength = split_size;
        original_unit.current_strength -= split_size;

        // Add the new unit
        military_comp->garrison_units.push_back(new_unit);

        CORE_LOG_INFO("MilitarySystem",
            "Split unit " + std::to_string(unit_index) + " (size: " + std::to_string(split_size) +
            ") in province " + std::to_string(static_cast<int>(province_id)));
    }

    game::types::EntityID MilitarySystem::CreateArmy(game::types::EntityID home_province, const std::string& army_name) {
        if (!m_initialized) {
            CORE_LOG_ERROR("MilitarySystem", "System not initialized");
            return 0;
        }

        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            CORE_LOG_ERROR("MilitarySystem", "EntityManager not available");
            return 0;
        }

        // Create a new entity for the army
        auto army_entity = entity_manager->CreateEntity();

        // Add ArmyComponent
        auto army_comp = entity_manager->AddComponent<ArmyComponent>(army_entity, army_name);
        if (army_comp) {
            army_comp->home_province = home_province;
            army_comp->current_location = home_province;

            CORE_LOG_INFO("MilitarySystem",
                "Created army '" + army_name + "' (ID: " + std::to_string(army_entity.id) +
                ") at province " + std::to_string(static_cast<int>(home_province)));

            // Publish army creation event
            Json::Value army_event;
            army_event["event_type"] = "ArmyCreated";
            army_event["army_id"] = static_cast<int>(army_entity.id);
            army_event["army_name"] = army_name;
            army_event["home_province"] = static_cast<int>(home_province);
            // TODO: Update to use typed message instead of deprecated string topic API
            // m_message_bus.Publish("military.army_created", army_event);

            // Add to army registry
            {
                std::lock_guard<std::mutex> lock(m_armies_registry_mutex);
                m_all_armies.push_back(static_cast<game::types::EntityID>(army_entity.id));
            }

            return static_cast<game::types::EntityID>(army_entity.id);
        }

        CORE_LOG_ERROR("MilitarySystem", "Failed to create army component");
        return 0;
    }

    // ============================================================================
    // Internal Helper Methods
    // ============================================================================

    void MilitarySystem::InitializeUnitTemplates() {
        CORE_LOG_DEBUG("MilitarySystem", "Initializing unit templates from JSON");

        // Load unit definitions from JSON file
        const std::string units_file_path = "data/definitions/units.json";
        std::ifstream units_file(units_file_path);

        if (!units_file.is_open()) {
            CORE_LOG_ERROR("MilitarySystem", "Failed to open units definition file: " + units_file_path);
            CORE_LOG_WARN("MilitarySystem", "Falling back to hardcoded unit templates");
            InitializeUnitTemplatesFromHardcodedDefaults();
            return;
        }

        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errors;

        if (!Json::parseFromStream(builder, units_file, &root, &errors)) {
            CORE_LOG_ERROR("MilitarySystem", "Failed to parse units.json: " + errors);
            CORE_LOG_WARN("MilitarySystem", "Falling back to hardcoded unit templates");
            InitializeUnitTemplatesFromHardcodedDefaults();
            return;
        }

        units_file.close();

        // Parse units array
        if (!root.isMember("units") || !root["units"].isArray()) {
            CORE_LOG_ERROR("MilitarySystem", "Invalid units.json format: missing 'units' array");
            InitializeUnitTemplatesFromHardcodedDefaults();
            return;
        }

        const Json::Value& units_array = root["units"];
        int loaded_count = 0;

        for (const auto& unit_json : units_array) {
            if (!unit_json.isMember("type") || !unit_json["type"].isString()) {
                CORE_LOG_WARN("MilitarySystem", "Skipping unit definition without type");
                continue;
            }

            std::string type_str = unit_json["type"].asString();
            UnitType unit_type = StringToUnitType(type_str);

            if (unit_type == UnitType::COUNT) {
                CORE_LOG_WARN("MilitarySystem", "Unknown unit type: " + type_str);
                continue;
            }

            // Create unit template from JSON
            MilitaryUnit unit_template;
            unit_template.type = unit_type;

            // Load basic stats
            if (unit_json.isMember("max_strength"))
                unit_template.max_strength = unit_json["max_strength"].asUInt();
            if (unit_json.isMember("attack_strength"))
                unit_template.attack_strength = unit_json["attack_strength"].asDouble();
            if (unit_json.isMember("defense_strength"))
                unit_template.defense_strength = unit_json["defense_strength"].asDouble();
            if (unit_json.isMember("movement_speed"))
                unit_template.movement_speed = unit_json["movement_speed"].asDouble();
            if (unit_json.isMember("range"))
                unit_template.range = unit_json["range"].asDouble();
            if (unit_json.isMember("equipment_quality"))
                unit_template.equipment_quality = unit_json["equipment_quality"].asDouble();
            if (unit_json.isMember("training"))
                unit_template.training = unit_json["training"].asDouble();
            if (unit_json.isMember("recruitment_cost"))
                unit_template.recruitment_cost = unit_json["recruitment_cost"].asDouble();
            if (unit_json.isMember("monthly_maintenance"))
                unit_template.monthly_maintenance = unit_json["monthly_maintenance"].asDouble();

            // Set current strength to max by default
            unit_template.current_strength = unit_template.max_strength;

            // Store in templates map
            m_unit_templates[unit_type] = unit_template;
            loaded_count++;
        }

        CORE_LOG_INFO("MilitarySystem",
            "Unit templates loaded from JSON: " + std::to_string(loaded_count) + " unit types");
    }

    void MilitarySystem::InitializeUnitTemplatesFromHardcodedDefaults() {
        CORE_LOG_DEBUG("MilitarySystem", "Initializing hardcoded unit templates (fallback)");

        // Fallback hardcoded templates for essential units
        MilitaryUnit levies_template;
        levies_template.type = UnitType::LEVIES;
        levies_template.max_strength = 1000;
        levies_template.current_strength = 1000;
        levies_template.attack_strength = 5.0;
        levies_template.defense_strength = 4.0;
        levies_template.recruitment_cost = 50.0;
        levies_template.monthly_maintenance = 2.0;
        m_unit_templates[UnitType::LEVIES] = levies_template;

        MilitaryUnit spearmen_template;
        spearmen_template.type = UnitType::SPEARMEN;
        spearmen_template.max_strength = 1000;
        spearmen_template.current_strength = 1000;
        spearmen_template.attack_strength = 10.0;
        spearmen_template.defense_strength = 12.0;
        spearmen_template.recruitment_cost = 150.0;
        spearmen_template.monthly_maintenance = 8.0;
        m_unit_templates[UnitType::SPEARMEN] = spearmen_template;

        CORE_LOG_INFO("MilitarySystem", "Loaded " + std::to_string(m_unit_templates.size()) + " hardcoded unit templates");
    }

    UnitType MilitarySystem::StringToUnitType(const std::string& type_str) const {
        // Convert string to UnitType enum
        if (type_str == "LEVIES") return UnitType::LEVIES;
        if (type_str == "SPEARMEN") return UnitType::SPEARMEN;
        if (type_str == "SWORDSMEN") return UnitType::SWORDSMEN;
        if (type_str == "CROSSBOWMEN") return UnitType::CROSSBOWMEN;
        if (type_str == "LONGBOWMEN") return UnitType::LONGBOWMEN;
        if (type_str == "MEN_AT_ARMS") return UnitType::MEN_AT_ARMS;
        if (type_str == "PIKEMEN") return UnitType::PIKEMEN;
        if (type_str == "ARQUEBUSIERS") return UnitType::ARQUEBUSIERS;
        if (type_str == "MUSKETEERS") return UnitType::MUSKETEERS;
        if (type_str == "LIGHT_CAVALRY") return UnitType::LIGHT_CAVALRY;
        if (type_str == "HEAVY_CAVALRY") return UnitType::HEAVY_CAVALRY;
        if (type_str == "MOUNTED_ARCHERS") return UnitType::MOUNTED_ARCHERS;
        if (type_str == "DRAGOONS") return UnitType::DRAGOONS;
        if (type_str == "CATAPULTS") return UnitType::CATAPULTS;
        if (type_str == "TREBUCHETS") return UnitType::TREBUCHETS;
        if (type_str == "CANNONS") return UnitType::CANNONS;
        if (type_str == "SIEGE_TOWERS") return UnitType::SIEGE_TOWERS;
        if (type_str == "GALLEYS") return UnitType::GALLEYS;
        if (type_str == "COGS") return UnitType::COGS;
        if (type_str == "CARRACKS") return UnitType::CARRACKS;
        if (type_str == "GALLEONS") return UnitType::GALLEONS;
        if (type_str == "WAR_GALLEONS") return UnitType::WAR_GALLEONS;
        if (type_str == "SHIPS_OF_THE_LINE") return UnitType::SHIPS_OF_THE_LINE;

        return UnitType::COUNT; // Invalid/unknown type
    }

    void MilitarySystem::InitializeTechnologyUnlocks() {
        CORE_LOG_DEBUG("MilitarySystem", "Initializing technology unlocks");

        // For now, use hardcoded technology-to-unit mappings
        // In future, load from config file: data/military/technology_unlocks.json
        // Expected format:
        // {
        //   "technologies": {
        //     "BRONZE_WORKING": { "unlocks": ["SPEARMEN", "BRONZE_SWORDS"] },
        //     "IRON_WORKING": { "unlocks": ["HEAVY_INFANTRY", "IRON_SWORDS"] },
        //     "STIRRUP": { "unlocks": ["HEAVY_CAVALRY"] },
        //     "GUNPOWDER": { "unlocks": ["ARQUEBUSIERS", "CANNONS"] }
        //   }
        // }

        // Technology unlocks would be stored in a map:
        // std::unordered_map<std::string, std::vector<UnitType>> m_technology_unlocks;

        // Example hardcoded mappings (for future implementation):
        // m_technology_unlocks["BRONZE_WORKING"] = { UnitType::SPEARMEN };
        // m_technology_unlocks["IRON_WORKING"] = { UnitType::HEAVY_INFANTRY };
        // m_technology_unlocks["STIRRUP"] = { UnitType::HEAVY_CAVALRY };
        // m_technology_unlocks["GUNPOWDER"] = { UnitType::ARQUEBUSIERS };
        // m_technology_unlocks["NAVAL_ARTILLERY"] = { UnitType::GALLEONS };

        CORE_LOG_INFO("MilitarySystem", "Technology unlocks initialized (using defaults)");
    }

    void MilitarySystem::SubscribeToEvents() {
        CORE_LOG_DEBUG("MilitarySystem", "Subscribing to military-relevant events");

        // Subscribe to population changes (affects recruitment pools)
        m_message_bus.Subscribe("population.class_population_changed",
            [this](const Json::Value& event) {
                // Population changes affect available recruits
                CORE_LOG_DEBUG("MilitarySystem", "Population change event received");
            });

        // Subscribe to economic events (affects military budgets)
        m_message_bus.Subscribe("economic.treasury_changed",
            [this](const Json::Value& event) {
                // Treasury changes may affect military spending capacity
                CORE_LOG_DEBUG("MilitarySystem", "Treasury change event received");
            });

        // Subscribe to technology discoveries (unlocks new units/capabilities)
        m_message_bus.Subscribe("technology.discovered",
            [this](const Json::Value& event) {
                // New technologies may unlock new unit types or upgrades
                CORE_LOG_DEBUG("MilitarySystem", "Technology discovery event received");
            });

        // Subscribe to building construction (affects military infrastructure)
        m_message_bus.Subscribe("construction.building_completed",
            [this](const Json::Value& event) {
                // Completed buildings may enhance military capabilities
                CORE_LOG_DEBUG("MilitarySystem", "Building completion event received");
            });

        // Subscribe to faction events (affects unit loyalty and morale)
        m_message_bus.Subscribe("faction.satisfaction_changed",
            [this](const Json::Value& event) {
                // Faction satisfaction affects military morale and loyalty
                CORE_LOG_DEBUG("MilitarySystem", "Faction satisfaction change event received");
            });

        CORE_LOG_INFO("MilitarySystem", "Successfully subscribed to 5 event channels");
    }

    MilitaryUnit MilitarySystem::CreateUnitTemplate(UnitType unit_type) const {
        // Try to find the unit template in our loaded templates
        auto it = m_unit_templates.find(unit_type);
        if (it != m_unit_templates.end()) {
            // Return a copy of the template
            return it->second;
        }

        // Fallback: Create a basic unit with default values if template not found
        CORE_LOG_WARN("MilitarySystem",
            "Unit template not found for type " + std::to_string(static_cast<int>(unit_type)) +
            ", using fallback defaults");

        MilitaryUnit unit;
        unit.type = unit_type;
        unit.max_strength = 500;
        unit.current_strength = 500;
        unit.attack_strength = 5.0;
        unit.defense_strength = 5.0;
        unit.recruitment_cost = 100.0;
        unit.monthly_maintenance = 10.0;
        unit.equipment_quality = 0.5;
        unit.training = 0.5;
        unit.morale = MoraleState::STEADY;

        return unit;
    }

    void MilitarySystem::PerformArmyRegistryCleanup() {
        std::lock_guard<std::mutex> lock(m_armies_registry_mutex);

        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return;
        }

        // Remove disbanded/deleted armies from registry
        size_t original_size = m_all_armies.size();

        m_all_armies.erase(
            std::remove_if(m_all_armies.begin(), m_all_armies.end(),
                [entity_manager](game::types::EntityID army_id) {
                    ::core::ecs::EntityID army_handle(static_cast<uint64_t>(army_id), 1);
                    auto army_comp = entity_manager->GetComponent<ArmyComponent>(army_handle);

                    // Remove if army doesn't exist or is not active
                    return !army_comp || !army_comp->is_active;
                }),
            m_all_armies.end()
        );

        size_t cleaned_count = original_size - m_all_armies.size();
        if (cleaned_count > 0) {
            CORE_LOG_DEBUG("MilitarySystem",
                "Army registry cleanup: removed " + std::to_string(cleaned_count) +
                " disbanded armies, " + std::to_string(m_all_armies.size()) + " remain");
        }
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
        
        CORE_LOG_INFO("MilitarySystem", "Serialization completed for version " + std::to_string(version));
        return root;
    }

    bool MilitarySystem::Deserialize(const Json::Value& data, int version) {
        if (!data.isObject()) {
            CORE_LOG_ERROR("MilitarySystem", "Invalid serialization data - not an object");
            return false;
        }
        
        if (!data.isMember("version") || !data.isMember("system_name")) {
            CORE_LOG_ERROR("MilitarySystem", "Missing required serialization fields");
            return false;
        }
        
        if (data["system_name"].asString() != "MilitarySystem") {
            CORE_LOG_ERROR("MilitarySystem", "Serialization data is for wrong system type");
            return false;
        }
        
        // Restore system state
        if (data.isMember("initialized")) {
            m_initialized = data["initialized"].asBool();
        }
        
        // Note: Detailed component data is deserialized by the ECS system itself
        // This method only restores system-specific configuration and state
        
        CORE_LOG_INFO("MilitarySystem", "Deserialization completed for version " + std::to_string(version));
        return true;
    }

    // ============================================================================
    // Threading Strategy
    // ============================================================================

    ::core::threading::ThreadingStrategy MilitarySystem::GetThreadingStrategy() const {
        return ::core::threading::ThreadingStrategy::THREAD_POOL;
    }

} // namespace game::military} // namespace game::military