// ============================================================================
// Date/Time Created: September 27, 2025 - 4:15 PM PST
// Intended Folder Location: src/game/EconomicSystem.cpp
// FIXED: Externalized hardcoded values to GameConfig
// ============================================================================

#include "game/economy/EconomicSystem.h"
#include "game/administration/AdministrativeSystem.h"
#include "game/config/GameConfig.h"
#include "core/logging/Logger.h"
#include "game/economy/EconomicComponents.h"
#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace game {

    static bool starts_with(const std::string& str, const std::string& prefix) {
        return str.size() >= prefix.size() &&
            str.compare(0, prefix.size(), prefix) == 0;
    }

    void EconomicSystem::Initialize(AdministrativeSystem* administrative_system) {
        if (m_initialized) {
            ::core::logging::LogWarning("EconomicSystem", "System already initialized");
            return;
        }

        ::core::logging::LogInfo("EconomicSystem", "Initializing Economic System");
        
        admin_system = administrative_system;
        m_initialized = true;

        ::core::logging::LogInfo("EconomicSystem", "Economic System initialization complete");
    }

    void EconomicSystem::Shutdown() {
        if (!m_initialized) {
            return;
        }

        ::core::logging::LogInfo("EconomicSystem", "Shutting down Economic System");
        m_initialized = false;
    }

    void EconomicSystem::CreateEconomicComponents(game::types::EntityID entity_id) {
        if (!m_initialized) {
            ::core::logging::LogWarning("EconomicSystem", "System not initialized, cannot create components");
            return;
        }

        ::core::logging::LogInfo("EconomicSystem", 
            "Creating economic components for entity " + std::to_string(static_cast<int>(entity_id)));

        // Get EntityManager from ComponentAccessManager
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            ::core::logging::LogError("EconomicSystem", "EntityManager not available");
            return;
        }

        // Create EntityID handle
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);

        // Create main economic component
        auto economic_component = entity_manager->AddComponent<economy::EconomicComponent>(entity_handle);
        if (economic_component) {
            // Load configuration values
            auto& config = game::config::GameConfig::Instance();
            economic_component->treasury = config.GetInt("economy.starting_treasury", 1000);
            economic_component->tax_rate = config.GetFloat("economy.default_tax_rate", 0.1f);
            economic_component->tax_collection_efficiency = config.GetFloat("economy.tax_collection_efficiency", 0.8f);
            economic_component->infrastructure_quality = config.GetFloat("economy.base_infrastructure", 0.5f);
            
            ::core::logging::LogInfo("EconomicSystem", "Created EconomicComponent");
        }

        // Create trade component
        auto trade_component = entity_manager->AddComponent<economy::TradeComponent>(entity_handle);
        if (trade_component) {
            trade_component->trade_node_efficiency = 1.0f;
            trade_component->piracy_risk = 0.1f;
            trade_component->diplomatic_trade_modifier = 1.0f;
            trade_component->technology_trade_modifier = 1.0f;
            
            ::core::logging::LogInfo("EconomicSystem", "Created TradeComponent");
        }

        // Create treasury component
        auto treasury_component = entity_manager->AddComponent<economy::TreasuryComponent>(entity_handle);
        if (treasury_component) {
            auto& config = game::config::GameConfig::Instance();
            treasury_component->gold_reserves = config.GetInt("economy.starting_treasury", 1000);
            treasury_component->credit_rating = 0.8f;
            treasury_component->max_borrowing_capacity = treasury_component->gold_reserves * 5;
            
            ::core::logging::LogInfo("EconomicSystem", "Created TreasuryComponent");
        }

        // Create economic events component
        auto events_component = entity_manager->AddComponent<economy::EconomicEventsComponent>(entity_handle);
        if (events_component) {
            events_component->event_frequency_modifier = 1.0f;
            events_component->months_since_last_event = 0;
            events_component->max_history_size = 50;
            
            ::core::logging::LogInfo("EconomicSystem", "Created EconomicEventsComponent");
        }

        // Create market component
        auto market_component = entity_manager->AddComponent<economy::MarketComponent>(entity_handle);
        if (market_component) {
            market_component->market_size = 1.0f;
            market_component->market_sophistication = 0.5f;
            market_component->market_stability = 100;
            
            ::core::logging::LogInfo("EconomicSystem", "Created MarketComponent");
        }

        ::core::logging::LogInfo("EconomicSystem", "Successfully created all economic components");
    }

    void EconomicSystem::ProcessMonthlyUpdate(game::types::EntityID entity_id) {
        if (!m_initialized) {
            ::core::logging::LogWarning("EconomicSystem", "System not initialized, skipping update");
            return;
        }

        ::core::logging::LogDebug("EconomicSystem", 
            "Processing monthly update for entity " + std::to_string(static_cast<int>(entity_id)));

        // Get EntityManager from ComponentAccessManager
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            ::core::logging::LogError("EconomicSystem", "EntityManager not available");
            return;
        }

        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);

        // Get economic component
        auto economic_component = entity_manager->GetComponent<economy::EconomicComponent>(entity_handle);
        if (!economic_component) {
            ::core::logging::LogWarning("EconomicSystem", 
                "No EconomicComponent found for entity " + std::to_string(static_cast<int>(entity_id)));
            return;
        }

        // Process different aspects of the economy
        ProcessEntityEconomy(entity_id);
        ProcessTradeRoutes(entity_id);
        ProcessRandomEvents(entity_id);
        CalculateMonthlyTotals(entity_id);

        ::core::logging::LogDebug("EconomicSystem", "Monthly update complete");
    }

    // Legacy method for backward compatibility
    void EconomicSystem::processMonthlyUpdate(std::vector<Province>& provinces) {
        ::core::logging::LogWarning("EconomicSystem", "Using deprecated province-based update method");
        
        // For now, process first province as entity 1
        if (!provinces.empty()) {
            ProcessMonthlyUpdate(1);
        }
    }

    void EconomicSystem::AddTradeRoute(game::types::EntityID from_entity, game::types::EntityID to_entity, 
                                      float efficiency, int base_value) {
        if (!m_initialized) {
            ::core::logging::LogWarning("EconomicSystem", "System not initialized");
            return;
        }

        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            ::core::logging::LogError("EconomicSystem", "EntityManager not available");
            return;
        }

        ::core::ecs::EntityID from_handle(static_cast<uint64_t>(from_entity), 1);
        auto trade_component = entity_manager->GetComponent<economy::TradeComponent>(from_handle);
        
        if (!trade_component) {
            ::core::logging::LogWarning("EconomicSystem", "No TradeComponent found for entity");
            return;
        }

        // Check if route already exists
        auto& routes = trade_component->outgoing_routes;
        auto it = std::find_if(routes.begin(), routes.end(),
            [to_entity](const economy::TradeRoute& route) {
                return route.to_province == to_entity;
            });

        if (it == routes.end()) {
            routes.emplace_back(from_entity, to_entity, efficiency, base_value);
            ::core::logging::LogInfo("EconomicSystem", 
                "Added trade route from " + std::to_string(static_cast<int>(from_entity)) + 
                " to " + std::to_string(static_cast<int>(to_entity)));
        }
    }

    void EconomicSystem::RemoveTradeRoute(game::types::EntityID from_entity, game::types::EntityID to_entity) {
        if (!m_initialized) {
            ::core::logging::LogWarning("EconomicSystem", "System not initialized");
            return;
        }

        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            ::core::logging::LogError("EconomicSystem", "EntityManager not available");
            return;
        }

        ::core::ecs::EntityID from_handle(static_cast<uint64_t>(from_entity), 1);
        auto trade_component = entity_manager->GetComponent<economy::TradeComponent>(from_handle);
        
        if (!trade_component) {
            ::core::logging::LogWarning("EconomicSystem", "No TradeComponent found for entity");
            return;
        }

        auto& routes = trade_component->outgoing_routes;
        routes.erase(
            std::remove_if(routes.begin(), routes.end(),
                [to_entity](const economy::TradeRoute& route) {
                    return route.to_province == to_entity;
                }),
            routes.end());

        ::core::logging::LogInfo("EconomicSystem", 
            "Removed trade route from " + std::to_string(static_cast<int>(from_entity)) + 
            " to " + std::to_string(static_cast<int>(to_entity)));
    }

    std::vector<economy::TradeRoute> EconomicSystem::GetTradeRoutesForEntity(game::types::EntityID entity_id) const {
        if (!m_initialized) {
            return {};
        }

        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return {};
        }

        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        auto trade_component = entity_manager->GetComponent<economy::TradeComponent>(entity_handle);
        
        if (!trade_component) {
            return {};
        }

        return trade_component->outgoing_routes;
    }

    // ECS-based treasury methods
    bool EconomicSystem::SpendMoney(game::types::EntityID entity_id, int amount) {
        if (!m_initialized) {
            return false;
        }

        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return false;
        }

        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        auto treasury_component = entity_manager->GetComponent<economy::TreasuryComponent>(entity_handle);
        
        if (!treasury_component) {
            return false;
        }

        if (treasury_component->gold_reserves >= amount) {
            treasury_component->gold_reserves -= amount;
            ::core::logging::LogDebug("EconomicSystem", 
                "Spent " + std::to_string(amount) + " gold from entity " + std::to_string(static_cast<int>(entity_id)));
            return true;
        }
        return false;
    }

    void EconomicSystem::AddMoney(game::types::EntityID entity_id, int amount) {
        if (!m_initialized) {
            return;
        }

        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return;
        }

        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        auto treasury_component = entity_manager->GetComponent<economy::TreasuryComponent>(entity_handle);
        
        if (treasury_component) {
            treasury_component->gold_reserves += amount;
            ::core::logging::LogDebug("EconomicSystem", 
                "Added " + std::to_string(amount) + " gold to entity " + std::to_string(static_cast<int>(entity_id)));
        }
    }

    int EconomicSystem::GetTreasury(game::types::EntityID entity_id) const {
        if (!m_initialized) {
            return 0;
        }

        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return 0;
        }

        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        auto treasury_component = entity_manager->GetComponent<economy::TreasuryComponent>(entity_handle);
        
        return treasury_component ? treasury_component->gold_reserves : 0;
    }

    int EconomicSystem::GetMonthlyIncome(game::types::EntityID entity_id) const {
        if (!m_initialized) {
            return 0;
        }

        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return 0;
        }

        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        auto economic_component = entity_manager->GetComponent<economy::EconomicComponent>(entity_handle);
        
        return economic_component ? economic_component->monthly_income : 0;
    }

    int EconomicSystem::GetMonthlyExpenses(game::types::EntityID entity_id) const {
        if (!m_initialized) {
            return 0;
        }

        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return 0;
        }

        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        auto economic_component = entity_manager->GetComponent<economy::EconomicComponent>(entity_handle);
        
        return economic_component ? economic_component->monthly_expenses : 0;
    }

    int EconomicSystem::GetNetIncome(game::types::EntityID entity_id) const {
        return GetMonthlyIncome(entity_id) - GetMonthlyExpenses(entity_id);
    }



    // ECS-based private method implementations
    void EconomicSystem::CalculateMonthlyTotals(game::types::EntityID entity_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return;
        }

        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        auto economic_component = entity_manager->GetComponent<economy::EconomicComponent>(entity_handle);
        auto treasury_component = entity_manager->GetComponent<economy::TreasuryComponent>(entity_handle);
        
        if (!economic_component || !treasury_component) {
            return;
        }

        // Calculate total income
        economic_component->monthly_income = treasury_component->tax_income + 
                                           treasury_component->trade_income + 
                                           treasury_component->tribute_income;

        // Calculate total expenses
        economic_component->monthly_expenses = treasury_component->military_expenses + 
                                             treasury_component->administrative_expenses + 
                                             treasury_component->infrastructure_expenses;

        // Update net income
        economic_component->net_income = economic_component->monthly_income - economic_component->monthly_expenses;
    }

    void EconomicSystem::ProcessEntityEconomy(game::types::EntityID entity_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return;
        }

        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        auto economic_component = entity_manager->GetComponent<economy::EconomicComponent>(entity_handle);
        
        if (!economic_component) {
            return;
        }

        // Process basic economic calculations
        // This is where we'd integrate with population data for tax calculations
        economic_component->tax_income = static_cast<int>(
            economic_component->taxable_population * economic_component->tax_rate * 
            economic_component->tax_collection_efficiency);
    }

    void EconomicSystem::ProcessTradeRoutes(game::types::EntityID entity_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return;
        }

        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        auto trade_component = entity_manager->GetComponent<economy::TradeComponent>(entity_handle);
        auto economic_component = entity_manager->GetComponent<economy::EconomicComponent>(entity_handle);
        
        if (!trade_component || !economic_component) {
            return;
        }

        // Calculate trade income from all routes
        int total_trade_income = 0;
        for (const auto& route : trade_component->outgoing_routes) {
            if (route.is_active) {
                total_trade_income += static_cast<int>(route.base_value * route.efficiency * 
                                                     trade_component->trade_node_efficiency);
            }
        }
        
        economic_component->trade_income = total_trade_income;
    }

    void EconomicSystem::GenerateRandomEvent(game::types::EntityID entity_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return;
        }

        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        auto events_component = entity_manager->GetComponent<economy::EconomicEventsComponent>(entity_handle);
        
        if (!events_component) {
            return;
        }

        // Simple random event generation
        if (events_component->active_events.size() < 3) {
            economy::EconomicEvent new_event;
            new_event.type = static_cast<economy::EconomicEvent::Type>(rand() % 9);
            new_event.affected_province = entity_id;
            new_event.duration_months = 1 + (rand() % 6);
            new_event.effect_magnitude = 0.05f + static_cast<float>(rand() % 20) / 100.0f;
            new_event.description = "Random economic event occurred";
            new_event.is_active = true;

            events_component->active_events.push_back(new_event);
        }
    }

    void EconomicSystem::ProcessRandomEvents(game::types::EntityID entity_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return;
        }

        // Generate new events occasionally
        if (rand() % 100 < 5) { // 5% chance per month
            GenerateRandomEvent(entity_id);
        }

        // Process existing events
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        auto events_component = entity_manager->GetComponent<economy::EconomicEventsComponent>(entity_handle);
        
        if (!events_component) {
            return;
        }

        // Update event durations and remove expired events
        for (auto it = events_component->active_events.begin(); it != events_component->active_events.end();) {
            it->duration_months--;
            if (it->duration_months <= 0) {
                it = events_component->active_events.erase(it);
            } else {
                ApplyEventEffects(entity_id, *it);
                ++it;
            }
        }
    }

    void EconomicSystem::ApplyEventEffects(game::types::EntityID entity_id, const economy::EconomicEvent& event) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return;
        }

        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        auto economic_component = entity_manager->GetComponent<economy::EconomicComponent>(entity_handle);
        
        if (!economic_component) {
            return;
        }

        // Apply event effects based on type
        switch (event.type) {
            case economy::EconomicEvent::GOOD_HARVEST:
                economic_component->tax_income += static_cast<int>(economic_component->tax_income * event.effect_magnitude);
                break;
            case economy::EconomicEvent::BAD_HARVEST:
                economic_component->tax_income -= static_cast<int>(economic_component->tax_income * event.effect_magnitude);
                break;
            case economy::EconomicEvent::MARKET_BOOM:
                economic_component->trade_income += static_cast<int>(economic_component->trade_income * event.effect_magnitude);
                break;
            default:
                // Other event types can be implemented as needed
                break;
        }
    }

    void EconomicSystem::UpdateEventDurations() {
        // This method is now handled within ProcessRandomEvents for each entity
        ::core::logging::LogDebug("EconomicSystem", "UpdateEventDurations - using ECS-based event processing");
    }

    std::vector<economy::EconomicEvent> EconomicSystem::GetActiveEvents(game::types::EntityID entity_id) const {
        if (!m_initialized) {
            return {};
        }

        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return {};
        }

        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        auto events_component = entity_manager->GetComponent<economy::EconomicEventsComponent>(entity_handle);
        
        return events_component ? events_component->active_events : std::vector<economy::EconomicEvent>{};
    }

} // namespace game
