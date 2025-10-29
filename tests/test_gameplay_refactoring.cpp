// ============================================================================
// Mechanica Imperii - Gameplay System Refactoring Tests
// ============================================================================

#include "game/gameplay/GameplayCalculator.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace game::gameplay;

bool TestDecisionQuality() {
    std::cout << "\n========== Testing Decision Quality ==========\n";

    // Test base quality
    double quality = GameplayCalculator::CalculateBaseDecisionQuality(0.7);
    assert(quality > 0.6 && quality < 0.8);
    std::cout << "Base decision quality: " << quality << " PASSED\n";

    // Test urgency penalty
    double with_urgency = GameplayCalculator::ApplyUrgencyPenalty(0.7, true);
    assert(with_urgency < 0.7);
    std::cout << "Urgency penalty applied: " << with_urgency << " PASSED\n";

    // Test importance bonus
    double with_bonus = GameplayCalculator::ApplyImportanceBonus(0.7, 1.5);
    assert(with_bonus > 0.7);
    std::cout << "Importance bonus applied: " << with_bonus << " PASSED\n";

    std::cout << "Decision quality tests: ALL PASSED\n";
    return true;
}

bool TestEscalation() {
    std::cout << "\n========== Testing Escalation ==========\n";

    // Test escalation factor
    double factor = GameplayCalculator::CalculateEscalationFactor(
        0.3,  // Low performance
        true,  // Urgent
        1.5,   // Important
        DecisionScope::NATIONAL,
        0.6    // Threshold
    );
    assert(factor > 1.0 && factor <= 5.0);
    std::cout << "Escalation factor: " << factor << " PASSED\n";

    // Test should escalate
    bool should_escalate = GameplayCalculator::ShouldEscalate(
        ConsequenceSeverity::MODERATE,
        0.3,  // Low performance
        false,
        1.0,
        DecisionScope::LOCAL,
        0.6
    );
    assert(should_escalate == true);
    std::cout << "Should escalate (low performance): PASSED\n";

    std::cout << "Escalation tests: ALL PASSED\n";
    return true;
}

bool TestSeverity() {
    std::cout << "\n========== Testing Severity ==========\n";

    // Test severity determination
    ConsequenceSeverity high_quality = GameplayCalculator::DetermineSeverity(0.9);
    assert(high_quality == ConsequenceSeverity::MINOR);
    std::cout << "High quality = MINOR severity: PASSED\n";

    ConsequenceSeverity low_quality = GameplayCalculator::DetermineSeverity(0.2);
    assert(low_quality == ConsequenceSeverity::CRITICAL);
    std::cout << "Low quality = CRITICAL severity: PASSED\n";

    // Test severity escalation
    ConsequenceSeverity escalated = GameplayCalculator::EscalateSeverity(ConsequenceSeverity::MINOR);
    assert(escalated == ConsequenceSeverity::MODERATE);
    std::cout << "Severity escalation: PASSED\n";

    std::cout << "Severity tests: ALL PASSED\n";
    return true;
}

int main() {
    std::cout << "\n";
    std::cout << "========================================================\n";
    std::cout << "   GAMEPLAY SYSTEM REFACTORING - TEST SUITE\n";
    std::cout << "========================================================\n";

    bool all_passed = true;

    all_passed &= TestDecisionQuality();
    all_passed &= TestEscalation();
    all_passed &= TestSeverity();

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
    std::cout << "  - GameplayCalculator: Pure calculation functions extracted\n";
    std::cout << "\n";
    std::cout << "Expected Benefits:\n";
    std::cout << "  - Reduced CoreGameplaySystem.cpp from ~1,353 lines\n";
    std::cout << "  - Decision quality calculations testable\n";
    std::cout << "  - Escalation logic isolated and tunable\n";
    std::cout << "\n";

    return all_passed ? 0 : 1;
}
