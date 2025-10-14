// ============================================================================
// Date/Time Created: September 27, 2025 - 4:25 PM PST
// Intended Folder Location: src/game/EconomicSystemSerialization.cpp
// EconomicSystem Serialization Implementation
// ============================================================================

#include "game/economy/EconomicSystem.h"
#include <json/json.h>

namespace game {

    Json::Value EconomicSystem::Serialize(int version) const {
        Json::Value root;
        root["version"] = version;
        root["system_name"] = "EconomicSystem";
        
        // Treasury state
        root["national_treasury"] = national_treasury;
        root["monthly_income"] = monthly_income;
        root["monthly_expenses"] = monthly_expenses;
        
        // Trade routes
        Json::Value routes(Json::arrayValue);
        for (const auto& route : trade_routes) {
            Json::Value r;
            r["from_province"] = route.from_province;
            r["to_province"] = route.to_province;
            r["efficiency"] = route.efficiency;
            r["base_value"] = route.base_value;
            routes.append(r);
        }
        root["trade_routes"] = routes;
        
        // Active events
        Json::Value events(Json::arrayValue);
        for (const auto& event : active_events) {
            Json::Value e;
            e["type"] = static_cast<int>(event.type);
            e["affected_province"] = event.affected_province;
            e["duration_months"] = event.duration_months;
            e["effect_magnitude"] = event.effect_magnitude;
            e["description"] = event.description;
            events.append(e);
        }
        root["active_events"] = events;
        
        return root;
    }

    bool EconomicSystem::Deserialize(const Json::Value& data, int version) {
        try {
            if (!data.isMember("system_name") || data["system_name"].asString() != "EconomicSystem") {
                return false;
            }
            
            // Treasury state
            national_treasury = data.get("national_treasury", 1000).asInt();
            monthly_income = data.get("monthly_income", 0).asInt();
            monthly_expenses = data.get("monthly_expenses", 0).asInt();
            
            // Trade routes
            trade_routes.clear();
            if (data.isMember("trade_routes") && data["trade_routes"].isArray()) {
                const Json::Value& routes = data["trade_routes"];
                for (const auto& r : routes) {
                    int from = r.get("from_province", 0).asInt();
                    int to = r.get("to_province", 0).asInt();
                    float eff = r.get("efficiency", 0.5f).asFloat();
                    int value = r.get("base_value", 0).asInt();
                    trade_routes.emplace_back(from, to, eff, value);
                }
            }
            
            // Active events
            active_events.clear();
            if (data.isMember("active_events") && data["active_events"].isArray()) {
                const Json::Value& events = data["active_events"];
                for (const auto& e : events) {
                    RandomEvent event;
                    event.type = static_cast<RandomEvent::Type>(e.get("type", 0).asInt());
                    event.affected_province = e.get("affected_province", 0).asInt();
                    event.duration_months = e.get("duration_months", 0).asInt();
                    event.effect_magnitude = e.get("effect_magnitude", 0.0f).asFloat();
                    event.description = e.get("description", "").asString();
                    active_events.push_back(event);
                }
            }
            
            return true;
            
        } catch (const std::exception&) {
            return false;
        }
    }

} // namespace game
