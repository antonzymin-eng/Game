// ============================================================================
// Mechanica Imperii - AI System Refactoring Tests
// ============================================================================

#include "game/ai/calculators/AICalculator.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace AI;

bool TestPlotCalculations() {
    std::cout << "\n========== Testing Plot Calculations ==========\n";

    // Test assassination success chance (dishonorable + bold)
    float assassin_chance = AICalculator::CalculatePlotSuccessChance(
        PlotDecision::ASSASSINATION, 0.9f, 0.2f, 0.5f);
    assert(assassin_chance > 0.2f);
    std::cout << "Assassination success chance (bold, dishonorable): " << assassin_chance << " PASSED\n";

    // Test coup success chance (bold, disloyal)
    float coup_chance = AICalculator::CalculatePlotSuccessChance(
        PlotDecision::COUP, 0.9f, 0.5f, 0.2f);
    assert(coup_chance > 0.1f && coup_chance < 0.3f);
    std::cout << "Coup success chance: " << coup_chance << " PASSED\n";

    // Test plot desirability
    float desirability = AICalculator::CalculatePlotDesirability(
        PlotDecision::FABRICATE_CLAIM, 0.8f, 0.5f, 0.4f, 0.6f);
    assert(desirability > 0.5f);
    std::cout << "Plot desirability (ambitious): " << desirability << " PASSED\n";

    // Test plot risk
    float risk = AICalculator::CalculatePlotRisk(PlotDecision::ASSASSINATION);
    assert(risk == 0.9f);
    std::cout << "Assassination risk: " << risk << " PASSED\n";

    // Test should execute
    bool should_execute = AICalculator::ShouldExecutePlot(0.7f, 0.5f, 0.8f, 0.6f);
    assert(should_execute == true);
    std::cout << "Should execute plot (desirable + viable): PASSED\n";

    std::cout << "Plot calculation tests: ALL PASSED\n";
    return true;
}

bool TestProposalCalculations() {
    std::cout << "\n========== Testing Proposal Calculations ==========\n";

    // Test loyal character requesting title
    float title_acceptance = AICalculator::CalculateProposalAcceptance(
        ProposalDecision::REQUEST_TITLE, 0.8f, 0.5f);
    assert(title_acceptance > 0.6f);
    std::cout << "Title request acceptance (loyal): " << title_acceptance << " PASSED\n";

    // Test compassionate modifier
    float adjusted = AICalculator::AdjustAcceptanceByPersonality(0.5f, 0.8f);
    assert(adjusted > 0.5f);
    std::cout << "Compassion increases acceptance: " << adjusted << " PASSED\n";

    std::cout << "Proposal calculation tests: ALL PASSED\n";
    return true;
}

bool TestRelationshipCalculations() {
    std::cout << "\n========== Testing Relationship Calculations ==========\n";

    // Test opinion decay
    float decay_positive = AICalculator::CalculateOpinionDecay(50.0f);
    assert(decay_positive < 0.0f);
    std::cout << "Positive opinion decays: " << decay_positive << " PASSED\n";

    float decay_negative = AICalculator::CalculateOpinionDecay(-50.0f);
    assert(decay_negative > 0.0f);
    std::cout << "Negative opinion recovers: " << decay_negative << " PASSED\n";

    // Test relationship type determination
    std::string rival_type = AICalculator::DetermineRelationshipType(-60.0f, false, false);
    assert(rival_type == "rival");
    std::cout << "Low opinion = rival: PASSED\n";

    std::string friend_type = AICalculator::DetermineRelationshipType(75.0f, false, false);
    assert(friend_type == "friend");
    std::cout << "High opinion = friend: PASSED\n";

    // Test relationship desirability
    float desirability = AICalculator::CalculateRelationshipDesirability(
        75.0f, 0.7f, 0.6f, 0.8f);
    assert(desirability > 0.5f);
    std::cout << "Relationship desirability (high opinion): " << desirability << " PASSED\n";

    std::cout << "Relationship calculation tests: ALL PASSED\n";
    return true;
}

bool TestAmbitionCalculations() {
    std::cout << "\n========== Testing Ambition Calculations ==========\n";

    // Test wealth ambition for greedy character
    float wealth_score = AICalculator::ScoreAmbitionDesirability(
        CharacterAmbition::ACCUMULATE_WEALTH, 0.6f, 0.9f, 0.3f);
    assert(wealth_score > 0.7f);
    std::cout << "Wealth ambition (greedy): " << wealth_score << " PASSED\n";

    // Test power ambition for ambitious character
    float power_score = AICalculator::ScoreAmbitionDesirability(
        CharacterAmbition::POWER, 0.9f, 0.5f, 0.5f);
    assert(power_score > 0.8f);
    std::cout << "Power ambition (ambitious): " << power_score << " PASSED\n";

    // Test ambition progress
    float progress = AICalculator::CalculateAmbitionProgress(
        CharacterAmbition::GAIN_LAND, 5, 10);
    assert(std::abs(progress - 0.5f) < 0.01f);
    std::cout << "Ambition progress (50%): " << progress << " PASSED\n";

    std::cout << "Ambition calculation tests: ALL PASSED\n";
    return true;
}

bool TestMoodCalculations() {
    std::cout << "\n========== Testing Mood Calculations ==========\n";

    // Test happy mood
    CharacterMood happy = AICalculator::DetermineMood(0.2f, 10.0f, 0.8f);
    assert(happy == CharacterMood::HAPPY || happy == CharacterMood::ELATED);
    std::cout << "Happy mood determination: PASSED\n";

    // Test afraid mood
    CharacterMood afraid = AICalculator::DetermineMood(0.9f, -20.0f, 0.1f);
    assert(afraid == CharacterMood::AFRAID || afraid == CharacterMood::ANGRY);
    std::cout << "Afraid mood determination: PASSED\n";

    // Test mood modifiers
    float happy_modifier = AICalculator::CalculateMoodModifier(CharacterMood::HAPPY);
    assert(happy_modifier > 1.0f);
    std::cout << "Happy mood modifier: " << happy_modifier << " PASSED\n";

    float afraid_modifier = AICalculator::CalculateMoodModifier(CharacterMood::AFRAID);
    assert(afraid_modifier < 1.0f);
    std::cout << "Afraid mood modifier: " << afraid_modifier << " PASSED\n";

    std::cout << "Mood calculation tests: ALL PASSED\n";
    return true;
}

bool TestDecisionScoring() {
    std::cout << "\n========== Testing Decision Scoring ==========\n";

    // Test high-quality decision
    float high_score = AICalculator::CalculateDecisionScore(
        0.8f, 0.7f, 0.6f, 1.2f);
    assert(high_score > 0.7f);
    std::cout << "High-quality decision score: " << high_score << " PASSED\n";

    // Test low-quality decision
    float low_score = AICalculator::CalculateDecisionScore(
        0.3f, 0.2f, 0.1f, 0.8f);
    assert(low_score < 0.4f);
    std::cout << "Low-quality decision score: " << low_score << " PASSED\n";

    // Test normalization
    float normalized = AICalculator::NormalizeScore(1.5f);
    assert(normalized == 1.0f);
    std::cout << "Score normalization: PASSED\n";

    std::cout << "Decision scoring tests: ALL PASSED\n";
    return true;
}

int main() {
    std::cout << "\n";
    std::cout << "========================================================\n";
    std::cout << "      AI SYSTEM REFACTORING - TEST SUITE\n";
    std::cout << "========================================================\n";

    bool all_passed = true;

    all_passed &= TestPlotCalculations();
    all_passed &= TestProposalCalculations();
    all_passed &= TestRelationshipCalculations();
    all_passed &= TestAmbitionCalculations();
    all_passed &= TestMoodCalculations();
    all_passed &= TestDecisionScoring();

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
    std::cout << "  - AICalculator: Pure calculation functions for character AI\n";
    std::cout << "\n";
    std::cout << "Expected Benefits:\n";
    std::cout << "  - Reduced CharacterAI.cpp from ~1,267 lines\n";
    std::cout << "  - All AI decision calculations testable\n";
    std::cout << "  - Plot, proposal, relationship logic isolated\n";
    std::cout << "  - Easy to tune AI behavior through pure functions\n";
    std::cout << "\n";

    return all_passed ? 0 : 1;
}
