// ============================================================================
// Mechanica Imperii - Realm System Refactoring Tests
// Comprehensive Test Suite for Extracted Components
// ============================================================================

#include "game/realm/RealmCalculator.h"
#include "game/realm/RealmComponents.h"
#include <iostream>
#include <cassert>

using namespace game::realm;

bool TestRealmCalculator() {
    std::cout << "\n========== Testing RealmCalculator ==========\n";

    // Create test realm
    RealmComponent testRealm(1);
    testRealm.realmName = "Test Kingdom";
    testRealm.levySize = 10000;
    testRealm.standingArmy = 2000;
    testRealm.stability = 0.8f;
    testRealm.legitimacy = 0.9f;
    testRealm.centralAuthority = 0.6f;
    testRealm.treasury = 10000.0;
    testRealm.monthlyIncome = 500.0;
    testRealm.ownedProvinces = {1, 2, 3, 4, 5};

    // Test military strength
    float militaryStrength = RealmCalculator::CalculateMilitaryStrength(testRealm);
    assert(militaryStrength > 0.0f);
    std::cout << "Military strength: " << militaryStrength << " PASSED\n";

    // Test economic strength
    float economicStrength = RealmCalculator::CalculateEconomicStrength(testRealm);
    assert(economicStrength > 0.0f);
    std::cout << "Economic strength: " << economicStrength << " PASSED\n";

    // Test political strength
    float politicalStrength = RealmCalculator::CalculatePoliticalStrength(testRealm);
    assert(politicalStrength > 0.0f);
    std::cout << "Political strength: " << politicalStrength << " PASSED\n";

    // Test realm power
    float power = RealmCalculator::CalculateRealmPower(testRealm);
    assert(power > 0.0f);
    std::cout << "Total realm power: " << power << " PASSED\n";

    // Test rank determination
    RealmRank rank = RealmCalculator::DetermineRealmRank(5);
    assert(rank == RealmRank::COUNTY);
    std::cout << "Rank determination (5 provinces = COUNTY): PASSED\n";

    rank = RealmCalculator::DetermineRealmRank(30);
    assert(rank == RealmRank::KINGDOM);
    std::cout << "Rank determination (30 provinces = KINGDOM): PASSED\n";

    // Test rank multiplier
    float multiplier = RealmCalculator::GetRankMultiplier(RealmRank::KINGDOM);
    assert(multiplier == 4.0f);
    std::cout << "Rank multiplier (KINGDOM = 4.0): PASSED\n";

    // Test succession stability
    float stability = RealmCalculator::CalculateSuccessionStability(SuccessionLaw::PRIMOGENITURE);
    assert(stability > 0.0f && stability <= 1.0f);
    std::cout << "Succession stability: " << stability << " PASSED\n";

    // Test war calculations
    size_t transfer = RealmCalculator::CalculateProvinceTransfer(10, 60.0f);
    assert(transfer == 3); // 10/3 for 60% warscore
    std::cout << "Province transfer (60% warscore): " << transfer << " PASSED\n";

    double reparations = RealmCalculator::CalculateWarReparations(10000.0, 50.0f);
    assert(reparations == 2500.0); // 50% of 50% warscore
    std::cout << "War reparations: " << reparations << " PASSED\n";

    // Test central authority
    float authority = RealmCalculator::CalculateCentralAuthority(CrownAuthority::MEDIUM);
    assert(authority == 0.6f);
    std::cout << "Central authority (MEDIUM = 0.6): PASSED\n";

    // Test legitimacy by government
    float legitimacy = RealmCalculator::CalculateLegitimacyByGovernment(GovernmentType::THEOCRACY);
    assert(legitimacy == 1.0f);
    std::cout << "Legitimacy by government (THEOCRACY = 1.0): PASSED\n";

    std::cout << "RealmCalculator tests: ALL PASSED\n";
    return true;
}

bool TestRealmComponents() {
    std::cout << "\n========== Testing Realm Components ==========\n";

    // Test RealmComponent initialization
    RealmComponent realm(1);
    realm.realmName = "Test Realm";
    realm.governmentType = GovernmentType::FEUDAL_MONARCHY;
    realm.rank = RealmRank::DUCHY;

    assert(realm.realmId == 1);
    assert(realm.realmName == "Test Realm");
    assert(realm.governmentType == GovernmentType::FEUDAL_MONARCHY);
    std::cout << "RealmComponent initialization: PASSED\n";

    // Test DiplomaticRelationsComponent
    DiplomaticRelationsComponent diplomacy(1);
    DiplomaticRelation relation;
    relation.otherRealm = 2;
    relation.status = DiplomaticStatus::ALLIED;
    relation.opinion = 75.0f;

    diplomacy.SetRelation(2, relation);
    auto* storedRelation = diplomacy.GetRelation(2);
    assert(storedRelation != nullptr);
    assert(storedRelation->status == DiplomaticStatus::ALLIED);
    assert(storedRelation->opinion == 75.0f);
    std::cout << "DiplomaticRelationsComponent: PASSED\n";

    // Test alliance checking
    diplomacy.alliances.push_back(2);
    bool isAllied = diplomacy.IsAlliedWith(2);
    assert(isAllied == true);
    std::cout << "Alliance checking: PASSED\n";

    // Test war checking
    relation.atWar = true;
    diplomacy.SetRelation(3, relation);
    bool atWar = diplomacy.IsAtWarWith(3);
    assert(atWar == true);
    std::cout << "War checking: PASSED\n";

    std::cout << "Realm Components tests: ALL PASSED\n";
    return true;
}

bool TestSuccessionScenario() {
    std::cout << "\n========== Testing Succession Scenario ==========\n";

    RealmComponent realm(1);
    realm.realmName = "Test Kingdom";
    realm.currentRuler = 100;
    realm.successionLaw = SuccessionLaw::PRIMOGENITURE;
    realm.stability = 0.9f;
    realm.legitimacy = 0.85f;

    std::cout << "Initial state:\n";
    std::cout << "   Ruler: " << realm.currentRuler << "\n";
    std::cout << "   Stability: " << realm.stability << "\n";
    std::cout << "   Legitimacy: " << realm.legitimacy << "\n";

    // Calculate succession effects
    float stabilityMultiplier = RealmCalculator::CalculateSuccessionStability(realm.successionLaw);
    float legitimacyMultiplier = RealmCalculator::CalculateLegitimacyChange(realm.successionLaw);

    std::cout << "Succession (PRIMOGENITURE):\n";
    std::cout << "   Stability multiplier: " << stabilityMultiplier << "\n";
    std::cout << "   Legitimacy multiplier: " << legitimacyMultiplier << "\n";

    realm.stability *= stabilityMultiplier;
    realm.legitimacy *= legitimacyMultiplier;

    std::cout << "After succession:\n";
    std::cout << "   New stability: " << realm.stability << "\n";
    std::cout << "   New legitimacy: " << realm.legitimacy << "\n";

    assert(realm.stability > 0.8f && realm.stability <= 0.9f);
    assert(realm.legitimacy > 0.8f && realm.legitimacy <= 0.9f);

    std::cout << "Succession scenario: PASSED\n";
    return true;
}

bool TestWarScenario() {
    std::cout << "\n========== Testing War Scenario ==========\n";

    RealmComponent aggressor(1);
    aggressor.realmName = "Aggressor Kingdom";
    aggressor.levySize = 15000;
    aggressor.stability = 0.9f;
    aggressor.treasury = 10000.0;
    aggressor.ownedProvinces = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    RealmComponent defender(2);
    defender.realmName = "Defender Kingdom";
    defender.levySize = 10000;
    defender.stability = 0.8f;
    defender.treasury = 8000.0;
    defender.ownedProvinces = {11, 12, 13, 14, 15};

    std::cout << "Initial state:\n";
    std::cout << "   Aggressor provinces: " << aggressor.ownedProvinces.size() << "\n";
    std::cout << "   Defender provinces: " << defender.ownedProvinces.size() << "\n";
    std::cout << "   Defender treasury: " << defender.treasury << "\n";

    // Simulate war with 60% warscore (aggressor victory)
    float warscore = 60.0f;

    size_t provincesToTransfer = RealmCalculator::CalculateProvinceTransfer(
        defender.ownedProvinces.size(), warscore);
    double reparations = RealmCalculator::CalculateWarReparations(
        defender.treasury, warscore);

    std::cout << "War result (60% warscore):\n";
    std::cout << "   Provinces to transfer: " << provincesToTransfer << "\n";
    std::cout << "   Reparations: " << reparations << "\n";

    // Apply consequences
    defender.treasury -= reparations;
    aggressor.treasury += reparations;

    float aggressorStabilityChange = RealmCalculator::CalculateStabilityLoss(true, warscore);
    float defenderStabilityChange = RealmCalculator::CalculateStabilityLoss(false, warscore);

    aggressor.stability += aggressorStabilityChange;
    defender.stability += defenderStabilityChange;

    std::cout << "After war:\n";
    std::cout << "   Aggressor treasury: " << aggressor.treasury << "\n";
    std::cout << "   Defender treasury: " << defender.treasury << "\n";
    std::cout << "   Aggressor stability: " << aggressor.stability << "\n";
    std::cout << "   Defender stability: " << defender.stability << "\n";

    assert(aggressor.treasury > 10000.0);
    assert(defender.treasury < 8000.0);
    assert(provincesToTransfer == 1); // 5/10 = 0 but with 60% should be 1

    std::cout << "War scenario: PASSED\n";
    return true;
}

int main() {
    std::cout << "\n";
    std::cout << "========================================================\n";
    std::cout << "     REALM SYSTEM REFACTORING - TEST SUITE\n";
    std::cout << "========================================================\n";

    bool all_passed = true;

    all_passed &= TestRealmCalculator();
    all_passed &= TestRealmComponents();
    all_passed &= TestSuccessionScenario();
    all_passed &= TestWarScenario();

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
    std::cout << "  - RealmRepository: Component access layer created\n";
    std::cout << "  - RealmCalculator: Pure calculation functions extracted\n";
    std::cout << "\n";
    std::cout << "Expected Benefits:\n";
    std::cout << "  - Reduced RealmManager.cpp from ~1,602 lines\n";
    std::cout << "  - Improved testability with pure functions\n";
    std::cout << "  - Better separation of concerns\n";
    std::cout << "  - Centralized component access (Repository Pattern)\n";
    std::cout << "\n";

    return all_passed ? 0 : 1;
}
