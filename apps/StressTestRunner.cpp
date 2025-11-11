#include "StressTestRunner.h"

#include "core/threading/ThreadedSystemManager.h"
#include "utils/PlatformCompat.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <json/json.h>
#include <numeric>
#include <sstream>
#include <thread>
#include <vector>

#if defined(PLATFORM_WINDOWS)
#    include <windows.h>
#    include <psapi.h>
#endif

#if defined(__linux__)
#    include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace {

using Clock = std::chrono::steady_clock;

struct CountAggregate {
    std::size_t total{0};
    std::size_t max_per_file{0};
    std::string max_file;
};

bool LoadJsonFile(const fs::path& path, Json::Value& out_value, std::string& error_message) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::ostringstream oss;
        oss << "Failed to open " << path;
        error_message = oss.str();
        return false;
    }

    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    std::string errs;
    if (!Json::parseFromStream(builder, file, &out_value, &errs)) {
        std::ostringstream oss;
        oss << "JSON parse error in " << path << ": " << errs;
        error_message = oss.str();
        return false;
    }

    return true;
}

std::size_t ExtractProvinceCount(const Json::Value& root) {
    if (root.isMember("provinces") && root["provinces"].isArray()) {
        return static_cast<std::size_t>(root["provinces"].size());
    }

    if (root.isMember("map_region") && root["map_region"].isObject()) {
        const auto& region = root["map_region"];
        if (region.isMember("provinces") && region["provinces"].isArray()) {
            return static_cast<std::size_t>(region["provinces"].size());
        }
    }

    return 0;
}

std::size_t ExtractNationCount(const Json::Value& root) {
    if (root.isMember("nations") && root["nations"].isArray()) {
        return static_cast<std::size_t>(root["nations"].size());
    }
    return 0;
}

CountAggregate CountProvinces(const fs::path& maps_directory, bool verbose) {
    CountAggregate aggregate;

    if (!fs::exists(maps_directory)) {
        std::ostringstream oss;
        oss << "Maps directory does not exist: " << maps_directory;
        throw std::runtime_error(oss.str());
    }

    for (const auto& entry : fs::recursive_directory_iterator(maps_directory)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        if (entry.path().extension() != ".json") {
            continue;
        }

        Json::Value root;
        std::string error_message;
        if (!LoadJsonFile(entry.path(), root, error_message)) {
            throw std::runtime_error(error_message);
        }

        auto count = ExtractProvinceCount(root);
        if (count == 0 && verbose) {
            std::cout << "[stress] Skipping map without provinces: " << entry.path() << std::endl;
        }

        aggregate.total += count;
        if (count > aggregate.max_per_file) {
            aggregate.max_per_file = count;
            aggregate.max_file = entry.path().string();
        }
    }

    return aggregate;
}

CountAggregate CountNations(const fs::path& nations_directory, bool verbose) {
    CountAggregate aggregate;

    if (!fs::exists(nations_directory)) {
        std::ostringstream oss;
        oss << "Nations directory does not exist: " << nations_directory;
        throw std::runtime_error(oss.str());
    }

    for (const auto& entry : fs::recursive_directory_iterator(nations_directory)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        if (entry.path().extension() != ".json") {
            continue;
        }

        Json::Value root;
        std::string error_message;
        if (!LoadJsonFile(entry.path(), root, error_message)) {
            throw std::runtime_error(error_message);
        }

        auto count = ExtractNationCount(root);
        if (count == 0 && verbose) {
            std::cout << "[stress] Skipping nations file without entries: " << entry.path() << std::endl;
        }

        aggregate.total += count;
        if (count > aggregate.max_per_file) {
            aggregate.max_per_file = count;
            aggregate.max_file = entry.path().string();
        }
    }

    return aggregate;
}

std::size_t GetResidentMemoryKB() {
#if defined(PLATFORM_WINDOWS)
    PROCESS_MEMORY_COUNTERS counters{};
    if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters))) {
        return static_cast<std::size_t>(counters.WorkingSetSize / 1024);
    }
    return 0;
#elif defined(__linux__)
    std::ifstream statm("/proc/self/statm");
    long pages = 0;
    long resident = 0;
    if (statm >> pages >> resident) {
        long page_size = sysconf(_SC_PAGESIZE);
        return static_cast<std::size_t>((resident * page_size) / 1024);
    }
    return 0;
#else
    return 0;
#endif
}

double CalculatePercentile(const std::vector<double>& samples, double percentile) {
    if (samples.empty()) {
        return 0.0;
    }

    auto sorted = samples;
    std::sort(sorted.begin(), sorted.end());
    if (sorted.size() == 1) {
        return sorted.front();
    }

    double rank = percentile * static_cast<double>(sorted.size() - 1);
    auto lower_index = static_cast<std::size_t>(std::floor(rank));
    auto upper_index = static_cast<std::size_t>(std::ceil(rank));
    double fraction = rank - static_cast<double>(lower_index);

    double lower_value = sorted[lower_index];
    double upper_value = sorted[upper_index];
    return lower_value + (upper_value - lower_value) * fraction;
}

std::string CurrentTimestampUTC() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm utc_tm{};
#if defined(_WIN32)
    gmtime_s(&utc_tm, &now_time);
#else
    gmtime_r(&now_time, &utc_tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&utc_tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

} // namespace

namespace apps::stress {

bool RunStressTest(const StressTestConfig& config, StressTestResult& out_result) {
    out_result = StressTestResult{};
    out_result.warmup_ticks = config.warmup_ticks;
    out_result.measured_ticks = config.measured_ticks;
    out_result.timestamp_utc = CurrentTimestampUTC();

    try {
        auto province_counts = CountProvinces(config.maps_directory, config.verbose);
        auto nation_counts = CountNations(config.nations_directory, config.verbose);

        out_result.total_province_count = province_counts.total;
        out_result.total_nation_count = nation_counts.total;
        out_result.max_provinces_per_map = province_counts.max_per_file;
        out_result.max_province_file = province_counts.max_file;
        out_result.max_nations_per_file = nation_counts.max_per_file;
        out_result.max_nations_file = nation_counts.max_file;
    }
    catch (const std::exception& e) {
        std::cerr << "[stress] Data inspection failed: " << e.what() << std::endl;
        return false;
    }

    const std::size_t total_units = std::max<std::size_t>(1, out_result.total_province_count + out_result.total_nation_count);
    const std::size_t hardware_threads = std::max<std::size_t>(1, std::thread::hardware_concurrency());
    out_result.worker_threads = config.worker_threads > 0 ? config.worker_threads : hardware_threads;

    std::size_t units_per_task = config.units_per_task_hint;
    if (units_per_task == 0) {
        std::size_t baseline = total_units / (out_result.worker_threads * 4);
        units_per_task = std::clamp<std::size_t>(baseline, 64, 4096);
    }
    out_result.simulated_units_per_tick = static_cast<double>(total_units);

    core::threading::ThreadPool thread_pool(out_result.worker_threads);

    std::vector<double> tick_times;
    tick_times.reserve(config.measured_ticks);

    const std::size_t warmup_total = config.warmup_ticks + config.measured_ticks;
    std::atomic<std::size_t> peak_active{0};
    std::atomic<std::size_t> peak_queue{0};

    auto simulate_chunk = [](std::size_t start, std::size_t end) {
        double accumulator = 0.0;
        for (std::size_t index = start; index < end; ++index) {
            double x = static_cast<double>((index % 1024) + 1);
            accumulator += std::sin(x * 0.0003) * std::cos(x * 0.0001);
            accumulator = std::fmod(accumulator + 1000.0, 1000.0);
        }
        return accumulator;
    };

    double accumulator_guard = 0.0;

    for (std::size_t tick_index = 0; tick_index < warmup_total; ++tick_index) {
        auto tick_start = Clock::now();
        std::vector<std::future<double>> futures;

        const std::size_t task_count = (total_units + units_per_task - 1) / units_per_task;
        futures.reserve(task_count);

        std::size_t processed = 0;
        for (std::size_t task = 0; task < task_count; ++task) {
            std::size_t start = processed;
            std::size_t end = std::min(total_units, start + units_per_task);
            processed = end;

            futures.emplace_back(thread_pool.Submit(simulate_chunk, start, end));
        }

        bool tasks_completed = false;
        while (!tasks_completed) {
            tasks_completed = true;
            for (auto& future : futures) {
                if (future.wait_for(std::chrono::microseconds(0)) != std::future_status::ready) {
                    tasks_completed = false;
                    break;
                }
            }

            auto active = thread_pool.GetActiveTaskCount();
            auto queued = thread_pool.GetQueuedTaskCount();
            peak_active.store(std::max<std::size_t>(peak_active.load(), active));
            peak_queue.store(std::max<std::size_t>(peak_queue.load(), queued));

            if (!tasks_completed) {
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }
        }

        for (auto& future : futures) {
            accumulator_guard += future.get();
        }

        auto tick_end = Clock::now();
        double tick_ms = std::chrono::duration<double, std::milli>(tick_end - tick_start).count();
        bool measure_tick = tick_index >= config.warmup_ticks;
        if (measure_tick) {
            tick_times.push_back(tick_ms);
            if (config.verbose && !config.summary_only) {
                std::cout << "[stress] tick=" << (tick_index - config.warmup_ticks)
                          << " duration_ms=" << std::fixed << std::setprecision(3) << tick_ms << std::endl;
            }
        }
    }

    thread_pool.Shutdown();

    out_result.tick_times_ms = tick_times;
    out_result.peak_active_tasks = peak_active.load();
    out_result.peak_queue_depth = peak_queue.load();
    out_result.average_task_time_ms = thread_pool.GetAverageTaskTime();
    out_result.resident_memory_kb = GetResidentMemoryKB();

    if (!tick_times.empty()) {
        double sum = std::accumulate(tick_times.begin(), tick_times.end(), 0.0);
        out_result.average_tick_ms = sum / static_cast<double>(tick_times.size());
        out_result.max_tick_ms = *std::max_element(tick_times.begin(), tick_times.end());
        out_result.min_tick_ms = *std::min_element(tick_times.begin(), tick_times.end());
        out_result.median_tick_ms = CalculatePercentile(tick_times, 0.5);
        out_result.p95_tick_ms = CalculatePercentile(tick_times, 0.95);
    }

    if (config.summary_only) {
        std::cout << "[stress] Average tick: " << std::fixed << std::setprecision(3)
                  << out_result.average_tick_ms << " ms (p95 " << out_result.p95_tick_ms << " ms)" << std::endl;
    }

    // Prevent compiler from optimizing away accumulator
    if (accumulator_guard == 42.0) {
        std::cout << "[stress] accumulator guard triggered" << std::endl;
    }

    if (config.json_output_path.has_value()) {
        try {
            auto json = SerializeResult(config, out_result);
            std::ofstream output(*config.json_output_path);
            if (!output.is_open()) {
                std::cerr << "[stress] Failed to write JSON output to " << *config.json_output_path << std::endl;
                return false;  // Fail when JSON output was explicitly requested but failed
            }
            output << json.toStyledString();
            if (!output.good()) {
                std::cerr << "[stress] Error writing JSON data to " << *config.json_output_path << std::endl;
                return false;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[stress] JSON serialization failed: " << e.what() << std::endl;
            return false;
        }
    }

    if (!config.summary_only) {
        PrintHumanReadableReport(config, out_result);
    }

    return true;
}

Json::Value SerializeResult(const StressTestConfig& config, const StressTestResult& result) {
    Json::Value root(Json::objectValue);

    Json::Value config_node(Json::objectValue);
    config_node["maps_directory"] = config.maps_directory;
    config_node["nations_directory"] = config.nations_directory;
    config_node["warmup_ticks"] = static_cast<Json::UInt64>(config.warmup_ticks);
    config_node["measured_ticks"] = static_cast<Json::UInt64>(config.measured_ticks);
    config_node["worker_threads"] = static_cast<Json::UInt64>(result.worker_threads);
    config_node["units_per_task_hint"] = static_cast<Json::UInt64>(config.units_per_task_hint);
    config_node["verbose"] = config.verbose;
    root["config"] = config_node;

    Json::Value counts(Json::objectValue);
    counts["total_provinces"] = static_cast<Json::UInt64>(result.total_province_count);
    counts["total_nations"] = static_cast<Json::UInt64>(result.total_nation_count);
    counts["max_provinces_per_map"] = static_cast<Json::UInt64>(result.max_provinces_per_map);
    counts["max_province_file"] = result.max_province_file;
    counts["max_nations_per_file"] = static_cast<Json::UInt64>(result.max_nations_per_file);
    counts["max_nations_file"] = result.max_nations_file;
    root["counts"] = counts;

    Json::Value metrics(Json::objectValue);
    metrics["timestamp_utc"] = result.timestamp_utc;
    metrics["average_tick_ms"] = result.average_tick_ms;
    metrics["median_tick_ms"] = result.median_tick_ms;
    metrics["p95_tick_ms"] = result.p95_tick_ms;
    metrics["max_tick_ms"] = result.max_tick_ms;
    metrics["min_tick_ms"] = result.min_tick_ms;
    metrics["simulated_units_per_tick"] = result.simulated_units_per_tick;
    metrics["peak_active_tasks"] = static_cast<Json::UInt64>(result.peak_active_tasks);
    metrics["peak_queue_depth"] = static_cast<Json::UInt64>(result.peak_queue_depth);
    metrics["average_task_time_ms"] = result.average_task_time_ms;
    metrics["resident_memory_kb"] = static_cast<Json::UInt64>(result.resident_memory_kb);

    Json::Value samples(Json::arrayValue);
    for (double tick : result.tick_times_ms) {
        samples.append(tick);
    }
    metrics["tick_samples_ms"] = samples;

    root["metrics"] = metrics;
    return root;
}

void PrintHumanReadableReport(const StressTestConfig& config, const StressTestResult& result) {
    std::cout << "\n=== Stress Test Summary ===" << std::endl;
    std::cout << "Timestamp (UTC): " << result.timestamp_utc << std::endl;
    std::cout << "Maps directory:  " << config.maps_directory << std::endl;
    std::cout << "Nations dir:     " << config.nations_directory << std::endl;
    std::cout << "Worker threads:  " << result.worker_threads << std::endl;
    std::cout << "Total provinces: " << result.total_province_count
              << " (peak file: " << result.max_province_file << " => " << result.max_provinces_per_map << ")" << std::endl;
    std::cout << "Total nations:   " << result.total_nation_count
              << " (peak file: " << result.max_nations_file << " => " << result.max_nations_per_file << ")" << std::endl;
    std::cout << "Measured ticks:  " << result.measured_ticks << " (warmup " << result.warmup_ticks << ")" << std::endl;

    if (!result.tick_times_ms.empty()) {
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "Average tick:   " << result.average_tick_ms << " ms" << std::endl;
        std::cout << "Median tick:    " << result.median_tick_ms << " ms" << std::endl;
        std::cout << "95th percentile: " << result.p95_tick_ms << " ms" << std::endl;
        std::cout << "Max tick:       " << result.max_tick_ms << " ms" << std::endl;
        std::cout << "Min tick:       " << result.min_tick_ms << " ms" << std::endl;
    }

    std::cout << "Thread pool peak active tasks: " << result.peak_active_tasks << std::endl;
    std::cout << "Thread pool peak queue depth:  " << result.peak_queue_depth << std::endl;
    std::cout << std::fixed << std::setprecision(4)
              << "Average task time: " << result.average_task_time_ms << " ms" << std::endl;

    if (result.resident_memory_kb > 0) {
        double mb = static_cast<double>(result.resident_memory_kb) / 1024.0;
        std::cout << "Resident memory: " << std::setprecision(2) << mb << " MiB" << std::endl;
    }
    else {
        std::cout << "Resident memory: (platform query unavailable)" << std::endl;
    }
    std::cout << "==========================\n" << std::endl;
}

} // namespace apps::stress

