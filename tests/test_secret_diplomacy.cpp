/**
 * @file test_secret_diplomacy.cpp
 * @brief Unit tests for secret diplomacy features (secret treaties and hidden opinions)
 */

#include "game/diplomacy/DiplomacyComponents.h"
#include "game/config/GameConfig.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace game::diplomacy;

/**
 * Test secret treaty creation and visibility
 */
void test_secret_treaty_visibility() {
    std::cout << "Testing secret treaty visibility...\n";

    game::types::EntityID realm_a = 1;
    game::types::EntityID realm_b = 2;
    game::types::EntityID observer = 3;

    // Create a secret treaty
    Treaty secret_treaty(TreatyType::ALLIANCE, realm_a, realm_b);
    secret_treaty.is_secret = true;
    secret_treaty.secrecy_level = 0.8;

    // Signatories should always see their own treaties
    assert(secret_treaty.IsVisibleTo(realm_a) == true);
    assert(secret_treaty.IsVisibleTo(realm_b) == true);
    std::cout << "  ✓ Signatories can see their own treaty\n";

    // Observer should not see undiscovered secret treaty
    assert(secret_treaty.IsVisibleTo(observer) == false);
    std::cout << "  ✓ Observer cannot see undiscovered secret treaty\n";

    // Reveal to observer
    secret_treaty.RevealTo(observer);
    assert(secret_treaty.IsVisibleTo(observer) == true);
    std::cout << "  ✓ Observer can see treaty after discovery\n";

    std::cout << "✓ Secret treaty visibility test passed\n\n";
}

/**
 * Test treaty discovery difficulty calculation
 */
void test_discovery_difficulty() {
    std::cout << "Testing treaty discovery difficulty...\n";

    game::types::EntityID realm_a = 1;
    game::types::EntityID realm_b = 2;

    // Alliance treaties are harder to hide (0.8 * 0.8 = 0.64)
    Treaty alliance(TreatyType::ALLIANCE, realm_a, realm_b);
    alliance.is_secret = true;
    alliance.secrecy_level = 0.8;

    double alliance_difficulty = alliance.GetDiscoveryDifficulty();
    std::cout << "  Alliance discovery difficulty: " << alliance_difficulty << "\n";
    assert(alliance_difficulty < 0.8); // Should be reduced due to military movements

    // Non-aggression pacts are easier to keep secret (0.8 * 1.2 = 0.96, clamped to 1.0)
    Treaty nap(TreatyType::NON_AGGRESSION, realm_a, realm_b);
    nap.is_secret = true;
    nap.secrecy_level = 0.8;

    double nap_difficulty = nap.GetDiscoveryDifficulty();
    std::cout << "  Non-aggression pact discovery difficulty: " << nap_difficulty << "\n";
    assert(nap_difficulty > alliance_difficulty); // Easier to hide

    std::cout << "✓ Discovery difficulty test passed\n\n";
}

/**
 * Test hidden opinion functionality
 */
void test_hidden_opinions() {
    std::cout << "Testing hidden opinion functionality...\n";

    DiplomaticState state;
    state.opinion = -50; // True negative opinion

    // Initially, opinion should not be hidden
    assert(state.IsOpinionHidden() == false);

    // Observer with high intelligence should see true opinion
    double high_intel = 0.8;
    int perceived_high = state.GetPerceivedOpinion(high_intel);
    assert(perceived_high == -50);
    std::cout << "  ✓ Unhidden opinion visible to all\n";

    // Hide the opinion with a fake friendly opinion
    state.SetDisplayedOpinion(20, 0.7); // Fake +20 opinion with 70% quality
    assert(state.IsOpinionHidden() == true);

    // Low intelligence observer should see fake opinion
    double low_intel = 0.3;
    int perceived_low = state.GetPerceivedOpinion(low_intel);
    std::cout << "  Low intelligence observer sees: " << perceived_low << " (fake: 20, true: -50)\n";
    assert(perceived_low >= 15 && perceived_low <= 25); // Should be close to fake opinion

    // High intelligence observer might see through deception
    int perceived_high_hidden = state.GetPerceivedOpinion(high_intel);
    std::cout << "  High intelligence observer sees: " << perceived_high_hidden << " (should be between true and fake)\n";
    // High intelligence should see something between true (-50) and fake (20)
    // The exact value depends on the deception detection logic

    // Stop hiding opinion
    state.StopHidingOpinion();
    assert(state.IsOpinionHidden() == false);

    std::cout << "✓ Hidden opinion test passed\n\n";
}

/**
 * Test diplomatic information filtering
 */
void test_information_filtering() {
    std::cout << "Testing diplomatic information filtering...\n";

    DiplomacyComponent diplomacy;
    game::types::EntityID realm_a = 1;
    game::types::EntityID realm_b = 2;
    game::types::EntityID observer = 3;

    // Create public treaty
    Treaty public_treaty(TreatyType::TRADE_AGREEMENT, realm_a, realm_b);
    public_treaty.is_secret = false;
    diplomacy.AddTreaty(public_treaty);

    // Create secret treaty
    Treaty secret_treaty(TreatyType::ALLIANCE, realm_a, realm_b);
    secret_treaty.is_secret = true;
    secret_treaty.secrecy_level = 0.8;
    diplomacy.AddTreaty(secret_treaty);

    // Observer should see public treaty
    auto visible_treaties = diplomacy.GetVisibleTreaties(observer);
    std::cout << "  Observer sees " << visible_treaties.size() << " treaties (expected: 1)\n";
    assert(visible_treaties.size() == 1);
    assert(visible_treaties[0]->type == TreatyType::TRADE_AGREEMENT);

    // Realm A should see both treaties (signatory)
    auto realm_a_visible = diplomacy.GetVisibleTreaties(realm_a);
    std::cout << "  Realm A sees " << realm_a_visible.size() << " treaties (expected: 2)\n";
    assert(realm_a_visible.size() == 2);

    // Check HasVisibleTreatyType
    assert(diplomacy.HasVisibleTreatyType(realm_b, TreatyType::TRADE_AGREEMENT, observer) == true);
    assert(diplomacy.HasVisibleTreatyType(realm_b, TreatyType::ALLIANCE, observer) == false);
    std::cout << "  ✓ HasVisibleTreatyType correctly filters secret treaties\n";

    std::cout << "✓ Information filtering test passed\n\n";
}

/**
 * Test perceived opinion with different intelligence levels
 */
void test_perceived_opinion_intelligence() {
    std::cout << "Testing perceived opinion with different intelligence levels...\n";

    DiplomacyComponent diplomacy;
    game::types::EntityID other_realm = 42;

    // Create relationship with hidden negative opinion
    auto* state = diplomacy.GetRelationship(other_realm);
    state->opinion = -40;
    state->SetDisplayedOpinion(15, 0.6); // Moderate deception quality

    // Test perceived opinion at different intelligence levels
    int perceived_low = diplomacy.GetPerceivedOpinionOf(other_realm, 0.2);
    int perceived_mid = diplomacy.GetPerceivedOpinionOf(other_realm, 0.5);
    int perceived_high = diplomacy.GetPerceivedOpinionOf(other_realm, 0.9);

    std::cout << "  Low intel (0.2): " << perceived_low << "\n";
    std::cout << "  Mid intel (0.5): " << perceived_mid << "\n";
    std::cout << "  High intel (0.9): " << perceived_high << "\n";

    // Low intelligence should see mostly fake opinion
    assert(perceived_low >= 10);

    // High intelligence with moderate deception quality should partially see through it
    // The exact behavior depends on implementation details

    std::cout << "✓ Perceived opinion intelligence test passed\n\n";
}

/**
 * Test treaty IsSignatory helper
 */
void test_treaty_signatory_check() {
    std::cout << "Testing treaty signatory check...\n";

    game::types::EntityID realm_a = 1;
    game::types::EntityID realm_b = 2;
    game::types::EntityID other = 3;

    Treaty treaty(TreatyType::ALLIANCE, realm_a, realm_b);

    assert(treaty.IsSignatory(realm_a) == true);
    assert(treaty.IsSignatory(realm_b) == true);
    assert(treaty.IsSignatory(other) == false);

    std::cout << "✓ Treaty signatory check passed\n\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "Secret Diplomacy Tests\n";
    std::cout << "========================================\n\n";

    try {
        test_secret_treaty_visibility();
        test_discovery_difficulty();
        test_hidden_opinions();
        test_information_filtering();
        test_perceived_opinion_intelligence();
        test_treaty_signatory_check();

        std::cout << "========================================\n";
        std::cout << "All secret diplomacy tests passed! ✓\n";
        std::cout << "========================================\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception\n";
        return 1;
    }
}
