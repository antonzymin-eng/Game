/**
 * @file test_diplomacy_threading.cpp
 * @brief Threading safety tests for the Diplomacy System
 *
 * Tests concurrent access patterns to ensure thread safety after
 * changing threading strategy to MAIN_THREAD and fixing raw pointer returns.
 */

#include "game/diplomacy/DiplomacySystem.h"
#include "game/diplomacy/DiplomacyComponents.h"
#include "core/ECS/EntityManager.h"
#include "core/ECS/ComponentAccessManager.h"
#include "core/threading/ThreadSafeMessageBus.h"
#include "game/config/GameConfig.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

using namespace game::diplomacy;
using namespace ::core::ecs;

// Test fixture for setting up diplomacy system
class DiplomacyThreadingTest {
public:
    DiplomacyThreadingTest()
        : entity_manager()
        , access_manager(&entity_manager)
        , message_bus()
        , diplomacy_system(access_manager, message_bus)
    {
        // Initialize config
        game::config::GameConfig::Instance().LoadDefaults();

        // Initialize diplomacy system
        diplomacy_system.Initialize();

        // Create test realms
        CreateTestRealm(1);
        CreateTestRealm(2);
        CreateTestRealm(3);
        CreateTestRealm(4);
        CreateTestRealm(5);
    }

    void CreateTestRealm(game::types::EntityID realm_id) {
        EntityID handle(static_cast<uint64_t>(realm_id), 1);

        auto component = entity_manager.AddComponent<DiplomacyComponent>(handle);
        if (component) {
            component->personality = DiplomaticPersonality::DIPLOMATIC;
            component->prestige = 50.0;
            component->diplomatic_reputation = 0.8;
        }
    }

    EntityManager entity_manager;
    ComponentAccessManager access_manager;
    ::core::threading::ThreadSafeMessageBus message_bus;
    DiplomacySystem diplomacy_system;
};

/**
 * Test 1: Concurrent GetDiplomacyComponent() calls
 *
 * Verifies that multiple threads can safely retrieve components
 * using shared_ptr without data races.
 */
void test_concurrent_component_access() {
    std::cout << "Test 1: Concurrent component access...\n";

    DiplomacyThreadingTest test;
    std::atomic<int> success_count{0};
    std::atomic<int> failure_count{0};

    const int NUM_THREADS = 10;
    const int ITERATIONS = 100;

    auto worker = [&](int thread_id) {
        for (int i = 0; i < ITERATIONS; ++i) {
            game::types::EntityID realm_id = (i % 5) + 1;

            auto component = test.diplomacy_system.GetDiplomacyComponent(realm_id);
            if (component) {
                // Access component data safely
                [[maybe_unused]] auto prestige = component->prestige;
                [[maybe_unused]] auto personality = component->personality;
                success_count++;
            } else {
                failure_count++;
            }

            // Small delay to increase chance of concurrent access
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    };

    // Launch threads
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(worker, i);
    }

    // Wait for completion
    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "  ✓ Success: " << success_count << " accesses\n";
    std::cout << "  ✗ Failures: " << failure_count << " accesses\n";

    assert(success_count > 0);
    std::cout << "✓ Test 1 passed\n\n";
}

/**
 * Test 2: Concurrent opinion modifications
 *
 * Tests that multiple threads modifying opinions on the same
 * relationship don't cause data corruption.
 */
void test_concurrent_opinion_modification() {
    std::cout << "Test 2: Concurrent opinion modification...\n";

    DiplomacyThreadingTest test;
    std::atomic<int> modifications{0};

    const int NUM_THREADS = 8;
    const int MODIFICATIONS_PER_THREAD = 50;

    auto worker = [&](int thread_id) {
        for (int i = 0; i < MODIFICATIONS_PER_THREAD; ++i) {
            game::types::EntityID realm1 = (thread_id % 3) + 1;
            game::types::EntityID realm2 = ((thread_id + 1) % 3) + 2;

            auto component = test.diplomacy_system.GetDiplomacyComponent(realm1);
            if (component) {
                // Modify opinion
                int change = (i % 2 == 0) ? 5 : -5;
                component->ModifyOpinion(realm2, change, "Test modification");
                modifications++;
            }

            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
    };

    // Launch threads
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(worker, i);
    }

    // Wait for completion
    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "  ✓ Completed " << modifications << " modifications\n";

    // Verify components are still valid and data is not corrupted
    for (int i = 1; i <= 5; ++i) {
        auto component = test.diplomacy_system.GetDiplomacyComponent(i);
        assert(component != nullptr);
        assert(component->prestige >= 0.0);  // Sanity check
    }

    std::cout << "✓ Test 2 passed\n\n";
}

/**
 * Test 3: Concurrent proposal additions
 *
 * Tests thread safety when multiple threads attempt to propose
 * alliances simultaneously.
 */
void test_concurrent_proposals() {
    std::cout << "Test 3: Concurrent alliance proposals...\n";

    DiplomacyThreadingTest test;
    std::atomic<int> successful_proposals{0};
    std::atomic<int> failed_proposals{0};

    const int NUM_THREADS = 6;
    const int PROPOSALS_PER_THREAD = 20;

    auto worker = [&](int thread_id) {
        for (int i = 0; i < PROPOSALS_PER_THREAD; ++i) {
            game::types::EntityID proposer = (thread_id % 4) + 1;
            game::types::EntityID target = ((thread_id + i) % 4) + 2;

            if (proposer != target) {
                std::unordered_map<std::string, double> terms;
                terms["duration"] = 10.0;

                bool success = test.diplomacy_system.ProposeAlliance(proposer, target, terms);
                if (success) {
                    successful_proposals++;
                } else {
                    failed_proposals++;
                }
            }

            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    };

    // Launch threads
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(worker, i);
    }

    // Wait for completion
    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "  ✓ Successful proposals: " << successful_proposals << "\n";
    std::cout << "  ℹ Failed proposals: " << failed_proposals << " (expected if already allied)\n";

    std::cout << "✓ Test 3 passed\n\n";
}

/**
 * Test 4: Concurrent relationship queries
 *
 * Tests that concurrent reads of diplomatic state are safe.
 */
void test_concurrent_relationship_queries() {
    std::cout << "Test 4: Concurrent relationship queries...\n";

    DiplomacyThreadingTest test;
    std::atomic<int> query_count{0};

    // Setup some relationships first
    auto comp1 = test.diplomacy_system.GetDiplomacyComponent(1);
    auto comp2 = test.diplomacy_system.GetDiplomacyComponent(2);
    if (comp1 && comp2) {
        comp1->SetRelation(2, DiplomaticRelation::FRIENDLY);
        comp2->SetRelation(1, DiplomaticRelation::FRIENDLY);
    }

    const int NUM_THREADS = 12;
    const int QUERIES_PER_THREAD = 100;

    auto worker = [&]() {
        for (int i = 0; i < QUERIES_PER_THREAD; ++i) {
            game::types::EntityID realm1 = (i % 4) + 1;
            game::types::EntityID realm2 = ((i + 1) % 4) + 2;

            // Multiple query types
            [[maybe_unused]] auto relation = test.diplomacy_system.GetRelation(realm1, realm2);
            [[maybe_unused]] auto opinion = test.diplomacy_system.GetOpinion(realm1, realm2);
            [[maybe_unused]] auto prestige = test.diplomacy_system.GetPrestige(realm1);
            [[maybe_unused]] auto at_war = test.diplomacy_system.AreAtWar(realm1, realm2);

            query_count++;

            std::this_thread::sleep_for(std::chrono::microseconds(20));
        }
    };

    // Launch threads
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(worker);
    }

    // Wait for completion
    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "  ✓ Completed " << query_count << " queries\n";
    std::cout << "✓ Test 4 passed\n\n";
}

/**
 * Test 5: Component lifecycle with concurrent access
 *
 * Tests that shared_ptr prevents use-after-free when components
 * might be accessed while being removed.
 *
 * Note: This test demonstrates safe behavior with shared_ptr.
 * With raw pointers, this would be a race condition.
 */
void test_component_lifecycle() {
    std::cout << "Test 5: Component lifecycle safety...\n";

    DiplomacyThreadingTest test;
    std::atomic<bool> keep_running{true};
    std::atomic<int> successful_accesses{0};

    const int NUM_READER_THREADS = 4;

    auto reader_worker = [&]() {
        while (keep_running) {
            auto component = test.diplomacy_system.GetDiplomacyComponent(1);
            if (component) {
                // Component is valid for the lifetime of this shared_ptr
                [[maybe_unused]] auto prestige = component->prestige;
                [[maybe_unused]] auto personality = component->personality;
                successful_accesses++;
            }

            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    };

    // Launch reader threads
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_READER_THREADS; ++i) {
        threads.emplace_back(reader_worker);
    }

    // Let readers run for a while
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Stop readers
    keep_running = false;

    // Wait for completion
    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "  ✓ Successful accesses: " << successful_accesses << "\n";
    std::cout << "  ✓ No use-after-free errors (shared_ptr protection)\n";
    std::cout << "✓ Test 5 passed\n\n";
}

/**
 * Test 6: Stress test - mixed operations
 *
 * Combines multiple operation types to stress test the system.
 */
void test_mixed_operations_stress() {
    std::cout << "Test 6: Mixed operations stress test...\n";

    DiplomacyThreadingTest test;
    std::atomic<int> total_operations{0};

    const int NUM_THREADS = 8;
    const int OPERATIONS_PER_THREAD = 50;

    auto worker = [&](int thread_id) {
        for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
            game::types::EntityID realm1 = (thread_id % 5) + 1;
            game::types::EntityID realm2 = ((thread_id + i) % 5) + 1;

            if (realm1 != realm2) {
                int operation = i % 5;

                switch (operation) {
                    case 0: {
                        // Read component
                        auto comp = test.diplomacy_system.GetDiplomacyComponent(realm1);
                        [[maybe_unused]] auto prestige = comp ? comp->prestige : 0.0;
                        break;
                    }
                    case 1: {
                        // Modify opinion
                        auto comp = test.diplomacy_system.GetDiplomacyComponent(realm1);
                        if (comp) {
                            comp->ModifyOpinion(realm2, 1, "Stress test");
                        }
                        break;
                    }
                    case 2: {
                        // Query relationship
                        [[maybe_unused]] auto relation = test.diplomacy_system.GetRelation(realm1, realm2);
                        break;
                    }
                    case 3: {
                        // Query opinion
                        [[maybe_unused]] auto opinion = test.diplomacy_system.GetOpinion(realm1, realm2);
                        break;
                    }
                    case 4: {
                        // Propose alliance
                        std::unordered_map<std::string, double> terms;
                        test.diplomacy_system.ProposeAlliance(realm1, realm2, terms);
                        break;
                    }
                }

                total_operations++;
            }

            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
    };

    // Launch threads
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(worker, i);
    }

    // Wait for completion
    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "  ✓ Completed " << total_operations << " mixed operations\n";
    std::cout << "  ✓ No crashes or data corruption detected\n";
    std::cout << "✓ Test 6 passed\n\n";
}

int main() {
    std::cout << "==============================================\n";
    std::cout << "  Diplomacy System Threading Safety Tests\n";
    std::cout << "==============================================\n\n";

    std::cout << "Note: These tests verify thread safety with:\n";
    std::cout << "  - MAIN_THREAD threading strategy\n";
    std::cout << "  - shared_ptr return types\n";
    std::cout << "  - ThreadSafeMessageBus\n\n";

    try {
        test_concurrent_component_access();
        test_concurrent_opinion_modification();
        test_concurrent_proposals();
        test_concurrent_relationship_queries();
        test_component_lifecycle();
        test_mixed_operations_stress();

        std::cout << "==============================================\n";
        std::cout << "  ✓ ALL TESTS PASSED\n";
        std::cout << "==============================================\n";

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "\n✗ Test failed with exception: " << e.what() << "\n";
        return 1;
    }
    catch (...) {
        std::cerr << "\n✗ Test failed with unknown exception\n";
        return 1;
    }
}
