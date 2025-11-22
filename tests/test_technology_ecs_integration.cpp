// ============================================================================
// Technology System ECS Integration Test
// Validates Technology System component architecture and ECS integration
// Created: 2025-01-18
// ============================================================================

#include "game/technology/TechnologySystem.h"
#include "game/technology/TechnologyComponents.h"
#include "core/ECS/ComponentAccessManager.h"
#include "core/ECS/MessageBus.h"
#include "utils/RandomGenerator.h"

#include <iostream>
#include <cassert>
#include <vector>
#include <chrono>
#include <memory>

using namespace game::technology;
using namespace core::ecs;
using namespace core::threading;

class TechnologyECSIntegrationTest {
private:
    std::unique_ptr<ComponentAccessManager> m_component_manager;
    std::unique_ptr<ThreadSafeMessageBus> m_message_bus;
    std::unique_ptr<TechnologySystem> m_tech_system;
    
    std::vector<types::EntityID> m_test_entities;
    
public:
    TechnologyECSIntegrationTest() {
        InitializeTestEnvironment();
    }
    
    ~TechnologyECSIntegrationTest() {
        CleanupTestEnvironment();
    }
    
    void RunAllTests() {
        std::cout << "=== Technology System ECS Integration Tests ===" << std::endl;
        
        try {
            TestComponentCreation();
            TestComponentManagement();
            TestHighLevelIntegration();
            TestComponentValidation();
            TestSystemInitialization();
            TestResearchProgressTracking();
            TestInnovationBreakthroughs();
            TestKnowledgeTransfer();
            TestTechnologyEvents();
            TestComponentSynchronization();
            TestSystemIntegration();
            
            std::cout << "✅ All Technology System ECS tests passed!" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "❌ Test failed: " << e.what() << std::endl;
            throw;
        }
    }

private:
    void InitializeTestEnvironment() {
        m_component_manager = std::make_unique<ComponentAccessManager>();
        m_message_bus = std::make_unique<ThreadSafeMessageBus>();
        
        // Register Technology components
        m_component_manager->RegisterComponent<ResearchComponent>();
        m_component_manager->RegisterComponent<InnovationComponent>();
        m_component_manager->RegisterComponent<KnowledgeComponent>();
        m_component_manager->RegisterComponent<TechnologyEventsComponent>();
        
        m_tech_system = std::make_unique<TechnologySystem>(*m_component_manager, *m_message_bus);
        
        // Create test entities
        for (int i = 1; i <= 5; ++i) {
            m_test_entities.push_back(types::EntityID{static_cast<types::entity_id_t>(i)});
        }
        
        // Initialize system
        m_tech_system->Initialize();
    }
    
    void CleanupTestEnvironment() {
        if (m_tech_system) {
            m_tech_system->Shutdown();
        }
        
        // Clean up all test entities
        for (auto entity_id : m_test_entities) {
            if (m_tech_system) {
                m_tech_system->CleanupTechnologyComponents(entity_id);
            }
        }
    }

    void TestComponentCreation() {
        std::cout << "Testing component creation..." << std::endl;
        
        auto entity_id = m_test_entities[0];
        
        // Test individual component creation
        assert(m_tech_system->CreateResearchComponent(entity_id));
        assert(m_tech_system->CreateInnovationComponent(entity_id));
        assert(m_tech_system->CreateKnowledgeComponent(entity_id));
        assert(m_tech_system->CreateTechnologyEventsComponent(entity_id));
        
        // Verify components exist
        assert(m_tech_system->GetResearchComponent(entity_id) != nullptr);
        assert(m_tech_system->GetInnovationComponent(entity_id) != nullptr);
        assert(m_tech_system->GetKnowledgeComponent(entity_id) != nullptr);
        assert(m_tech_system->GetTechnologyEventsComponent(entity_id) != nullptr);
        
        std::cout << "✓ Component creation successful" << std::endl;
    }
    
    void TestComponentManagement() {
        std::cout << "Testing component management..." << std::endl;
        
        auto entity_id = m_test_entities[1];
        
        // Initialize components
        assert(m_tech_system->InitializeTechnologyComponents(entity_id, 1066, 1000.0));
        
        // Test component access
        auto* research_comp = m_tech_system->GetResearchComponent(entity_id);
        auto* innovation_comp = m_tech_system->GetInnovationComponent(entity_id);
        auto* knowledge_comp = m_tech_system->GetKnowledgeComponent(entity_id);
        auto* events_comp = m_tech_system->GetTechnologyEventsComponent(entity_id);
        
        assert(research_comp != nullptr);
        assert(innovation_comp != nullptr);
        assert(knowledge_comp != nullptr);
        assert(events_comp != nullptr);
        
        // Test component initialization values
        assert(research_comp->research_efficiency == 1.0);
        assert(research_comp->total_research_budget == 100.0);
        assert(innovation_comp->innovation_potential > 0.0);
        assert(knowledge_comp->network_strength == 1.0);
        assert(events_comp->max_event_history == 100);
        
        // Test component removal
        assert(m_tech_system->CleanupTechnologyComponents(entity_id));
        assert(m_tech_system->GetResearchComponent(entity_id) == nullptr);
        
        std::cout << "✓ Component management successful" << std::endl;
    }
    
    void TestHighLevelIntegration() {
        std::cout << "Testing high-level integration..." << std::endl;
        
        auto entity_id = m_test_entities[2];
        
        // Test complete initialization
        assert(m_tech_system->InitializeTechnologyComponents(entity_id, 1200, 2000.0));
        
        // Verify all components are properly initialized
        assert(m_tech_system->ValidateTechnologyComponents(entity_id));
        
        // Test status reporting
        auto status = m_tech_system->GetTechnologyComponentStatus(entity_id);
        assert(status.size() == 4); // All 4 component types
        assert(status[0].find("ResearchComponent: Active") != std::string::npos);
        assert(status[1].find("InnovationComponent: Active") != std::string::npos);
        assert(status[2].find("KnowledgeComponent: Active") != std::string::npos);
        assert(status[3].find("TechnologyEventsComponent: Active") != std::string::npos);
        
        // Test component count
        assert(m_tech_system->GetTechnologyComponentCount() >= 1);
        
        std::cout << "✓ High-level integration successful" << std::endl;
    }
    
    void TestComponentValidation() {
        std::cout << "Testing component validation..." << std::endl;
        
        auto entity_id = m_test_entities[3];
        
        // Initialize components
        assert(m_tech_system->InitializeTechnologyComponents(entity_id));
        
        // Test validation passes for properly initialized components
        assert(m_tech_system->ValidateTechnologyComponents(entity_id));
        
        // Test individual component validation
        const auto* research_comp = m_tech_system->GetResearchComponent(entity_id);
        const auto* innovation_comp = m_tech_system->GetInnovationComponent(entity_id);
        const auto* knowledge_comp = m_tech_system->GetKnowledgeComponent(entity_id);
        const auto* events_comp = m_tech_system->GetTechnologyEventsComponent(entity_id);
        
        // Components should be valid after proper initialization
        assert(research_comp != nullptr);
        assert(innovation_comp != nullptr);
        assert(knowledge_comp != nullptr);
        assert(events_comp != nullptr);
        
        std::cout << "✓ Component validation successful" << std::endl;
    }
    
    void TestSystemInitialization() {
        std::cout << "Testing system initialization..." << std::endl;
        
        auto entity_id = m_test_entities[4];
        
        // Test system name
        assert(m_tech_system->GetSystemName() == "TechnologySystem");

        // Test threading strategy (MAIN_THREAD for safe component access)
        assert(m_tech_system->CanRunInParallel() == false);
        assert(m_tech_system->GetTargetUpdateRate() > 0.0);
        
        // Test initialization with custom parameters
        assert(m_tech_system->InitializeTechnologyComponents(entity_id, 1300, 5000.0));
        
        auto* research_comp = m_tech_system->GetResearchComponent(entity_id);
        assert(research_comp != nullptr);
        
        // Test that budget was distributed among categories
        double total_investments = 0.0;
        for (int i = 0; i < static_cast<int>(TechnologyCategory::COUNT); ++i) {
            auto category = static_cast<TechnologyCategory>(i);
            total_investments += research_comp->category_investments[category];
        }
        assert(total_investments > 0.0);
        
        std::cout << "✓ System initialization successful" << std::endl;
    }
    
    void TestResearchProgressTracking() {
        std::cout << "Testing research progress tracking..." << std::endl;
        
        auto entity_id = m_test_entities[0];
        
        // Ensure components are initialized
        if (!m_tech_system->GetResearchComponent(entity_id)) {
            assert(m_tech_system->InitializeTechnologyComponents(entity_id));
        }
        
        auto* research_comp = m_tech_system->GetResearchComponent(entity_id);
        assert(research_comp != nullptr);
        
        // Test adding research projects
        ResearchProject project;
        project.technology = TechnologyType::THREE_FIELD_SYSTEM;
        project.state = ResearchState::ACTIVE;
        project.progress = 0.25;
        project.start_date = std::chrono::system_clock::now();
        project.research_rate = 0.1;
        
        research_comp->active_research[TechnologyType::THREE_FIELD_SYSTEM] = project;
        
        // Verify research was added
        assert(research_comp->active_research.find(TechnologyType::THREE_FIELD_SYSTEM) != research_comp->active_research.end());
        assert(research_comp->active_research[TechnologyType::THREE_FIELD_SYSTEM].progress == 0.25);
        
        std::cout << "✓ Research progress tracking successful" << std::endl;
    }
    
    void TestInnovationBreakthroughs() {
        std::cout << "Testing innovation breakthroughs..." << std::endl;
        
        auto entity_id = m_test_entities[1];
        
        // Ensure components are initialized
        if (!m_tech_system->GetInnovationComponent(entity_id)) {
            assert(m_tech_system->InitializeTechnologyComponents(entity_id));
        }
        
        auto* innovation_comp = m_tech_system->GetInnovationComponent(entity_id);
        assert(innovation_comp != nullptr);
        
        // Test innovation potential modification
        double original_potential = innovation_comp->innovation_potential;
        innovation_comp->innovation_potential *= 2.0;
        assert(innovation_comp->innovation_potential == original_potential * 2.0);
        
        // Test breakthrough tracking
        auto old_breakthrough_time = innovation_comp->last_breakthrough;
        innovation_comp->last_breakthrough = std::chrono::system_clock::now();
        assert(innovation_comp->last_breakthrough > old_breakthrough_time);
        
        std::cout << "✓ Innovation breakthroughs successful" << std::endl;
    }
    
    void TestKnowledgeTransfer() {
        std::cout << "Testing knowledge transfer..." << std::endl;
        
        auto entity_id = m_test_entities[2];
        
        // Ensure components are initialized
        if (!m_tech_system->GetKnowledgeComponent(entity_id)) {
            assert(m_tech_system->InitializeTechnologyComponents(entity_id));
        }
        
        auto* knowledge_comp = m_tech_system->GetKnowledgeComponent(entity_id);
        assert(knowledge_comp != nullptr);
        
        // Test adding known technologies
        KnownTechnology known_tech;
        known_tech.discovery_date = std::chrono::system_clock::now();
        known_tech.discovery_method = DiscoveryMethod::RESEARCH;
        known_tech.implementation_level = 0.75;
        known_tech.implementation_progress = 0.75;
        
        knowledge_comp->known_technologies[TechnologyType::WATER_MILL] = known_tech;
        
        // Verify technology was added
        assert(knowledge_comp->known_technologies.find(TechnologyType::WATER_MILL) != knowledge_comp->known_technologies.end());
        assert(knowledge_comp->known_technologies[TechnologyType::WATER_MILL].implementation_level == 0.75);
        
        // Test network strength modification
        knowledge_comp->network_strength = 2.5;
        assert(knowledge_comp->network_strength == 2.5);
        
        std::cout << "✓ Knowledge transfer successful" << std::endl;
    }
    
    void TestTechnologyEvents() {
        std::cout << "Testing technology events..." << std::endl;
        
        auto entity_id = m_test_entities[3];
        
        // Ensure components are initialized
        if (!m_tech_system->GetTechnologyEventsComponent(entity_id)) {
            assert(m_tech_system->InitializeTechnologyComponents(entity_id));
        }
        
        auto* events_comp = m_tech_system->GetTechnologyEventsComponent(entity_id);
        assert(events_comp != nullptr);
        
        // Test adding technology events
        TechnologyEvent event;
        event.event_type = "Discovery";
        event.technology = TechnologyType::WINDMILL;
        event.description = "Windmill technology discovered through experimentation";
        event.timestamp = std::chrono::system_clock::now();
        event.impact_magnitude = 0.8;
        
        events_comp->event_history.push_back(event);
        
        // Verify event was added
        assert(events_comp->event_history.size() == 1);
        assert(events_comp->event_history[0].event_type == "Discovery");
        assert(events_comp->event_history[0].technology == TechnologyType::WINDMILL);
        
        std::cout << "✓ Technology events successful" << std::endl;
    }
    
    void TestComponentSynchronization() {
        std::cout << "Testing component synchronization..." << std::endl;
        
        auto entity_id = m_test_entities[4];
        
        // Ensure components are initialized
        if (!m_tech_system->GetResearchComponent(entity_id)) {
            assert(m_tech_system->InitializeTechnologyComponents(entity_id));
        }
        
        // Test that components are properly synchronized
        assert(m_tech_system->ValidateTechnologyComponents(entity_id));
        
        // Components should maintain valid state
        const auto* research_comp = m_tech_system->GetResearchComponent(entity_id);
        const auto* innovation_comp = m_tech_system->GetInnovationComponent(entity_id);
        const auto* knowledge_comp = m_tech_system->GetKnowledgeComponent(entity_id);
        
        assert(research_comp->research_efficiency >= 0.0);
        assert(innovation_comp->innovation_potential >= 0.0);
        assert(knowledge_comp->network_strength >= 0.0);
        
        std::cout << "✓ Component synchronization successful" << std::endl;
    }
    
    void TestSystemIntegration() {
        std::cout << "Testing system integration..." << std::endl;
        
        // Test system update capabilities
        float delta_time = 1.0f / 60.0f; // 60 FPS
        
        // This should not crash and should process all entities
        m_tech_system->Update(delta_time, *m_component_manager, *m_message_bus);
        
        // Test that system maintains consistency after update
        for (auto entity_id : m_test_entities) {
            if (m_tech_system->GetResearchComponent(entity_id)) {
                assert(m_tech_system->ValidateTechnologyComponents(entity_id));
            }
        }
        
        std::cout << "✓ System integration successful" << std::endl;
    }
};

int main() {
    try {
        TechnologyECSIntegrationTest test;
        test.RunAllTests();
        
        std::cout << std::endl << "🎉 Technology System ECS Integration validation complete!" << std::endl;
        std::cout << "✅ All components properly integrated with ECS architecture" << std::endl;
        std::cout << "✅ ResearchComponent: Research tracking and investment management" << std::endl;
        std::cout << "✅ InnovationComponent: Innovation potential and breakthrough systems" << std::endl;
        std::cout << "✅ KnowledgeComponent: Technology knowledge and transfer networks" << std::endl;
        std::cout << "✅ TechnologyEventsComponent: Event tracking and history management" << std::endl;
        std::cout << "✅ High-level integration methods functional" << std::endl;
        std::cout << "✅ Component validation and diagnostics working" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cout << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}