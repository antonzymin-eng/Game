// ISystem.h
// Created: October 9, 2025 at 3:47 PM
// Location: include/core/
// Purpose: Base interface for all game systems

#pragma once

#include "core/ECS/ISerializable.h"
#include "core/threading/ThreadedSystemManager.h"
#include <string>

namespace game::core {

/**
 * @brief Base interface for all game systems
 * 
 * All systems in Mechanica Imperii must implement this interface.
 * Provides lifecycle management, threading control, and serialization.
 */
class ISystem : public ISerializable {
public:
    virtual ~ISystem() = default;

    /**
     * @brief Initialize system resources and register components
     * Called once during game startup before first Update()
     */
    virtual void Initialize() = 0;

    /**
     * @brief Update system logic
     * @param deltaTime Time since last update in seconds
     * Called every frame or according to system's update frequency
     */
    virtual void Update(float deltaTime) = 0;

    /**
     * @brief Shutdown system and cleanup resources
     * Called once during game shutdown after last Update()
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Get threading strategy for this system
     * @return Threading strategy (MAIN_THREAD, THREAD_POOL, DEDICATED_THREAD)
     * 
     * Threading strategies:
     * - MAIN_THREAD: UI systems, rendering (ImGui, OpenGL requirements)
     * - THREAD_POOL: CPU-intensive calculations (population, economics, military)
     * - DEDICATED_THREAD: Continuous processing (AIDirector)
     * - MAIN_THREAD_ONLY: Strict main thread requirement (render systems)
     */
    virtual threading::ThreadingStrategy GetThreadingStrategy() const = 0;

    /**
     * @brief Get human-readable system name
     * @return Unique system identifier (e.g., "PopulationSystem")
     * Used for logging, debugging, and save file organization
     */
    std::string GetSystemName() const override = 0;

    // Inherited from ISerializable:
    // virtual Json::Value Serialize(int version) const = 0;
    // virtual bool Deserialize(const Json::Value& data, int version) = 0;
};

} // namespace game::core
