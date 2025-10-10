// ============================================================================
// Date/Time Created: September 27, 2025 - 7:45 PM PST
// Intended Folder Location: include/core/config/GameConfig.h
// GameConfig - Enhanced Configuration Manager with Hot Reload
// ============================================================================

#pragma once

#include <string>
#include <unordered_map>
#include <jsoncpp/json/json.h>
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
            bool ValidateConfiguration() const;
            std::vector<std::string> GetValidationErrors() const;

            // Section management
            std::vector<std::string> GetKeysWithPrefix(const std::string& prefix) const;
            std::vector<std::string> GetAllSections() const;
            bool HasSection(const std::string& section) const;

            // Debug utilities
            void PrintAllConfig() const;
            void PrintSection(const std::string& section) const;
            std::string GetConfigSummary() const;

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
            bool ValidateSection(const std::string& section) const;
            bool ValidateNumericRange(const std::string& key, double value, double min_val, double max_val) const;

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
        };

    } // namespace config
} // namespace game
