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

    if (!economic_component) {
        return false;
    }

    // Check if spending would violate minimum treasury (CRITICAL FIX)
    if (economic_component->treasury - amount < m_config.min_treasury) {
        CORE_LOG_WARN("EconomicSystem",
                     "Cannot spend " + std::to_string(amount) +
                     " for entity " + std::to_string(static_cast<int>(entity_id)) +
                     ": would violate minimum treasury (" + std::to_string(m_config.min_treasury) + ")");
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
                                   double efficiency, int base_value) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID from_handle(static_cast<uint64_t>(from_entity), 1);
    auto economic_component = entity_manager->GetComponent<EconomicComponent>(from_handle);

    if (economic_component) {
        std::lock_guard<std::mutex> lock(economic_component->trade_routes_mutex);
        TradeRoute route(from_entity, to_entity, efficiency, base_value);
        economic_component->active_trade_routes.push_back(route);

        CORE_LOG_INFO("EconomicSystem", "Added trade route from " + std::to_string(static_cast<int>(from_entity)) +
                      " to " + std::to_string(static_cast<int>(to_entity)) +
                      " (efficiency: " + std::to_string(efficiency) + ")");
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

        CORE_LOG_INFO("EconomicSystem", "Removed trade route from " + std::to_string(static_cast<int>(from_entity)) +
                      " to " + std::to_string(static_cast<int>(to_entity)));
    }
}

std::vector<TradeRoute> EconomicSystem::GetTradeRoutesForEntity(game::types::EntityID entity_id) const {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return {};

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto economic_component = entity_manager->GetComponent<EconomicComponent>(entity_handle);

    if (economic_component) {
        std::lock_guard<std::mutex> lock(economic_component->trade_routes_mutex);
        return economic_component->active_trade_routes;
    }

    return {};
}

// ============================================================================
// Economic Events
// ============================================================================

void EconomicSystem::ProcessRandomEvents(game::types::EntityID entity_id) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto events_component = entity_manager->GetComponent<EconomicEventsComponent>(entity_handle);

    if (!events_component) {
        // Create component if it doesn't exist
        events_component = entity_manager->AddComponent<EconomicEventsComponent>(entity_handle);
        if (!events_component) return;
    }

    // Update event durations and remove expired events
    for (auto& event : events_component->active_events) {
        if (event.is_active && event.duration_months > 0) {
            event.duration_months--;
            if (event.duration_months <= 0) {
                event.is_active = false;
            }
        }
    }

    // Remove inactive events
    events_component->active_events.erase(
        std::remove_if(events_component->active_events.begin(),
                      events_component->active_events.end(),
                      [](const EconomicEvent& e) { return !e.is_active; }),
        events_component->active_events.end()
    );

    // Track months since last event
    events_component->months_since_last_event++;

    // Roll for random event generation
    double event_roll = static_cast<double>(std::rand()) / RAND_MAX;
    double event_chance = m_config.event_chance_per_month * events_component->event_frequency_modifier;

    if (event_roll < event_chance && events_component->months_since_last_event >= 3) {
        GenerateRandomEvent(entity_id);
        events_component->months_since_last_event = 0;
    }
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

        {
            // Lock mutex when reading trade routes to prevent race conditions
            std::lock_guard<std::mutex> lock(economic_component->trade_routes_mutex);

            for (const auto& route : economic_component->active_trade_routes) {
                if (route.is_active) {
                    int route_income = static_cast<int>(route.base_value * route.efficiency);

                    // Check for overflow BEFORE accumulation (CRITICAL FIX)
                    if (route_income > 0 && total_trade_income > MAX_TRADE_INCOME - route_income) {
                        CORE_LOG_WARN("EconomicSystem",
                            "Trade income overflow prevented for entity " + std::to_string(static_cast<int>(entity_id)));
                        total_trade_income = MAX_TRADE_INCOME;
                        break;
                    }
                    total_trade_income += route_income;
                }
            }
        } // Release lock here

        economic_component->trade_income = total_trade_income;
    }
}

void EconomicSystem::GenerateRandomEvent(game::types::EntityID entity_id) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto events_component = entity_manager->GetComponent<EconomicEventsComponent>(entity_handle);

    if (!events_component) return;

    // Determine if good or bad event
    double event_type_roll = static_cast<double>(std::rand()) / RAND_MAX;
    bool is_good_event = event_type_roll < m_config.good_event_weight /
                         (m_config.good_event_weight + m_config.bad_event_weight);

    // Create the event
    EconomicEvent new_event;
    new_event.affected_province = entity_id;
    new_event.duration_months = 3 + (std::rand() % 9); // 3-12 months
    new_event.is_active = true;

    // Select event type and magnitude
    if (is_good_event) {
        int event_choice = std::rand() % 3;
        switch (event_choice) {
            case 0:
                new_event.type = EconomicEvent::Type::GOOD_HARVEST;
                new_event.effect_magnitude = 0.1 + (std::rand() % 20) / 100.0; // 10-30% boost
                new_event.description = "Bountiful Harvest: Agricultural output increased";
                break;
            case 1:
                new_event.type = EconomicEvent::Type::MERCHANT_CARAVAN;
                new_event.effect_magnitude = 0.15 + (std::rand() % 25) / 100.0; // 15-40% boost
                new_event.description = "Merchant Caravan: Trade income increased";
                break;
            case 2:
                new_event.type = EconomicEvent::Type::MARKET_BOOM;
                new_event.effect_magnitude = 0.2 + (std::rand() % 30) / 100.0; // 20-50% boost
                new_event.description = "Market Boom: Economic activity surging";
                break;
        }
    } else {
        int event_choice = std::rand() % 3;
        switch (event_choice) {
            case 0:
                new_event.type = EconomicEvent::Type::BAD_HARVEST;
                new_event.effect_magnitude = -(0.1 + (std::rand() % 20) / 100.0); // -10 to -30%
                new_event.description = "Poor Harvest: Agricultural output decreased";
                break;
            case 1:
                new_event.type = EconomicEvent::Type::BANDIT_RAID;
                new_event.effect_magnitude = -(0.15 + (std::rand() % 25) / 100.0); // -15 to -40%
                new_event.description = "Bandit Raid: Trade routes disrupted";
                break;
            case 2:
                new_event.type = EconomicEvent::Type::TRADE_DISRUPTION;
                new_event.effect_magnitude = -(0.2 + (std::rand() % 30) / 100.0); // -20 to -50%
                new_event.description = "Trade Disruption: Commerce heavily affected";
                break;
        }
    }

    // Add to active events
    events_component->active_events.push_back(new_event);

    // Add to history (limit size)
    events_component->event_history.push_back(new_event);
    if (events_component->event_history.size() > static_cast<size_t>(events_component->max_history_size)) {
        events_component->event_history.erase(events_component->event_history.begin());
    }

    // Apply immediate effects
    ApplyEventEffects(entity_id, new_event);

    CORE_LOG_INFO("EconomicSystem", "Generated economic event for entity " +
                  std::to_string(static_cast<int>(entity_id)) + ": " + new_event.description);
}

void EconomicSystem::ApplyEventEffects(game::types::EntityID entity_id, const EconomicEvent& event) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) return;

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto economic_component = entity_manager->GetComponent<EconomicComponent>(entity_handle);

    if (!economic_component) return;

    // Apply effects based on event type
    switch (event.type) {
        case EconomicEvent::Type::GOOD_HARVEST:
        case EconomicEvent::Type::BAD_HARVEST:
            // Affects agricultural production (would need resource system integration)
            economic_component->economic_growth += event.effect_magnitude;
            break;

        case EconomicEvent::Type::MERCHANT_CARAVAN:
        case EconomicEvent::Type::BANDIT_RAID:
        case EconomicEvent::Type::TRADE_DISRUPTION:
            // Affects trade income
            economic_component->trade_efficiency += event.effect_magnitude;
            economic_component->trade_efficiency = std::max(0.1, std::min(2.0, economic_component->trade_efficiency));
            break;

        case EconomicEvent::Type::MARKET_BOOM:
            // Boosts overall economic activity
            economic_component->economic_growth += event.effect_magnitude;
            economic_component->market_demand *= (1.0 + event.effect_magnitude);
            break;

        case EconomicEvent::Type::PLAGUE_OUTBREAK:
            // Severe negative effects
            economic_component->economic_growth += event.effect_magnitude;
            economic_component->employment_rate *= (1.0 + event.effect_magnitude);
            break;

        case EconomicEvent::Type::TAX_REVOLT:
            // Reduces tax collection efficiency
            economic_component->tax_collection_efficiency *= (1.0 + event.effect_magnitude);
            economic_component->tax_collection_efficiency = std::max(0.1, std::min(1.0, economic_component->tax_collection_efficiency));
            break;

        case EconomicEvent::Type::MERCHANT_GUILD_FORMATION:
            // Boosts trade permanently
            economic_component->trade_efficiency += event.effect_magnitude * 0.5; // Permanent partial bonus
            break;
    }

    CORE_LOG_DEBUG("EconomicSystem", "Applied event effects for entity " + std::to_string(static_cast<int>(entity_id)));
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
