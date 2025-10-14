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

    void EconomicSystem::addTradeRoute(int from_province, int to_province, float efficiency, int base_value) {
        auto it = std::find_if(trade_routes.begin(), trade_routes.end(),
            [from_province, to_province](const TradeRoute& route) {
                return (route.from_province == from_province && route.to_province == to_province) ||
                    (route.from_province == to_province && route.to_province == from_province);
            });

        if (it == trade_routes.end()) {
            trade_routes.emplace_back(from_province, to_province, efficiency, base_value);
        }
    }

    void EconomicSystem::removeTradeRoute(int from_province, int to_province) {
        trade_routes.erase(
            std::remove_if(trade_routes.begin(), trade_routes.end(),
                [from_province, to_province](const TradeRoute& route) {
                    return (route.from_province == from_province && route.to_province == to_province) ||
                        (route.from_province == to_province && route.to_province == from_province);
                }),
            trade_routes.end());
    }

    std::vector<TradeRoute> EconomicSystem::getTradeRoutesForProvince(int province_id) const {
        std::vector<TradeRoute> routes;
        for (const auto& route : trade_routes) {
            if (route.from_province == province_id || route.to_province == province_id) {
                routes.push_back(route);
            }
        }
        return routes;
    }

    int EconomicSystem::calculateTradeIncome(const Province& province) const {
        int trade_income = 0;

        for (const auto& route : trade_routes) {
            if (route.from_province == province.id || route.to_province == province.id) {
                float base_trade = route.base_value * route.efficiency;

                base_trade *= (1.0f + province.getMarketTradeBonus());

                // FIXED: Load stability modifier from config
                auto& config = game::config::GameConfig::Instance();
                float stability_min = config.GetFloat("economy.stability_min_modifier", 0.5f);
                float stability_range = config.GetFloat("economy.stability_range_modifier", 0.5f);
                base_trade *= (stability_min + (province.stability * stability_range));

                if (admin_system) {
                    float trade_efficiency = admin_system->getProvinceTradeEfficiency(province.id);
                    base_trade *= trade_efficiency;
                }

                trade_income += static_cast<int>(base_trade);
            }
        }

        return trade_income;
    }

    bool EconomicSystem::spendMoney(int amount) {
        if (national_treasury >= amount) {
            national_treasury -= amount;
            return true;
        }
        return false;
    }

    void EconomicSystem::addMoney(int amount) {
        national_treasury += amount;
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

    void EconomicSystem::processRandomEvents(std::vector<Province>& provinces) {
        auto& config = game::config::GameConfig::Instance();
        int event_chance = config.GetInt("economy.random_event_chance_percent", 5);
        int max_concurrent_events = config.GetInt("economy.max_concurrent_events", 3);

        if (active_events.size() < static_cast<size_t>(max_concurrent_events) && 
            (rand() % 100) < event_chance) {
            generateRandomEvent(provinces);
        }
    }

    int EconomicSystem::getTotalTaxIncome(const std::vector<Province>& provinces) const {
        int total = 0;
        for (const auto& province : provinces) {
            float admin_efficiency = admin_system ? 
                admin_system->getProvinceTaxEfficiency(province.id) : 0.5f;
            total += province.calculateActualTaxIncome(admin_efficiency);
        }
        return total;
    }

    int EconomicSystem::getTotalTradeIncome(const std::vector<Province>& provinces) const {
        int total = 0;
        for (const auto& province : provinces) {
            total += calculateTradeIncome(province);
        }
        return total;
    }

    int EconomicSystem::getTotalPopulation(const std::vector<Province>& provinces) const {
        int total = 0;
        for (const auto& province : provinces) {
            total += province.current_population;
        }
        return total;
    }

    float EconomicSystem::getAverageStability(const std::vector<Province>& provinces) const {
        if (provinces.empty()) return 0.0f;

        float total_stability = 0.0f;
        for (const auto& province : provinces) {
            total_stability += province.stability;
        }

        return total_stability / provinces.size();
    }

    void EconomicSystem::calculateMonthlyTotals(const std::vector<Province>& provinces) {
        monthly_income = getTotalTaxIncome(provinces) + getTotalTradeIncome(provinces);

        auto& config = game::config::GameConfig::Instance();
        monthly_expenses = config.GetInt("economy.base_monthly_expenses", 50);

        for (const auto& province : provinces) {
            if (province.construction.is_active) {
                monthly_expenses += province.construction.total_cost / 
                    std::max(1, province.construction.months_remaining);
            }
        }
    }

    void EconomicSystem::processProvinceEconomy(Province& province) {
        int growth = province.calculateActualPopulationGrowth();
        province.current_population += growth;

        float stability_change = 0.0f;

        if (admin_system) {
            auto& config = game::config::GameConfig::Instance();
            float good_admin_threshold = config.GetFloat("economy.good_admin_threshold", 0.7f);
            float poor_admin_threshold = config.GetFloat("economy.poor_admin_threshold", 0.3f);
            float good_admin_bonus = config.GetFloat("economy.good_admin_stability_bonus", 0.02f);
            float poor_admin_penalty = config.GetFloat("economy.poor_admin_stability_penalty", -0.01f);

            float admin_eff = admin_system->getProvinceAdministrativeEfficiency(province.id);
            if (admin_eff > good_admin_threshold) {
                stability_change += good_admin_bonus;
            }
            else if (admin_eff < poor_admin_threshold) {
                stability_change += poor_admin_penalty;
            }
        }

        stability_change += province.getTempleStabilityBonus();

        if (province.war_exhaustion > 0.0f) {
            auto& config = game::config::GameConfig::Instance();
            float war_recovery_rate = config.GetFloat("economy.war_exhaustion_recovery_rate", 0.05f);
            province.war_exhaustion -= war_recovery_rate;
            province.war_exhaustion = std::max(0.0f, province.war_exhaustion);
        }

        auto& config = game::config::GameConfig::Instance();
        float neutral_stability = config.GetFloat("economy.neutral_stability", 0.5f);
        float stability_increase_rate = config.GetFloat("economy.stability_increase_rate", 0.01f);
        float stability_decrease_rate = config.GetFloat("economy.stability_decrease_rate", 0.005f);

        if (province.stability < neutral_stability) {
            stability_change += stability_increase_rate;
        }
        else if (province.stability > neutral_stability) {
            stability_change -= stability_decrease_rate;
        }

        province.stability += stability_change;
        province.stability = std::max(0.0f, std::min(1.0f, province.stability));

        if (province.construction.is_active) {
            if (province.processConstructionMonth()) {
                // Construction completed
            }
        }

        for (const auto& event : active_events) {
            if (event.affected_province == province.id) {
                applyEventEffects(province, event);
            }
        }

        float admin_tax_eff = admin_system ? 
            admin_system->getProvinceTaxEfficiency(province.id) : 0.5f;
        province.actual_tax_collected = province.calculateActualTaxIncome(admin_tax_eff);
    }

    void EconomicSystem::processTradeRoutes(std::vector<Province>& provinces) {
        auto& config = game::config::GameConfig::Instance();
        float base_efficiency = config.GetFloat("economy.trade_base_efficiency", 0.5f);
        float stability_modifier = config.GetFloat("economy.trade_stability_modifier", 0.4f);
        float efficiency_increase_rate = config.GetFloat("economy.trade_efficiency_increase_rate", 0.02f);
        float efficiency_decrease_rate = config.GetFloat("economy.trade_efficiency_decrease_rate", 0.01f);

        for (auto& route : trade_routes) {
            Province* from_prov = nullptr;
            Province* to_prov = nullptr;

            for (auto& province : provinces) {
                if (province.id == route.from_province) from_prov = &province;
                if (province.id == route.to_province) to_prov = &province;
            }

            if (from_prov && to_prov) {
                float avg_stability = (from_prov->stability + to_prov->stability) / 2.0f;

                float target_efficiency = base_efficiency + (avg_stability * stability_modifier);

                if (route.efficiency < target_efficiency) {
                    route.efficiency = std::min(target_efficiency, route.efficiency + efficiency_increase_rate);
                }
                else if (route.efficiency > target_efficiency) {
                    route.efficiency = std::max(target_efficiency, route.efficiency - efficiency_decrease_rate);
                }
            }
        }
    }

    void EconomicSystem::generateRandomEvent(const std::vector<Province>& provinces) {
        if (provinces.empty()) return;

        auto& config = game::config::GameConfig::Instance();

        RandomEvent event;
        event.affected_province = provinces[rand() % provinces.size()].id;
        
        int min_duration = config.GetInt("economy.event_min_duration_months", 1);
        int max_duration = config.GetInt("economy.event_max_duration_months", 3);
        event.duration_months = min_duration + (rand() % (max_duration - min_duration + 1));

        int event_type = rand() % 6;
        switch (event_type) {
        case 0:
            event.type = RandomEvent::GOOD_HARVEST;
            event.effect_magnitude = config.GetFloat("economy.good_harvest_min", 0.2f) + 
                (rand() % 30) / 100.0f;
            event.description = "Exceptional harvest yields in Province " + 
                std::to_string(event.affected_province);
            break;

        case 1:
            event.type = RandomEvent::BAD_HARVEST;
            event.effect_magnitude = config.GetFloat("economy.bad_harvest_min", 0.1f) + 
                (rand() % 20) / 100.0f;
            event.description = "Poor harvest affects Province " + 
                std::to_string(event.affected_province);
            break;

        case 2:
            event.type = RandomEvent::MERCHANT_CARAVAN;
            event.effect_magnitude = static_cast<float>(
                config.GetInt("economy.merchant_caravan_min_gold", 15) + 
                (rand() % 25));
            event.description = "Wealthy merchant caravan brings trade to Province " + 
                std::to_string(event.affected_province);
            break;

        case 3:
            event.type = RandomEvent::BANDIT_RAID;
            event.effect_magnitude = config.GetFloat("economy.bandit_raid_min", 0.05f) + 
                (rand() % 15) / 100.0f;
            event.description = "Bandit raids disrupt Province " + 
                std::to_string(event.affected_province);
            break;

        case 4:
            event.type = RandomEvent::PLAGUE_OUTBREAK;
            event.effect_magnitude = config.GetFloat("economy.plague_min", 0.02f) + 
                (rand() % 8) / 100.0f;
            event.description = "Disease outbreak in Province " + 
                std::to_string(event.affected_province);
            break;

        case 5:
            event.type = RandomEvent::MARKET_BOOM;
            event.effect_magnitude = config.GetFloat("economy.market_boom_min", 0.3f) + 
                (rand() % 40) / 100.0f;
            event.description = "Market boom increases trade in Province " + 
                std::to_string(event.affected_province);
            break;
        }

        active_events.push_back(event);
    }

    void EconomicSystem::applyEventEffects(Province& province, const RandomEvent& event) {
        auto& config = game::config::GameConfig::Instance();

        switch (event.type) {
        case RandomEvent::GOOD_HARVEST:
            province.stability += config.GetFloat("economy.good_harvest_stability_bonus", 0.01f);
            break;

        case RandomEvent::BAD_HARVEST:
            province.stability -= config.GetFloat("economy.bad_harvest_stability_penalty", 0.01f);
            break;

        case RandomEvent::MERCHANT_CARAVAN:
            province.stability += config.GetFloat("economy.merchant_caravan_stability_bonus", 0.005f);
            break;

        case RandomEvent::BANDIT_RAID:
            province.stability -= config.GetFloat("economy.bandit_raid_stability_penalty", 0.02f);
            break;

        case RandomEvent::PLAGUE_OUTBREAK:
            province.current_population = std::max(
                config.GetInt("economy.plague_min_population", 100),
                static_cast<int>(province.current_population * (1.0f - event.effect_magnitude)));
            province.stability -= config.GetFloat("economy.plague_stability_penalty", 0.03f);
            break;

        case RandomEvent::MARKET_BOOM:
            province.stability += config.GetFloat("economy.market_boom_stability_bonus", 0.01f);
            break;
        }

        province.stability = std::max(0.0f, std::min(1.0f, province.stability));
    }

    void EconomicSystem::updateEventDurations() {
        for (auto it = active_events.begin(); it != active_events.end();) {
            it->duration_months--;
            if (it->duration_months <= 0) {
                it = active_events.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    // DEPRECATED: Legacy serialization
    void EconomicSystem::serializeToString(std::string& out) const {
        std::ostringstream oss;
        oss << "ECONOMIC_SYSTEM_V1\n";
        oss << "treasury:" << national_treasury << "\n";
        oss << "trade_route_count:" << trade_routes.size() << "\n";

        for (const auto& route : trade_routes) {
            oss << "route:" << route.from_province << "," << route.to_province << ","
                << route.efficiency << "," << route.base_value << "\n";
        }

        oss << "event_count:" << active_events.size() << "\n";
        for (const auto& event : active_events) {
            oss << "event:" << static_cast<int>(event.type) << "," << event.affected_province << ","
                << event.duration_months << "," << event.effect_magnitude << ","
                << event.description << "\n";
        }

        out = oss.str();
    }

    bool EconomicSystem::deserializeFromString(const std::string& data) {
        std::istringstream iss(data);
        std::string line;

        if (!std::getline(iss, line) || line != "ECONOMIC_SYSTEM_V1") {
            return false;
        }

        trade_routes.clear();
        active_events.clear();

        while (std::getline(iss, line)) {
            if (starts_with(line, "treasury:")) {
                national_treasury = std::stoi(line.substr(9));
            }
            else if (starts_with(line, "route:")) {
                std::string route_data = line.substr(6);
                std::istringstream route_stream(route_data);
                std::string token;

                std::vector<std::string> tokens;
                while (std::getline(route_stream, token, ',')) {
                    tokens.push_back(token);
                }

                if (tokens.size() == 4) {
                    int from = std::stoi(tokens[0]);
                    int to = std::stoi(tokens[1]);
                    float eff = std::stof(tokens[2]);
                    int value = std::stoi(tokens[3]);
                    trade_routes.emplace_back(from, to, eff, value);
                }
            }
            else if (starts_with(line, "event:")) {
                std::string event_data = line.substr(6);
                std::istringstream event_stream(event_data);
                std::string token;

                std::vector<std::string> tokens;
                while (std::getline(event_stream, token, ',')) {
                    tokens.push_back(token);
                }

                if (tokens.size() >= 5) {
                    RandomEvent event;
                    event.type = static_cast<RandomEvent::Type>(std::stoi(tokens[0]));
                    event.affected_province = std::stoi(tokens[1]);
                    event.duration_months = std::stoi(tokens[2]);
                    event.effect_magnitude = std::stof(tokens[3]);
                    event.description = tokens[4];

                    for (size_t i = 5; i < tokens.size(); ++i) {
                        event.description += "," + tokens[i];
                    }

                    active_events.push_back(event);
                }
            }
        }

        return true;
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
