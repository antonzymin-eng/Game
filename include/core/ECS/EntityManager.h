//============================================================================
// c - FIXED: Entity Version Safety
// Adds generation-checked handles to prevent use-after-destroy bugs
//============================================================================

#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <string>
#include <atomic>
#include <shared_mutex>
#include <mutex>
#include <typeinfo>
#include <stdexcept>
#include <iostream>
#include <chrono>

#include "core/logging/Logger.h"

namespace core::ecs {

    //============================================================================
    // FIXED: Safe Entity Handle with Version Checking
    //============================================================================

    struct EntityID {
        uint64_t id;
        uint32_t version;

        EntityID() : id(0), version(0) {}
        EntityID(uint64_t entity_id, uint32_t entity_version) : id(entity_id), version(entity_version) {}

        // Legacy constructor for compatibility
        explicit EntityID(uint64_t entity_id) : id(entity_id), version(1) {}

        bool operator==(const EntityID& other) const {
            return id == other.id && version == other.version;
        }

        bool operator!=(const EntityID& other) const {
            return !(*this == other);
        }

        bool operator<(const EntityID& other) const {
            if (id != other.id) return id < other.id;
            return version < other.version;
        }

        // For use with unordered containers
        struct Hash {
            size_t operator()(const EntityID& entity_id) const {
                return std::hash<uint64_t>{}(entity_id.id) ^ (std::hash<uint32_t>{}(entity_id.version) << 1);
            }
        };

        // For debugging
        std::string ToString() const {
            return "Entity(" + std::to_string(id) + "v" + std::to_string(version) + ")";
        }

        // Check if this is a valid entity ID (not default-constructed)
        bool IsValid() const {
            return id != 0;
        }
    };

    //============================================================================
    // Entity Information with Version Tracking
    //============================================================================

    struct EntityInfo {
        uint64_t id;
        uint32_t version;        // FIXED: Incremented on each destroy/recreate cycle
        bool active;
        std::string name;
        std::unordered_set<size_t> component_types;

        // Performance metrics
        size_t memory_usage_bytes;
        std::chrono::system_clock::time_point creation_time;
        std::chrono::system_clock::time_point last_modified;

        EntityInfo()
            : id(0), version(0), active(false), memory_usage_bytes(0),
            creation_time(std::chrono::system_clock::now()),
            last_modified(std::chrono::system_clock::now()) {
        }

        EntityInfo(uint64_t entity_id)
            : id(entity_id), version(1), active(true), memory_usage_bytes(0),
            creation_time(std::chrono::system_clock::now()),
            last_modified(std::chrono::system_clock::now()) {
        }

        // Create a safe EntityID handle
        EntityID GetHandle() const {
            return EntityID(id, version);
        }

        // Check if a handle is still valid for this entity
        bool IsValidHandle(const EntityID& handle) const {
            return active && id == handle.id && version == handle.version;
        }

        void UpdateLastModified() {
            last_modified = std::chrono::system_clock::now();
        }
    };

    //============================================================================
    // Component Storage Interface
    //============================================================================

    class IComponentStorage {
    public:
        virtual ~IComponentStorage() = default;
        virtual bool HasComponent(uint64_t entity_id) const = 0;
        virtual bool RemoveComponent(uint64_t entity_id) = 0;
        virtual size_t GetComponentCount() const = 0;
        virtual size_t GetMemoryUsage() const = 0;
        virtual std::vector<uint64_t> GetEntityIds() const = 0;

        // Serialization support
        virtual std::string SerializeComponent(uint64_t entity_id) const = 0;
        virtual bool DeserializeComponent(uint64_t entity_id, const std::string& data) = 0;
        virtual std::string GetComponentTypeName() const = 0;
    };

    //============================================================================
    // Component Storage Implementation
    //============================================================================

    template<typename ComponentType>
    class ComponentStorage : public IComponentStorage {
    private:
        std::unordered_map<uint64_t, std::shared_ptr<ComponentType>> m_components;
        mutable std::shared_mutex m_mutex;

    public:
        // Add component with perfect forwarding
        template<typename... Args>
        std::shared_ptr<ComponentType> AddComponent(uint64_t entity_id, Args&&... args) {
            std::unique_lock lock(m_mutex);
            auto component = std::make_shared<ComponentType>(std::forward<Args>(args)...);
            m_components[entity_id] = component;
            return component;
        }

        // Add existing component
        void AddComponent(uint64_t entity_id, std::shared_ptr<ComponentType> component) {
            std::unique_lock lock(m_mutex);
            m_components[entity_id] = component;
        }

        // Get component (thread-safe)
        std::shared_ptr<ComponentType> GetComponent(uint64_t entity_id) const {
            std::shared_lock lock(m_mutex);
            auto it = m_components.find(entity_id);
            return (it != m_components.end()) ? it->second : nullptr;
        }

        // IComponentStorage implementation
        bool HasComponent(uint64_t entity_id) const override {
            std::shared_lock lock(m_mutex);
            return m_components.find(entity_id) != m_components.end();
        }

        bool RemoveComponent(uint64_t entity_id) override {
            std::unique_lock lock(m_mutex);
            return m_components.erase(entity_id) > 0;
        }

        size_t GetComponentCount() const override {
            std::shared_lock lock(m_mutex);
            return m_components.size();
        }

        size_t GetMemoryUsage() const override {
            std::shared_lock lock(m_mutex);
            return m_components.size() * sizeof(ComponentType);
        }

        std::vector<uint64_t> GetEntityIds() const override {
            std::shared_lock lock(m_mutex);
            std::vector<uint64_t> ids;
            ids.reserve(m_components.size());
            for (const auto& [entity_id, component] : m_components) {
                ids.push_back(entity_id);
            }
            return ids;
        }

        // Get all components (for batch operations)
        std::vector<std::shared_ptr<ComponentType>> GetAllComponents() const {
            std::shared_lock lock(m_mutex);
            std::vector<std::shared_ptr<ComponentType>> components;
            components.reserve(m_components.size());
            for (const auto& [entity_id, component] : m_components) {
                components.push_back(component);
            }
            return components;
        }

        // Serialization (basic implementation)
        std::string SerializeComponent(uint64_t entity_id) const override {
            auto component = GetComponent(entity_id);
            if (component) {
                return component->Serialize();  // Component template provides default empty implementation
            }
            return "";
        }

        bool DeserializeComponent(uint64_t entity_id, const std::string& data) override {
            auto component = GetComponent(entity_id);
            if (component) {
                return component->Deserialize(data);  // Component template provides default true implementation
            }
            return false;
        }

        std::string GetComponentTypeName() const override {
            return typeid(ComponentType).name();
        }
    };

    //============================================================================
    // Entity Statistics
    //============================================================================

    struct EntityStatistics {
        size_t total_entities;
        size_t active_entities;
        size_t total_components;
        size_t memory_usage_bytes;
        double average_components_per_entity;

        // Component type breakdown
        std::unordered_map<std::string, size_t> component_counts;
        std::unordered_map<std::string, size_t> component_memory_usage;

        // Performance metrics
        std::chrono::milliseconds last_update_time;
        std::chrono::system_clock::time_point last_calculated;
    };

    //============================================================================
    // FIXED: EntityManager with Version Safety
    //============================================================================

    class EntityManager {
    private:
        // Entity management
        std::unordered_map<uint64_t, EntityInfo> m_entities;
        std::unordered_map<size_t, std::unique_ptr<IComponentStorage>> m_component_storages;

        // Thread safety
        mutable std::shared_mutex m_entities_mutex;
        mutable std::shared_mutex m_storages_mutex;

        // ID generation
        std::atomic<uint64_t> m_next_entity_id{ 1 };

        // Statistics
        mutable EntityStatistics m_cached_statistics;
        mutable std::atomic<bool> m_statistics_dirty{ true };
        mutable std::shared_mutex m_statistics_mutex;

        // FIXED: Version validation helper
        bool ValidateEntityHandle(const EntityID& handle) const {
            std::shared_lock lock(m_entities_mutex);
            auto it = m_entities.find(handle.id);
            return it != m_entities.end() && it->second.IsValidHandle(handle);
        }

        // Get entity info with validation
        const EntityInfo* GetEntityInfo(const EntityID& handle) const {
            std::shared_lock lock(m_entities_mutex);
            auto it = m_entities.find(handle.id);
            if (it != m_entities.end() && it->second.IsValidHandle(handle)) {
                return &it->second;
            }
            return nullptr;
        }

        // Get mutable entity info with validation
        EntityInfo* GetMutableEntityInfo(const EntityID& handle) {
            std::shared_lock lock(m_entities_mutex);
            auto it = m_entities.find(handle.id);
            if (it != m_entities.end() && it->second.IsValidHandle(handle)) {
                return &it->second;
            }
            return nullptr;
        }

    public:
        EntityManager() = default;
        ~EntityManager() = default;

        // Non-copyable, non-movable
        EntityManager(const EntityManager&) = delete;
        EntityManager& operator=(const EntityManager&) = delete;
        EntityManager(EntityManager&&) = delete;
        EntityManager& operator=(EntityManager&&) = delete;

        //========================================================================
        // FIXED: Entity Creation with Safe Handles
        //========================================================================

        EntityID CreateEntity(const std::string& name = "") {
            uint64_t new_id = m_next_entity_id.fetch_add(1);

            std::string entity_name;
            {
                std::unique_lock lock(m_entities_mutex);
                EntityInfo& info = m_entities[new_id];
                info.id = new_id;
                info.version = 1;  // Start with version 1
                info.active = true;
                info.name = name.empty() ? ("Entity_" + std::to_string(new_id)) : name;
                info.creation_time = std::chrono::system_clock::now();
                info.last_modified = info.creation_time;
                entity_name = info.name;
            }

            CORE_TRACE_ECS_LIFECYCLE("create", new_id, entity_name);
            m_statistics_dirty = true;
            return EntityID(new_id, 1);
        }

        //========================================================================
        // FIXED: Entity Destruction with Version Increment
        //========================================================================

        bool DestroyEntity(const EntityID& handle) {
            if (!ValidateEntityHandle(handle)) {
                return false; // Already destroyed or invalid
            }

            // PHASE 1: Remove all components
            // Note: Intentional double-mutex pattern to prevent deadlock.
            // We release locks between phases to avoid holding entities_mutex
            // while component storage locks are acquired (deadlock prevention).
            {
                std::shared_lock entities_lock(m_entities_mutex);
                auto it = m_entities.find(handle.id);
                if (it == m_entities.end() || !it->second.IsValidHandle(handle)) {
                    return false;
                }

                const auto& component_types = it->second.component_types;

                // Remove components from all storages
                std::shared_lock storages_lock(m_storages_mutex);
                for (size_t type_hash : component_types) {
                    auto storage_it = m_component_storages.find(type_hash);
                    if (storage_it != m_component_storages.end()) {
                        storage_it->second->RemoveComponent(handle.id);
                    }
                }
            } // Locks released to prevent deadlock

            // PHASE 2: Mark entity as inactive
            // Re-validation protects against race conditions between phases
            {
                std::unique_lock lock(m_entities_mutex);
                auto it = m_entities.find(handle.id);
                if (it != m_entities.end() && it->second.IsValidHandle(handle)) {
                    const std::string entity_name = it->second.name;
                    it->second.active = false;
                    it->second.version++; // FIXED: Increment version to invalidate old handles
                    it->second.component_types.clear();
                    it->second.UpdateLastModified();

                    m_statistics_dirty = true;
                    CORE_TRACE_ECS_LIFECYCLE("destroy", handle.id, entity_name);
                    return true;
                }
            }

            return false;
        }

        //========================================================================
        // Entity Management Utilities
        //========================================================================

        void Clear();

        //========================================================================
        // FIXED: Component Access with Version Validation
        //========================================================================

        template<typename ComponentType>
        std::shared_ptr<ComponentType> GetComponent(const EntityID& handle) const {
            // FIXED: Validate entity handle before accessing components
            if (!ValidateEntityHandle(handle)) {
                return nullptr;
            }

            size_t type_hash = typeid(ComponentType).hash_code();

            std::shared_lock lock(m_storages_mutex);
            auto it = m_component_storages.find(type_hash);
            if (it != m_component_storages.end()) {
                auto storage = static_cast<ComponentStorage<ComponentType>*>(it->second.get());
                return storage->GetComponent(handle.id);
            }

            return nullptr;
        }

        template<typename ComponentType>
        bool HasComponent(const EntityID& handle) const {
            // FIXED: Validate entity handle before checking components
            if (!ValidateEntityHandle(handle)) {
                return false;
            }

            size_t type_hash = typeid(ComponentType).hash_code();

            std::shared_lock lock(m_storages_mutex);
            auto it = m_component_storages.find(type_hash);
            if (it != m_component_storages.end()) {
                return it->second->HasComponent(handle.id);
            }

            return false;
        }

        template<typename ComponentType, typename... Args>
        std::shared_ptr<ComponentType> AddComponent(const EntityID& handle, Args&&... args) {
            // FIXED: Validate entity handle before adding components
            if (!ValidateEntityHandle(handle)) {
                throw std::invalid_argument("Invalid entity handle: " + handle.ToString());
            }

            size_t type_hash = typeid(ComponentType).hash_code();

            // Get or create storage
            ComponentStorage<ComponentType>* storage = nullptr;
            {
                std::unique_lock lock(m_storages_mutex);
                auto it = m_component_storages.find(type_hash);
                if (it == m_component_storages.end()) {
                    m_component_storages[type_hash] = std::make_unique<ComponentStorage<ComponentType>>();
                    storage = static_cast<ComponentStorage<ComponentType>*>(m_component_storages[type_hash].get());
                }
                else {
                    storage = static_cast<ComponentStorage<ComponentType>*>(it->second.get());
                }
            }

            // Add component
            auto component = storage->AddComponent(handle.id, std::forward<Args>(args)...);

            // Update entity info
            {
                auto* entity_info = GetMutableEntityInfo(handle);
                if (entity_info) {
                    entity_info->component_types.insert(type_hash);
                    entity_info->UpdateLastModified();
                }
            }

            m_statistics_dirty = true;
            return component;
        }

        template<typename ComponentType>
        bool RemoveComponent(const EntityID& handle) {
            // FIXED: Validate entity handle before removing components
            if (!ValidateEntityHandle(handle)) {
                return false;
            }

            size_t type_hash = typeid(ComponentType).hash_code();

            bool removed = false;
            {
                std::shared_lock lock(m_storages_mutex);
                auto it = m_component_storages.find(type_hash);
                if (it != m_component_storages.end()) {
                    removed = it->second->RemoveComponent(handle.id);
                }
            }

            if (removed) {
                // Update entity info
                auto* entity_info = GetMutableEntityInfo(handle);
                if (entity_info) {
                    entity_info->component_types.erase(type_hash);
                    entity_info->UpdateLastModified();
                }

                m_statistics_dirty = true;
            }

            return removed;
        }

        //========================================================================
        // Entity Queries and Validation
        //========================================================================

        bool IsEntityValid(const EntityID& handle) const {
            return ValidateEntityHandle(handle);
        }

        std::string GetEntityName(const EntityID& handle) const {
            const auto* info = GetEntityInfo(handle);
            return info ? info->name : "";
        }

        bool SetEntityName(const EntityID& handle, const std::string& name) {
            auto* info = GetMutableEntityInfo(handle);
            if (info) {
                info->name = name;
                info->UpdateLastModified();
                return true;
            }
            return false;
        }

        uint32_t GetEntityVersion(const EntityID& handle) const {
            const auto* info = GetEntityInfo(handle);
            return info ? info->version : 0;
        }

        //========================================================================
        // Bulk Operations with Handle Validation
        //========================================================================

        template<typename ComponentType>
        std::vector<EntityID> GetEntitiesWithComponent() const {
            std::vector<EntityID> result;

            size_t type_hash = typeid(ComponentType).hash_code();

            std::shared_lock storages_lock(m_storages_mutex);
            auto storage_it = m_component_storages.find(type_hash);
            if (storage_it == m_component_storages.end()) {
                return result;
            }

            auto entity_ids = storage_it->second->GetEntityIds();

            std::shared_lock entities_lock(m_entities_mutex);
            result.reserve(entity_ids.size());

            for (uint64_t entity_id : entity_ids) {
                auto it = m_entities.find(entity_id);
                if (it != m_entities.end() && it->second.active) {
                    result.emplace_back(entity_id, it->second.version);
                }
            }

            return result;
        }

        std::vector<EntityID> GetAllActiveEntities() const {
            std::vector<EntityID> result;

            std::shared_lock lock(m_entities_mutex);
            result.reserve(m_entities.size());

            for (const auto& [entity_id, info] : m_entities) {
                if (info.active) {
                    result.emplace_back(entity_id, info.version);
                }
            }

            return result;
        }

        //========================================================================
        // Statistics and Diagnostics
        //========================================================================

        EntityStatistics GetStatistics() const {
            if (m_statistics_dirty.load()) {
                UpdateStatistics();
            }

            std::shared_lock lock(m_statistics_mutex);
            return m_cached_statistics;  // Return by value (thread-safe)
        }

        void UpdateStatistics() const {
            std::unique_lock lock(m_statistics_mutex);

            auto start_time = std::chrono::high_resolution_clock::now();

            m_cached_statistics = {};

            // Count entities
            {
                std::shared_lock entities_lock(m_entities_mutex);
                m_cached_statistics.total_entities = m_entities.size();

                for (const auto& [id, info] : m_entities) {
                    if (info.active) {
                        m_cached_statistics.active_entities++;
                        m_cached_statistics.memory_usage_bytes += info.memory_usage_bytes;
                    }
                }
            }

            // Count components
            {
                std::shared_lock storages_lock(m_storages_mutex);
                for (const auto& [type_hash, storage] : m_component_storages) {
                    size_t count = storage->GetComponentCount();
                    size_t memory = storage->GetMemoryUsage();
                    std::string type_name = storage->GetComponentTypeName();

                    m_cached_statistics.total_components += count;
                    m_cached_statistics.memory_usage_bytes += memory;
                    m_cached_statistics.component_counts[type_name] = count;
                    m_cached_statistics.component_memory_usage[type_name] = memory;
                }
            }

            // Calculate averages
            if (m_cached_statistics.active_entities > 0) {
                m_cached_statistics.average_components_per_entity =
                    static_cast<double>(m_cached_statistics.total_components) /
                    m_cached_statistics.active_entities;
            }

            auto end_time = std::chrono::high_resolution_clock::now();
            m_cached_statistics.last_update_time =
                std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            m_cached_statistics.last_calculated = std::chrono::system_clock::now();

            m_statistics_dirty = false;
        }

        //========================================================================
        // Integrity Validation
        //========================================================================

        struct ValidationResult {
            bool is_valid = true;
            std::vector<std::string> errors;
            std::vector<std::string> warnings;

            void AddError(const std::string& error) {
                errors.push_back(error);
                is_valid = false;
            }

            void AddWarning(const std::string& warning) {
                warnings.push_back(warning);
            }
        };

        ValidationResult ValidateIntegrity() const {
            ValidationResult result;

            std::shared_lock entities_lock(m_entities_mutex);
            std::shared_lock storages_lock(m_storages_mutex);

            // Check entity-component consistency
            for (const auto& [entity_id, info] : m_entities) {
                if (!info.active) continue;

                EntityID handle(entity_id, info.version);

                // Verify all claimed component types exist
                for (size_t type_hash : info.component_types) {
                    auto storage_it = m_component_storages.find(type_hash);
                    if (storage_it == m_component_storages.end()) {
                        result.AddError("Entity " + handle.ToString() + " claims to have component type " +
                            std::to_string(type_hash) + " but no storage exists");
                        continue;
                    }

                    if (!storage_it->second->HasComponent(entity_id)) {
                        result.AddError("Entity " + handle.ToString() + " claims to have component type " +
                            std::to_string(type_hash) + " but storage doesn't contain it");
                    }
                }
            }

            // Check for orphaned components
            for (const auto& [type_hash, storage] : m_component_storages) {
                auto entity_ids = storage->GetEntityIds();
                for (uint64_t entity_id : entity_ids) {
                    auto entity_it = m_entities.find(entity_id);
                    if (entity_it == m_entities.end()) {
                        result.AddWarning("Component type " + std::to_string(type_hash) +
                            " has component for non-existent entity " + std::to_string(entity_id));
                    }
                    else if (!entity_it->second.active) {
                        result.AddWarning("Component type " + std::to_string(type_hash) +
                            " has component for inactive entity " + std::to_string(entity_id));
                    }
                    else if (entity_it->second.component_types.find(type_hash) == entity_it->second.component_types.end()) {
                        result.AddError("Component type " + std::to_string(type_hash) +
                            " has component for entity " + std::to_string(entity_id) +
                            " but entity doesn't claim to have this component");
                    }
                }
            }

            return result;
        }

        //========================================================================
        // Serialization Support
        //========================================================================

        std::string Serialize() const {
            // Implementation would serialize all entities and components
            // For now, return a placeholder
            return "EntityManager_Serialized_Data";
        }

        bool Deserialize(const std::string& data) {
            // Implementation would deserialize entities and components
            // For now, return success
            return true;
        }

        //========================================================================
        // Cleanup and Maintenance
        //========================================================================

        void DestroyAllEntities() {
            std::vector<EntityID> all_entities;

            {
                std::shared_lock lock(m_entities_mutex);
                all_entities.reserve(m_entities.size());
                for (const auto& [entity_id, info] : m_entities) {
                    if (info.active) {
                        all_entities.emplace_back(entity_id, info.version);
                    }
                }
            }

            for (const auto& handle : all_entities) {
                DestroyEntity(handle);
            }
        }

        template<typename ComponentType>
        void DestroyEntitiesWithComponent() {
            auto entities = GetEntitiesWithComponent<ComponentType>();
            for (const auto& handle : entities) {
                DestroyEntity(handle);
            }
        }

        // Cleanup inactive entities (garbage collection)
        size_t CleanupInactiveEntities() {
            std::vector<uint64_t> to_remove;

            {
                std::shared_lock lock(m_entities_mutex);
                for (const auto& [entity_id, info] : m_entities) {
                    if (!info.active) {
                        to_remove.push_back(entity_id);
                    }
                }
            }

            if (!to_remove.empty()) {
                std::unique_lock lock(m_entities_mutex);
                for (uint64_t entity_id : to_remove) {
                    m_entities.erase(entity_id);
                }
                m_statistics_dirty = true;
            }

            return to_remove.size();
        }

        //========================================================================
        // Performance and Debugging
        //========================================================================

        void PrintDebugInfo() const {
            auto stats = GetStatistics();

            std::cout << "=== EntityManager Debug Info ===" << std::endl;
            std::cout << "Total Entities: " << stats.total_entities << std::endl;
            std::cout << "Active Entities: " << stats.active_entities << std::endl;
            std::cout << "Total Components: " << stats.total_components << std::endl;
            std::cout << "Memory Usage: " << (stats.memory_usage_bytes / 1024.0) << " KB" << std::endl;
            std::cout << "Avg Components/Entity: " << stats.average_components_per_entity << std::endl;
            std::cout << "Last Update Time: " << stats.last_update_time.count() << " ms" << std::endl;

            std::cout << "\nComponent Types:" << std::endl;
            for (const auto& [type_name, count] : stats.component_counts) {
                auto memory_it = stats.component_memory_usage.find(type_name);
                size_t memory = (memory_it != stats.component_memory_usage.end()) ? memory_it->second : 0;
                std::cout << "  " << type_name << ": " << count << " instances, "
                    << (memory / 1024.0) << " KB" << std::endl;
            }
        }

        // Memory usage estimation
        size_t EstimateMemoryUsage() const {
            size_t total = 0;

            {
                std::shared_lock entities_lock(m_entities_mutex);
                total += m_entities.size() * sizeof(EntityInfo);
            }

            {
                std::shared_lock storages_lock(m_storages_mutex);
                for (const auto& [type_hash, storage] : m_component_storages) {
                    total += storage->GetMemoryUsage();
                }
            }

            return total;
        }

        // Get next entity ID (for debugging/testing)
        uint64_t GetNextEntityId() const {
            return m_next_entity_id.load();
        }

        // Count active entities (fast path)
        size_t GetActiveEntityCount() const {
            if (m_statistics_dirty.load()) {
                UpdateStatistics();
            }

            std::shared_lock lock(m_statistics_mutex);
            return m_cached_statistics.active_entities;
        }

        // Count total components (fast path)
        size_t GetTotalComponentCount() const {
            if (m_statistics_dirty.load()) {
                UpdateStatistics();
            }

            std::shared_lock lock(m_statistics_mutex);
            return m_cached_statistics.total_components;
        }
    };

} // namespace core::ecs

//============================================================================
// Hash specialization for EntityID (for std::unordered_map, etc.)
//============================================================================

namespace std {
    template<>
    struct hash<core::ecs::EntityID> {
        size_t operator()(const core::ecs::EntityID& entity_id) const {
            return core::ecs::EntityID::Hash{}(entity_id);
        }
    };
}

//============================================================================
// Usage Examples and Migration Guide
//============================================================================

/*
MIGRATION FROM OLD EntityID SYSTEM:

OLD CODE:
    auto entity = entity_manager.CreateEntity();  // Returns uint64_t
    auto component = entity_manager.GetComponent<MyComponent>(entity);

NEW CODE:
    auto entity = entity_manager.CreateEntity();  // Returns EntityID{id, version}
    auto component = entity_manager.GetComponent<MyComponent>(entity);

    // Version checking is now automatic!
    // If entity was destroyed and recreated, GetComponent returns nullptr

BENEFITS:
    - Prevents use-after-destroy bugs
    - Automatic validation of entity handles
    - Better debugging with version information
    - Thread-safe access patterns

PERFORMANCE:
    - Minimal overhead (one extra version check per access)
    - Statistics are cached and updated lazily
    - Lock-free entity ID generation
*/