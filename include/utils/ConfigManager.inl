// Created: September 22, 2025, 4:47 PM
// Location: include/core/config/ConfigManager.inl

#pragma once

namespace core::config {

    // ============================================================================
    // ConfigManager Template Implementation
    // ============================================================================

    template<typename T>
    T ConfigManager::GetValue(const std::string& path, const T& default_value) const {
        std::shared_lock<std::shared_mutex> lock(m_config_mutex);
        
        if (!m_initialized) {
            LogWarning("ConfigManager not initialized, returning default for: " + path);
            return default_value;
        }
        
        if (!IsValidPath(path)) {
            LogError("Invalid config path format: " + path);
            return default_value;
        }
        
        try {
            auto keys = SplitConfigPath(path);
            json current = m_merged_config;
            
            // Navigate through the JSON structure
            for (const auto& key : keys) {
                if (!current.contains(key)) {
                    LogWarning("Config path not found: " + path + " (missing key: " + key + ")");
                    return default_value;
                }
                current = current[key];
            }
            
            // Type-safe conversion with validation
            if constexpr (std::is_same_v<T, bool>) {
                if (!current.is_boolean()) {
                    LogWarning("Config value at '" + path + "' is not boolean, using default");
                    return default_value;
                }
            } else if constexpr (std::is_integral_v<T>) {
                if (!current.is_number_integer()) {
                    LogWarning("Config value at '" + path + "' is not integer, using default");
                    return default_value;
                }
            } else if constexpr (std::is_floating_point_v<T>) {
                if (!current.is_number()) {
                    LogWarning("Config value at '" + path + "' is not numeric, using default");
                    return default_value;
                }
            } else if constexpr (std::is_same_v<T, std::string>) {
                if (!current.is_string()) {
                    LogWarning("Config value at '" + path + "' is not string, using default");
                    return default_value;
                }
            } else if constexpr (std::is_same_v<T, std::vector<double>>) {
                if (!current.is_array()) {
                    LogWarning("Config value at '" + path + "' is not array, using default");
                    return default_value;
                }
                // Validate all elements are numeric
                for (const auto& element : current) {
                    if (!element.is_number()) {
                        LogWarning("Config array at '" + path + "' contains non-numeric elements, using default");
                        return default_value;
                    }
                }
            } else if constexpr (std::is_same_v<T, std::vector<int>>) {
                if (!current.is_array()) {
                    LogWarning("Config value at '" + path + "' is not array, using default");
                    return default_value;
                }
                // Validate all elements are integers
                for (const auto& element : current) {
                    if (!element.is_number_integer()) {
                        LogWarning("Config array at '" + path + "' contains non-integer elements, using default");
                        return default_value;
                    }
                }
            } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                if (!current.is_array()) {
                    LogWarning("Config value at '" + path + "' is not array, using default");
                    return default_value;
                }
                // Validate all elements are strings
                for (const auto& element : current) {
                    if (!element.is_string()) {
                        LogWarning("Config array at '" + path + "' contains non-string elements, using default");
                        return default_value;
                    }
                }
            }
            
            return current.get<T>();
            
        } catch (const json::type_error& e) {
            LogError("Type conversion error for config '" + path + "': " + e.what());
            return default_value;
        } catch (const json::out_of_range& e) {
            LogError("Out of range error for config '" + path + "': " + e.what());
            return default_value;
        } catch (const std::exception& e) {
            LogError("Unexpected error getting config value '" + path + "': " + e.what());
            return default_value;
        }
    }

    template<typename T>
    void ConfigManager::SetValue(const std::string& path, const T& value) {
        std::unique_lock<std::shared_mutex> lock(m_config_mutex);
        
        if (!m_initialized) {
            LogError("Cannot set config value - ConfigManager not initialized");
            return;
        }
        
        if (!IsValidPath(path)) {
            LogError("Invalid config path format: " + path);
            return;
        }
        
        try {
            auto keys = SplitConfigPath(path);
            json* current = &m_runtime_overrides;
            
            // Navigate to the parent object, creating intermediate objects as needed
            for (size_t i = 0; i < keys.size() - 1; ++i) {
                if (!current->contains(keys[i])) {
                    (*current)[keys[i]] = json::object();
                }
                
                if (!(*current)[keys[i]].is_object()) {
                    LogError("Config path '" + path + "' conflicts with existing non-object value");
                    return;
                }
                
                current = &(*current)[keys[i]];
            }
            
            // Store old value for change notification
            json old_value;
            if (current->contains(keys.back())) {
                old_value = (*current)[keys.back()];
            }
            
            // Set new value with type validation
            json new_json_value;
            if constexpr (std::is_same_v<T, bool>) {
                new_json_value = static_cast<bool>(value);
            } else if constexpr (std::is_integral_v<T>) {
                // Ensure integer types are stored as integers, not floating point
                new_json_value = static_cast<int64_t>(value);
            } else if constexpr (std::is_floating_point_v<T>) {
                if (std::isfinite(value)) {
                    new_json_value = static_cast<double>(value);
                } else {
                    LogError("Cannot set non-finite floating point value for '" + path + "'");
                    return;
                }
            } else if constexpr (std::is_same_v<T, std::string>) {
                new_json_value = value;
            } else if constexpr (std::is_same_v<T, const char*>) {
                new_json_value = std::string(value);
            } else if constexpr (std::is_same_v<T, std::vector<double>>) {
                json array = json::array();
                for (const auto& element : value) {
                    if (std::isfinite(element)) {
                        array.push_back(element);
                    } else {
                        LogError("Cannot set array with non-finite values for '" + path + "'");
                        return;
                    }
                }
                new_json_value = array;
            } else if constexpr (std::is_same_v<T, std::vector<int>>) {
                json array = json::array();
                for (const auto& element : value) {
                    array.push_back(static_cast<int64_t>(element));
                }
                new_json_value = array;
            } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                json array = json::array();
                for (const auto& element : value) {
                    array.push_back(element);
                }
                new_json_value = array;
            } else {
                // For other types, try direct assignment
                new_json_value = value;
            }
            
            (*current)[keys.back()] = new_json_value;
            RebuildMergedConfig();
            
            // Notify change listeners (unlock first to avoid deadlock)
            lock.unlock();
            NotifyConfigChanged(path, old_value, new_json_value);
            
            // Log the change
            if (m_log_level == "DEBUG") {
                std::ostringstream oss;
                oss << "Config value changed: " << path << " = ";
                if (new_json_value.is_string()) {
                    oss << "\"" << new_json_value.get<std::string>() << "\"";
                } else {
                    oss << new_json_value.dump();
                }
                LogInfo(oss.str());
            }
            
        } catch (const json::type_error& e) {
            LogError("Type error setting config '" + path + "': " + e.what());
        } catch (const std::exception& e) {
            LogError("Error setting config value '" + path + "': " + e.what());
        }
    }

    // Specializations for common types to optimize performance
    template<>
    inline bool ConfigManager::GetValue<bool>(const std::string& path, const bool& default_value) const {
        std::shared_lock<std::shared_mutex> lock(m_config_mutex);
        
        if (!m_initialized) return default_value;
        
        try {
            auto keys = SplitConfigPath(path);
            json current = m_merged_config;
            
            for (const auto& key : keys) {
                if (!current.contains(key)) return default_value;
                current = current[key];
            }
            
            if (current.is_boolean()) {
                return current.get<bool>();
            } else if (current.is_number_integer()) {
                // Allow integer to bool conversion (0 = false, non-zero = true)
                return current.get<int>() != 0;
            } else if (current.is_string()) {
                // Allow string to bool conversion
                std::string str_value = current.get<std::string>();
                std::transform(str_value.begin(), str_value.end(), str_value.begin(), ::tolower);
                return (str_value == "true" || str_value == "1" || str_value == "yes" || str_value == "on");
            }
            
            return default_value;
            
        } catch (const std::exception&) {
            return default_value;
        }
    }

    template<>
    inline int ConfigManager::GetValue<int>(const std::string& path, const int& default_value) const {
        std::shared_lock<std::shared_mutex> lock(m_config_mutex);
        
        if (!m_initialized) return default_value;
        
        try {
            auto keys = SplitConfigPath(path);
            json current = m_merged_config;
            
            for (const auto& key : keys) {
                if (!current.contains(key)) return default_value;
                current = current[key];
            }
            
            if (current.is_number_integer()) {
                return current.get<int>();
            } else if (current.is_number_float()) {
                // Allow float to int conversion with rounding
                return static_cast<int>(std::round(current.get<double>()));
            } else if (current.is_string()) {
                // Allow string to int conversion
                try {
                    return std::stoi(current.get<std::string>());
                } catch (const std::exception&) {
                    return default_value;
                }
            }
            
            return default_value;
            
        } catch (const std::exception&) {
            return default_value;
        }
    }

    template<>
    inline double ConfigManager::GetValue<double>(const std::string& path, const double& default_value) const {
        std::shared_lock<std::shared_mutex> lock(m_config_mutex);
        
        if (!m_initialized) return default_value;
        
        try {
            auto keys = SplitConfigPath(path);
            json current = m_merged_config;
            
            for (const auto& key : keys) {
                if (!current.contains(key)) return default_value;
                current = current[key];
            }
            
            if (current.is_number()) {
                return current.get<double>();
            } else if (current.is_string()) {
                // Allow string to double conversion
                try {
                    return std::stod(current.get<std::string>());
                } catch (const std::exception&) {
                    return default_value;
                }
            }
            
            return default_value;
            
        } catch (const std::exception&) {
            return default_value;
        }
    }

    template<>
    inline std::string ConfigManager::GetValue<std::string>(const std::string& path, const std::string& default_value) const {
        std::shared_lock<std::shared_mutex> lock(m_config_mutex);
        
        if (!m_initialized) return default_value;
        
        try {
            auto keys = SplitConfigPath(path);
            json current = m_merged_config;
            
            for (const auto& key : keys) {
                if (!current.contains(key)) return default_value;
                current = current[key];
            }
            
            if (current.is_string()) {
                return current.get<std::string>();
            } else if (current.is_number()) {
                // Allow number to string conversion
                return std::to_string(current.get<double>());
            } else if (current.is_boolean()) {
                // Allow bool to string conversion
                return current.get<bool>() ? "true" : "false";
            }
            
            return default_value;
            
        } catch (const std::exception&) {
            return default_value;
        }
    }

} // namespace core::config
