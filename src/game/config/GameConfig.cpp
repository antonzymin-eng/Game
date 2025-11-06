// ============================================================================
// Date/Time Created: September 27, 2025 - 7:50 PM PST
// Intended Folder Location: src/game/config/GameConfig.cpp
// GameConfig Implementation - Thread-safe configuration with hot reload
// ============================================================================

#include "game/config/GameConfig.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <regex>
#include <cmath>
#include <mutex>
#include <type_traits>

namespace game {
    namespace config {

        GameConfig::GameConfig() 
            : m_hot_reload_enabled(false)
            , m_check_interval(1000)
            , m_last_reload_time(std::chrono::system_clock::now()) {
            
            // Load default formulas
            m_formulas["tax_income"] = "${base_tax} * ${admin_efficiency} * ${autonomy_penalty} * ${stability}";
            m_formulas["trade_income"] = "${base_trade} * ${market_bonus} * ${route_efficiency}";
        }

        GameConfig& GameConfig::Instance() {
            static GameConfig instance;
            return instance;
        }

        bool GameConfig::LoadFromFile(const std::string& filepath) {
            std::unique_lock<std::shared_mutex> lock(m_config_mutex);

            std::ifstream file(filepath);
            if (!file.is_open()) {
                std::cerr << "[GameConfig] Failed to open config file: " << filepath << std::endl;
                return false;
            }

            Json::CharReaderBuilder reader_builder;
            std::string errors;

            Json::Value new_config;
            if (!Json::parseFromStream(reader_builder, file, &new_config, &errors)) {
                std::cerr << "[GameConfig] Failed to parse config file: " << errors << std::endl;
                return false;
            }

            m_previous_config_data = m_config_data;
            m_config_data = new_config;
            m_current_filepath = filepath;
            m_last_reload_time = std::chrono::system_clock::now();

            // Track loaded file
            if (std::find(m_loaded_files.begin(), m_loaded_files.end(), filepath) == m_loaded_files.end()) {
                m_loaded_files.push_back(filepath);
            }

            if (UpdateFileTimestamp()) {
                std::cout << "[GameConfig] Configuration loaded from: " << filepath << std::endl;
                return true;
            }

            return false;
        }

        bool GameConfig::SaveToFile(const std::string& filepath) const {
            std::unique_lock<std::shared_mutex> lock(m_config_mutex);

            std::ofstream file(filepath);
            if (!file.is_open()) {
                std::cerr << "[GameConfig] Failed to open config file for writing: " << filepath << std::endl;
                return false;
            }

            Json::StreamWriterBuilder writer_builder;
            writer_builder["indentation"] = "  ";
            std::unique_ptr<Json::StreamWriter> writer(writer_builder.newStreamWriter());
            writer->write(m_config_data, &file);

            std::cout << "[GameConfig] Configuration saved to: " << filepath << std::endl;
            return true;
        }

        int GameConfig::GetInt(const std::string& key, int default_value) const {
            std::shared_lock<std::shared_mutex> lock(m_config_mutex);
            Json::Value value = GetValueFromPath(key);
            return value.isInt() ? value.asInt() : default_value;
        }

        double GameConfig::GetDouble(const std::string& key, double default_value) const {
            std::shared_lock<std::shared_mutex> lock(m_config_mutex);
            Json::Value value = GetValueFromPath(key);
            return value.isDouble() ? value.asDouble() : default_value;
        }

        float GameConfig::GetFloat(const std::string& key, float default_value) const {
            std::shared_lock<std::shared_mutex> lock(m_config_mutex);
            Json::Value value = GetValueFromPath(key);
            return value.isDouble() ? value.asFloat() : default_value;
        }

        bool GameConfig::GetBool(const std::string& key, bool default_value) const {
            std::shared_lock<std::shared_mutex> lock(m_config_mutex);
            Json::Value value = GetValueFromPath(key);
            return value.isBool() ? value.asBool() : default_value;
        }

        std::string GameConfig::GetString(const std::string& key, const std::string& default_value) const {
            std::shared_lock<std::shared_mutex> lock(m_config_mutex);
            Json::Value value = GetValueFromPath(key);
            return value.isString() ? value.asString() : default_value;
        }

        void GameConfig::SetInt(const std::string& key, int value) {
            std::unique_lock<std::shared_mutex> lock(m_config_mutex);
            SetValueAtPath(key, Json::Value(value));
        }

        void GameConfig::SetDouble(const std::string& key, double value) {
            std::unique_lock<std::shared_mutex> lock(m_config_mutex);
            SetValueAtPath(key, Json::Value(value));
        }

        void GameConfig::SetFloat(const std::string& key, float value) {
            std::unique_lock<std::shared_mutex> lock(m_config_mutex);
            SetValueAtPath(key, Json::Value(value));
        }

        void GameConfig::SetBool(const std::string& key, bool value) {
            std::unique_lock<std::shared_mutex> lock(m_config_mutex);
            SetValueAtPath(key, Json::Value(value));
        }

        void GameConfig::SetString(const std::string& key, const std::string& value) {
            std::unique_lock<std::shared_mutex> lock(m_config_mutex);
            SetValueAtPath(key, Json::Value(value));
        }

        bool GameConfig::HasKey(const std::string& key) const {
            std::shared_lock<std::shared_mutex> lock(m_config_mutex);
            Json::Value value = GetValueFromPath(key);
            return !value.isNull();
        }

        bool GameConfig::EnableHotReload(double check_interval_seconds) {
            if (m_current_filepath.empty()) {
                std::cerr << "[GameConfig] Cannot enable hot reload: no config file loaded" << std::endl;
                return false;
            }

            m_check_interval = std::chrono::milliseconds(static_cast<int>(check_interval_seconds * 1000.0));
            m_last_check_time = std::chrono::steady_clock::now();
            
            if (!UpdateFileTimestamp()) {
                std::cerr << "[GameConfig] Failed to initialize file timestamp for hot reload" << std::endl;
                return false;
            }

            m_hot_reload_enabled.store(true);
            std::cout << "[GameConfig] Hot reload enabled (check interval: " << check_interval_seconds << "s)" << std::endl;
            return true;
        }

        void GameConfig::DisableHotReload() {
            m_hot_reload_enabled.store(false);
            std::cout << "[GameConfig] Hot reload disabled" << std::endl;
        }

        bool GameConfig::IsHotReloadEnabled() const {
            return m_hot_reload_enabled.load();
        }

        bool GameConfig::CheckForChanges() {
            if (!IsHotReloadEnabled()) {
                return false;
            }

            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_check_time);

            if (elapsed < m_check_interval) {
                return false;
            }

            m_last_check_time = now;

            if (HasFileChanged()) {
                std::cout << "[GameConfig] Configuration file changed, reloading..." << std::endl;
                return Reload();
            }

            return false;
        }

        bool GameConfig::Reload() {
            if (m_current_filepath.empty()) {
                std::cerr << "[GameConfig] No config file loaded to reload" << std::endl;
                return false;
            }

            std::unique_lock<std::shared_mutex> lock(m_config_mutex);

            std::ifstream file(m_current_filepath);
            if (!file.is_open()) {
                std::cerr << "[GameConfig] Failed to open config for reload: " << m_current_filepath << std::endl;
                return false;
            }

            Json::CharReaderBuilder reader_builder;
            std::string errors;

            Json::Value new_config;
            if (!Json::parseFromStream(reader_builder, file, &new_config, &errors)) {
                std::cerr << "[GameConfig] Failed to parse config during reload: " << errors << std::endl;
                return false;
            }

            auto changed_sections = DetectChangedSections(m_config_data, new_config);
            
            m_previous_config_data = m_config_data;
            m_config_data = new_config;
            
            UpdateFileTimestamp();

            std::cout << "[GameConfig] Configuration reloaded successfully" << std::endl;
            
            if (!changed_sections.empty()) {
                NotifyCallbacks(changed_sections);
            }

            return true;
        }

        void GameConfig::RegisterChangeCallback(const std::string& section, ConfigChangeCallback callback) {
            std::unique_lock<std::shared_mutex> lock(m_callback_mutex);
            m_change_callbacks[section] = callback;
            std::cout << "[GameConfig] Registered change callback for section: " << section << std::endl;
        }

        void GameConfig::UnregisterChangeCallback(const std::string& section) {
            std::unique_lock<std::shared_mutex> lock(m_callback_mutex);
            m_change_callbacks.erase(section);
            std::cout << "[GameConfig] Unregistered change callback for section: " << section << std::endl;
        }

        void GameConfig::ClearAllCallbacks() {
            std::unique_lock<std::shared_mutex> lock(m_callback_mutex);
            m_change_callbacks.clear();
            std::cout << "[GameConfig] Cleared all change callbacks" << std::endl;
        }

        bool GameConfig::ValidateConfiguration() const {
            auto errors = GetValidationErrors();
            return errors.empty();
        }

        std::vector<std::string> GameConfig::GetValidationErrors() const {
            auto validation_result = ValidateAllSections();
            return validation_result.errors;
        }

        std::vector<std::string> GameConfig::GetKeysWithPrefix(const std::string& prefix) const {
            std::unique_lock<std::shared_mutex> lock(m_config_mutex);
            std::vector<std::string> keys;

            auto path_parts = SplitPath(prefix);
            Json::Value current = m_config_data;

            for (const auto& part : path_parts) {
                if (current.isMember(part)) {
                    current = current[part];
                } else {
                    return keys;
                }
            }

            if (current.isObject()) {
                for (const auto& key : current.getMemberNames()) {
                    keys.push_back(prefix + (prefix.empty() ? "" : ".") + key);
                }
            }

            return keys;
        }

        std::vector<std::string> GameConfig::GetAllSections() const {
            std::unique_lock<std::shared_mutex> lock(m_config_mutex);
            std::vector<std::string> sections;

            if (m_config_data.isObject()) {
                for (const auto& key : m_config_data.getMemberNames()) {
                    sections.push_back(key);
                }
            }

            return sections;
        }

        bool GameConfig::HasSection(const std::string& section) const {
            std::unique_lock<std::shared_mutex> lock(m_config_mutex);
            return m_config_data.isMember(section);
        }

        void GameConfig::PrintAllConfig() const {
            std::unique_lock<std::shared_mutex> lock(m_config_mutex);
            Json::StreamWriterBuilder writer_builder;
            writer_builder["indentation"] = "  ";
            std::unique_ptr<Json::StreamWriter> writer(writer_builder.newStreamWriter());
            
            std::ostringstream oss;
            writer->write(m_config_data, &oss);
            std::cout << "=== Configuration ===" << std::endl;
            std::cout << oss.str() << std::endl;
        }

        void GameConfig::PrintSection(const std::string& section) const {
            std::unique_lock<std::shared_mutex> lock(m_config_mutex);
            
            if (!m_config_data.isMember(section)) {
                std::cout << "[GameConfig] Section not found: " << section << std::endl;
                return;
            }

            Json::StreamWriterBuilder writer_builder;
            writer_builder["indentation"] = "  ";
            std::unique_ptr<Json::StreamWriter> writer(writer_builder.newStreamWriter());
            
            std::ostringstream oss;
            writer->write(m_config_data[section], &oss);
            std::cout << "=== Section: " << section << " ===" << std::endl;
            std::cout << oss.str() << std::endl;
        }

        std::string GameConfig::GetConfigSummary() const {
            std::unique_lock<std::shared_mutex> lock(m_config_mutex);
            std::ostringstream summary;
            
            summary << "Configuration Summary:\n";
            summary << "  File: " << m_current_filepath << "\n";
            summary << "  Hot Reload: " << (IsHotReloadEnabled() ? "Enabled" : "Disabled") << "\n";
            summary << "  Sections: " << m_config_data.getMemberNames().size() << "\n";
            
            for (const auto& section : m_config_data.getMemberNames()) {
                if (m_config_data[section].isObject()) {
                    summary << "    - " << section << " (" 
                           << m_config_data[section].getMemberNames().size() << " keys)\n";
                }
            }
            
            return summary.str();
        }

        bool GameConfig::UpdateFileTimestamp() {
            if (m_current_filepath.empty()) {
                return false;
            }

            try {
                if (std::filesystem::exists(m_current_filepath)) {
                    m_last_write_time = std::filesystem::last_write_time(m_current_filepath);
                    return true;
                }
            } catch (const std::filesystem::filesystem_error& e) {
                std::cerr << "[GameConfig] Failed to update file timestamp: " << e.what() << std::endl;
            }

            return false;
        }

        bool GameConfig::HasFileChanged() const {
            if (m_current_filepath.empty()) {
                return false;
            }

            try {
                if (std::filesystem::exists(m_current_filepath)) {
                    auto current_time = std::filesystem::last_write_time(m_current_filepath);
                    return current_time != m_last_write_time;
                }
            } catch (const std::filesystem::filesystem_error& e) {
                std::cerr << "[GameConfig] Error checking file timestamp: " << e.what() << std::endl;
            }

            return false;
        }

        void GameConfig::NotifyCallbacks(const std::vector<std::string>& changed_sections) {
            std::unique_lock<std::shared_mutex> lock(m_callback_mutex);

            for (const auto& section : changed_sections) {
                auto it = m_change_callbacks.find(section);
                if (it != m_change_callbacks.end()) {
                    std::cout << "[GameConfig] Notifying callback for changed section: " << section << std::endl;
                    it->second(section);
                }
            }
        }

        std::vector<std::string> GameConfig::DetectChangedSections(
            const Json::Value& old_config, 
            const Json::Value& new_config) const {
            
            std::vector<std::string> changed;

            if (!old_config.isObject() || !new_config.isObject()) {
                return changed;
            }

            auto old_sections = old_config.getMemberNames();
            auto new_sections = new_config.getMemberNames();

            for (const auto& section : new_sections) {
                if (!old_config.isMember(section) || old_config[section] != new_config[section]) {
                    changed.push_back(section);
                }
            }

            return changed;
        }

        Json::Value GameConfig::GetValueFromPath(const std::string& path) const {
            auto parts = SplitPath(path);
            Json::Value current = m_config_data;

            for (const auto& part : parts) {
                if (current.isMember(part)) {
                    current = current[part];
                } else {
                    return Json::Value();  // Return empty/null value
                }
            }

            return current;
        }

        void GameConfig::SetValueAtPath(const std::string& path, const Json::Value& value) {
            auto parts = SplitPath(path);
            Json::Value* current = &m_config_data;

            for (size_t i = 0; i < parts.size() - 1; ++i) {
                if (!current->isMember(parts[i])) {
                    (*current)[parts[i]] = Json::Value(Json::objectValue);
                }
                current = &(*current)[parts[i]];
            }

            (*current)[parts.back()] = value;
        }

        std::vector<std::string> GameConfig::SplitPath(const std::string& path) const {
            std::vector<std::string> parts;
            std::string current;

            for (char c : path) {
                if (c == '.') {
                    if (!current.empty()) {
                        parts.push_back(current);
                        current.clear();
                    }
                } else {
                    current += c;
                }
            }

            if (!current.empty()) {
                parts.push_back(current);
            }

            return parts;
        }

        // Legacy ValidateSection method removed - replaced with ValidationResult-based validation

        bool GameConfig::ValidateNumericRange(const std::string& key, double value, 
                                              double min_val, double max_val) const {
            if (value < min_val || value > max_val) {
                std::cerr << "[GameConfig] Validation failed for " << key << ": " << value 
                         << " is outside range [" << min_val << ", " << max_val << "]" << std::endl;
                return false;
            }
            return true;
        }

        // Static initialization method
        void GameConfig::Initialize(const std::string& config_directory) {
            std::string config_file = config_directory + "/GameConfig.json";
            
            if (!Instance().LoadFromFile(config_file)) {
                std::cerr << "[GameConfig] Failed to load configuration from: " << config_file << std::endl;
                throw std::runtime_error("Failed to initialize GameConfig");
            }
            
            std::cout << "[GameConfig] Successfully initialized from: " << config_file << std::endl;
        }

        // Configuration structure getters
        GameConfig::CouncilConfiguration GameConfig::GetCouncilConfiguration() const {
            CouncilConfiguration config;
            config.default_delegation_level = GetDouble("council.default_delegation_level", config.default_delegation_level);
            config.max_council_members = GetInt("council.max_council_members", config.max_council_members);
            config.decision_threshold = GetDouble("council.decision_threshold", config.decision_threshold);
            return config;
        }

        GameConfig::ThreadingConfiguration GameConfig::GetThreadingConfiguration() const {
            ThreadingConfiguration config;
            config.worker_thread_count = GetInt("threading.worker_thread_count", config.worker_thread_count);
            config.max_systems_per_frame = GetInt("threading.max_systems_per_frame", config.max_systems_per_frame);
            config.frame_budget_ms = GetDouble("threading.frame_budget_ms", config.frame_budget_ms);
            config.performance_monitoring = GetBool("threading.performance_monitoring", config.performance_monitoring);
            return config;
        }

        GameConfig::PopulationConfiguration GameConfig::GetPopulationConfiguration() const {
            PopulationConfiguration config;
            config.base_growth_rate = GetDouble("population.base_growth_rate", config.base_growth_rate);
            config.happiness_growth_modifier = GetDouble("population.happiness_growth_modifier", config.happiness_growth_modifier);
            config.famine_threshold = GetDouble("population.famine_threshold", config.famine_threshold);
            config.plague_base_chance = GetDouble("population.plague_base_chance", config.plague_base_chance);
            return config;
        }

        // Hot reload methods
        bool GameConfig::CheckForConfigurationUpdates() {
            return CheckForChanges();
        }

        void GameConfig::ForceReloadConfiguration() {
            if (!m_current_filepath.empty()) {
                LoadFromFile(m_current_filepath);
            }
        }

        // ============================================================================
        // Enhanced Configuration Methods
        // ============================================================================

        std::unordered_map<std::string, Json::Value> GameConfig::GetSection(const std::string& section_path) const {
            std::unique_lock<std::shared_mutex> lock(m_config_mutex);
            std::unordered_map<std::string, Json::Value> result;

            try {
                Json::Value section = NavigateToPath(section_path);
                if (section.isObject()) {
                    for (auto it = section.begin(); it != section.end(); ++it) {
                        result[it.key().asString()] = *it;
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "[GameConfig] Error getting section '" << section_path << "': " << e.what() << std::endl;
            }

            return result;
        }

        std::vector<int> GameConfig::GetIntArray(const std::string& key, const std::vector<int>& default_value) const {
            return GetValue<std::vector<int>>(key, default_value);
        }

        std::vector<double> GameConfig::GetDoubleArray(const std::string& key, const std::vector<double>& default_value) const {
            return GetValue<std::vector<double>>(key, default_value);
        }

        std::vector<std::string> GameConfig::GetStringArray(const std::string& key, const std::vector<std::string>& default_value) const {
            return GetValue<std::vector<std::string>>(key, default_value);
        }

        // ============================================================================
        // Configuration Validation
        // ============================================================================

        GameConfig::ValidationResult GameConfig::ValidateAllSections() const {
            ValidationResult result;
            
            auto economics_result = ValidateEconomicsSection();
            auto buildings_result = ValidateBuildingsSection();
            auto military_result = ValidateMilitarySection();
            auto system_result = ValidateSystemSection();
            
            // Combine all results
            auto combine = [&result](const ValidationResult& section_result) {
                if (!section_result.is_valid) result.is_valid = false;
                result.errors.insert(result.errors.end(), section_result.errors.begin(), section_result.errors.end());
                result.warnings.insert(result.warnings.end(), section_result.warnings.begin(), section_result.warnings.end());
            };
            
            combine(economics_result);
            combine(buildings_result);
            combine(military_result);
            combine(system_result);
            
            return result;
        }

        GameConfig::ValidationResult GameConfig::ValidateSection(const std::string& section) const {
            if (section == "economics") return ValidateEconomicsSection();
            if (section == "buildings") return ValidateBuildingsSection();
            if (section == "military") return ValidateMilitarySection();
            if (section == "system") return ValidateSystemSection();
            
            ValidationResult result;
            result.AddError("Unknown validation section: " + section);
            return result;
        }

        GameConfig::ValidationResult GameConfig::ValidateEconomicsSection() const {
            ValidationResult result;
            
            try {
                auto tax_rate = GetValue<double>("economics.tax.base_rate", -1.0);
                if (tax_rate < 0.0 || tax_rate > 1.0) {
                    result.AddError("Economics: Tax base rate must be between 0.0 and 1.0, got: " + std::to_string(tax_rate));
                }
                
                auto autonomy_penalty = GetValue<double>("economics.tax.autonomy_penalty_multiplier", -1.0);
                if (autonomy_penalty < 0.0 || autonomy_penalty > 1.0) {
                    result.AddError("Economics: Autonomy penalty must be between 0.0 and 1.0, got: " + std::to_string(autonomy_penalty));
                }
                
                auto efficiency_range = GetValue<std::vector<double>>("economics.trade.base_efficiency_range", {});
                if (efficiency_range.size() != 2) {
                    result.AddError("Economics: Trade efficiency range must have exactly 2 values [min, max]");
                } else if (efficiency_range[0] >= efficiency_range[1]) {
                    result.AddError("Economics: Trade efficiency range invalid - min must be less than max");
                }
                
            } catch (const std::exception& e) {
                result.AddError("Economics validation exception: " + std::string(e.what()));
            }
            
            return result;
        }

        GameConfig::ValidationResult GameConfig::ValidateBuildingsSection() const {
            ValidationResult result;
            
            try {
                std::vector<std::string> required_buildings = {"tax_office", "market", "fortification"};
                
                for (const auto& building : required_buildings) {
                    std::string base_path = "buildings." + building;
                    
                    auto base_cost = GetValue<int>(base_path + ".base_cost", -1);
                    if (base_cost <= 0) {
                        result.AddError("Buildings: '" + building + "' has invalid base cost: " + std::to_string(base_cost));
                    }
                    
                    auto cost_multiplier = GetValue<double>(base_path + ".cost_multiplier", 0.0);
                    if (cost_multiplier <= 1.0) {
                        result.AddError("Buildings: '" + building + "' cost multiplier must be > 1.0, got: " + std::to_string(cost_multiplier));
                    }
                }
                
            } catch (const std::exception& e) {
                result.AddError("Buildings validation exception: " + std::string(e.what()));
            }
            
            return result;
        }

        GameConfig::ValidationResult GameConfig::ValidateMilitarySection() const {
            ValidationResult result;
            
            try {
                auto units_section = GetSection("military.units");
                if (units_section.empty()) {
                    result.AddError("Military: Units section is missing or empty");
                }
                
                for (const auto& [unit_name, unit_data] : units_section) {
                    if (!unit_data.isObject()) continue;
                    
                    if (!unit_data.isMember("cost")) {
                        result.AddError("Military: Unit '" + unit_name + "' missing cost field");
                    }
                    
                    if (!unit_data.isMember("combat_strength")) {
                        result.AddError("Military: Unit '" + unit_name + "' missing combat_strength field");
                    }
                }
                
            } catch (const std::exception& e) {
                result.AddError("Military validation exception: " + std::string(e.what()));
            }
            
            return result;
        }

        GameConfig::ValidationResult GameConfig::ValidateSystemSection() const {
            ValidationResult result;
            
            try {
                auto thread_pool_size = GetValue<int>("system.threading.thread_pool_size", 0);
                if (thread_pool_size < 1 || thread_pool_size > 32) {
                    result.AddError("System: Thread pool size must be between 1 and 32, got: " + std::to_string(thread_pool_size));
                }
                
                auto target_fps = GetValue<int>("system.performance.target_fps", 0);
                if (target_fps < 30 || target_fps > 240) {
                    result.AddWarning("System: Target FPS outside typical range [30, 240]: " + std::to_string(target_fps));
                }
                
            } catch (const std::exception& e) {
                result.AddError("System validation exception: " + std::string(e.what()));
            }
            
            return result;
        }

        // ============================================================================
        // Simple Formula Evaluation
        // ============================================================================

        double GameConfig::EvaluateFormula(const std::string& formula, const std::unordered_map<std::string, double>& variables) const {
            std::unique_lock<std::shared_mutex> lock(m_formula_mutex);
            
            auto it = m_formulas.find(formula);
            if (it != m_formulas.end()) {
                return EvaluateSimpleExpression(it->second, variables);
            }
            
            // Try to evaluate the formula string directly
            return EvaluateSimpleExpression(formula, variables);
        }

        bool GameConfig::HasFormula(const std::string& formula_name) const {
            std::unique_lock<std::shared_mutex> lock(m_formula_mutex);
            return m_formulas.find(formula_name) != m_formulas.end();
        }

        // ============================================================================
        // Configuration Export/Import
        // ============================================================================

        bool GameConfig::ExportConfig(const std::string& filepath) const {
            return SaveToFile(filepath);
        }

        bool GameConfig::LoadConfigOverride(const std::string& filepath) {
            std::unique_lock<std::shared_mutex> lock(m_config_mutex);
            
            try {
                std::ifstream file(filepath);
                if (!file.is_open()) {
                    std::cerr << "[GameConfig] Cannot open override file: " << filepath << std::endl;
                    return false;
                }
                
                Json::CharReaderBuilder reader_builder;
                std::string errors;
                Json::Value override_config;
                
                if (!Json::parseFromStream(reader_builder, file, &override_config, &errors)) {
                    std::cerr << "[GameConfig] Failed to parse override file: " << errors << std::endl;
                    return false;
                }
                
                // Merge override into main config
                MergeJson(m_config_data, override_config);
                
                std::cout << "[GameConfig] Loaded config override from: " << filepath << std::endl;
                return true;
                
            } catch (const std::exception& e) {
                std::cerr << "[GameConfig] Exception loading override: " << e.what() << std::endl;
                return false;
            }
        }

        void GameConfig::CreateDefaultConfig() {
            std::unique_lock<std::shared_mutex> lock(m_config_mutex);
            
            m_config_data = Json::Value(Json::objectValue);
            
            // System defaults
            m_config_data["system"]["version"] = "1.0.0";
            m_config_data["system"]["threading"]["enable_threading"] = true;
            m_config_data["system"]["threading"]["thread_pool_size"] = 4;
            m_config_data["system"]["performance"]["target_fps"] = 60;
            
            // Economics defaults
            m_config_data["economics"]["tax"]["base_rate"] = 0.12;
            m_config_data["economics"]["tax"]["autonomy_penalty_multiplier"] = 0.75;
            m_config_data["economics"]["trade"]["market_bonus_per_level"] = 0.25;
            
            // Buildings defaults
            m_config_data["buildings"]["tax_office"]["base_cost"] = 150;
            m_config_data["buildings"]["tax_office"]["cost_multiplier"] = 1.5;
            m_config_data["buildings"]["market"]["base_cost"] = 200;
            m_config_data["buildings"]["market"]["cost_multiplier"] = 1.4;
            
            std::cout << "[GameConfig] Default configuration created" << std::endl;
        }

        // ============================================================================
        // Statistics and Info
        // ============================================================================

        size_t GameConfig::GetConfigSize() const {
            std::unique_lock<std::shared_mutex> lock(m_config_mutex);
            return m_config_data.size();
        }

        std::chrono::system_clock::time_point GameConfig::GetLastReloadTime() const {
            return m_last_reload_time;
        }

        std::vector<std::string> GameConfig::GetLoadedFiles() const {
            return m_loaded_files;
        }

        // ============================================================================
        // Helper Methods
        // ============================================================================

        std::vector<std::string> GameConfig::SplitConfigPath(const std::string& path) const {
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

        Json::Value GameConfig::NavigateToPath(const std::string& path) const {
            auto keys = SplitConfigPath(path);
            Json::Value current = m_config_data;
            
            for (const auto& key : keys) {
                if (!current.isMember(key)) {
                    return Json::Value();  // Return null if path doesn't exist
                }
                current = current[key];
            }
            
            return current;
        }

        void GameConfig::MergeJson(Json::Value& target, const Json::Value& source) {
            if (!source.isObject()) return;
            
            for (auto it = source.begin(); it != source.end(); ++it) {
                std::string key = it.key().asString();
                if (it->isObject() && target.isMember(key) && target[key].isObject()) {
                    // Recursive merge for nested objects
                    MergeJson(target[key], *it);
                } else {
                    // Direct assignment for values
                    target[key] = *it;
                }
            }
        }

        double GameConfig::EvaluateSimpleExpression(const std::string& expression, const std::unordered_map<std::string, double>& vars) const {
            try {
                std::string substituted = SubstituteVariables(expression, vars);
                
                // Very basic evaluation - in production you'd use a proper expression parser
                // For now, try direct conversion for simple values
                return std::stod(substituted);
                
            } catch (const std::exception&) {
                // Fallback to average of variable values
                if (!vars.empty()) {
                    double sum = 0.0;
                    for (const auto& [_, value] : vars) {
                        sum += value;
                    }
                    return sum / vars.size();
                }
                return 0.0;
            }
        }

        std::string GameConfig::SubstituteVariables(const std::string& formula, const std::unordered_map<std::string, double>& vars) const {
            std::string result = formula;
            
            for (const auto& [var_name, value] : vars) {
                std::string placeholder = "${" + var_name + "}";
                size_t pos = 0;
                while ((pos = result.find(placeholder, pos)) != std::string::npos) {
                    result.replace(pos, placeholder.length(), std::to_string(value));
                    pos += std::to_string(value).length();
                }
            }
            
            return result;
        }

    } // namespace config
} // namespace game
