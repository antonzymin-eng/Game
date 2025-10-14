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

namespace game {
    namespace config {

        GameConfig::GameConfig() 
            : m_hot_reload_enabled(false)
            , m_check_interval(1000) {
        }

        GameConfig& GameConfig::Instance() {
            static GameConfig instance;
            return instance;
        }

        bool GameConfig::LoadFromFile(const std::string& filepath) {
            std::lock_guard<std::mutex> lock(m_config_mutex);

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

            if (UpdateFileTimestamp()) {
                std::cout << "[GameConfig] Configuration loaded from: " << filepath << std::endl;
                return true;
            }

            return false;
        }

        bool GameConfig::SaveToFile(const std::string& filepath) const {
            std::lock_guard<std::mutex> lock(m_config_mutex);

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
            std::lock_guard<std::mutex> lock(m_config_mutex);
            Json::Value value = GetValueFromPath(key);
            return value.isInt() ? value.asInt() : default_value;
        }

        double GameConfig::GetDouble(const std::string& key, double default_value) const {
            std::lock_guard<std::mutex> lock(m_config_mutex);
            Json::Value value = GetValueFromPath(key);
            return value.isDouble() ? value.asDouble() : default_value;
        }

        float GameConfig::GetFloat(const std::string& key, float default_value) const {
            std::lock_guard<std::mutex> lock(m_config_mutex);
            Json::Value value = GetValueFromPath(key);
            return value.isDouble() ? value.asFloat() : default_value;
        }

        bool GameConfig::GetBool(const std::string& key, bool default_value) const {
            std::lock_guard<std::mutex> lock(m_config_mutex);
            Json::Value value = GetValueFromPath(key);
            return value.isBool() ? value.asBool() : default_value;
        }

        std::string GameConfig::GetString(const std::string& key, const std::string& default_value) const {
            std::lock_guard<std::mutex> lock(m_config_mutex);
            Json::Value value = GetValueFromPath(key);
            return value.isString() ? value.asString() : default_value;
        }

        void GameConfig::SetInt(const std::string& key, int value) {
            std::lock_guard<std::mutex> lock(m_config_mutex);
            SetValueAtPath(key, Json::Value(value));
        }

        void GameConfig::SetDouble(const std::string& key, double value) {
            std::lock_guard<std::mutex> lock(m_config_mutex);
            SetValueAtPath(key, Json::Value(value));
        }

        void GameConfig::SetFloat(const std::string& key, float value) {
            std::lock_guard<std::mutex> lock(m_config_mutex);
            SetValueAtPath(key, Json::Value(value));
        }

        void GameConfig::SetBool(const std::string& key, bool value) {
            std::lock_guard<std::mutex> lock(m_config_mutex);
            SetValueAtPath(key, Json::Value(value));
        }

        void GameConfig::SetString(const std::string& key, const std::string& value) {
            std::lock_guard<std::mutex> lock(m_config_mutex);
            SetValueAtPath(key, Json::Value(value));
        }

        bool GameConfig::HasKey(const std::string& key) const {
            std::lock_guard<std::mutex> lock(m_config_mutex);
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

            std::lock_guard<std::mutex> lock(m_config_mutex);

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
            std::lock_guard<std::mutex> lock(m_callback_mutex);
            m_change_callbacks[section] = callback;
            std::cout << "[GameConfig] Registered change callback for section: " << section << std::endl;
        }

        void GameConfig::UnregisterChangeCallback(const std::string& section) {
            std::lock_guard<std::mutex> lock(m_callback_mutex);
            m_change_callbacks.erase(section);
            std::cout << "[GameConfig] Unregistered change callback for section: " << section << std::endl;
        }

        void GameConfig::ClearAllCallbacks() {
            std::lock_guard<std::mutex> lock(m_callback_mutex);
            m_change_callbacks.clear();
            std::cout << "[GameConfig] Cleared all change callbacks" << std::endl;
        }

        bool GameConfig::ValidateConfiguration() const {
            auto errors = GetValidationErrors();
            return errors.empty();
        }

        std::vector<std::string> GameConfig::GetValidationErrors() const {
            std::vector<std::string> errors;
            std::lock_guard<std::mutex> lock(m_config_mutex);

            auto sections = GetAllSections();
            for (const auto& section : sections) {
                if (!ValidateSection(section)) {
                    errors.push_back("Section validation failed: " + section);
                }
            }

            return errors;
        }

        std::vector<std::string> GameConfig::GetKeysWithPrefix(const std::string& prefix) const {
            std::lock_guard<std::mutex> lock(m_config_mutex);
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
            std::lock_guard<std::mutex> lock(m_config_mutex);
            std::vector<std::string> sections;

            if (m_config_data.isObject()) {
                for (const auto& key : m_config_data.getMemberNames()) {
                    sections.push_back(key);
                }
            }

            return sections;
        }

        bool GameConfig::HasSection(const std::string& section) const {
            std::lock_guard<std::mutex> lock(m_config_mutex);
            return m_config_data.isMember(section);
        }

        void GameConfig::PrintAllConfig() const {
            std::lock_guard<std::mutex> lock(m_config_mutex);
            Json::StreamWriterBuilder writer_builder;
            writer_builder["indentation"] = "  ";
            std::unique_ptr<Json::StreamWriter> writer(writer_builder.newStreamWriter());
            
            std::ostringstream oss;
            writer->write(m_config_data, &oss);
            std::cout << "=== Configuration ===" << std::endl;
            std::cout << oss.str() << std::endl;
        }

        void GameConfig::PrintSection(const std::string& section) const {
            std::lock_guard<std::mutex> lock(m_config_mutex);
            
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
            std::lock_guard<std::mutex> lock(m_config_mutex);
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
            std::lock_guard<std::mutex> lock(m_callback_mutex);

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
                    return Json::Value::null;
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

        bool GameConfig::ValidateSection(const std::string& section) const {
            if (!m_config_data.isMember(section)) {
                return false;
            }

            const Json::Value& section_data = m_config_data[section];

            if (section == "diplomacy") {
                return ValidateNumericRange("diplomacy.max_opinion", 
                    section_data.get("max_opinion", 100).asDouble(), -200, 200);
            } else if (section == "economy") {
                return ValidateNumericRange("economy.starting_treasury", 
                    section_data.get("starting_treasury", 1000).asDouble(), 0, 1000000);
            } else if (section == "military") {
                return ValidateNumericRange("military.recruitment_base_cost", 
                    section_data.get("recruitment_base_cost", 100).asDouble(), 1, 10000);
            }

            return true;
        }

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

    } // namespace config
} // namespace game
