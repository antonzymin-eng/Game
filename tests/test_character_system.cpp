// Created: December 5, 2025
// Location: tests/test_character_system.cpp
// Purpose: Unit tests for CharacterSystem core functionality

#include "game/systems/CharacterSystem.h"
#include "game/character/CharacterTypes.h"
#include "core/ECS/ComponentAccessManager.h"
#include "core/ECS/EntityManager.h"
#include "core/threading/ThreadSafeMessageBus.h"
#include "core/logging/Logger.h"

#include <iostream>
#include <cassert>
#include <memory>

using namespace game::character;

// ============================================================================
// Test Fixtures - Setup and Teardown
// ============================================================================

struct CharacterSystemTestFixture {
    std::unique_ptr<core::ecs::EntityManager> entity_manager;
    std::unique_ptr<core::ecs::ComponentAccessManager> component_access;
    std::unique_ptr<core::threading::ThreadSafeMessageBus> message_bus;
    std::unique_ptr<CharacterSystem> character_system;

    CharacterSystemTestFixture() {
        // Initialize ECS infrastructure
        entity_manager = std::make_unique<core::ecs::EntityManager>();
        component_access = std::make_unique<core::ecs::ComponentAccessManager>(*entity_manager);
        message_bus = std::make_unique<core::threading::ThreadSafeMessageBus>();

        // Register required components
        entity_manager->RegisterComponent<game::components::CharacterComponent>();
        entity_manager->RegisterComponent<game::components::TraitsComponent>();
        entity_manager->RegisterComponent<game::character::CharacterRelationshipsComponent>();
        entity_manager->RegisterComponent<game::character::CharacterEducationComponent>();
        entity_manager->RegisterComponent<game::character::CharacterLifeEventsComponent>();
        entity_manager->RegisterComponent<game::components::NobleArtsComponent>();

        // Create CharacterSystem
        character_system = std::make_unique<CharacterSystem>(*component_access, *message_bus);
    }

    ~CharacterSystemTestFixture() {
        // Cleanup in reverse order
        character_system.reset();
        message_bus.reset();
        component_access.reset();
        entity_manager.reset();
    }
};

// ============================================================================
// Input Validation Tests
// ============================================================================

bool TestCreateCharacter_EmptyName() {
    std::cout << "\n========== Testing CreateCharacter - Empty Name ==========\n";

    CharacterSystemTestFixture fixture;
    CharacterStats stats = CharacterStats::CreateRandom();

    // Attempt to create character with empty name
    core::ecs::EntityID id = fixture.character_system->CreateCharacter("", 25, stats);

    // Should return invalid EntityID
    assert(!id.IsValid());
    std::cout << "Empty name rejected: PASSED\n";

    return true;
}

bool TestCreateCharacter_NameTooLong() {
    std::cout << "\n========== Testing CreateCharacter - Name Too Long ==========\n";

    CharacterSystemTestFixture fixture;
    CharacterStats stats = CharacterStats::CreateRandom();

    // Create name longer than 64 characters
    std::string long_name(65, 'A');

    core::ecs::EntityID id = fixture.character_system->CreateCharacter(long_name, 25, stats);

    // Should return invalid EntityID
    assert(!id.IsValid());
    std::cout << "Name too long (65 chars) rejected: PASSED\n";

    return true;
}

bool TestCreateCharacter_InvalidAge() {
    std::cout << "\n========== Testing CreateCharacter - Invalid Age ==========\n";

    CharacterSystemTestFixture fixture;
    CharacterStats stats = CharacterStats::CreateRandom();

    // Attempt to create character with age > 120
    core::ecs::EntityID id = fixture.character_system->CreateCharacter("Old Person", 121, stats);

    // Should return invalid EntityID
    assert(!id.IsValid());
    std::cout << "Age > 120 rejected: PASSED\n";

    return true;
}

bool TestCreateCharacter_InvalidStats() {
    std::cout << "\n========== Testing CreateCharacter - Invalid Stats ==========\n";

    CharacterSystemTestFixture fixture;

    // Create stats with out-of-range values
    CharacterStats invalid_stats;
    invalid_stats.diplomacy = 25;  // Max is 20
    invalid_stats.martial = 10;
    invalid_stats.stewardship = 10;
    invalid_stats.intrigue = 10;
    invalid_stats.learning = 10;
    invalid_stats.health = 100.0f;

    core::ecs::EntityID id = fixture.character_system->CreateCharacter("Invalid", 25, invalid_stats);

    // Should return invalid EntityID
    assert(!id.IsValid());
    std::cout << "Stats out of range (diplomacy=25) rejected: PASSED\n";

    return true;
}

bool TestCreateCharacter_InvalidHealth() {
    std::cout << "\n========== Testing CreateCharacter - Invalid Health ==========\n";

    CharacterSystemTestFixture fixture;
    CharacterStats stats = CharacterStats::CreateRandom();
    stats.health = 150.0f;  // Health should be 0-100

    core::ecs::EntityID id = fixture.character_system->CreateCharacter("Unhealthy", 25, stats);

    // Should return invalid EntityID
    assert(!id.IsValid());
    std::cout << "Health out of range (150.0) rejected: PASSED\n";

    return true;
}

// ============================================================================
// Character Creation Tests
// ============================================================================

bool TestCreateCharacter_ValidInput() {
    std::cout << "\n========== Testing CreateCharacter - Valid Input ==========\n";

    CharacterSystemTestFixture fixture;
    CharacterStats stats = CharacterStats::CreateRandom();

    // Create character with valid input
    core::ecs::EntityID id = fixture.character_system->CreateCharacter("William the Conqueror", 35, stats);

    // Should return valid EntityID
    assert(id.IsValid());
    std::cout << "Valid character created: PASSED (ID=" << id.id << ", version=" << id.version << ")\n";

    // Verify character count
    assert(fixture.character_system->GetCharacterCount() == 1);
    std::cout << "Character count = 1: PASSED\n";

    return true;
}

bool TestCreateCharacter_Components() {
    std::cout << "\n========== Testing CreateCharacter - Components Created ==========\n";

    CharacterSystemTestFixture fixture;
    CharacterStats stats;
    stats.diplomacy = 15;
    stats.martial = 12;
    stats.stewardship = 10;
    stats.intrigue = 8;
    stats.learning = 7;
    stats.health = 95.0f;
    stats.prestige = 100.0f;
    stats.gold = 500.0f;

    core::ecs::EntityID id = fixture.character_system->CreateCharacter("Test Character", 30, stats);
    assert(id.IsValid());

    // Verify CharacterComponent was created with correct values
    auto char_comp = fixture.entity_manager->GetComponent<game::components::CharacterComponent>(id);
    assert(char_comp != nullptr);
    assert(char_comp->GetName() == "Test Character");
    assert(char_comp->GetAge() == 30);
    assert(char_comp->GetDiplomacy() == 15);
    assert(char_comp->GetMartial() == 12);
    std::cout << "CharacterComponent created with correct values: PASSED\n";

    // Verify TraitsComponent exists
    auto traits_comp = fixture.entity_manager->GetComponent<game::components::TraitsComponent>(id);
    assert(traits_comp != nullptr);
    std::cout << "TraitsComponent created: PASSED\n";

    // Verify CharacterRelationshipsComponent exists
    auto rel_comp = fixture.entity_manager->GetComponent<game::character::CharacterRelationshipsComponent>(id);
    assert(rel_comp != nullptr);
    std::cout << "CharacterRelationshipsComponent created: PASSED\n";

    // Verify CharacterEducationComponent exists
    auto edu_comp = fixture.entity_manager->GetComponent<game::character::CharacterEducationComponent>(id);
    assert(edu_comp != nullptr);
    std::cout << "CharacterEducationComponent created: PASSED\n";

    // Verify CharacterLifeEventsComponent exists
    auto events_comp = fixture.entity_manager->GetComponent<game::character::CharacterLifeEventsComponent>(id);
    assert(events_comp != nullptr);
    std::cout << "CharacterLifeEventsComponent created: PASSED\n";

    // Verify NobleArtsComponent exists
    auto arts_comp = fixture.entity_manager->GetComponent<game::components::NobleArtsComponent>(id);
    assert(arts_comp != nullptr);
    std::cout << "NobleArtsComponent created: PASSED\n";

    return true;
}

bool TestCreateCharacter_MultipleCharacters() {
    std::cout << "\n========== Testing CreateCharacter - Multiple Characters ==========\n";

    CharacterSystemTestFixture fixture;

    // Create multiple characters
    std::vector<core::ecs::EntityID> character_ids;

    for (int i = 0; i < 10; ++i) {
        CharacterStats stats = CharacterStats::CreateRandom();
        std::string name = "Character " + std::to_string(i);

        core::ecs::EntityID id = fixture.character_system->CreateCharacter(name, 20 + i, stats);
        assert(id.IsValid());
        character_ids.push_back(id);
    }

    // Verify all characters were created
    assert(fixture.character_system->GetCharacterCount() == 10);
    std::cout << "Created 10 characters: PASSED\n";

    // Verify all IDs are unique
    for (size_t i = 0; i < character_ids.size(); ++i) {
        for (size_t j = i + 1; j < character_ids.size(); ++j) {
            assert(character_ids[i].id != character_ids[j].id);
        }
    }
    std::cout << "All character IDs are unique: PASSED\n";

    return true;
}

// ============================================================================
// Character Query Tests
// ============================================================================

bool TestGetCharacterByName() {
    std::cout << "\n========== Testing GetCharacterByName ==========\n";

    CharacterSystemTestFixture fixture;
    CharacterStats stats = CharacterStats::CreateRandom();

    // Create test characters
    core::ecs::EntityID william = fixture.character_system->CreateCharacter("William", 35, stats);
    core::ecs::EntityID harold = fixture.character_system->CreateCharacter("Harold", 42, stats);

    // Test finding existing character
    core::ecs::EntityID found = fixture.character_system->GetCharacterByName("William");
    assert(found.IsValid());
    assert(found.id == william.id);
    std::cout << "Found character 'William': PASSED\n";

    // Test finding another character
    found = fixture.character_system->GetCharacterByName("Harold");
    assert(found.IsValid());
    assert(found.id == harold.id);
    std::cout << "Found character 'Harold': PASSED\n";

    // Test not finding non-existent character
    found = fixture.character_system->GetCharacterByName("Nonexistent");
    assert(!found.IsValid());
    std::cout << "Non-existent character returns invalid ID: PASSED\n";

    return true;
}

bool TestGetAllCharacters() {
    std::cout << "\n========== Testing GetAllCharacters ==========\n";

    CharacterSystemTestFixture fixture;
    CharacterStats stats = CharacterStats::CreateRandom();

    // Initially empty
    const auto& characters = fixture.character_system->GetAllCharacters();
    assert(characters.empty());
    std::cout << "Initially empty: PASSED\n";

    // Create 5 characters
    std::vector<core::ecs::EntityID> created_ids;
    for (int i = 0; i < 5; ++i) {
        core::ecs::EntityID id = fixture.character_system->CreateCharacter(
            "Char" + std::to_string(i), 20 + i, stats);
        created_ids.push_back(id);
    }

    // Get all characters
    const auto& all_chars = fixture.character_system->GetAllCharacters();
    assert(all_chars.size() == 5);
    std::cout << "All 5 characters returned: PASSED\n";

    // Verify all created IDs are in the returned list
    for (const auto& created_id : created_ids) {
        bool found = false;
        for (const auto& char_id : all_chars) {
            if (char_id.id == created_id.id) {
                found = true;
                break;
            }
        }
        assert(found);
    }
    std::cout << "All created characters found in list: PASSED\n";

    return true;
}

bool TestGetCharactersByRealm() {
    std::cout << "\n========== Testing GetCharactersByRealm ==========\n";

    CharacterSystemTestFixture fixture;
    CharacterStats stats = CharacterStats::CreateRandom();

    // Create characters
    core::ecs::EntityID char1 = fixture.character_system->CreateCharacter("Ruler1", 40, stats);
    core::ecs::EntityID char2 = fixture.character_system->CreateCharacter("Ruler2", 35, stats);
    core::ecs::EntityID char3 = fixture.character_system->CreateCharacter("Ruler3", 30, stats);

    // Create mock realm IDs (using EntityID struct)
    core::ecs::EntityID realm1{1, 1};
    core::ecs::EntityID realm2{2, 1};

    // Assign characters to realms via CharacterComponent
    auto comp1 = fixture.entity_manager->GetComponent<game::components::CharacterComponent>(char1);
    comp1->SetPrimaryTitle(1);  // Legacy realm ID

    auto comp2 = fixture.entity_manager->GetComponent<game::components::CharacterComponent>(char2);
    comp2->SetPrimaryTitle(1);  // Same realm

    auto comp3 = fixture.entity_manager->GetComponent<game::components::CharacterComponent>(char3);
    comp3->SetPrimaryTitle(2);  // Different realm

    // Test getting characters for realm 1
    auto realm1_chars = fixture.character_system->GetCharactersByRealm(realm1);
    assert(realm1_chars.size() == 2);
    std::cout << "Realm 1 has 2 characters: PASSED\n";

    // Test getting characters for realm 2
    auto realm2_chars = fixture.character_system->GetCharactersByRealm(realm2);
    assert(realm2_chars.size() == 1);
    std::cout << "Realm 2 has 1 character: PASSED\n";

    // Test getting characters for non-existent realm
    core::ecs::EntityID realm99{99, 1};
    auto realm99_chars = fixture.character_system->GetCharactersByRealm(realm99);
    assert(realm99_chars.empty());
    std::cout << "Non-existent realm returns empty list: PASSED\n";

    return true;
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════╗\n";
    std::cout << "║       Character System Unit Tests                     ║\n";
    std::cout << "╚════════════════════════════════════════════════════════╝\n";

    bool all_passed = true;

    // Input validation tests
    all_passed &= TestCreateCharacter_EmptyName();
    all_passed &= TestCreateCharacter_NameTooLong();
    all_passed &= TestCreateCharacter_InvalidAge();
    all_passed &= TestCreateCharacter_InvalidStats();
    all_passed &= TestCreateCharacter_InvalidHealth();

    // Character creation tests
    all_passed &= TestCreateCharacter_ValidInput();
    all_passed &= TestCreateCharacter_Components();
    all_passed &= TestCreateCharacter_MultipleCharacters();

    // Character query tests
    all_passed &= TestGetCharacterByName();
    all_passed &= TestGetAllCharacters();
    all_passed &= TestGetCharactersByRealm();

    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════╗\n";
    if (all_passed) {
        std::cout << "║  ✓ ALL TESTS PASSED                                   ║\n";
    } else {
        std::cout << "║  ✗ SOME TESTS FAILED                                  ║\n";
    }
    std::cout << "╚════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";

    return all_passed ? 0 : 1;
}
