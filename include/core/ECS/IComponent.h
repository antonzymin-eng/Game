// IComponent.h
// Created: October 9, 2025 at 3:46 PM
// Location: include/core/ECS/
// Purpose: Base interface for all ECS components with extended features

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <typeindex>

namespace game::core {

// Forward declarations
class JsonWriter;
class JsonReader;

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
     * @brief Serialize component data
     * @param writer JSON writer for output
     */
    virtual void Serialize(JsonWriter& writer) const {}

    /**
     * @brief Deserialize component data
     * @param reader JSON reader for input
     * @return true if deserialization succeeded
     */
    virtual bool Deserialize(const JsonReader& reader) { return true; }

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
};

} // namespace game::core
