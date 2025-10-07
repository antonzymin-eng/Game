// ============================================================================
// Date/Time Created: Tuesday, September 16, 2025 - 11:45 AM PST
// Intended Folder Location: src/game/GameSystemsIntegration.cpp
// GameSystemsIntegration.cpp - Fixed Threading, Validation, and Type Safety
// ============================================================================

#include "GameSystemsIntegration.h"
#include "game/province/EnhancedProvinceSystem.h"
#include "game/ai/GameAI.h"
#include "game/population/PopulationSystem.h"
#include "core/ECS/EntityManager.h"
#include "core/ECS/MessageBus.h"
#include "core/ECS/ComponentAccessManager.h"
#include "core/Threading/ThreadedSystemManager.h"
#include <iostream>
#include <memory>
#include <algorithm>
#include <cassert>

namespace game {

    // ============================================================================
    // GameSystemsManager Implementation - FIXED
    // ============================================================================

    GameSystemsManager::GameSystemsManager() {
        std::cout << "Initializing Game Systems Manager..." << std::endl;
    }

    GameSystemsManager::~GameSystemsManager() {
        Shutdown();
    }

    bool GameSystemsManager::Initialize() {
        std::cout << "Initializing enhanced game systems..." << std::endl;

        try {
            // Initialize ECS foundation
            if (!InitializeECSFoundation()) {
                std::cerr << "Failed to initialize ECS foundation" << std::endl;
                return false;
            }

            // Initialize game systems
            if (!InitializeGameSystems()) {
                std::cerr << "Failed to initialize game systems" << std::endl;
                return false;
            }

            // Create test provinces
            CreateTestProvinces();

            // Start system threads
            if (!StartSystemThreads()) {
                std::cerr << "Failed to start system threads" << std::endl;
                return false;
            }

            m_initialized = true;
            std::cout << "Game systems initialized successfully!" << std::endl;
            return true;

        }
        catch (const std::exception& e) {
            std::cerr << "Exception during initialization: " << e.what() << std::endl;
            return false;
        }
    }

    void GameSystemsManager::Update(float delta_time) {
        if (!m_initialized) {
            return;
        }

        m_system_update_in_progress = true;
        m_frame_timer += delta_time;

        // Update systems at 60 FPS
        if (m_frame_timer >= 1.0f / 60.0f) {
            // Update threaded system manager
            if (m_system_manager) {
                m_system_manager->Update(m_frame_timer);
            }

            // FIXED: Ensure worker threads complete before AI/UI access
            FlushSystemUpdates();

            // Update AI systems (run on main thread for now)
            if (m_game_ai) {
                m_game_ai->Update(m_frame_timer);
            }

            // Process cross-thread messages
            if (m_message_bus) {
                m_message_bus->ProcessQueuedMessages();
            }

            m_frame_timer = 0.0f;
            m_frame_count++;

            // Log status every 5 seconds
            if (m_frame_count % 300 == 0) {
                LogSystemStatus();
            }
        }

        m_system_update_in_progress = false;
    }

    // FIXED: Add synchronization barrier
    void GameSystemsManager::FlushSystemUpdates() noexcept {
        if (m_system_manager) {
            // Wait for all worker threads to complete current frame
            m_system_manager->WaitForFrameCompletion();
            
            // Drain message bus to ensure all cross-system communication is processed
            if (m_message_bus) {
                m_message_bus->DrainAllMessages();
            }
        }
    }

    bool GameSystemsManager::IsSystemUpdateComplete() const noexcept {
        return !m_system_update_in_progress.load() && 
               (m_system_manager ? m_system_manager->IsFrameComplete() : true);
    }

    void GameSystemsManager::Shutdown() {
        std::cout << "Shutting down game systems..." << std::endl;

        if (m_game_ai) {
            m_game_ai->Shutdown();
            m_game_ai.reset();
        }

        if (m_system_manager) {
            m_system_manager->Shutdown();
            m_system_manager.reset();
        }

        if (m_province_system) {
            m_province_system->Shutdown();
            m_province_system.reset();
        }

        if (m_population_system) {
            m_population_system->Shutdown();
            m_population_system.reset();
        }

        m_component_access_manager.reset();
        m_message_bus.reset();
        m_entity_manager.reset();

        m_initialized = false;
        std::cout << "Game systems shutdown complete" << std::endl;
    }

    // [InitializeECSFoundation and InitializeGameSystems remain the same]
    bool GameSystemsManager::InitializeECSFoundation() {
        std::cout << "Initializing ECS foundation..." << std::endl;

        m_entity_manager = std::make_unique<core::ecs::EntityManager>();
        if (!m_entity_manager) {
            std::cerr << "Failed to create EntityManager" << std::endl;
            return false;
        }

        m_message_bus = std::make_unique<core::ecs::MessageBus>();
        if (!m_message_bus) {
            std::cerr << "Failed to create MessageBus" << std::endl;
            return false;
        }

        m_component_access_manager = std::make_unique<core::ecs::ComponentAccessManager>(*m_entity_manager);
        if (!m_component_access_manager) {
            std::cerr << "Failed to create ComponentAccessManager" << std::endl;
            return false;
        }

        m_system_manager = std::make_unique<core::threading::ThreadedSystemManager>(*m_component_access_manager, *m_message_bus);
        if (!m_system_manager) {
            std::cerr << "Failed to create ThreadedSystemManager" << std::endl;
            return false;
        }

        std::cout << "ECS foundation initialized successfully" << std::endl;
        return true;
    }

    bool GameSystemsManager::InitializeGameSystems() {
        std::cout << "Initializing game systems..." << std::endl;

        m_province_system = std::make_unique<province::EnhancedProvinceSystem>(*m_component_access_manager, *m_message_bus);
        if (!m_province_system) {
            std::cerr << "Failed to create EnhancedProvinceSystem" << std::endl;
            return false;
        }

        m_province_system->Initialize();
        m_system_manager->AddSystem("EnhancedProvinceSystem", m_province_system.get(), core::threading::ThreadingStrategy::MAIN_THREAD);

        m_game_ai = std::make_unique<GameAI>(*m_component_access_manager, *m_message_bus);
        if (!m_game_ai) {
            std::cerr << "Failed to create GameAI" << std::endl;
            return false;
        }

        m_game_ai->Initialize();
        m_game_ai->SetPersonality(AIPersonality(AIPersonality::Trait::BALANCED));

        AIConfig ai_config;
        ai_config.difficulty = AIConfig::Difficulty::NORMAL;
        ai_config.decision_interval = 3.0f;
        ai_config.debug_logging = true;
        m_game_ai->SetConfig(ai_config);

        std::cout << "Game systems initialized successfully" << std::endl;
        return true;
    }

    // [CreateTestProvinces, StartSystemThreads, LogSystemStatus remain the same]
    void GameSystemsManager::CreateTestProvinces() {
        std::cout << "Creating test provinces..." << std::endl;

        if (!m_province_system) {
            std::cerr << "Province system not available for test creation" << std::endl;
            return;
        }

        // Create test province 1 - Agricultural focus
        province::ProvinceComponent agricultural_province("Farmlands");
        agricultural_province.fertility = 0.8;
        agricultural_province.mineral_richness = 0.2;
        agricultural_province.trade_access = 0.4;
        agricultural_province.river_access = true;
        agricultural_province.infrastructure_quality = 0.5;

        types::EntityID province1 = m_province_system->CreateProvince("Farmlands", agricultural_province);
        m_province_system->ConstructBuilding(province1, province::ProductionBuilding::FARM);
        m_province_system->ConstructBuilding(province1, province::ProductionBuilding::FARM);
        m_province_system->ConstructBuilding(province1, province::ProductionBuilding::MILL);

        m_test_provinces.push_back(province1);

        std::cout << "Created " << m_test_provinces.size() << " test provinces" << std::endl;
    }

    bool GameSystemsManager::StartSystemThreads() {
        std::cout << "Starting system threads..." << std::endl;

        if (!m_system_manager) {
            std::cerr << "System manager not available" << std::endl;
            return false;
        }

        m_system_manager->Initialize();
        std::cout << "System threads started successfully" << std::endl;
        return true;
    }

    void GameSystemsManager::LogSystemStatus() {
        if (!m_initialized) {
            return;
        }

        std::cout << "\n=== Game Systems Status (Frame " << m_frame_count << ") ===" << std::endl;

        if (m_province_system) {
            auto provinces = m_province_system->GetAllProvinces();
            std::cout << "Provinces: " << provinces.size() << std::endl;

            for (auto province_id : provinces) {
                auto* province_data = m_province_system->GetProvinceData(province_id);
                if (province_data) {
                    std::cout << "  " << province_data->name
                        << " (Buildings: " << province_data->total_building_levels
                        << ", Infrastructure: " << static_cast<int>(province_data->infrastructure_quality * 100) << "%)"
                        << std::endl;
                }
            }
        }

        if (m_game_ai) {
            std::cout << "AI Goals: " << m_game_ai->GetActiveGoals().size() << std::endl;
            m_game_ai->LogAIState();
        }

        std::cout << "========================\n" << std::endl;
    }

    // ============================================================================
    // UI Integration Helper Methods - FIXED with proper validation
    // ============================================================================

    std::vector<ProvinceInfo> GameSystemsManager::GetProvinceInformation() const noexcept {
        std::vector<ProvinceInfo> province_info;

        if (!m_province_system || !IsSystemUpdateComplete()) {
            return province_info;
        }

        try {
            auto provinces = m_province_system->GetAllProvinces();

            for (auto province_id : provinces) {
                auto* province_comp = m_province_system->GetProvinceData(province_id);

                if (province_comp) {
                    ProvinceInfo info;
                    info.entity_id = province_id;
                    info.name = province_comp->name;
                    info.settlement_type = ConvertFromSettlementEnum(province_comp->settlement_type);
                    info.total_buildings = province_comp->total_building_levels;
                    info.infrastructure_quality = province_comp->infrastructure_quality;
                    info.fertility = province_comp->fertility;
                    info.mineral_richness = province_comp->mineral_richness;
                    info.trade_access = province_comp->trade_access;
                    info.coastal = province_comp->coastal;
                    info.river_access = province_comp->river_access;

                    // Get economic data
                    auto econ_read = m_component_access_manager->GetReadAccess<province::EconomicComponent>("UI_ProvinceInfo");
                    auto* econ_comp = econ_read.GetComponent(province_id);
                    if (econ_comp) {
                        info.treasury = econ_comp->treasury;
                        info.monthly_income = econ_comp->monthly_income;
                        info.monthly_expenses = econ_comp->monthly_expenses;
                        info.tax_rate = econ_comp->tax_rate;
                        info.prosperity = econ_comp->prosperity;
                        info.unemployment = econ_comp->unemployment;
                    }

                    // Get population data
                    auto pop_read = m_component_access_manager->GetReadAccess<population::PopulationComponent>("UI_ProvinceInfo");
                    auto* pop_comp = pop_read.GetComponent(province_id);
                    if (pop_comp) {
                        info.total_population = pop_comp->total_population.value;
                        info.happiness = pop_comp->overall_metrics.happiness;
                        info.stability = pop_comp->overall_metrics.stability;
                        info.growth_rate = pop_comp->growth_rate;
                    }

                    // Get resource production data
                    auto prod_read = m_component_access_manager->GetReadAccess<province::ProductionComponent>("UI_ProvinceInfo");
                    auto* prod_comp = prod_read.GetComponent(province_id);
                    if (prod_comp) {
                        info.grain_production = m_province_system->GetResourceProduction(province_id, province::ResourceType::GRAIN);
                        info.timber_production = m_province_system->GetResourceProduction(province_id, province::ResourceType::TIMBER);
                        info.iron_production = m_province_system->GetResourceProduction(province_id, province::ResourceType::IRON_ORE);
                        info.craft_production = m_province_system->GetResourceProduction(province_id, province::ResourceType::IRON_TOOLS);
                    }

                    province_info.push_back(info);
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error getting province information: " << e.what() << std::endl;
        }

        return province_info;
    }

    bool GameSystemsManager::ConstructBuilding(types::EntityID province_id, UIBuildingType building_type) noexcept {
        if (!m_province_system) {
            return false;
        }

        try {
            auto building_enum = ConvertToBuildingEnum(building_type);
            return m_province_system->ConstructBuilding(province_id, building_enum);
        } catch (const std::exception& e) {
            std::cerr << "Error constructing building: " << e.what() << std::endl;
            return false;
        }
    }

    // FIXED: Add proper validation and event publishing
    bool GameSystemsManager::AdjustTaxRate(types::EntityID province_id, double new_tax_rate) noexcept {
        if (!m_component_access_manager) {
            return false;
        }

        // FIXED: Validate tax rate bounds
        double clamped_rate = std::clamp(new_tax_rate, 0.01, 0.5); // 1% to 50%
        
        if (clamped_rate != new_tax_rate) {
            std::cout << "Tax rate clamped from " << new_tax_rate << " to " << clamped_rate << std::endl;
        }

        try {
            auto econ_write = m_component_access_manager->GetWriteAccess<province::EconomicComponent>("UI_TaxAdjustment");
            auto* econ_comp = econ_write.GetComponent(province_id);

            if (econ_comp) {
                double old_rate = econ_comp->tax_rate;
                econ_comp->tax_rate = clamped_rate;

                // FIXED: Publish tax rate change event
                if (m_message_bus) {
                    province::messages::TaxRateChanged tax_event;
                    tax_event.province = province_id;
                    tax_event.old_rate = old_rate;
                    tax_event.new_rate = clamped_rate;
                    m_message_bus->PublishMessage(tax_event);
                }

                std::cout << "Adjusted tax rate to " << clamped_rate * 100 << "% for province " << province_id << std::endl;
                return true;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error adjusting tax rate: " << e.what() << std::endl;
        }

        return false;
    }

    std::vector<AIDecisionInfo> GameSystemsManager::GetAIDecisions() const noexcept {
        std::vector<AIDecisionInfo> decisions_info;

        if (!m_game_ai || !IsSystemUpdateComplete()) {
            return decisions_info;
        }

        try {
            auto decisions = m_game_ai->GenerateDecisions();

            for (const auto& decision : decisions) {
                AIDecisionInfo info;
                info.target_province = decision.GetTargetProvince();
                info.decision_type = ConvertFromDecisionEnum(decision);
                info.priority = decision.GetPriorityScore();
                info.description = decision.GetDescription();
                info.can_execute = decision.CanExecute(*m_component_access_manager);

                decisions_info.push_back(info);
            }
        } catch (const std::exception& e) {
            std::cerr << "Error getting AI decisions: " << e.what() << std::endl;
        }

        return decisions_info;
    }

    void GameSystemsManager::SetAIPersonality(UIPersonalityType personality_type) noexcept {
        if (!m_game_ai) {
            return;
        }

        try {
            // FIXED: Proper type conversion
            AIPersonality::Trait trait;
            switch (personality_type) {
                case UIPersonalityType::ECONOMIC_FOCUSED: trait = AIPersonality::Trait::ECONOMIC_FOCUSED; break;
                case UIPersonalityType::MILITARY_FOCUSED: trait = AIPersonality::Trait::MILITARY_FOCUSED; break;
                case UIPersonalityType::POPULATION_FOCUSED: trait = AIPersonality::Trait::POPULATION_FOCUSED; break;
                case UIPersonalityType::BALANCED: trait = AIPersonality::Trait::BALANCED; break;
                case UIPersonalityType::OPPORTUNISTIC: trait = AIPersonality::Trait::OPPORTUNISTIC; break;
                case UIPersonalityType::CONSERVATIVE: trait = AIPersonality::Trait::CONSERVATIVE; break;
                case UIPersonalityType::AGGRESSIVE: trait = AIPersonality::Trait::AGGRESSIVE; break;
                case UIPersonalityType::DIPLOMATIC: trait = AIPersonality::Trait::DIPLOMATIC; break;
                default: trait = AIPersonality::Trait::BALANCED; break;
            }

            m_game_ai->SetPersonality(AIPersonality(trait));
            std::cout << "Set AI personality to " << static_cast<int>(personality_type) << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error setting AI personality: " << e.what() << std::endl;
        }
    }

    void GameSystemsManager::TriggerEconomicCrisis(types::EntityID province_id) noexcept {
        if (!m_message_bus) {
            return;
        }

        try {
            province::messages::EconomicCrisis crisis;
            crisis.province = province_id;
            crisis.crisis_type = "Test Economic Crisis";
            crisis.severity = 0.8;

            m_message_bus->PublishMessage(crisis);
            std::cout << "Triggered economic crisis in province " << province_id << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error triggering economic crisis: " << e.what() << std::endl;
        }
    }

    // ============================================================================
    // FIXED: Type conversion helpers
    // ============================================================================

    province::ProductionBuilding GameSystemsManager::ConvertToBuildingEnum(UIBuildingType ui_type) const {
        switch (ui_type) {
            case UIBuildingType::FARM: return province::ProductionBuilding::FARM;
            case UIBuildingType::LOGGING_CAMP: return province::ProductionBuilding::LOGGING_CAMP;
            case UIBuildingType::QUARRY: return province::ProductionBuilding::QUARRY;
            case UIBuildingType::MINE: return province::ProductionBuilding::MINE;
            case UIBuildingType::FISHING_DOCK: return province::ProductionBuilding::FISHING_DOCK;
            case UIBuildingType::PASTURE: return province::ProductionBuilding::PASTURE;
            case UIBuildingType::MILL: return province::ProductionBuilding::MILL;
            case UIBuildingType::SAWMILL: return province::ProductionBuilding::SAWMILL;
            case UIBuildingType::SMITHY: return province::ProductionBuilding::SMITHY;
            case UIBuildingType::WORKSHOP: return province::ProductionBuilding::WORKSHOP;
            case UIBuildingType::MARKET: return province::ProductionBuilding::MARKET;
            case UIBuildingType::GUILD_HALL: return province::ProductionBuilding::GUILD_HALL;
            case UIBuildingType::TRADING_POST: return province::ProductionBuilding::TRADING_POST;
            case UIBuildingType::WAREHOUSE: return province::ProductionBuilding::WAREHOUSE;
            default: return province::ProductionBuilding::FARM; // Safe default
        }
    }

    UISettlementType GameSystemsManager::ConvertFromSettlementEnum(province::SettlementType game_type) const {
        switch (game_type) {
            case province::SettlementType::HAMLET: return UISettlementType::HAMLET;
            case province::SettlementType::VILLAGE: return UISettlementType::VILLAGE;
            case province::SettlementType::TOWN: return UISettlementType::TOWN;
            case province::SettlementType::CITY: return UISettlementType::CITY;
            default: return UISettlementType::HAMLET; // Safe default
        }
    }

    UIDecisionType GameSystemsManager::ConvertFromDecisionEnum(const AIDecision& decision) const {
        // This would need to be implemented based on your AIDecision class
        // For now, return a default value
        return UIDecisionType::ECONOMIC;
    }

    // ============================================================================
    // C-Style Interface Functions - FIXED with proper synchronization
    // ============================================================================

    static std::unique_ptr<GameSystemsManager> g_game_systems_manager;

    bool InitializeGameSystems() {
        g_game_systems_manager = std::make_unique<GameSystemsManager>();
        return g_game_systems_manager->Initialize();
    }

    void UpdateGameSystems(float delta_time) {
        if (g_game_systems_manager) {
            g_game_systems_manager->Update(delta_time);
        }
    }

    void ShutdownGameSystems() {
        if (g_game_systems_manager) {
            g_game_systems_manager->Shutdown();
            g_game_systems_manager.reset();
        }
    }

    std::vector<ProvinceInfo> GetAllProvinceInfo() {
        if (g_game_systems_manager) {
            return g_game_systems_manager->GetProvinceInformation();
        }
        return {};
    }

    bool PlayerConstructBuilding(types::EntityID province_id, UIBuildingType building_type) {
        if (g_game_systems_manager) {
            return g_game_systems_manager->ConstructBuilding(province_id, building_type);
        }
        return false;
    }

    bool PlayerAdjustTaxRate(types::EntityID province_id, double new_tax_rate) {
        if (g_game_systems_manager) {
            return g_game_systems_manager->AdjustTaxRate(province_id, new_tax_rate);
        }
        return false;
    }

    std::vector<AIDecisionInfo> GetCurrentAIDecisions() {
        if (g_game_systems_manager) {
            return g_game_systems_manager->GetAIDecisions();
        }
        return {};
    }

    void SetAIPersonalityType(UIPersonalityType personality_type) {
        if (g_game_systems_manager) {
            g_game_systems_manager->SetAIPersonality(personality_type);
        }
    }

    // FIXED: Add synchronization functions
    void FlushAllSystemUpdates() {
        if (g_game_systems_manager) {
            g_game_systems_manager->FlushSystemUpdates();
        }
    }

    bool AreSystemUpdatesComplete() {
        if (g_game_systems_manager) {
            return g_game_systems_manager->IsSystemUpdateComplete();
        }
        return true;
    }

    void TestTriggerCrisis(types::EntityID province_id) {
        if (g_game_systems_manager) {
            g_game_systems_manager->TriggerEconomicCrisis(province_id);
        }
    }

} // namespace game
