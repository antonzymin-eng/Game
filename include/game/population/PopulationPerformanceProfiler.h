// ============================================================================
// Population System Performance Profiler
// Created: 2025-11-19
// Location: include/game/population/PopulationPerformanceProfiler.h
// ============================================================================

#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <numeric>
#include <fstream>
#include <iomanip>

namespace game::population {

/**
 * @brief Performance profiling utility for population system
 *
 * Tracks execution time of various operations and provides
 * statistical analysis for optimization.
 */
class PopulationPerformanceProfiler {
public:
    struct ProfileEntry {
        std::string operation_name;
        std::chrono::microseconds duration;
        std::chrono::system_clock::time_point timestamp;
        size_t entity_count;
        size_t data_size;
    };

    struct ProfileStatistics {
        std::string operation_name;
        size_t call_count;
        std::chrono::microseconds total_time;
        std::chrono::microseconds avg_time;
        std::chrono::microseconds min_time;
        std::chrono::microseconds max_time;
        std::chrono::microseconds median_time;
        double std_deviation;

        // Performance metrics
        double calls_per_second;
        double avg_microseconds_per_entity;
        double throughput_mb_per_sec;
    };

    // Singleton pattern
    static PopulationPerformanceProfiler& GetInstance() {
        static PopulationPerformanceProfiler instance;
        return instance;
    }

    // Scoped timer for RAII-based profiling
    class ScopedTimer {
    public:
        ScopedTimer(const std::string& operation_name,
                   size_t entity_count = 0,
                   size_t data_size = 0)
            : m_operation_name(operation_name)
            , m_entity_count(entity_count)
            , m_data_size(data_size)
            , m_start(std::chrono::high_resolution_clock::now()) {}

        ~ScopedTimer() {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - m_start);

            PopulationPerformanceProfiler::GetInstance().RecordOperation(
                m_operation_name, duration, m_entity_count, m_data_size);
        }

    private:
        std::string m_operation_name;
        size_t m_entity_count;
        size_t m_data_size;
        std::chrono::high_resolution_clock::time_point m_start;
    };

    // Recording operations
    void RecordOperation(const std::string& operation_name,
                        std::chrono::microseconds duration,
                        size_t entity_count = 0,
                        size_t data_size = 0) {
        ProfileEntry entry;
        entry.operation_name = operation_name;
        entry.duration = duration;
        entry.timestamp = std::chrono::system_clock::now();
        entry.entity_count = entity_count;
        entry.data_size = data_size;

        m_entries[operation_name].push_back(entry);

        // Keep history limited
        if (m_entries[operation_name].size() > m_max_history_per_operation) {
            m_entries[operation_name].erase(m_entries[operation_name].begin());
        }
    }

    // Statistics generation
    ProfileStatistics GetStatistics(const std::string& operation_name) const {
        ProfileStatistics stats;
        stats.operation_name = operation_name;

        auto it = m_entries.find(operation_name);
        if (it == m_entries.end() || it->second.empty()) {
            return stats;
        }

        const auto& entries = it->second;
        stats.call_count = entries.size();

        // Collect durations
        std::vector<std::chrono::microseconds> durations;
        size_t total_entities = 0;
        size_t total_data = 0;

        for (const auto& entry : entries) {
            durations.push_back(entry.duration);
            stats.total_time += entry.duration;
            total_entities += entry.entity_count;
            total_data += entry.data_size;
        }

        // Calculate statistics
        stats.avg_time = stats.total_time / stats.call_count;
        stats.min_time = *std::min_element(durations.begin(), durations.end());
        stats.max_time = *std::max_element(durations.begin(), durations.end());

        // Median
        std::vector<std::chrono::microseconds> sorted_durations = durations;
        std::sort(sorted_durations.begin(), sorted_durations.end());
        stats.median_time = sorted_durations[sorted_durations.size() / 2];

        // Standard deviation
        double mean = stats.avg_time.count();
        double variance = 0.0;
        for (const auto& duration : durations) {
            double diff = duration.count() - mean;
            variance += diff * diff;
        }
        stats.std_deviation = std::sqrt(variance / stats.call_count);

        // Performance metrics
        if (stats.call_count > 1) {
            auto time_span = entries.back().timestamp - entries.front().timestamp;
            auto time_span_seconds = std::chrono::duration_cast<std::chrono::seconds>(time_span).count();
            if (time_span_seconds > 0) {
                stats.calls_per_second = static_cast<double>(stats.call_count) / time_span_seconds;
            }
        }

        if (total_entities > 0) {
            stats.avg_microseconds_per_entity = static_cast<double>(stats.total_time.count()) / total_entities;
        }

        if (total_data > 0 && stats.total_time.count() > 0) {
            double mb = static_cast<double>(total_data) / (1024 * 1024);
            double seconds = static_cast<double>(stats.total_time.count()) / 1'000'000;
            stats.throughput_mb_per_sec = mb / seconds;
        }

        return stats;
    }

    // Get all tracked operations
    std::vector<std::string> GetTrackedOperations() const {
        std::vector<std::string> operations;
        for (const auto& [name, _] : m_entries) {
            operations.push_back(name);
        }
        return operations;
    }

    // Reporting
    std::string GenerateReport() const {
        std::ostringstream report;

        report << "\n";
        report << "╔══════════════════════════════════════════════════════════════════════╗\n";
        report << "║         POPULATION SYSTEM PERFORMANCE PROFILE REPORT                ║\n";
        report << "╚══════════════════════════════════════════════════════════════════════╝\n";
        report << "\n";

        auto operations = GetTrackedOperations();

        // Sort by total time
        std::vector<std::pair<std::string, ProfileStatistics>> stats_list;
        for (const auto& op : operations) {
            stats_list.push_back({op, GetStatistics(op)});
        }
        std::sort(stats_list.begin(), stats_list.end(),
            [](const auto& a, const auto& b) {
                return a.second.total_time > b.second.total_time;
            });

        // Summary table
        report << "┌────────────────────────────────────────┬──────────┬──────────┬──────────┬──────────┐\n";
        report << "│ Operation                              │  Calls   │   Total  │   Avg    │   Max    │\n";
        report << "├────────────────────────────────────────┼──────────┼──────────┼──────────┼──────────┤\n";

        for (const auto& [name, stats] : stats_list) {
            report << "│ " << std::left << std::setw(38) << TruncateString(name, 38) << " │ "
                   << std::right << std::setw(8) << stats.call_count << " │ "
                   << std::right << std::setw(7) << FormatTime(stats.total_time) << " │ "
                   << std::right << std::setw(7) << FormatTime(stats.avg_time) << " │ "
                   << std::right << std::setw(7) << FormatTime(stats.max_time) << " │\n";
        }

        report << "└────────────────────────────────────────┴──────────┴──────────┴──────────┴──────────┘\n";
        report << "\n";

        // Detailed statistics for top 5 operations
        report << "Detailed Statistics (Top 5 by Total Time):\n";
        report << "─────────────────────────────────────────────────────────────────────────\n";

        int count = 0;
        for (const auto& [name, stats] : stats_list) {
            if (count++ >= 5) break;

            report << "\n📊 " << name << "\n";
            report << "   Calls: " << stats.call_count << "\n";
            report << "   Total Time: " << FormatTime(stats.total_time) << "\n";
            report << "   Average Time: " << FormatTime(stats.avg_time) << "\n";
            report << "   Median Time: " << FormatTime(stats.median_time) << "\n";
            report << "   Min Time: " << FormatTime(stats.min_time) << "\n";
            report << "   Max Time: " << FormatTime(stats.max_time) << "\n";
            report << "   Std Deviation: " << std::fixed << std::setprecision(2)
                   << stats.std_deviation << " μs\n";

            if (stats.calls_per_second > 0) {
                report << "   Throughput: " << std::fixed << std::setprecision(2)
                       << stats.calls_per_second << " calls/sec\n";
            }

            if (stats.avg_microseconds_per_entity > 0) {
                report << "   Per-Entity Cost: " << std::fixed << std::setprecision(3)
                       << stats.avg_microseconds_per_entity << " μs/entity\n";
            }
        }

        report << "\n";
        return report.str();
    }

    // Export to file
    void ExportToFile(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return;
        }

        file << GenerateReport();

        // Export raw data as CSV
        file << "\n\nRaw Performance Data (CSV):\n";
        file << "Operation,Timestamp,Duration_us,Entity_Count,Data_Size\n";

        for (const auto& [operation, entries] : m_entries) {
            for (const auto& entry : entries) {
                auto timestamp = std::chrono::system_clock::to_time_t(entry.timestamp);
                file << operation << ","
                     << timestamp << ","
                     << entry.duration.count() << ","
                     << entry.entity_count << ","
                     << entry.data_size << "\n";
            }
        }

        file.close();
    }

    // Configuration
    void SetMaxHistoryPerOperation(size_t max_history) {
        m_max_history_per_operation = max_history;
    }

    void Clear() {
        m_entries.clear();
    }

    void ClearOperation(const std::string& operation_name) {
        m_entries.erase(operation_name);
    }

private:
    PopulationPerformanceProfiler() = default;
    ~PopulationPerformanceProfiler() = default;
    PopulationPerformanceProfiler(const PopulationPerformanceProfiler&) = delete;
    PopulationPerformanceProfiler& operator=(const PopulationPerformanceProfiler&) = delete;

    std::unordered_map<std::string, std::vector<ProfileEntry>> m_entries;
    size_t m_max_history_per_operation = 1000;

    // Utility functions
    static std::string TruncateString(const std::string& str, size_t max_length) {
        if (str.length() <= max_length) {
            return str;
        }
        return str.substr(0, max_length - 3) + "...";
    }

    static std::string FormatTime(std::chrono::microseconds duration) {
        auto us = duration.count();

        if (us < 1000) {
            return std::to_string(us) + "μs";
        } else if (us < 1'000'000) {
            return std::to_string(us / 1000) + "ms";
        } else {
            return std::to_string(us / 1'000'000) + "s";
        }
    }
};

// Convenience macro for profiling
#define PROFILE_POPULATION_OPERATION(name) \
    PopulationPerformanceProfiler::ScopedTimer __profile_timer_##__LINE__(name)

#define PROFILE_POPULATION_OPERATION_WITH_COUNT(name, count) \
    PopulationPerformanceProfiler::ScopedTimer __profile_timer_##__LINE__(name, count)

#define PROFILE_POPULATION_OPERATION_WITH_DATA(name, count, size) \
    PopulationPerformanceProfiler::ScopedTimer __profile_timer_##__LINE__(name, count, size)

} // namespace game::population
