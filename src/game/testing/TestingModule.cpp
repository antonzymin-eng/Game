// =============================================================================
// Mechanica Imperii - In-Game Testing and Tuning Module Implementation
// =============================================================================

#include "game/testing/TestingModule.h"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace game::testing {

    namespace {
        double ClampValue(double value, double min_value, double max_value) {
            if (value < min_value) {
                return min_value;
            }
            if (value > max_value) {
                return max_value;
            }
            return value;
        }
    } // namespace

    TestContext::TestContext(const std::unordered_map<std::string, ParameterDefinition>& definitions,
                             std::unordered_map<std::string, double>& values,
                             std::vector<MetricSample>& metrics,
                             std::vector<LogEntry>& log,
                             std::chrono::steady_clock::time_point start_time) noexcept
        : m_definitions(definitions)
        , m_values(values)
        , m_metrics(metrics)
        , m_log(log)
        , m_start_time(start_time) {}

    bool TestContext::HasParameter(const std::string& name) const noexcept {
        return m_values.find(name) != m_values.end();
    }

    double TestContext::GetParameterValue(const std::string& name) const noexcept {
        auto it = m_values.find(name);
        if (it != m_values.end()) {
            return it->second;
        }
        return 0.0;
    }

    void TestContext::SetParameterValue(const std::string& name, double value) {
        auto def_it = m_definitions.find(name);
        if (def_it == m_definitions.end()) {
            LogWarning("Attempted to set unknown tuning parameter '" + name + "'");
            return;
        }

        const auto& definition = def_it->second;
        double clamped = definition.Clamp(value);

        if (definition.step > 0.0) {
            double steps = std::round((clamped - definition.min_value) / definition.step);
            clamped = definition.min_value + steps * definition.step;
            clamped = definition.Clamp(clamped);
        }

        if (!definition.IsWithinRange(value)) {
            std::ostringstream oss;
            oss << "Parameter '" << name << "' value " << value
                << " was clamped to " << clamped << " (range " << definition.min_value
                << " - " << definition.max_value << ")";
            LogWarning(oss.str());
        }

        m_values[name] = clamped;
    }

    void TestContext::RecordMetric(const std::string& name, double value) {
        auto now = std::chrono::steady_clock::now();
        m_metrics.push_back({name, value, now - m_start_time});
    }

    void TestContext::RecordMetricWithOffset(const std::string& name,
                                             double value,
                                             std::chrono::steady_clock::duration offset) {
        m_metrics.push_back({name, value, offset});
    }

    void TestContext::Log(LogSeverity severity, const std::string& message) {
        auto now = std::chrono::steady_clock::now();
        m_log.push_back({message, severity, now - m_start_time});
    }

    void TestContext::LogInfo(const std::string& message) {
        Log(LogSeverity::Info, message);
    }

    void TestContext::LogWarning(const std::string& message) {
        Log(LogSeverity::Warning, message);
    }

    void TestContext::LogError(const std::string& message) {
        Log(LogSeverity::Error, message);
    }

    void TestContext::FailTest(const std::string& reason) {
        m_failed = true;
        LogError(reason);
    }

    void TestContext::Abort(const std::string& reason) {
        m_aborted = true;
        LogWarning("Test aborted: " + reason);
    }

    std::unordered_map<std::string, double> TestContext::GetParameterSnapshot() const {
        return m_values;
    }

    bool TestingModule::RegisterTestCase(TestCase test_case) {
        if (test_case.id.empty() || !test_case.execute) {
            return false;
        }

        return m_test_cases.emplace(test_case.id, std::move(test_case)).second;
    }

    bool TestingModule::RegisterProfile(TuningProfile profile) {
        if (profile.name.empty()) {
            return false;
        }

        return m_profiles.emplace(profile.name, std::move(profile)).second;
    }

    bool TestingModule::RemoveTestCase(const std::string& id) noexcept {
        return m_test_cases.erase(id) > 0;
    }

    bool TestingModule::RemoveProfile(const std::string& name) noexcept {
        return m_profiles.erase(name) > 0;
    }

    const TestCase* TestingModule::FindTestCase(const std::string& id) const noexcept {
        auto it = m_test_cases.find(id);
        if (it != m_test_cases.end()) {
            return &it->second;
        }
        return nullptr;
    }

    const TuningProfile* TestingModule::FindProfile(const std::string& name) const noexcept {
        auto it = m_profiles.find(name);
        if (it != m_profiles.end()) {
            return &it->second;
        }
        return nullptr;
    }

    std::optional<TestResult> TestingModule::RunTest(const std::string& test_id,
                                                     const std::string& profile_name) {
        const auto* test_case = FindTestCase(test_id);
        if (!test_case) {
            return std::nullopt;
        }

        const TuningProfile* profile = nullptr;
        if (!profile_name.empty()) {
            profile = FindProfile(profile_name);
            if (!profile) {
                return std::nullopt;
            }
        }

        auto result = ExecuteTestCase(*test_case, profile);
        AppendResult(result);
        return result;
    }

    std::vector<TestResult> TestingModule::RunParameterSweep(const std::string& test_id,
                                                             const std::string& parameter_name,
                                                             double min_value,
                                                             double max_value,
                                                             double step,
                                                             const std::string& profile_name) {
        std::vector<TestResult> results;

        if (step <= 0.0 || min_value > max_value) {
            return results;
        }

        const auto* test_case = FindTestCase(test_id);
        if (!test_case) {
            return results;
        }

        const TuningProfile* base_profile = nullptr;
        if (!profile_name.empty()) {
            base_profile = FindProfile(profile_name);
            if (!base_profile) {
                return results;
            }
        }

        const double epsilon = step * 0.25;
        for (double value = min_value; value <= max_value + epsilon; value += step) {
            TuningProfile profile_copy;
            if (base_profile) {
                profile_copy = *base_profile;
                profile_copy.name = base_profile->name + " [sweep]";
            } else {
                profile_copy.name = parameter_name + "_sweep";
            }

            ParameterOverride override;
            override.value = ClampValue(value, min_value, max_value);
            override.min_value = min_value;
            override.max_value = max_value;
            override.step = step;
            profile_copy.overrides[parameter_name] = override;

            auto result = ExecuteTestCase(*test_case, &profile_copy);
            AppendResult(result);
            results.push_back(std::move(result));
        }

        return results;
    }

    void TestingModule::SetHistoryLimit(std::size_t limit) noexcept {
        m_history_limit = std::max<std::size_t>(1, limit);
        TrimHistory();
    }

    TestResult TestingModule::ExecuteTestCase(const TestCase& test_case, const TuningProfile* profile) const {
        std::unordered_map<std::string, ParameterDefinition> definition_map;
        std::unordered_map<std::string, double> value_map;
        definition_map.reserve(test_case.parameters.size());
        value_map.reserve(test_case.parameters.size());

        for (const auto& definition : test_case.parameters) {
            definition_map.emplace(definition.name, definition);
            value_map.emplace(definition.name, definition.default_value);
        }

        if (profile) {
            for (const auto& [name, override] : profile->overrides) {
                auto def_it = definition_map.find(name);
                if (def_it == definition_map.end()) {
                    continue;
                }

                const auto& definition = def_it->second;
                double min_value = override.min_value.value_or(definition.min_value);
                double max_value = override.max_value.value_or(definition.max_value);
                double applied = ClampValue(override.value, min_value, max_value);

                if (override.step.has_value() && override.step.value() > 0.0) {
                    double step_value = override.step.value();
                    double steps = std::round((applied - min_value) / step_value);
                    applied = min_value + steps * step_value;
                    applied = ClampValue(applied, min_value, max_value);
                }

                value_map[name] = applied;
            }
        }

        TestResult result;
        result.test_id = test_case.id;
        result.test_name = test_case.name;
        result.profile_name = profile ? profile->name : std::string{};
        result.start_time = std::chrono::steady_clock::now();

        TestContext context(definition_map, value_map, result.metrics, result.log, result.start_time);

        try {
            test_case.execute(context);
        }
        catch (const std::exception& e) {
            context.FailTest(std::string("Unhandled exception: ") + e.what());
        }
        catch (...) {
            context.FailTest("Unhandled non-standard exception during test execution");
        }

        auto end_time = std::chrono::steady_clock::now();
        result.duration = end_time - result.start_time;
        result.final_parameter_values = context.GetParameterSnapshot();
        result.success = !context.IsFailed() && !context.IsAborted();
        result.aborted = context.IsAborted();

        return result;
    }

    void TestingModule::AppendResult(TestResult result) {
        m_history.push_back(std::move(result));
        TrimHistory();
    }

    void TestingModule::TrimHistory() {
        if (m_history_limit == 0) {
            m_history.clear();
            return;
        }

        if (m_history.size() <= m_history_limit) {
            return;
        }

        auto trim_count = m_history.size() - m_history_limit;
        m_history.erase(m_history.begin(), m_history.begin() + static_cast<std::ptrdiff_t>(trim_count));
    }

} // namespace game::testing

