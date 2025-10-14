#include "game/config/ConfigHelpers.h"
#include "game/config/GameConfig.h"
#include "core/logging/Logger.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>

namespace game::config::helpers {

    void GenerateDefaultConfigurations() {
        std::string config_dir = "config";
        
        if (!EnsureConfigDirectoryExists(config_dir)) {
            throw std::runtime_error("Failed to create config directory");
        }
        
        if (!CreateDefaultGameConfig(config_dir)) {
            throw std::runtime_error("Failed to create default GameConfig.json");
        }
        
        std::cout << "[ConfigHelpers] Default configuration files generated in: " << config_dir << std::endl;
    }

    bool CreateDefaultGameConfig(const std::string& config_directory) {
        std::string config_file = config_directory + "/GameConfig.json";
        
        // Check if file already exists
        if (std::filesystem::exists(config_file)) {
            std::cout << "[ConfigHelpers] Configuration file already exists: " << config_file << std::endl;
            return true;
        }
        
        std::ofstream file(config_file);
        if (!file.is_open()) {
            std::cerr << "[ConfigHelpers] Failed to create config file: " << config_file << std::endl;
            return false;
        }
        
        file << GetDefaultConfigContent();
        file.close();
        
        std::cout << "[ConfigHelpers] Created default config file: " << config_file << std::endl;
        return true;
    }

    ::core::threading::ThreadingStrategy GetThreadingStrategyForSystem(const std::string& system_name) {
        // Define optimal threading strategies for different systems
        static const std::unordered_map<std::string, ::core::threading::ThreadingStrategy> system_strategies = {
            {"PopulationSystem", ::core::threading::ThreadingStrategy::THREAD_POOL},
            {"TechnologySystem", ::core::threading::ThreadingStrategy::BACKGROUND_THREAD},
            {"CoreGameplaySystem", ::core::threading::ThreadingStrategy::MAIN_THREAD},
            {"TimeManagementSystem", ::core::threading::ThreadingStrategy::MAIN_THREAD},
            {"EconomicSystem", ::core::threading::ThreadingStrategy::THREAD_POOL},
            {"MilitarySystem", ::core::threading::ThreadingStrategy::THREAD_POOL},
            {"AdministrativeSystem", ::core::threading::ThreadingStrategy::THREAD_POOL},
            {"DiplomacySystem", ::core::threading::ThreadingStrategy::BACKGROUND_THREAD},
            {"AISystem", ::core::threading::ThreadingStrategy::DEDICATED_THREAD}
        };
        
        auto it = system_strategies.find(system_name);
        if (it != system_strategies.end()) {
            return it->second;
        }
        
        // Default strategy for unknown systems
        return ::core::threading::ThreadingStrategy::MAIN_THREAD;
    }

    std::string GetThreadingRationale(const std::string& system_name) {
        static const std::unordered_map<std::string, std::string> system_rationales = {
            {"PopulationSystem", "High computation load with parallelizable population calculations"},
            {"TechnologySystem", "Research calculations can run in background without blocking gameplay"},
            {"CoreGameplaySystem", "Core game logic must run on main thread for deterministic behavior"},
            {"TimeManagementSystem", "Time progression requires main thread synchronization"},
            {"EconomicSystem", "Trade and economic calculations benefit from parallel processing"},
            {"MilitarySystem", "Combat calculations can be parallelized across units"},
            {"AdministrativeSystem", "Administrative tasks can be processed in parallel"},
            {"DiplomacySystem", "AI diplomacy calculations can run in background"},
            {"AISystem", "AI decision making benefits from dedicated processing thread"}
        };
        
        auto it = system_rationales.find(system_name);
        if (it != system_rationales.end()) {
            return it->second;
        }
        
        return "Default main thread execution for unknown system";
    }

    bool EnsureConfigDirectoryExists(const std::string& config_directory) {
        try {
            if (!std::filesystem::exists(config_directory)) {
                return std::filesystem::create_directories(config_directory);
            }
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "[ConfigHelpers] Failed to create directory " << config_directory << ": " << e.what() << std::endl;
            return false;
        }
    }

    std::string GetDefaultConfigContent() {
        return R"({
  "config_version": 1,
  "last_updated": "2025-10-13",
  
  "diplomacy": {
    "non_aggression_duration_years": 5,
    "trade_agreement_duration_years": 20,
    "alliance_duration_years": 25,
    "default_treaty_duration_years": 10,
    "treaty_compliance_threshold": 0.5,
    "marriage_base_bonus": 20.0,
    "max_opinion": 100,
    "min_opinion": -100,
    "friendly_threshold": 80,
    "neutral_threshold": 20,
    "hostile_threshold": -50
  },
  
  "economy": {
    "starting_treasury": 1000,
    "base_monthly_expenses": 50,
    "stability_min_modifier": 0.5,
    "stability_range_modifier": 0.5,
    "random_event_chance_percent": 5,
    "max_concurrent_events": 3,
    "good_admin_threshold": 0.7,
    "poor_admin_threshold": 0.3
  },
  
  "population": {
    "base_growth_rate": 0.01,
    "happiness_growth_modifier": 0.5,
    "famine_threshold": 0.3,
    "plague_base_chance": 0.02,
    "initial_population": {
      "HIGH_NOBILITY": 50,
      "LESSER_NOBILITY": 200,
      "HIGH_CLERGY": 30,
      "CLERGY": 100,
      "WEALTHY_MERCHANTS": 150,
      "BURGHERS": 800,
      "GUILD_MASTERS": 300,
      "CRAFTSMEN": 1200,
      "SCHOLARS": 100,
      "FREE_PEASANTS": 5000,
      "VILLEINS": 3000,
      "SERFS": 2000,
      "URBAN_LABORERS": 1500,
      "FOREIGNERS": 200,
      "RELIGIOUS_ORDERS": 80
    }
  },
  
  "technology": {
    "research_base_cost": 1000,
    "research_speed_modifier": 1.0,
    "tech_cost_scaling": 1.5
  },
  
  "administration": {
    "efficiency_base": 0.5,
    "corruption_decay_rate": 0.01,
    "official_salary_base": 50
  },

  "council": {
    "default_delegation_level": 0.6,
    "max_council_members": 12,
    "decision_threshold": 0.6
  },
  
  "threading": {
    "worker_thread_count": 4,
    "max_systems_per_frame": 10,
    "frame_budget_ms": 16.67,
    "performance_monitoring": true
  },
  
  "debug": {
    "enable_logging": true,
    "log_level": "INFO",
    "performance_monitoring": true,
    "save_every_n_months": 12
  },
  
  "hot_reload": {
    "enabled": true,
    "check_interval_seconds": 1.0
  }
})";
    }

} // namespace game::config::helpers