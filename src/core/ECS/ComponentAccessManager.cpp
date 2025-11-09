// ============================================================================
// ComponentAccessManager.cpp - Thread-safe component access implementation
// Created: October 10, 2025, 5:45 PM
// Location: src/core/ECS/ComponentAccessManager.cpp
// FIXED: Matches ComponentAccessManager.h interface exactly
// ============================================================================

#include "core/ECS/ComponentAccessManager.h"
#include "core/ECS/EntityManager.h"
#include "core/ECS/MessageBus.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

namespace core::ecs {

    // ============================================================================
    // ComponentAccessManager - NON-TEMPLATE IMPLEMENTATIONS ONLY
    // Templates are implemented in ComponentAccessManager.inl
    // ============================================================================

    // ============================================================================
    // AccessStatistics Implementation
    // ============================================================================

    AccessStatistics::AccessStatistics() = default;

    void AccessStatistics::RecordRead(const std::string& component_type) {
        std::unique_lock<std::shared_mutex> lock(m_stats_mutex);
        
        auto it = m_stats.find(component_type);
        if (it == m_stats.end()) {
            m_stats[component_type] = std::make_unique<ComponentStats>();
        }
        
        m_stats[component_type]->read_count.fetch_add(1, std::memory_order_relaxed);
    }

    void AccessStatistics::RecordWrite(const std::string& component_type) {
        std::unique_lock<std::shared_mutex> lock(m_stats_mutex);
        
        auto it = m_stats.find(component_type);
        if (it == m_stats.end()) {
            m_stats[component_type] = std::make_unique<ComponentStats>();
        }
        
        m_stats[component_type]->write_count.fetch_add(1, std::memory_order_relaxed);
    }

    void AccessStatistics::RecordContention(const std::string& component_type, double wait_time_ms) {
        std::unique_lock<std::shared_mutex> lock(m_stats_mutex);
        
        auto it = m_stats.find(component_type);
        if (it == m_stats.end()) {
            m_stats[component_type] = std::make_unique<ComponentStats>();
        }
        
        auto& stats = *m_stats[component_type];
        
        // Update atomic counters
        stats.contention_events.fetch_add(1, std::memory_order_relaxed);
        
        // Update precise time with mutex protection
        {
            std::lock_guard<std::mutex> time_lock(stats.contention_mutex);
            stats.total_contention_time_precise += wait_time_ms;
            
            // Update atomic version (truncated to uint64_t milliseconds)
            stats.total_contention_time_ms.store(
                static_cast<uint64_t>(stats.total_contention_time_precise),
                std::memory_order_relaxed);
        }
    }

    unsigned long long AccessStatistics::GetReadCount(const std::string& component_type) const {
        std::shared_lock<std::shared_mutex> lock(m_stats_mutex);
        
        auto it = m_stats.find(component_type);
        if (it == m_stats.end()) return 0;
        
        return it->second->read_count.load(std::memory_order_relaxed);
    }

    unsigned long long AccessStatistics::GetWriteCount(const std::string& component_type) const {
        std::shared_lock<std::shared_mutex> lock(m_stats_mutex);
        
        auto it = m_stats.find(component_type);
        if (it == m_stats.end()) return 0;
        
        return it->second->write_count.load(std::memory_order_relaxed);
    }

    double AccessStatistics::GetAverageContentionTime(const std::string& component_type) const {
        std::shared_lock<std::shared_mutex> lock(m_stats_mutex);
        
        auto it = m_stats.find(component_type);
        if (it == m_stats.end()) return 0.0;
        
        auto& stats = *it->second;
        uint64_t events = stats.contention_events.load(std::memory_order_relaxed);
        
        if (events == 0) return 0.0;
        
        std::lock_guard<std::mutex> time_lock(stats.contention_mutex);
        return stats.total_contention_time_precise / events;
    }

    std::vector<std::string> AccessStatistics::GetMostContentedComponents() const {
        std::shared_lock<std::shared_mutex> lock(m_stats_mutex);
        
        std::vector<std::pair<std::string, double>> contention_data;
        
        for (const auto& [type, stats] : m_stats) {
            uint64_t events = stats->contention_events.load(std::memory_order_relaxed);
            if (events > 0) {
                std::lock_guard<std::mutex> time_lock(stats->contention_mutex);
                double avg_time = stats->total_contention_time_precise / events;
                contention_data.emplace_back(type, avg_time);
            }
        }
        
        std::sort(contention_data.begin(), contention_data.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });
        
        std::vector<std::string> result;
        result.reserve(contention_data.size());
        for (const auto& [type, _] : contention_data) {
            result.push_back(type);
        }
        
        return result;
    }

    void AccessStatistics::Reset() {
        std::unique_lock<std::shared_mutex> lock(m_stats_mutex);
        m_stats.clear();
    }

    // ============================================================================
    // ComponentAccessManager Implementation
    // ============================================================================

    ComponentAccessManager::ComponentAccessManager(EntityManager* entity_manager, MessageBus* message_bus)
        : m_entity_manager(entity_manager)
        , m_message_bus(message_bus)
        , m_statistics(std::make_unique<AccessStatistics>()) {
    }

    ComponentAccessManager::~ComponentAccessManager() = default;

    void ComponentAccessManager::LockAllComponentsForRead() {
        std::shared_lock<std::shared_mutex> lock(m_mutex_map_mutex);
        
        for (auto& [type_name, mutex_ptr] : m_component_mutexes) {
            mutex_ptr->lock_shared();
        }
    }

    void ComponentAccessManager::LockAllComponentsForWrite() {
        std::shared_lock<std::shared_mutex> lock(m_mutex_map_mutex);
        
        for (auto& [type_name, mutex_ptr] : m_component_mutexes) {
            mutex_ptr->lock();
        }
    }

    void ComponentAccessManager::UnlockAllComponents() {
        std::shared_lock<std::shared_mutex> lock(m_mutex_map_mutex);
        
        for (auto& [type_name, mutex_ptr] : m_component_mutexes) {
            mutex_ptr->unlock();
        }
    }

    bool ComponentAccessManager::TryLockComponentForRead(
        const std::string& component_type,
        std::chrono::milliseconds timeout) {
        
        // FIXED: std::shared_mutex doesn't support timed locks
        // Use simple try_lock with manual timeout loop
        std::shared_lock<std::shared_mutex> lock(m_mutex_map_mutex);
        
        auto it = m_component_mutexes.find(component_type);
        if (it == m_component_mutexes.end()) {
            RegisterComponentType(component_type);
            return true; // Newly created, not locked
        }
        
        lock.unlock();
        
        // Simple spin-wait with timeout
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < timeout) {
            if (it->second->try_lock_shared()) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        return false; // Timeout expired
    }

    bool ComponentAccessManager::TryLockComponentForWrite(
        const std::string& component_type,
        std::chrono::milliseconds timeout) {
        
        // FIXED: std::shared_mutex doesn't support timed locks
        std::shared_lock<std::shared_mutex> lock(m_mutex_map_mutex);
        
        auto it = m_component_mutexes.find(component_type);
        if (it == m_component_mutexes.end()) {
            RegisterComponentType(component_type);
            return true; // Newly created, not locked
        }
        
        lock.unlock();
        
        // Simple spin-wait with timeout
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < timeout) {
            if (it->second->try_lock()) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        return false; // Timeout expired
    }

    const AccessStatistics& ComponentAccessManager::GetAccessStatistics() const {
        return *m_statistics;
    }

    void ComponentAccessManager::EnablePerformanceMonitoring(bool enable) {
        m_performance_monitoring_enabled.store(enable, std::memory_order_release);
    }

    std::vector<std::string> ComponentAccessManager::GetPerformanceReport() const {
        std::vector<std::string> report;
        
        auto contended = m_statistics->GetMostContentedComponents();
        
        for (const auto& component_type : contended) {
            auto read_count = m_statistics->GetReadCount(component_type);
            auto write_count = m_statistics->GetWriteCount(component_type);
            auto avg_contention = m_statistics->GetAverageContentionTime(component_type);
            
            std::ostringstream line;
            line << component_type << ": "
                 << "Reads=" << read_count << ", "
                 << "Writes=" << write_count << ", "
                 << "AvgWait=" << avg_contention << "ms";
            
            report.push_back(line.str());
        }
        
        return report;
    }

    void ComponentAccessManager::ResetPerformanceCounters() {
        m_statistics->Reset();
    }

    std::vector<std::string> ComponentAccessManager::GetLockedComponents() const {
        std::vector<std::string> locked;
        
        std::shared_lock<std::shared_mutex> lock(m_mutex_map_mutex);
        
        for (const auto& [type_name, mutex_ptr] : m_component_mutexes) {
            // Try to acquire read lock with zero timeout
            if (!mutex_ptr->try_lock_shared()) {
                locked.push_back(type_name);
            } else {
                mutex_ptr->unlock_shared();
            }
        }
        
        return locked;
    }

    bool ComponentAccessManager::HasWriteLock(const std::string& component_type) const {
        std::shared_lock<std::shared_mutex> lock(m_mutex_map_mutex);
        
        auto it = m_component_mutexes.find(component_type);
        if (it == m_component_mutexes.end()) return false;
        
        // Try to acquire write lock with zero timeout
        if (!it->second->try_lock()) {
            return true; // Someone else has it
        }
        
        it->second->unlock();
        return false;
    }

    void ComponentAccessManager::RegisterComponentType(const std::string& type_name) {
        std::unique_lock<std::shared_mutex> lock(m_mutex_map_mutex);
        
        if (m_component_mutexes.find(type_name) == m_component_mutexes.end()) {
            m_component_mutexes[type_name] = std::make_unique<std::shared_mutex>();
        }
    }

    void ComponentAccessManager::RecordAccess(const std::string& component_type, bool is_write) {
        if (!m_performance_monitoring_enabled.load(std::memory_order_acquire)) {
            return;
        }
        
        if (is_write) {
            m_statistics->RecordWrite(component_type);
        } else {
            m_statistics->RecordRead(component_type);
        }
    }

    // Note: Previously contained incomplete deadlock detection methods (HasDeadlocks,
    // GetActiveReadLocks, DetectPotentialDeadlock). These have been removed as they
    // provided no actual functionality and could create false sense of safety.
    // TODO: Implement proper deadlock detection with lock ordering graph if needed.

} // namespace core::ecs
