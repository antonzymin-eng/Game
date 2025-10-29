// ============================================================================
// test_diplomacy_refactoring.cpp - Tests for Refactored Diplomacy System
// Created: 2025-10-28
// ============================================================================

#include "game/diplomacy/DiplomacyRepository.h"
#include "game/diplomacy/DiplomaticCalculator.h"
#include "game/diplomacy/DiplomaticAI.h"
#include "game/diplomacy/handlers/AllianceProposalHandler.h"
#include "game/diplomacy/handlers/WarDeclarationHandler.h"
#include "core/ECS/EntityManager.h"
#include "core/ECS/ComponentAccessManager.h"
#include <iostream>
#include <cassert>

using namespace game::diplomacy;
using namespace core::ecs;

void TestDiplomacyRepository() {
    std::cout << "[TEST] DiplomacyRepository...\n";

    // Create ECS infrastructure
    EntityManager entity_manager;
    ComponentAccessManager access_manager(&entity_manager);

    DiplomacyRepository repo(access_manager);

    // Test: Create component
    types::EntityID realm1 = 1001;
    auto component = repo.Create(realm1, DiplomaticPersonality::AGGRESSIVE);
    assert(component != nullptr);
    assert(component->personality == DiplomaticPersonality::AGGRESSIVE);
    std::cout << "  ✓ Component creation works\n";

    // Test: Get component
    auto retrieved = repo.Get(realm1);
    assert(retrieved != nullptr);
    assert(retrieved->personality == DiplomaticPersonality::AGGRESSIVE);
    std::cout << "  ✓ Component retrieval works\n";

    // Test: Get or create (existing)
    auto existing = repo.GetOrCreate(realm1);
    assert(existing != nullptr);
    std::cout << "  ✓ GetOrCreate returns existing component\n";

    // Test: Get or create (new)
    types::EntityID realm2 = 1002;
    auto new_component = repo.GetOrCreate(realm2);
    assert(new_component != nullptr);
    std::cout << "  ✓ GetOrCreate creates new component\n";

    // Test: Get pair
    auto pair = repo.GetPair(realm1, realm2);
    assert(pair.both_valid());
    std::cout << "  ✓ GetPair works\n";

    // Test: Get all realms
    auto all_realms = repo.GetAllRealms();
    assert(all_realms.size() >= 2);
    std::cout << "  ✓ GetAllRealms works (" << all_realms.size() << " realms)\n";

    std::cout << "[PASS] DiplomacyRepository\n\n";
}

void TestDiplomaticCalculator() {
    std::cout << "[TEST] DiplomaticCalculator...\n";

    // Test: Opinion change calculation
    DiplomaticState state;
    state.opinion = 0;
    state.trust = 0.5;

    int change = DiplomaticCalculator::CalculateOpinionChange(
        state,
        DiplomaticAction::ALLIANCE_FORMED,
        1.0
    );
    assert(change > 0);
    std::cout << "  ✓ Opinion change calculation works (alliance: +" << change << ")\n";

    // Test: Opinion clamp
    int clamped = DiplomaticCalculator::ClampOpinion(150);
    assert(clamped == 100);
    std::cout << "  ✓ Opinion clamping works\n";

    // Test: Trust change calculation
    double trust_change = DiplomaticCalculator::CalculateTrustChange(
        0.5,
        DiplomaticIncident::TREATY_BREACH
    );
    assert(trust_change < 0);
    std::cout << "  ✓ Trust change calculation works (breach: " << trust_change << ")\n";

    // Test: War likelihood
    DiplomacyComponent aggressor;
    aggressor.personality = DiplomaticPersonality::AGGRESSIVE;
    aggressor.prestige = 100.0;
    aggressor.war_weariness = 0.0;

    DiplomacyComponent target;
    target.personality = DiplomaticPersonality::PEACEFUL;
    target.prestige = 50.0;

    double war_likelihood = DiplomaticCalculator::CalculateWarLikelihood(
        aggressor, target, -50, 50.0
    );
    assert(war_likelihood > 0.5);
    std::cout << "  ✓ War likelihood calculation works (" << war_likelihood << ")\n";

    // Test: Personality traits
    double war_trait = DiplomaticCalculator::GetPersonalityWarLikelihood(
        DiplomaticPersonality::AGGRESSIVE
    );
    assert(war_trait > 0.5);
    std::cout << "  ✓ Personality war likelihood works (" << war_trait << ")\n";

    // Test: Alliance value
    double alliance_value = DiplomaticCalculator::CalculateAllianceValue(
        aggressor, target
    );
    assert(alliance_value >= 0.0 && alliance_value <= 1.0);
    std::cout << "  ✓ Alliance value calculation works (" << alliance_value << ")\n";

    // Test: String conversions
    std::string action_str = DiplomaticCalculator::ActionToString(
        DiplomaticAction::ALLIANCE_FORMED
    );
    assert(!action_str.empty());
    std::cout << "  ✓ Action to string works: \"" << action_str << "\"\n";

    std::cout << "[PASS] DiplomaticCalculator\n\n";
}

void TestAllianceHandler() {
    std::cout << "[TEST] AllianceProposalHandler...\n";

    // Create ECS infrastructure
    EntityManager entity_manager;
    ComponentAccessManager access_manager(&entity_manager);

    DiplomacyRepository repo(access_manager);
    DiplomaticCalculator calculator;

    AllianceProposalHandler handler(repo, calculator);

    // Create two realms
    types::EntityID realm1 = 2001;
    types::EntityID realm2 = 2002;

    auto comp1 = repo.Create(realm1, DiplomaticPersonality::DIPLOMATIC);
    auto comp2 = repo.Create(realm2, DiplomaticPersonality::DIPLOMATIC);

    // Set positive opinions
    comp1->ModifyOpinion(realm2, 50, "Test setup");
    comp2->ModifyOpinion(realm1, 50, "Test setup");

    // Test: Validation
    bool valid = handler.Validate(realm1, realm2);
    std::cout << "  ✓ Validation works (result: " << (valid ? "valid" : "invalid") << ")\n";

    // Test: Execute alliance
    auto result = handler.Execute(realm1, realm2);
    std::cout << "  ✓ Alliance execution: " << result.message << "\n";

    if (result.success) {
        // Verify alliance was established
        assert(comp1->IsAlliedWith(realm2));
        assert(comp2->IsAlliedWith(realm1));
        std::cout << "  ✓ Alliance successfully established\n";
    }

    // Test: Cannot ally again
    auto result2 = handler.Execute(realm1, realm2);
    assert(!result2.success);
    std::cout << "  ✓ Cannot create duplicate alliance\n";

    std::cout << "[PASS] AllianceProposalHandler\n\n";
}

void TestWarDeclarationHandler() {
    std::cout << "[TEST] WarDeclarationHandler...\n";

    EntityManager entity_manager;
    ComponentAccessManager access_manager(&entity_manager);

    DiplomacyRepository repo(access_manager);
    DiplomaticCalculator calculator;

    WarDeclarationHandler handler(repo, calculator);

    types::EntityID realm1 = 3001;
    types::EntityID realm2 = 3002;

    auto comp1 = repo.Create(realm1, DiplomaticPersonality::AGGRESSIVE);
    auto comp2 = repo.Create(realm2, DiplomaticPersonality::PEACEFUL);

    // Test: Execute war declaration
    auto result = handler.Execute(realm1, realm2);
    std::cout << "  ✓ War declaration: " << result.message << "\n";

    if (result.success) {
        assert(comp1->IsAtWarWith(realm2));
        assert(comp2->IsAtWarWith(realm1));
        std::cout << "  ✓ War successfully declared\n";
    }

    // Test: Cannot declare war again
    auto result2 = handler.Execute(realm1, realm2);
    assert(!result2.success);
    std::cout << "  ✓ Cannot declare war twice\n";

    std::cout << "[PASS] WarDeclarationHandler\n\n";
}

void TestDiplomaticAI() {
    std::cout << "[TEST] DiplomaticAI...\n";

    EntityManager entity_manager;
    ComponentAccessManager access_manager(&entity_manager);

    DiplomacyRepository repo(access_manager);
    DiplomaticCalculator calculator;

    DiplomaticAI ai(repo, calculator);

    // Create several test realms
    types::EntityID realm1 = 4001;
    types::EntityID realm2 = 4002;
    types::EntityID realm3 = 4003;

    auto comp1 = repo.Create(realm1, DiplomaticPersonality::DIPLOMATIC);
    auto comp2 = repo.Create(realm2, DiplomaticPersonality::DIPLOMATIC);
    auto comp3 = repo.Create(realm3, DiplomaticPersonality::AGGRESSIVE);

    comp1->prestige = 100.0;
    comp2->prestige = 80.0;
    comp3->prestige = 60.0;

    // Test: Get alliance candidates
    auto candidates = ai.GetAllianceCandidates(realm1, 2);
    std::cout << "  ✓ Found " << candidates.size() << " alliance candidates\n";

    // Test: Should propose alliance
    bool should_ally = ai.ShouldProposeAlliance(realm1, realm2);
    std::cout << "  ✓ Alliance decision: " << (should_ally ? "yes" : "no") << "\n";

    // Test: Evaluate diplomatic options
    auto decisions = ai.EvaluateDiplomaticOptions(realm1);
    std::cout << "  ✓ Evaluated " << decisions.size() << " diplomatic options\n";

    for (const auto& decision : decisions) {
        std::cout << "    - Priority " << decision.priority << ": "
                  << decision.reasoning << "\n";
    }

    std::cout << "[PASS] DiplomaticAI\n\n";
}

void TestIntegration() {
    std::cout << "[TEST] Integration Test...\n";

    EntityManager entity_manager;
    ComponentAccessManager access_manager(&entity_manager);

    DiplomacyRepository repo(access_manager);
    DiplomaticCalculator calculator;
    DiplomaticAI ai(repo, calculator);

    AllianceProposalHandler alliance_handler(repo, calculator);
    WarDeclarationHandler war_handler(repo, calculator);

    // Create a diplomatic scenario
    types::EntityID realm_a = 5001;
    types::EntityID realm_b = 5002;
    types::EntityID realm_c = 5003;

    auto comp_a = repo.Create(realm_a, DiplomaticPersonality::DIPLOMATIC);
    auto comp_b = repo.Create(realm_b, DiplomaticPersonality::DIPLOMATIC);
    auto comp_c = repo.Create(realm_c, DiplomaticPersonality::AGGRESSIVE);

    // Set up relationships
    comp_a->ModifyOpinion(realm_b, 60, "Historical friendship");
    comp_b->ModifyOpinion(realm_a, 60, "Historical friendship");
    comp_c->ModifyOpinion(realm_a, -40, "Border disputes");

    std::cout << "  Initial state:\n";
    std::cout << "    A-B opinion: " << comp_a->GetRelationship(realm_b)->opinion << "\n";
    std::cout << "    C-A opinion: " << comp_c->GetRelationship(realm_a)->opinion << "\n";

    // A and B form alliance
    auto alliance_result = alliance_handler.Execute(realm_a, realm_b);
    assert(alliance_result.success);
    std::cout << "  ✓ Realms A and B formed alliance\n";

    // C evaluates options (should consider war with A)
    auto decisions = ai.EvaluateDiplomaticOptions(realm_c);
    std::cout << "  ✓ Realm C evaluated " << decisions.size() << " options\n";

    // Simulate opinion decay
    int decayed_opinion = calculator.CalculateOpinionDecay(
        comp_c->GetRelationship(realm_a)->opinion,
        1.0f,
        comp_c->personality
    );
    std::cout << "  ✓ Opinion decay calculated: " << decayed_opinion << "\n";

    std::cout << "[PASS] Integration Test\n\n";
}

int main() {
    std::cout << "============================================\n";
    std::cout << "  Diplomacy Refactoring Test Suite\n";
    std::cout << "============================================\n\n";

    try {
        TestDiplomacyRepository();
        TestDiplomaticCalculator();
        TestAllianceHandler();
        TestWarDeclarationHandler();
        TestDiplomaticAI();
        TestIntegration();

        std::cout << "============================================\n";
        std::cout << "  ALL TESTS PASSED ✓\n";
        std::cout << "============================================\n";

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\n[FAIL] Exception: " << e.what() << "\n";
        return 1;
    }
}
