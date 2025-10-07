#include "game/EconomicSystem.h"
#include "game/AdministrativeSystem.h"
#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace game {

    // Helper function to replace starts_with for C++17 compatibility
    static bool starts_with(const std::string& str, const std::string& prefix) {
        return str.size() >= prefix.size() &&
            str.compare(0, prefix.size(), prefix) == 0;
    }

    void EconomicSystem::initialize(AdministrativeSystem* administrative_system) {
        admin_system = administrative_system;

        // Initialize with some basic trade routes (these could be data-driven later)
        addTradeRoute(0, 1, 0.8f, 25);
        addTradeRoute(1, 2, 0.7f, 20);
        addTradeRoute(0, 2, 0.6f, 15);
    }

    void EconomicSystem::processMonthlyUpdate(std::vector<Province>& provinces) {
        // Process each province's economy with administrative efficiency
        for (auto& province : provinces) {
            processProvinceEconomy(province);
        }

        // Process trade routes
        processTradeRoutes(provinces);

        // Handle random events
        processRandomEvents(provinces);
        updateEventDurations();

        // Calculate totals
        calculateMonthlyTotals(provinces);

        // Administrative expenses (salaries)
        if (admin_system) {
            monthly_expenses += admin_system->getMonthlySalaryCost();
        }

        // Apply net income to treasury
        national_treasury += getNetIncome();
        if (national_treasury < 0) {
            national_treasury = 0; // Prevent negative treasury (could add debt system later)
        }
    }

    void EconomicSystem::addTradeRoute(int from_province, int to_province, float efficiency, int base_value) {
        // Check if route already exists
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

                // Apply province market building bonus
                base_trade *= (1.0f + province.getMarketTradeBonus());

                // Apply stability modifier
                base_trade *= (0.5f + (province.stability * 0.5f));

                // Apply administrative efficiency if admin system is available
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
        // 5% chance of a new random event each month
        if (active_events.size() < 3 && (rand() % 100) < 5) {
            generateRandomEvent(provinces);
        }
    }

    int EconomicSystem::getTotalTaxIncome(const std::vector<Province>& provinces) const {
        int total = 0;
        for (const auto& province : provinces) {
            // Use administrative efficiency if available
            float admin_efficiency = admin_system ? admin_system->getProvinceTaxEfficiency(province.id) : 0.5f;
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

        // Base expenses (maintenance, court costs, etc.)
        monthly_expenses = 50;

        // Construction costs
        for (const auto& province : provinces) {
            if (province.construction.is_active) {
                monthly_expenses += province.construction.total_cost / std::max(1, province.construction.months_remaining);
            }
        }
    }

    void EconomicSystem::processProvinceEconomy(Province& province) {
        // Population growth
        int growth = province.calculateActualPopulationGrowth();
        province.current_population += growth;

        // Stability changes
        float stability_change = 0.0f;

        // Administrative efficiency affects stability
        if (admin_system) {
            float admin_eff = admin_system->getProvinceAdministrativeEfficiency(province.id);
            if (admin_eff > 0.7f) {
                stability_change += 0.02f; // Good administration improves stability
            }
            else if (admin_eff < 0.3f) {
                stability_change -= 0.01f; // Poor administration hurts stability
            }
        }

        // Temple buildings provide stability bonus
        stability_change += province.getTempleStabilityBonus();

        // War exhaustion recovery
        if (province.war_exhaustion > 0.0f) {
            province.war_exhaustion -= 0.05f;
            province.war_exhaustion = std::max(0.0f, province.war_exhaustion);
        }

        // Natural stability recovery towards 0.5 (neutral)
        if (province.stability < 0.5f) {
            stability_change += 0.01f;
        }
        else if (province.stability > 0.5f) {
            stability_change -= 0.005f; // Slower decline from high stability
        }

        province.stability += stability_change;
        province.stability = std::max(0.0f, std::min(1.0f, province.stability));

        // Process construction
        if (province.construction.is_active) {
            if (province.processConstructionMonth()) {
                // Construction completed - building level increased in processConstructionMonth()
            }
        }

        // Apply random event effects
        for (const auto& event : active_events) {
            if (event.affected_province == province.id) {
                applyEventEffects(province, event);
            }
        }

        // Tax calculation with administrative efficiency
        float admin_tax_eff = admin_system ? admin_system->getProvinceTaxEfficiency(province.id) : 0.5f;
        province.actual_tax_collected = province.calculateActualTaxIncome(admin_tax_eff);
    }

    void EconomicSystem::processTradeRoutes(std::vector<Province>& provinces) {
        // Trade routes can degrade over time due to poor administration or events
        for (auto& route : trade_routes) {
            // Find provinces involved in this route
            Province* from_prov = nullptr;
            Province* to_prov = nullptr;

            for (auto& province : provinces) {
                if (province.id == route.from_province) from_prov = &province;
                if (province.id == route.to_province) to_prov = &province;
            }

            if (from_prov && to_prov) {
                // Trade efficiency depends on stability of both provinces
                float avg_stability = (from_prov->stability + to_prov->stability) / 2.0f;

                // Gradually adjust route efficiency based on conditions
                float target_efficiency = 0.5f + (avg_stability * 0.4f); // 0.5 to 0.9 range

                if (route.efficiency < target_efficiency) {
                    route.efficiency = std::min(target_efficiency, route.efficiency + 0.02f);
                }
                else if (route.efficiency > target_efficiency) {
                    route.efficiency = std::max(target_efficiency, route.efficiency - 0.01f);
                }
            }
        }
    }

    void EconomicSystem::generateRandomEvent(const std::vector<Province>& provinces) {
        if (provinces.empty()) return;

        RandomEvent event;
        event.affected_province = provinces[rand() % provinces.size()].id;
        event.duration_months = 1 + (rand() % 3); // 1-3 months

        int event_type = rand() % 6;
        switch (event_type) {
        case 0: // Good harvest
            event.type = RandomEvent::GOOD_HARVEST;
            event.effect_magnitude = 0.2f + (rand() % 30) / 100.0f; // 0.2-0.5
            event.description = "Exceptional harvest yields in Province " + std::to_string(event.affected_province);
            break;

        case 1: // Bad harvest
            event.type = RandomEvent::BAD_HARVEST;
            event.effect_magnitude = 0.1f + (rand() % 20) / 100.0f; // 0.1-0.3
            event.description = "Poor harvest affects Province " + std::to_string(event.affected_province);
            break;

        case 2: // Merchant caravan
            event.type = RandomEvent::MERCHANT_CARAVAN;
            event.effect_magnitude = 15 + (rand() % 25); // 15-40 extra gold
            event.description = "Wealthy merchant caravan brings trade to Province " + std::to_string(event.affected_province);
            break;

        case 3: // Bandit raid
            event.type = RandomEvent::BANDIT_RAID;
            event.effect_magnitude = 0.05f + (rand() % 15) / 100.0f; // 0.05-0.2
            event.description = "Bandit raids disrupt Province " + std::to_string(event.affected_province);
            break;

        case 4: // Plague
            event.type = RandomEvent::PLAGUE_OUTBREAK;
            event.effect_magnitude = 0.02f + (rand() % 8) / 100.0f; // 0.02-0.1
            event.description = "Disease outbreak in Province " + std::to_string(event.affected_province);
            break;

        case 5: // Market boom
            event.type = RandomEvent::MARKET_BOOM;
            event.effect_magnitude = 0.3f + (rand() % 40) / 100.0f; // 0.3-0.7
            event.description = "Market boom increases trade in Province " + std::to_string(event.affected_province);
            break;
        }

        active_events.push_back(event);
    }

    void EconomicSystem::applyEventEffects(Province& province, const RandomEvent& event) {
        switch (event.type) {
        case RandomEvent::GOOD_HARVEST:
            province.stability += 0.01f;
            // Bonus handled in tax income calculation
            break;

        case RandomEvent::BAD_HARVEST:
            province.stability -= 0.01f;
            // Penalty handled in tax income calculation
            break;

        case RandomEvent::MERCHANT_CARAVAN:
            // Direct treasury bonus handled elsewhere
            province.stability += 0.005f;
            break;

        case RandomEvent::BANDIT_RAID:
            province.stability -= 0.02f;
            break;

        case RandomEvent::PLAGUE_OUTBREAK:
            province.current_population = std::max(100,
                static_cast<int>(province.current_population * (1.0f - event.effect_magnitude)));
            province.stability -= 0.03f;
            break;

        case RandomEvent::MARKET_BOOM:
            // Trade bonus handled in trade income calculation
            province.stability += 0.01f;
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
            else if (starts_with(line, "trade_route_count:")) {
                // Just for validation
            }
            else if (starts_with(line, "route:")) {
                std::string route_data = line.substr(6);
                // Parse comma-separated values: from,to,efficiency,value
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
            else if (starts_with(line, "event_count:")) {
                // Just for validation
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

                    // Handle description that might contain commas
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