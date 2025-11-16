// IComponent.h
// Created: October 9, 2025 at 3:46 PM
// Location: include/core/ECS/
// Purpose: Base interface for all ECS components with extended features

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <typeindex>

namespace game::core {

using ComponentTypeID = uint32_t;

/**
 * @brief Base interface for all ECS components
 * 
 * Provides type identification, cloning, serialization, and validation.
 * All game components must inherit from Component<Derived> template.
 */
class IComponent {
public:
    virtual ~IComponent() = default;

    /**
     * @brief Get runtime type ID for this component
     * @return Unique component type identifier
     */
    virtual ComponentTypeID GetTypeID() const = 0;

    /**
     * @brief Get human-readable component type name
     * @return String name (e.g., "PopulationComponent")
     */
    virtual std::string GetComponentTypeName() const = 0;

    /**
     * @brief Create deep copy of this component
     * @return Unique pointer to cloned component
     */
    virtual std::unique_ptr<IComponent> Clone() const = 0;

    // Serialization support (integrates with save system)
    /**
     * @brief Serialize component data to string
     * @return Serialized JSON string
     */
    virtual std::string Serialize() const { return "{}"; }

    /**
     * @brief Deserialize component data from string
     * @param data JSON string input
     * @return true if deserialization succeeded
     */
    virtual bool Deserialize(const std::string& data) { return true; }

    // Validation support
    /**
     * @brief Check if component data is valid
     * @return true if component state is valid
     */
    virtual bool IsValid() const { return true; }

    /**
     * @brief Get list of validation errors
     * @return Vector of error messages (empty if valid)
     */
    virtual std::vector<std::string> GetValidationErrors() const { return {}; }
};

/**
 * @brief CRTP base for typed components
 * 
 * Automatically provides type name and ensures proper inheritance.
 * Use as: class MyComponent : public Component<MyComponent> { ... };
 */
template<typename Derived>
class Component : public IComponent {
public:
    std::string GetComponentTypeName() const override {
        return typeid(Derived).name();
    }

    std::unique_ptr<IComponent> Clone() const override {
        return std::make_unique<Derived>(static_cast<const Derived&>(*this));
    }

    static std::type_index GetStaticTypeIndex() {
        return std::type_index(typeid(Derived));
    }

    // Implement GetTypeID using type hash
    ComponentTypeID GetTypeID() const override {
        return static_cast<ComponentTypeID>(std::hash<std::type_index>{}(GetStaticTypeIndex()));
    }

    // EntityManager-compatible serialization methods (different signature from IComponent)
    virtual std::string Serialize() const {
        return "";  // Default: empty serialization (can be overridden)
    }

    virtual bool Deserialize(const std::string& data) {
        return true;  // Default: successful deserialization (can be overridden)
    }
};

} // namespace game::core
