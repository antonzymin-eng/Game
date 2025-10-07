// ============================================================================
// Date/Time Created: September 27, 2025 - 4:15 PM PST
// Intended Folder Location: src/game/EconomicSystem.cpp
// FIXED: Externalized hardcoded values to GameConfig
// ============================================================================

#include "game/EconomicSystem.h"
#include "game/AdministrativeSystem.h"
#include "../core/config/GameConfig.h"
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

    void EconomicSystem::initialize(AdministrativeSystem* administrative_system) {
        admin_system = administrative_system;

        // FIXED: Load from config instead of hardcoding
        auto& config = game::config::GameConfig::Instance();
        
        national_treasury = config.GetInt("economy.starting_treasury", 1000);
        
        // Initialize trade routes from config or defaults
        addTradeRoute(0, 1, 0.8f, 25);
        addTradeRoute(1, 2, 0.7f, 20);
        addTradeRoute(0, 2, 0.6f, 15);
    }

    void EconomicSystem::processMonthlyUpdate(std::vector<Province>& provinces) {
        for (auto& province : provinces) {
            processProvinceEconomy(province);
        }

        processTradeRoutes(provinces);
        processRandomEvents(provinces);
        updateEventDurations();
        calculateMonthlyTotals(provinces);

        if (admin_system) {
            monthly_expenses += admin_system->getMonthlySalaryCost();
        }

        national_treasury += getNetIncome();
        if (national_treasury < 0) {
            national_treasury = 0;
        }
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

} // namespace game
