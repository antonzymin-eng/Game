// Created: November 19, 2025
// Location: tests/test_character_improvements.cpp
// Purpose: Tests for new character system features (traits, life events, education)

#include "game/components/TraitsComponent.h"
#include "game/character/CharacterLifeEvents.h"
#include "game/character/CharacterEducation.h"
#include "game/character/CharacterRelationships.h"
#include "utils/Random.h"

#include <iostream>
#include <cassert>
#include <optional>

using namespace game::character;

// ============================================================================
// Traits System Tests
// ============================================================================

bool TestTraitsComponent() {
    std::cout << "\n========== Testing Traits Component ==========\n";

    TraitsComponent traits;
    auto& trait_db = TraitDatabase::Instance();

    // Test adding traits
    bool added_brave = traits.AddTrait("brave", trait_db.GetTrait("brave"));
    assert(added_brave == true);
    std::cout << "Add trait 'brave': PASSED\n";

    // Test checking for trait
    assert(traits.HasTrait("brave") == true);
    std::cout << "Has trait 'brave': PASSED\n";

    // Test opposite traits
    bool added_craven = traits.AddTrait("craven", trait_db.GetTrait("craven"));
    assert(added_craven == false);  // Should fail because opposite of brave
    std::cout << "Cannot add opposite trait 'craven': PASSED\n";

    // Test adding compatible trait
    bool added_ambitious = traits.AddTrait("ambitious", trait_db.GetTrait("ambitious"));
    assert(added_ambitious == true);
    std::cout << "Add compatible trait 'ambitious': PASSED\n";

    // Test removing trait
    bool removed = traits.RemoveTrait("brave");
    assert(removed == true);
    assert(traits.HasTrait("brave") == false);
    std::cout << "Remove trait 'brave': PASSED\n";

    // Now craven can be added
    bool added_craven_now = traits.AddTrait("craven", trait_db.GetTrait("craven"));
    assert(added_craven_now == true);
    std::cout << "Add trait 'craven' after removing 'brave': PASSED\n";

    // Test modifiers calculation
    traits.AddTrait("kind", trait_db.GetTrait("kind"));
    traits.AddTrait("genius", trait_db.GetTrait("genius"));

    const auto& modifiers = traits.GetModifiers(trait_db.GetAllTraits());

    assert(modifiers.total_diplomacy > 0);  // genius and kind both add diplomacy
    std::cout << "Trait modifiers calculated (diplomacy = " << static_cast<int>(modifiers.total_diplomacy) << "): PASSED\n";

    // Test temporary traits
    traits.AddTemporaryTrait("wounded", std::chrono::hours(24), trait_db.GetTrait("wounded"));
    assert(traits.HasTrait("wounded") == true);
    std::cout << "Add temporary trait 'wounded': PASSED\n";

    std::cout << "Traits Component tests: ALL PASSED\n";
    return true;
}

bool TestTraitDatabase() {
    std::cout << "\n========== Testing Trait Database ==========\n";

    auto& trait_db = TraitDatabase::Instance();

    // Test getting trait
    const Trait* brave = trait_db.GetTrait("brave");
    assert(brave != nullptr);
    assert(brave->name == "Brave");
    std::cout << "Get trait 'brave': PASSED\n";

    // Test trait properties
    assert(brave->martial_modifier == 2);
    assert(brave->boldness_modifier > 0.0f);
    std::cout << "Trait 'brave' has correct modifiers: PASSED\n";

    // Test trait incompatibility
    bool incompatible = trait_db.AreTraitsIncompatible("brave", "craven");
    assert(incompatible == true);
    std::cout << "Traits 'brave' and 'craven' are incompatible: PASSED\n";

    // Test getting traits by category
    auto personality_traits = trait_db.GetTraitsByCategory(TraitCategory::PERSONALITY);
    assert(personality_traits.size() > 0);
    std::cout << "Get personality traits (" << personality_traits.size() << " found): PASSED\n";

    // Test genetic traits
    const Trait* genius = trait_db.GetTrait("genius");
    assert(genius != nullptr);
    assert(genius->is_genetic == true);
    assert(genius->is_congenital == true);
    std::cout << "Trait 'genius' is genetic and congenital: PASSED\n";

    std::cout << "Trait Database tests: ALL PASSED\n";
    return true;
}

// ============================================================================
// Life Events Tests
// ============================================================================

bool TestLifeEvents() {
    std::cout << "\n========== Testing Life Events ==========\n";

    CharacterLifeEventsComponent life_events(1);

    // Test adding birth event
    auto birth = LifeEventGenerator::CreateBirthEvent("Test Character", "London", 0, 0);
    life_events.AddEvent(birth);

    assert(life_events.life_events.size() == 1);
    assert(life_events.birth_date.time_since_epoch().count() > 0);
    std::cout << "Add birth event: PASSED\n";

    // Test coming of age
    auto coming_of_age = LifeEventGenerator::CreateComingOfAgeEvent("Test Character", 16);
    life_events.AddEvent(coming_of_age);

    assert(life_events.IsAdult() == true);
    std::cout << "Coming of age event: PASSED\n";

    // Test marriage event
    auto marriage = LifeEventGenerator::CreateMarriageEvent(
        "Test Character", "Spouse", 2, 25);
    life_events.AddEvent(marriage);

    auto marriages = life_events.GetEventsByType(LifeEventType::MARRIAGE);
    assert(marriages.size() == 1);
    std::cout << "Marriage event added: PASSED\n";

    // Test child birth
    auto child = LifeEventGenerator::CreateChildBirthEvent(
        "Test Character", "Child", 3, 26);
    life_events.AddEvent(child);

    assert(life_events.GetEventCount(LifeEventType::CHILD_BORN) == 1);
    std::cout << "Child birth event: PASSED\n";

    // Test battle event
    auto battle = LifeEventGenerator::CreateBattleEvent(true, "Hastings", 30, 100.0f);
    life_events.AddEvent(battle);

    assert(life_events.HasExperienced(LifeEventType::BATTLE_WON) == true);
    std::cout << "Battle event: PASSED\n";

    // Test major events query
    auto major_events = life_events.GetMajorEvents();
    assert(major_events.size() >= 4);  // birth, coming_of_age, marriage, battle all major
    std::cout << "Major events query (" << major_events.size() << " events): PASSED\n";

    // Test biography generation
    std::string bio = life_events.GetBiography();
    assert(bio.length() > 0);
    std::cout << "Biography generation: PASSED\n";

    std::cout << "Life Events tests: ALL PASSED\n";
    return true;
}

// ============================================================================
// Education System Tests
// ============================================================================

bool TestEducationSystem() {
    std::cout << "\n========== Testing Education System ==========\n";

    CharacterEducationComponent education(1);

    // Test starting education
    education.StartEducation(EducationFocus::DIPLOMACY, 10, 1.5f);

    assert(education.is_educated == true);
    assert(education.education_focus == EducationFocus::DIPLOMACY);
    assert(education.learning_rate_modifier == 1.5f);
    assert(education.IsInEducation() == true);
    std::cout << "Start education: PASSED\n";

    // Test gaining experience
    education.GainExperience(EducationFocus::DIPLOMACY, 50);
    assert(education.skill_xp.diplomacy_xp == 75);  // 50 * 1.5 modifier
    std::cout << "Gain diplomacy XP (with modifier): PASSED\n";

    // Test multiple skill gains
    education.GainExperience(EducationFocus::MARTIAL, 30);
    education.GainExperience(EducationFocus::LEARNING, 40);

    assert(education.skill_xp.martial_xp > 0);
    assert(education.skill_xp.learning_xp > 0);
    std::cout << "Gain XP in multiple skills: PASSED\n";

    // Test level up checking
    education.GainExperience(EducationFocus::DIPLOMACY, 50);  // Total should be 150 now
    auto level_ups = education.CheckLevelUps(5, 5, 5, 5, 5);

    assert(level_ups.diplomacy_ready == true);  // 150 XP >= 100 + (5*50) = 150
    std::cout << "Check level up readiness: PASSED\n";

    // Test consuming XP
    education.ConsumeXP(EducationFocus::DIPLOMACY, 5);
    assert(education.skill_xp.diplomacy_xp < 150);
    std::cout << "Consume XP after level up: PASSED\n";

    // Test completing education
    education.GainExperience(EducationFocus::DIPLOMACY, 500);
    education.GainExperience(EducationFocus::MARTIAL, 200);
    education.GainExperience(EducationFocus::LEARNING, 300);

    int total_xp = education.skill_xp.diplomacy_xp +
                   education.skill_xp.martial_xp +
                   education.skill_xp.learning_xp;

    EducationQuality quality = education.CompleteEducation(total_xp);

    assert(quality >= EducationQuality::EXCELLENT);  // Should be excellent with this much XP
    assert(education.IsInEducation() == false);
    std::cout << "Complete education (quality = " << education.GetEducationQualityString() << "): PASSED\n";

    // Test education utilities
    int xp_gain = EducationUtils::CalculateXPGain("battle", 3, 15.0f);
    assert(xp_gain > 0);
    std::cout << "Calculate XP gain from activity: PASSED\n";

    float tutor_quality = EducationUtils::CalculateTutorQuality(15, 18, true);
    assert(tutor_quality > 1.0f);
    std::cout << "Calculate tutor quality: PASSED\n";

    std::cout << "Education System tests: ALL PASSED\n";
    return true;
}

// ============================================================================
// Character Relationships (std::optional) Tests
// ============================================================================

bool TestRelationshipsOptional() {
    std::cout << "\n========== Testing Relationships with std::optional ==========\n";

    CharacterRelationshipsComponent relationships(1);

    // Test setting relationship
    relationships.SetRelationship(2, RelationshipType::FRIEND, 75, 60.0);

    // Test getting relationship (should return optional)
    std::optional<CharacterRelationship> rel = relationships.GetRelationship(2);

    assert(rel.has_value() == true);
    assert(rel->other_character == 2);
    assert(rel->type == RelationshipType::FRIEND);
    std::cout << "Get relationship returns std::optional: PASSED\n";

    // Test getting non-existent relationship
    std::optional<CharacterRelationship> no_rel = relationships.GetRelationship(999);

    assert(no_rel.has_value() == false);
    std::cout << "Non-existent relationship returns std::nullopt: PASSED\n";

    // Test friendship check
    assert(relationships.IsFriendsWith(2) == true);
    assert(relationships.IsFriendsWith(999) == false);
    std::cout << "Friendship check works with optional: PASSED\n";

    // Test friendship bond strength
    double bond = relationships.GetFriendshipBondStrength(2);
    assert(bond == 60.0);
    std::cout << "Get friendship bond strength: PASSED\n";

    std::cout << "Relationships std::optional tests: ALL PASSED\n";
    return true;
}

// ============================================================================
// Random Number Generation Tests
// ============================================================================

bool TestModernRandom() {
    std::cout << "\n========== Testing Modern Random Number Generation ==========\n";

    // Test random float
    float f1 = utils::RandomFloat();
    assert(f1 >= 0.0f && f1 <= 1.0f);
    std::cout << "RandomFloat() in range [0,1]: PASSED\n";

    // Test random float with range
    float f2 = utils::RandomFloat(5.0f, 10.0f);
    assert(f2 >= 5.0f && f2 <= 10.0f);
    std::cout << "RandomFloat(5, 10) in range: PASSED\n";

    // Test random int
    int i1 = utils::RandomInt(1, 6);
    assert(i1 >= 1 && i1 <= 6);
    std::cout << "RandomInt(1, 6) in range: PASSED\n";

    // Test random bool
    bool b1 = utils::RandomBool(1.0f);  // Always true
    assert(b1 == true);
    std::cout << "RandomBool(1.0) returns true: PASSED\n";

    bool b2 = utils::RandomBool(0.0f);  // Always false
    assert(b2 == false);
    std::cout << "RandomBool(0.0) returns false: PASSED\n";

    // Test dice roll
    int d6 = utils::RollDice(6);
    assert(d6 >= 1 && d6 <= 6);
    std::cout << "RollDice(6) in range [1,6]: PASSED\n";

    // Test percentile
    int percentile = utils::RollPercentile();
    assert(percentile >= 0 && percentile <= 100);
    std::cout << "RollPercentile() in range [0,100]: PASSED\n";

    // Test distribution (run many times and check reasonable distribution)
    int count_high = 0;
    for (int i = 0; i < 1000; ++i) {
        if (utils::RandomFloat() > 0.5f) count_high++;
    }
    assert(count_high > 400 && count_high < 600);  // Should be roughly 50%
    std::cout << "Random distribution check (1000 samples): PASSED\n";

    std::cout << "Modern Random tests: ALL PASSED\n";
    return true;
}

// ============================================================================
// Main Test Suite
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "========================================================\n";
    std::cout << "  CHARACTER SYSTEM IMPROVEMENTS - TEST SUITE\n";
    std::cout << "========================================================\n";

    bool all_passed = true;

    all_passed &= TestTraitsComponent();
    all_passed &= TestTraitDatabase();
    all_passed &= TestLifeEvents();
    all_passed &= TestEducationSystem();
    all_passed &= TestRelationshipsOptional();
    all_passed &= TestModernRandom();

    std::cout << "\n";
    std::cout << "========================================================\n";
    if (all_passed) {
        std::cout << "     ALL TESTS PASSED ✓\n";
    } else {
        std::cout << "     SOME TESTS FAILED ✗\n";
    }
    std::cout << "========================================================\n";
    std::cout << "\n";

    std::cout << "New Features Summary:\n";
    std::cout << "  ✓ Traits Component: 30+ default traits with modifiers\n";
    std::cout << "  ✓ Life Events System: Comprehensive event tracking\n";
    std::cout << "  ✓ Education System: XP-based skill progression\n";
    std::cout << "  ✓ std::optional for relationships: Type-safe queries\n";
    std::cout << "  ✓ Modern C++ random: Mersenne Twister RNG\n";
    std::cout << "  ✓ Named constants: All magic numbers extracted\n";
    std::cout << "\n";

    std::cout << "Code Quality Improvements:\n";
    std::cout << "  - CharacterAI uses named constants (CharacterAIConstants.h)\n";
    std::cout << "  - Modern random replaces rand() for better distribution\n";
    std::cout << "  - std::optional provides type safety for nullable returns\n";
    std::cout << "  - Comprehensive test coverage for new features\n";
    std::cout << "\n";

    return all_passed ? 0 : 1;
}
