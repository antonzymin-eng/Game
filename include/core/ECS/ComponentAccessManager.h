// ============================================================================
// ComponentAccessManager.h - Thread-safe component access with FIXED vector API
// CRITICAL BUG FIX: Removed data race in vector operations
// Location: include/core/ecs/ComponentAccessManager.h
// ============================================================================

#pragma once

#include "IComponent.h"
#include "EntityManager.h"
#include "MessageBus.h"
#include "core/Types/game_types.h"
#include <shared_mutex>
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>
#include <chrono>
#include <mutex>
#include <atomic>

namespace core::ecs {

    // ============================================================================
    // Forward Declarations
    // ============================================================================

    template<typename ComponentType>
    class ComponentAccessResult;

    template<typename ComponentType>
    class ComponentWriteGuard;

    class AccessStatistics;

    // ============================================================================
    // Component Access Result - Safe Read Access
    // ============================================================================

    template<typename ComponentType>
    class ComponentAccessResult {
    public:
        ComponentAccessResult(ComponentType* component, std::shared_lock<std::shared_mutex>&& lock);
        ComponentAccessResult(ComponentAccessResult&& other) noexcept;
        ComponentAccessResult& operator=(ComponentAccessResult&& other) noexcept;
        ~ComponentAccessResult() = default;

        // Disable copy
        ComponentAccessResult(const ComponentAccessResult&) = delete;
        ComponentAccessResult& operator=(const ComponentAccessResult&) = delete;

        bool IsValid() const;
        const ComponentType* Get() const;
        const ComponentType* operator->() const;
        const ComponentType& operator*() const;
        explicit operator bool() const;

    private:
        ComponentType* m_component;
        std::shared_lock<std::shared_mutex> m_lock;
    };

    // ============================================================================
    // Component Write Guard - Exclusive Write Access
    // ============================================================================

    template<typename ComponentType>
    class ComponentWriteGuard {
    public:
        ComponentWriteGuard(ComponentType* component, std::unique_lock<std::shared_mutex>&& lock);
        ComponentWriteGuard(ComponentWriteGuard&& other) noexcept;
        ComponentWriteGuard& operator=(ComponentWriteGuard&& other) noexcept;
        ~ComponentWriteGuard() = default;

        // Disable copy
        ComponentWriteGuard(const ComponentWriteGuard&) = delete;
        ComponentWriteGuard& operator=(const ComponentWriteGuard&) = delete;

        bool IsValid() const;
        ComponentType* Get();
        ComponentType* operator->();
        ComponentType& operator*();
        explicit operator bool() const;

    private:
        ComponentType* m_component;
        std::unique_lock<std::shared_mutex> m_lock;
    };

    // ============================================================================
    // SAFE Vector Access Result - CRITICAL FIX
    // ============================================================================

    template<typename ComponentType>
    class VectorAccessResult {
    public:
        VectorAccessResult(std::shared_lock<std::shared_mutex>&& lock);
        ~VectorAccessResult() = default;

        // Move-only semantics
        VectorAccessResult(VectorAccessResult&& other) noexcept;
        VectorAccessResult& operator=(VectorAccessResult&& other) noexcept;
        VectorAccessResult(const VectorAccessResult&) = delete;
        VectorAccessResult& operator=(const VectorAccessResult&) = delete;

        // Safe component access
        const ComponentType* GetComponent(types::EntityID entity_id, EntityManager& entity_manager) const;
        ComponentType* GetComponentMutable(types::EntityID entity_id, EntityManager& entity_manager);

        // Iterator-like access
        class ConstIterator {
        public:
            ConstIterator(EntityManager& em, const std::vector<types::EntityID>& entities, size_t index);

            const ComponentType* operator*() const;
            const ComponentType* operator->() const;
            ConstIterator& operator++();
            bool operator!=(const ConstIterator& other) const;
            bool operator==(const ConstIterator& other) const;

        private:
            EntityManager& m_entity_manager;
            const std::vector<types::EntityID>& m_entities;
            size_t m_index;
        };

        ConstIterator begin(EntityManager& entity_manager, const std::vector<types::EntityID>& entities) const;
        ConstIterator end(EntityManager& entity_manager, const std::vector<types::EntityID>& entities) const;

    private:
        std::shared_lock<std::shared_mutex> m_lock;
    };

    template<typename ComponentType>
    class VectorWriteResult {
    public:
        VectorWriteResult(std::unique_lock<std::shared_mutex>&& lock);
        ~VectorWriteResult() = default;

        // Move-only semantics
        VectorWriteResult(VectorWriteResult&& other) noexcept;
        VectorWriteResult& operator=(VectorWriteResult&& other) noexcept;
        VectorWriteResult(const VectorWriteResult&) = delete;
        VectorWriteResult& operator=(const VectorWriteResult&) = delete;

        // Safe component access
        const ComponentType* GetComponent(types::EntityID entity_id, EntityManager& entity_manager) const;
        ComponentType* GetComponentMutable(types::EntityID entity_id, EntityManager& entity_manager);

        // Mutable iterator-like access
        class Iterator {
        public:
            Iterator(EntityManager& em, const std::vector<types::EntityID>& entities, size_t index);

            ComponentType* operator*();
            ComponentType* operator->();
            Iterator& operator++();
            bool operator!=(const Iterator& other) const;
            bool operator==(const Iterator& other) const;

        private:
            EntityManager& m_entity_manager;
            const std::vector<types::EntityID>& m_entities;
            size_t m_index;
        };

        Iterator begin(EntityManager& entity_manager, const std::vector<types::EntityID>& entities);
        Iterator end(EntityManager& entity_manager, const std::vector<types::EntityID>& entities);

    private:
        std::unique_lock<std::shared_mutex> m_lock;
    };

    // ============================================================================
    // Access Statistics for Performance Monitoring
    // ============================================================================

    class AccessStatistics {
    public:
        AccessStatistics();

        void RecordRead(const std::string& component_type);
        void RecordWrite(const std::string& component_type);
        void RecordContention(const std::string& component_type, double wait_time_ms);

        uint64_t GetReadCount(const std::string& component_type) const;
        uint64_t GetWriteCount(const std::string& component_type) const;
        double GetAverageContentionTime(const std::string& component_type) const;
        std::vector<std::string> GetMostContentedComponents() const;
        void Reset();

    private:
        struct ComponentStats {
            std::atomic<uint64_t> read_count{ 0 };
            std::atomic<uint64_t> write_count{ 0 };
            std::atomic<double> total_contention_time{ 0.0 };
            std::atomic<uint64_t> contention_events{ 0 };
        };

        std::unordered_map<std::string, std::unique_ptr<ComponentStats>> m_stats;
        mutable std::shared_mutex m_stats_mutex;
    };

    // ============================================================================
    // Main Component Access Manager - THREAD-SAFE
    // ============================================================================

    class ComponentAccessManager {
    public:
        explicit ComponentAccessManager(EntityManager* entity_manager, MessageBus* message_bus);
        ~ComponentAccessManager();

        // SAFE Individual component access
        template<typename ComponentType>
        ComponentAccessResult<ComponentType> GetComponent(types::EntityID entity_id);

        template<typename ComponentType>
        ComponentWriteGuard<ComponentType> GetComponentForWrite(types::EntityID entity_id);

        // FIXED: Safe vector access - CRITICAL SECURITY FIX
        template<typename ComponentType>
        VectorAccessResult<ComponentType> GetAllComponentsForRead();

        template<typename ComponentType>
        VectorWriteResult<ComponentType> GetAllComponentsForWrite();

        // FIXED: Safe batch operations
        template<typename ComponentType>
        VectorAccessResult<ComponentType> GetComponentsBatchForRead(const std::vector<types::EntityID>& entity_ids);

        template<typename ComponentType>
        VectorWriteResult<ComponentType> GetComponentsBatchForWrite(const std::vector<types::EntityID>& entity_ids);

        // Lock management
        void LockAllComponentsForRead();
        void LockAllComponentsForWrite();
        void UnlockAllComponents();
        bool TryLockComponentForRead(const std::string& component_type, std::chrono::milliseconds timeout);
        bool TryLockComponentForWrite(const std::string& component_type, std::chrono::milliseconds timeout);

        // Performance monitoring
        AccessStatistics GetAccessStatistics() const;
        void EnablePerformanceMonitoring(bool enable);
        std::vector<std::string> GetPerformanceReport() const;
        void ResetPerformanceCounters();

        // Debug and diagnostic methods
        bool HasDeadlocks() const;
        std::vector<std::string> GetLockedComponents() const;
        size_t GetActiveReadLocks(const std::string& component_type) const;
        bool HasWriteLock(const std::string& component_type) const;

    private:
        EntityManager* m_entity_manager;
        MessageBus* m_message_bus;

        // Thread-safe mutex storage
        std::unordered_map<std::string, std::unique_ptr<std::shared_mutex>> m_component_mutexes;
        mutable std::shared_mutex m_mutex_map_mutex;

        // Performance monitoring
        std::unique_ptr<AccessStatistics> m_statistics;
        std::atomic<bool> m_performance_monitoring_enabled{ true };

        // Internal helpers
        template<typename ComponentType>
        std::shared_mutex& GetComponentMutex();

        void RegisterComponentType(const std::string& type_name);
        void RecordAccess(const std::string& component_type, bool is_write);
        void DetectPotentialDeadlock(const std::string& component_type);
    };

} // namespace core::ecs

// Template implementations
#include "ComponentAccessManager.inl"