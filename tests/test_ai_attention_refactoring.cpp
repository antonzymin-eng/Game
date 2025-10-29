// ============================================================================
// Mechanica Imperii - AI Attention System Refactoring Tests
// ============================================================================

#include "game/ai/calculators/AIAttentionCalculator.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>

using namespace AI;

bool TestAttentionScoreCalculations() {
    std::cout << "\n========== Testing Attention Score Calculations ==========\n";

    // Test component calculations
    float typeComponent = AIAttentionCalculator::CalculateTypeWeightComponent(1.0f);
    assert(std::abs(typeComponent - 0.4f) < 0.001f); // 40%
    std::cout << "Type weight component (40%): " << typeComponent << " PASSED\n";

    float severityComponent = AIAttentionCalculator::CalculateSeverityComponent(1.0f);
    assert(std::abs(severityComponent - 0.3f) < 0.001f); // 30%
    std::cout << "Severity component (30%): " << severityComponent << " PASSED\n";

    float accuracyComponent = AIAttentionCalculator::CalculateAccuracyComponent(1.0f);
    assert(std::abs(accuracyComponent - 0.2f) < 0.001f); // 20%
    std::cout << "Accuracy component (20%): " << accuracyComponent << " PASSED\n";

    float relevanceComponent = AIAttentionCalculator::CalculateRelevanceComponent(
        InformationRelevance::CRITICAL);
    assert(std::abs(relevanceComponent - 0.1f) < 0.001f); // 10% of 1.0
    std::cout << "Relevance component (10%): " << relevanceComponent << " PASSED\n";

    // Test full attention score calculation
    float score = AIAttentionCalculator::CalculateAttentionScore(
        1.0f,  // type weight
        1.0f,  // severity
        1.0f,  // accuracy
        InformationRelevance::CRITICAL,  // relevance
        1.0f   // global multiplier
    );
    assert(std::abs(score - 1.0f) < 0.001f); // Should sum to 100%
    std::cout << "Full attention score (1.0): " << score << " PASSED\n";

    // Test with partial values
    float partialScore = AIAttentionCalculator::CalculateAttentionScore(
        0.5f,  // type weight
        0.6f,  // severity
        0.7f,  // accuracy
        InformationRelevance::MEDIUM,  // relevance (0.4)
        1.0f   // global multiplier
    );
    float expected = (0.5f * 0.4f) + (0.6f * 0.3f) + (0.7f * 0.2f) + (0.4f * 0.1f);
    assert(std::abs(partialScore - expected) < 0.001f);
    std::cout << "Partial attention score: " << partialScore << " PASSED\n";

    // Test global multiplier
    float multiplied = AIAttentionCalculator::CalculateAttentionScore(
        1.0f, 1.0f, 1.0f, InformationRelevance::CRITICAL, 0.5f);
    assert(std::abs(multiplied - 0.5f) < 0.001f); // Should be halved
    std::cout << "Global multiplier (0.5x): " << multiplied << " PASSED\n";

    std::cout << "Attention score calculation tests: ALL PASSED\n";
    return true;
}

bool TestRelevanceConversion() {
    std::cout << "\n========== Testing Relevance Conversion ==========\n";

    // Test relevance to score mapping
    float critical = AIAttentionCalculator::RelevanceToScore(InformationRelevance::CRITICAL);
    assert(std::abs(critical - 1.0f) < 0.001f);
    std::cout << "CRITICAL relevance (1.0): PASSED\n";

    float high = AIAttentionCalculator::RelevanceToScore(InformationRelevance::HIGH);
    assert(std::abs(high - 0.7f) < 0.001f);
    std::cout << "HIGH relevance (0.7): PASSED\n";

    float medium = AIAttentionCalculator::RelevanceToScore(InformationRelevance::MEDIUM);
    assert(std::abs(medium - 0.4f) < 0.001f);
    std::cout << "MEDIUM relevance (0.4): PASSED\n";

    float low = AIAttentionCalculator::RelevanceToScore(InformationRelevance::LOW);
    assert(std::abs(low - 0.2f) < 0.001f);
    std::cout << "LOW relevance (0.2): PASSED\n";

    std::cout << "Relevance conversion tests: ALL PASSED\n";
    return true;
}

bool TestDistanceFiltering() {
    std::cout << "\n========== Testing Distance Filtering ==========\n";

    // Test distance estimation (200 units per hop)
    float distance = AIAttentionCalculator::CalculateEstimatedDistance(5);
    assert(std::abs(distance - 1000.0f) < 0.001f); // 5 hops * 200 = 1000
    std::cout << "Distance estimation (5 hops = 1000): PASSED\n";

    // Test distance filter pass
    bool passes = AIAttentionCalculator::PassesDistanceFilter(5, 2000.0f);
    assert(passes == true); // 1000 <= 2000
    std::cout << "Distance filter pass (1000 <= 2000): PASSED\n";

    // Test distance filter block
    bool blocks = AIAttentionCalculator::PassesDistanceFilter(15, 2000.0f);
    assert(blocks == false); // 3000 > 2000
    std::cout << "Distance filter block (3000 > 2000): PASSED\n";

    // Test boundary case
    bool boundary = AIAttentionCalculator::PassesDistanceFilter(10, 2000.0f);
    assert(boundary == true); // 2000 == 2000
    std::cout << "Distance filter boundary (2000 == 2000): PASSED\n";

    std::cout << "Distance filtering tests: ALL PASSED\n";
    return true;
}

bool TestTypeFiltering() {
    std::cout << "\n========== Testing Type Filtering ==========\n";

    // Test type filter pass
    bool passes = AIAttentionCalculator::PassesTypeFilter(0.5f, 0.1f);
    assert(passes == true); // 0.5 > 0.1
    std::cout << "Type filter pass (0.5 > 0.1): PASSED\n";

    // Test type filter block
    bool blocks = AIAttentionCalculator::PassesTypeFilter(0.05f, 0.1f);
    assert(blocks == false); // 0.05 <= 0.1
    std::cout << "Type filter block (0.05 <= 0.1): PASSED\n";

    // Test boundary case
    bool boundary = AIAttentionCalculator::PassesTypeFilter(0.1f, 0.1f);
    assert(boundary == false); // 0.1 == 0.1 (not greater)
    std::cout << "Type filter boundary (0.1 == 0.1): PASSED\n";

    std::cout << "Type filtering tests: ALL PASSED\n";
    return true;
}

bool TestSpecialInterestDetection() {
    std::cout << "\n========== Testing Special Interest Detection ==========\n";

    std::vector<uint32_t> rivals = {100, 200, 300};
    std::vector<uint32_t> allies = {400, 500};
    std::vector<uint32_t> watched = {10, 20, 30};

    // Test rival nation detection
    bool isRival = AIAttentionCalculator::IsSpecialInterest(
        200, 0, rivals, allies, watched);
    assert(isRival == true);
    std::cout << "Special interest (rival nation): PASSED\n";

    // Test allied nation detection
    bool isAlly = AIAttentionCalculator::IsSpecialInterest(
        400, 0, rivals, allies, watched);
    assert(isAlly == true);
    std::cout << "Special interest (allied nation): PASSED\n";

    // Test watched province detection
    bool isWatched = AIAttentionCalculator::IsSpecialInterest(
        0, 20, rivals, allies, watched);
    assert(isWatched == true);
    std::cout << "Special interest (watched province): PASSED\n";

    // Test non-special interest
    bool notSpecial = AIAttentionCalculator::IsSpecialInterest(
        999, 999, rivals, allies, watched);
    assert(notSpecial == false);
    std::cout << "Not special interest: PASSED\n";

    // Test IsInList helper
    bool inList = AIAttentionCalculator::IsInList(200, rivals);
    assert(inList == true);
    std::cout << "IsInList (found): PASSED\n";

    bool notInList = AIAttentionCalculator::IsInList(999, rivals);
    assert(notInList == false);
    std::cout << "IsInList (not found): PASSED\n";

    std::cout << "Special interest detection tests: ALL PASSED\n";
    return true;
}

bool TestRelevanceAdjustment() {
    std::cout << "\n========== Testing Relevance Adjustment ==========\n";

    // Test upgrade to CRITICAL
    InformationRelevance critical = AIAttentionCalculator::AdjustRelevanceByScore(
        InformationRelevance::LOW, 0.85f, 0.8f, 0.6f, 0.3f, 0.1f);
    assert(critical == InformationRelevance::CRITICAL);
    std::cout << "Upgrade to CRITICAL (score >= 0.8): PASSED\n";

    // Test upgrade to HIGH
    InformationRelevance high = AIAttentionCalculator::AdjustRelevanceByScore(
        InformationRelevance::LOW, 0.65f, 0.8f, 0.6f, 0.3f, 0.1f);
    assert(high == InformationRelevance::HIGH);
    std::cout << "Upgrade to HIGH (score >= 0.6): PASSED\n";

    // Test upgrade to MEDIUM
    InformationRelevance medium = AIAttentionCalculator::AdjustRelevanceByScore(
        InformationRelevance::LOW, 0.35f, 0.8f, 0.6f, 0.3f, 0.1f);
    assert(medium == InformationRelevance::MEDIUM);
    std::cout << "Upgrade to MEDIUM (score >= 0.3): PASSED\n";

    // Test maintain HIGH when already high
    InformationRelevance maintainHigh = AIAttentionCalculator::AdjustRelevanceByScore(
        InformationRelevance::HIGH, 0.65f, 0.8f, 0.6f, 0.3f, 0.1f);
    assert(maintainHigh == InformationRelevance::HIGH);
    std::cout << "Maintain HIGH relevance: PASSED\n";

    // Test no upgrade below threshold
    InformationRelevance below = AIAttentionCalculator::AdjustRelevanceByScore(
        InformationRelevance::LOW, 0.05f, 0.8f, 0.6f, 0.3f, 0.1f);
    assert(below == InformationRelevance::LOW);
    std::cout << "No upgrade (below threshold): PASSED\n";

    std::cout << "Relevance adjustment tests: ALL PASSED\n";
    return true;
}

bool TestProcessingDelayCalculations() {
    std::cout << "\n========== Testing Processing Delay Calculations ==========\n";

    // Test critical delay (immediate)
    float criticalDelay = AIAttentionCalculator::CalculateProcessingDelay(
        0.85f, 0.8f, 0.6f, 0.3f);
    assert(std::abs(criticalDelay - 0.0f) < 0.001f);
    std::cout << "Critical delay (0 days): PASSED\n";

    // Test high delay (1 day)
    float highDelay = AIAttentionCalculator::CalculateProcessingDelay(
        0.65f, 0.8f, 0.6f, 0.3f);
    assert(std::abs(highDelay - 1.0f) < 0.001f);
    std::cout << "High delay (1 day): PASSED\n";

    // Test medium delay (3 days)
    float mediumDelay = AIAttentionCalculator::CalculateProcessingDelay(
        0.35f, 0.8f, 0.6f, 0.3f);
    assert(std::abs(mediumDelay - 3.0f) < 0.001f);
    std::cout << "Medium delay (3 days): PASSED\n";

    // Test low delay (7 days)
    float lowDelay = AIAttentionCalculator::CalculateProcessingDelay(
        0.15f, 0.8f, 0.6f, 0.3f);
    assert(std::abs(lowDelay - 7.0f) < 0.001f);
    std::cout << "Low delay (7 days): PASSED\n";

    std::cout << "Processing delay calculation tests: ALL PASSED\n";
    return true;
}

bool TestPersonalityArchetypeMapping() {
    std::cout << "\n========== Testing Personality/Archetype Mapping ==========\n";

    // Test archetype to personality
    NationPersonality expansionist = AIAttentionCalculator::ArchetypeToPersonality(
        CharacterArchetype::THE_CONQUEROR);
    assert(expansionist == NationPersonality::EXPANSIONIST);
    std::cout << "Archetype to Personality (Conqueror -> Expansionist): PASSED\n";

    NationPersonality diplomatic = AIAttentionCalculator::ArchetypeToPersonality(
        CharacterArchetype::THE_DIPLOMAT);
    assert(diplomatic == NationPersonality::DIPLOMATIC);
    std::cout << "Archetype to Personality (Diplomat -> Diplomatic): PASSED\n";

    // Test personality to archetype
    CharacterArchetype conqueror = AIAttentionCalculator::PersonalityToArchetype(
        NationPersonality::EXPANSIONIST);
    assert(conqueror == CharacterArchetype::THE_CONQUEROR);
    std::cout << "Personality to Archetype (Expansionist -> Conqueror): PASSED\n";

    CharacterArchetype diplomat = AIAttentionCalculator::PersonalityToArchetype(
        NationPersonality::DIPLOMATIC);
    assert(diplomat == CharacterArchetype::THE_DIPLOMAT);
    std::cout << "Personality to Archetype (Diplomatic -> Diplomat): PASSED\n";

    // Test round-trip conversion
    NationPersonality original = NationPersonality::ECONOMIC;
    CharacterArchetype arch = AIAttentionCalculator::PersonalityToArchetype(original);
    NationPersonality roundTrip = AIAttentionCalculator::ArchetypeToPersonality(arch);
    assert(roundTrip == original);
    std::cout << "Round-trip conversion (Economic): PASSED\n";

    std::cout << "Personality/Archetype mapping tests: ALL PASSED\n";
    return true;
}

bool TestAttentionTierClassification() {
    std::cout << "\n========== Testing Attention Tier Classification ==========\n";

    // Test CRITICAL tier
    auto critical = AIAttentionCalculator::ClassifyAttentionTier(
        0.85f, 0.8f, 0.6f, 0.3f, 0.1f);
    assert(critical == AIAttentionCalculator::AttentionTier::CRITICAL);
    std::cout << "CRITICAL tier (0.85 >= 0.8): PASSED\n";

    // Test HIGH tier
    auto high = AIAttentionCalculator::ClassifyAttentionTier(
        0.65f, 0.8f, 0.6f, 0.3f, 0.1f);
    assert(high == AIAttentionCalculator::AttentionTier::HIGH);
    std::cout << "HIGH tier (0.65 >= 0.6): PASSED\n";

    // Test MEDIUM tier
    auto medium = AIAttentionCalculator::ClassifyAttentionTier(
        0.35f, 0.8f, 0.6f, 0.3f, 0.1f);
    assert(medium == AIAttentionCalculator::AttentionTier::MEDIUM);
    std::cout << "MEDIUM tier (0.35 >= 0.3): PASSED\n";

    // Test LOW tier
    auto low = AIAttentionCalculator::ClassifyAttentionTier(
        0.15f, 0.8f, 0.6f, 0.3f, 0.1f);
    assert(low == AIAttentionCalculator::AttentionTier::LOW);
    std::cout << "LOW tier (0.15 >= 0.1): PASSED\n";

    // Test BELOW_THRESHOLD
    auto below = AIAttentionCalculator::ClassifyAttentionTier(
        0.05f, 0.8f, 0.6f, 0.3f, 0.1f);
    assert(below == AIAttentionCalculator::AttentionTier::BELOW_THRESHOLD);
    std::cout << "BELOW_THRESHOLD tier (0.05 < 0.1): PASSED\n";

    std::cout << "Attention tier classification tests: ALL PASSED\n";
    return true;
}

bool TestUtilityFunctions() {
    std::cout << "\n========== Testing Utility Functions ==========\n";

    // Test Clamp01
    float clamped_low = AIAttentionCalculator::Clamp01(-0.5f);
    assert(std::abs(clamped_low - 0.0f) < 0.001f);
    std::cout << "Clamp01 (-0.5 -> 0.0): PASSED\n";

    float clamped_high = AIAttentionCalculator::Clamp01(1.5f);
    assert(std::abs(clamped_high - 1.0f) < 0.001f);
    std::cout << "Clamp01 (1.5 -> 1.0): PASSED\n";

    float clamped_ok = AIAttentionCalculator::Clamp01(0.5f);
    assert(std::abs(clamped_ok - 0.5f) < 0.001f);
    std::cout << "Clamp01 (0.5 -> 0.5): PASSED\n";

    // Test Clamp with range
    float clamped_range = AIAttentionCalculator::Clamp(15.0f, 10.0f, 20.0f);
    assert(std::abs(clamped_range - 15.0f) < 0.001f);
    std::cout << "Clamp (15.0 in [10, 20]): PASSED\n";

    float clamped_min = AIAttentionCalculator::Clamp(5.0f, 10.0f, 20.0f);
    assert(std::abs(clamped_min - 10.0f) < 0.001f);
    std::cout << "Clamp (5.0 -> 10.0): PASSED\n";

    std::cout << "Utility function tests: ALL PASSED\n";
    return true;
}

int main() {
    std::cout << "\n";
    std::cout << "========================================================\n";
    std::cout << "  AI ATTENTION SYSTEM REFACTORING - TEST SUITE\n";
    std::cout << "========================================================\n";

    bool all_passed = true;

    all_passed &= TestAttentionScoreCalculations();
    all_passed &= TestRelevanceConversion();
    all_passed &= TestDistanceFiltering();
    all_passed &= TestTypeFiltering();
    all_passed &= TestSpecialInterestDetection();
    all_passed &= TestRelevanceAdjustment();
    all_passed &= TestProcessingDelayCalculations();
    all_passed &= TestPersonalityArchetypeMapping();
    all_passed &= TestAttentionTierClassification();
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
    std::cout << "  - AIAttentionCalculator: Pure calculation functions for attention system\n";
    std::cout << "\n";
    std::cout << "Expected Benefits:\n";
    std::cout << "  - Reduced AIAttentionManager.cpp from ~874 lines\n";
    std::cout << "  - Attention scoring algorithm testable in isolation\n";
    std::cout << "  - Filtering logic (distance, type, special interest) centralized\n";
    std::cout << "  - Relevance adjustment and processing delay calculations reusable\n";
    std::cout << "  - Personality/Archetype mapping bidirectional and testable\n";
    std::cout << "\n";

    return all_passed ? 0 : 1;
}
