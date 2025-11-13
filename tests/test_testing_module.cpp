// =============================================================================
// Mechanica Imperii - Testing Module Validation Tests
// =============================================================================

#include "game/testing/TestingModule.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace game::testing;

namespace {

TestCase CreateGrowthRateTestCase() {
    TestCase test_case;
    test_case.id = "growth_rate_stability";
    test_case.name = "Growth Rate Stability";
    test_case.description = "Validates tuning context helpers and logging";
    test_case.tags = {"economy", "stability"};

    ParameterDefinition definition;
    definition.name = "growth_rate";
    definition.default_value = 0.3;
    definition.min_value = 0.0;
    definition.max_value = 1.0;
    definition.step = 0.05;
    test_case.parameters.push_back(definition);

    test_case.execute = [](TestContext& context) {
        context.LogInfo("Starting growth rate stability test");

        const double initial_value = context.GetParameterValue("growth_rate");
        context.RecordMetric("initial_growth", initial_value);

        context.SetParameterValue("growth_rate", initial_value + 0.4);
        context.RecordMetricWithOffset(
            "adjusted_growth",
            context.GetParameterValue("growth_rate"),
            std::chrono::milliseconds(10));

        if (context.GetParameterValue("growth_rate") < 0.2) {
            context.FailTest("Growth rate fell below safety threshold");
        }
    };

    return test_case;
}

TuningProfile CreateAggressiveProfile() {
    TuningProfile profile;
    profile.name = "aggressive_growth";
    profile.description = "Pushes growth to the upper bound";

    ParameterOverride override;
    override.value = 0.95;
    override.min_value = 0.0;
    override.max_value = 1.0;
    override.step = 0.05;
    profile.overrides.emplace("growth_rate", override);

    return profile;
}

bool ContainsClampWarning(const std::vector<LogEntry>& log_entries) {
    return std::any_of(log_entries.begin(), log_entries.end(), [](const LogEntry& entry) {
        return entry.severity == LogSeverity::Warning && entry.message.find("clamped") != std::string::npos;
    });
}

} // namespace

int main() {
    std::cout << "\n========== TestingModule validation ==========" << std::endl;

    TestingModule module;
    module.SetHistoryLimit(4);

    TestCase growth_test = CreateGrowthRateTestCase();
    assert(module.RegisterTestCase(std::move(growth_test)) && "Expected test case registration to succeed");

    TuningProfile aggressive_profile = CreateAggressiveProfile();
    assert(module.RegisterProfile(std::move(aggressive_profile)) && "Expected profile registration to succeed");

    // Validate baseline execution
    auto baseline_result = module.RunTest("growth_rate_stability");
    assert(baseline_result.has_value() && "Baseline test should run");
    assert(baseline_result->success && !baseline_result->aborted);
    assert(baseline_result->metrics.size() == 2);

    const auto baseline_param = baseline_result->final_parameter_values.find("growth_rate");
    assert(baseline_param != baseline_result->final_parameter_values.end());
    assert(std::abs(baseline_param->second - 0.7) < 1e-6);

    // Validate profile override execution and clamping behaviour
    auto aggressive_result = module.RunTest("growth_rate_stability", "aggressive_growth");
    assert(aggressive_result.has_value() && "Profile-driven test should run");
    assert(aggressive_result->success && !aggressive_result->aborted);
    assert(ContainsClampWarning(aggressive_result->log));

    const auto aggressive_param = aggressive_result->final_parameter_values.find("growth_rate");
    assert(aggressive_param != aggressive_result->final_parameter_values.end());
    assert(std::abs(aggressive_param->second - 1.0) < 1e-6);

    // Validate parameter sweep and history trimming
    auto sweep_results = module.RunParameterSweep("growth_rate_stability", "growth_rate", 0.2, 0.4, 0.1);
    assert(sweep_results.size() == 3);
    assert(module.GetHistory().size() == 4);

    module.SetHistoryLimit(2);
    assert(module.GetHistory().size() == 2);

    // Invalid lookups should gracefully fail
    auto missing_test = module.RunTest("unknown_test");
    assert(!missing_test.has_value());

    auto missing_profile = module.RunTest("growth_rate_stability", "missing_profile");
    assert(!missing_profile.has_value());

    std::cout << "TestingModule validation: ALL PASSED" << std::endl;
    return 0;
}
