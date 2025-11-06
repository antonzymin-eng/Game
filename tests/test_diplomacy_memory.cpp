/**
 * @file test_diplomacy_memory.cpp
 * @brief Unit tests for long-term diplomatic memory (rolling opinion history)
 */

#include "game/diplomacy/DiplomacyComponents.h"
#include "game/config/GameConfig.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace game::diplomacy;

/**
 * Test basic opinion history tracking
 */
void test_opinion_history_basic() {
    std::cout << "Testing basic opinion history tracking...\n";
    
    DiplomaticState state;
    state.opinion = 0;
    
    // Add several opinion values
    state.UpdateOpinionHistory(10);
    state.UpdateOpinionHistory(20);
    state.UpdateOpinionHistory(30);
    
    // Average should be (10 + 20 + 30) / 3 = 20
    double expected_avg = 20.0;
    double actual_avg = state.GetHistoricalOpinionAverage();
    
    assert(std::abs(actual_avg - expected_avg) < 0.01);
    std::cout << "  Historical average after 3 values: " << actual_avg << " (expected: " << expected_avg << ")\n";
    
    std::cout << "✓ Basic opinion history test passed\n\n";
}

/**
 * Test rolling window - old values should be dropped
 */
void test_opinion_history_window() {
    std::cout << "Testing opinion history rolling window...\n";
    
    DiplomaticState state;
    
    // Fill history beyond window size (default: 12)
    for (int i = 1; i <= 20; i++) {
        state.UpdateOpinionHistory(i * 10);
    }
    
    // Only last 12 values should be kept: 90, 100, 110, ..., 200
    // Average = (90 + 100 + 110 + 120 + 130 + 140 + 150 + 160 + 170 + 180 + 190 + 200) / 12
    // Sum = 1740, Average = 145
    double expected_avg = 145.0;
    double actual_avg = state.GetHistoricalOpinionAverage();
    
    assert(std::abs(actual_avg - expected_avg) < 0.01);
    std::cout << "  Historical average after 20 values (window=12): " << actual_avg 
              << " (expected: " << expected_avg << ")\n";
    
    std::cout << "✓ Rolling window test passed\n\n";
}

/**
 * Test integration with ModifyOpinion
 */
void test_modify_opinion_integration() {
    std::cout << "Testing ModifyOpinion integration with history...\n";
    
    DiplomacyComponent diplomacy;
    game::types::EntityID other_realm = 42;
    
    // Make several opinion changes
    diplomacy.ModifyOpinion(other_realm, 10, "Gift sent");
    diplomacy.ModifyOpinion(other_realm, 15, "Trade agreement");
    diplomacy.ModifyOpinion(other_realm, -5, "Border incident");
    
    auto* state = diplomacy.GetRelationship(other_realm);
    assert(state != nullptr);
    
    // Current opinion should be 10 + 15 - 5 = 20
    assert(state->opinion == 20);
    
    // Historical average should reflect the progression: 10, 25, 20
    // Average = (10 + 25 + 20) / 3 = 18.33
    double avg = state->GetHistoricalOpinionAverage();
    double expected_avg = (10.0 + 25.0 + 20.0) / 3.0;
    
    assert(std::abs(avg - expected_avg) < 0.01);
    std::cout << "  Current opinion: " << state->opinion << "\n";
    std::cout << "  Historical average: " << avg << " (expected: " << expected_avg << ")\n";
    
    std::cout << "✓ ModifyOpinion integration test passed\n\n";
}

/**
 * Test memory shows trend over volatile opinions
 */
void test_volatile_opinions() {
    std::cout << "Testing historical average with volatile opinions...\n";
    
    DiplomaticState state;
    
    // Simulate volatile relationship: alternating positive/negative
    int opinions[] = {50, -30, 40, -20, 30, -10, 20, 0, 10, -5};
    
    for (int op : opinions) {
        state.UpdateOpinionHistory(op);
    }
    
    // Average should show overall slightly positive trend
    double avg = state.GetHistoricalOpinionAverage();
    
    // Sum = 50 - 30 + 40 - 20 + 30 - 10 + 20 + 0 + 10 - 5 = 85
    // Average = 85 / 10 = 8.5
    double expected_avg = 8.5;
    
    assert(std::abs(avg - expected_avg) < 0.01);
    std::cout << "  Average of volatile opinions: " << avg << " (expected: " << expected_avg << ")\n";
    std::cout << "  This shows long-term trend despite volatility\n";
    
    std::cout << "✓ Volatile opinions test passed\n\n";
}

/**
 * Test empty history
 */
void test_empty_history() {
    std::cout << "Testing empty opinion history...\n";
    
    DiplomaticState state;
    
    // Historical average should be 0.0 when no history exists
    double avg = state.GetHistoricalOpinionAverage();
    assert(avg == 0.0);
    std::cout << "  Empty history average: " << avg << " (expected: 0.0)\n";
    
    std::cout << "✓ Empty history test passed\n\n";
}

/**
 * Test that historical average differs from current opinion
 */
void test_current_vs_historical() {
    std::cout << "Testing current opinion vs historical average difference...\n";
    
    DiplomacyComponent diplomacy;
    game::types::EntityID other_realm = 99;
    
    // Start friendly
    diplomacy.ModifyOpinion(other_realm, 60, "Long friendship");
    diplomacy.ModifyOpinion(other_realm, 70, "Military alliance");
    diplomacy.ModifyOpinion(other_realm, 80, "Mutual defense");
    
    // Sudden betrayal
    diplomacy.ModifyOpinion(other_realm, -100, "BETRAYAL");
    
    auto* state = diplomacy.GetRelationship(other_realm);
    assert(state != nullptr);
    
    // Current opinion is very negative
    std::cout << "  Current opinion after betrayal: " << state->opinion << "\n";
    
    // But historical average shows the long friendship
    // History: 60, 130 (60+70), 210 (but clamped to 100), 10 (100-90 after betrayal causes clamp)
    // Actually: 60, 100 (clamped), 100 (clamped), 0 (100-100)
    double avg = state->GetHistoricalOpinionAverage();
    std::cout << "  Historical average (shows past friendship): " << avg << "\n";
    
    // Historical average should be much higher than current (AI can remember the friendship)
    assert(avg > state->opinion);
    std::cout << "  AI can use this to distinguish sudden betrayal from long-term enemy\n";
    
    std::cout << "✓ Current vs historical test passed\n\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "Diplomacy Long-Term Memory Test Suite\n";
    std::cout << "========================================\n\n";
    
    try {
        test_opinion_history_basic();
        test_opinion_history_window();
        test_modify_opinion_integration();
        test_volatile_opinions();
        test_empty_history();
        test_current_vs_historical();
        
        std::cout << "========================================\n";
        std::cout << "✓ All memory tests passed successfully!\n";
        std::cout << "========================================\n";
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "✗ Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
