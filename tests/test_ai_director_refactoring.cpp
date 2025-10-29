// ============================================================================
// Mechanica Imperii - AI Director System Refactoring Tests
// ============================================================================

#include "game/ai/calculators/AIDirectorCalculator.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace AI;

bool TestSchedulingCalculations() {
    std::cout << "\n========== Testing Scheduling Calculations ==========\n";

    // Test critical priority (immediate)
    auto critical_delay = AIDirectorCalculator::CalculateSchedulingDelay(
        MessagePriority::CRITICAL);
    assert(critical_delay.count() == 0);
    std::cout << "Critical priority (immediate): " << critical_delay.count() << "ms PASSED\n";

    // Test high priority (1 day)
    auto high_delay = AIDirectorCalculator::CalculateSchedulingDelay(
        MessagePriority::HIGH);
    assert(high_delay == std::chrono::hours(24));
    std::cout << "High priority (1 day): " << high_delay.count() << "ms PASSED\n";

    // Test medium priority (7 days)
    auto medium_delay = AIDirectorCalculator::CalculateSchedulingDelay(
        MessagePriority::MEDIUM);
    assert(medium_delay == std::chrono::hours(24 * 7));
    std::cout << "Medium priority (7 days): " << medium_delay.count() << "ms PASSED\n";

    // Test low priority (14 days)
    auto low_delay = AIDirectorCalculator::CalculateSchedulingDelay(
        MessagePriority::LOW);
    assert(low_delay == std::chrono::hours(24 * 14));
    std::cout << "Low priority (14 days): " << low_delay.count() << "ms PASSED\n";

    // Test relevance to priority mapping
    MessagePriority priority = AIDirectorCalculator::MapRelevanceToPriority(
        InformationRelevance::CRITICAL);
    assert(priority == MessagePriority::CRITICAL);
    std::cout << "Relevance mapping (CRITICAL): PASSED\n";

    std::cout << "Scheduling calculation tests: ALL PASSED\n";
    return true;
}

bool TestLoadBalancingCalculations() {
    std::cout << "\n========== Testing Load Balancing Calculations ==========\n";

    // Test actor overload detection
    bool overloaded = AIDirectorCalculator::IsActorOverloaded(60, 50);
    assert(overloaded == true);
    std::cout << "Actor overloaded (60 > 50): PASSED\n";

    bool not_overloaded = AIDirectorCalculator::IsActorOverloaded(30, 50);
    assert(not_overloaded == false);
    std::cout << "Actor not overloaded (30 <= 50): PASSED\n";

    // Test counting overloaded actors
    std::vector<uint32_t> queueSizes = {60, 70, 30, 20, 80, 10};
    uint32_t overloaded_count = AIDirectorCalculator::CountOverloadedActors(queueSizes, 50);
    assert(overloaded_count == 3); // 60, 70, 80
    std::cout << "Overloaded actors count (3): PASSED\n";

    // Test load balance action determination (increase)
    LoadBalanceAction increase = AIDirectorCalculator::DetermineLoadBalanceAction(
        6, 500, 5, 100);
    assert(increase == LoadBalanceAction::INCREASE_PROCESSING);
    std::cout << "Load balance action (INCREASE): PASSED\n";

    // Test load balance action determination (decrease)
    LoadBalanceAction decrease = AIDirectorCalculator::DetermineLoadBalanceAction(
        0, 50, 5, 100);
    assert(decrease == LoadBalanceAction::DECREASE_PROCESSING);
    std::cout << "Load balance action (DECREASE): PASSED\n";

    // Test load balance action determination (maintain)
    LoadBalanceAction maintain = AIDirectorCalculator::DetermineLoadBalanceAction(
        3, 150, 5, 100);
    assert(maintain == LoadBalanceAction::MAINTAIN);
    std::cout << "Load balance action (MAINTAIN): PASSED\n";

    // Test optimal actors per frame calculation (increase)
    uint32_t new_actors_increase = AIDirectorCalculator::CalculateOptimalActorsPerFrame(
        10, LoadBalanceAction::INCREASE_PROCESSING, 5, 20, 2);
    assert(new_actors_increase == 12); // 10 + 2
    std::cout << "Optimal actors (increase 10->12): PASSED\n";

    // Test optimal actors per frame calculation (decrease)
    uint32_t new_actors_decrease = AIDirectorCalculator::CalculateOptimalActorsPerFrame(
        10, LoadBalanceAction::DECREASE_PROCESSING, 5, 20, 2);
    assert(new_actors_decrease == 9); // 10 - 1
    std::cout << "Optimal actors (decrease 10->9): PASSED\n";

    // Test optimal actors clamping (max)
    uint32_t clamped_max = AIDirectorCalculator::CalculateOptimalActorsPerFrame(
        19, LoadBalanceAction::INCREASE_PROCESSING, 5, 20, 2);
    assert(clamped_max == 20); // Clamped to max
    std::cout << "Optimal actors (clamped to max 20): PASSED\n";

    // Test system idle detection
    bool idle = AIDirectorCalculator::IsSystemIdle(4, 10);
    assert(idle == true); // 4 < 10/2
    std::cout << "System idle (4 < 5): PASSED\n";

    bool busy = AIDirectorCalculator::IsSystemIdle(8, 10);
    assert(busy == false); // 8 >= 10/2
    std::cout << "System busy (8 >= 5): PASSED\n";

    std::cout << "Load balancing calculation tests: ALL PASSED\n";
    return true;
}

bool TestActorTypeClassification() {
    std::cout << "\n========== Testing Actor Type Classification ==========\n";

    // Test nation actor identification
    bool is_nation = AIDirectorCalculator::IsNationActor(1500);
    assert(is_nation == true);
    std::cout << "Nation actor (1500): PASSED\n";

    ActorType nation_type = AIDirectorCalculator::GetActorType(2000);
    assert(nation_type == ActorType::NATION);
    std::cout << "Actor type NATION (2000): PASSED\n";

    // Test character actor identification
    bool is_character = AIDirectorCalculator::IsCharacterActor(6000);
    assert(is_character == true);
    std::cout << "Character actor (6000): PASSED\n";

    ActorType character_type = AIDirectorCalculator::GetActorType(7500);
    assert(character_type == ActorType::CHARACTER);
    std::cout << "Actor type CHARACTER (7500): PASSED\n";

    // Test council actor identification
    bool is_council = AIDirectorCalculator::IsCouncilActor(9500);
    assert(is_council == true);
    std::cout << "Council actor (9500): PASSED\n";

    ActorType council_type = AIDirectorCalculator::GetActorType(10000);
    assert(council_type == ActorType::COUNCIL);
    std::cout << "Actor type COUNCIL (10000): PASSED\n";

    // Test boundary cases
    ActorType boundary_nation = AIDirectorCalculator::GetActorType(4999);
    assert(boundary_nation == ActorType::NATION);
    std::cout << "Boundary case (4999 = NATION): PASSED\n";

    ActorType boundary_character = AIDirectorCalculator::GetActorType(5000);
    assert(boundary_character == ActorType::CHARACTER);
    std::cout << "Boundary case (5000 = CHARACTER): PASSED\n";

    std::cout << "Actor type classification tests: ALL PASSED\n";
    return true;
}

bool TestProcessingPriorityCalculations() {
    std::cout << "\n========== Testing Processing Priority Calculations ==========\n";

    // Test priority with critical messages
    float priority_critical = AIDirectorCalculator::CalculateActorProcessingPriority(
        2, 0, ActorType::NATION);
    assert(priority_critical >= 200.0f); // 2*100 + nation bonus
    std::cout << "Priority with critical messages: " << priority_critical << " PASSED\n";

    // Test priority with high messages
    float priority_high = AIDirectorCalculator::CalculateActorProcessingPriority(
        0, 5, ActorType::CHARACTER);
    assert(priority_high >= 50.0f); // 5*10 + character bonus
    std::cout << "Priority with high messages: " << priority_high << " PASSED\n";

    // Test nation actor type bonus
    float nation_priority = AIDirectorCalculator::CalculateActorProcessingPriority(
        0, 0, ActorType::NATION);
    float character_priority = AIDirectorCalculator::CalculateActorProcessingPriority(
        0, 0, ActorType::CHARACTER);
    assert(nation_priority > character_priority); // Nations more important
    std::cout << "Nation priority > Character priority: PASSED\n";

    // Test actor comparison
    bool actor1_higher = AIDirectorCalculator::CompareActorPriority(
        1, 0, ActorType::NATION,  // Actor 1: 1 critical, nation
        0, 5, ActorType::CHARACTER // Actor 2: 5 high, character
    );
    assert(actor1_higher == true); // Critical message trumps high messages
    std::cout << "Actor comparison (critical > high): PASSED\n";

    std::cout << "Processing priority calculation tests: ALL PASSED\n";
    return true;
}

bool TestPerformanceMetricsCalculations() {
    std::cout << "\n========== Testing Performance Metrics Calculations ==========\n";

    // Test exponential moving average
    double ema = AIDirectorCalculator::CalculateExponentialMovingAverage(
        10.0, 20.0, 0.1);
    assert(std::abs(ema - 11.0) < 0.01); // 0.1*20 + 0.9*10 = 11
    std::cout << "Exponential moving average (11.0): " << ema << " PASSED\n";

    // Test average decision time
    double avg_time = AIDirectorCalculator::CalculateAverageDecisionTime(
        100.0, 10);
    assert(avg_time == 10.0); // 100 / 10 = 10
    std::cout << "Average decision time (10.0): " << avg_time << " PASSED\n";

    // Test average decision time with zero decisions
    double avg_time_zero = AIDirectorCalculator::CalculateAverageDecisionTime(
        100.0, 0);
    assert(avg_time_zero == 0.0);
    std::cout << "Average decision time (zero decisions): PASSED\n";

    // Test frame sleep time calculation
    double sleep_time = AIDirectorCalculator::CalculateFrameSleepTime(
        15.0, 20.0);
    assert(sleep_time == 5.0); // 20 - 15 = 5
    std::cout << "Frame sleep time (5.0ms): " << sleep_time << " PASSED\n";

    // Test frame sleep time when over budget
    double no_sleep = AIDirectorCalculator::CalculateFrameSleepTime(
        25.0, 20.0);
    assert(no_sleep == 0.0); // No sleep needed
    std::cout << "Frame sleep time (over budget): PASSED\n";

    // Test background task batch size (idle)
    int batch_idle = AIDirectorCalculator::CalculateBackgroundTaskBatchSize(
        true, 10);
    assert(batch_idle == 10);
    std::cout << "Background batch size (idle = 10): PASSED\n";

    // Test background task batch size (busy)
    int batch_busy = AIDirectorCalculator::CalculateBackgroundTaskBatchSize(
        false, 10);
    assert(batch_busy == 5);
    std::cout << "Background batch size (busy = 5): PASSED\n";

    std::cout << "Performance metrics calculation tests: ALL PASSED\n";
    return true;
}

bool TestUtilityFunctions() {
    std::cout << "\n========== Testing Utility Functions ==========\n";

    // Test uint32_t clamp
    uint32_t clamped_min = AIDirectorCalculator::Clamp(3u, 5u, 20u);
    assert(clamped_min == 5);
    std::cout << "Clamp (3 -> 5): PASSED\n";

    uint32_t clamped_max = AIDirectorCalculator::Clamp(25u, 5u, 20u);
    assert(clamped_max == 20);
    std::cout << "Clamp (25 -> 20): PASSED\n";

    uint32_t clamped_ok = AIDirectorCalculator::Clamp(10u, 5u, 20u);
    assert(clamped_ok == 10);
    std::cout << "Clamp (10 -> 10): PASSED\n";

    // Test double clamp
    double clamped_double = AIDirectorCalculator::Clamp(15.5, 10.0, 20.0);
    assert(clamped_double == 15.5);
    std::cout << "Clamp double (15.5 -> 15.5): PASSED\n";

    // Test percentage calculation
    float percentage = AIDirectorCalculator::CalculatePercentage(25, 100);
    assert(percentage == 25.0f);
    std::cout << "Percentage (25/100 = 25%): PASSED\n";

    float percentage_zero = AIDirectorCalculator::CalculatePercentage(10, 0);
    assert(percentage_zero == 0.0f);
    std::cout << "Percentage (division by zero = 0%): PASSED\n";

    std::cout << "Utility function tests: ALL PASSED\n";
    return true;
}

int main() {
    std::cout << "\n";
    std::cout << "========================================================\n";
    std::cout << "    AI DIRECTOR SYSTEM REFACTORING - TEST SUITE\n";
    std::cout << "========================================================\n";

    bool all_passed = true;

    all_passed &= TestSchedulingCalculations();
    all_passed &= TestLoadBalancingCalculations();
    all_passed &= TestActorTypeClassification();
    all_passed &= TestProcessingPriorityCalculations();
    all_passed &= TestPerformanceMetricsCalculations();
    all_passed &= TestUtilityFunctions();

    std::cout << "\n";
    std::cout << "========================================================\n";
    if (all_passed) {
        std::cout << "     ALL TESTS PASSED ✓\n";
    } else {
        std::cout << "     SOME TESTS FAILED ✗\n";
    }
    std::cout << "========================================================\n";
    std::cout << "\n";

    std::cout << "Refactoring Summary:\n";
    std::cout << "  - AIDirectorCalculator: Pure calculation functions for AI coordination\n";
    std::cout << "\n";
    std::cout << "Expected Benefits:\n";
    std::cout << "  - Reduced AIDirector.cpp from ~960 lines\n";
    std::cout << "  - Message scheduling and priority calculations testable\n";
    std::cout << "  - Load balancing logic isolated and tunable\n";
    std::cout << "  - Actor type classification centralized\n";
    std::cout << "  - Performance metrics calculations reusable\n";
    std::cout << "\n";

    return all_passed ? 0 : 1;
}
