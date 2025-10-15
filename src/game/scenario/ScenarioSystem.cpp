// ============================================================================
// ScenarioSystem.cpp - Configuration-Based Gameplay Scenario System Implementation
// Created: October 15, 2025
// Demonstrates Phase 1 ECS systems working together in meaningful gameplay
// ============================================================================

#include "game/scenario/ScenarioSystem.h"
#include <fstream>
#include <iostream>
#include <regex>

namespace game::scenario {

    ScenarioSystem::ScenarioSystem(
        ::core::ecs::ComponentAccessManager* comp_mgr,
        ::core::ecs::MessageBus* msg_bus
    ) : component_manager(comp_mgr)
      , message_bus(msg_bus)
      , population_system(nullptr)
      , economic_system(nullptr)
      , military_system(nullptr)
      , technology_system(nullptr)
      , diplomacy_system(nullptr)
      , admin_system(nullptr)
      , active_scenario(nullptr)
    {
        std::cout << "[ScenarioSystem] Scenario system initialized\n";
    }
    
    void ScenarioSystem::RegisterSystems(
        void* pop_sys,
        void* econ_sys,
        void* mil_sys,
        void* tech_sys,
        void* dip_sys,
        void* admin_sys
    ) {
        population_system = pop_sys;
        economic_system = econ_sys;
        military_system = mil_sys;
        technology_system = tech_sys;
        diplomacy_system = dip_sys;
        admin_system = admin_sys;
        
        std::cout << "[ScenarioSystem] Phase 1 systems registered for cross-system scenarios\n";
    }
    
    bool ScenarioSystem::LoadScenario(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "[ScenarioSystem] Failed to open scenario file: " << filename << std::endl;
            return false;
        }
        
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errors;
        
        if (!Json::parseFromStream(builder, file, &root, &errors)) {
            std::cerr << "[ScenarioSystem] JSON parse error: " << errors << std::endl;
            return false;
        }
        
        try {
            const Json::Value& scenario_json = root["scenario"];
            
            ScenarioData scenario;
            scenario.id = scenario_json["id"].asString();
            scenario.name = scenario_json["name"].asString();
            scenario.description = scenario_json["description"].asString();
            scenario.duration_days = scenario_json["duration_days"].asInt();
            
            // Parse triggers
            const Json::Value& triggers = scenario_json["triggers"];
            for (const auto& trigger_json : triggers) {
                ScenarioTrigger trigger;
                trigger.condition = trigger_json["condition"].asString();
                
                // Parse events in this trigger
                const Json::Value& events = trigger_json["events"];
                for (const auto& event_json : events) {
                    ScenarioEvent event;
                    event.type = ParseEventType(event_json["type"].asString());
                    event.target_system = event_json["target_system"].asString();
                    event.message = event_json["message"].asString();
                    
                    // Parse effects
                    const Json::Value& effects = event_json["effects"];
                    for (const auto& member_name : effects.getMemberNames()) {
                        ScenarioEffect effect;
                        effect.parameter = member_name;
                        effect.value = effects[member_name].asFloat();
                        effect.operation = "set"; // Default operation
                        event.effects.push_back(effect);
                    }
                    
                    trigger.events.push_back(event);
                }
                
                scenario.triggers.push_back(trigger);
            }
            
            // Parse completion conditions
            const Json::Value& completion = scenario_json["completion_conditions"];
            for (const auto& condition : completion) {
                scenario.completion_messages.push_back(condition["message"].asString());
            }
            
            loaded_scenarios.push_back(scenario);
            std::cout << "[ScenarioSystem] Loaded scenario: " << scenario.name << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "[ScenarioSystem] Error parsing scenario: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool ScenarioSystem::StartScenario(const std::string& scenario_id) {
        // Find the scenario
        for (auto& scenario : loaded_scenarios) {
            if (scenario.id == scenario_id) {
                if (active_scenario) {
                    StopCurrentScenario();
                }
                
                active_scenario = &scenario;
                active_scenario->current_day = 0;
                active_scenario->is_active = true;
                active_scenario->is_completed = false;
                
                // Reset all event execution states
                for (auto& trigger : active_scenario->triggers) {
                    for (auto& event : trigger.events) {
                        event.executed = false;
                    }
                }
                
                SendMessage("🎭 SCENARIO STARTED: " + active_scenario->name);
                SendMessage("📖 " + active_scenario->description);
                
                std::cout << "[ScenarioSystem] Started scenario: " << active_scenario->name << std::endl;
                return true;
            }
        }
        
        std::cerr << "[ScenarioSystem] Scenario not found: " << scenario_id << std::endl;
        return false;
    }
    
    void ScenarioSystem::StopCurrentScenario() {
        if (active_scenario) {
            SendMessage("🏁 SCENARIO ENDED: " + active_scenario->name);
            active_scenario->is_active = false;
            active_scenario = nullptr;
        }
    }
    
    void ScenarioSystem::Update(float delta_time) {
        // Scenarios are day-based, so no real-time updates needed
        // Day advancement happens via AdvanceDay()
    }
    
    void ScenarioSystem::AdvanceDay() {
        if (!active_scenario || !active_scenario->is_active || active_scenario->is_completed) {
            return;
        }
        
        active_scenario->current_day++;
        
        // Check for scenario completion
        if (active_scenario->current_day >= active_scenario->duration_days) {
            for (const auto& message : active_scenario->completion_messages) {
                SendMessage("🏆 " + message);
            }
            active_scenario->is_completed = true;
            active_scenario->is_active = false;
            active_scenario = nullptr;
            return;
        }
        
        // Process triggers for current day
        for (auto& trigger : active_scenario->triggers) {
            if (EvaluateCondition(trigger.condition)) {
                for (auto& event : trigger.events) {
                    if (!event.executed) {
                        ExecuteEvent(event);
                        event.executed = true;
                    }
                }
            }
        }
    }
    
    std::vector<std::string> ScenarioSystem::GetAvailableScenarios() const {
        std::vector<std::string> scenarios;
        for (const auto& scenario : loaded_scenarios) {
            scenarios.push_back(scenario.id + ": " + scenario.name);
        }
        return scenarios;
    }
    
    EventType ScenarioSystem::ParseEventType(const std::string& type_str) {
        if (type_str == "economic_shock") return EventType::ECONOMIC_SHOCK;
        if (type_str == "population_unrest") return EventType::POPULATION_UNREST;
        if (type_str == "military_budget_cut") return EventType::MILITARY_BUDGET_CUT;
        if (type_str == "administrative_response") return EventType::ADMINISTRATIVE_RESPONSE;
        if (type_str == "recovery_begins") return EventType::RECOVERY_BEGINS;
        if (type_str == "technology_breakthrough") return EventType::TECHNOLOGY_BREAKTHROUGH;
        if (type_str == "military_enhancement") return EventType::MILITARY_ENHANCEMENT;
        if (type_str == "diplomatic_tension") return EventType::DIPLOMATIC_TENSION;
        if (type_str == "economic_boost") return EventType::ECONOMIC_BOOST;
        if (type_str == "population_pride") return EventType::POPULATION_PRIDE;
        if (type_str == "administrative_adaptation") return EventType::ADMINISTRATIVE_ADAPTATION;
        return EventType::UNKNOWN;
    }
    
    bool ScenarioSystem::EvaluateCondition(const std::string& condition) {
        // Simple condition evaluation: "day >= X"
        std::regex day_regex(R"(day\s*>=\s*(\d+))");
        std::smatch match;
        
        if (std::regex_search(condition, match, day_regex)) {
            int required_day = std::stoi(match[1].str());
            return active_scenario->current_day >= required_day;
        }
        
        return false;
    }
    
    void ScenarioSystem::ExecuteEvent(ScenarioEvent& event) {
        SendMessage("⚡ " + event.message);
        
        // Apply all effects
        for (const auto& effect : event.effects) {
            ApplyEffect(event.target_system, effect);
        }
        
        LogEvent("Executed " + event.target_system + " event with " + 
                std::to_string(event.effects.size()) + " effects");
    }
    
    void ScenarioSystem::ApplyEffect(const std::string& target_system, const ScenarioEffect& effect) {
        if (target_system == "population") {
            ApplyPopulationEffect(effect);
        } else if (target_system == "economic") {
            ApplyEconomicEffect(effect);
        } else if (target_system == "military") {
            ApplyMilitaryEffect(effect);
        } else if (target_system == "technology") {
            ApplyTechnologyEffect(effect);
        } else if (target_system == "diplomacy") {
            ApplyDiplomacyEffect(effect);
        } else if (target_system == "administrative") {
            ApplyAdministrativeEffect(effect);
        }
    }
    
    void ScenarioSystem::ApplyPopulationEffect(const ScenarioEffect& effect) {
        std::cout << "[ScenarioSystem] Applied population effect: " << effect.parameter 
                  << " = " << effect.value << std::endl;
        // Note: In a full implementation, these would modify actual component values
        // For this demo, we're logging the intended effects
    }
    
    void ScenarioSystem::ApplyEconomicEffect(const ScenarioEffect& effect) {
        std::cout << "[ScenarioSystem] Applied economic effect: " << effect.parameter 
                  << " = " << effect.value << std::endl;
    }
    
    void ScenarioSystem::ApplyMilitaryEffect(const ScenarioEffect& effect) {
        std::cout << "[ScenarioSystem] Applied military effect: " << effect.parameter 
                  << " = " << effect.value << std::endl;
    }
    
    void ScenarioSystem::ApplyTechnologyEffect(const ScenarioEffect& effect) {
        std::cout << "[ScenarioSystem] Applied technology effect: " << effect.parameter 
                  << " = " << effect.value << std::endl;
    }
    
    void ScenarioSystem::ApplyDiplomacyEffect(const ScenarioEffect& effect) {
        std::cout << "[ScenarioSystem] Applied diplomacy effect: " << effect.parameter 
                  << " = " << effect.value << std::endl;
    }
    
    void ScenarioSystem::ApplyAdministrativeEffect(const ScenarioEffect& effect) {
        std::cout << "[ScenarioSystem] Applied administrative effect: " << effect.parameter 
                  << " = " << effect.value << std::endl;
    }
    
    void ScenarioSystem::SendMessage(const std::string& message) {
        recent_messages.push_back(message);
        
        // Keep only last 10 messages
        if (recent_messages.size() > 10) {
            recent_messages.erase(recent_messages.begin());
        }
        
        std::cout << "[ScenarioSystem] " << message << std::endl;
        
        if (message_callback) {
            message_callback(message);
        }
    }
    
    void ScenarioSystem::LogEvent(const std::string& event_description) {
        std::cout << "[ScenarioSystem] Event: " << event_description << std::endl;
    }

} // namespace game::scenario