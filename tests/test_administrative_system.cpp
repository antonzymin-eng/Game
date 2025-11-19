// ============================================================================
// test_administrative_system.cpp - Comprehensive Administrative System Tests
// Created: 2025-11-19 - Administrative System Validation Testing
// Location: tests/test_administrative_system.cpp
// ============================================================================

#include "game/administration/AdministrativeSystem.h"
#include "game/administration/AdministrativeComponents.h"
#include "core/ECS/EntityManager.h"
#include "core/ECS/ComponentAccessManager.h"
#include "core/threading/ThreadSafeMessageBus.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <vector>

using namespace game::administration;

// Global test fixtures
std::unique_ptr<::core::ecs::EntityManager> g_entity_manager;
std::unique_ptr<::core::ecs::ComponentAccessManager> g_access_manager;
std::unique_ptr<::core::threading::ThreadSafeMessageBus> g_message_bus;
std::unique_ptr<AdministrativeSystem> g_admin_system;

void SetupTestEnvironment() {
    g_entity_manager = std::make_unique<::core::ecs::EntityManager>();
    g_access_manager = std::make_unique<::core::ecs::ComponentAccessManager>(*g_entity_manager);
    g_message_bus = std::make_unique<::core::threading::ThreadSafeMessageBus>();
    g_admin_system = std::make_unique<AdministrativeSystem>(*g_access_manager, *g_message_bus);
    g_admin_system->Initialize();
}

void TeardownTestEnvironment() {
    if (g_admin_system) g_admin_system->Shutdown();
    g_admin_system.reset();
    g_message_bus.reset();
    g_access_manager.reset();
    g_entity_manager.reset();
}

// ============================================================================
// Basic Functionality Tests
// ============================================================================

void TestSystemInitialization() {
    std::cout << "[TEST] System Initialization" << std::endl;

    SetupTestEnvironment();

    // System should be initialized
    assert(g_admin_system != nullptr);
    std::cout << "  ✅ System initialized successfully" << std::endl;

    TeardownTestEnvironment();
}

void TestCreateAdministrativeComponents() {
    std::cout << "[TEST] Create Administrative Components" << std::endl;

    SetupTestEnvironment();

    // Create test entity
    auto entity = g_entity_manager->CreateEntity();
    game::types::EntityID entity_id = static_cast<game::types::EntityID>(entity.GetID());

    // Create administrative components
    g_admin_system->CreateAdministrativeComponents(entity_id);

    // Verify components exist
    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto governance = g_entity_manager->GetComponent<GovernanceComponent>(entity_handle);
    auto bureaucracy = g_entity_manager->GetComponent<BureaucracyComponent>(entity_handle);
    auto law = g_entity_manager->GetComponent<LawComponent>(entity_handle);

    assert(governance != nullptr);
    assert(bureaucracy != nullptr);
    assert(law != nullptr);
    assert(governance->governance_type == GovernanceType::FEUDAL);
    assert(governance->administrative_efficiency == 0.5);

    std::cout << "  ✅ Components created successfully" << std::endl;

    TeardownTestEnvironment();
}

// ============================================================================
// Official Management Tests
// ============================================================================

void TestAppointOfficial() {
    std::cout << "[TEST] Appoint Official" << std::endl;

    SetupTestEnvironment();

    auto entity = g_entity_manager->CreateEntity();
    game::types::EntityID entity_id = static_cast<game::types::EntityID>(entity.GetID());
    g_admin_system->CreateAdministrativeComponents(entity_id);

    // Appoint tax collector
    bool result = g_admin_system->AppointOfficial(entity_id, OfficialType::TAX_COLLECTOR, "Marcus Aurelius");
    assert(result == true);

    // Verify official was added
    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto governance = g_entity_manager->GetComponent<GovernanceComponent>(entity_handle);
    assert(governance->appointed_officials.size() == 1);
    assert(governance->appointed_officials[0].name == "Marcus Aurelius");
    assert(governance->appointed_officials[0].type == OfficialType::TAX_COLLECTOR);

    std::cout << "  ✅ Official appointed successfully" << std::endl;

    TeardownTestEnvironment();
}

void TestDismissOfficial() {
    std::cout << "[TEST] Dismiss Official" << std::endl;

    SetupTestEnvironment();

    auto entity = g_entity_manager->CreateEntity();
    game::types::EntityID entity_id = static_cast<game::types::EntityID>(entity.GetID());
    g_admin_system->CreateAdministrativeComponents(entity_id);

    // Appoint and then dismiss
    g_admin_system->AppointOfficial(entity_id, OfficialType::TAX_COLLECTOR, "Marcus Aurelius");

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto governance = g_entity_manager->GetComponent<GovernanceComponent>(entity_handle);
    uint32_t official_id = governance->appointed_officials[0].official_id;

    bool result = g_admin_system->DismissOfficial(entity_id, official_id);
    assert(result == true);
    assert(governance->appointed_officials.size() == 0);

    std::cout << "  ✅ Official dismissed successfully" << std::endl;

    TeardownTestEnvironment();
}

void TestAppointMultipleOfficials() {
    std::cout << "[TEST] Appoint Multiple Officials" << std::endl;

    SetupTestEnvironment();

    auto entity = g_entity_manager->CreateEntity();
    game::types::EntityID entity_id = static_cast<game::types::EntityID>(entity.GetID());
    g_admin_system->CreateAdministrativeComponents(entity_id);

    // Appoint 4 different types
    g_admin_system->AppointOfficial(entity_id, OfficialType::TAX_COLLECTOR, "Marcus");
    g_admin_system->AppointOfficial(entity_id, OfficialType::TRADE_MINISTER, "Julius");
    g_admin_system->AppointOfficial(entity_id, OfficialType::MILITARY_GOVERNOR, "Aurelius");
    g_admin_system->AppointOfficial(entity_id, OfficialType::COURT_ADVISOR, "Constantine");

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto governance = g_entity_manager->GetComponent<GovernanceComponent>(entity_handle);
    assert(governance->appointed_officials.size() == 4);
    assert(governance->monthly_administrative_costs > 0.0);

    std::cout << "  ✅ Multiple officials appointed successfully" << std::endl;

    TeardownTestEnvironment();
}

// ============================================================================
// Efficiency Calculation Tests
// ============================================================================

void TestGetAdministrativeEfficiency() {
    std::cout << "[TEST] Get Administrative Efficiency" << std::endl;

    SetupTestEnvironment();

    auto entity = g_entity_manager->CreateEntity();
    game::types::EntityID entity_id = static_cast<game::types::EntityID>(entity.GetID());
    g_admin_system->CreateAdministrativeComponents(entity_id);

    double efficiency = g_admin_system->GetAdministrativeEfficiency(entity_id);
    assert(efficiency == 0.5); // Default base efficiency

    std::cout << "  ✅ Efficiency retrieved successfully" << std::endl;

    TeardownTestEnvironment();
}

void TestTaxCollectionRate() {
    std::cout << "[TEST] Tax Collection Rate" << std::endl;

    SetupTestEnvironment();

    auto entity = g_entity_manager->CreateEntity();
    game::types::EntityID entity_id = static_cast<game::types::EntityID>(entity.GetID());
    g_admin_system->CreateAdministrativeComponents(entity_id);

    double tax_rate = g_admin_system->GetTaxCollectionRate(entity_id);
    assert(tax_rate > 0.0 && tax_rate <= 1.0);

    std::cout << "  ✅ Tax collection rate valid" << std::endl;

    TeardownTestEnvironment();
}

// ============================================================================
// Governance Operations Tests
// ============================================================================

void TestUpdateGovernanceType() {
    std::cout << "[TEST] Update Governance Type" << std::endl;

    SetupTestEnvironment();

    auto entity = g_entity_manager->CreateEntity();
    game::types::EntityID entity_id = static_cast<game::types::EntityID>(entity.GetID());
    g_admin_system->CreateAdministrativeComponents(entity_id);

    g_admin_system->UpdateGovernanceType(entity_id, GovernanceType::CENTRALIZED);

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto governance = g_entity_manager->GetComponent<GovernanceComponent>(entity_handle);
    assert(governance->governance_type == GovernanceType::CENTRALIZED);

    std::cout << "  ✅ Governance type updated successfully" << std::endl;

    TeardownTestEnvironment();
}

void TestProcessAdministrativeReforms() {
    std::cout << "[TEST] Process Administrative Reforms" << std::endl;

    SetupTestEnvironment();

    auto entity = g_entity_manager->CreateEntity();
    game::types::EntityID entity_id = static_cast<game::types::EntityID>(entity.GetID());
    g_admin_system->CreateAdministrativeComponents(entity_id);

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto governance = g_entity_manager->GetComponent<GovernanceComponent>(entity_handle);
    double initial_efficiency = governance->administrative_efficiency;

    g_admin_system->ProcessAdministrativeReforms(entity_id);

    // Efficiency should increase after reform
    assert(governance->administrative_efficiency > initial_efficiency);

    std::cout << "  ✅ Reforms processed successfully" << std::endl;

    TeardownTestEnvironment();
}

// ============================================================================
// Bureaucracy Operations Tests
// ============================================================================

void TestExpandBureaucracy() {
    std::cout << "[TEST] Expand Bureaucracy" << std::endl;

    SetupTestEnvironment();

    auto entity = g_entity_manager->CreateEntity();
    game::types::EntityID entity_id = static_cast<game::types::EntityID>(entity.GetID());
    g_admin_system->CreateAdministrativeComponents(entity_id);

    g_admin_system->ExpandBureaucracy(entity_id, 10);

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto bureaucracy = g_entity_manager->GetComponent<BureaucracyComponent>(entity_handle);
    assert(bureaucracy->clerks_employed == 13); // 3 initial + 10

    std::cout << "  ✅ Bureaucracy expanded successfully" << std::endl;

    TeardownTestEnvironment();
}

void TestImproveRecordKeeping() {
    std::cout << "[TEST] Improve Record Keeping" << std::endl;

    SetupTestEnvironment();

    auto entity = g_entity_manager->CreateEntity();
    game::types::EntityID entity_id = static_cast<game::types::EntityID>(entity.GetID());
    g_admin_system->CreateAdministrativeComponents(entity_id);

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto bureaucracy = g_entity_manager->GetComponent<BureaucracyComponent>(entity_handle);
    double initial_quality = bureaucracy->record_keeping_quality;

    g_admin_system->ImproveRecordKeeping(entity_id, 1000.0);

    assert(bureaucracy->record_keeping_quality > initial_quality);

    std::cout << "  ✅ Record keeping improved successfully" << std::endl;

    TeardownTestEnvironment();
}

// ============================================================================
// Law System Tests
// ============================================================================

void TestEstablishCourt() {
    std::cout << "[TEST] Establish Court" << std::endl;

    SetupTestEnvironment();

    auto entity = g_entity_manager->CreateEntity();
    game::types::EntityID entity_id = static_cast<game::types::EntityID>(entity.GetID());
    g_admin_system->CreateAdministrativeComponents(entity_id);

    g_admin_system->EstablishCourt(entity_id);

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto law = g_entity_manager->GetComponent<LawComponent>(entity_handle);
    assert(law->courts_established == 2); // 1 initial + 1

    std::cout << "  ✅ Court established successfully" << std::endl;

    TeardownTestEnvironment();
}

void TestAppointJudge() {
    std::cout << "[TEST] Appoint Judge" << std::endl;

    SetupTestEnvironment();

    auto entity = g_entity_manager->CreateEntity();
    game::types::EntityID entity_id = static_cast<game::types::EntityID>(entity.GetID());
    g_admin_system->CreateAdministrativeComponents(entity_id);

    g_admin_system->AppointJudge(entity_id, "Judge Dredd");

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto law = g_entity_manager->GetComponent<LawComponent>(entity_handle);
    assert(law->judges_appointed == 3); // 2 initial + 1

    std::cout << "  ✅ Judge appointed successfully" << std::endl;

    TeardownTestEnvironment();
}

void TestEnactLaw() {
    std::cout << "[TEST] Enact Law" << std::endl;

    SetupTestEnvironment();

    auto entity = g_entity_manager->CreateEntity();
    game::types::EntityID entity_id = static_cast<game::types::EntityID>(entity.GetID());
    g_admin_system->CreateAdministrativeComponents(entity_id);

    g_admin_system->EnactLaw(entity_id, "Tax Reform Act");

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto law = g_entity_manager->GetComponent<LawComponent>(entity_handle);
    assert(law->active_laws.size() == 1);
    assert(law->active_laws[0] == "Tax Reform Act");

    std::cout << "  ✅ Law enacted successfully" << std::endl;

    TeardownTestEnvironment();
}

// ============================================================================
// Serialization Tests
// ============================================================================

void TestSerialization() {
    std::cout << "[TEST] Serialization" << std::endl;

    SetupTestEnvironment();

    // Serialize
    Json::Value serialized = g_admin_system->Serialize(1);
    assert(serialized["system_name"].asString() == "AdministrativeSystem");
    assert(serialized["version"].asInt() == 1);
    assert(serialized.isMember("config"));
    assert(serialized["config"].isMember("base_efficiency"));

    std::cout << "  ✅ Serialization works correctly" << std::endl;

    TeardownTestEnvironment();
}

void TestDeserialization() {
    std::cout << "[TEST] Deserialization" << std::endl;

    SetupTestEnvironment();

    // Create test data
    Json::Value data;
    data["system_name"] = "AdministrativeSystem";
    data["version"] = 1;
    data["initialized"] = true;
    data["config"]["base_efficiency"] = 0.8;

    // Deserialize
    bool result = g_admin_system->Deserialize(data, 1);
    assert(result == true);
    assert(g_admin_system->GetConfiguration().base_efficiency == 0.8);

    std::cout << "  ✅ Deserialization works correctly" << std::endl;

    TeardownTestEnvironment();
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

void TestConcurrentAppointments() {
    std::cout << "[TEST] Concurrent Appointments (Thread Safety)" << std::endl;

    SetupTestEnvironment();

    auto entity = g_entity_manager->CreateEntity();
    game::types::EntityID entity_id = static_cast<game::types::EntityID>(entity.GetID());
    g_admin_system->CreateAdministrativeComponents(entity_id);

    const int num_threads = 10;
    std::vector<std::thread> threads;

    // Launch concurrent appointment threads
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([entity_id, i]() {
            std::string name = "Official_" + std::to_string(i);
            g_admin_system->AppointOfficial(entity_id, OfficialType::TAX_COLLECTOR, name);
        });
    }

    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }

    // Verify all officials were added
    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto governance = g_entity_manager->GetComponent<GovernanceComponent>(entity_handle);
    assert(governance->appointed_officials.size() == num_threads);

    std::cout << "  ✅ Concurrent appointments safe (no data races)" << std::endl;

    TeardownTestEnvironment();
}

void TestConcurrentDismissals() {
    std::cout << "[TEST] Concurrent Dismissals (Thread Safety)" << std::endl;

    SetupTestEnvironment();

    auto entity = g_entity_manager->CreateEntity();
    game::types::EntityID entity_id = static_cast<game::types::EntityID>(entity.GetID());
    g_admin_system->CreateAdministrativeComponents(entity_id);

    // Appoint 20 officials
    std::vector<uint32_t> official_ids;
    for (int i = 0; i < 20; ++i) {
        g_admin_system->AppointOfficial(entity_id, OfficialType::TAX_COLLECTOR, "Official_" + std::to_string(i));
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        auto governance = g_entity_manager->GetComponent<GovernanceComponent>(entity_handle);
        official_ids.push_back(governance->appointed_officials.back().official_id);
    }

    // Dismiss them concurrently
    std::vector<std::thread> threads;
    for (uint32_t id : official_ids) {
        threads.emplace_back([entity_id, id]() {
            g_admin_system->DismissOfficial(entity_id, id);
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // All should be dismissed
    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
    auto governance = g_entity_manager->GetComponent<GovernanceComponent>(entity_handle);
    assert(governance->appointed_officials.size() == 0);

    std::cout << "  ✅ Concurrent dismissals safe (no crashes)" << std::endl;

    TeardownTestEnvironment();
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Administrative System Test Suite" << std::endl;
    std::cout << "========================================\n" << std::endl;

    try {
        // Basic functionality
        TestSystemInitialization();
        TestCreateAdministrativeComponents();

        // Official management
        TestAppointOfficial();
        TestDismissOfficial();
        TestAppointMultipleOfficials();

        // Efficiency calculations
        TestGetAdministrativeEfficiency();
        TestTaxCollectionRate();

        // Governance operations
        TestUpdateGovernanceType();
        TestProcessAdministrativeReforms();

        // Bureaucracy operations
        TestExpandBureaucracy();
        TestImproveRecordKeeping();

        // Law system
        TestEstablishCourt();
        TestAppointJudge();
        TestEnactLaw();

        // Serialization
        TestSerialization();
        TestDeserialization();

        // Thread safety
        TestConcurrentAppointments();
        TestConcurrentDismissals();

        std::cout << "\n========================================" << std::endl;
        std::cout << "✅ ALL TESTS PASSED" << std::endl;
        std::cout << "========================================" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cout << "\n========================================" << std::endl;
        std::cout << "❌ TEST FAILED: " << e.what() << std::endl;
        std::cout << "========================================" << std::endl;
        return 1;
    }
}
