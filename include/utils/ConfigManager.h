// Created: September 22, 2025, 4:47 PM
// Location: include/core/config/ConfigManager.h

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <filesystem>
#include <chrono>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <optional>
#include <jsoncpp/json/json.h>

using json = Json::Value;

namespace core::config {

    // Forward declarations
    class FormulaEngine;
    class ProvinceConfigAdapter;
    
    // Configuration validation result
    struct ConfigValidationResult {
        bool is_valid = false;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        
        void AddError(const std::string& error);
        void AddWarning(const std::string& warning);
        bool HasIssues() const;
    };

    // Configuration change notification
    struct ConfigChangeEvent {
        std::string path;
        json old_value;
        json new_value;
        std::chrono::system_clock::time_point timestamp;
    };

    // Main configuration manager class
    class ConfigManager {
    public:
        // Singleton access
        static ConfigManager& Instance();
        
        // Initialization and shutdown
        void Initialize(const std::string& config_dir = "config/");
        void Shutdown();
        
        // Configuration access (thread-safe)
        template<typename T>
        T GetValue(const std::string& path, const T& default_value = T{}) const;
        
        template<typename T>
        void SetValue(const std::string& path, const T& value);
        
        // Batch operations
        void SetValues(const std::unordered_map<std::string, json>& values);
        std::unordered_map<std::string, json> GetSection(const std::string& section_path) const;
        
        // File management
        void CheckForUpdates();
        void ReloadConfigs();
        void SaveCurrentConfig(const std::string& filename) const;
        void LoadConfigOverride(const std::string& filename);
        
        // Validation
        ConfigValidationResult ValidateAllConfigs() const;
        ConfigValidationResult ValidateSection(const std::string& section) const;
        
        // Change notifications
        using ChangeCallback = std::function<void(const ConfigChangeEvent&)>;
        void RegisterChangeCallback(const std::string& path, ChangeCallback callback);
        void UnregisterChangeCallback(const std::string& path);
        
        // Development features
        void EnableHotReload(bool enable = true);
        void EnableValidationLogging(bool enable = true);
        void SetLogLevel(const std::string& level);
        
        // Debugging and diagnostics
        void ExportMergedConfig(const std::string& filename) const;
        std::vector<std::string> GetLoadedFiles() const;
        std::chrono::system_clock::time_point GetLastReloadTime() const;
        size_t GetConfigSize() const;
        
        // Thread safety verification
        bool IsInitialized() const;
        
    private:
        ConfigManager() = default;
        ~ConfigManager() = default;
        
        // Prevent copying
        ConfigManager(const ConfigManager&) = delete;
        ConfigManager& operator=(const ConfigManager&) = delete;
        
        // Internal file operations
        void LoadAllConfigs();
        void LoadConfigFile(const std::string& filename);
        bool CreateDefaultConfigs();
        void SetupFileWatching();
        
        // Configuration merging
        void RebuildMergedConfig();
        void MergeJson(json& target, const json& source);
        
        // Path utilities
        std::vector<std::string> SplitConfigPath(const std::string& path) const;
        bool IsValidPath(const std::string& path) const;
        
        // Change notification system
        void NotifyConfigChanged(const std::string& path, const json& old_value, const json& new_value);
        
        // Validation methods
        ConfigValidationResult ValidateEconomicConfig() const;
        ConfigValidationResult ValidateBuildingConfig() const;
        ConfigValidationResult ValidateMilitaryConfig() const;
        ConfigValidationResult ValidateUIConfig() const;
        ConfigValidationResult ValidateSystemConfig() const;
        
        // Logging
        void LogInfo(const std::string& message) const;
        void LogWarning(const std::string& message) const;
        void LogError(const std::string& message) const;
        
    private:
        // Configuration data
        std::string m_config_directory;
        json m_base_config;
        json m_runtime_overrides;
        json m_merged_config;
        
        // File tracking
        std::unordered_map<std::string, std::filesystem::file_time_type> m_file_timestamps;
        std::vector<std::string> m_loaded_files;
        std::chrono::system_clock::time_point m_last_reload_time;
        
        // Change notification system
        std::unordered_map<std::string, std::vector<ChangeCallback>> m_change_callbacks;
        
        // Thread safety
        mutable std::shared_mutex m_config_mutex;
        mutable std::mutex m_callback_mutex;
        
        // Configuration state
        bool m_initialized = false;
        bool m_enable_hot_reload = true;
        bool m_enable_validation_logging = true;
        std::string m_log_level = "INFO";
    };

    // Formula evaluation engine
    class FormulaEngine {
    public:
        static FormulaEngine& Instance();
        
        // Formula management
        void RegisterFormula(const std::string& name, const std::string& formula);
        void UnregisterFormula(const std::string& name);
        bool HasFormula(const std::string& name) const;
        
        // Evaluation
        double Evaluate(const std::string& formula_name, 
                       const std::unordered_map<std::string, double>& variables);
        std::optional<double> TryEvaluate(const std::string& formula_name,
                                         const std::unordered_map<std::string, double>& variables);
        
        // Formula analysis
        bool ValidateFormula(const std::string& formula) const;
        std::vector<std::string> GetRequiredVariables(const std::string& formula) const;
        std::vector<std::string> GetAvailableFormulas() const;
        
        // Batch operations
        void LoadFormulasFromConfig();
        void ClearAllFormulas();
        
    private:
        FormulaEngine() = default;
        
        double EvaluateExpression(const std::string& expression,
                                 const std::unordered_map<std::string, double>& variables) const;
        std::string PreprocessFormula(const std::string& formula) const;
        std::string SubstituteVariables(const std::string& formula,
                                      const std::unordered_map<std::string, double>& variables) const;
        
    private:
        std::unordered_map<std::string, std::string> m_formulas;
        mutable std::shared_mutex m_formula_mutex;
    };

    // Game-specific configuration adapters
    class ProvinceConfigAdapter {
    public:
        // Economic calculations
        static double CalculateTaxIncome(double base_tax, double admin_efficiency,
                                       double autonomy, double stability);
        static double CalculateTradeIncome(double base_trade, double market_level,
                                         double route_efficiency, double stability);
        static double CalculateMaintenanceCost(double base_cost, double efficiency_modifier);
        
        // Population calculations
        static double CalculatePopulationGrowth(double base_growth, double stability,
                                              double war_exhaustion, double prosperity);
        static double CalculateMigrationRate(double push_factors, double pull_factors);
        
        // Military calculations
        static double CalculateRecruitmentCapacity(double population, double development,
                                                 double military_tech);
        static double CalculateSupplyCapacity(double base_supply, double infrastructure);
        
        // Building calculations
        static int GetBuildingCost(const std::string& building_type, int current_level);
        static double GetBuildingEffect(const std::string& building_type,
                                      const std::string& effect_type, int level);
        static int GetBuildingUpgradeTime(const std::string& building_type, int target_level);
        
        // Technology calculations
        static double CalculateResearchCost(const std::string& tech_category, int current_level);
        static double CalculateResearchSpeed(double base_speed, double research_efficiency);
        
    private:
        static std::unordered_map<std::string, double> CreateVariableMap(
            const std::vector<std::pair<std::string, double>>& variables);
    };

    // Convenience macros for easy migration
    #define CONFIG_VALUE(path, default_val) \
        core::config::ConfigManager::Instance().GetValue<decltype(default_val)>(path, default_val)

    #define CONFIG_FORMULA(name, variables) \
        core::config::FormulaEngine::Instance().Evaluate(name, variables)

    #define CONFIG_VALIDATE_SECTION(section) \
        core::config::ConfigManager::Instance().ValidateSection(section)

} // namespace core::config

// Template implementations
#include "ConfigManager.inl"
