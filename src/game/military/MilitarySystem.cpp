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

        // Monthly processing (30 seconds = 1 month in-game)
        if (m_monthly_timer >= 30.0f) {
            CORE_LOG_INFO("MilitarySystem", "Processing monthly military updates");
            m_monthly_timer = 0.0f;
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
            // Initialize military data
            military_component->military_budget = 1000.0; // Default budget
            military_component->recruitment_capacity = 100; // Default capacity
            
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
        if (military_comp->military_budget >= (quantity * 50.0)) { // 50 gold per unit
            MilitaryUnit new_unit;
            new_unit.type = unit_type;
            new_unit.current_strength = quantity;
            new_unit.max_strength = quantity;
            new_unit.recruitment_cost = quantity * 50.0;

            std::lock_guard<std::mutex> lock(military_comp->garrison_mutex);
            military_comp->garrison_units.push_back(new_unit);
            military_comp->military_budget -= new_unit.recruitment_cost;
            
            CORE_LOG_INFO("MilitarySystem", "Recruited " + std::to_string(quantity) + " units for province " + std::to_string(static_cast<int>(province_id)));
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
        // TODO: Implement when entity iteration API is available
        return {};
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

        // TODO: Publish battle result event
        // m_message_bus.Publish("BattleResolved",
        //     "Battle at location " + std::to_string(static_cast<int>(combat_comp->location)) +
        //     " resolved: " + outcome_str);
    }

    void MilitarySystem::ApplyCasualties(ArmyComponent& army, uint32_t total_casualties) {
        if (total_casualties == 0) return;

        std::lock_guard<std::mutex> lock(army.units_mutex);
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

        // Recalculate army strength (mutex already locked)
        army.RecalculateStrength();
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
            fort_comp->structural_integrity = std::max(0.0, fort_comp->structural_integrity - 0.3);
            CORE_LOG_INFO("MilitarySystem", "Siege successful - fortifications breached");
        } else {
            CORE_LOG_INFO("MilitarySystem", "Siege failed - defenders held");
        }
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
        double improvement = investment / 1000.0; // $1000 = +0.1 training
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
        double improvement = investment / 500.0; // $500 = +0.1 quality
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
                fort_comp->siege_resistance += 0.1;
                break;
            case 1: // Towers
                fort_comp->towers_level++;
                fort_comp->artillery_effectiveness += 0.1;
                break;
            case 2: // Gates
                fort_comp->gates_level++;
                fort_comp->structural_integrity = std::min(1.0, fort_comp->structural_integrity + 0.05);
                break;
            case 3: // Citadel
                fort_comp->citadel_level++;
                fort_comp->garrison_capacity += 100;
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
        double refund = military_comp->garrison_units[unit_index].recruitment_cost * 0.2;
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

            return static_cast<game::types::EntityID>(army_entity.id);
        }

        CORE_LOG_ERROR("MilitarySystem", "Failed to create army component");
        return 0;
    }

    // ============================================================================
    // Internal Helper Methods
    // ============================================================================

    void MilitarySystem::InitializeUnitTemplates() {
        CORE_LOG_DEBUG("MilitarySystem", "Initializing unit templates");
        // TODO: Load from configuration
    }

    void MilitarySystem::InitializeTechnologyUnlocks() {
        CORE_LOG_DEBUG("MilitarySystem", "Initializing technology unlocks");
        // TODO: Load from configuration
    }

    void MilitarySystem::SubscribeToEvents() {
        CORE_LOG_DEBUG("MilitarySystem", "Subscribing to events");
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