// ISerializable.h
// Created: October 9, 2025 at 3:45 PM
// Location: include/core/ECS
// Purpose: Base interface for all serializable game objects

#pragma once

#include <string>
#include <vector>

// Forward declarations for serialization system
namespace Json {
    class Value;
}

namespace game::core {

/**
 * @brief Base interface for all objects that can be saved/loaded
 * 
 * All game systems and components that need persistence must implement
 * this interface to work with the SaveManager system.
 */
class ISerializable {
public:
    virtual ~ISerializable() = default;

    /**
     * @brief Serialize object state to JSON
     * @param version Save format version number
     * @return JSON value containing serialized state
     */
    virtual Json::Value Serialize(int version) const = 0;

    /**
     * @brief Deserialize object state from JSON
     * @param data JSON value containing saved state
     * @param version Save format version that was used
     * @return true if deserialization succeeded, false otherwise
     */
    virtual bool Deserialize(const Json::Value& data, int version) = 0;

    /**
     * @brief Get unique name for this serializable object
     * Used for save file organization and validation
     * @return String identifier (e.g., "PopulationSystem", "EconomicComponent")
     */
    virtual std::string GetSystemName() const = 0;
};

} // namespace game::core
