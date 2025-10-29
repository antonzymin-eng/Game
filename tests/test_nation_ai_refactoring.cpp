// ============================================================================
// Mechanica Imperii - Nation AI System Refactoring Tests
// ============================================================================

#include "game/ai/calculators/NationAICalculator.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace game::ai;

bool TestStrategicGoalCalculations() {
    std::cout << "\n========== Testing Strategic Goal Calculations ==========\n";

    // Test expansion goal (aggressive nation)
    float expansion_score = NationAICalculator::ScoreGoalDesirability(
        StrategicGoal::EXPANSION, 0.9f, 0.7f, 0.8f, 0.6f);
    assert(expansion_score > 0.7f);
    std::cout << "Expansion goal (aggressive): " << expansion_score << " PASSED\n";

    // Test consolidation goal (peaceful nation)
    float consolidation_score = NationAICalculator::ScoreGoalDesirability(
        StrategicGoal::CONSOLIDATION, 0.2f, 0.6f, 0.5f, 0.9f);
    assert(consolidation_score > 0.6f);
    std::cout << "Consolidation goal (peaceful): " << consolidation_score << " PASSED\n";

    // Test goal progress
    float progress = NationAICalculator::CalculateGoalProgress(
        StrategicGoal::EXPANSION, 25, 10000.0, 0.8f);
    assert(progress == 0.5f); // 25/50 provinces
    std::cout << "Goal progress (50%): " << progress << " PASSED\n";

    // Test goal achieved
    bool achieved = NationAICalculator::IsGoalAchieved(StrategicGoal::EXPANSION, 0.85f);
    assert(achieved == true);
    std::cout << "Goal achieved (85%): PASSED\n";

    std::cout << "Strategic goal calculation tests: ALL PASSED\n";
    return true;
}

bool TestWarDecisionCalculations() {
    std::cout << "\n========== Testing War Decision Calculations ==========\n";

    // Test war success chance
    float success_chance = NationAICalculator::CalculateWarSuccessChance(
        1.5f, 0.7f, 0.6f);
    assert(success_chance > 0.9f); // Strong relative strength
    std::cout << "War success chance (strong): " << success_chance << " PASSED\n";

    // Test relative strength
    float relative_strength = NationAICalculator::CalculateRelativeStrength(
        2000, 1000, 0.8f, 0.6f);
    assert(relative_strength > 1.5f); // 2:1 military advantage
    std::cout << "Relative strength (2:1): " << relative_strength << " PASSED\n";

    // Test war desirability
    float war_desirability = NationAICalculator::CalculateWarDesirability(
        0.8f, 0.9f, StrategicGoal::EXPANSION, 0.5f);
    assert(war_desirability > 0.7f);
    std::cout << "War desirability (expansion goal): " << war_desirability << " PASSED\n";

    // Test should declare war
    bool should_declare = NationAICalculator::ShouldDeclareWar(0.7f, 0.8f, 0.6f);
    assert(should_declare == true);
    std::cout << "Should declare war (high desirability): PASSED\n";

    std::cout << "War decision calculation tests: ALL PASSED\n";
    return true;
}

bool TestThreatAssessment() {
    std::cout << "\n========== Testing Threat Assessment ==========\n";

    // Test critical threat (at war)
    ThreatLevel critical = NationAICalculator::AssessThreat(-0.8f, -80.0f, 1.0f, true);
    assert(critical == ThreatLevel::CRITICAL);
    std::cout << "Critical threat (at war): PASSED\n";

    // Test severe threat
    ThreatLevel severe = NationAICalculator::AssessThreat(-0.6f, -70.0f, 0.9f, false);
    assert(severe >= ThreatLevel::SEVERE);
    std::cout << "Severe threat (strong hostile neighbor): PASSED\n";

    // Test minor threat
    ThreatLevel minor = NationAICalculator::AssessThreat(0.3f, -10.0f, 0.5f, false);
    assert(minor <= ThreatLevel::MINOR);
    std::cout << "Minor threat (weak dislike): PASSED\n";

    // Test threat score
    float threat_score = NationAICalculator::CalculateThreatScore(
        3000, 1000, -60.0f, true);
    assert(threat_score > 0.6f); // Much stronger hostile neighbor
    std::cout << "Threat score (3:1 hostile neighbor): " << threat_score << " PASSED\n";

    std::cout << "Threat assessment tests: ALL PASSED\n";
    return true;
}

bool TestEconomicCalculations() {
    std::cout << "\n========== Testing Economic Calculations ==========\n";

    // Test healthy economy
    float healthy = NationAICalculator::CalculateEconomicHealth(
        12000.0, 1000.0, 800.0);
    assert(healthy > 0.8f); // Good reserves and positive income
    std::cout << "Healthy economy: " << healthy << " PASSED\n";

    // Test struggling economy
    float struggling = NationAICalculator::CalculateEconomicHealth(
        1000.0, 500.0, 600.0);
    assert(struggling < 0.5f); // Low reserves and deficit
    std::cout << "Struggling economy: " << struggling << " PASSED\n";

    // Test economic action determination
    auto action_emergency = NationAICalculator::DetermineEconomicAction(
        0.2f, StrategicGoal::EXPANSION);
    assert(action_emergency == EconomicDecision::ADJUST_TAXES);
    std::cout << "Economic action (emergency): ADJUST_TAXES PASSED\n";

    auto action_investment = NationAICalculator::DetermineEconomicAction(
        0.8f, StrategicGoal::ECONOMIC_GROWTH);
    assert(action_investment == EconomicDecision::BUILD_INFRASTRUCTURE);
    std::cout << "Economic action (wealthy): BUILD_INFRASTRUCTURE PASSED\n";

    // Test tax adjustment
    float tax_adjustment = NationAICalculator::CalculateTaxAdjustment(0.3f, 0.6f);
    assert(tax_adjustment > 0.0f && tax_adjustment <= 0.2f);
    std::cout << "Tax adjustment: " << tax_adjustment << " PASSED\n";

    std::cout << "Economic calculation tests: ALL PASSED\n";
    return true;
}

bool TestMilitaryCalculations() {
    std::cout << "\n========== Testing Military Calculations ==========\n";

    // Test military readiness
    float readiness = NationAICalculator::CalculateMilitaryReadiness(
        6000, 10, 300);
    assert(readiness == 2.0f); // 6000 / (10*300) = 2.0
    std::cout << "Military readiness (overstaffed): " << readiness << " PASSED\n";

    // Test required forces
    int required = NationAICalculator::CalculateRequiredForces(
        StrategicGoal::EXPANSION, 10, 0.5f);
    assert(required == 3500); // 10 * (300 + 50)
    std::cout << "Required forces (expansion): " << required << " PASSED\n";

    // Test military action determination
    auto action_insufficient = NationAICalculator::DetermineMilitaryAction(
        0.4f, StrategicGoal::EXPANSION, 3000.0f);
    assert(action_insufficient == MilitaryDecision::RAISE_LEVIES);
    std::cout << "Military action (insufficient): RAISE_LEVIES PASSED\n";

    auto action_overstaffed = NationAICalculator::DetermineMilitaryAction(
        1.8f, StrategicGoal::CONSOLIDATION, 5000.0f);
    assert(action_overstaffed == MilitaryDecision::DISMISS_TROOPS);
    std::cout << "Military action (overstaffed): DISMISS_TROOPS PASSED\n";

    std::cout << "Military calculation tests: ALL PASSED\n";
    return true;
}

bool TestDiplomaticCalculations() {
    std::cout << "\n========== Testing Diplomatic Calculations ==========\n";

    // Test relationship score with alliance
    float score_allied = NationAICalculator::CalculateRelationshipScore(
        60.0f, true, false, true);
    assert(score_allied > 1.0f); // 0.6 + 0.5 + 0.2
    std::cout << "Relationship score (allied): " << score_allied << " PASSED\n";

    // Test relationship score at war
    float score_war = NationAICalculator::CalculateRelationshipScore(
        -30.0f, false, true, false);
    assert(score_war < -1.0f); // -0.3 - 1.0
    std::cout << "Relationship score (at war): " << score_war << " PASSED\n";

    // Test diplomatic action
    auto action_alliance = NationAICalculator::DetermineDiplomaticAction(
        0.8f, ThreatLevel::MINOR, StrategicGoal::DIPLOMATIC_DOMINANCE);
    assert(action_alliance == DiplomaticDecision::FORM_ALLIANCE);
    std::cout << "Diplomatic action (friendly): FORM_ALLIANCE PASSED\n";

    auto action_hostile = NationAICalculator::DetermineDiplomaticAction(
        -0.6f, ThreatLevel::SEVERE, StrategicGoal::EXPANSION);
    assert(action_hostile == DiplomaticDecision::DENOUNCE);
    std::cout << "Diplomatic action (hostile threat): DENOUNCE PASSED\n";

    // Test alliance value
    float alliance_value = NationAICalculator::CalculateAllianceValue(
        0.8f, 0.7f, 0.6f);
    assert(alliance_value > 0.6f); // High value alliance
    std::cout << "Alliance value (high): " << alliance_value << " PASSED\n";

    std::cout << "Diplomatic calculation tests: ALL PASSED\n";
    return true;
}

bool TestPersonalityAdjustments() {
    std::cout << "\n========== Testing Personality Adjustments ==========\n";

    // Test aggressiveness adjustment (low stability)
    float aggression_reduced = NationAICalculator::AdjustAggressiveness(
        0.8f, 0.2f, 0.5f, 2);
    assert(aggression_reduced < 0.6f); // Significantly reduced
    std::cout << "Aggressiveness reduced (low stability): " << aggression_reduced << " PASSED\n";

    // Test risk tolerance adjustment (high treasury)
    float risk_increased = NationAICalculator::AdjustRiskTolerance(
        0.5f, 15000.0f, 1);
    assert(risk_increased > 0.5f);
    std::cout << "Risk tolerance increased (wealthy): " << risk_increased << " PASSED\n";

    // Test risk tolerance adjustment (many threats)
    float risk_reduced = NationAICalculator::AdjustRiskTolerance(
        0.7f, 5000.0f, 3);
    assert(risk_reduced < 0.6f);
    std::cout << "Risk tolerance reduced (many threats): " << risk_reduced << " PASSED\n";

    std::cout << "Personality adjustment tests: ALL PASSED\n";
    return true;
}

int main() {
    std::cout << "\n";
    std::cout << "========================================================\n";
    std::cout << "    NATION AI SYSTEM REFACTORING - TEST SUITE\n";
    std::cout << "========================================================\n";

    bool all_passed = true;

    all_passed &= TestStrategicGoalCalculations();
    all_passed &= TestWarDecisionCalculations();
    all_passed &= TestThreatAssessment();
    all_passed &= TestEconomicCalculations();
    all_passed &= TestMilitaryCalculations();
    all_passed &= TestDiplomaticCalculations();
    all_passed &= TestPersonalityAdjustments();

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
    std::cout << "  - NationAICalculator: Pure calculation functions for nation AI\n";
    std::cout << "\n";
    std::cout << "Expected Benefits:\n";
    std::cout << "  - Reduced NationAI.cpp from ~1,040 lines\n";
    std::cout << "  - Strategic goal scoring testable\n";
    std::cout << "  - War decisions and threat assessment isolated\n";
    std::cout << "  - Economic and military calculations centralized\n";
    std::cout << "  - Easy to tune nation AI behavior\n";
    std::cout << "\n";

    return all_passed ? 0 : 1;
}
