// ============================================================================
// Date/Time Created: September 27, 2025 - 7:45 PM PST
// Intended Folder Location: include/core/config/GameConfig.h
// GameConfig - Enhanced Configuration Manager with Hot Reload
// ============================================================================

#pragma once

#include <string>
#include <unordered_map>
#include "json_compat.h"
#include <mutex>
#include <atomic>
#include <filesystem>
#include <chrono>
#include <functional>
#include <vector>

namespace game {
    namespace config {

        // Configuration change callback type
        using ConfigChangeCallback = std::function<void(const std::string& section)>;

        class GameConfig {
        public:
            // Singleton access
            static GameConfig& Instance();

            // Delete copy/move constructors
            GameConfig(const GameConfig&) = delete;
            GameConfig& operator=(const GameConfig&) = delete;
            GameConfig(GameConfig&&) = delete;
            GameConfig& operator=(GameConfig&&) = delete;

            // Configuration loading
            bool LoadFromFile(const std::string& filepath);
            bool SaveToFile(const std::string& filepath) const;

            // Get values with defaults (thread-safe)
            int GetInt(const std::string& key, int default_value = 0) const;
            double GetDouble(const std::string& key, double default_value = 0.0) const;
            float GetFloat(const std::string& key, float default_value = 0.0f) const;
            bool GetBool(const std::string& key, bool default_value = false) const;
            std::string GetString(const std::string& key, const std::string& default_value = "") const;
            
            // Advanced value access with path notation (e.g., "economics.tax.base_rate")
            template<typename T>
            T GetValue(const std::string& path, const T& default_value = T{}) const;
            
            // Get entire configuration sections
            std::unordered_map<std::string, Json::Value> GetSection(const std::string& section_path) const;
            
            // Array/vector support
            std::vector<int> GetIntArray(const std::string& key, const std::vector<int>& default_value = {}) const;
            std::vector<double> GetDoubleArray(const std::string& key, const std::vector<double>& default_value = {}) const;
            std::vector<std::string> GetStringArray(const std::string& key, const std::vector<std::string>& default_value = {}) const;

            // Set values (thread-safe)
            void SetInt(const std::string& key, int value);
            void SetDouble(const std::string& key, double value);
            void SetFloat(const std::string& key, float value);
            void SetBool(const std::string& key, bool value);
            void SetString(const std::string& key, const std::string& value);

            // Key existence check
            bool HasKey(const std::string& key) const;

            // Hot reload functionality
            bool EnableHotReload(double check_interval_seconds = 1.0);
            void DisableHotReload();
            bool IsHotReloadEnabled() const;
            bool CheckForChanges();  // Manual check, returns true if config changed
            bool Reload();  // Force reload from disk

            // Change notification system
            void RegisterChangeCallback(const std::string& section, ConfigChangeCallback callback);
            void UnregisterChangeCallback(const std::string& section);
            void ClearAllCallbacks();

            // Validation
            struct ValidationResult {
                bool is_valid = true;
                std::vector<std::string> errors;
                std::vector<std::string> warnings;
                
                void AddError(const std::string& error) {
                    errors.push_back(error);
                    is_valid = false;
                }
                
                void AddWarning(const std::string& warning) {
                    warnings.push_back(warning);
                }
                
                bool HasIssues() const {
                    return !errors.empty() || !warnings.empty();
                }
            };
            
            bool ValidateConfiguration() const;
            ValidationResult ValidateAllSections() const;
            ValidationResult ValidateSection(const std::string& section) const;
            std::vector<std::string> GetValidationErrors() const;

            // Configuration structures
            struct CouncilConfiguration {
                double default_delegation_level = 0.5;
                int max_council_members = 12;
                double decision_threshold = 0.6;
            };

            struct ThreadingConfiguration {
                int worker_thread_count = 4;
                int max_systems_per_frame = 10;
                double frame_budget_ms = 16.67;
                bool performance_monitoring = true;
            };

            struct PopulationConfiguration {
                double base_growth_rate = 0.01;
                double happiness_growth_modifier = 0.5;
                double famine_threshold = 0.3;
                double plague_base_chance = 0.02;
                
                template<typename T>
                T GetValue(const std::string& key, T default_value) const {
                    // Delegate to main config system
                    return GameConfig::Instance().GetDouble("population." + key, static_cast<double>(default_value));
                }
            };

            // Static initialization and specialized getters
            static void Initialize(const std::string& config_directory);
            CouncilConfiguration GetCouncilConfiguration() const;
            ThreadingConfiguration GetThreadingConfiguration() const;
            PopulationConfiguration GetPopulationConfiguration() const;
            bool CheckForConfigurationUpdates();
            void ForceReloadConfiguration();

            // Section management
            std::vector<std::string> GetKeysWithPrefix(const std::string& prefix) const;
            std::vector<std::string> GetAllSections() const;
            bool HasSection(const std::string& section) const;

            // Simple formula evaluation
            double EvaluateFormula(const std::string& formula, const std::unordered_map<std::string, double>& variables) const;
            bool HasFormula(const std::string& formula_name) const;
            
            // Configuration export/import
            bool ExportConfig(const std::string& filepath) const;
            bool LoadConfigOverride(const std::string& filepath);
            void CreateDefaultConfig();
            
            // Debug utilities
            void PrintAllConfig() const;
            void PrintSection(const std::string& section) const;
            std::string GetConfigSummary() const;
            
            // Statistics
            size_t GetConfigSize() const;
            std::chrono::system_clock::time_point GetLastReloadTime() const;
            std::vector<std::string> GetLoadedFiles() const;

        private:
            GameConfig();
            ~GameConfig() = default;

            // Hot reload internals
            bool UpdateFileTimestamp();
            bool HasFileChanged() const;
            void NotifyCallbacks(const std::vector<std::string>& changed_sections);
            std::vector<std::string> DetectChangedSections(const Json::Value& old_config, const Json::Value& new_config) const;

            // JSON path parsing
            Json::Value GetValueFromPath(const std::string& path) const;
            void SetValueAtPath(const std::string& path, const Json::Value& value);
            std::vector<std::string> SplitPath(const std::string& path) const;

            // Validation helpers
            ValidationResult ValidateEconomicsSection() const;
            ValidationResult ValidateBuildingsSection() const;
            ValidationResult ValidateMilitarySection() const;
            ValidationResult ValidateSystemSection() const;
            bool ValidateNumericRange(const std::string& key, double value, double min_val, double max_val) const;
            
            // Path utilities
            std::vector<std::string> SplitConfigPath(const std::string& path) const;
            Json::Value NavigateToPath(const std::string& path) const;
            void MergeJson(Json::Value& target, const Json::Value& source);
            
            // Formula evaluation helpers
            double EvaluateSimpleExpression(const std::string& expression, const std::unordered_map<std::string, double>& vars) const;
            std::string SubstituteVariables(const std::string& formula, const std::unordered_map<std::string, double>& vars) const;

            // Member variables
            Json::Value m_config_data;
            Json::Value m_previous_config_data;  // For change detection
            std::string m_current_filepath;
            mutable std::mutex m_config_mutex;

            // Hot reload state
            std::atomic<bool> m_hot_reload_enabled;
            std::filesystem::file_time_type m_last_write_time;
            std::chrono::steady_clock::time_point m_last_check_time;
            std::chrono::milliseconds m_check_interval;

            // Change notification
            std::unordered_map<std::string, ConfigChangeCallback> m_change_callbacks;
            std::mutex m_callback_mutex;
            
            // Enhanced state tracking
            std::chrono::system_clock::time_point m_last_reload_time;
            std::vector<std::string> m_loaded_files;
            std::unordered_map<std::string, std::string> m_formulas;
            mutable std::mutex m_formula_mutex;
        };

    } // namespace config
} // namespace game

// Include template implementations
#include "GameConfig.inl"
