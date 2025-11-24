// ============================================================================
// test_administrative_components_simple.cpp - Simple Administrative Components Test
// Created: October 11, 2025 - Administrative Components Validation
// Location: src/test_administrative_components_simple.cpp
// ============================================================================

#include "game/administration/AdministrativeComponents.h"
#include <iostream>
#include <cassert>

using namespace game::administration;

void TestAdministrativeComponents() {
    std::cout << "[AdministrativeComponentTest] Testing Administrative Components directly" << std::endl;

    // Test GovernanceComponent creation
    GovernanceComponent governance_comp;
    governance_comp.governance_type = GovernanceType::CENTRALIZED;
    governance_comp.administrative_efficiency = 0.75;
    governance_comp.tax_collection_efficiency = 0.8;
    governance_comp.bureaucratic_capacity = 150.0;

    assert(governance_comp.governance_type == GovernanceType::CENTRALIZED);
    assert(governance_comp.administrative_efficiency == 0.75);
    assert(governance_comp.tax_collection_efficiency == 0.8);
    assert(governance_comp.bureaucratic_capacity == 150.0);

    std::cout << "[AdministrativeComponentTest] ✅ GovernanceComponent test passed" << std::endl;

    // Test BureaucracyComponent creation
    BureaucracyComponent bureau_comp;
    bureau_comp.bureaucracy_level = 3;
    bureau_comp.scribes_employed = 15;
    bureau_comp.clerks_employed = 9;
    bureau_comp.record_keeping_quality = 0.7;
    bureau_comp.corruption_level = 0.15;

    assert(bureau_comp.bureaucracy_level == 3);
    assert(bureau_comp.scribes_employed == 15);
    assert(bureau_comp.clerks_employed == 9);
    assert(bureau_comp.record_keeping_quality == 0.7);
    assert(bureau_comp.corruption_level == 0.15);

    std::cout << "[AdministrativeComponentTest] ✅ BureaucracyComponent test passed" << std::endl;

    // Test LawComponent creation
    LawComponent law_comp;
    law_comp.primary_law_system = LawType::CIVIL_LAW;
    law_comp.judges_appointed = 4;
    law_comp.courts_established = 2;
    law_comp.law_enforcement_effectiveness = 0.65;
    law_comp.crime_rate = 0.25;

    assert(law_comp.primary_law_system == LawType::CIVIL_LAW);
    assert(law_comp.judges_appointed == 4);
    assert(law_comp.courts_established == 2);
    assert(law_comp.law_enforcement_effectiveness == 0.65);
    assert(law_comp.crime_rate == 0.25);

    std::cout << "[AdministrativeComponentTest] ✅ LawComponent test passed" << std::endl;

    // Test AdministrativeEventsComponent creation
    AdministrativeEventsComponent events_comp;
    events_comp.active_appointments.push_back("Appointed new tax collector");
    events_comp.reform_initiatives.push_back("Administrative efficiency reform");
    events_comp.administrative_reputation = 0.72;
    events_comp.public_trust = 0.68;

    assert(events_comp.active_appointments.size() == 1);
    assert(events_comp.reform_initiatives.size() == 1);
    assert(events_comp.administrative_reputation == 0.72);
    assert(events_comp.public_trust == 0.68);
    assert(events_comp.active_appointments[0] == "Appointed new tax collector");

    std::cout << "[AdministrativeComponentTest] ✅ AdministrativeEventsComponent test passed" << std::endl;

    // Test AdministrativeOfficial structure
    // Constructor: AdministrativeOfficial(id, name, type, province)
    AdministrativeOfficial official(42, "Sir Edmund", OfficialType::TAX_COLLECTOR, 1);

    // Verify constructor set the values correctly
    assert(official.name == "Sir Edmund");
    assert(official.official_id == 42);
    assert(official.type == OfficialType::TAX_COLLECTOR);
    assert(official.assigned_province == 1);

    // Constructor sets these randomly, so just verify they're in valid ranges
    assert(official.competence >= 0.0 && official.competence <= 1.0);
    assert(official.loyalty >= 0.0 && official.loyalty <= 1.0);
    assert(official.efficiency >= 0.0 && official.efficiency <= 1.0);
    assert(official.corruption_resistance >= 0.0 && official.corruption_resistance <= 1.0);
    assert(official.satisfaction >= 0.0 && official.satisfaction <= 1.0);
    assert(official.age >= 25 && official.age <= 44);

    std::cout << "[AdministrativeComponentTest] ✅ AdministrativeOfficial structure test passed" << std::endl;

    // Test AdministrativeEvent structure
    AdministrativeEvent event;
    event.event_id = 123;
    event.event_title = "Corruption Investigation";
    event.event_type = "corruption";
    event.urgency_level = 0.8;
    event.importance_level = 0.9;
    event.requires_immediate_attention = true;

    assert(event.event_id == 123);
    assert(event.event_title == "Corruption Investigation");
    assert(event.event_type == "corruption");
    assert(event.urgency_level == 0.8);
    assert(event.importance_level == 0.9);
    assert(event.requires_immediate_attention == true);

    std::cout << "[AdministrativeComponentTest] ✅ AdministrativeEvent structure test passed" << std::endl;

    // Test enum values
    assert(static_cast<int>(OfficialType::TAX_COLLECTOR) == 0);
    assert(static_cast<int>(OfficialType::TRADE_MINISTER) == 1);
    assert(static_cast<int>(GovernanceType::FEUDAL) == 0);
    assert(static_cast<int>(GovernanceType::CENTRALIZED) == 1);
    assert(static_cast<int>(LawType::COMMON_LAW) == 0);
    assert(static_cast<int>(LawType::CIVIL_LAW) == 1);

    std::cout << "[AdministrativeComponentTest] ✅ Enum values test passed" << std::endl;

    std::cout << "[AdministrativeComponentTest] ✅ ALL ADMINISTRATIVE COMPONENT TESTS PASSED" << std::endl;
}

int main() {
    try {
        TestAdministrativeComponents();
        std::cout << "✅ Administrative Components Test Suite: SUCCESS" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cout << "❌ Administrative Components Test Suite: FAILED - " << e.what() << std::endl;
        return 1;
    }
}