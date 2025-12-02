// ============================================================================
// EntityIDUtils.h - Utilities for safe EntityID type conversions
// ============================================================================
//
// This header provides utilities for converting between core::ecs::EntityID
// (versioned handles) and game::types::EntityID (numeric IDs) safely.
//
// See docs/ENTITY_ID_TYPE_SYSTEM.md for detailed explanation of the type system.
// ============================================================================

#pragma once

#include "core/ECS/EntityManager.h"
#include "core/types/game_types.h"
#include "core/Logger.h"
#include <optional>

namespace game::ecs {

// ============================================================================
// Type Conversion Utilities
// ============================================================================

/**
 * @brief Safely convert core::ecs::EntityID to game::types::EntityID
 *
 * @param ecs_id The ECS entity ID (with version)
 * @return game::types::EntityID The numeric ID
 *
 * @warning This loses version information! Use only when passing to game logic
 *          that doesn't need version checking (UI, serialization, etc.)
 *
 * @note If ecs_id.id > UINT32_MAX, this will log an error and return 0
 */
inline game::types::EntityID ToGameEntityID(const core::ecs::EntityID& ecs_id) {
    // Check for narrowing conversion
    if constexpr (sizeof(game::types::EntityID) < sizeof(decltype(ecs_id.id))) {
        if (ecs_id.id > std::numeric_limits<game::types::EntityID>::max()) {
            CORE_LOG_ERROR("EntityIDUtils",
                "Entity ID overflow: " + std::to_string(ecs_id.id) +
                " exceeds game::types::EntityID max (" +
                std::to_string(std::numeric_limits<game::types::EntityID>::max()) + ")");
            return game::types::INVALID_ENTITY;
        }
    }

    return static_cast<game::types::EntityID>(ecs_id.id);
}

/**
 * @brief Create core::ecs::EntityID from game::types::EntityID with version lookup
 *
 * @param game_id The game logic entity ID (numeric only)
 * @param entity_manager Reference to EntityManager for version lookup
 * @return std::optional<core::ecs::EntityID> The versioned entity ID, or nullopt if invalid
 *
 * @note This performs a lookup to get the current version. Prefer GetComponentById
 *       if you just need component access.
 */
inline std::optional<core::ecs::EntityID> ToECSEntityID(
    game::types::EntityID game_id,
    const core::ecs::EntityManager& entity_manager)
{
    // FIXED: Use GetEntityInfoById to lookup by numeric ID only
    // This works regardless of current entity version
    auto entity_info = entity_manager.GetEntityInfoById(static_cast<uint64_t>(game_id));

    if (!entity_info) {
        CORE_LOG_WARN("EntityIDUtils",
            "Cannot convert game ID " + std::to_string(game_id) +
            " to ECS EntityID: entity not found or inactive");
        return std::nullopt;
    }

    return core::ecs::EntityID(game_id, entity_info->version);
}

/**
 * @brief Validate that a game::types::EntityID refers to an active entity
 *
 * @param game_id The game logic entity ID
 * @param entity_manager Reference to EntityManager
 * @return bool True if entity exists and is active
 */
inline bool IsValidGameEntityID(
    game::types::EntityID game_id,
    const core::ecs::EntityManager& entity_manager)
{
    if (game_id == game::types::INVALID_ENTITY) {
        return false;
    }

    // FIXED: Use GetEntityInfoById to check existence regardless of version
    auto entity_info = entity_manager.GetEntityInfoById(static_cast<uint64_t>(game_id));
    return static_cast<bool>(entity_info);  // entity_info is non-null only if entity is active
}

/**
 * @brief Get the current version of an entity by numeric ID
 *
 * @param game_id The game logic entity ID
 * @param entity_manager Reference to EntityManager
 * @return std::optional<uint32_t> The current version, or nullopt if entity doesn't exist
 */
inline std::optional<uint32_t> GetEntityVersion(
    game::types::EntityID game_id,
    const core::ecs::EntityManager& entity_manager)
{
    // FIXED: Use GetEntityInfoById to lookup version by numeric ID
    auto entity_info = entity_manager.GetEntityInfoById(static_cast<uint64_t>(game_id));

    if (!entity_info) {
        return std::nullopt;
    }

    return entity_info->version;
}

// ============================================================================
// Component Access Helpers
// ============================================================================

/**
 * @brief Get component with validation and error logging (with custom component name)
 *
 * This is a wrapper around GetComponentById that provides better error messages.
 * Use this in game systems that need clear diagnostics.
 *
 * @tparam ComponentType The component type to retrieve
 * @param game_id The game logic entity ID
 * @param entity_manager Reference to EntityManager
 * @param context String describing where this is called from (for logging)
 * @param component_name Human-readable component name for error messages (e.g., "EconomicComponent")
 * @return std::shared_ptr<ComponentType> The component, or nullptr if not found
 */
template<typename ComponentType>
inline std::shared_ptr<ComponentType> GetComponentSafe(
    game::types::EntityID game_id,
    const core::ecs::EntityManager& entity_manager,
    const std::string& context,
    const std::string& component_name)
{
    if (game_id == game::types::INVALID_ENTITY) {
        if (!context.empty()) {
            CORE_LOG_ERROR("EntityIDUtils",
                context + ": Invalid entity ID (INVALID_ENTITY)");
        }
        return nullptr;
    }

    auto component = entity_manager.GetComponentById<ComponentType>(
        static_cast<uint64_t>(game_id));

    if (!component && !context.empty()) {
        CORE_LOG_WARN("EntityIDUtils",
            context + ": No " + component_name +
            " found for entity " + std::to_string(game_id));
    }

    return component;
}

/**
 * @brief Get component with validation and error logging (component name from typeid)
 *
 * Overload that uses typeid().name() for component name - results in mangled names
 * but doesn't require specifying the name explicitly. Prefer the version above
 * that takes a component_name parameter for better error messages.
 *
 * @tparam ComponentType The component type to retrieve
 * @param game_id The game logic entity ID
 * @param entity_manager Reference to EntityManager
 * @param context String describing where this is called from (for logging)
 * @return std::shared_ptr<ComponentType> The component, or nullptr if not found
 */
template<typename ComponentType>
inline std::shared_ptr<ComponentType> GetComponentSafe(
    game::types::EntityID game_id,
    const core::ecs::EntityManager& entity_manager,
    const std::string& context = "")
{
    // Note: typeid().name() produces compiler-specific mangled names
    // Consider using the overload that takes component_name for better logging
    return GetComponentSafe<ComponentType>(game_id, entity_manager, context, typeid(ComponentType).name());
}

/**
 * @brief Check if entity has component with validation
 *
 * @tparam ComponentType The component type to check for
 * @param game_id The game logic entity ID
 * @param entity_manager Reference to EntityManager
 * @return bool True if entity exists and has the component
 */
template<typename ComponentType>
inline bool HasComponentSafe(
    game::types::EntityID game_id,
    const core::ecs::EntityManager& entity_manager)
{
    if (game_id == game::types::INVALID_ENTITY) {
        return false;
    }

    return entity_manager.HasComponentById<ComponentType>(
        static_cast<uint64_t>(game_id));
}

} // namespace game::ecs
