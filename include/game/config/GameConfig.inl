// ============================================================================
// GameConfig Template Implementation
// Location: include/game/config/GameConfig.inl
// ============================================================================

#pragma once

namespace game {
    namespace config {

        template<typename T>
        T GameConfig::GetValue(const std::string& path, const T& default_value) const {
            std::shared_lock<std::shared_mutex> lock(m_config_mutex);
            
            try {
                Json::Value value = NavigateToPath(path);
                if (value.isNull()) {
                    return default_value;
                }
                
                // Type-specific conversions
                if constexpr (std::is_same_v<T, bool>) {
                    if (value.isBool()) return value.asBool();
                    if (value.isString()) {
                        std::string str = value.asString();
                        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
                        return (str == "true" || str == "1" || str == "yes");
                    }
                    return value.asInt() != 0;
                }
                else if constexpr (std::is_same_v<T, int>) {
                    return value.asInt();
                }
                else if constexpr (std::is_same_v<T, double>) {
                    return value.asDouble();
                }
                else if constexpr (std::is_same_v<T, float>) {
                    return static_cast<float>(value.asDouble());
                }
                else if constexpr (std::is_same_v<T, std::string>) {
                    return value.asString();
                }
                else if constexpr (std::is_same_v<T, std::vector<int>>) {
                    std::vector<int> result;
                    if (value.isArray()) {
                        for (const auto& item : value) {
                            result.push_back(item.asInt());
                        }
                    }
                    return result;
                }
                else if constexpr (std::is_same_v<T, std::vector<double>>) {
                    std::vector<double> result;
                    if (value.isArray()) {
                        for (const auto& item : value) {
                            result.push_back(item.asDouble());
                        }
                    }
                    return result;
                }
                else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                    std::vector<std::string> result;
                    if (value.isArray()) {
                        for (const auto& item : value) {
                            result.push_back(item.asString());
                        }
                    }
                    return result;
                }
                else {
                    static_assert(sizeof(T) == 0, "Unsupported type for GameConfig::GetValue");
                }
                
            } catch (const std::exception&) {
                return default_value;
            }
        }

    } // namespace config
} // namespace game