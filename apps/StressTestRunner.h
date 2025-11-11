#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace Json {
class Value;
}

namespace apps::stress {

struct StressTestConfig {
    std::string maps_directory{"data/maps"};
    std::string nations_directory{"data/nations"};
    std::size_t warmup_ticks{30};
    std::size_t measured_ticks{600};
    std::size_t worker_threads{0};
    std::size_t units_per_task_hint{0};
    bool verbose{false};
    bool summary_only{false};
    std::optional<std::string> json_output_path{};
};

struct StressTestResult {
    std::size_t total_province_count{0};
    std::size_t total_nation_count{0};
    std::size_t max_provinces_per_map{0};
    std::string max_province_file;
    std::size_t max_nations_per_file{0};
    std::string max_nations_file;

    std::size_t worker_threads{0};
    std::size_t warmup_ticks{0};
    std::size_t measured_ticks{0};

    std::vector<double> tick_times_ms;

    double average_tick_ms{0.0};
    double median_tick_ms{0.0};
    double p95_tick_ms{0.0};
    double max_tick_ms{0.0};
    double min_tick_ms{0.0};

    double simulated_units_per_tick{0.0};

    std::size_t peak_active_tasks{0};
    std::size_t peak_queue_depth{0};
    double average_task_time_ms{0.0};

    std::size_t resident_memory_kb{0};

    std::string timestamp_utc;
};

bool RunStressTest(const StressTestConfig& config, StressTestResult& out_result);

Json::Value SerializeResult(const StressTestConfig& config, const StressTestResult& result);

void PrintHumanReadableReport(const StressTestConfig& config, const StressTestResult& result);

} // namespace apps::stress

