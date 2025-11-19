// Created: November 19, 2025
// Location: tests/performance/test_character_system_performance.cpp
// Purpose: Performance benchmarks for character system with 1000+ characters

#include "game/components/CharacterComponent.h"
#include "game/components/NobleArtsComponent.h"
#include "game/components/TraitsComponent.h"
#include "game/character/CharacterRelationships.h"
#include "game/character/CharacterLifeEvents.h"
#include "game/character/CharacterEducation.h"
#include "game/ai/CharacterAI.h"
#include "utils/Random.h"

#include <iostream>
#include <chrono>
#include <vector>
#include <memory>
#include <iomanip>

using namespace game::character;
using namespace AI;
using namespace std::chrono;

// ============================================================================
// Performance Benchmark Utilities
// ============================================================================

class PerformanceBenchmark {
public:
    PerformanceBenchmark(const std::string& name) : m_name(name) {
        m_start = high_resolution_clock::now();
    }

    ~PerformanceBenchmark() {
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - m_start);

        std::cout << std::setw(50) << std::left << m_name
                  << std::setw(15) << std::right << duration.count() << " μs"
                  << std::setw(12) << std::right << (duration.count() / 1000.0) << " ms"
                  << std::endl;
    }

private:
    std::string m_name;
    high_resolution_clock::time_point m_start;
};

// ============================================================================
// Character Creation Benchmarks
// ============================================================================

void BenchmarkCharacterCreation(int count) {
    std::cout << "\n=== Character Creation Benchmark (" << count << " characters) ===\n\n";

    std::vector<CharacterComponent> characters;
    characters.reserve(count);

    {
        PerformanceBenchmark bench("Create " + std::to_string(count) + " CharacterComponents");
        for (int i = 0; i < count; ++i) {
            CharacterComponent character;
            character.SetName("Character_" + std::to_string(i));
            character.SetAge(18 + utils::RandomInt(0, 50));
            character.SetDiplomacy(utils::RandomInt(0, 20));
            character.SetMartial(utils::RandomInt(0, 20));
            character.SetStewardship(utils::RandomInt(0, 20));
            character.SetIntrigue(utils::RandomInt(0, 20));
            character.SetLearning(utils::RandomInt(0, 20));
            characters.push_back(character);
        }
    }

    std::vector<NobleArtsComponent> arts;
    arts.reserve(count);

    {
        PerformanceBenchmark bench("Create " + std::to_string(count) + " NobleArtsComponents");
        for (int i = 0; i < count; ++i) {
            NobleArtsComponent art;
            art.SetPoetrySkill(utils::RandomInt(0, 10));
            art.SetMusicSkill(utils::RandomInt(0, 10));
            art.SetPaintingSkill(utils::RandomInt(0, 10));
            arts.push_back(art);
        }
    }

    std::vector<TraitsComponent> traits;
    traits.reserve(count);

    {
        PerformanceBenchmark bench("Create " + std::to_string(count) + " TraitsComponents");
        for (int i = 0; i < count; ++i) {
            TraitsComponent trait;
            // Add random traits
            if (utils::RandomBool(0.3f)) trait.AddTrait("brave");
            if (utils::RandomBool(0.3f)) trait.AddTrait("ambitious");
            if (utils::RandomBool(0.2f)) trait.AddTrait("scholarly");
            traits.push_back(trait);
        }
    }
}

// ============================================================================
// Relationship System Benchmarks
// ============================================================================

void BenchmarkRelationshipSystem(int count) {
    std::cout << "\n=== Relationship System Benchmark (" << count << " characters) ===\n\n";

    std::vector<CharacterRelationshipsComponent> relationships(count);

    {
        PerformanceBenchmark bench("Create relationships for " + std::to_string(count) + " characters");
        for (int i = 0; i < count; ++i) {
            relationships[i].character_id = static_cast<types::EntityID>(i);

            // Each character has 5-15 relationships
            int rel_count = utils::RandomInt(5, 15);
            for (int j = 0; j < rel_count; ++j) {
                types::EntityID other = static_cast<types::EntityID>(utils::RandomInt(0, count - 1));
                if (other != i) {
                    relationships[i].SetRelationship(
                        other,
                        static_cast<RelationshipType>(utils::RandomInt(0, static_cast<int>(RelationshipType::COUNT) - 1)),
                        utils::RandomInt(-100, 100),
                        utils::RandomFloat(0.0, 100.0)
                    );
                }
            }
        }
    }

    {
        PerformanceBenchmark bench("Query " + std::to_string(count * 10) + " relationships");
        int found = 0;
        for (int i = 0; i < count; ++i) {
            for (int j = 0; j < 10; ++j) {
                types::EntityID other = static_cast<types::EntityID>(utils::RandomInt(0, count - 1));
                auto rel = relationships[i].GetRelationship(other);
                if (rel.has_value()) {
                    found++;
                }
            }
        }
        std::cout << "  Found " << found << " relationships\n";
    }

    {
        PerformanceBenchmark bench("Check friendship for " + std::to_string(count * 10) + " pairs");
        int friends = 0;
        for (int i = 0; i < count; ++i) {
            for (int j = 0; j < 10; ++j) {
                types::EntityID other = static_cast<types::EntityID>(utils::RandomInt(0, count - 1));
                if (relationships[i].IsFriendsWith(other)) {
                    friends++;
                }
            }
        }
        std::cout << "  Found " << friends << " friendships\n";
    }
}

// ============================================================================
// Traits System Benchmarks
// ============================================================================

void BenchmarkTraitsSystem(int count) {
    std::cout << "\n=== Traits System Benchmark (" << count << " characters) ===\n\n";

    std::vector<TraitsComponent> characters(count);
    auto& trait_db = TraitDatabase::Instance();

    {
        PerformanceBenchmark bench("Add 5 traits to " + std::to_string(count) + " characters");
        const std::vector<std::string> possible_traits = {
            "brave", "ambitious", "kind", "scholarly", "strong"
        };

        for (int i = 0; i < count; ++i) {
            for (const auto& trait : possible_traits) {
                characters[i].AddTrait(trait, trait_db.GetTrait(trait));
            }
        }
    }

    {
        PerformanceBenchmark bench("Recalculate modifiers for " + std::to_string(count) + " characters");
        for (int i = 0; i < count; ++i) {
            characters[i].RecalculateModifiers(trait_db.GetAllTraits());
        }
    }

    {
        PerformanceBenchmark bench("Query traits " + std::to_string(count * 20) + " times");
        int total_traits = 0;
        for (int i = 0; i < count; ++i) {
            for (int j = 0; j < 20; ++j) {
                if (characters[i].HasTrait("brave")) total_traits++;
                if (characters[i].HasTrait("ambitious")) total_traits++;
            }
        }
        std::cout << "  Trait queries: " << total_traits << "\n";
    }
}

// ============================================================================
// Life Events Benchmarks
// ============================================================================

void BenchmarkLifeEvents(int count) {
    std::cout << "\n=== Life Events System Benchmark (" << count << " characters) ===\n\n";

    std::vector<CharacterLifeEventsComponent> characters(count);

    {
        PerformanceBenchmark bench("Add 20 life events to " + std::to_string(count) + " characters");
        for (int i = 0; i < count; ++i) {
            characters[i].character_id = static_cast<types::EntityID>(i);

            // Add various life events
            for (int j = 0; j < 20; ++j) {
                LifeEventType event_type = static_cast<LifeEventType>(
                    utils::RandomInt(0, static_cast<int>(LifeEventType::COUNT) - 1));

                characters[i].AddSimpleEvent(
                    event_type,
                    "Event description " + std::to_string(j),
                    utils::RandomInt(10, 60),
                    utils::RandomBool(0.3f)
                );
            }
        }
    }

    {
        PerformanceBenchmark bench("Query events for " + std::to_string(count) + " characters");
        int total_major = 0;
        int total_marriages = 0;

        for (int i = 0; i < count; ++i) {
            total_major += characters[i].GetMajorEvents().size();
            total_marriages += characters[i].GetEventCount(LifeEventType::MARRIAGE);
        }

        std::cout << "  Total major events: " << total_major << "\n";
        std::cout << "  Total marriages: " << total_marriages << "\n";
    }
}

// ============================================================================
// Education System Benchmarks
// ============================================================================

void BenchmarkEducationSystem(int count) {
    std::cout << "\n=== Education System Benchmark (" << count << " characters) ===\n\n";

    std::vector<CharacterEducationComponent> characters(count);

    {
        PerformanceBenchmark bench("Start education for " + std::to_string(count) + " characters");
        for (int i = 0; i < count; ++i) {
            characters[i].character_id = static_cast<types::EntityID>(i);
            EducationFocus focus = static_cast<EducationFocus>(
                utils::RandomInt(0, static_cast<int>(EducationFocus::COUNT) - 1));
            characters[i].StartEducation(focus, 0, utils::RandomFloat(0.8f, 1.5f));
        }
    }

    {
        PerformanceBenchmark bench("Gain XP " + std::to_string(count * 100) + " times");
        for (int i = 0; i < count; ++i) {
            for (int j = 0; j < 100; ++j) {
                EducationFocus skill = static_cast<EducationFocus>(
                    utils::RandomInt(0, static_cast<int>(EducationFocus::LEARNING)));
                characters[i].GainExperience(skill, utils::RandomInt(5, 20));
            }
        }
    }

    {
        PerformanceBenchmark bench("Check level ups for " + std::to_string(count) + " characters");
        int can_level_up = 0;
        for (int i = 0; i < count; ++i) {
            auto check = characters[i].CheckLevelUps(10, 10, 10, 10, 10);
            if (check.diplomacy_ready || check.martial_ready || check.stewardship_ready ||
                check.intrigue_ready || check.learning_ready) {
                can_level_up++;
            }
        }
        std::cout << "  Characters ready to level up: " << can_level_up << "\n";
    }
}

// ============================================================================
// Character AI Benchmarks
// ============================================================================

void BenchmarkCharacterAI(int count) {
    std::cout << "\n=== Character AI System Benchmark (" << count << " AI instances) ===\n\n";

    std::vector<std::unique_ptr<CharacterAI>> ai_characters;
    ai_characters.reserve(count);

    {
        PerformanceBenchmark bench("Create " + std::to_string(count) + " CharacterAI instances");
        for (int i = 0; i < count; ++i) {
            CharacterArchetype archetype = static_cast<CharacterArchetype>(
                utils::RandomInt(0, static_cast<int>(CharacterArchetype::COUNT) - 1));

            ai_characters.push_back(std::make_unique<CharacterAI>(
                i,
                static_cast<types::EntityID>(i),
                "AI_Character_" + std::to_string(i),
                archetype
            ));
        }
    }

    {
        PerformanceBenchmark bench("Update ambitions for " + std::to_string(count) + " characters");
        for (auto& ai : ai_characters) {
            ai->UpdateAmbitions();
        }
    }

    {
        PerformanceBenchmark bench("Update relationships for " + std::to_string(count) + " characters");
        for (auto& ai : ai_characters) {
            ai->UpdateRelationships();
        }
    }

    {
        PerformanceBenchmark bench("Evaluate " + std::to_string(count * 3) + " decisions");
        for (auto& ai : ai_characters) {
            ai->EvaluatePlot(utils::RandomInt(1, count));
            ai->EvaluateProposal();
            ai->EvaluateRelationship(utils::RandomInt(1, count));
        }
    }
}

// ============================================================================
// Main Benchmark Suite
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "========================================================\n";
    std::cout << "  CHARACTER SYSTEM PERFORMANCE BENCHMARKS\n";
    std::cout << "========================================================\n";

    std::cout << "\nConfiguration:\n";
    std::cout << "  - Modern C++ random number generation\n";
    std::cout << "  - High-resolution timers\n";
    std::cout << "  - Optimized STL containers\n";
    std::cout << "\n";

    // Run benchmarks with different character counts
    const std::vector<int> counts = {100, 500, 1000, 2000};

    for (int count : counts) {
        std::cout << "\n";
        std::cout << "========================================================\n";
        std::cout << "  TESTING WITH " << count << " CHARACTERS\n";
        std::cout << "========================================================\n";

        BenchmarkCharacterCreation(count);
        BenchmarkRelationshipSystem(count);
        BenchmarkTraitsSystem(count);
        BenchmarkLifeEvents(count);
        BenchmarkEducationSystem(count);

        // Only run AI benchmarks for smaller counts (AI is more expensive)
        if (count <= 1000) {
            BenchmarkCharacterAI(count);
        }
    }

    std::cout << "\n";
    std::cout << "========================================================\n";
    std::cout << "  ALL BENCHMARKS COMPLETED\n";
    std::cout << "========================================================\n";
    std::cout << "\n";

    std::cout << "Performance Summary:\n";
    std::cout << "  - Character creation: O(n) linear scaling\n";
    std::cout << "  - Relationship queries: O(1) hash map lookup\n";
    std::cout << "  - Trait system: O(n*t) where t = trait count\n";
    std::cout << "  - Life events: O(n*e) where e = event count\n";
    std::cout << "  - Education system: O(n) linear scaling\n";
    std::cout << "  - Character AI: O(n) per update cycle\n";
    std::cout << "\n";

    return 0;
}
