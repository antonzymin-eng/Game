// ============================================================================
// File: ComponentAccessManager.cpp
// Created: 2025-09-13 15:30:00
// Intended Location: /src/core/ECS/
// ============================================================================

#include "core/ECS/ComponentAccessManager.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <typeinfo>

namespace game::core {

    // ============================================================================
    // AccessStatistics Implementation
    // ============================================================================

    AccessStatistics::AccessStatistics() = default;

    void AccessStatistics::RecordRead(const std::string& component_type) {
        std::lock_guard<std::mutex> lock(m_stats_mutex);
        m_stats[component_type].read_count++;
    }

    void AccessStatistics::RecordWrite(const std::string& component_type) {
        std::lock_guard<std::mutex> lock(m_stats_mutex);
        m_stats[component_type].write_count++;
    }

    void AccessStatistics::RecordContention(const std::string& component_type, double wait_time_ms) {
        std::lock_guard<std::mutex> lock(m_stats_mutex);
        auto& stats = m_stats[component_type];
        stats.total_contention_time += wait_time_ms;
        stats.contention_events++;
    }

    uint64_t AccessStatistics::GetReadCount(const std::string& component_type) const {
        std::lock_guard<std::mutex> lock(m_stats_mutex);
        auto it = m_stats.find(component_type);
        return (it != m_stats.end()) ? it->second.read_count : 0;
    }

    uint64_t AccessStatistics::GetWriteCount(const std::string& component_type) const {
        std::lock_guard<std::mutex> lock(m_stats_mutex);
        auto it = m_stats.find(component_type);
        return (it != m_stats.end()) ? it->second.write_count : 0;
    }

    double AccessStatistics::GetAverageContentionTime(const std::string& component_type) const {
        std::lock_guard<std::mutex> lock(m_stats_mutex);
        auto it = m_stats.find(component_type);
        if (it == m_stats.end() || it->second.contention_events == 0) {
            return 0.0;
        }
        return it->second.total_contention_time / it->second.contention_events;
    }

    std::vector<std::string> AccessStatistics::GetMostContentedComponents() const {
        std::lock_guard<std::mutex> lock(m_stats_mutex);

        std::vector<std::pair<std::string, double>> contention_data;
        for (const auto& [type, stats] : m_stats) {
            if (stats.contention_events > 0) {
                double avg_time = stats.total_contention_time / stats.contention_events;
                contention_data.emplace_back(type, avg_time);
            }
        }

        std::sort(contention_data.begin(), contention_data.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

        std::vector<std::string> result;
        for (const auto& [type, time] : contention_data) {
            result.push_back(type);
        }

        return result;
    }

    void AccessStatistics::Reset() {
        std::lock_guard<std::mutex> lock(m_stats_mutex);
        m_stats.clear();
    }

    // ============================================================================
    // ComponentAccessManager Implementation
    // ============================================================================

    ComponentAccessManager::ComponentAccessManager(std::shared_ptr<EntityManager> entity_manager)
        : m_entity_manager(std::move(entity_manager))
        , m_performance_monitoring_enabled(false) {
    }

    ComponentAccessManager::~ComponentAccessManager() = default;

    void ComponentAccessManager::Initialize() {
        // Initialize any required subsystems
        std::cout << "ComponentAccessManager initialized" << std::endl;
    }

    void ComponentAccessManager::Shutdown() {
        std::lock_guard<std::mutex> lock(m_type_registry_mutex);
        m_component_mutexes.clear();
        m_registered_types.clear();
        std::cout << "ComponentAccessManager shutdown" << std::endl;
    }

    void ComponentAccessManager::RegisterComponentType(const std::string& type_name) {
        std::lock_guard<std::mutex> lock(m_type_registry_mutex);

        if (m_registered_types.find(type_name) == m_registered_types.end()) {
            m_component_mutexes[type_name] = std::make_unique<std::shared_mutex>();
            m_registered_types.insert(type_name);
        }
    }

    void ComponentAccessManager::SetPerformanceMonitoring(bool enabled) {
        m_performance_monitoring_enabled = enabled;
    }

    bool ComponentAccessManager::IsPerformanceMonitoringEnabled() const {
        return m_performance_monitoring_enabled;
    }

    void ComponentAccessManager::RecordAccess(const std::string& component_type, bool is_write) {
        if (!m_performance_monitoring_enabled) {
            return;
        }

        if (is_write) {
            m_statistics.RecordWrite(component_type);
        }
        else {
            m_statistics.RecordRead(component_type);
        }
    }

    void ComponentAccessManager::RecordContention(const std::string& component_type, double wait_time_ms) {
        if (m_performance_monitoring_enabled) {
            m_statistics.RecordContention(component_type, wait_time_ms);
        }
    }

    AccessStatistics ComponentAccessManager::GetAccessStatistics() const {
        return m_statistics;
    }

    void ComponentAccessManager::ResetStatistics() {
        m_statistics.Reset();
    }

    std::vector<std::string> ComponentAccessManager::GetRegisteredComponentTypes() const {
        std::lock_guard<std::mutex> lock(m_type_registry_mutex);
        return std::vector<std::string>(m_registered_types.begin(), m_registered_types.end());
    }

    std::shared_ptr<EntityManager> ComponentAccessManager::GetEntityManager() const {
        return m_entity_manager;
    }

    void ComponentAccessManager::SetEntityManager(std::shared_ptr<EntityManager> entity_manager) {
        m_entity_manager = std::move(entity_manager);
    }

    std::string ComponentAccessManager::GenerateAccessReport() const {
        std::ostringstream report;
        report << "=== Component Access Report ===\n";

        auto types = GetRegisteredComponentTypes();
        if (types.empty()) {
            report << "No component types registered.\n";
            return report.str();
        }

        report << "Registered Component Types: " << types.size() << "\n\n";

        for (const auto& type : types) {
            auto read_count = m_statistics.GetReadCount(type);
            auto write_count = m_statistics.GetWriteCount(type);
            auto avg_contention = m_statistics.GetAverageContentionTime(type);

            report << "Component: " << type << "\n";
            report << "  Read Access: " << read_count << "\n";
            report << "  Write Access: " << write_count << "\n";
            report << "  Avg Contention Time: " << avg_contention << "ms\n\n";
        }

        auto contended_components = m_statistics.GetMostContentedComponents();
        if (!contended_components.empty()) {
            report << "Most Contended Components:\n";
            for (size_t i = 0; i < std::min(size_t(5), contended_components.size()); ++i) {
                report << "  " << (i + 1) << ". " << contended_components[i] << "\n";
            }
        }

        return report.str();
    }

    std::vector<std::string> ComponentAccessManager::GetPerformanceRecommendations() const {
        std::vector<std::string> recommendations;

        auto contended_components = m_statistics.GetMostContentedComponents();

        for (const auto& component_type : contended_components) {
            auto avg_contention = m_statistics.GetAverageContentionTime(component_type);
            auto read_count = m_statistics.GetReadCount(component_type);
            auto write_count = m_statistics.GetWriteCount(component_type);

            if (avg_contention > 5.0) { // More than 5ms average contention
                recommendations.push_back(
                    "High contention on " + component_type +
                    " (avg: " + std::to_string(avg_contention) + "ms). " +
                    "Consider reducing write frequency or batching operations."
                );
            }

            if (write_count > read_count * 2) {
                recommendations.push_back(
                    "High write-to-read ratio for " + component_type +
                    ". Consider caching or reducing update frequency."
                );
            }

            if (read_count > 10000 && write_count < 100) {
                recommendations.push_back(
                    "Very high read frequency for " + component_type +
                    " with low writes. Consider read-only optimization strategies."
                );
            }
        }

        if (recommendations.empty()) {
            recommendations.push_back("No performance issues detected. System is operating efficiently.");
        }

        return recommendations;
    }

    void ComponentAccessManager::OptimizeForReadHeavyWorkload(const std::string& component_type) {
        // This is a placeholder for potential optimizations
        // Could implement read-optimized data structures or caching
        std::cout << "Optimizing " << component_type << " for read-heavy workload" << std::endl;
    }

    void ComponentAccessManager::OptimizeForWriteHeavyWorkload(const std::string& component_type) {
        // This is a placeholder for potential optimizations
        // Could implement write batching or reduced contention strategies
        std::cout << "Optimizing " << component_type << " for write-heavy workload" << std::endl;
    }

    // ============================================================================
    // Debugging and Validation
    // ============================================================================

    bool ComponentAccessManager::ValidateComponentAccess(const std::string& component_type) const {
        std::lock_guard<std::mutex> lock(m_type_registry_mutex);
        return m_registered_types.find(component_type) != m_registered_types.end();
    }

    void ComponentAccessManager::DumpComponentState() const {
        std::cout << "=== Component Access Manager State ===\n";
        std::cout << "Performance Monitoring: " << (m_performance_monitoring_enabled ? "Enabled" : "Disabled") << "\n";
        std::cout << "Registered Types: " << m_registered_types.size() << "\n";

        for (const auto& type : m_registered_types) {
            std::cout << "  - " << type << "\n";
        }

        std::cout << "==========================================\n";
    }

} // namespace game::core