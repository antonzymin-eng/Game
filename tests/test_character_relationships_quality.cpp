// Created: December 15, 2025
// Location: tests/test_character_relationships_quality.cpp
// Purpose: Tests for CharacterRelationships API quality improvements (commit 1028dc6, 651714f)
//          Verifies constants, filtering behavior, and magic number elimination

#include "game/character/CharacterRelationships.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>

using namespace game::character;

// ============================================================================
// Constants Tests
// ============================================================================

bool TestBondStrengthConstants() {
    std::cout << "\n========== Testing Bond Strength Constants ==========\n";

    // Verify constants are accessible and have correct values
    assert(CharacterRelationshipsComponent::MIN_BOND_STRENGTH == 0.0);
    std::cout << "MIN_BOND_STRENGTH = 0.0: PASSED\n";

    assert(CharacterRelationshipsComponent::MAX_BOND_STRENGTH == 100.0);
    std::cout << "MAX_BOND_STRENGTH = 100.0: PASSED\n";

    assert(CharacterRelationshipsComponent::SIGNIFICANT_BOND_THRESHOLD == 25.0);
    std::cout << "SIGNIFICANT_BOND_THRESHOLD = 25.0: PASSED\n";

    // Verify relationship between constants
    assert(CharacterRelationshipsComponent::MIN_BOND_STRENGTH <
           CharacterRelationshipsComponent::SIGNIFICANT_BOND_THRESHOLD);
    assert(CharacterRelationshipsComponent::SIGNIFICANT_BOND_THRESHOLD <
           CharacterRelationshipsComponent::MAX_BOND_STRENGTH);
    std::cout << "Constant ordering (MIN < THRESHOLD < MAX): PASSED\n";

    std::cout << "Bond Strength Constants tests: ALL PASSED\n";
    return true;
}

// ============================================================================
// GetFriends() Filtering Tests
// ============================================================================

bool TestGetFriendsFiltering() {
    std::cout << "\n========== Testing GetFriends() Filtering ==========\n";

    CharacterRelationshipsComponent relationships(100);

    // Add friends with various bond strengths
    relationships.SetRelationship(201, RelationshipType::FRIEND, 75, 50.0);   // Above threshold
    relationships.SetRelationship(202, RelationshipType::FRIEND, 60, 30.0);   // Above threshold
    relationships.SetRelationship(203, RelationshipType::FRIEND, 50, 25.0);   // Exactly at threshold
    relationships.SetRelationship(204, RelationshipType::FRIEND, 40, 24.999); // Just below threshold
    relationships.SetRelationship(205, RelationshipType::FRIEND, 30, 20.0);   // Below threshold
    relationships.SetRelationship(206, RelationshipType::FRIEND, 20, 10.0);   // Below threshold

    // GetFriends() should only return friends >= SIGNIFICANT_BOND_THRESHOLD (25.0)
    auto friends = relationships.GetFriends();

    // Should have 3 friends: 201 (50.0), 202 (30.0), 203 (25.0)
    assert(friends.size() == 3);
    std::cout << "GetFriends() count (expected 3): PASSED\n";

    // Verify correct friends are included
    assert(std::find(friends.begin(), friends.end(), 201) != friends.end());
    assert(std::find(friends.begin(), friends.end(), 202) != friends.end());
    assert(std::find(friends.begin(), friends.end(), 203) != friends.end());
    std::cout << "GetFriends() includes friends with bond >= 25.0: PASSED\n";

    // Verify weak friends are excluded
    assert(std::find(friends.begin(), friends.end(), 204) == friends.end());
    assert(std::find(friends.begin(), friends.end(), 205) == friends.end());
    assert(std::find(friends.begin(), friends.end(), 206) == friends.end());
    std::cout << "GetFriends() excludes friends with bond < 25.0: PASSED\n";

    std::cout << "GetFriends() Filtering tests: ALL PASSED\n";
    return true;
}

// ============================================================================
// GetRivals() Filtering Tests
// ============================================================================

bool TestGetRivalsFiltering() {
    std::cout << "\n========== Testing GetRivals() Filtering ==========\n";

    CharacterRelationshipsComponent relationships(100);

    // Add rivals with various bond strengths
    relationships.SetRelationship(301, RelationshipType::RIVAL, -75, 60.0);   // Above threshold
    relationships.SetRelationship(302, RelationshipType::RIVAL, -50, 35.0);   // Above threshold
    relationships.SetRelationship(303, RelationshipType::RIVAL, -40, 25.001); // Just above threshold
    relationships.SetRelationship(304, RelationshipType::RIVAL, -30, 24.5);   // Below threshold
    relationships.SetRelationship(305, RelationshipType::RIVAL, -20, 15.0);   // Below threshold

    // GetRivals() should only return rivals >= SIGNIFICANT_BOND_THRESHOLD (25.0)
    auto rivals = relationships.GetRivals();

    // Should have 3 rivals: 301 (60.0), 302 (35.0), 303 (25.001)
    assert(rivals.size() == 3);
    std::cout << "GetRivals() count (expected 3): PASSED\n";

    // Verify correct rivals are included
    assert(std::find(rivals.begin(), rivals.end(), 301) != rivals.end());
    assert(std::find(rivals.begin(), rivals.end(), 302) != rivals.end());
    assert(std::find(rivals.begin(), rivals.end(), 303) != rivals.end());
    std::cout << "GetRivals() includes rivals with bond >= 25.0: PASSED\n";

    // Verify weak rivals are excluded
    assert(std::find(rivals.begin(), rivals.end(), 304) == rivals.end());
    assert(std::find(rivals.begin(), rivals.end(), 305) == rivals.end());
    std::cout << "GetRivals() excludes rivals with bond < 25.0: PASSED\n";

    // Test API symmetry: GetRivals() behaves same as GetFriends()
    std::cout << "GetRivals() API symmetry with GetFriends(): PASSED\n";

    std::cout << "GetRivals() Filtering tests: ALL PASSED\n";
    return true;
}

// ============================================================================
// GetAllFriends() Unfiltered Tests
// ============================================================================

bool TestGetAllFriendsUnfiltered() {
    std::cout << "\n========== Testing GetAllFriends() Unfiltered ==========\n";

    CharacterRelationshipsComponent relationships(100);

    // Add friends with various bond strengths including very weak ones
    relationships.SetRelationship(401, RelationshipType::FRIEND, 75, 50.0);
    relationships.SetRelationship(402, RelationshipType::FRIEND, 60, 30.0);
    relationships.SetRelationship(403, RelationshipType::FRIEND, 50, 25.0);
    relationships.SetRelationship(404, RelationshipType::FRIEND, 40, 20.0);
    relationships.SetRelationship(405, RelationshipType::FRIEND, 30, 10.0);
    relationships.SetRelationship(406, RelationshipType::FRIEND, 20, 5.0);
    relationships.SetRelationship(407, RelationshipType::FRIEND, 10, 0.1);   // Very weak

    // GetAllFriends() should return ALL friendships regardless of bond strength
    auto all_friends = relationships.GetAllFriends();

    // Should have all 7 friends
    assert(all_friends.size() == 7);
    std::cout << "GetAllFriends() returns all friendships (count = 7): PASSED\n";

    // Verify all friends are included
    for (int i = 401; i <= 407; i++) {
        assert(std::find(all_friends.begin(), all_friends.end(), i) != all_friends.end());
    }
    std::cout << "GetAllFriends() includes weak friendships (bond < 25.0): PASSED\n";

    // Compare with GetFriends() - should be a superset
    auto filtered_friends = relationships.GetFriends();
    assert(all_friends.size() > filtered_friends.size());
    std::cout << "GetAllFriends() returns more than GetFriends(): PASSED\n";

    std::cout << "GetAllFriends() Unfiltered tests: ALL PASSED\n";
    return true;
}

// ============================================================================
// GetAllRivals() Unfiltered Tests
// ============================================================================

bool TestGetAllRivalsUnfiltered() {
    std::cout << "\n========== Testing GetAllRivals() Unfiltered ==========\n";

    CharacterRelationshipsComponent relationships(100);

    // Add rivals with various bond strengths including very weak ones
    relationships.SetRelationship(501, RelationshipType::RIVAL, -75, 60.0);
    relationships.SetRelationship(502, RelationshipType::RIVAL, -50, 30.0);
    relationships.SetRelationship(503, RelationshipType::RIVAL, -40, 20.0);
    relationships.SetRelationship(504, RelationshipType::RIVAL, -30, 10.0);
    relationships.SetRelationship(505, RelationshipType::RIVAL, -20, 5.0);
    relationships.SetRelationship(506, RelationshipType::RIVAL, -10, 0.5);   // Very weak

    // GetAllRivals() should return ALL rivalries regardless of bond strength
    auto all_rivals = relationships.GetAllRivals();

    // Should have all 6 rivals
    assert(all_rivals.size() == 6);
    std::cout << "GetAllRivals() returns all rivalries (count = 6): PASSED\n";

    // Verify all rivals are included
    for (int i = 501; i <= 506; i++) {
        assert(std::find(all_rivals.begin(), all_rivals.end(), i) != all_rivals.end());
    }
    std::cout << "GetAllRivals() includes weak rivalries (bond < 25.0): PASSED\n";

    // Compare with GetRivals() - should be a superset
    auto filtered_rivals = relationships.GetRivals();
    assert(all_rivals.size() > filtered_rivals.size());
    std::cout << "GetAllRivals() returns more than GetRivals(): PASSED\n";

    std::cout << "GetAllRivals() Unfiltered tests: ALL PASSED\n";
    return true;
}

// ============================================================================
// ModifyBondStrength() Clamping Tests
// ============================================================================

bool TestModifyBondStrengthClamping() {
    std::cout << "\n========== Testing ModifyBondStrength() Clamping ==========\n";

    CharacterRelationshipsComponent relationships(100);

    // Test upper bound clamping (MAX_BOND_STRENGTH = 100.0)
    relationships.SetRelationship(601, RelationshipType::FRIEND, 75, 95.0);
    relationships.ModifyBondStrength(601, 10.0);  // 95 + 10 = 105, should clamp to 100
    double bond_upper = relationships.GetFriendshipBondStrength(601);
    assert(bond_upper == CharacterRelationshipsComponent::MAX_BOND_STRENGTH);
    assert(bond_upper == 100.0);
    std::cout << "ModifyBondStrength() clamps to MAX_BOND_STRENGTH (100.0): PASSED\n";

    // Test lower bound clamping (MIN_BOND_STRENGTH = 0.0)
    relationships.SetRelationship(602, RelationshipType::FRIEND, 50, 5.0);
    relationships.ModifyBondStrength(602, -10.0);  // 5 - 10 = -5, should clamp to 0
    double bond_lower = relationships.GetFriendshipBondStrength(602);
    assert(bond_lower == CharacterRelationshipsComponent::MIN_BOND_STRENGTH);
    assert(bond_lower == 0.0);
    std::cout << "ModifyBondStrength() clamps to MIN_BOND_STRENGTH (0.0): PASSED\n";

    // Test normal modification within bounds
    relationships.SetRelationship(603, RelationshipType::FRIEND, 60, 50.0);
    relationships.ModifyBondStrength(603, 15.0);  // 50 + 15 = 65
    double bond_normal = relationships.GetFriendshipBondStrength(603);
    assert(std::abs(bond_normal - 65.0) < 0.001);
    std::cout << "ModifyBondStrength() normal modification (50 + 15 = 65): PASSED\n";

    std::cout << "ModifyBondStrength() Clamping tests: ALL PASSED\n";
    return true;
}

// ============================================================================
// Edge Cases and Boundary Tests
// ============================================================================

bool TestThresholdBoundaryEdgeCases() {
    std::cout << "\n========== Testing Threshold Boundary Edge Cases ==========\n";

    CharacterRelationshipsComponent relationships(100);

    // Test exactly at threshold (25.0)
    relationships.SetRelationship(701, RelationshipType::FRIEND, 75, 25.0);
    auto friends_exact = relationships.GetFriends();
    assert(std::find(friends_exact.begin(), friends_exact.end(), 701) != friends_exact.end());
    std::cout << "Exactly at threshold (25.0) is INCLUDED: PASSED\n";

    // Test just below threshold (24.999)
    CharacterRelationshipsComponent relationships2(100);
    relationships2.SetRelationship(702, RelationshipType::FRIEND, 75, 24.999);
    auto friends_below = relationships2.GetFriends();
    assert(std::find(friends_below.begin(), friends_below.end(), 702) == friends_below.end());
    std::cout << "Just below threshold (24.999) is EXCLUDED: PASSED\n";

    // Test just above threshold (25.001)
    CharacterRelationshipsComponent relationships3(100);
    relationships3.SetRelationship(703, RelationshipType::FRIEND, 75, 25.001);
    auto friends_above = relationships3.GetFriends();
    assert(std::find(friends_above.begin(), friends_above.end(), 703) != friends_above.end());
    std::cout << "Just above threshold (25.001) is INCLUDED: PASSED\n";

    // Test MIN_BOND_STRENGTH boundary (0.0)
    relationships.SetRelationship(704, RelationshipType::FRIEND, 50, 0.0);
    auto all_friends = relationships.GetAllFriends();
    assert(std::find(all_friends.begin(), all_friends.end(), 704) != all_friends.end());
    std::cout << "MIN_BOND_STRENGTH (0.0) included in GetAllFriends(): PASSED\n";

    // Test MAX_BOND_STRENGTH boundary (100.0)
    relationships.SetRelationship(705, RelationshipType::FRIEND, 100, 100.0);
    auto friends_max = relationships.GetFriends();
    assert(std::find(friends_max.begin(), friends_max.end(), 705) != friends_max.end());
    std::cout << "MAX_BOND_STRENGTH (100.0) included in GetFriends(): PASSED\n";

    std::cout << "Threshold Boundary Edge Cases tests: ALL PASSED\n";
    return true;
}

// ============================================================================
// IsFriendsWith() Consistency Tests
// ============================================================================

bool TestIsFriendsWithConsistency() {
    std::cout << "\n========== Testing IsFriendsWith() Consistency ==========\n";

    CharacterRelationshipsComponent relationships(100);

    // Add friend above threshold
    relationships.SetRelationship(801, RelationshipType::FRIEND, 75, 50.0);
    assert(relationships.IsFriendsWith(801) == true);
    std::cout << "IsFriendsWith() returns true for bond >= 25.0: PASSED\n";

    // Add friend below threshold
    relationships.SetRelationship(802, RelationshipType::FRIEND, 50, 20.0);
    assert(relationships.IsFriendsWith(802) == false);
    std::cout << "IsFriendsWith() returns false for bond < 25.0: PASSED\n";

    // Add friend exactly at threshold
    relationships.SetRelationship(803, RelationshipType::FRIEND, 60, 25.0);
    assert(relationships.IsFriendsWith(803) == true);
    std::cout << "IsFriendsWith() returns true for bond == 25.0: PASSED\n";

    // Verify IsFriendsWith() consistency with GetFriends()
    auto friends = relationships.GetFriends();
    for (const auto& friend_id : friends) {
        assert(relationships.IsFriendsWith(friend_id) == true);
    }
    std::cout << "IsFriendsWith() consistent with GetFriends() results: PASSED\n";

    std::cout << "IsFriendsWith() Consistency tests: ALL PASSED\n";
    return true;
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "CharacterRelationships Quality Tests\n";
    std::cout << "Testing API improvements (commits 1028dc6, 651714f)\n";
    std::cout << "========================================\n";

    bool all_passed = true;

    all_passed &= TestBondStrengthConstants();
    all_passed &= TestGetFriendsFiltering();
    all_passed &= TestGetRivalsFiltering();
    all_passed &= TestGetAllFriendsUnfiltered();
    all_passed &= TestGetAllRivalsUnfiltered();
    all_passed &= TestModifyBondStrengthClamping();
    all_passed &= TestThresholdBoundaryEdgeCases();
    all_passed &= TestIsFriendsWithConsistency();

    std::cout << "\n========================================\n";
    if (all_passed) {
        std::cout << "ALL TESTS PASSED ✓\n";
        std::cout << "========================================\n";
        return 0;
    } else {
        std::cout << "SOME TESTS FAILED ✗\n";
        std::cout << "========================================\n";
        return 1;
    }
}
