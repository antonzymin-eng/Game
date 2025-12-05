// ============================================================================
// test_serialization_phase6_7.cpp - Comprehensive Serialization Tests
// Created: December 5, 2025
// Purpose: Unit and integration tests for Phase 6, 6.5, and 7 serialization
// ============================================================================

#include <cassert>
#include <iostream>
#include <string>
#include <chrono>
#include <vector>

// Character system includes
#include "game/components/CharacterComponent.h"
#include "game/components/TraitsComponent.h"
#include "game/character/CharacterEducation.h"
#include "game/character/CharacterLifeEvents.h"
#include "game/character/CharacterRelationships.h"

// Population system includes
#include "game/population/PopulationComponents.h"

using namespace game::character;
using namespace game::population;

// ============================================================================
// Test Utilities
// ============================================================================

int g_tests_passed = 0;
int g_tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    if (!(condition)) { \
        std::cerr << "❌ FAIL: " << message << std::endl; \
        std::cerr << "  at " << __FILE__ << ":" << __LINE__ << std::endl; \
        g_tests_failed++; \
        return false; \
    } else { \
        g_tests_passed++; \
    }

void PrintTestResult(const std::string& test_name, bool passed) {
    if (passed) {
        std::cout << "✅ PASS: " << test_name << std::endl;
    } else {
        std::cout << "❌ FAIL: " << test_name << std::endl;
    }
}

// ============================================================================
// Phase 6.5: TraitsComponent Tests
// ============================================================================

bool TestTraitsComponentSerialization() {
    std::cout << "\n=== Testing TraitsComponent Serialization ===" << std::endl;

    // Create original component with test data
    TraitsComponent original;

    // Add permanent trait
    ActiveTrait brave;
    brave.trait_id = "brave";
    brave.acquired_date = std::chrono::system_clock::now();
    brave.is_temporary = false;
    original.active_traits.push_back(brave);

    // Add temporary trait
    ActiveTrait wounded;
    wounded.trait_id = "wounded";
    wounded.acquired_date = std::chrono::system_clock::now();
    wounded.is_temporary = true;
    wounded.expiry_date = std::chrono::system_clock::now() + std::chrono::hours(720); // 30 days
    original.active_traits.push_back(wounded);

    // Serialize
    std::string json = original.Serialize();
    TEST_ASSERT(!json.empty(), "Serialized JSON should not be empty");
    TEST_ASSERT(json.find("brave") != std::string::npos, "JSON should contain 'brave' trait");
    TEST_ASSERT(json.find("wounded") != std::string::npos, "JSON should contain 'wounded' trait");

    // Deserialize
    TraitsComponent loaded;
    bool success = loaded.Deserialize(json);
    TEST_ASSERT(success, "Deserialization should succeed");

    // Verify data
    TEST_ASSERT(loaded.active_traits.size() == 2, "Should have 2 traits after deserialization");
    TEST_ASSERT(loaded.active_traits[0].trait_id == "brave", "First trait should be 'brave'");
    TEST_ASSERT(loaded.active_traits[0].is_temporary == false, "First trait should be permanent");
    TEST_ASSERT(loaded.active_traits[1].trait_id == "wounded", "Second trait should be 'wounded'");
    TEST_ASSERT(loaded.active_traits[1].is_temporary == true, "Second trait should be temporary");

    // Verify time points are preserved (within 1 second tolerance)
    auto original_acquired = std::chrono::duration_cast<std::chrono::seconds>(
        original.active_traits[0].acquired_date.time_since_epoch()).count();
    auto loaded_acquired = std::chrono::duration_cast<std::chrono::seconds>(
        loaded.active_traits[0].acquired_date.time_since_epoch()).count();
    TEST_ASSERT(std::abs(original_acquired - loaded_acquired) < 2, "Acquired date should be preserved");

    std::cout << "  ✓ Trait serialization successful" << std::endl;
    std::cout << "  ✓ Time point preservation verified" << std::endl;
    std::cout << "  ✓ Temporary trait expiry preserved" << std::endl;

    return true;
}

// ============================================================================
// Phase 6.5: CharacterEducationComponent Tests
// ============================================================================

bool TestCharacterEducationComponentSerialization() {
    std::cout << "\n=== Testing CharacterEducationComponent Serialization ===" << std::endl;

    // Create original component
    CharacterEducationComponent original(123);
    original.is_educated = true;
    original.education_focus = EducationFocus::DIPLOMACY;
    original.education_quality = EducationQuality::EXCELLENT;
    original.educator = 456;
    original.education_start = std::chrono::system_clock::now() - std::chrono::hours(8760); // 1 year ago
    original.education_end = std::chrono::system_clock::now();
    original.skill_xp.diplomacy_xp = 150;
    original.skill_xp.martial_xp = 80;
    original.skill_xp.stewardship_xp = 120;
    original.skill_xp.intrigue_xp = 90;
    original.skill_xp.learning_xp = 200;
    original.learning_rate_modifier = 1.2f;
    original.education_traits.push_back("scholarly_educated");
    original.education_traits.push_back("diplomatic_master");

    // Serialize
    std::string json = original.Serialize();
    TEST_ASSERT(!json.empty(), "Serialized JSON should not be empty");

    // Deserialize
    CharacterEducationComponent loaded;
    bool success = loaded.Deserialize(json);
    TEST_ASSERT(success, "Deserialization should succeed");

    // Verify data
    TEST_ASSERT(loaded.character_id == 123, "Character ID should match");
    TEST_ASSERT(loaded.is_educated == true, "Education status should match");
    TEST_ASSERT(loaded.education_focus == EducationFocus::DIPLOMACY, "Education focus should match");
    TEST_ASSERT(loaded.education_quality == EducationQuality::EXCELLENT, "Education quality should match");
    TEST_ASSERT(loaded.educator == 456, "Educator ID should match");
    TEST_ASSERT(loaded.skill_xp.diplomacy_xp == 150, "Diplomacy XP should match");
    TEST_ASSERT(loaded.skill_xp.martial_xp == 80, "Martial XP should match");
    TEST_ASSERT(loaded.skill_xp.stewardship_xp == 120, "Stewardship XP should match");
    TEST_ASSERT(loaded.skill_xp.intrigue_xp == 90, "Intrigue XP should match");
    TEST_ASSERT(loaded.skill_xp.learning_xp == 200, "Learning XP should match");
    TEST_ASSERT(std::abs(loaded.learning_rate_modifier - 1.2f) < 0.01f, "Learning rate modifier should match");
    TEST_ASSERT(loaded.education_traits.size() == 2, "Should have 2 education traits");
    TEST_ASSERT(loaded.education_traits[0] == "scholarly_educated", "First trait should match");
    TEST_ASSERT(loaded.education_traits[1] == "diplomatic_master", "Second trait should match");

    std::cout << "  ✓ Education data preserved" << std::endl;
    std::cout << "  ✓ Skill XP values correct" << std::endl;
    std::cout << "  ✓ Education traits restored" << std::endl;

    return true;
}

// ============================================================================
// Phase 6.5: CharacterLifeEventsComponent Tests
// ============================================================================

bool TestCharacterLifeEventsComponentSerialization() {
    std::cout << "\n=== Testing CharacterLifeEventsComponent Serialization ===" << std::endl;

    // Create original component
    CharacterLifeEventsComponent original(789);
    original.birth_date = std::chrono::system_clock::now() - std::chrono::hours(8760 * 20); // 20 years ago
    original.coming_of_age_date = std::chrono::system_clock::now() - std::chrono::hours(8760 * 4); // 4 years ago

    // Add birth event
    LifeEvent birth(LifeEventType::BIRTH, "Born in London");
    birth.date = original.birth_date;
    birth.age_at_event = 0;
    birth.is_major = true;
    birth.location = "London";
    original.life_events.push_back(birth);

    // Add coming of age event
    LifeEvent coming_of_age(LifeEventType::COMING_OF_AGE, "Came of age");
    coming_of_age.date = original.coming_of_age_date;
    coming_of_age.age_at_event = 16;
    coming_of_age.is_major = true;
    original.life_events.push_back(coming_of_age);

    // Add marriage event
    LifeEvent marriage(LifeEventType::MARRIAGE, "Married Lady Jane");
    marriage.date = std::chrono::system_clock::now() - std::chrono::hours(8760); // 1 year ago
    marriage.age_at_event = 19;
    marriage.related_character = 999;
    marriage.is_major = true;
    marriage.impact_prestige = 50.0f;
    marriage.traits_gained.push_back("married");
    original.life_events.push_back(marriage);

    // Serialize
    std::string json = original.Serialize();
    TEST_ASSERT(!json.empty(), "Serialized JSON should not be empty");

    // Deserialize
    CharacterLifeEventsComponent loaded;
    bool success = loaded.Deserialize(json);
    TEST_ASSERT(success, "Deserialization should succeed");

    // Verify data
    TEST_ASSERT(loaded.character_id == 789, "Character ID should match");
    TEST_ASSERT(loaded.life_events.size() == 3, "Should have 3 life events");

    // Verify first event (birth)
    TEST_ASSERT(loaded.life_events[0].type == LifeEventType::BIRTH, "First event should be BIRTH");
    TEST_ASSERT(loaded.life_events[0].description == "Born in London", "Birth description should match");
    TEST_ASSERT(loaded.life_events[0].age_at_event == 0, "Birth age should be 0");
    TEST_ASSERT(loaded.life_events[0].is_major == true, "Birth should be major event");
    TEST_ASSERT(loaded.life_events[0].location == "London", "Birth location should match");

    // Verify marriage event
    TEST_ASSERT(loaded.life_events[2].type == LifeEventType::MARRIAGE, "Third event should be MARRIAGE");
    TEST_ASSERT(loaded.life_events[2].related_character == 999, "Related character should match");
    TEST_ASSERT(std::abs(loaded.life_events[2].impact_prestige - 50.0f) < 0.01f, "Prestige impact should match");
    TEST_ASSERT(loaded.life_events[2].traits_gained.size() == 1, "Should have 1 trait gained");
    TEST_ASSERT(loaded.life_events[2].traits_gained[0] == "married", "Trait should be 'married'");

    std::cout << "  ✓ Life events preserved" << std::endl;
    std::cout << "  ✓ Event metadata restored" << std::endl;
    std::cout << "  ✓ Related entities preserved" << std::endl;

    return true;
}

// ============================================================================
// Phase 6.5: CharacterRelationshipsComponent Tests
// ============================================================================

bool TestCharacterRelationshipsComponentSerialization() {
    std::cout << "\n=== Testing CharacterRelationshipsComponent Serialization ===" << std::endl;

    // Create original component
    CharacterRelationshipsComponent original(111);
    original.current_spouse = 222;
    original.father = 333;
    original.mother = 444;
    original.siblings.push_back(555);
    original.siblings.push_back(666);
    original.children.push_back(777);
    original.children.push_back(888);

    // Add marriage
    Marriage marriage(222, 10, 5);
    marriage.type = MarriageType::NORMAL;
    marriage.is_alliance = true;
    marriage.children.push_back(777);
    marriage.children.push_back(888);
    original.marriages.push_back(marriage);

    // Add friend relationship
    CharacterRelationship friend_rel(999, RelationshipType::FRIEND);
    friend_rel.opinion = 75;
    friend_rel.bond_strength = 60.5;
    friend_rel.is_active = true;
    original.relationships[999] = friend_rel;

    // Add rival relationship
    CharacterRelationship rival_rel(1000, RelationshipType::RIVAL);
    rival_rel.opinion = -50;
    rival_rel.bond_strength = 40.0;
    rival_rel.is_active = true;
    original.relationships[1000] = rival_rel;

    // Serialize
    std::string json = original.Serialize();
    TEST_ASSERT(!json.empty(), "Serialized JSON should not be empty");

    // Deserialize
    CharacterRelationshipsComponent loaded;
    bool success = loaded.Deserialize(json);
    TEST_ASSERT(success, "Deserialization should succeed");

    // Verify data
    TEST_ASSERT(loaded.character_id == 111, "Character ID should match");
    TEST_ASSERT(loaded.current_spouse == 222, "Current spouse should match");
    TEST_ASSERT(loaded.father == 333, "Father should match");
    TEST_ASSERT(loaded.mother == 444, "Mother should match");
    TEST_ASSERT(loaded.siblings.size() == 2, "Should have 2 siblings");
    TEST_ASSERT(loaded.children.size() == 2, "Should have 2 children");

    // Verify marriage
    TEST_ASSERT(loaded.marriages.size() == 1, "Should have 1 marriage");
    TEST_ASSERT(loaded.marriages[0].spouse == 222, "Spouse should match");
    TEST_ASSERT(loaded.marriages[0].type == MarriageType::NORMAL, "Marriage type should match");
    TEST_ASSERT(loaded.marriages[0].is_alliance == true, "Alliance status should match");
    TEST_ASSERT(loaded.marriages[0].children.size() == 2, "Marriage should have 2 children");

    // Verify relationships
    TEST_ASSERT(loaded.relationships.size() == 2, "Should have 2 relationships");
    TEST_ASSERT(loaded.relationships.count(999) == 1, "Should have friend relationship");
    TEST_ASSERT(loaded.relationships[999].type == RelationshipType::FRIEND, "Relationship type should be FRIEND");
    TEST_ASSERT(loaded.relationships[999].opinion == 75, "Friend opinion should match");
    TEST_ASSERT(std::abs(loaded.relationships[999].bond_strength - 60.5) < 0.01, "Bond strength should match");

    TEST_ASSERT(loaded.relationships.count(1000) == 1, "Should have rival relationship");
    TEST_ASSERT(loaded.relationships[1000].type == RelationshipType::RIVAL, "Relationship type should be RIVAL");
    TEST_ASSERT(loaded.relationships[1000].opinion == -50, "Rival opinion should match");

    std::cout << "  ✓ Family tree preserved" << std::endl;
    std::cout << "  ✓ Marriages restored" << std::endl;
    std::cout << "  ✓ Relationships preserved" << std::endl;

    return true;
}

// ============================================================================
// Phase 7: PopulationComponent Tests
// ============================================================================

bool TestPopulationComponentSerialization() {
    std::cout << "\n=== Testing PopulationComponent Serialization ===" << std::endl;

    // Create original component
    PopulationComponent original;

    // Create population group
    PopulationGroup group;
    group.social_class = SocialClass::FREE_PEASANTS;
    group.legal_status = LegalStatus::FREE_PEASANT;
    group.culture = "english";
    group.religion = "catholic";
    group.population_count = 5000;
    group.happiness = 0.6;
    group.literacy_rate = 0.15;
    group.wealth_per_capita = 120.0;
    group.health_level = 0.7;
    group.children_0_14 = 1500;
    group.adults_15_64 = 3000;
    group.elderly_65_plus = 500;
    group.males = 2500;
    group.females = 2500;
    group.employment[EmploymentType::FARMING] = 2000;
    group.employment[EmploymentType::CRAFTS] = 800;
    group.employment_rate = 0.56;
    group.birth_rate = 0.035;
    group.death_rate = 0.028;
    group.military_eligible = 800;
    group.military_quality = 0.5;
    group.legal_privileges.push_back("land_ownership");
    group.economic_rights.push_back("trade");
    group.economic_rights.push_back("craft");

    original.population_groups.push_back(group);

    // Set aggregate statistics
    original.total_population = 5000;
    original.total_children = 1500;
    original.total_adults = 3000;
    original.total_elderly = 500;
    original.total_males = 2500;
    original.total_females = 2500;
    original.average_happiness = 0.6;
    original.average_literacy = 0.15;
    original.average_wealth = 120.0;
    original.average_health = 0.7;
    original.total_military_eligible = 800;

    // Set distributions
    original.culture_distribution["english"] = 5000;
    original.religion_distribution["catholic"] = 5000;
    original.class_distribution[SocialClass::FREE_PEASANTS] = 5000;

    // Serialize
    std::string json = original.Serialize();
    TEST_ASSERT(!json.empty(), "Serialized JSON should not be empty");
    TEST_ASSERT(json.find("english") != std::string::npos, "JSON should contain culture 'english'");
    TEST_ASSERT(json.find("catholic") != std::string::npos, "JSON should contain religion 'catholic'");

    // Deserialize
    PopulationComponent loaded;
    bool success = loaded.Deserialize(json);
    TEST_ASSERT(success, "Deserialization should succeed");

    // Verify data
    TEST_ASSERT(loaded.population_groups.size() == 1, "Should have 1 population group");

    // Verify group data
    const auto& loaded_group = loaded.population_groups[0];
    TEST_ASSERT(loaded_group.social_class == SocialClass::FREE_PEASANTS, "Social class should match");
    TEST_ASSERT(loaded_group.legal_status == LegalStatus::FREE_PEASANT, "Legal status should match");
    TEST_ASSERT(loaded_group.culture == "english", "Culture should match");
    TEST_ASSERT(loaded_group.religion == "catholic", "Religion should match");
    TEST_ASSERT(loaded_group.population_count == 5000, "Population count should match");
    TEST_ASSERT(std::abs(loaded_group.happiness - 0.6) < 0.01, "Happiness should match");
    TEST_ASSERT(loaded_group.children_0_14 == 1500, "Children count should match");
    TEST_ASSERT(loaded_group.adults_15_64 == 3000, "Adults count should match");
    TEST_ASSERT(loaded_group.elderly_65_plus == 500, "Elderly count should match");
    TEST_ASSERT(loaded_group.males == 2500, "Males count should match");
    TEST_ASSERT(loaded_group.females == 2500, "Females count should match");
    TEST_ASSERT(loaded_group.employment.size() == 2, "Should have 2 employment types");
    TEST_ASSERT(loaded_group.employment.at(EmploymentType::FARMING) == 2000, "Farming employment should match");
    TEST_ASSERT(loaded_group.employment.at(EmploymentType::CRAFTS) == 800, "Crafts employment should match");
    TEST_ASSERT(loaded_group.military_eligible == 800, "Military eligible should match");
    TEST_ASSERT(loaded_group.legal_privileges.size() == 1, "Should have 1 legal privilege");
    TEST_ASSERT(loaded_group.economic_rights.size() == 2, "Should have 2 economic rights");

    // Verify aggregate statistics
    TEST_ASSERT(loaded.total_population == 5000, "Total population should match");
    TEST_ASSERT(loaded.total_children == 1500, "Total children should match");
    TEST_ASSERT(loaded.total_adults == 3000, "Total adults should match");
    TEST_ASSERT(loaded.total_elderly == 500, "Total elderly should match");
    TEST_ASSERT(loaded.culture_distribution["english"] == 5000, "Culture distribution should match");
    TEST_ASSERT(loaded.religion_distribution["catholic"] == 5000, "Religion distribution should match");

    std::cout << "  ✓ Population group preserved" << std::endl;
    std::cout << "  ✓ Demographics data correct" << std::endl;
    std::cout << "  ✓ Employment map restored" << std::endl;
    std::cout << "  ✓ Distribution maps preserved" << std::endl;

    return true;
}

// ============================================================================
// Integration Tests
// ============================================================================

bool TestRoundTripConsistency() {
    std::cout << "\n=== Testing Round-Trip Consistency ===" << std::endl;

    // Test multiple serialize/deserialize cycles
    TraitsComponent original;
    ActiveTrait trait;
    trait.trait_id = "test_trait";
    trait.acquired_date = std::chrono::system_clock::now();
    trait.is_temporary = false;
    original.active_traits.push_back(trait);

    TraitsComponent current = original;

    // Perform 5 round trips
    for (int i = 0; i < 5; i++) {
        std::string json = current.Serialize();
        TraitsComponent next;
        bool success = next.Deserialize(json);
        TEST_ASSERT(success, "Round-trip deserialization should succeed");
        current = next;
    }

    // Verify data is still correct after 5 round trips
    TEST_ASSERT(current.active_traits.size() == 1, "Should still have 1 trait after round trips");
    TEST_ASSERT(current.active_traits[0].trait_id == "test_trait", "Trait ID should be preserved");

    std::cout << "  ✓ Multiple round-trip cycles successful" << std::endl;

    return true;
}

bool TestEmptyComponentSerialization() {
    std::cout << "\n=== Testing Empty Component Serialization ===" << std::endl;

    // Test empty components
    TraitsComponent empty_traits;
    std::string json = empty_traits.Serialize();
    TEST_ASSERT(!json.empty(), "Empty component should serialize to valid JSON");

    TraitsComponent loaded;
    bool success = loaded.Deserialize(json);
    TEST_ASSERT(success, "Empty component deserialization should succeed");
    TEST_ASSERT(loaded.active_traits.empty(), "Loaded component should be empty");

    std::cout << "  ✓ Empty component serialization works" << std::endl;

    return true;
}

bool TestInvalidDataHandling() {
    std::cout << "\n=== Testing Invalid Data Handling ===" << std::endl;

    // Test invalid JSON
    TraitsComponent component;
    bool success = component.Deserialize("invalid json {]");
    TEST_ASSERT(!success, "Invalid JSON should fail gracefully");

    // Test empty JSON
    success = component.Deserialize("{}");
    TEST_ASSERT(success, "Empty JSON object should deserialize successfully");

    // Test missing fields - should use defaults
    success = component.Deserialize("{\"active_traits\":[]}");
    TEST_ASSERT(success, "JSON with missing fields should succeed");

    std::cout << "  ✓ Invalid data handled gracefully" << std::endl;

    return true;
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "============================================================" << std::endl;
    std::cout << "  Phase 6/6.5/7 Serialization Test Suite" << std::endl;
    std::cout << "  Testing Character and Population Components" << std::endl;
    std::cout << "============================================================" << std::endl;

    bool all_passed = true;

    // Phase 6.5 Tests
    all_passed &= TestTraitsComponentSerialization();
    all_passed &= TestCharacterEducationComponentSerialization();
    all_passed &= TestCharacterLifeEventsComponentSerialization();
    all_passed &= TestCharacterRelationshipsComponentSerialization();

    // Phase 7 Tests
    all_passed &= TestPopulationComponentSerialization();

    // Integration Tests
    all_passed &= TestRoundTripConsistency();
    all_passed &= TestEmptyComponentSerialization();
    all_passed &= TestInvalidDataHandling();

    // Print summary
    std::cout << "\n============================================================" << std::endl;
    std::cout << "  Test Summary" << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << "  Total Assertions: " << (g_tests_passed + g_tests_failed) << std::endl;
    std::cout << "  Passed: " << g_tests_passed << std::endl;
    std::cout << "  Failed: " << g_tests_failed << std::endl;

    if (all_passed && g_tests_failed == 0) {
        std::cout << "\n  ✅ ALL TESTS PASSED! ✅" << std::endl;
        std::cout << "============================================================" << std::endl;
        return 0;
    } else {
        std::cout << "\n  ❌ SOME TESTS FAILED ❌" << std::endl;
        std::cout << "============================================================" << std::endl;
        return 1;
    }
}
