// ============================================================================
// EconomicSystem.cpp - Economic System Implementation
// Strategic Rebuild: October 21, 2025 - Following PopulationSystem pattern
// Location: src/game/economy/EconomicSystem.cpp
// ============================================================================

#include "game/economy/EconomicSystem.h"
#include "game/economy/EconomicComponents.h"
#include "game/config/GameConfig.h"
#include "core/logging/Logger.h"
#include "core/types/game_types.h"
#include "utils/DebugAssert.h"
#include <json/json.h>
#include <algorithm>
#include <cmath>

namespace game::economy {

// ============================================================================
// EconomicSystem Implementation
// ============================================================================

EconomicSystem::EconomicSystem(::core::ecs::ComponentAccessManager& access_manager,
                               ::core::threading::ThreadSafeMessageBus& message_bus)
    : m_access_manager(access_manager), m_message_bus(message_bus) {

    CORE_LOG_INFO("EconomicSystem", "Economic System created");
}

void EconomicSystem::Initialize() {
    if (m_initialized) {
        return;
    }

    CORE_LOG_INFO("EconomicSystem", "Initializing Economic System");

    LoadConfiguration();
    SubscribeToEvents();

    m_initialized = true;
    CORE_LOG_INFO("EconomicSystem", "Economic System initialized successfully");
}

void EconomicSystem::Update(float delta_time) {
    if (!m_initialized) {
        return;
    }

    m_accumulated_time += delta_time;

    // Process regular updates
    ProcessRegularUpdates(delta_time);

    // Process monthly economic cycle
    m_monthly_timer += delta_time;
    if (m_monthly_timer >= m_config.monthly_update_interval) {
        ProcessMonthlyUpdates(delta_time);
        m_monthly_timer = 0.0f;
    }
}

void EconomicSystem::Shutdown() {
    if (!m_initialized) {
        return;
    }

    CORE_LOG_INFO("EconomicSystem", "Shutting down Economic System");
    m_initialized = false;
}

::core::threading::ThreadingStrategy EconomicSystem::GetThreadingStrategy() const {
    return ::core::threading::ThreadingStrategy::THREAD_POOL;
}

std::string EconomicSystem::GetThreadingRationale() const {
    return "Economic calculations are independent per entity and benefit from parallelization";
}

// ============================================================================
// System Initialization
// ============================================================================

void EconomicSystem::LoadConfiguration() {
    // Load configuration values
    m_config.base_tax_rate = 0.10;
    m_config.trade_efficiency = 0.85;
    m_config.inflation_rate = 0.02;
    m_config.starting_treasury = 1000;
    m_config.event_chance_per_month = 0.15;
    
    CORE_LOG_INFO("EconomicSystem", "Configuration loaded successfully");
}

void EconomicSystem::SubscribeToEvents() {
    // TODO: Implement proper message bus subscriptions
    CORE_LOG_DEBUG("EconomicSystem", "Event subscriptions established");
}

// ============================================================================
// Update Processing
// ============================================================================

void EconomicSystem::ProcessRegularUpdates(float delta_time) {
    // Regular economic processing (trade, market fluctuations, etc.)
}

void EconomicSystem::ProcessMonthlyUpdates(float delta_time) {
    // Monthly processing: tax collection, expenses, etc.
    CORE_LOG_DEBUG("EconomicSystem", "Processing monthly economic updates");
}

// ============================================================================
// Economic Management Methods
// ============================================================================

void EconomicSystem::CreateEconomicComponents(game::types::EntityID entity_id) {
    CORE_LOG_INFO("EconomicSystem", 
        "Creating economic components for entity " + std::to_string(static_cast<int>(entity_id)));

    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) {
        CORE_LOG_ERROR("EconomicSystem", "EntityManager not available");
        return;
    }

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);

    // Create main economic component
    auto economic_component = entity_manager->AddComponent<EconomicComponent>(entity_handle);
    if (economic_component) {
        economic_component->treasury = m_config.starting_treasury;
        economic_component->tax_rate = m_config.base_tax_rate;
        economic_component->tax_collection_efficiency = 0.8f;
        economic_component->infrastructure_quality = 0.5f;
        
        CORE_LOG_INFO("EconomicSystem", "Created EconomicComponent");
    }
}

void EconomicSystem::ProcessMonthlyUpdate(game::types::EntityID entity_id) {
    CalculateMonthlyTotals(entity_id);
    ProcessEntityEconomy(entity_id);
    ProcessTradeRoutes(entity_id);
}

// ============================================================================
// Treasury Management
// ============================================================================

bool EconomicSystem::SpendMoney(game::types::EntityID entity_id, int amount) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return false;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto economic_component = entity_manager->GetComponent<EconomicComponent>(entity_handle);

    // Debug assertion: Verify component lifetime
    VERIFY_COMPONENT(economic_component, "EconomicComponent", entity_id);

    if (!economic_component || economic_component->treasury < amount) {
        return false;
    }

    economic_component->treasury -= amount;
    return true;
}

void EconomicSystem::AddMoney(game::types::EntityID entity_id, int amount) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto economic_component = entity_manager->GetComponent<EconomicComponent>(entity_handle);

    if (economic_component) {
        // Check for integer overflow before adding
        const int MAX_TREASURY = 2000000000; // Safe limit below INT_MAX
        if (amount > 0 && economic_component->treasury > MAX_TREASURY - amount) {
            CORE_LOG_WARN("EconomicSystem",
                "Treasury overflow prevented for entity " + std::to_string(static_cast<int>(entity_id)));
            economic_component->treasury = MAX_TREASURY;
        } else if (amount < 0 && economic_component->treasury < -MAX_TREASURY - amount) {
            CORE_LOG_WARN("EconomicSystem",
                "Treasury underflow prevented for entity " + std::to_string(static_cast<int>(entity_id)));
            economic_component->treasury = -MAX_TREASURY;
        } else {
            economic_component->treasury += amount;
        }
    }
}

int EconomicSystem::GetTreasury(game::types::EntityID entity_id) const {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return 0;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto economic_component = entity_manager->GetComponent<EconomicComponent>(entity_handle);
    
    return economic_component ? economic_component->treasury : 0;
}

int EconomicSystem::GetMonthlyIncome(game::types::EntityID entity_id) const {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return 0;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto economic_component = entity_manager->GetComponent<EconomicComponent>(entity_handle);
    
    return economic_component ? economic_component->monthly_income : 0;
}

int EconomicSystem::GetMonthlyExpenses(game::types::EntityID entity_id) const {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return 0;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto economic_component = entity_manager->GetComponent<EconomicComponent>(entity_handle);
    
    return economic_component ? economic_component->monthly_expenses : 0;
}

int EconomicSystem::GetNetIncome(game::types::EntityID entity_id) const {
    return GetMonthlyIncome(entity_id) - GetMonthlyExpenses(entity_id);
}

// ============================================================================
// Trade Route Management
// ============================================================================

void EconomicSystem::AddTradeRoute(game::types::EntityID from_entity, game::types::EntityID to_entity,
                                   float efficiency, int base_value) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID from_handle(static_cast<uint64_t>(from_entity), 1);
    auto economic_component = entity_manager->GetComponent<EconomicComponent>(from_handle);

    if (economic_component) {
        std::lock_guard<std::mutex> lock(economic_component->trade_routes_mutex);
        TradeRoute route(from_entity, to_entity, efficiency, base_value);
        economic_component->active_trade_routes.push_back(route);
    }
}

void EconomicSystem::RemoveTradeRoute(game::types::EntityID from_entity, game::types::EntityID to_entity) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID from_handle(static_cast<uint64_t>(from_entity), 1);
    auto economic_component = entity_manager->GetComponent<EconomicComponent>(from_handle);

    if (economic_component) {
        std::lock_guard<std::mutex> lock(economic_component->trade_routes_mutex);
        auto& routes = economic_component->active_trade_routes;
        routes.erase(
            std::remove_if(routes.begin(), routes.end(),
                [to_entity](const TradeRoute& route) {
                    return route.to_province == to_entity;
                }),
            routes.end()
        );
    }
}

// ============================================================================
// Economic Events
// ============================================================================

void EconomicSystem::ProcessRandomEvents(game::types::EntityID entity_id) {
    // TODO: Implement random event generation and processing
}

std::vector<EconomicEvent> EconomicSystem::GetActiveEvents(game::types::EntityID entity_id) const {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return {};

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto events_component = entity_manager->GetComponent<EconomicEventsComponent>(entity_handle);
    
    return events_component ? events_component->active_events : std::vector<EconomicEvent>{};
}

// ============================================================================
// Configuration Access
// ============================================================================

const EconomicSystemConfig& EconomicSystem::GetConfiguration() const {
    return m_config;
}

// ============================================================================
// Internal Methods
// ============================================================================

void EconomicSystem::CalculateMonthlyTotals(game::types::EntityID entity_id) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto economic_component = entity_manager->GetComponent<EconomicComponent>(entity_handle);
    
    if (economic_component) {
        // Calculate tax income (simplified - would need population data for real calculation)
        int tax_income = static_cast<int>(
            economic_component->treasury * 
            economic_component->tax_rate * 
            economic_component->tax_collection_efficiency * 0.01f  // 1% of treasury as baseline
        );
        
        // Use existing trade income
        int trade_income = economic_component->trade_income;
        
        economic_component->monthly_income = tax_income + trade_income;
        economic_component->net_income = economic_component->monthly_income - economic_component->monthly_expenses;
    }
}

void EconomicSystem::ProcessEntityEconomy(game::types::EntityID entity_id) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto economic_component = entity_manager->GetComponent<EconomicComponent>(entity_handle);

    if (economic_component) {
        int net_income = economic_component->monthly_income - economic_component->monthly_expenses;

        // Check for overflow before adding net income to treasury
        const int MAX_TREASURY = 2000000000;
        if (net_income > 0 && economic_component->treasury > MAX_TREASURY - net_income) {
            CORE_LOG_WARN("EconomicSystem",
                "Treasury overflow prevented during monthly update for entity " + std::to_string(static_cast<int>(entity_id)));
            economic_component->treasury = MAX_TREASURY;
        } else if (net_income < 0 && economic_component->treasury < -MAX_TREASURY - net_income) {
            economic_component->treasury = -MAX_TREASURY;
        } else {
            economic_component->treasury += net_income;
        }

        // Apply inflation
        economic_component->inflation_rate = m_config.inflation_rate;
    }
}

void EconomicSystem::ProcessTradeRoutes(game::types::EntityID entity_id) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto economic_component = entity_manager->GetComponent<EconomicComponent>(entity_handle);

    if (economic_component) {
        int total_trade_income = 0;
        const int MAX_TRADE_INCOME = 1000000000; // Safe accumulation limit

        for (const auto& route : economic_component->active_trade_routes) {
            if (route.is_active) {
                int route_income = static_cast<int>(route.base_value * route.efficiency);

                // Check for overflow during accumulation
                if (route_income > 0 && total_trade_income > MAX_TRADE_INCOME - route_income) {
                    CORE_LOG_WARN("EconomicSystem",
                        "Trade income overflow prevented for entity " + std::to_string(static_cast<int>(entity_id)));
                    total_trade_income = MAX_TRADE_INCOME;
                    break;
                } else {
                    total_trade_income += route_income;
                }
            }
        }

        economic_component->trade_income = total_trade_income;
    }
}

void EconomicSystem::GenerateRandomEvent(game::types::EntityID entity_id) {
    // TODO: Implement random event generation
}

void EconomicSystem::ApplyEventEffects(game::types::EntityID entity_id, const EconomicEvent& event) {
    // TODO: Implement event effects
}

// ============================================================================
// ISystem Interface Implementation
// ============================================================================

std::string EconomicSystem::GetSystemName() const {
    return "EconomicSystem";
}

Json::Value EconomicSystem::Serialize(int version) const {
    Json::Value data;
    data["system_name"] = "EconomicSystem";
    data["version"] = version;
    data["initialized"] = m_initialized;
    // TODO: Serialize economic state
    return data;
}

bool EconomicSystem::Deserialize(const Json::Value& data, int version) {
    if (data["system_name"].asString() != "EconomicSystem") {
        return false;
    }
    m_initialized = data["initialized"].asBool();
    // TODO: Deserialize economic state
    return true;
}

} // namespace game::economy
