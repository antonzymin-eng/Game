// Created: January 16, 2025 - 16:00:00
// Location: tests/core/save/SaveSystemTests.h
// Mechanica Imperii - Save System Unit Tests (C++17 Compliant)

#pragma once

#include "core/save/SaveManager.h"
#include "core/save/SaveCompression.h"
#include "core/save/IncrementalSaveTracker.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace core::save::testing {

// ============================================================================
// Test Framework Integration
// ============================================================================

class TestResult {
public:
    enum class Status { PASSED, FAILED, SKIPPED };
    
    Status status = Status::PASSED;
    std::string test_name;
    std::string error_message;
    std::chrono::milliseconds duration{0};
    
    bool Passed() const { return status == Status::PASSED; }
    bool Failed() const { return status == Status::FAILED; }
};

class TestSuite {
public:
    virtual ~TestSuite() = default;
    virtual std::string GetName() const = 0;
    virtual std::vector<TestResult> RunAllTests() = 0;
};

// ============================================================================
// Mock Serializable System for Testing
// ============================================================================

class MockSerializableSystem : public core::ecs::ISerializable {
public:
    explicit MockSerializableSystem(const std::string& name);
    
    std::string GetSystemName() const override { return m_name; }
    
    bool Serialize(Json::Value& out, int version_int) override;
    bool Deserialize(const Json::Value& in, int version_int) override;
    
    // Test helpers
    void SetData(const std::string& key, const std::string& value);
    std::string GetData(const std::string& key) const;
    void SetShouldFail(bool fail) { m_should_fail = fail; }
    int GetSerializeCallCount() const { return m_serialize_count; }
    int GetDeserializeCallCount() const { return m_deserialize_count; }
    void ResetCounts();
    
private:
    std::string m_name;
    std::unordered_map<std::string, std::string> m_data;
    bool m_should_fail = false;
    int m_serialize_count = 0;
    int m_deserialize_count = 0;
};

// ============================================================================
// SaveManager Test Suite
// ============================================================================

class SaveManagerTests : public TestSuite {
public:
    SaveManagerTests();
    
    std::string GetName() const override { return "SaveManager Tests"; }
    std::vector<TestResult> RunAllTests() override;
    
    // Individual test methods
    TestResult TestBasicSaveLoad();
    TestResult TestAtomicWrites();
    TestResult TestBackupCreation();
    TestResult TestChecksumValidation();
    TestResult TestConcurrentSaves();
    TestResult TestPathTraversalPrevention();
    TestResult TestInvalidFilenames();
    TestResult TestDiskSpaceCheck();
    TestResult TestCorruptionDetection();
    TestResult TestMigrationSystem();
    TestResult TestCrashRecovery();
    TestResult TestValidationSystem();
    TestResult TestOperationTimeout();
    TestResult TestCancellation();
    
private:
    std::filesystem::path m_test_dir;
    
    void SetupTestEnvironment();
    void CleanupTestEnvironment();
    TestResult RunTest(const std::string& name, std::function<bool()> test_func);
};

// ============================================================================
// Compression Test Suite
// ============================================================================

class CompressionTests : public TestSuite {
public:
    CompressionTests();
    
    std::string GetName() const override { return "Compression Tests"; }
    std::vector<TestResult> RunAllTests() override;
    
    // Individual test methods
    TestResult TestLZ4Compression();
    TestResult TestLZ4Decompression();
    TestResult TestCompressionRatio();
    TestResult TestLargeDataCompression();
    TestResult TestEmptyDataHandling();
    TestResult TestCorruptedDataDetection();
    TestResult TestChecksumValidation();
    TestResult TestCompressionLevels();
    TestResult TestNullCompressor();
    TestResult TestCompressionManager();
    TestResult TestBenchmarking();
    TestResult TestEntropyCalculation();
    
private:
    TestResult RunTest(const std::string& name, std::function<bool()> test_func);
    std::vector<uint8_t> GenerateTestData(size_t size, uint8_t pattern = 0);
    std::vector<uint8_t> GenerateRandomData(size_t size);
};

// ============================================================================
// Incremental Save Test Suite
// ============================================================================

class IncrementalSaveTests : public TestSuite {
public:
    IncrementalSaveTests();
    
    std::string GetName() const override { return "Incremental Save Tests"; }
    std::vector<TestResult> RunAllTests() override;
    
    // Individual test methods
    TestResult TestSystemRegistration();
    TestResult TestDirtyTracking();
    TestResult TestAutoSaveTriggers();
    TestResult TestCriticalChanges();
    TestResult TestContentHashing();
    TestResult TestChangeEvents();
    TestResult TestStatistics();
    TestResult TestSaveOptimization();
    TestResult TestIncrementalSaveManager();
    TestResult TestDirtySystemFilter();
    TestResult TestBulkOperations();
    
private:
    TestResult RunTest(const std::string& name, std::function<bool()> test_func);
};

// ============================================================================
// Integration Test Suite
// ============================================================================

class IntegrationTests : public TestSuite {
public:
    IntegrationTests();
    
    std::string GetName() const override { return "Integration Tests"; }
    std::vector<TestResult> RunAllTests() override;
    
    // Individual test methods
    TestResult TestFullSaveLoadCycle();
    TestResult TestIncrementalSaveWithCompression();
    TestResult TestMigrationWithValidation();
    TestResult TestCrashRecoveryWithBackups();
    TestResult TestConcurrentOperationsStress();
    TestResult TestLargeScaleSave();
    TestResult TestAutoSaveIntegration();
    
private:
    std::filesystem::path m_test_dir;
    TestResult RunTest(const std::string& name, std::function<bool()> test_func);
    void SetupTestEnvironment();
    void CleanupTestEnvironment();
};

// ============================================================================
// Performance Test Suite
// ============================================================================

class PerformanceTests : public TestSuite {
public:
    PerformanceTests();
    
    std::string GetName() const override { return "Performance Tests"; }
    std::vector<TestResult> RunAllTests() override;
    
    // Individual test methods
    TestResult TestSavePerformance();
    TestResult TestLoadPerformance();
    TestResult TestCompressionPerformance();
    TestResult TestIncrementalVsFullSave();
    TestResult TestConcurrentSavePerformance();
    TestResult TestValidationPerformance();
    
    struct PerformanceMetrics {
        std::chrono::milliseconds duration{0};
        size_t operations_per_second = 0;
        size_t bytes_per_second = 0;
        double throughput_mbps = 0.0;
        
        std::string GenerateReport() const;
    };
    
private:
    TestResult RunTest(const std::string& name, std::function<bool()> test_func);
    PerformanceMetrics MeasurePerformance(std::function<void()> operation, size_t iterations);
};

// ============================================================================
// Test Runner
// ============================================================================

class SaveSystemTestRunner {
public:
    SaveSystemTestRunner();
    
    void AddTestSuite(std::unique_ptr<TestSuite> suite);
    void RunAllSuites();
    void GenerateReport() const;
    
    struct TestSummary {
        size_t total_tests = 0;
        size_t passed_tests = 0;
        size_t failed_tests = 0;
        size_t skipped_tests = 0;
        std::chrono::milliseconds total_duration{0};
        std::vector<TestResult> all_results;
        
        double GetPassRate() const;
        std::string GenerateReport() const;
    };
    
    TestSummary GetSummary() const { return m_summary; }
    
private:
    std::vector<std::unique_ptr<TestSuite>> m_suites;
    TestSummary m_summary;
};

// ============================================================================
// Test Utilities
// ============================================================================

namespace test_utils {
    // Assertion helpers
    void AssertTrue(bool condition, const std::string& message);
    void AssertFalse(bool condition, const std::string& message);
    void AssertEqual(int expected, int actual, const std::string& message);
    void AssertEqual(const std::string& expected, const std::string& actual, const std::string& message);
    void AssertNotNull(const void* ptr, const std::string& message);
    
    // File system helpers
    std::filesystem::path CreateTempDirectory();
    void RemoveDirectory(const std::filesystem::path& path);
    void CreateTestFile(const std::filesystem::path& path, const std::string& content);
    std::string ReadTestFile(const std::filesystem::path& path);
    
    // Data generation helpers
    std::vector<uint8_t> GenerateRandomBytes(size_t size);
    std::string GenerateRandomString(size_t length);
    Json::Value GenerateTestJSON(size_t complexity);
    
    // Timing helpers
    class Timer {
    public:
        void Start();
        void Stop();
        std::chrono::milliseconds GetDuration() const;
    private:
        std::chrono::steady_clock::time_point m_start;
        std::chrono::steady_clock::time_point m_stop;
    };
}

} // namespace core::save::testing
