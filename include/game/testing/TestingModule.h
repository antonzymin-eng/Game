// =============================================================================
// Mechanica Imperii - In-Game Testing and Tuning Module
// =============================================================================

#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace game::testing {

    // -------------------------------------------------------------------------
    // Enumerations and helper structures
    // -------------------------------------------------------------------------

    enum class LogSeverity {
        Info,
        Warning,
        Error
    };

    struct ParameterDefinition {
        std::string name;
        double default_value = 0.0;
        double min_value = 0.0;
        double max_value = 1.0;
        double step = 0.1;

        [[nodiscard]] bool IsWithinRange(double value) const noexcept {
            return value >= min_value && value <= max_value;
        }

        [[nodiscard]] double Clamp(double value) const noexcept {
            if (value < min_value) {
                return min_value;
            }
            if (value > max_value) {
                return max_value;
            }
            return value;
        }
    };

    struct ParameterOverride {
        double value = 0.0;
        std::optional<double> min_value;
        std::optional<double> max_value;
        std::optional<double> step;
    };

    struct MetricSample {
        std::string name;
        double value = 0.0;
        std::chrono::steady_clock::duration timestamp{};
    };

    struct LogEntry {
        std::string message;
        LogSeverity severity = LogSeverity::Info;
        std::chrono::steady_clock::duration timestamp{};
    };

    class TestContext;

    struct TestCase {
        std::string id;
        std::string name;
        std::string description;
        std::vector<std::string> tags;
        std::vector<ParameterDefinition> parameters;
        std::function<void(TestContext&)> execute;
    };

    struct TuningProfile {
        std::string name;
        std::string description;
        std::unordered_map<std::string, ParameterOverride> overrides;
    };

    struct TestResult {
        std::string test_id;
        std::string test_name;
        std::string profile_name;
        std::chrono::steady_clock::time_point start_time{};
        std::chrono::steady_clock::duration duration{};
        std::unordered_map<std::string, double> final_parameter_values;
        std::vector<MetricSample> metrics;
        std::vector<LogEntry> log;
        bool success = false;
        bool aborted = false;
    };

    // -------------------------------------------------------------------------
    // Test execution context
    // -------------------------------------------------------------------------

    class TestContext {
    public:
        TestContext(const std::unordered_map<std::string, ParameterDefinition>& definitions,
                    std::unordered_map<std::string, double>& values,
                    std::vector<MetricSample>& metrics,
                    std::vector<LogEntry>& log,
                    std::chrono::steady_clock::time_point start_time) noexcept;

        [[nodiscard]] bool HasParameter(const std::string& name) const noexcept;
        [[nodiscard]] double GetParameterValue(const std::string& name) const noexcept;
        void SetParameterValue(const std::string& name, double value);

        void RecordMetric(const std::string& name, double value);
        void RecordMetricWithOffset(const std::string& name,
                                    double value,
                                    std::chrono::steady_clock::duration offset);

        void Log(LogSeverity severity, const std::string& message);
        void LogInfo(const std::string& message);
        void LogWarning(const std::string& message);
        void LogError(const std::string& message);

        void FailTest(const std::string& reason);
        void Abort(const std::string& reason);

        [[nodiscard]] bool IsFailed() const noexcept { return m_failed; }
        [[nodiscard]] bool IsAborted() const noexcept { return m_aborted; }

        [[nodiscard]] std::unordered_map<std::string, double> GetParameterSnapshot() const;

    private:
        const std::unordered_map<std::string, ParameterDefinition>& m_definitions;
        std::unordered_map<std::string, double>& m_values;
        std::vector<MetricSample>& m_metrics;
        std::vector<LogEntry>& m_log;
        std::chrono::steady_clock::time_point m_start_time;
        bool m_failed = false;
        bool m_aborted = false;
    };

    // -------------------------------------------------------------------------
    // Testing module interface
    // -------------------------------------------------------------------------

    class TestingModule {
    public:
        TestingModule() = default;

        bool RegisterTestCase(TestCase test_case);
        bool RegisterProfile(TuningProfile profile);
        bool RemoveTestCase(const std::string& id) noexcept;
        bool RemoveProfile(const std::string& name) noexcept;

        [[nodiscard]] const TestCase* FindTestCase(const std::string& id) const noexcept;
        [[nodiscard]] const TuningProfile* FindProfile(const std::string& name) const noexcept;

        std::optional<TestResult> RunTest(const std::string& test_id,
                                          const std::string& profile_name = {});
        std::vector<TestResult> RunParameterSweep(const std::string& test_id,
                                                  const std::string& parameter_name,
                                                  double min_value,
                                                  double max_value,
                                                  double step,
                                                  const std::string& profile_name = {});

        [[nodiscard]] const std::vector<TestResult>& GetHistory() const noexcept { return m_history; }
        void ClearHistory() noexcept { m_history.clear(); }
        void SetHistoryLimit(std::size_t limit) noexcept;

    private:
        TestResult ExecuteTestCase(const TestCase& test_case, const TuningProfile* profile) const;
        void AppendResult(TestResult result);
        void TrimHistory();

    private:
        std::unordered_map<std::string, TestCase> m_test_cases;
        std::unordered_map<std::string, TuningProfile> m_profiles;
        std::vector<TestResult> m_history;
        std::size_t m_history_limit = 32;
    };

} // namespace game::testing

