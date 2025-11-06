/**
 * @file test_diplomacy_decay.cpp
 * @brief Unit tests for diplomatic opinion and trust decay mechanics
 */

#include "game/diplomacy/DiplomacyComponents.h"
#include "game/config/GameConfig.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace game::diplomacy;

/**
 * Test basic opinion decay toward neutral baseline
 */
void test_opinion_decay_basic() {
    std::cout << "Testing basic opinion decay...\n";
    
    DiplomaticState state;
    state.opinion = 50;  // Positive opinion
    
    // Apply decay for 10 time units (default rate: 0.1 * 10 = 1 point per unit)
    float time_delta = 10.0f;
    state.ApplyOpinionDecay(time_delta, 0);
    
    // Opinion should decay toward 0
    assert(state.opinion < 50);
    assert(state.opinion >= 0);
    std::cout << "  Positive opinion decayed from 50 to " << state.opinion << "\n";
    
    // Test negative opinion recovery
    state.opinion = -50;
    state.ApplyOpinionDecay(time_delta, 0);
    
    // Negative opinion should recover toward 0
    assert(state.opinion > -50);
    assert(state.opinion <= 0);
    std::cout << "  Negative opinion recovered from -50 to " << state.opinion << "\n";
    
    std::cout << "✓ Basic opinion decay test passed\n\n";
}

/**
 * Test opinion decay with custom neutral baseline
 */
void test_opinion_decay_custom_baseline() {
    std::cout << "Testing opinion decay with custom baseline...\n";
    
    DiplomaticState state;
    state.opinion = 80;
    
    // Use custom baseline of 20 (slightly positive)
    float time_delta = 10.0f;
    int custom_baseline = 20;
    state.ApplyOpinionDecay(time_delta, custom_baseline);
    
    // Opinion should decay toward baseline (20)
    assert(state.opinion < 80);
    assert(state.opinion >= custom_baseline);
    std::cout << "  Opinion decayed from 80 toward baseline " << custom_baseline << ": " << state.opinion << "\n";
    
    std::cout << "✓ Custom baseline opinion decay test passed\n\n";
}

/**
 * Test trust decay toward neutral (0.5)
 */
void test_trust_decay_basic() {
    std::cout << "Testing basic trust decay...\n";
    
    DiplomaticState state;
    state.trust = 0.9;  // High trust
    
    // Apply decay (default rate: 0.01 * 10 = 0.1 per unit)
    float time_delta = 10.0f;
    double initial_trust = state.trust;
    state.ApplyTrustDecay(time_delta, 0.5);
    
    // Trust should decay toward 0.5
    assert(state.trust < initial_trust);
    assert(state.trust >= 0.5);
    assert(state.trust >= 0.0 && state.trust <= 1.0);  // Trust clamped to [0, 1]
    std::cout << "  High trust decayed from " << initial_trust << " to " << state.trust << "\n";
    
    // Test low trust recovery
    state.trust = 0.1;
    initial_trust = state.trust;
    state.ApplyTrustDecay(time_delta, 0.5);
    
    // Trust should recover toward 0.5
    assert(state.trust > initial_trust);
    assert(state.trust <= 0.5);
    assert(state.trust >= 0.0 && state.trust <= 1.0);
    std::cout << "  Low trust recovered from " << initial_trust << " to " << state.trust << "\n";
    
    std::cout << "✓ Basic trust decay test passed\n\n";
}

/**
 * Test trust clamping to valid range
 */
void test_trust_clamping() {
    std::cout << "Testing trust clamping to [0.0, 1.0]...\n";
    
    DiplomaticState state;
    
    // Set trust to exactly neutral
    state.trust = 0.5;
    state.ApplyTrustDecay(10.0f, 0.5);
    
    // Trust at baseline should not change
    assert(std::abs(state.trust - 0.5) < 0.001);
    std::cout << "  Trust at baseline remained: " << state.trust << "\n";
    
    // Verify clamping happens in implementation
    state.trust = 1.0;  // Maximum trust
    state.ApplyTrustDecay(100.0f, 0.5);  // Large decay
    assert(state.trust >= 0.0 && state.trust <= 1.0);
    std::cout << "  High trust with large decay clamped correctly: " << state.trust << "\n";
    
    state.trust = 0.0;  // Minimum trust
    state.ApplyTrustDecay(100.0f, 0.5);  // Large recovery
    assert(state.trust >= 0.0 && state.trust <= 1.0);
    std::cout << "  Low trust with large recovery clamped correctly: " << state.trust << "\n";
    
    std::cout << "✓ Trust clamping test passed\n\n";
}

/**
 * Test decay does not overshoot neutral baseline
 */
void test_decay_no_overshoot() {
    std::cout << "Testing decay does not overshoot baseline...\n";
    
    DiplomaticState state;
    
    // Opinion close to baseline
    state.opinion = 5;
    float large_time_delta = 100.0f;  // Very large time delta
    state.ApplyOpinionDecay(large_time_delta, 0);
    
    // Should not overshoot to negative
    assert(state.opinion >= 0);
    std::cout << "  Opinion close to baseline did not overshoot: " << state.opinion << "\n";
    
    // Negative opinion close to baseline
    state.opinion = -3;
    state.ApplyOpinionDecay(large_time_delta, 0);
    
    // Should not overshoot to positive
    assert(state.opinion <= 0);
    std::cout << "  Negative opinion close to baseline did not overshoot: " << state.opinion << "\n";
    
    // Trust close to baseline
    state.trust = 0.52;
    state.ApplyTrustDecay(large_time_delta, 0.5);
    
    // Should not drop below baseline
    assert(state.trust >= 0.5);
    std::cout << "  Trust close to baseline did not overshoot: " << state.trust << "\n";
    
    std::cout << "✓ No overshoot test passed\n\n";
}

/**
 * Test decay with different time deltas
 */
void test_decay_time_scaling() {
    std::cout << "Testing decay scales with time delta...\n";
    
    DiplomaticState state1, state2;
    state1.opinion = 100;
    state2.opinion = 100;
    
    // Small time delta
    state1.ApplyOpinionDecay(1.0f, 0);
    int decay_small = 100 - state1.opinion;
    
    // Large time delta (10x)
    state2.ApplyOpinionDecay(10.0f, 0);
    int decay_large = 100 - state2.opinion;
    
    // Larger time delta should cause proportionally more decay
    assert(decay_large > decay_small);
    std::cout << "  Small delta (1.0) decay: " << decay_small << " points\n";
    std::cout << "  Large delta (10.0) decay: " << decay_large << " points\n";
    
    // Test trust scaling similarly
    state1.trust = 1.0;
    state2.trust = 1.0;
    
    state1.ApplyTrustDecay(1.0f, 0.5);
    double trust_decay_small = 1.0 - state1.trust;
    
    state2.ApplyTrustDecay(10.0f, 0.5);
    double trust_decay_large = 1.0 - state2.trust;
    
    assert(trust_decay_large > trust_decay_small);
    std::cout << "  Small delta (1.0) trust decay: " << trust_decay_small << "\n";
    std::cout << "  Large delta (10.0) trust decay: " << trust_decay_large << "\n";
    
    std::cout << "✓ Time scaling test passed\n\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "Diplomacy Decay Test Suite\n";
    std::cout << "========================================\n\n";
    
    try {
        test_opinion_decay_basic();
        test_opinion_decay_custom_baseline();
        test_trust_decay_basic();
        test_trust_clamping();
        test_decay_no_overshoot();
        test_decay_time_scaling();
        
        std::cout << "========================================\n";
        std::cout << "✓ All decay tests passed successfully!\n";
        std::cout << "========================================\n";
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "✗ Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
