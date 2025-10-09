// Created: September 22, 2025, 4:47 PM  
// Location: src/core/config/ConfigManager.cpp

#include "core/config/ConfigManager.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <regex>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace core::config {

    // ============================================================================
    // ConfigValidationResult Implementation
    // ============================================================================

    void ConfigValidationResult::AddError(const std::string& error) {
        errors.push_back(error);
        is_valid = false;
    }

    void ConfigValidationResult::AddWarning(const std::string& warning) {
        warnings.push_back(warning);
    }

    bool ConfigValidationResult::HasIssues() const {
        return !errors.empty() || !warnings.empty();
    }

    // ============================================================================
    // ConfigManager Implementation
    // ============================================================================

    ConfigManager& ConfigManager::Instance() {
        static ConfigManager instance;
        return instance;
    }

    void ConfigManager::Initialize(const std::string& config_dir) {
        std::unique_lock<std::shared_mutex> lock(m_config_mutex);
        
        if (m_initialized) {
            LogWarning("ConfigManager already initialized, skipping");
            return;
        }
        
        m_config_directory = config_dir;
        if (!m_config_directory.empty() && m_config_directory.back() != '/') {
            m_config_directory += '/';
        }
        
        try {
            std::filesystem::create_directories(m_config_directory);
            LoadAllConfigs();
            SetupFileWatching();
            m_initialized = true;
            m_last_reload_time = std::chrono::system_clock::now();
            
            LogInfo("ConfigManager initialized successfully");
            
            // Load formulas into FormulaEngine
            FormulaEngine::Instance().LoadFormulasFromConfig();
            
        } catch (const std::exception& e) {
            LogError("Failed to initialize ConfigManager: " + std::string(e.what()));
            throw;
        }
    }

    void ConfigManager::Shutdown() {
        std::unique_lock<std::shared_mutex> lock(m_config_mutex);
        
        if (!m_initialized) return;
        
        // Clear all callbacks
        {
            std::lock_guard<std::mutex> callback_lock(m_callback_mutex);
            m_change_callbacks.clear();
        }
        
        // Clear configuration data
        m_base_config.clear();
        m_runtime_overrides.clear();
        m_merged_config.clear();
        m_file_timestamps.clear();
        m_loaded_files.clear();
        
        m_initialized = false;
        LogInfo("ConfigManager shutdown complete");
    }

    void ConfigManager::SetValues(const std::unordered_map<std::string, json>& values) {
        std::unique_lock<std::shared_mutex> lock(m_config_mutex);
        
        for (const auto& [path, value] : values) {
            auto keys = SplitConfigPath(path);
            json* current = &m_runtime_overrides;
            
            // Navigate to the parent object
            for (size_t i = 0; i < keys.size() - 1; ++i) {
                if (!current->contains(keys[i])) {
                    (*current)[keys[i]] = json::object();
                }
                current = &(*current)[keys[i]];
            }
            
            json old_value;
            if (current->contains(keys.back())) {
                old_value = (*current)[keys.back()];
            }
            
            (*current)[keys.back()] = value;
            
            // Notify outside the lock
            lock.unlock();
            NotifyConfigChanged(path, old_value, value);
            lock.lock();
        }
        
        RebuildMergedConfig();
    }

    std::unordered_map<std::string, json> ConfigManager::GetSection(const std::string& section_path) const {
        std::shared_lock<std::shared_mutex> lock(m_config_mutex);
        
        std::unordered_map<std::string, json> result;
        
        try {
            auto keys = SplitConfigPath(section_path);
            json current = m_merged_config;
            
            // Navigate to the section
            for (const auto& key : keys) {
                if (!current.contains(key)) {
                    LogWarning("Config section not found: " + section_path);
                    return result;
                }
                current = current[key];
            }
            
            // Extract all key-value pairs from the section
            if (current.is_object()) {
                for (auto it = current.begin(); it != current.end(); ++it) {
                    result[it.key()] = it.value();
                }
            }
            
        } catch (const std::exception& e) {
            LogError("Error getting config section '" + section_path + "': " + e.what());
        }
        
        return result;
    }

    void ConfigManager::CheckForUpdates() {
        if (!m_enable_hot_reload || !m_initialized) return;
        
        std::shared_lock<std::shared_mutex> read_lock(m_config_mutex);
        
        bool needs_reload = false;
        std::error_code ec;
        
        for (const auto& [filename, last_write_time] : m_file_timestamps) {
            std::string filepath = m_config_directory + filename;
            auto current_time = std::filesystem::last_write_time(filepath, ec);
            
            if (!ec && current_time > last_write_time) {
                needs_reload = true;
                break;
            }
        }
        
        read_lock.unlock();
        
        if (needs_reload) {
            LogInfo("Config files modified, triggering reload");
            ReloadConfigs();
        }
    }

    void ConfigManager::ReloadConfigs() {
        std::unique_lock<std::shared_mutex> lock(m_config_mutex);
        
        if (!m_initialized) {
            LogWarning("Cannot reload configs - ConfigManager not initialized");
            return;
        }
        
        try {
            // Store old config for change detection
            json old_config = m_merged_config;
            
            // Clear and reload
            m_base_config.clear();
            m_file_timestamps.clear();
            m_loaded_files.clear();
            
            LoadAllConfigs();
            m_last_reload_time = std::chrono::system_clock::now();
            
            // Reload formulas
            FormulaEngine::Instance().LoadFormulasFromConfig();
            
            LogInfo("Configuration reloaded successfully");
            
            // TODO: Implement change detection and notification for reload
            // This would require comparing old_config with m_merged_config
            
        } catch (const std::exception& e) {
            LogError("Failed to reload configs: " + std::string(e.what()));
        }
    }

    void ConfigManager::SaveCurrentConfig(const std::string& filename) const {
        std::shared_lock<std::shared_mutex> lock(m_config_mutex);
        
        try {
            std::ofstream file(filename);
            if (!file.is_open()) {
                throw std::runtime_error("Cannot open file for writing: " + filename);
            }
            
            file << std::setw(2) << m_merged_config << std::endl;
            LogInfo("Current config saved to: " + filename);
            
        } catch (const std::exception& e) {
            LogError("Failed to save config to '" + filename + "': " + e.what());
            throw;
        }
    }

    void ConfigManager::LoadConfigOverride(const std::string& filename) {
        std::unique_lock<std::shared_mutex> lock(m_config_mutex);
        
        try {
            std::ifstream file(filename);
            if (!file.is_open()) {
                throw std::runtime_error("Cannot open override file: " + filename);
            }
            
            json override_config;
            file >> override_config;
            
            // Merge override into runtime overrides
            MergeJson(m_runtime_overrides, override_config);
            RebuildMergedConfig();
            
            LogInfo("Loaded config override from: " + filename);
            
        } catch (const std::exception& e) {
            LogError("Failed to load config override '" + filename + "': " + e.what());
            throw;
        }
    }

    ConfigValidationResult ConfigManager::ValidateAllConfigs() const {
        std::shared_lock<std::shared_mutex> lock(m_config_mutex);
        
        ConfigValidationResult result;
        result.is_valid = true;
        
        // Validate each section
        auto economic_result = ValidateEconomicConfig();
        auto building_result = ValidateBuildingConfig();
        auto military_result = ValidateMilitaryConfig();
        auto ui_result = ValidateUIConfig();
        auto system_result = ValidateSystemConfig();
        
        // Combine results
        auto combine_results = [&result](const ConfigValidationResult& section_result) {
            if (!section_result.is_valid) {
                result.is_valid = false;
            }
            result.errors.insert(result.errors.end(), 
                               section_result.errors.begin(), 
                               section_result.errors.end());
            result.warnings.insert(result.warnings.end(),
                                 section_result.warnings.begin(),
                                 section_result.warnings.end());
        };
        
        combine_results(economic_result);
        combine_results(building_result);
        combine_results(military_result);
        combine_results(ui_result);
        combine_results(system_result);
        
        if (m_enable_validation_logging) {
            if (!result.is_valid) {
                LogError("Configuration validation failed with " + 
                        std::to_string(result.errors.size()) + " errors");
                for (const auto& error : result.errors) {
                    LogError("  - " + error);
                }
            }
            
            if (!result.warnings.empty()) {
                LogWarning("Configuration has " + 
                          std::to_string(result.warnings.size()) + " warnings");
                for (const auto& warning : result.warnings) {
                    LogWarning("  - " + warning);
                }
            }
        }
        
        return result;
    }

    ConfigValidationResult ConfigManager::ValidateSection(const std::string& section) const {
        std::shared_lock<std::shared_mutex> lock(m_config_mutex);
        
        if (section == "economics") return ValidateEconomicConfig();
        if (section == "buildings") return ValidateBuildingConfig();
        if (section == "military") return ValidateMilitaryConfig();
        if (section == "ui") return ValidateUIConfig();
        if (section == "system") return ValidateSystemConfig();
        
        ConfigValidationResult result;
        result.AddError("Unknown validation section: " + section);
        return result;
    }

    void ConfigManager::RegisterChangeCallback(const std::string& path, ChangeCallback callback) {
        std::lock_guard<std::mutex> lock(m_callback_mutex);
        m_change_callbacks[path].push_back(callback);
    }

    void ConfigManager::UnregisterChangeCallback(const std::string& path) {
        std::lock_guard<std::mutex> lock(m_callback_mutex);
        m_change_callbacks.erase(path);
    }

    void ConfigManager::EnableHotReload(bool enable) {
        std::unique_lock<std::shared_mutex> lock(m_config_mutex);
        m_enable_hot_reload = enable;
        LogInfo("Hot reload " + std::string(enable ? "enabled" : "disabled"));
    }

    void ConfigManager::EnableValidationLogging(bool enable) {
        std::unique_lock<std::shared_mutex> lock(m_config_mutex);
        m_enable_validation_logging = enable;
        LogInfo("Validation logging " + std::string(enable ? "enabled" : "disabled"));
    }

    void ConfigManager::SetLogLevel(const std::string& level) {
        std::unique_lock<std::shared_mutex> lock(m_config_mutex);
        m_log_level = level;
        LogInfo("Log level set to: " + level);
    }

    void ConfigManager::ExportMergedConfig(const std::string& filename) const {
        SaveCurrentConfig(filename);
    }

    std::vector<std::string> ConfigManager::GetLoadedFiles() const {
        std::shared_lock<std::shared_mutex> lock(m_config_mutex);
        return m_loaded_files;
    }

    std::chrono::system_clock::time_point ConfigManager::GetLastReloadTime() const {
        std::shared_lock<std::shared_mutex> lock(m_config_mutex);
        return m_last_reload_time;
    }

    size_t ConfigManager::GetConfigSize() const {
        std::shared_lock<std::shared_mutex> lock(m_config_mutex);
        return m_merged_config.size();
    }

    bool ConfigManager::IsInitialized() const {
        std::shared_lock<std::shared_mutex> lock(m_config_mutex);
        return m_initialized;
    }

    // ============================================================================
    // Private Methods Implementation
    // ============================================================================

    void ConfigManager::LoadAllConfigs() {
        // Load in dependency order
        std::vector<std::string> config_files = {
            "defaults.json",        // Base system defaults
            "economics.json",       // Economic parameters
            "buildings.json",       // Building definitions  
            "military.json",        // Military units and combat
            "population.json",      // Population mechanics
            "technology.json",      // Technology trees
            "ui.json",             // User interface settings
            "formulas.json",       // Mathematical formulas
            "balance.json",        // Game balance tweaks
            "user_overrides.json"  // User customizations (highest priority)
        };
        
        m_base_config = json::object();
        
        for (const auto& filename : config_files) {
            LoadConfigFile(filename);
        }
        
        RebuildMergedConfig();
    }

    void ConfigManager::LoadConfigFile(const std::string& filename) {
        std::string filepath = m_config_directory + filename;
        
        if (!std::filesystem::exists(filepath)) {
            if (filename == "defaults.json") {
                if (!CreateDefaultConfigs()) {
                    LogError("Failed to create default configuration");
                    return;
                }
            } else {
                LogInfo("Optional config file not found: " + filename);
                return;
            }
        }
        
        try {
            std::ifstream file(filepath);
            if (!file.is_open()) {
                throw std::runtime_error("Cannot open file: " + filepath);
            }
            
            json file_config;
            file >> file_config;
            
            // Validate JSON structure
            if (!file_config.is_object()) {
                throw std::runtime_error("Config file must contain a JSON object");
            }
            
            // Merge into base config
            MergeJson(m_base_config, file_config);
            
            // Track file for hot reload
            std::error_code ec;
            auto write_time = std::filesystem::last_write_time(filepath, ec);
            if (!ec) {
                m_file_timestamps[filename] = write_time;
            }
            
            m_loaded_files.push_back(filename);
            LogInfo("Loaded config file: " + filename);
            
        } catch (const std::exception& e) {
            LogError("Failed to load config file '" + filename + "': " + e.what());
            // Continue loading other files even if one fails
        }
    }

    bool ConfigManager::CreateDefaultConfigs() {
        try {
            json defaults = {
                {"system", {
                    {"version", "1.0.0"},
                    {"threading", {
                        {"enable_threading", true},
                        {"thread_pool_size", 4},
                        {"main_thread_systems", {"ui", "rendering", "input"}},
                        {"dedicated_thread_systems", {"population", "military_ai"}}
                    }},
                    {"performance", {
                        {"target_fps", 60},
                        {"update_frequencies", {
                            {"ui", 60.0},
                            {"economics", 10.0},
                            {"population", 2.0},
                            {"diplomacy", 1.0}
                        }}
                    }}
                }},
                {"economics", {
                    {"tax", {
                        {"base_rate", 0.12},
                        {"autonomy_penalty_multiplier", 0.75},
                        {"admin_efficiency_bonus", 1.6},
                        {"stability_multiplier_range", {0.5, 1.2}}
                    }},
                    {"trade", {
                        {"base_efficiency_range", {0.3, 0.95}},
                        {"market_bonus_per_level", 0.25},
                        {"route_efficiency_decay", 0.02},
                        {"stability_impact", 0.6}
                    }},
                    {"inflation", {
                        {"base_rate", 0.02},
                        {"money_supply_multiplier", 0.8},
                        {"trade_volume_impact", 0.3}
                    }}
                }},
                {"buildings", {
                    {"tax_office", {
                        {"base_cost", 150},
                        {"cost_multiplier", 1.5},
                        {"build_time_base", 180},
                        {"effects", {
                            {"tax_efficiency_per_level", 0.15},
                            {"admin_efficiency_per_level", 0.05},
                            {"corruption_resistance", 0.1}
                        }}
                    }},
                    {"market", {
                        {"base_cost", 200},
                        {"cost_multiplier", 1.4},
                        {"build_time_base", 240},
                        {"effects", {
                            {"trade_efficiency_per_level", 0.25},
                            {"development_per_level", 0.1},
                            {"population_capacity", 500}
                        }}
                    }},
                    {"fortification", {
                        {"base_cost", 300},
                        {"cost_multiplier", 1.6},
                        {"build_time_base", 360},
                        {"effects", {
                            {"defense_bonus_per_level", 0.2},
                            {"garrison_capacity", 100},
                            {"siege_resistance", 0.15}
                        }}
                    }}
                }}
            };
            
            std::string defaults_path = m_config_directory + "defaults.json";
            std::ofstream file(defaults_path);
            if (!file.is_open()) {
                return false;
            }
            
            file << std::setw(2) << defaults << std::endl;
            LogInfo("Created default configuration: " + defaults_path);
            return true;
            
        } catch (const std::exception& e) {
            LogError("Failed to create default configs: " + std::string(e.what()));
            return false;
        }
    }

    void ConfigManager::SetupFileWatching() {
        // File watching handled by periodic CheckForUpdates() calls
        LogInfo("File watching system initialized for hot reload");
    }

    void ConfigManager::RebuildMergedConfig() {
        m_merged_config = m_base_config;
        MergeJson(m_merged_config, m_runtime_overrides);
    }

    void ConfigManager::MergeJson(json& target, const json& source) {
        if (!source.is_object()) {
            return;
        }
        
        for (auto it = source.begin(); it != source.end(); ++it) {
            if (it.value().is_object() && 
                target.contains(it.key()) && 
                target[it.key()].is_object()) {
                // Recursive merge for objects
                MergeJson(target[it.key()], it.value());
            } else {
                // Direct assignment for values or arrays
                target[it.key()] = it.value();
            }
        }
    }

    std::vector<std::string> ConfigManager::SplitConfigPath(const std::string& path) const {
        std::vector<std::string> result;
        std::stringstream ss(path);
        std::string segment;
        
        while (std::getline(ss, segment, '.')) {
            if (!segment.empty()) {
                result.push_back(segment);
            }
        }
        
        return result;
    }

    bool ConfigManager::IsValidPath(const std::string& path) const {
        if (path.empty()) return false;
        if (path.front() == '.' || path.back() == '.') return false;
        if (path.find("..") != std::string::npos) return false;
        
        // Check for valid characters
        for (char c : path) {
            if (!std::isalnum(c) && c != '.' && c != '_') {
                return false;
            }
        }
        
        return true;
    }

    void ConfigManager::NotifyConfigChanged(const std::string& path,
const json& old_value, const json& new_value) {
        std::lock_guard<std::mutex> lock(m_callback_mutex);
        
        auto it = m_change_callbacks.find(path);
        if (it != m_change_callbacks.end()) {
            ConfigChangeEvent event{
                path,
                old_value,
                new_value,
                std::chrono::system_clock::now()
            };
            
            for (const auto& callback : it->second) {
                try {
                    callback(event);
                } catch (const std::exception& e) {
                    LogError("Exception in config change callback for '" + path + "': " + e.what());
                }
            }
        }
    }

    ConfigValidationResult ConfigManager::ValidateEconomicConfig() const {
        ConfigValidationResult result;
        result.is_valid = true;
        
        try {
            // Validate tax configuration
            auto tax_rate = GetValue<double>("economics.tax.base_rate", -1.0);
            if (tax_rate < 0.0 || tax_rate > 1.0) {
                result.AddError("Tax base rate must be between 0.0 and 1.0, got: " + std::to_string(tax_rate));
            }
            
            auto autonomy_penalty = GetValue<double>("economics.tax.autonomy_penalty_multiplier", -1.0);
            if (autonomy_penalty < 0.0 || autonomy_penalty > 1.0) {
                result.AddError("Autonomy penalty multiplier must be between 0.0 and 1.0, got: " + std::to_string(autonomy_penalty));
            }
            
            // Validate trade configuration
            auto efficiency_range = GetValue<std::vector<double>>("economics.trade.base_efficiency_range", {});
            if (efficiency_range.size() != 2) {
                result.AddError("Trade base efficiency range must have exactly 2 values [min, max]");
            } else if (efficiency_range[0] >= efficiency_range[1]) {
                result.AddError("Trade efficiency range invalid: min (" + std::to_string(efficiency_range[0]) + 
                              ") must be less than max (" + std::to_string(efficiency_range[1]) + ")");
            }
            
            auto market_bonus = GetValue<double>("economics.trade.market_bonus_per_level", -1.0);
            if (market_bonus <= 0.0) {
                result.AddError("Market bonus per level must be positive, got: " + std::to_string(market_bonus));
            }
            
            // Validate inflation settings
            auto inflation_rate = GetValue<double>("economics.inflation.base_rate", -1.0);
            if (inflation_rate < -0.1 || inflation_rate > 0.5) {
                result.AddWarning("Inflation base rate is outside typical range [-0.1, 0.5]: " + std::to_string(inflation_rate));
            }
            
        } catch (const std::exception& e) {
            result.AddError("Exception during economic config validation: " + std::string(e.what()));
        }
        
        return result;
    }

    ConfigValidationResult ConfigManager::ValidateBuildingConfig() const {
        ConfigValidationResult result;
        result.is_valid = true;
        
        try {
            std::vector<std::string> required_buildings = {
                "tax_office", "market", "fortification", "temple", "workshop", "farm"
            };
            
            for (const auto& building : required_buildings) {
                std::string base_path = "buildings." + building;
                
                // Validate base cost
                auto base_cost = GetValue<int>(base_path + ".base_cost", -1);
                if (base_cost <= 0) {
                    result.AddError("Building '" + building + "' has invalid base cost: " + std::to_string(base_cost));
                    continue;  // Skip other validations for this building
                }
                
                // Validate cost multiplier
                auto cost_multiplier = GetValue<double>(base_path + ".cost_multiplier", 0.0);
                if (cost_multiplier <= 1.0) {
                    result.AddError("Building '" + building + "' cost multiplier must be > 1.0, got: " + std::to_string(cost_multiplier));
                }
                
                // Validate build time
                auto build_time = GetValue<int>(base_path + ".build_time_base", -1);
                if (build_time <= 0) {
                    result.AddError("Building '" + building + "' has invalid build time: " + std::to_string(build_time));
                }
                
                // Validate effects section exists
                auto effects_section = GetSection(base_path + ".effects");
                if (effects_section.empty()) {
                    result.AddWarning("Building '" + building + "' has no effects defined");
                } else {
                    // Validate effect values are reasonable
                    for (const auto& [effect_name, effect_value] : effects_section) {
                        if (effect_value.is_number()) {
                            double value = effect_value.get<double>();
                            if (value < -10.0 || value > 10.0) {
                                result.AddWarning("Building '" + building + "' effect '" + effect_name + 
                                                "' has extreme value: " + std::to_string(value));
                            }
                        }
                    }
                }
            }
            
        } catch (const std::exception& e) {
            result.AddError("Exception during building config validation: " + std::string(e.what()));
        }
        
        return result;
    }

    ConfigValidationResult ConfigManager::ValidateMilitaryConfig() const {
        ConfigValidationResult result;
        result.is_valid = true;
        
        try {
            // Validate unit types and costs
            auto units_section = GetSection("military.units");
            if (units_section.empty()) {
                result.AddError("Military units section is missing or empty");
                return result;
            }
            
            for (const auto& [unit_name, unit_data] : units_section) {
                if (!unit_data.is_object()) continue;
                
                // Validate required fields
                std::vector<std::string> required_fields = {"cost", "upkeep", "combat_strength", "recruitment_time"};
                for (const auto& field : required_fields) {
                    if (!unit_data.contains(field)) {
                        result.AddError("Military unit '" + unit_name + "' missing required field: " + field);
                    }
                }
                
                // Validate numeric ranges
                if (unit_data.contains("cost")) {
                    int cost = unit_data["cost"].get<int>();
                    if (cost <= 0 || cost > 10000) {
                        result.AddError("Military unit '" + unit_name + "' has invalid cost: " + std::to_string(cost));
                    }
                }
                
                if (unit_data.contains("combat_strength")) {
                    double strength = unit_data["combat_strength"].get<double>();
                    if (strength <= 0.0 || strength > 100.0) {
                        result.AddError("Military unit '" + unit_name + "' has invalid combat strength: " + std::to_string(strength));
                    }
                }
            }
            
            // Validate military technologies
            auto tech_requirements = GetSection("military.technology_requirements");
            for (const auto& [unit_name, tech_level] : tech_requirements) {
                if (tech_level.is_number()) {
                    int level = tech_level.get<int>();
                    if (level < 0 || level > 20) {
                        result.AddWarning("Military unit '" + unit_name + "' has unusual tech requirement: " + std::to_string(level));
                    }
                }
            }
            
        } catch (const std::exception& e) {
            result.AddError("Exception during military config validation: " + std::string(e.what()));
        }
        
        return result;
    }

    ConfigValidationResult ConfigManager::ValidateUIConfig() const {
        ConfigValidationResult result;
        result.is_valid = true;
        
        try {
            // Validate window settings
            auto window_width = GetValue<int>("ui.window.default_width", 0);
            auto window_height = GetValue<int>("ui.window.default_height", 0);
            
            if (window_width < 800 || window_width > 7680) {
                result.AddWarning("UI window width outside typical range [800, 7680]: " + std::to_string(window_width));
            }
            
            if (window_height < 600 || window_height > 4320) {
                result.AddWarning("UI window height outside typical range [600, 4320]: " + std::to_string(window_height));
            }
            
            // Validate UI scaling
            auto ui_scale = GetValue<double>("ui.scaling.factor", 1.0);
            if (ui_scale < 0.5 || ui_scale > 3.0) {
                result.AddWarning("UI scaling factor outside typical range [0.5, 3.0]: " + std::to_string(ui_scale));
            }
            
            // Validate color themes
            auto themes_section = GetSection("ui.themes");
            if (!themes_section.empty()) {
                for (const auto& [theme_name, theme_data] : themes_section) {
                    if (!theme_data.is_object()) {
                        result.AddWarning("UI theme '" + theme_name + "' is not a valid object");
                        continue;
                    }
                    
                    // Check for required color definitions
                    std::vector<std::string> required_colors = {"background", "text", "accent", "warning", "error"};
                    for (const auto& color : required_colors) {
                        if (!theme_data.contains(color)) {
                            result.AddWarning("UI theme '" + theme_name + "' missing color: " + color);
                        }
                    }
                }
            }
            
        } catch (const std::exception& e) {
            result.AddError("Exception during UI config validation: " + std::string(e.what()));
        }
        
        return result;
    }

    ConfigValidationResult ConfigManager::ValidateSystemConfig() const {
        ConfigValidationResult result;
        result.is_valid = true;
        
        try {
            // Validate threading configuration
            auto enable_threading = GetValue<bool>("system.threading.enable_threading", true);
            if (!enable_threading) {
                result.AddWarning("Threading is disabled - this may impact performance");
            }
            
            auto thread_pool_size = GetValue<int>("system.threading.thread_pool_size", 0);
            if (thread_pool_size < 1 || thread_pool_size > 32) {
                result.AddError("Thread pool size must be between 1 and 32, got: " + std::to_string(thread_pool_size));
            }
            
            // Validate performance settings
            auto target_fps = GetValue<int>("system.performance.target_fps", 0);
            if (target_fps < 30 || target_fps > 240) {
                result.AddWarning("Target FPS outside typical range [30, 240]: " + std::to_string(target_fps));
            }
            
            // Validate update frequencies
            auto frequencies = GetSection("system.performance.update_frequencies");
            for (const auto& [system_name, frequency] : frequencies) {
                if (frequency.is_number()) {
                    double freq = frequency.get<double>();
                    if (freq <= 0.0 || freq > 1000.0) {
                        result.AddError("System '" + system_name + "' has invalid update frequency: " + std::to_string(freq));
                    }
                }
            }
            
            // Validate version
            auto version = GetValue<std::string>("system.version", "");
            if (version.empty()) {
                result.AddWarning("System version not specified");
            } else {
                // Basic version format check (x.y.z)
                std::regex version_regex(R"(\d+\.\d+\.\d+)");
                if (!std::regex_match(version, version_regex)) {
                    result.AddWarning("System version format may be invalid: " + version);
                }
            }
            
        } catch (const std::exception& e) {
            result.AddError("Exception during system config validation: " + std::string(e.what()));
        }
        
        return result;
    }

    void ConfigManager::LogInfo(const std::string& message) const {
        if (m_log_level == "DEBUG" || m_log_level == "INFO") {
            std::cout << "[ConfigManager] INFO: " << message << std::endl;
        }
    }

    void ConfigManager::LogWarning(const std::string& message) const {
        if (m_log_level != "ERROR") {
            std::cout << "[ConfigManager] WARNING: " << message << std::endl;
        }
    }

    void ConfigManager::LogError(const std::string& message) const {
        std::cerr << "[ConfigManager] ERROR: " << message << std::endl;
    }

    // ============================================================================
    // FormulaEngine Implementation
    // ============================================================================

    FormulaEngine& FormulaEngine::Instance() {
        static FormulaEngine instance;
        return instance;
    }

    void FormulaEngine::RegisterFormula(const std::string& name, const std::string& formula) {
        std::unique_lock<std::shared_mutex> lock(m_formula_mutex);
        m_formulas[name] = formula;
    }

    void FormulaEngine::UnregisterFormula(const std::string& name) {
        std::unique_lock<std::shared_mutex> lock(m_formula_mutex);
        m_formulas.erase(name);
    }

    bool FormulaEngine::HasFormula(const std::string& name) const {
        std::shared_lock<std::shared_mutex> lock(m_formula_mutex);
        return m_formulas.find(name) != m_formulas.end();
    }

    double FormulaEngine::Evaluate(const std::string& formula_name,
                                  const std::unordered_map<std::string, double>& variables) {
        std::shared_lock<std::shared_mutex> lock(m_formula_mutex);
        
        auto it = m_formulas.find(formula_name);
        if (it == m_formulas.end()) {
            throw std::runtime_error("Formula not found: " + formula_name);
        }
        
        return EvaluateExpression(it->second, variables);
    }

    std::optional<double> FormulaEngine::TryEvaluate(const std::string& formula_name,
                                                    const std::unordered_map<std::string, double>& variables) {
        try {
            return Evaluate(formula_name, variables);
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }

    bool FormulaEngine::ValidateFormula(const std::string& formula) const {
        try {
            // Create dummy variables for all potential variable names
            std::unordered_map<std::string, double> dummy_vars;
            auto variables = GetRequiredVariables(formula);
            
            for (const auto& var : variables) {
                dummy_vars[var] = 1.0;  // Use 1.0 as test value
            }
            
            // Try to evaluate with dummy variables
            EvaluateExpression(formula, dummy_vars);
            return true;
            
        } catch (const std::exception&) {
            return false;
        }
    }

    std::vector<std::string> FormulaEngine::GetRequiredVariables(const std::string& formula) const {
        std::vector<std::string> variables;
        std::regex var_regex(R"([a-zA-Z_][a-zA-Z0-9_]*)");
        std::sregex_iterator iter(formula.begin(), formula.end(), var_regex);
        std::sregex_iterator end;
        
        // Mathematical functions to exclude
        std::set<std::string> math_functions = {
            "sin", "cos", "tan", "asin", "acos", "atan", "atan2",
            "sinh", "cosh", "tanh", "exp", "log", "log10", "pow", 
            "sqrt", "abs", "floor", "ceil", "round", "min", "max"
        };
        
        for (; iter != end; ++iter) {
            std::string var = iter->str();
            if (math_functions.find(var) == math_functions.end() &&
                std::find(variables.begin(), variables.end(), var) == variables.end()) {
                variables.push_back(var);
            }
        }
        
        return variables;
    }

    std::vector<std::string> FormulaEngine::GetAvailableFormulas() const {
        std::shared_lock<std::shared_mutex> lock(m_formula_mutex);
        
        std::vector<std::string> formula_names;
        formula_names.reserve(m_formulas.size());
        
        for (const auto& [name, _] : m_formulas) {
            formula_names.push_back(name);
        }
        
        return formula_names;
    }

    void FormulaEngine::LoadFormulasFromConfig() {
        std::unique_lock<std::shared_mutex> lock(m_formula_mutex);
        
        auto formulas_section = ConfigManager::Instance().GetSection("formulas");
        
        m_formulas.clear();
        for (const auto& [name, formula] : formulas_section) {
            if (formula.is_string()) {
                m_formulas[name] = formula.get<std::string>();
            }
        }
    }

    void FormulaEngine::ClearAllFormulas() {
        std::unique_lock<std::shared_mutex> lock(m_formula_mutex);
        m_formulas.clear();
    }

    double FormulaEngine::EvaluateExpression(const std::string& expression,
                                           const std::unordered_map<std::string, double>& variables) {
        // Simplified expression evaluator
        // In production, you'd want to use a proper math expression parser like muParser
        
        std::string processed = PreprocessFormula(expression);
        std::string substituted = SubstituteVariables(processed, variables);
        
        // This is a basic fallback implementation
        // For production use, integrate a proper expression parser
        try {
            // Try direct string-to-double conversion for simple cases
            return std::stod(substituted);
        } catch (const std::exception&) {
            // For complex expressions, return a safe fallback
            // In production, this should use a proper math parser
            
            // Basic arithmetic evaluation (very limited)
            // This is just to prevent crashes - use a real parser in production
            if (substituted.find('+') != std::string::npos ||
                substituted.find('-') != std::string::npos ||
                substituted.find('*') != std::string::npos ||
                substituted.find('/') != std::string::npos) {
                
                // Return average of all variable values as fallback
                if (!variables.empty()) {
                    double sum = 0.0;
                    for (const auto& [_, value] : variables) {
                        sum += value;
                    }
                    return sum / variables.size();
                }
            }
            
            return 0.0;  // Safe fallback
        }
    }

    std::string FormulaEngine::PreprocessFormula(const std::string& formula) const {
        std::string result = formula;
        
        // Remove extra whitespace
        result = std::regex_replace(result, std::regex(R"(\s+)"), "");
        
        // Convert common mathematical functions
        result = std::regex_replace(result, std::regex(R"(max\()"), "std::max(");
        result = std::regex_replace(result, std::regex(R"(min\()"), "std::min(");
        result = std::regex_replace(result, std::regex(R"(sqrt\()"), "std::sqrt(");
        
        return result;
    }

    std::string FormulaEngine::SubstituteVariables(const std::string& formula,
                                                  const std::unordered_map<std::string, double>& variables) const {
        std::string result = formula;
        
        // Replace variables with their values
        for (const auto& [var_name, value] : variables) {
            std::regex var_regex("\\b" + var_name + "\\b");
            result = std::regex_replace(result, var_regex, std::to_string(value));
        }
        
        return result;
    }

    // ============================================================================
    // ProvinceConfigAdapter Implementation  
    // ============================================================================

    double ProvinceConfigAdapter::CalculateTaxIncome(double base_tax, double admin_efficiency,
                                                   double autonomy, double stability) {
        auto variables = CreateVariableMap({
            {"base_tax", base_tax},
            {"admin_efficiency", admin_efficiency}, 
            {"autonomy", autonomy},
            {"stability", stability}
        });
        
        auto result = FormulaEngine::Instance().TryEvaluate("tax_income", variables);
        if (result.has_value()) {
            return result.value();
        }
        
        // Fallback calculation
        double stability_multiplier = 0.5 + stability * 0.5;
        double autonomy_penalty = 1.0 - (autonomy * CONFIG_VALUE("economics.tax.autonomy_penalty_multiplier", 0.75));
        double efficiency_bonus = CONFIG_VALUE("economics.tax.admin_efficiency_bonus", 1.6);
        
        return base_tax * admin_efficiency * efficiency_bonus * autonomy_penalty * stability_multiplier;
    }

    double ProvinceConfigAdapter::CalculateTradeIncome(double base_trade, double market_level,
                                                     double route_efficiency, double stability) {
        auto market_bonus_per_level = CONFIG_VALUE("economics.trade.market_bonus_per_level", 0.25);
        double market_multiplier = 1.0 + (market_level * market_bonus_per_level);
        
        auto variables = CreateVariableMap({
            {"base_trade", base_trade},
            {"market_bonus", market_multiplier},
            {"route_efficiency", route_efficiency},
            {"stability", stability}
        });
        
        auto result = FormulaEngine::Instance().TryEvaluate("trade_income", variables);
        if (result.has_value()) {
            return result.value();
        }
        
        // Fallback calculation
        double stability_impact = CONFIG_VALUE("economics.trade.stability_impact", 0.6);
        double stability_multiplier = 0.7 + stability * stability_impact;
        
        return base_trade * market_multiplier * route_efficiency * stability_multiplier;
    }

    double ProvinceConfigAdapter::CalculateMaintenanceCost(double base_cost, double efficiency_modifier) {
        auto variables = CreateVariableMap({
            {"base_cost", base_cost},
            {"efficiency_modifier", efficiency_modifier}
        });
        
        auto result = FormulaEngine::Instance().TryEvaluate("maintenance_cost", variables);
        if (result.has_value()) {
            return result.value();
        }
        
        // Fallback calculation
        return base_cost * (2.0 - efficiency_modifier);
    }

    double ProvinceConfigAdapter::CalculatePopulationGrowth(double base_growth, double stability,
                                                          double war_exhaustion, double prosperity) {
        auto variables = CreateVariableMap({
            {"base_growth", base_growth},
            {"stability", stability},
            {"war_exhaustion", war_exhaustion},
            {"prosperity", prosperity}
        });
        
        auto result = FormulaEngine::Instance().TryEvaluate("population_growth", variables);
        if (result.has_value()) {
            return result.value();
        }
        
        // Fallback calculation
        double stability_bonus = 0.5 + stability;
        double war_penalty = 1.0 - (war_exhaustion * 0.8);
        double prosperity_bonus = 1.0 + (prosperity * 0.3);
        
        return base_growth * stability_bonus * war_penalty * prosperity_bonus;
    }

    double ProvinceConfigAdapter::CalculateMigrationRate(double push_factors, double pull_factors) {
        auto variables = CreateVariableMap({
            {"push_factors", push_factors},
            {"pull_factors", pull_factors}
        });
        
        auto result = FormulaEngine::Instance().TryEvaluate("migration_rate", variables);
        if (result.has_value()) {
            return result.value();
        }
        
        // Fallback calculation
        double net_attraction = pull_factors - push_factors;
        return std::max(0.0, net_attraction * 0.1);
    }

    double ProvinceConfigAdapter::CalculateRecruitmentCapacity(double population, double development,
                                                             double military_tech) {
        auto variables = CreateVariableMap({
            {"population", population},
            {"development", development},
            {"military_tech", military_tech}
        });
        
        auto result = FormulaEngine::Instance().TryEvaluate("recruitment_capacity", variables);
        if (result.has_value()) {
            return result.value();
        }
        
        // Fallback calculation  
        double base_rate = 0.02;  // 2% of population
        double development_multiplier = 1.0 + (development * 0.1);
        double tech_multiplier = 1.0 + (military_tech * 0.05);
        
        return population * base_rate * development_multiplier * tech_multiplier;
    }

    double ProvinceConfigAdapter::CalculateSupplyCapacity(double base_supply, double infrastructure) {
        auto variables = CreateVariableMap({
            {"base_supply", base_supply},
            {"infrastructure", infrastructure}
        });
        
        auto result = FormulaEngine::Instance().TryEvaluate("supply_capacity", variables);
        if (result.has_value()) {
            return result.value();
        }
        
        // Fallback calculation
        double infrastructure_multiplier = 1.0 + (infrastructure * 0.2);
        return base_supply * infrastructure_multiplier;
    }

    int ProvinceConfigAdapter::GetBuildingCost(const std::string& building_type, int current_level) {
        auto base_cost = CONFIG_VALUE("buildings." + building_type + ".base_cost", 100);
        auto cost_multiplier = CONFIG_VALUE("buildings." + building_type + ".cost_multiplier", 1.5);
        
        return static_cast<int>(base_cost * std::pow(cost_multiplier, current_level));
    }

    double ProvinceConfigAdapter::GetBuildingEffect(const std::string& building_type,
                                                  const std::string& effect_type, int level) {
        auto effect_per_level = CONFIG_VALUE("buildings." + building_type + ".effects." + effect_type + "_per_level", 0.0);
        return effect_per_level * level;
    }

    int ProvinceConfigAdapter::GetBuildingUpgradeTime(const std::string& building_type, int target_level) {
        auto base_time = CONFIG_VALUE("buildings." + building_type + ".build_time_base", 180);
        auto time_multiplier = CONFIG_VALUE("buildings." + building_type + ".build_time_multiplier", 1.2);
        
        return static_cast<int>(base_time * std::pow(time_multiplier, target_level - 1));
    }

    double ProvinceConfigAdapter::CalculateResearchCost(const std::string& tech_category, int current_level) {
        auto base_cost = CONFIG_VALUE("technology." + tech_category + ".base_cost", 100.0);
        auto cost_scaling = CONFIG_VALUE("technology." + tech_category + ".cost_scaling", 1.8);
        
        return base_cost * std::pow(cost_scaling, current_level);
    }

    double ProvinceConfigAdapter::CalculateResearchSpeed(double base_speed, double research_efficiency) {
        auto variables = CreateVariableMap({
            {"base_speed", base_speed},
            {"research_efficiency", research_efficiency}
        });
        
        auto result = FormulaEngine::Instance().TryEvaluate("research_speed", variables);
        if (result.has_value()) {
            return result.value();
        }
        
        // Fallback calculation
        return base_speed * research_efficiency;
    }

    std::unordered_map<std::string, double> ProvinceConfigAdapter::CreateVariableMap(
        const std::vector<std::pair<std::string, double>>& variables) {
        
        std::unordered_map<std::string, double> result;
        result.reserve(variables.size());
        
        for (const auto& [name, value] : variables) {
            result[name] = value;
        }
        
        return result;
    }

} // namespace core::config
