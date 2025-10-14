// Created: September 22, 2025, 4:47 PM// Created: September 22, 2025, 4:47 PM

// Location: include/utils/ConfigManager.inl// Location: include/core/config/ConfigManager.inl

// Fixed: October 12, 2025 - Converted from nlohmann::json to jsoncpp API

#pragma once

#pragma once

namespace core::config {

#include <cmath>

#include <algorithm>    // ============================================================================

#include <cctype>    // ConfigManager Template Implementation

    // ============================================================================

namespace core::config {

    template<typename T>

    template<typename T>    T ConfigManager::GetValue(const std::string& path, const T& default_value) const {

    T ConfigManager::GetValue(const std::string& path, const T& default_value) const {        std::shared_lock<std::shared_mutex> lock(m_config_mutex);

        std::shared_lock<std::shared_mutex> lock(m_config_mutex);        

                if (!m_initialized) {

        if (!m_initialized) {            LogWarning("ConfigManager not initialized, returning default for: " + path);

            LogWarning("ConfigManager not initialized, using default for: " + path);            return default_value;

            return default_value;        }

        }        

                if (!IsValidPath(path)) {

        try {            LogError("Invalid config path format: " + path);

            json current = m_current_config;            return default_value;

            std::vector<std::string> keys = SplitPath(path);        }

                    

            // Navigate to the target value        try {

            for (size_t i = 0; i < keys.size() - 1; ++i) {            auto keys = SplitConfigPath(path);

                const std::string& key = keys[i];            json current = m_merged_config;

                if (!current.isMember(key)) {            

                    LogWarning("Config path '" + path + "' not found, using default");            // Navigate through the JSON structure

                    return default_value;            for (const auto& key : keys) {

                }                if (!current.contains(key)) {

                current = current[key];                    LogWarning("Config path not found: " + path + " (missing key: " + key + ")");

                if (!current.isObject()) {                    return default_value;

                    LogWarning("Config path '" + path + "' contains non-object intermediate, using default");                }

                    return default_value;                current = current[key];

                }            }

            }            

                        // Type-safe conversion with validation

            // Check if final key exists            if constexpr (std::is_same_v<T, bool>) {

            const std::string& final_key = keys.back();                if (!current.is_boolean()) {

            if (!current.isMember(final_key)) {                    LogWarning("Config value at '" + path + "' is not boolean, using default");

                LogWarning("Config key '" + path + "' not found, using default");                    return default_value;

                return default_value;                }

            }            } else if constexpr (std::is_integral_v<T>) {

                            if (!current.is_number_integer()) {

            current = current[final_key];                    LogWarning("Config value at '" + path + "' is not integer, using default");

                                return default_value;

            // Type validation and conversion                }

            if constexpr (std::is_same_v<T, bool>) {            } else if constexpr (std::is_floating_point_v<T>) {

                if (current.isBool()) {                if (!current.is_number()) {

                    return current.asBool();                    LogWarning("Config value at '" + path + "' is not numeric, using default");

                } else if (current.isInt()) {                    return default_value;

                    // Convert integer to bool (0 = false, non-zero = true)                }

                    return current.asInt() != 0;            } else if constexpr (std::is_same_v<T, std::string>) {

                } else if (current.isString()) {                if (!current.is_string()) {

                    // Convert string to bool                    LogWarning("Config value at '" + path + "' is not string, using default");

                    std::string str_value = current.asString();                    return default_value;

                    std::transform(str_value.begin(), str_value.end(), str_value.begin(), ::tolower);                }

                    return (str_value == "true" || str_value == "1" || str_value == "yes");            } else if constexpr (std::is_same_v<T, std::vector<double>>) {

                } else {                if (!current.is_array()) {

                    LogWarning("Config value at '" + path + "' is not convertible to boolean, using default");                    LogWarning("Config value at '" + path + "' is not array, using default");

                    return default_value;                    return default_value;

                }                }

            } else if constexpr (std::is_same_v<T, int>) {                // Validate all elements are numeric

                if (current.isInt()) {                for (const auto& element : current) {

                    return current.asInt();                    if (!element.is_number()) {

                } else if (current.isDouble()) {                        LogWarning("Config array at '" + path + "' contains non-numeric elements, using default");

                    // Convert double to int with rounding                        return default_value;

                    double val = current.asDouble();                    }

                    if (std::isfinite(val)) {                }

                        return static_cast<int>(std::round(val));            } else if constexpr (std::is_same_v<T, std::vector<int>>) {

                    } else {                if (!current.is_array()) {

                        LogWarning("Config value at '" + path + "' is not finite, using default");                    LogWarning("Config value at '" + path + "' is not array, using default");

                        return default_value;                    return default_value;

                    }                }

                } else if (current.isString()) {                // Validate all elements are integers

                    // Try to convert string to int                for (const auto& element : current) {

                    try {                    if (!element.is_number_integer()) {

                        return std::stoi(current.asString());                        LogWarning("Config array at '" + path + "' contains non-integer elements, using default");

                    } catch (const std::exception&) {                        return default_value;

                        LogWarning("Config string at '" + path + "' is not convertible to integer, using default");                    }

                        return default_value;                }

                    }            } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {

                } else {                if (!current.is_array()) {

                    LogWarning("Config value at '" + path + "' is not convertible to integer, using default");                    LogWarning("Config value at '" + path + "' is not array, using default");

                    return default_value;                    return default_value;

                }                }

            } else if constexpr (std::is_same_v<T, double>) {                // Validate all elements are strings

                if (current.isNumeric()) {                for (const auto& element : current) {

                    return current.asDouble();                    if (!element.is_string()) {

                } else if (current.isString()) {                        LogWarning("Config array at '" + path + "' contains non-string elements, using default");

                    // Try to convert string to double                        return default_value;

                    try {                    }

                        return std::stod(current.asString());                }

                    } catch (const std::exception&) {            }

                        LogWarning("Config string at '" + path + "' is not convertible to double, using default");            

                        return default_value;            // Convert based on type T

                    }            if constexpr (std::is_same_v<T, std::string>) {

                } else {                return current.asString();

                    LogWarning("Config value at '" + path + "' is not convertible to double, using default");            } else if constexpr (std::is_same_v<T, int>) {

                    return default_value;                return current.asInt();

                }            } else if constexpr (std::is_same_v<T, double>) {

            } else if constexpr (std::is_same_v<T, std::string>) {                return current.asDouble();

                if (current.isString()) {            } else if constexpr (std::is_same_v<T, bool>) {

                    return current.asString();                return current.asBool();

                } else if (current.isNumeric()) {            } else if constexpr (std::is_same_v<T, std::vector<int>>) {

                    // Convert number to string                std::vector<int> result;

                    return std::to_string(current.asDouble());                for (Json::ArrayIndex i = 0; i < current.size(); ++i) {

                } else if (current.isBool()) {                    result.push_back(current[i].asInt());

                    // Convert bool to string                }

                    return current.asBool() ? "true" : "false";                return result;

                } else {            } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {

                    LogWarning("Config value at '" + path + "' is not convertible to string, using default");                std::vector<std::string> result;

                    return default_value;                for (Json::ArrayIndex i = 0; i < current.size(); ++i) {

                }                    result.push_back(current[i].asString());

            } else if constexpr (std::is_same_v<T, std::vector<double>>) {                }

                if (!current.isArray()) {                return result;

                    LogWarning("Config value at '" + path + "' is not array, using default");            } else {

                    return default_value;                static_assert(sizeof(T) == 0, "Unsupported type for ConfigManager::GetValue");

                }            }

                std::vector<double> result;            

                for (Json::ArrayIndex i = 0; i < current.size(); ++i) {        } catch (const std::exception& e) {

                    const Json::Value& element = current[i];            LogError("Error getting config value '" + path + "': " + e.what());

                    if (element.isNumeric()) {            return default_value;

                        double val = element.asDouble();        }

                        if (std::isfinite(val)) {    }

                            result.push_back(val);

                        } else {    template<typename T>

                            LogWarning("Config array at '" + path + "' contains non-finite values, using default");    void ConfigManager::SetValue(const std::string& path, const T& value) {

                            return default_value;        std::unique_lock<std::shared_mutex> lock(m_config_mutex);

                        }        

                    } else {        if (!m_initialized) {

                        LogWarning("Config array at '" + path + "' contains non-numeric elements, using default");            LogError("Cannot set config value - ConfigManager not initialized");

                        return default_value;            return;

                    }        }

                }        

                return result;        if (!IsValidPath(path)) {

            } else if constexpr (std::is_same_v<T, std::vector<int>>) {            LogError("Invalid config path format: " + path);

                if (!current.isArray()) {            return;

                    LogWarning("Config value at '" + path + "' is not array, using default");        }

                    return default_value;        

                }        try {

                std::vector<int> result;            auto keys = SplitConfigPath(path);

                for (Json::ArrayIndex i = 0; i < current.size(); ++i) {            json* current = &m_runtime_overrides;

                    const Json::Value& element = current[i];            

                    if (element.isInt()) {            // Navigate to the parent object, creating intermediate objects as needed

                        result.push_back(element.asInt());            for (size_t i = 0; i < keys.size() - 1; ++i) {

                    } else if (element.isDouble()) {                if (!current->contains(keys[i])) {

                        double val = element.asDouble();                    (*current)[keys[i]] = json::object();

                        if (std::isfinite(val)) {                }

                            result.push_back(static_cast<int>(std::round(val)));                

                        } else {                if (!(*current)[keys[i]].is_object()) {

                            LogWarning("Config array at '" + path + "' contains non-finite values, using default");                    LogError("Config path '" + path + "' conflicts with existing non-object value");

                            return default_value;                    return;

                        }                }

                    } else {                

                        LogWarning("Config array at '" + path + "' contains non-integer elements, using default");                current = &(*current)[keys[i]];

                        return default_value;            }

                    }            

                }            // Store old value for change notification

                return result;            json old_value;

            } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {            if (current->contains(keys.back())) {

                if (!current.isArray()) {                old_value = (*current)[keys.back()];

                    LogWarning("Config value at '" + path + "' is not array, using default");            }

                    return default_value;            

                }            // Set new value with type validation

                std::vector<std::string> result;            json new_json_value;

                for (Json::ArrayIndex i = 0; i < current.size(); ++i) {            if constexpr (std::is_same_v<T, bool>) {

                    const Json::Value& element = current[i];                new_json_value = static_cast<bool>(value);

                    if (element.isString()) {            } else if constexpr (std::is_integral_v<T>) {

                        result.push_back(element.asString());                // Ensure integer types are stored as integers, not floating point

                    } else {                new_json_value = static_cast<int64_t>(value);

                        LogWarning("Config array at '" + path + "' contains non-string elements, using default");            } else if constexpr (std::is_floating_point_v<T>) {

                        return default_value;                if (std::isfinite(value)) {

                    }                    new_json_value = static_cast<double>(value);

                }                } else {

                return result;                    LogError("Cannot set non-finite floating point value for '" + path + "'");

            } else {                    return;

                static_assert(sizeof(T) == 0, "Unsupported type for ConfigManager::GetValue");                }

            }            } else if constexpr (std::is_same_v<T, std::string>) {

                            new_json_value = value;

        } catch (const std::exception& e) {            } else if constexpr (std::is_same_v<T, const char*>) {

            LogError("Error getting config value '" + path + "': " + e.what());                new_json_value = std::string(value);

            return default_value;            } else if constexpr (std::is_same_v<T, std::vector<double>>) {

        }                json array = json::array();

    }                for (const auto& element : value) {

                    if (std::isfinite(element)) {

    template<typename T>                        array.push_back(element);

    void ConfigManager::SetValue(const std::string& path, const T& value) {                    } else {

        std::unique_lock<std::shared_mutex> lock(m_config_mutex);                        LogError("Cannot set array with non-finite values for '" + path + "'");

                                return;

        if (!m_initialized) {                    }

            LogError("Cannot set config value - ConfigManager not initialized");                }

            return;                new_json_value = array;

        }            } else if constexpr (std::is_same_v<T, std::vector<int>>) {

                        json array = json::array();

        try {                for (const auto& element : value) {

            json* current = &m_current_config;                    array.push_back(static_cast<int64_t>(element));

            std::vector<std::string> keys = SplitPath(path);                }

                            new_json_value = array;

            // Navigate/create path to parent            } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {

            for (size_t i = 0; i < keys.size() - 1; ++i) {                json array = json::array();

                const std::string& key = keys[i];                for (const auto& element : value) {

                if (!current->isMember(key)) {                    array.push_back(element);

                    (*current)[key] = Json::Value(Json::objectValue);                }

                }                new_json_value = array;

                            } else {

                if (!(*current)[key].isObject()) {                // For other types, try direct assignment

                    LogError("Cannot set value at '" + path + "' - intermediate path is not object");                new_json_value = value;

                    return;            }

                }            

                            (*current)[keys.back()] = new_json_value;

                current = &((*current)[key]);            RebuildMergedConfig();

            }            

                        // Notify change listeners (unlock first to avoid deadlock)

            // Set the final value            lock.unlock();

            const std::string& final_key = keys.back();            NotifyConfigChanged(path, old_value, new_json_value);

            if ((*current).isMember(final_key)) {            

                LogInfo("Overwriting existing config value: " + path);            // Log the change

            }            if (m_log_level == "DEBUG") {

                            std::ostringstream oss;

            // Convert value to JSON based on type                oss << "Config value changed: " << path << " = ";

            if constexpr (std::is_same_v<T, std::string>) {                if (new_json_value.is_string()) {

                (*current)[final_key] = value;                    oss << "\"" << new_json_value.get<std::string>() << "\"";

            } else if constexpr (std::is_same_v<T, bool>) {                } else {

                (*current)[final_key] = value;                    oss << new_json_value.dump();

            } else if constexpr (std::is_integral_v<T>) {                }

                (*current)[final_key] = static_cast<int64_t>(value);                LogInfo(oss.str());

            } else if constexpr (std::is_floating_point_v<T>) {            }

                if (std::isfinite(value)) {            

                    (*current)[final_key] = static_cast<double>(value);        } catch (const json::type_error& e) {

                } else {            LogError("Type error setting config '" + path + "': " + e.what());

                    LogError("Cannot set non-finite floating point value at '" + path + "'");        } catch (const std::exception& e) {

                    return;            LogError("Error setting config value '" + path + "': " + e.what());

                }        }

            } else if constexpr (std::is_same_v<T, std::vector<double>>) {    }

                Json::Value array(Json::arrayValue);

                for (const auto& element : value) {    // Specializations for common types to optimize performance

                    if (std::isfinite(element)) {    template<>

                        array.append(element);    inline bool ConfigManager::GetValue<bool>(const std::string& path, const bool& default_value) const {

                    } else {        std::shared_lock<std::shared_mutex> lock(m_config_mutex);

                        LogError("Cannot set array with non-finite values at '" + path + "'");        

                        return;        if (!m_initialized) return default_value;

                    }        

                }        try {

                (*current)[final_key] = array;            auto keys = SplitConfigPath(path);

            } else if constexpr (std::is_same_v<T, std::vector<int>>) {            json current = m_merged_config;

                Json::Value array(Json::arrayValue);            

                for (const auto& element : value) {            for (const auto& key : keys) {

                    array.append(static_cast<int64_t>(element));                if (!current.contains(key)) return default_value;

                }                current = current[key];

                (*current)[final_key] = array;            }

            } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {            

                Json::Value array(Json::arrayValue);            if (current.is_boolean()) {

                for (const auto& element : value) {                return current.get<bool>();

                    array.append(element);            } else if (current.is_number_integer()) {

                }                // Allow integer to bool conversion (0 = false, non-zero = true)

                (*current)[final_key] = array;                return current.get<int>() != 0;

            } else {            } else if (current.is_string()) {

                static_assert(sizeof(T) == 0, "Unsupported type for ConfigManager::SetValue");                // Allow string to bool conversion

                return;                std::string str_value = current.get<std::string>();

            }                std::transform(str_value.begin(), str_value.end(), str_value.begin(), ::tolower);

                            return (str_value == "true" || str_value == "1" || str_value == "yes" || str_value == "on");

            // Convert value to string for logging            }

            std::ostringstream oss;            

            if constexpr (std::is_same_v<T, std::string>) {            return default_value;

                oss << "\"" << value << "\"";            

            } else {        } catch (const std::exception&) {

                Json::StreamWriterBuilder builder;            return default_value;

                builder["indentation"] = "";        }

                std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());    }

                writer->write((*current)[final_key], &oss);

            }    template<>

                inline int ConfigManager::GetValue<int>(const std::string& path, const int& default_value) const {

            LogInfo("Set config value '" + path + "' = " + oss.str());        std::shared_lock<std::shared_mutex> lock(m_config_mutex);

                    

        } catch (const std::exception& e) {        if (!m_initialized) return default_value;

            LogError("Error setting config value '" + path + "': " + e.what());        

        }        try {

    }            auto keys = SplitConfigPath(path);

            json current = m_merged_config;

} // namespace core::config            
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
