// ============================================================================
// Mechanica Imperii - Population System Refactoring Tests
// Comprehensive Test Suite for Extracted Components
// ============================================================================

#include "game/population/PopulationCalculator.h"
#include "game/population/PopulationTypes.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace game::population;

bool TestHistoricalPercentages() {
    std::cout << "\n========== Testing Historical Percentages ==========\n";

    // Test noble percentage
    double noble_pct = PopulationCalculator::GetHistoricalPercentage(
        SocialClass::HIGH_NOBILITY, 1200, 0.7);
    assert(noble_pct > 0.0 && noble_pct < 0.01);
    std::cout << "Noble percentage (1200 AD): " << noble_pct << " PASSED\n";

    // Test peasant percentage
    double peasant_pct = PopulationCalculator::GetHistoricalPercentage(
        SocialClass::SERFS, 1200, 0.5);
    assert(peasant_pct > 0.2 && peasant_pct < 0.4);
    std::cout << "Peasant percentage (1200 AD): " << peasant_pct << " PASSED\n";

    // Test period variation (early medieval more serfs)
    double serfs_early = PopulationCalculator::GetHistoricalPercentage(
        SocialClass::SERFS, 1000, 0.5);
    double serfs_late = PopulationCalculator::GetHistoricalPercentage(
        SocialClass::SERFS, 1400, 0.5);
    assert(serfs_early > serfs_late);
    std::cout << "Serfdom decline over time: PASSED\n";

    std::cout << "Historical percentages tests: ALL PASSED\n";
    return true;
}

bool TestUrbanization() {
    std::cout << "\n========== Testing Urbanization ==========\n";

    // Test base urbanization
    double urban_rate = PopulationCalculator::CalculateUrbanizationRate(50000, 0.5, 1200);
    assert(urban_rate >= 0.02 && urban_rate <= 0.25);
    std::cout << "Urbanization rate (50k pop, 1200 AD): " << urban_rate << " PASSED\n";

    // Test prosperity impact
    double high_prosperity = PopulationCalculator::CalculateUrbanizationRate(50000, 0.9, 1200);
    double low_prosperity = PopulationCalculator::CalculateUrbanizationRate(50000, 0.3, 1200);
    assert(high_prosperity > low_prosperity);
    std::cout << "Prosperity increases urbanization: PASSED\n";

    // Test historical period impact
    double medieval = PopulationCalculator::CalculateUrbanizationRate(50000, 0.5, 1200);
    double early = PopulationCalculator::CalculateUrbanizationRate(50000, 0.5, 900);
    assert(medieval > early);
    std::cout << "Urbanization increases over time: PASSED\n";

    std::cout << "Urbanization tests: ALL PASSED\n";
    return true;
}

bool TestWealthCalculations() {
    std::cout << "\n========== Testing Wealth Calculations ==========\n";

    // Test base wealth
    double noble_wealth = PopulationCalculator::GetClassBaseWealth(
        SocialClass::HIGH_NOBILITY, 0.7);
    double serf_wealth = PopulationCalculator::GetClassBaseWealth(
        SocialClass::SERFS, 0.7);

    assert(noble_wealth > serf_wealth * 10);
    std::cout << "Noble wealth: " << noble_wealth << ", Serf wealth: " << serf_wealth << " PASSED\n";

    // Test group wealth
    double group_wealth = PopulationCalculator::CalculateGroupWealth(1000, 100.0);
    assert(std::abs(group_wealth - 100000.0) < 0.01);
    std::cout << "Group wealth calculation: " << group_wealth << " PASSED\n";

    std::cout << "Wealth calculation tests: ALL PASSED\n";
    return true;
}

bool TestLiteracy() {
    std::cout << "\n========== Testing Literacy Rates ==========\n";

    // Test clergy high literacy
    double clergy_literacy = PopulationCalculator::GetClassLiteracyRate(
        SocialClass::HIGH_CLERGY, 1200);
    assert(clergy_literacy > 0.9);
    std::cout << "Clergy literacy rate: " << clergy_literacy << " PASSED\n";

    // Test peasant low literacy
    double peasant_literacy = PopulationCalculator::GetClassLiteracyRate(
        SocialClass::SERFS, 1200);
    assert(peasant_literacy < 0.05);
    std::cout << "Peasant literacy rate: " << peasant_literacy << " PASSED\n";

    // Test period variation
    double literacy_early = PopulationCalculator::GetClassLiteracyRate(
        SocialClass::BURGHERS, 1000);
    double literacy_late = PopulationCalculator::GetClassLiteracyRate(
        SocialClass::BURGHERS, 1400);
    assert(literacy_late > literacy_early);
    std::cout << "Literacy increases over time: PASSED\n";

    std::cout << "Literacy tests: ALL PASSED\n";
    return true;
}

bool TestDemographics() {
    std::cout << "\n========== Testing Demographics ==========\n";

    // Test age distribution
    auto [children, adults, elderly] = PopulationCalculator::CalculateAgeDistribution(1000);
    assert(children + adults + elderly == 1000);
    assert(children > 300 && children < 400); // ~35%
    assert(adults > 500 && adults < 600);     // ~55%
    std::cout << "Age distribution (1000 pop): " << children << "/" << adults << "/" << elderly << " PASSED\n";

    // Test gender distribution
    auto [males, females] = PopulationCalculator::CalculateGenderDistribution(1000);
    assert(males + females == 1000);
    assert(males > 450 && males < 500); // ~48%
    std::cout << "Gender distribution (1000 pop): " << males << " males, " << females << " females PASSED\n";

    std::cout << "Demographics tests: ALL PASSED\n";
    return true;
}

bool TestMilitaryCalculations() {
    std::cout << "\n========== Testing Military Calculations ==========\n";

    // Test noble military eligibility (high)
    int noble_eligible = PopulationCalculator::CalculateMilitaryEligible(
        1000, SocialClass::HIGH_NOBILITY);
    assert(noble_eligible > 800); // 90% eligible
    std::cout << "Noble military eligible: " << noble_eligible << "/1000 PASSED\n";

    // Test clergy military eligibility (low)
    int clergy_eligible = PopulationCalculator::CalculateMilitaryEligible(
        1000, SocialClass::CLERGY);
    assert(clergy_eligible < 150); // 10% eligible
    std::cout << "Clergy military eligible: " << clergy_eligible << "/1000 PASSED\n";

    // Test military quality
    double noble_quality = PopulationCalculator::CalculateMilitaryQuality(
        SocialClass::HIGH_NOBILITY, 0.7);
    double peasant_quality = PopulationCalculator::CalculateMilitaryQuality(
        SocialClass::SERFS, 0.7);
    assert(noble_quality > peasant_quality * 2);
    std::cout << "Noble quality: " << noble_quality << ", Peasant quality: " << peasant_quality << " PASSED\n";

    std::cout << "Military calculation tests: ALL PASSED\n";
    return true;
}

bool TestSettlementCalculations() {
    std::cout << "\n========== Testing Settlement Calculations ==========\n";

    // Test infrastructure by type
    double city_infra = PopulationCalculator::GetSettlementInfrastructure(
        SettlementType::MAJOR_CITY, 0.8);
    double hamlet_infra = PopulationCalculator::GetSettlementInfrastructure(
        SettlementType::RURAL_HAMLET, 0.8);
    assert(city_infra > hamlet_infra * 2);
    std::cout << "City infrastructure: " << city_infra << ", Hamlet: " << hamlet_infra << " PASSED\n";

    // Test fortification
    double fortress_fort = PopulationCalculator::GetSettlementFortification(
        SettlementType::ROYAL_CASTLE, 0.8);
    assert(fortress_fort > 0.9);
    std::cout << "Fortress fortification: " << fortress_fort << " PASSED\n";

    // Test disease risk
    double city_disease = PopulationCalculator::GetSettlementDiseaseRisk(
        SettlementType::MAJOR_CITY, 0.5);
    double village_disease = PopulationCalculator::GetSettlementDiseaseRisk(
        SettlementType::VILLAGE, 0.5);
    assert(city_disease > village_disease);
    std::cout << "City disease risk: " << city_disease << ", Village: " << village_disease << " PASSED\n";

    std::cout << "Settlement calculation tests: ALL PASSED\n";
    return true;
}

int main() {
    std::cout << "\n";
    std::cout << "========================================================\n";
    std::cout << "  POPULATION SYSTEM REFACTORING - TEST SUITE\n";
    std::cout << "========================================================\n";

    bool all_passed = true;

    all_passed &= TestHistoricalPercentages();
    all_passed &= TestUrbanization();
    all_passed &= TestWealthCalculations();
    all_passed &= TestLiteracy();
    all_passed &= TestDemographics();
    all_passed &= TestMilitaryCalculations();
    all_passed &= TestSettlementCalculations();

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
    std::cout << "  - PopulationCalculator: Pure calculation functions extracted\n";
    std::cout << "\n";
    std::cout << "Expected Benefits:\n";
    std::cout << "  - Reduced PopulationFactory.cpp from ~1,399 lines\n";
    std::cout << "  - All demographic calculations testable in isolation\n";
    std::cout << "  - Historical accuracy maintained through pure functions\n";
    std::cout << "  - Settlement calculations centralized\n";
    std::cout << "\n";

    return all_passed ? 0 : 1;
}
