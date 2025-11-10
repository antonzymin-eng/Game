// ============================================================================
// test_ai_director_performance.cpp - AI Director Performance Benchmarks
// Created: November 10, 2025
// Purpose: Week 2 verification - Performance benchmarking for AI Director
// ============================================================================

#include <gtest/gtest.h>
#include <chrono>
#include <vector>
#include <numeric>
#include <algorithm>
#include <iomanip>

#include "game/ai/AIDirector.h"
#include "core/ECS/EntityManager.h"
#include "core/ECS/MessageBus.h"
#include "core/ECS/ComponentAccessManager.h"
#include "core/threading/ThreadedSystemManager.h"

// ============================================================================
// Performance Metrics
// ============================================================================

struct PerformanceMetrics {
    double min_time_ms = std::numeric_limits<double>::max();
    double max_time_ms = 0.0;
    double avg_time_ms = 0.0;
    double median_time_ms = 0.0;
    double p95_time_ms = 0.0;
    double p99_time_ms = 0.0;
    size_t sample_count = 0;
};

// ============================================================================
// Test Fixture
// ============================================================================

class AIDirectorPerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        entity_manager = std::make_unique<core::ecs::EntityManager>();
        message_bus = std::make_unique<core::ecs::MessageBus>();
        access_manager = std::make_unique<core::ecs::ComponentAccessManager>();
        threaded_system_manager = std::make_unique<core::threading::ThreadedSystemManager>();

        ai_director = std::make_unique<AI::AIDirector>(
            *entity_manager,
            *message_bus,
            *access_manager,
            *threaded_system_manager
        );
        ai_director->Initialize();
        ai_director->Start();
    }

    void TearDown() override {
        if (ai_director) {
            ai_director->Shutdown();
        }
    }

    PerformanceMetrics MeasureUpdatePerformance(int num_iterations) {
        std::vector<double> sample_times;
        sample_times.reserve(num_iterations);

        constexpr float DELTA_TIME = 0.016f; // 60 FPS

        for (int i = 0; i < num_iterations; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            ai_director->Update(DELTA_TIME);
            auto end = std::chrono::high_resolution_clock::now();

            auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(
                end - start
            ).count();
            sample_times.push_back(duration_us / 1000.0); // Convert to ms
        }

        return CalculateMetrics(sample_times);
    }

    PerformanceMetrics CalculateMetrics(std::vector<double>& times) {
        PerformanceMetrics metrics;
        metrics.sample_count = times.size();

        if (times.empty()) return metrics;

        // Sort for percentiles
        std::sort(times.begin(), times.end());

        metrics.min_time_ms = times.front();
        metrics.max_time_ms = times.back();
        metrics.avg_time_ms = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
        metrics.median_time_ms = times[times.size() / 2];
        metrics.p95_time_ms = times[static_cast<size_t>(times.size() * 0.95)];
        metrics.p99_time_ms = times[static_cast<size_t>(times.size() * 0.99)];

        return metrics;
    }

    void PrintMetrics(const std::string& test_name, const PerformanceMetrics& metrics) {
        std::cout << "\n=== " << test_name << " ===" << std::endl;
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "Samples:    " << metrics.sample_count << std::endl;
        std::cout << "Min:        " << metrics.min_time_ms << " ms" << std::endl;
        std::cout << "Max:        " << metrics.max_time_ms << " ms" << std::endl;
        std::cout << "Average:    " << metrics.avg_time_ms << " ms" << std::endl;
        std::cout << "Median:     " << metrics.median_time_ms << " ms" << std::endl;
        std::cout << "95th %ile:  " << metrics.p95_time_ms << " ms" << std::endl;
        std::cout << "99th %ile:  " << metrics.p99_time_ms << " ms" << std::endl;
    }

    std::unique_ptr<core::ecs::EntityManager> entity_manager;
    std::unique_ptr<core::ecs::MessageBus> message_bus;
    std::unique_ptr<core::ecs::ComponentAccessManager> access_manager;
    std::unique_ptr<core::threading::ThreadedSystemManager> threaded_system_manager;
    std::unique_ptr<AI::AIDirector> ai_director;
};

// ============================================================================
// Performance Benchmarks
// ============================================================================

TEST_F(AIDirectorPerformanceTest, BaselinePerformance) {
    // Measure baseline Update() performance with no AI actors

    constexpr int NUM_ITERATIONS = 1000;
    auto metrics = MeasureUpdatePerformance(NUM_ITERATIONS);

    PrintMetrics("Baseline Performance (No Actors)", metrics);

    // Should be well under frame budget
    EXPECT_LT(metrics.avg_time_ms, 1.0) << "Baseline performance is too slow";
    EXPECT_LT(metrics.p99_time_ms, 5.0) << "99th percentile exceeds acceptable threshold";
}

TEST_F(AIDirectorPerformanceTest, PerformanceWithNationAI) {
    // Measure performance with Nation AI actors

    constexpr int NUM_NATIONS = 10;
    constexpr int NUM_ITERATIONS = 500;

    // Create Nation AI actors
    for (int i = 0; i < NUM_NATIONS; ++i) {
        auto nation_id = static_cast<EntityID>(1000 + i);
        ai_director->CreateNationAI(nation_id);
    }

    auto metrics = MeasureUpdatePerformance(NUM_ITERATIONS);
    PrintMetrics("Performance with 10 Nation AI", metrics);

    // Should still be under frame budget
    EXPECT_LT(metrics.avg_time_ms, 10.0) << "Average time with Nation AI too high";
    EXPECT_LT(metrics.p99_time_ms, 16.0) << "99th percentile exceeds frame budget (16ms)";
}

TEST_F(AIDirectorPerformanceTest, PerformanceWithCharacterAI) {
    // Measure performance with Character AI actors

    constexpr int NUM_CHARACTERS = 50;
    constexpr int NUM_ITERATIONS = 500;

    // Create Character AI actors
    for (int i = 0; i < NUM_CHARACTERS; ++i) {
        auto character_id = static_cast<EntityID>(5000 + i);
        ai_director->CreateCharacterAI(character_id);
    }

    auto metrics = MeasureUpdatePerformance(NUM_ITERATIONS);
    PrintMetrics("Performance with 50 Character AI", metrics);

    EXPECT_LT(metrics.avg_time_ms, 10.0) << "Average time with Character AI too high";
    EXPECT_LT(metrics.p99_time_ms, 16.0) << "99th percentile exceeds frame budget";
}

TEST_F(AIDirectorPerformanceTest, PerformanceWithMixedActors) {
    // Measure performance with realistic mix of AI actors

    constexpr int NUM_NATIONS = 20;
    constexpr int NUM_CHARACTERS = 100;
    constexpr int NUM_ITERATIONS = 500;

    // Create mixed AI actors
    for (int i = 0; i < NUM_NATIONS; ++i) {
        ai_director->CreateNationAI(static_cast<EntityID>(1000 + i));
    }
    for (int i = 0; i < NUM_CHARACTERS; ++i) {
        ai_director->CreateCharacterAI(static_cast<EntityID>(5000 + i));
    }

    auto metrics = MeasureUpdatePerformance(NUM_ITERATIONS);
    PrintMetrics("Performance with Mixed Actors (20 Nations, 100 Characters)", metrics);

    EXPECT_LT(metrics.avg_time_ms, 15.0) << "Average time with mixed actors too high";
    EXPECT_LT(metrics.p99_time_ms, 16.0) << "99th percentile exceeds frame budget";
}

TEST_F(AIDirectorPerformanceTest, StressTest) {
    // Stress test with maximum expected load

    constexpr int NUM_NATIONS = 50;
    constexpr int NUM_CHARACTERS = 500;
    constexpr int NUM_ITERATIONS = 100;

    for (int i = 0; i < NUM_NATIONS; ++i) {
        ai_director->CreateNationAI(static_cast<EntityID>(1000 + i));
    }
    for (int i = 0; i < NUM_CHARACTERS; ++i) {
        ai_director->CreateCharacterAI(static_cast<EntityID>(5000 + i));
    }

    auto metrics = MeasureUpdatePerformance(NUM_ITERATIONS);
    PrintMetrics("Stress Test (50 Nations, 500 Characters)", metrics);

    // May occasionally exceed frame budget under heavy load, but should be reasonable
    EXPECT_LT(metrics.median_time_ms, 16.0) << "Median time exceeds frame budget";
    std::cout << "NOTE: Stress test may occasionally exceed frame budget on slower hardware" << std::endl;
}

TEST_F(AIDirectorPerformanceTest, ConsistencyOverTime) {
    // Verify performance remains consistent over extended runtime

    constexpr int NUM_PHASES = 10;
    constexpr int SAMPLES_PER_PHASE = 100;

    std::vector<double> phase_averages;

    for (int phase = 0; phase < NUM_PHASES; ++phase) {
        auto metrics = MeasureUpdatePerformance(SAMPLES_PER_PHASE);
        phase_averages.push_back(metrics.avg_time_ms);
    }

    // Calculate variance
    double mean = std::accumulate(phase_averages.begin(), phase_averages.end(), 0.0) / NUM_PHASES;
    double variance = 0.0;
    for (double avg : phase_averages) {
        variance += (avg - mean) * (avg - mean);
    }
    variance /= NUM_PHASES;
    double std_dev = std::sqrt(variance);

    std::cout << "\n=== Consistency Over Time ===" << std::endl;
    std::cout << "Mean performance: " << mean << " ms" << std::endl;
    std::cout << "Std deviation: " << std_dev << " ms" << std::endl;

    // Standard deviation should be low (consistent performance)
    EXPECT_LT(std_dev, mean * 0.5) << "Performance variance too high";
}

TEST_F(AIDirectorPerformanceTest, MemoryStability) {
    // Verify no memory leaks during extended operation

    constexpr int NUM_ITERATIONS = 10000;
    constexpr float DELTA_TIME = 0.016f;

    // Note: This test should be run with Valgrind or AddressSanitizer
    // for proper memory leak detection

    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        ai_director->Update(DELTA_TIME);

        // Periodically create and destroy actors
        if (i % 100 == 0) {
            auto nation_id = static_cast<EntityID>(1000 + (i / 100));
            ai_director->CreateNationAI(nation_id);
        }
    }

    SUCCEED() << "Memory stability test completed - check with memory profiler";
}

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
