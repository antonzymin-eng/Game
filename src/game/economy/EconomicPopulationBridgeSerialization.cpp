// Created: September 27, 2025 - 15:00 PST
// Location: src/game/economic/EconomicPopulationBridgeSerialization.cpp
// Economic-Population Bridge System - Serialization Implementation

#include "../../../include/game/economy/EconomicPopulationBridge.h"
#include <iostream>

namespace mechanica {
namespace integration {

// ============================================================================
// Serialization Methods
// ============================================================================

Json::Value EconomicPopulationBridge::Serialize(int version) const {
    Json::Value root;
    
    root["system_name"] = GetSystemName();
    root["version"] = version;
    
    // Serialize configuration
    Json::Value config;
    config["bridge_update_interval"] = m_config.bridge_update_interval;
    config["tax_happiness_base_effect"] = m_config.tax_happiness_base_effect;
    config["tax_happiness_scaling"] = m_config.tax_happiness_scaling;
    config["unemployment_happiness_penalty"] = m_config.unemployment_happiness_penalty;
    config["wage_happiness_scaling"] = m_config.wage_happiness_scaling;
    config["inequality_threshold"] = m_config.inequality_threshold;
    config["inequality_happiness_penalty"] = m_config.inequality_happiness_penalty;
    config["literacy_productivity_bonus"] = m_config.literacy_productivity_bonus;
    config["happiness_productivity_scaling"] = m_config.happiness_productivity_scaling;
    config["economic_output_crisis_threshold"] = m_config.economic_output_crisis_threshold;
    config["happiness_crisis_threshold"] = m_config.happiness_crisis_threshold;
    config["default_tax_rate"] = m_config.default_tax_rate;
    config["default_wages"] = m_config.default_wages;
    config["default_infrastructure_quality"] = m_config.default_infrastructure_quality;
    config["default_inflation_rate"] = m_config.default_inflation_rate;
    config["default_economic_growth"] = m_config.default_economic_growth;
    config["taxable_population_ratio"] = m_config.taxable_population_ratio;
    config["consumer_spending_multiplier"] = m_config.consumer_spending_multiplier;
    config["luxury_wealth_threshold"] = m_config.luxury_wealth_threshold;
    config["luxury_demand_multiplier"] = m_config.luxury_demand_multiplier;
    config["tax_collection_literacy_base"] = m_config.tax_collection_literacy_base;
    config["tax_collection_literacy_bonus"] = m_config.tax_collection_literacy_bonus;
    config["tax_collection_happiness_base"] = m_config.tax_collection_happiness_base;
    config["tax_collection_happiness_bonus"] = m_config.tax_collection_happiness_bonus;
    config["infrastructure_good_threshold"] = m_config.infrastructure_good_threshold;
    config["infrastructure_capacity_bonus"] = m_config.infrastructure_capacity_bonus;
    config["wealth_increase_trade_multiplier"] = m_config.wealth_increase_trade_multiplier;
    config["crisis_severity_increase"] = m_config.crisis_severity_increase;
    config["crisis_severity_decrease"] = m_config.crisis_severity_decrease;
    config["crisis_reset_threshold"] = m_config.crisis_reset_threshold;
    config["employment_crisis_threshold"] = m_config.employment_crisis_threshold;
    config["tax_efficiency_crisis_threshold"] = m_config.tax_efficiency_crisis_threshold;
    config["happiness_baseline"] = m_config.happiness_baseline;
    config["wealth_normalization"] = m_config.wealth_normalization;
    config["max_history_size"] = m_config.max_history_size;
    config["performance_log_interval"] = m_config.performance_log_interval;
    
    root["config"] = config;
    
    // Serialize performance metrics
    root["updates_this_frame"] = m_updates_this_frame.load();
    root["last_performance_log"] = m_last_performance_log.load();
    
    // Serialize entity bridge components (if entity manager available)
    if (m_entity_manager) {
        Json::Value entities_array(Json::arrayValue);
        
        auto entities_with_bridge = m_entity_manager->GetEntitiesWithComponent<EconomicPopulationBridgeComponent>();
        for (auto entity_id : entities_with_bridge) {
            auto bridge_comp = m_entity_manager->GetComponent<EconomicPopulationBridgeComponent>(entity_id);
            if (bridge_comp) {
                Json::Value entity_data;
                entity_data["entity_id"] = static_cast<int>(entity_id.id);
                
                // Economic effects
                Json::Value effects;
                effects["tax_rate"] = bridge_comp->economic_effects.tax_rate;
                effects["tax_happiness_modifier"] = bridge_comp->economic_effects.tax_happiness_modifier;
                effects["employment_rate"] = bridge_comp->economic_effects.employment_rate;
                effects["average_wages"] = bridge_comp->economic_effects.average_wages;
                effects["wealth_inequality"] = bridge_comp->economic_effects.wealth_inequality;
                effects["trade_income_per_capita"] = bridge_comp->economic_effects.trade_income_per_capita;
                effects["infrastructure_quality"] = bridge_comp->economic_effects.infrastructure_quality;
                effects["public_investment"] = bridge_comp->economic_effects.public_investment;
                effects["inflation_rate"] = bridge_comp->economic_effects.inflation_rate;
                effects["economic_growth"] = bridge_comp->economic_effects.economic_growth;
                entity_data["economic_effects"] = effects;
                
                // Population contributions
                Json::Value contributions;
                contributions["total_workers"] = bridge_comp->population_contributions.total_workers;
                contributions["skilled_worker_ratio"] = bridge_comp->population_contributions.skilled_worker_ratio;
                contributions["literacy_rate"] = bridge_comp->population_contributions.literacy_rate;
                contributions["taxable_population"] = bridge_comp->population_contributions.taxable_population;
                contributions["tax_collection_efficiency"] = bridge_comp->population_contributions.tax_collection_efficiency;
                contributions["consumer_spending"] = bridge_comp->population_contributions.consumer_spending;
                contributions["luxury_demand"] = bridge_comp->population_contributions.luxury_demand;
                contributions["innovation_factor"] = bridge_comp->population_contributions.innovation_factor;
                contributions["productivity_modifier"] = bridge_comp->population_contributions.productivity_modifier;
                entity_data["population_contributions"] = contributions;
                
                // History
                Json::Value happiness_history(Json::arrayValue);
                for (double val : bridge_comp->happiness_history) {
                    happiness_history.append(val);
                }
                entity_data["happiness_history"] = happiness_history;
                
                Json::Value output_history(Json::arrayValue);
                for (double val : bridge_comp->economic_output_history) {
                    output_history.append(val);
                }
                entity_data["economic_output_history"] = output_history;
                
                // State
                entity_data["economic_population_balance"] = bridge_comp->economic_population_balance;
                entity_data["economic_crisis"] = bridge_comp->economic_crisis;
                entity_data["population_unrest"] = bridge_comp->population_unrest;
                entity_data["crisis_severity"] = bridge_comp->crisis_severity;
                entity_data["last_update_time"] = bridge_comp->last_update_time;
                
                entities_array.append(entity_data);
            }
        }
        
        root["entities"] = entities_array;
    }
    
    std::cout << "EconomicPopulationBridge: Serialization complete (version " << version << ")" << std::endl;
    
    return root;
}

bool EconomicPopulationBridge::Deserialize(const Json::Value& data, int version) {
    try {
        // Verify system name
        if (data["system_name"].asString() != GetSystemName()) {
            std::cerr << "EconomicPopulationBridge: System name mismatch in save data" << std::endl;
            return false;
        }
        
        // Load configuration
        if (data.isMember("config")) {
            const Json::Value& config = data["config"];
            
            m_config.bridge_update_interval = config.get("bridge_update_interval", 1.0).asDouble();
            m_config.tax_happiness_base_effect = config.get("tax_happiness_base_effect", -0.5).asDouble();
            m_config.tax_happiness_scaling = config.get("tax_happiness_scaling", -0.3).asDouble();
            m_config.unemployment_happiness_penalty = config.get("unemployment_happiness_penalty", -0.3).asDouble();
            m_config.wage_happiness_scaling = config.get("wage_happiness_scaling", 0.2).asDouble();
            m_config.inequality_threshold = config.get("inequality_threshold", 0.4).asDouble();
            m_config.inequality_happiness_penalty = config.get("inequality_happiness_penalty", -0.4).asDouble();
            m_config.literacy_productivity_bonus = config.get("literacy_productivity_bonus", 0.3).asDouble();
            m_config.happiness_productivity_scaling = config.get("happiness_productivity_scaling", 0.2).asDouble();
            m_config.economic_output_crisis_threshold = config.get("economic_output_crisis_threshold", 0.3).asDouble();
            m_config.happiness_crisis_threshold = config.get("happiness_crisis_threshold", 0.3).asDouble();
            m_config.default_tax_rate = config.get("default_tax_rate", 0.15).asDouble();
            m_config.default_wages = config.get("default_wages", 50.0).asDouble();
            m_config.default_infrastructure_quality = config.get("default_infrastructure_quality", 0.6).asDouble();
            m_config.default_inflation_rate = config.get("default_inflation_rate", 0.02).asDouble();
            m_config.default_economic_growth = config.get("default_economic_growth", 0.03).asDouble();
            m_config.taxable_population_ratio = config.get("taxable_population_ratio", 0.8).asDouble();
            m_config.consumer_spending_multiplier = config.get("consumer_spending_multiplier", 0.6).asDouble();
            m_config.luxury_wealth_threshold = config.get("luxury_wealth_threshold", 50.0).asDouble();
            m_config.luxury_demand_multiplier = config.get("luxury_demand_multiplier", 0.1).asDouble();
            m_config.tax_collection_literacy_base = config.get("tax_collection_literacy_base", 0.5).asDouble();
            m_config.tax_collection_literacy_bonus = config.get("tax_collection_literacy_bonus", 0.4).asDouble();
            m_config.tax_collection_happiness_base = config.get("tax_collection_happiness_base", 0.7).asDouble();
            m_config.tax_collection_happiness_bonus = config.get("tax_collection_happiness_bonus", 0.3).asDouble();
            m_config.infrastructure_good_threshold = config.get("infrastructure_good_threshold", 0.7).asDouble();
            m_config.infrastructure_capacity_bonus = config.get("infrastructure_capacity_bonus", 0.5).asDouble();
            m_config.wealth_increase_trade_multiplier = config.get("wealth_increase_trade_multiplier", 0.1).asDouble();
            m_config.crisis_severity_increase = config.get("crisis_severity_increase", 0.1).asDouble();
            m_config.crisis_severity_decrease = config.get("crisis_severity_decrease", 0.05).asDouble();
            m_config.crisis_reset_threshold = config.get("crisis_reset_threshold", 0.1).asDouble();
            m_config.employment_crisis_threshold = config.get("employment_crisis_threshold", 0.6).asDouble();
            m_config.tax_efficiency_crisis_threshold = config.get("tax_efficiency_crisis_threshold", 0.5).asDouble();
            m_config.happiness_baseline = config.get("happiness_baseline", 0.5).asDouble();
            m_config.wealth_normalization = config.get("wealth_normalization", 100.0).asDouble();
            m_config.max_history_size = config.get("max_history_size", 12).asInt();
            m_config.performance_log_interval = config.get("performance_log_interval", 10.0).asDouble();
        }
        
        // Load performance metrics
        m_updates_this_frame.store(data.get("updates_this_frame", 0).asInt());
        m_last_performance_log.store(data.get("last_performance_log", 0.0).asDouble());
        
        // Load entity bridge components
        if (m_entity_manager && data.isMember("entities")) {
            const Json::Value& entities = data["entities"];
            
            for (const auto& entity_data : entities) {
                core::ecs::EntityID entity_id(entity_data["entity_id"].asUInt());
                
                auto bridge_comp = m_entity_manager->GetComponent<EconomicPopulationBridgeComponent>(entity_id);
                if (!bridge_comp) {
                    bridge_comp = m_entity_manager->AddComponent<EconomicPopulationBridgeComponent>(entity_id);
                }
                
                // Load economic effects
                const Json::Value& effects = entity_data["economic_effects"];
                bridge_comp->economic_effects.tax_rate = effects.get("tax_rate", 0.0).asDouble();
                bridge_comp->economic_effects.tax_happiness_modifier = effects.get("tax_happiness_modifier", 0.0).asDouble();
                bridge_comp->economic_effects.employment_rate = effects.get("employment_rate", 0.0).asDouble();
                bridge_comp->economic_effects.average_wages = effects.get("average_wages", 0.0).asDouble();
                bridge_comp->economic_effects.wealth_inequality = effects.get("wealth_inequality", 0.0).asDouble();
                bridge_comp->economic_effects.trade_income_per_capita = effects.get("trade_income_per_capita", 0.0).asDouble();
                bridge_comp->economic_effects.infrastructure_quality = effects.get("infrastructure_quality", 0.0).asDouble();
                bridge_comp->economic_effects.public_investment = effects.get("public_investment", 0.0).asDouble();
                bridge_comp->economic_effects.inflation_rate = effects.get("inflation_rate", 0.0).asDouble();
                bridge_comp->economic_effects.economic_growth = effects.get("economic_growth", 0.0).asDouble();
                
                // Load population contributions
                const Json::Value& contributions = entity_data["population_contributions"];
                bridge_comp->population_contributions.total_workers = contributions.get("total_workers", 0.0).asDouble();
                bridge_comp->population_contributions.skilled_worker_ratio = contributions.get("skilled_worker_ratio", 0.0).asDouble();
                bridge_comp->population_contributions.literacy_rate = contributions.get("literacy_rate", 0.0).asDouble();
                bridge_comp->population_contributions.taxable_population = contributions.get("taxable_population", 0.0).asDouble();
                bridge_comp->population_contributions.tax_collection_efficiency = contributions.get("tax_collection_efficiency", 0.0).asDouble();
                bridge_comp->population_contributions.consumer_spending = contributions.get("consumer_spending", 0.0).asDouble();
                bridge_comp->population_contributions.luxury_demand = contributions.get("luxury_demand", 0.0).asDouble();
                bridge_comp->population_contributions.innovation_factor = contributions.get("innovation_factor", 0.0).asDouble();
                bridge_comp->population_contributions.productivity_modifier = contributions.get("productivity_modifier", 0.0).asDouble();
                
                // Load history
                bridge_comp->happiness_history.clear();
                const Json::Value& happiness_history = entity_data["happiness_history"];
                for (const auto& val : happiness_history) {
                    bridge_comp->happiness_history.push_back(val.asDouble());
                }
                
                bridge_comp->economic_output_history.clear();
                const Json::Value& output_history = entity_data["economic_output_history"];
                for (const auto& val : output_history) {
                    bridge_comp->economic_output_history.push_back(val.asDouble());
                }
                
                // Load state
                bridge_comp->economic_population_balance = entity_data.get("economic_population_balance", 0.5).asDouble();
                bridge_comp->economic_crisis = entity_data.get("economic_crisis", false).asBool();
                bridge_comp->population_unrest = entity_data.get("population_unrest", false).asBool();
                bridge_comp->crisis_severity = entity_data.get("crisis_severity", 0.0).asDouble();
                bridge_comp->last_update_time = entity_data.get("last_update_time", 0.0).asDouble();
            }
        }
        
        std::cout << "EconomicPopulationBridge: Deserialization complete (version " << version << ")" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "EconomicPopulationBridge: Deserialization failed: " << e.what() << std::endl;
        return false;
    }
}

} // namespace integration
} // namespace mechanica
