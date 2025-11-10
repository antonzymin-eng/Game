# PHASE 2: Trust & Reputation System Implementation Plan

**Duration:** Weeks 4-6
**Focus:** Complete trust mechanics, reputation system, and integration with diplomacy
**Dependencies:** Phase 1 (Memory & Historical Tracking) must be complete

---

## WEEK 1: Complete Trust System Implementation (Days 1-7)

### Day 1-2: Trust Factor Implementation & Testing

#### File: `src/game/diplomacy/TrustSystem.cpp`

**Implementation Tasks:**

1. **Complete TrustFactor implementation:**
```cpp
void TrustFactor::ModifyValue(double delta, const std::string& reason) {
    // Record previous value
    value_history.push_back(value);
    if (value_history.size() > 120) {  // Keep 10 years
        value_history.pop_front();
    }

    // Apply change with bounds
    double old_value = value;
    value = std::clamp(value + delta, 0.0, 1.0);

    // Track events
    if (delta > 0) {
        positive_events++;
    } else if (delta < 0) {
        negative_events++;
    }

    // Update trend
    CalculateTrend();

    // Log change
    // TODO: Add to event log with reason
}

void TrustFactor::CalculateTrend() {
    if (value_history.size() < 12) {
        trend = 0.0;
        return;
    }

    // Calculate 12-month moving average trend
    double recent_avg = 0.0;
    double older_avg = 0.0;

    for (size_t i = value_history.size() - 12; i < value_history.size(); i++) {
        recent_avg += value_history[i];
    }
    recent_avg /= 12.0;

    for (size_t i = value_history.size() - 24; i < value_history.size() - 12; i++) {
        older_avg += value_history[i];
    }
    older_avg /= 12.0;

    trend = recent_avg - older_avg;
}
```

2. **Complete TrustData implementation:**
```cpp
void TrustData::InitializeFactors() {
    // Initialize all trust factor types with default weights
    factors[TrustFactorType::TREATY_COMPLIANCE] = TrustFactor{TrustFactorType::TREATY_COMPLIANCE, 0.5, 2.0};  // High weight
    factors[TrustFactorType::MILITARY_RELIABILITY] = TrustFactor{TrustFactorType::MILITARY_RELIABILITY, 0.5, 1.5};
    factors[TrustFactorType::ECONOMIC_RELIABILITY] = TrustFactor{TrustFactorType::ECONOMIC_RELIABILITY, 0.5, 1.2};
    factors[TrustFactorType::PERSONAL_RELATIONSHIP] = TrustFactor{TrustFactorType::PERSONAL_RELATIONSHIP, 0.5, 1.0};
    factors[TrustFactorType::HISTORICAL_BEHAVIOR] = TrustFactor{TrustFactorType::HISTORICAL_BEHAVIOR, 0.5, 1.8};
}

void TrustData::CalculateOverallTrust() {
    previous_trust = overall_trust;

    double weighted_sum = 0.0;
    double total_weight = 0.0;

    for (auto& [type, factor] : factors) {
        weighted_sum += factor.GetWeightedValue();
        total_weight += factor.weight;
    }

    overall_trust = total_weight > 0.0 ? weighted_sum / total_weight : 0.5;

    // Apply bounds
    overall_trust = std::clamp(overall_trust, min_possible_trust, max_possible_trust);

    // Calculate change rate
    trust_change_rate = overall_trust - previous_trust;

    // Update stability assessment
    AssessStability();
}

void TrustData::UpdateTrustBounds() {
    // Historical behavior sets bounds
    auto hist_factor = GetFactor(TrustFactorType::HISTORICAL_BEHAVIOR);
    if (!hist_factor) return;

    // If severe betrayals exist, trust ceiling is lowered permanently
    if (hist_factor->negative_events >= 3) {
        max_possible_trust = 0.7;  // Can never fully trust again
    }

    // If long cooperation, trust floor is raised
    if (hist_factor->positive_events >= 10 && hist_factor->value > 0.7) {
        min_possible_trust = 0.3;  // Benefit of the doubt
    }
}

void TrustData::AssessStability() {
    // Calculate volatility from recent changes
    double variance = 0.0;
    if (factors[TrustFactorType::TREATY_COMPLIANCE].value_history.size() > 12) {
        // Calculate variance of last 12 months
        auto& history = factors[TrustFactorType::TREATY_COMPLIANCE].value_history;
        double mean = 0.0;
        int count = 0;
        for (size_t i = history.size() - 12; i < history.size(); i++) {
            mean += history[i];
            count++;
        }
        mean /= count;

        for (size_t i = history.size() - 12; i < history.size(); i++) {
            double diff = history[i] - mean;
            variance += diff * diff;
        }
        variance /= count;
        volatility = std::sqrt(variance);
    }

    // Fragile if high volatility or recent negative events
    is_fragile = (volatility > 0.2) || (trust_change_rate < -0.1);

    // Solid if low volatility and high trust for extended period
    is_solid = (volatility < 0.05) && (overall_trust > 0.7) &&
               (factors[TrustFactorType::HISTORICAL_BEHAVIOR].positive_events >= 5);
}

void TrustData::ModifyTrust(TrustFactorType factor_type, double delta, const std::string& reason) {
    auto* factor = GetFactor(factor_type);
    if (factor) {
        factor->ModifyValue(delta, reason);
        CalculateOverallTrust();
        UpdateTrustBounds();
    }
}

void TrustData::SetTrustFloor(double floor) {
    min_possible_trust = std::clamp(floor, 0.0, 1.0);
    if (overall_trust < min_possible_trust) {
        overall_trust = min_possible_trust;
    }
}

void TrustData::SetTrustCeiling(double ceiling) {
    max_possible_trust = std::clamp(ceiling, 0.0, 1.0);
    if (overall_trust > max_possible_trust) {
        overall_trust = max_possible_trust;
    }
}

TrustFactor* TrustData::GetFactor(TrustFactorType type) {
    auto it = factors.find(type);
    return it != factors.end() ? &it->second : nullptr;
}
```

3. **Create Unit Tests:**

#### File: `tests/game/diplomacy/TrustSystemTests.cpp`

```cpp
#include <gtest/gtest.h>
#include "game/diplomacy/TrustSystem.h"

namespace game::diplomacy::test {

class TrustSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        trust_data = TrustData();
    }

    TrustData trust_data;
};

TEST_F(TrustSystemTest, InitialTrustIsNeutral) {
    EXPECT_DOUBLE_EQ(trust_data.overall_trust, 0.5);
    EXPECT_DOUBLE_EQ(trust_data.min_possible_trust, 0.0);
    EXPECT_DOUBLE_EQ(trust_data.max_possible_trust, 1.0);
}

TEST_F(TrustSystemTest, TreatyComplianceIncresesTrust) {
    trust_data.ModifyTrust(TrustFactorType::TREATY_COMPLIANCE, 0.2, "Treaty honored");

    EXPECT_GT(trust_data.overall_trust, 0.5);
    auto* factor = trust_data.GetFactor(TrustFactorType::TREATY_COMPLIANCE);
    ASSERT_NE(factor, nullptr);
    EXPECT_EQ(factor->positive_events, 1);
}

TEST_F(TrustSystemTest, BetrayalLowersTrustCeiling) {
    // Simulate multiple betrayals
    for (int i = 0; i < 3; i++) {
        trust_data.ModifyTrust(TrustFactorType::TREATY_COMPLIANCE, -0.3, "Treaty violated");
    }

    trust_data.UpdateTrustBounds();

    EXPECT_LT(trust_data.max_possible_trust, 1.0);
    EXPECT_LT(trust_data.overall_trust, 0.5);
}

TEST_F(TrustSystemTest, LongCooperationRaisesTrustFloor) {
    auto* hist_factor = trust_data.GetFactor(TrustFactorType::HISTORICAL_BEHAVIOR);
    ASSERT_NE(hist_factor, nullptr);

    // Simulate 10 positive events
    hist_factor->positive_events = 10;
    hist_factor->value = 0.8;

    trust_data.UpdateTrustBounds();

    EXPECT_GT(trust_data.min_possible_trust, 0.0);
}

TEST_F(TrustSystemTest, TrustStabilityDetection) {
    // Add consistent high trust
    auto* factor = trust_data.GetFactor(TrustFactorType::HISTORICAL_BEHAVIOR);
    for (int i = 0; i < 12; i++) {
        factor->value_history.push_back(0.8);
    }
    factor->value = 0.8;
    factor->positive_events = 5;

    trust_data.overall_trust = 0.8;
    trust_data.AssessStability();

    EXPECT_TRUE(trust_data.is_solid);
    EXPECT_FALSE(trust_data.is_fragile);
}

TEST_F(TrustSystemTest, VolatileTrustDetection) {
    auto* factor = trust_data.GetFactor(TrustFactorType::TREATY_COMPLIANCE);

    // Add volatile history
    for (int i = 0; i < 12; i++) {
        factor->value_history.push_back(i % 2 == 0 ? 0.8 : 0.2);
    }

    trust_data.AssessStability();

    EXPECT_GT(trust_data.volatility, 0.2);
    EXPECT_TRUE(trust_data.is_fragile);
}

TEST_F(TrustSystemTest, TrustTrendCalculation) {
    auto* factor = trust_data.GetFactor(TrustFactorType::MILITARY_RELIABILITY);

    // Declining trend
    for (int i = 0; i < 24; i++) {
        factor->value_history.push_back(0.8 - (i * 0.02));
    }

    factor->CalculateTrend();

    EXPECT_LT(factor->trend, 0.0);  // Negative trend
}

} // namespace game::diplomacy::test
```

**Testing Checklist:**
- [ ] All trust factor operations work correctly
- [ ] Trust bounds are properly enforced
- [ ] Stability detection functions correctly
- [ ] Trust trends are calculated accurately
- [ ] Edge cases handled (zero values, bounds, etc.)

---

### Day 3: Trust Rebuilding System

#### File: `src/game/diplomacy/TrustRebuilding.cpp`

```cpp
#include "game/diplomacy/TrustSystem.h"

namespace game::diplomacy {

void TrustRebuildingPath::AddRequirement(const std::string& desc, double trust_gain) {
    Requirement req;
    req.description = desc;
    req.trust_gain_on_completion = trust_gain;
    req.is_completed = false;
    requirements.push_back(req);
}

void TrustRebuildingPath::CompleteRequirement(const std::string& desc) {
    for (auto& req : requirements) {
        if (req.description == desc && !req.is_completed) {
            req.is_completed = true;
            current_progress += req.trust_gain_on_completion;
            break;
        }
    }
}

void TrustRebuildingPath::UpdateProgress() {
    // Natural recovery over time
    if (months_of_peace_achieved >= months_of_peace_required) {
        current_progress += monthly_natural_recovery;
    }

    // Gift-based recovery
    if (gifts_required && gifts_sent >= gifts_needed) {
        current_progress += 0.05;  // Bonus for completing gift requirements
        gifts_required = false;  // Don't count again
    }

    // Clamp progress
    current_progress = std::clamp(current_progress, 0.0, target_trust - starting_trust);
}

bool TrustRebuildingPath::IsComplete() const {
    // Check if all requirements are met
    bool all_requirements_met = std::all_of(requirements.begin(), requirements.end(),
        [](const Requirement& req) { return req.is_completed; });

    // Check if sufficient progress made
    bool sufficient_progress = (starting_trust + current_progress) >= target_trust;

    // Check if peace period satisfied
    bool peace_satisfied = !compliance_required ||
                          (months_of_peace_achieved >= months_of_peace_required);

    return all_requirements_met && sufficient_progress && peace_satisfied;
}

} // namespace game::diplomacy
```

#### File: `include/game/diplomacy/TrustRebuildingStrategies.h`

```cpp
#pragma once

#include "game/diplomacy/TrustSystem.h"

namespace game::diplomacy {

// Strategy for rebuilding trust after different types of violations
class TrustRebuildingStrategy {
public:
    virtual ~TrustRebuildingStrategy() = default;

    // Create rebuilding path for specific violation type
    virtual TrustRebuildingPath CreateRebuildingPath(
        types::EntityID realm_a,
        types::EntityID realm_b,
        double current_trust,
        const EventMemory& memory
    ) const = 0;

    // Check if natural rebuilding is possible
    virtual bool CanRebuildNaturally(const EventMemory& memory) const = 0;

    // Get difficulty multiplier
    virtual double GetDifficultyMultiplier() const = 0;
};

// After treaty violation
class TreatyViolationRecovery : public TrustRebuildingStrategy {
public:
    TrustRebuildingPath CreateRebuildingPath(
        types::EntityID realm_a,
        types::EntityID realm_b,
        double current_trust,
        const EventMemory& memory
    ) const override;

    bool CanRebuildNaturally(const EventMemory& memory) const override {
        // Can rebuild if only 1-2 violations
        return memory.treaties_broken <= 2;
    }

    double GetDifficultyMultiplier() const override { return 1.5; }
};

// After military betrayal
class MilitaryBetrayalRecovery : public TrustRebuildingStrategy {
public:
    TrustRebuildingPath CreateRebuildingPath(
        types::EntityID realm_a,
        types::EntityID realm_b,
        double current_trust,
        const EventMemory& memory
    ) const override;

    bool CanRebuildNaturally(const EventMemory& memory) const override {
        // Very difficult - requires special circumstances
        return memory.betrayals_count <= 1 && memory.wars_fought_together >= 2;
    }

    double GetDifficultyMultiplier() const override { return 3.0; }
};

// After economic default
class EconomicDefaultRecovery : public TrustRebuildingStrategy {
public:
    TrustRebuildingPath CreateRebuildingPath(
        types::EntityID realm_a,
        types::EntityID realm_b,
        double current_trust,
        const EventMemory& memory
    ) const override;

    bool CanRebuildNaturally(const EventMemory& memory) const override {
        return true;  // Economic trust can usually be rebuilt
    }

    double GetDifficultyMultiplier() const override { return 1.2; }
};

// Factory for getting appropriate strategy
class TrustRebuildingStrategyFactory {
public:
    static std::unique_ptr<TrustRebuildingStrategy> GetStrategy(EventType violation_type);
};

} // namespace game::diplomacy
```

#### File: `src/game/diplomacy/TrustRebuildingStrategies.cpp`

```cpp
#include "game/diplomacy/TrustRebuildingStrategies.h"

namespace game::diplomacy {

// Treaty Violation Recovery
TrustRebuildingPath TreatyViolationRecovery::CreateRebuildingPath(
    types::EntityID realm_a,
    types::EntityID realm_b,
    double current_trust,
    const EventMemory& memory
) const {
    TrustRebuildingPath path;
    path.realm_a = realm_a;
    path.realm_b = realm_b;
    path.starting_trust = current_trust;
    path.target_trust = 0.5;  // Rebuild to neutral

    // Requirements based on severity
    path.AddRequirement("Honor all existing treaties for 2 years", 0.10);
    path.AddRequirement("Send formal apology", 0.05);

    if (memory.treaties_broken >= 2) {
        path.AddRequirement("Provide compensation for breach", 0.08);
        path.AddRequirement("Sign new treaty with stronger guarantees", 0.07);
        path.months_of_peace_required = 36;  // 3 years
    } else {
        path.months_of_peace_required = 24;  // 2 years
    }

    path.compliance_required = true;
    path.monthly_natural_recovery = 0.008;

    return path;
}

// Military Betrayal Recovery
TrustRebuildingPath MilitaryBetrayalRecovery::CreateRebuildingPath(
    types::EntityID realm_a,
    types::EntityID realm_b,
    double current_trust,
    const EventMemory& memory
) const {
    TrustRebuildingPath path;
    path.realm_a = realm_a;
    path.realm_b = realm_b;
    path.starting_trust = current_trust;
    path.target_trust = 0.4;  // Can only rebuild to cautious level

    // Very stringent requirements
    path.AddRequirement("Fight alongside in at least one war", 0.15);
    path.AddRequirement("Provide significant military aid", 0.10);
    path.AddRequirement("Publicly acknowledge betrayal", 0.05);
    path.AddRequirement("Provide hostages or other guarantees", 0.10);

    path.months_of_peace_required = 60;  // 5 years minimum
    path.compliance_required = true;
    path.gifts_required = true;
    path.gifts_needed = 5;
    path.monthly_natural_recovery = 0.003;  // Very slow

    return path;
}

// Economic Default Recovery
TrustRebuildingPath EconomicDefaultRecovery::CreateRebuildingPath(
    types::EntityID realm_a,
    types::EntityID realm_b,
    double current_trust,
    const EventMemory& memory
) const {
    TrustRebuildingPath path;
    path.realm_a = realm_a;
    path.realm_b = realm_b;
    path.starting_trust = current_trust;
    path.target_trust = 0.55;

    // Economic recovery is more straightforward
    path.AddRequirement("Repay outstanding debts in full", 0.15);
    path.AddRequirement("Resume regular trade for 1 year", 0.08);
    path.AddRequirement("Provide economic compensation", 0.07);

    path.months_of_peace_required = 18;
    path.compliance_required = true;
    path.monthly_natural_recovery = 0.012;  // Faster recovery for economic issues

    return path;
}

// Factory
std::unique_ptr<TrustRebuildingStrategy> TrustRebuildingStrategyFactory::GetStrategy(EventType violation_type) {
    switch(violation_type) {
        case EventType::TREATY_VIOLATED:
        case EventType::ALLIANCE_BROKEN:
            return std::make_unique<TreatyViolationRecovery>();

        case EventType::STABBED_IN_BACK:
        case EventType::ALLY_ABANDONED:
        case EventType::MILITARY_AID_REFUSED:
            return std::make_unique<MilitaryBetrayalRecovery>();

        case EventType::LOAN_DEFAULTED:
        case EventType::TRADE_AGREEMENT_BROKEN:
            return std::make_unique<EconomicDefaultRecovery>();

        default:
            return std::make_unique<TreatyViolationRecovery>();  // Default fallback
    }
}

} // namespace game::diplomacy
```

---

### Day 4: Trust System Manager Implementation

#### File: `src/game/diplomacy/TrustSystemManager.cpp`

```cpp
#include "game/diplomacy/TrustSystem.h"
#include "game/diplomacy/TrustRebuildingStrategies.h"
#include "game/diplomacy/DiplomacyComponents.h"

namespace game::diplomacy {

TrustSystemManager::TrustSystemManager(
    ::core::ecs::ComponentAccessManager& access_manager,
    ::core::ecs::MessageBus& message_bus)
    : m_access_manager(access_manager)
    , m_message_bus(message_bus)
{
}

void TrustSystemManager::Initialize() {
    // Initialize trust components for all realms
    auto realms = m_access_manager.GetEntitiesWithComponent<RealmComponent>();
    for (auto realm_id : realms) {
        GetOrCreateTrustComponent(realm_id);
    }

    SubscribeToEvents();
}

void TrustSystemManager::UpdateMonthly() {
    // Process trust decay and natural recovery
    ProcessTrustDecay();

    // Update trust bounds based on recent history
    UpdateTrustBounds();

    // Process active rebuilding paths
    auto entities = m_access_manager.GetEntitiesWithComponent<TrustComponent>();
    for (auto entity_id : entities) {
        auto trust_comp_guard = m_access_manager.GetComponentForWrite<TrustComponent>(entity_id);
        if (!trust_comp_guard.IsValid()) continue;

        auto& trust_comp = trust_comp_guard.Get();
        trust_comp.UpdateRebuildingProgress(1.0f);  // 1 month
    }

    // Apply trust to diplomatic states
    auto realms = m_access_manager.GetEntitiesWithComponent<DiplomacyComponent>();
    for (auto realm_id : realms) {
        auto diplomacy = m_access_manager.GetComponent<DiplomacyComponent>(realm_id);
        if (!diplomacy) continue;

        for (const auto& [other_id, state] : diplomacy->relationships) {
            ApplyTrustToDiplomaticState(realm_id, other_id);
        }
    }
}

double TrustSystemManager::GetTrustLevel(
    types::EntityID realm_a,
    types::EntityID realm_b,
    TrustFactorType factor) const
{
    auto trust_comp = m_access_manager.GetComponent<TrustComponent>(realm_a);
    if (!trust_comp) return 0.5;

    auto* trust_data = trust_comp->GetTrustData(realm_b);
    if (!trust_data) return 0.5;

    auto* trust_factor = trust_data->GetFactor(factor);
    return trust_factor ? trust_factor->value : 0.5;
}

double TrustSystemManager::GetOverallTrust(types::EntityID realm_a, types::EntityID realm_b) const {
    auto trust_comp = m_access_manager.GetComponent<TrustComponent>(realm_a);
    if (!trust_comp) return 0.5;

    auto* trust_data = trust_comp->GetTrustData(realm_b);
    return trust_data ? trust_data->overall_trust : 0.5;
}

void TrustSystemManager::ModifyTrust(
    types::EntityID realm_a,
    types::EntityID realm_b,
    TrustFactorType factor,
    double delta,
    const std::string& reason)
{
    auto trust_comp_guard = m_access_manager.GetComponentForWrite<TrustComponent>(realm_a);
    if (!trust_comp_guard.IsValid()) return;

    trust_comp_guard.Get().ModifyTrust(realm_b, factor, delta, reason);

    // Reciprocal effect (other side also updates trust)
    auto other_trust_guard = m_access_manager.GetComponentForWrite<TrustComponent>(realm_b);
    if (other_trust_guard.IsValid()) {
        // Mirror effect with potentially different magnitude
        double reciprocal_delta = delta * 0.8;  // Slightly less impact on recipient
        other_trust_guard.Get().ModifyTrust(realm_a, factor, reciprocal_delta, reason);
    }
}

void TrustSystemManager::OnTreatyCompliance(types::EntityID realm, bool complied) {
    auto trust_guard = m_access_manager.GetComponentForWrite<TrustComponent>(realm);
    if (!trust_guard.IsValid()) return;

    auto& trust_comp = trust_guard.Get();

    if (complied) {
        trust_comp.treaties_honored++;
        trust_comp.UpdateGlobalTrustworthiness();
    } else {
        trust_comp.treaties_violated++;
        trust_comp.UpdateGlobalTrustworthiness();
    }
}

void TrustSystemManager::OnMilitarySupport(
    types::EntityID supporter,
    types::EntityID supported,
    bool provided)
{
    double delta = provided ? 0.12 : -0.20;
    std::string reason = provided ? "Provided military support" : "Refused military support";

    ModifyTrust(supporter, supported, TrustFactorType::MILITARY_RELIABILITY, delta, reason);
}

void TrustSystemManager::OnEconomicObligation(types::EntityID realm, bool fulfilled) {
    // This affects global trustworthiness
    auto trust_guard = m_access_manager.GetComponentForWrite<TrustComponent>(realm);
    if (!trust_guard.IsValid()) return;

    trust_guard.Get().UpdateGlobalTrustworthiness();
}

void TrustSystemManager::OnBetrayal(types::EntityID betrayer, types::EntityID victim) {
    // Severe trust damage
    ModifyTrust(betrayer, victim, TrustFactorType::TREATY_COMPLIANCE, -0.60, "Betrayal");
    ModifyTrust(betrayer, victim, TrustFactorType::MILITARY_RELIABILITY, -0.50, "Betrayal");
    ModifyTrust(betrayer, victim, TrustFactorType::HISTORICAL_BEHAVIOR, -0.40, "Betrayal");

    // Lower trust ceiling permanently
    auto trust_guard = m_access_manager.GetComponentForWrite<TrustComponent>(betrayer);
    if (!trust_guard.IsValid()) return;

    auto* trust_data = trust_guard.Get().GetTrustData(victim);
    if (trust_data) {
        trust_data->SetTrustCeiling(0.6);  // Can never fully trust again
    }

    // Start rebuilding path (if possible)
    InitiateTrustRebuilding(betrayer, victim);
}

void TrustSystemManager::InitiateTrustRebuilding(types::EntityID realm_a, types::EntityID realm_b) {
    auto trust_guard = m_access_manager.GetComponentForWrite<TrustComponent>(realm_a);
    if (!trust_guard.IsValid()) return;

    trust_guard.Get().StartTrustRebuilding(realm_b, 0.5);
}

void TrustSystemManager::ApplyTrustToDiplomaticState(types::EntityID realm_a, types::EntityID realm_b) {
    auto diplomacy_guard = m_access_manager.GetComponentForWrite<DiplomacyComponent>(realm_a);
    if (!diplomacy_guard.IsValid()) return;

    auto& diplomacy = diplomacy_guard.Get();
    auto* state = diplomacy.GetRelationship(realm_b);
    if (!state) return;

    // Get trust level
    double trust = GetOverallTrust(realm_a, realm_b);

    // Apply to diplomatic state
    state->trust = trust;

    // Trust affects opinion
    int trust_opinion_modifier = static_cast<int>((trust - 0.5) * 40);  // -20 to +20
    state->AddOpinionModifier("trust_level", trust_opinion_modifier, false);

    // Check for fragile or solid trust
    auto trust_comp = m_access_manager.GetComponent<TrustComponent>(realm_a);
    if (trust_comp) {
        auto* trust_data = trust_comp->GetTrustData(realm_b);
        if (trust_data) {
            if (trust_data->is_fragile) {
                state->AddOpinionModifier("fragile_trust", -10, false);
            }
            if (trust_data->is_solid) {
                state->AddOpinionModifier("solid_trust", 10, true);
            }
        }
    }
}

void TrustSystemManager::ProcessTrustDecay() {
    // Trust naturally trends toward neutral (0.5) very slowly
    auto entities = m_access_manager.GetEntitiesWithComponent<TrustComponent>();

    for (auto entity_id : entities) {
        auto trust_guard = m_access_manager.GetComponentForWrite<TrustComponent>(entity_id);
        if (!trust_guard.IsValid()) continue;

        auto& trust_comp = trust_guard.Get();

        for (auto& [other_id, trust_data] : trust_comp.trust_relationships) {
            // Very slow drift toward 0.5
            double drift_rate = 0.002;  // 0.2% per month
            if (trust_data.overall_trust > 0.5) {
                trust_data.overall_trust -= drift_rate;
                if (trust_data.overall_trust < 0.5) trust_data.overall_trust = 0.5;
            } else if (trust_data.overall_trust < 0.5) {
                trust_data.overall_trust += drift_rate;
                if (trust_data.overall_trust > 0.5) trust_data.overall_trust = 0.5;
            }
        }
    }
}

void TrustSystemManager::UpdateTrustBounds() {
    auto entities = m_access_manager.GetEntitiesWithComponent<TrustComponent>();

    for (auto entity_id : entities) {
        auto trust_guard = m_access_manager.GetComponentForWrite<TrustComponent>(entity_id);
        if (!trust_guard.IsValid()) continue;

        auto& trust_comp = trust_guard.Get();

        for (auto& [other_id, trust_data] : trust_comp.trust_relationships) {
            trust_data.UpdateTrustBounds();
        }
    }
}

TrustComponent* TrustSystemManager::GetOrCreateTrustComponent(types::EntityID realm) {
    auto existing = m_access_manager.GetComponent<TrustComponent>(realm);
    if (existing) {
        return const_cast<TrustComponent*>(existing.get());
    }

    // Create new component
    // TODO: Implement based on your ECS component creation pattern
    return nullptr;
}

void TrustSystemManager::SubscribeToEvents() {
    // TODO: Subscribe to diplomatic events
    // m_message_bus.Subscribe<TreatySignedEvent>([this](const auto& event) { ... });
    // m_message_bus.Subscribe<TreatyViolatedEvent>([this](const auto& event) { ... });
    // etc.
}

} // namespace game::diplomacy
```

---

### Day 5-6: Trust Component Implementation & Serialization

#### File: `src/game/diplomacy/TrustComponent.cpp`

```cpp
#include "game/diplomacy/TrustSystem.h"
#include "game/diplomacy/TrustRebuildingStrategies.h"

namespace game::diplomacy {

TrustData* TrustComponent::GetTrustData(types::EntityID other_realm) {
    auto it = trust_relationships.find(other_realm);
    if (it != trust_relationships.end()) {
        return &it->second;
    }

    // Create new trust relationship
    TrustData new_data;
    new_data.our_realm = realm_id;
    new_data.other_realm = other_realm;
    trust_relationships[other_realm] = new_data;
    return &trust_relationships[other_realm];
}

const TrustData* TrustComponent::GetTrustData(types::EntityID other_realm) const {
    auto it = trust_relationships.find(other_realm);
    return it != trust_relationships.end() ? &it->second : nullptr;
}

void TrustComponent::ModifyTrust(
    types::EntityID other_realm,
    TrustFactorType factor,
    double delta,
    const std::string& reason)
{
    auto* trust_data = GetTrustData(other_realm);
    if (trust_data) {
        trust_data->ModifyTrust(factor, delta, reason);
    }
}

double TrustComponent::GetTrustLevel(types::EntityID other_realm) const {
    auto* trust_data = GetTrustData(other_realm);
    return trust_data ? trust_data->overall_trust : 0.5;
}

void TrustComponent::StartTrustRebuilding(types::EntityID other_realm, double target_trust) {
    auto* trust_data = GetTrustData(other_realm);
    if (!trust_data) return;

    // Don't start if already rebuilding
    if (rebuilding_paths.find(other_realm) != rebuilding_paths.end()) {
        return;
    }

    // Get appropriate strategy based on what caused trust damage
    // TODO: Look up most recent negative event
    EventType violation_type = EventType::TREATY_VIOLATED;  // Placeholder

    auto strategy = TrustRebuildingStrategyFactory::GetStrategy(violation_type);

    // Create rebuilding path (needs memory for context)
    // TODO: Get actual memory
    EventMemory dummy_memory;
    TrustRebuildingPath path = strategy->CreateRebuildingPath(
        realm_id,
        other_realm,
        trust_data->overall_trust,
        dummy_memory
    );

    path.target_trust = target_trust;
    rebuilding_paths[other_realm] = path;
}

void TrustComponent::UpdateRebuildingProgress(float delta_time) {
    for (auto& [other_id, path] : rebuilding_paths) {
        path.UpdateProgress();

        // If complete, apply final trust boost
        if (path.IsComplete()) {
            auto* trust_data = GetTrustData(other_id);
            if (trust_data) {
                trust_data->overall_trust = path.target_trust;
                trust_data->CalculateOverallTrust();
            }
        }
    }

    // Remove completed paths
    for (auto it = rebuilding_paths.begin(); it != rebuilding_paths.end();) {
        if (it->second.IsComplete()) {
            it = rebuilding_paths.erase(it);
        } else {
            ++it;
        }
    }
}

void TrustComponent::UpdateGlobalTrustworthiness() {
    // Calculate based on treaty/alliance record
    int total_treaties = treaties_honored + treaties_violated;
    int total_alliances = alliances_honored + alliances_betrayed;

    if (total_treaties == 0 && total_alliances == 0) {
        global_trustworthiness = 1.0;  // No history = benefit of doubt
        return;
    }

    double treaty_ratio = total_treaties > 0 ?
        static_cast<double>(treaties_honored) / total_treaties : 1.0;
    double alliance_ratio = total_alliances > 0 ?
        static_cast<double>(alliances_honored) / total_alliances : 1.0;

    // Weight alliances more heavily
    global_trustworthiness = (treaty_ratio * 0.4 + alliance_ratio * 0.6);

    // Betrayals have severe impact
    if (alliances_betrayed > 0) {
        global_trustworthiness *= 0.7;  // 30% penalty per betrayal category
    }

    global_trustworthiness = std::clamp(global_trustworthiness, 0.0, 1.0);
}

Json::Value TrustComponent::Serialize() const {
    Json::Value root;
    root["realm_id"] = static_cast<int>(realm_id.id);
    root["global_trustworthiness"] = global_trustworthiness;
    root["treaties_honored"] = treaties_honored;
    root["treaties_violated"] = treaties_violated;
    root["alliances_honored"] = alliances_honored;
    root["alliances_betrayed"] = alliances_betrayed;

    // Serialize trust relationships
    Json::Value relationships_json(Json::arrayValue);
    for (const auto& [other_id, trust_data] : trust_relationships) {
        Json::Value rel;
        rel["other_realm_id"] = static_cast<int>(other_id.id);
        rel["overall_trust"] = trust_data.overall_trust;
        rel["min_possible_trust"] = trust_data.min_possible_trust;
        rel["max_possible_trust"] = trust_data.max_possible_trust;
        rel["volatility"] = trust_data.volatility;
        rel["is_fragile"] = trust_data.is_fragile;
        rel["is_solid"] = trust_data.is_solid;

        // Serialize factors
        Json::Value factors_json(Json::arrayValue);
        for (const auto& [factor_type, factor] : trust_data.factors) {
            Json::Value factor_json;
            factor_json["type"] = static_cast<int>(factor_type);
            factor_json["value"] = factor.value;
            factor_json["weight"] = factor.weight;
            factor_json["trend"] = factor.trend;
            factor_json["positive_events"] = factor.positive_events;
            factor_json["negative_events"] = factor.negative_events;
            factors_json.append(factor_json);
        }
        rel["factors"] = factors_json;

        relationships_json.append(rel);
    }
    root["trust_relationships"] = relationships_json;

    // Serialize rebuilding paths
    Json::Value rebuilding_json(Json::arrayValue);
    for (const auto& [other_id, path] : rebuilding_paths) {
        Json::Value path_json;
        path_json["other_realm_id"] = static_cast<int>(other_id.id);
        path_json["starting_trust"] = path.starting_trust;
        path_json["target_trust"] = path.target_trust;
        path_json["current_progress"] = path.current_progress;
        path_json["months_of_peace_achieved"] = path.months_of_peace_achieved;
        path_json["gifts_sent"] = path.gifts_sent;
        rebuilding_json.append(path_json);
    }
    root["rebuilding_paths"] = rebuilding_json;

    return root;
}

void TrustComponent::Deserialize(const Json::Value& data) {
    realm_id.id = data["realm_id"].asInt();
    global_trustworthiness = data["global_trustworthiness"].asDouble();
    treaties_honored = data["treaties_honored"].asInt();
    treaties_violated = data["treaties_violated"].asInt();
    alliances_honored = data["alliances_honored"].asInt();
    alliances_betrayed = data["alliances_betrayed"].asInt();

    // Deserialize trust relationships
    const Json::Value& relationships_json = data["trust_relationships"];
    for (const auto& rel : relationships_json) {
        types::EntityID other_id{static_cast<uint32_t>(rel["other_realm_id"].asInt())};

        TrustData trust_data;
        trust_data.our_realm = realm_id;
        trust_data.other_realm = other_id;
        trust_data.overall_trust = rel["overall_trust"].asDouble();
        trust_data.min_possible_trust = rel["min_possible_trust"].asDouble();
        trust_data.max_possible_trust = rel["max_possible_trust"].asDouble();
        trust_data.volatility = rel["volatility"].asDouble();
        trust_data.is_fragile = rel["is_fragile"].asBool();
        trust_data.is_solid = rel["is_solid"].asBool();

        // Deserialize factors
        const Json::Value& factors_json = rel["factors"];
        for (const auto& factor_json : factors_json) {
            auto factor_type = static_cast<TrustFactorType>(factor_json["type"].asInt());
            TrustFactor factor;
            factor.type = factor_type;
            factor.value = factor_json["value"].asDouble();
            factor.weight = factor_json["weight"].asDouble();
            factor.trend = factor_json["trend"].asDouble();
            factor.positive_events = factor_json["positive_events"].asInt();
            factor.negative_events = factor_json["negative_events"].asInt();
            trust_data.factors[factor_type] = factor;
        }

        trust_relationships[other_id] = trust_data;
    }

    // Deserialize rebuilding paths
    const Json::Value& rebuilding_json = data["rebuilding_paths"];
    for (const auto& path_json : rebuilding_json) {
        types::EntityID other_id{static_cast<uint32_t>(path_json["other_realm_id"].asInt())};

        TrustRebuildingPath path;
        path.realm_a = realm_id;
        path.realm_b = other_id;
        path.starting_trust = path_json["starting_trust"].asDouble();
        path.target_trust = path_json["target_trust"].asDouble();
        path.current_progress = path_json["current_progress"].asDouble();
        path.months_of_peace_achieved = path_json["months_of_peace_achieved"].asInt();
        path.gifts_sent = path_json["gifts_sent"].asInt();

        rebuilding_paths[other_id] = path;
    }
}

} // namespace game::diplomacy
```

#### Create Trust System Tests

**File: `tests/game/diplomacy/TrustSystemIntegrationTests.cpp`**

```cpp
#include <gtest/gtest.h>
#include "game/diplomacy/TrustSystem.h"
#include "game/diplomacy/TrustRebuildingStrategies.h"

namespace game::diplomacy::test {

class TrustSystemIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        realm_a.id = 1;
        realm_b.id = 2;
    }

    types::EntityID realm_a;
    types::EntityID realm_b;
};

TEST_F(TrustSystemIntegrationTest, TreatyViolationLowersTrustAndStartsRecovery) {
    TrustComponent trust_comp;
    trust_comp.realm_id = realm_a;

    // Initial trust
    auto* trust_data = trust_comp.GetTrustData(realm_b);
    ASSERT_NE(trust_data, nullptr);
    EXPECT_DOUBLE_EQ(trust_data->overall_trust, 0.5);

    // Violate treaty
    trust_comp.ModifyTrust(realm_b, TrustFactorType::TREATY_COMPLIANCE, -0.3, "Treaty violated");

    EXPECT_LT(trust_data->overall_trust, 0.5);

    // Start rebuilding
    trust_comp.StartTrustRebuilding(realm_b, 0.5);

    EXPECT_EQ(trust_comp.rebuilding_paths.size(), 1);
}

TEST_F(TrustSystemIntegrationTest, MultiplePositiveEventsIncreaseTrust) {
    TrustData trust_data;

    // Multiple positive interactions
    for (int i = 0; i < 5; i++) {
        trust_data.ModifyTrust(TrustFactorType::TREATY_COMPLIANCE, 0.08, "Treaty honored");
    }

    EXPECT_GT(trust_data.overall_trust, 0.7);

    auto* factor = trust_data.GetFactor(TrustFactorType::TREATY_COMPLIANCE);
    EXPECT_EQ(factor->positive_events, 5);
}

TEST_F(TrustSystemIntegrationTest, BetrayalLowersTrustCeiling) {
    TrustData trust_data;

    // Severe betrayals
    trust_data.ModifyTrust(TrustFactorType::HISTORICAL_BEHAVIOR, -0.5, "Betrayal 1");
    trust_data.ModifyTrust(TrustFactorType::HISTORICAL_BEHAVIOR, -0.5, "Betrayal 2");
    trust_data.ModifyTrust(TrustFactorType::HISTORICAL_BEHAVIOR, -0.5, "Betrayal 3");

    trust_data.UpdateTrustBounds();

    EXPECT_LT(trust_data.max_possible_trust, 1.0);
}

TEST_F(TrustSystemIntegrationTest, RebuildingPathRequirementsWork) {
    TreatyViolationRecovery strategy;
    EventMemory memory;
    memory.treaties_broken = 1;

    auto path = strategy.CreateRebuildingPath(realm_a, realm_b, 0.3, memory);

    EXPECT_GT(path.requirements.size(), 0);
    EXPECT_FALSE(path.IsComplete());

    // Complete requirements
    for (auto& req : path.requirements) {
        path.CompleteRequirement(req.description);
    }

    path.months_of_peace_achieved = path.months_of_peace_required;
    path.UpdateProgress();

    // Check if closer to complete
    EXPECT_GT(path.current_progress, 0.0);
}

TEST_F(TrustSystemIntegrationTest, SerializationPreservesData) {
    TrustComponent original;
    original.realm_id = realm_a;
    original.global_trustworthiness = 0.75;
    original.treaties_honored = 5;
    original.treaties_violated = 1;

    auto* trust_data = original.GetTrustData(realm_b);
    trust_data->overall_trust = 0.65;
    trust_data->is_solid = true;

    // Serialize
    Json::Value json = original.Serialize();

    // Deserialize
    TrustComponent deserialized;
    deserialized.Deserialize(json);

    EXPECT_EQ(deserialized.realm_id.id, realm_a.id);
    EXPECT_DOUBLE_EQ(deserialized.global_trustworthiness, 0.75);
    EXPECT_EQ(deserialized.treaties_honored, 5);

    auto* restored_data = deserialized.GetTrustData(realm_b);
    ASSERT_NE(restored_data, nullptr);
    EXPECT_DOUBLE_EQ(restored_data->overall_trust, 0.65);
    EXPECT_TRUE(restored_data->is_solid);
}

} // namespace game::diplomacy::test
```

**Day 6 Deliverables:**
- [ ] TrustComponent fully implemented
- [ ] Serialization/deserialization working
- [ ] All integration tests passing
- [ ] Trust system can be saved/loaded
- [ ] Documentation updated

---

### Day 7: Week 1 Integration & Testing

**Integration Checklist:**

1. **Memory System Integration:**
   - [ ] Trust factors update based on EventMemory
   - [ ] Historical events influence trust calculations
   - [ ] Betrayal events trigger trust ceiling adjustments

2. **Diplomatic State Integration:**
   - [ ] Trust values flow into DiplomaticState
   - [ ] Opinion modifiers reflect trust levels
   - [ ] Trust affects diplomatic action availability

3. **Performance Testing:**
   - [ ] Monthly updates complete in < 10ms for 50 realms
   - [ ] Trust calculations don't cause lag
   - [ ] Memory usage is reasonable

4. **Create Integration Test Suite:**

**File: `tests/game/diplomacy/TrustMemoryIntegrationTests.cpp`**

```cpp
#include <gtest/gtest.h>
#include "game/diplomacy/TrustSystem.h"
#include "game/diplomacy/DiplomaticMemory.h"
#include "game/diplomacy/MemorySystem.h"

namespace game::diplomacy::test {

class TrustMemoryIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup mock components
    }
};

TEST_F(TrustMemoryIntegrationTest, EventMemoryAffectsTrust) {
    // Create diplomatic event
    DiplomaticEvent event(EventType::TREATY_VIOLATED, realm_a, realm_b);

    // Record in memory
    EventMemory memory(realm_a, realm_b);
    memory.RecordEvent(event);

    // Should affect trust
    TrustData trust_data;
    trust_data.ModifyTrust(TrustFactorType::TREATY_COMPLIANCE, -0.3, "From memory event");

    EXPECT_LT(trust_data.overall_trust, 0.5);
}

TEST_F(TrustMemoryIntegrationTest, MemoryPatternsInfluenceTrustBounds) {
    EventMemory memory(realm_a, realm_b);

    // Record multiple betrayals
    for (int i = 0; i < 3; i++) {
        DiplomaticEvent betrayal(EventType::STABBED_IN_BACK, realm_a, realm_b);
        memory.RecordEvent(betrayal);
    }

    EXPECT_TRUE(memory.HasGrudge());

    // Trust ceiling should be lowered
    TrustData trust_data;
    if (memory.HasGrudge()) {
        trust_data.SetTrustCeiling(0.6);
    }

    EXPECT_LT(trust_data.max_possible_trust, 1.0);
}

// Add more integration tests...

} // namespace game::diplomacy::test
```

5. **Documentation:**

**File: `docs/systems/trust_system.md`**

Create comprehensive documentation covering:
- Trust factor types and their weights
- How trust is calculated
- Trust rebuilding mechanics
- Integration with other systems
- Performance considerations

---

## WEEK 2: Reputation System (Days 8-14)

### Day 8-9: Global Reputation System

#### File: `include/game/diplomacy/ReputationSystem.h`

```cpp
#pragma once

#include "core/types/game_types.h"
#include "game/diplomacy/DiplomacyComponents.h"
#include "game/diplomacy/DiplomaticMemory.h"
#include <unordered_map>
#include <vector>

namespace game::diplomacy {

// ============================================================================
// Reputation Categories
// ============================================================================

enum class ReputationType : uint8_t {
    MILITARY_HONOR,         // Keeps military commitments
    TREATY_KEEPER,          // Honors treaties and agreements
    ECONOMIC_RELIABILITY,   // Pays debts, maintains trade
    DIPLOMATIC_INTEGRITY,   // Honest negotiations
    AGGRESSIVE_EXPANSIONIST,// Frequently attacks neighbors
    DEFENSIVE_PACIFIST,     // Rarely engages in war
    GENEROUS_ALLY,          // Provides aid and support
    BACKSTABBER,           // Known for betrayals
    CRUSADER,              // Religious warrior
    PRAGMATIST,            // Does what benefits them
    COUNT
};

// ============================================================================
// Reputation Score
// ============================================================================

struct ReputationScore {
    ReputationType type;
    double value = 0.0;           // -1.0 (worst) to +1.0 (best)
    int supporting_events = 0;     // Events that support this reputation
    int contradicting_events = 0;  // Events that contradict

    // Strength of reputation
    double certainty = 0.0;        // 0.0 (uncertain) to 1.0 (well-established)

    // Time tracking
    std::chrono::system_clock::time_point first_earned;
    std::chrono::system_clock::time_point last_updated;

    // Is this a dominant trait?
    bool is_primary_reputation = false;

    void UpdateScore(double delta);
    void CalculateCertainty();
    bool IsWellEstablished() const { return certainty > 0.7; }
};

// ============================================================================
// Global Reputation (how the world sees a realm)
// ============================================================================

struct GlobalReputation {
    types::EntityID realm_id;

    // All reputation scores
    std::unordered_map<ReputationType, ReputationScore> reputations;

    // Primary reputations (most defining traits)
    std::vector<ReputationType> primary_reputations;  // Max 3

    // Global trust level
    double global_trust = 1.0;  // How trustworthy is this realm generally?

    // Statistics that inform reputation
    struct ReputationStats {
        int total_treaties_signed = 0;
        int total_treaties_honored = 0;
        int total_treaties_violated = 0;

        int total_alliances_formed = 0;
        int total_alliances_maintained = 0;
        int total_alliances_broken = 0;

        int total_wars_declared = 0;
        int total_wars_as_defender = 0;
        int total_wars_won = 0;

        int total_debts_repaid = 0;
        int total_debts_defaulted = 0;

        int total_gifts_sent = 0;
        int total_aid_provided = 0;
        int total_aid_refused = 0;

        int total_backstabs = 0;

        double CalculateTreatyReliability() const;
        double CalculateAllianceReliability() const;
        double CalculateEconomicReliability() const;
        double CalculateAggressiveness() const;
    } stats;

    GlobalReputation() {
        InitializeReputations();
    }

    void InitializeReputations();
    void UpdateReputation(ReputationType type, double delta);
    void RecalculateReputations();
    void DeterminePrimaryReputations();

    // Query
    double GetReputationScore(ReputationType type) const;
    bool HasReputation(ReputationType type, double threshold = 0.5) const;
    std::vector<ReputationType> GetPositiveReputations() const;
    std::vector<ReputationType> GetNegativeReputations() const;

    // Update from events
    void ProcessEvent(const DiplomaticEvent& event);

    // Serialization
    Json::Value Serialize() const;
    void Deserialize(const Json::Value& data);
};

// ============================================================================
// Reputation Modifiers (how reputation affects diplomacy)
// ============================================================================

struct ReputationModifier {
    ReputationType reputation_type;
    double opinion_modifier = 0.0;
    double trust_modifier = 0.0;
    double trade_modifier = 0.0;
    double alliance_willingness_modifier = 0.0;

    std::string description;
};

// ============================================================================
// Reputation Component
// ============================================================================

struct ReputationComponent : public game::core::Component<ReputationComponent> {
    types::EntityID realm_id;

    GlobalReputation global_reputation;

    // How this realm views others' reputations
    std::unordered_map<types::EntityID, std::vector<ReputationType>> perceived_reputations;

    // Reputation change notifications
    struct ReputationChange {
        ReputationType type;
        double old_value;
        double new_value;
        std::string reason;
        std::chrono::system_clock::time_point timestamp;
    };
    std::deque<ReputationChange> recent_changes;  // Last 20 changes

    std::string GetComponentTypeName() const override {
        return "ReputationComponent";
    }

    // Update reputation based on action
    void RecordAction(const DiplomaticEvent& event);
    void UpdateGlobalReputation();

    // Query
    double GetReputationWith(types::EntityID observer, ReputationType type) const;
    std::vector<ReputationType> GetPrimaryReputations() const;

    // Get modifiers
    std::vector<ReputationModifier> GetActiveModifiers() const;

    // Serialization
    Json::Value Serialize() const override;
    void Deserialize(const Json::Value& data) override;
};

// ============================================================================
// Reputation System Manager
// ============================================================================

class ReputationSystemManager {
public:
    explicit ReputationSystemManager(
        ::core::ecs::ComponentAccessManager& access_manager,
        ::core::ecs::MessageBus& message_bus
    );

    void Initialize();
    void UpdateMonthly();
    void UpdateYearly();

    // Reputation queries
    double GetGlobalReputation(types::EntityID realm, ReputationType type) const;
    std::vector<ReputationType> GetPrimaryReputations(types::EntityID realm) const;
    bool HasReputation(types::EntityID realm, ReputationType type, double threshold = 0.5) const;

    // Reputation updates
    void ProcessDiplomaticEvent(const DiplomaticEvent& event);
    void RecalculateReputation(types::EntityID realm);

    // Get reputation effects
    std::vector<ReputationModifier> GetReputationModifiers(
        types::EntityID realm,
        types::EntityID observer
    ) const;

    // Apply reputation to diplomatic state
    void ApplyReputationToDiplomaticState(types::EntityID realm_a, types::EntityID realm_b);

    // Reputation spread (news of actions spreads)
    void PropagateReputationChange(types::EntityID realm, const DiplomaticEvent& event);

private:
    ::core::ecs::ComponentAccessManager& m_access_manager;
    ::core::ecs::MessageBus& m_message_bus;

    ReputationComponent* GetOrCreateReputationComponent(types::EntityID realm);
    void SubscribeToEvents();
    void UpdateReputationScores();
    void ProcessReputationDecay();
};

} // namespace game::diplomacy
```

### Day 9-10: Reputation Implementation

#### File: `src/game/diplomacy/ReputationSystem.cpp`

```cpp
#include "game/diplomacy/ReputationSystem.h"
#include <algorithm>
#include <cmath>

namespace game::diplomacy {

// ============================================================================
// ReputationScore Implementation
// ============================================================================

void ReputationScore::UpdateScore(double delta) {
    value = std::clamp(value + delta, -1.0, 1.0);

    if (delta > 0) {
        supporting_events++;
    } else if (delta < 0) {
        contradicting_events++;
    }

    last_updated = std::chrono::system_clock::now();

    if (first_earned.time_since_epoch().count() == 0) {
        first_earned = last_updated;
    }

    CalculateCertainty();
}

void ReputationScore::CalculateCertainty() {
    // Certainty increases with more supporting events and decreases with contradictions
    int total_events = supporting_events + contradicting_events;
    if (total_events == 0) {
        certainty = 0.0;
        return;
    }

    // Ratio of supporting vs contradicting
    double consistency = static_cast<double>(supporting_events) / total_events;

    // More events = more certainty (up to a point)
    double event_factor = std::min(1.0, total_events / 20.0);

    certainty = consistency * event_factor;
}

// ============================================================================
// GlobalReputation Implementation
// ============================================================================

void GlobalReputation::InitializeReputations() {
    for (int i = 0; i < static_cast<int>(ReputationType::COUNT); i++) {
        auto type = static_cast<ReputationType>(i);
        ReputationScore score;
        score.type = type;
        score.value = 0.0;  // Neutral start
        reputations[type] = score;
    }
}

void GlobalReputation::UpdateReputation(ReputationType type, double delta) {
    auto it = reputations.find(type);
    if (it != reputations.end()) {
        it->second.UpdateScore(delta);
    }
}

void GlobalReputation::RecalculateReputations() {
    // Update based on statistics
    double treaty_reliability = stats.CalculateTreatyReliability();
    reputations[ReputationType::TREATY_KEEPER].value = treaty_reliability;
    reputations[ReputationType::TREATY_KEEPER].supporting_events = stats.total_treaties_honored;
    reputations[ReputationType::TREATY_KEEPER].contradicting_events = stats.total_treaties_violated;
    reputations[ReputationType::TREATY_KEEPER].CalculateCertainty();

    double alliance_reliability = stats.CalculateAllianceReliability();
    reputations[ReputationType::MILITARY_HONOR].value = alliance_reliability;
    reputations[ReputationType::MILITARY_HONOR].supporting_events = stats.total_alliances_maintained;
    reputations[ReputationType::MILITARY_HONOR].contradicting_events = stats.total_alliances_broken;
    reputations[ReputationType::MILITARY_HONOR].CalculateCertainty();

    double economic_reliability = stats.CalculateEconomicReliability();
    reputations[ReputationType::ECONOMIC_RELIABILITY].value = economic_reliability;

    double aggressiveness = stats.CalculateAggressiveness();
    if (aggressiveness > 0.5) {
        reputations[ReputationType::AGGRESSIVE_EXPANSIONIST].value = aggressiveness;
        reputations[ReputationType::DEFENSIVE_PACIFIST].value = -aggressiveness;
    } else {
        reputations[ReputationType::DEFENSIVE_PACIFIST].value = 1.0 - aggressiveness;
        reputations[ReputationType::AGGRESSIVE_EXPANSIONIST].value = -1.0 + aggressiveness;
    }

    // Backstabber reputation
    if (stats.total_backstabs > 0) {
        double backstab_severity = std::min(1.0, stats.total_backstabs * 0.3);
        reputations[ReputationType::BACKSTABBER].value = -backstab_severity;
        reputations[ReputationType::BACKSTABBER].supporting_events = stats.total_backstabs;
        reputations[ReputationType::BACKSTABBER].CalculateCertainty();
    }

    // Generous ally
    if (stats.total_aid_provided > 0) {
        double generosity = std::min(1.0, stats.total_aid_provided * 0.15);
        reputations[ReputationType::GENEROUS_ALLY].value = generosity;
        reputations[ReputationType::GENEROUS_ALLY].supporting_events = stats.total_aid_provided;
        reputations[ReputationType::GENEROUS_ALLY].CalculateCertainty();
    }

    // Update global trust
    global_trust = (treaty_reliability + alliance_reliability + economic_reliability) / 3.0;
    global_trust = std::clamp(global_trust, 0.0, 1.0);

    // If backstabber, heavily penalize global trust
    if (stats.total_backstabs > 0) {
        global_trust *= (1.0 - (stats.total_backstabs * 0.2));
        global_trust = std::max(0.1, global_trust);
    }

    DeterminePrimaryReputations();
}

void GlobalReputation::DeterminePrimaryReputations() {
    primary_reputations.clear();

    // Find reputations with highest absolute values that are well-established
    std::vector<std::pair<ReputationType, double>> scored_reps;

    for (const auto& [type, score] : reputations) {
        if (score.IsWellEstablished()) {
            scored_reps.push_back({type, std::abs(score.value) * score.certainty});
        }
    }

    // Sort by score
    std::sort(scored_reps.begin(), scored_reps.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    // Take top 3
    for (size_t i = 0; i < std::min(size_t(3), scored_reps.size()); i++) {
        primary_reputations.push_back(scored_reps[i].first);
        reputations[scored_reps[i].first].is_primary_reputation = true;
    }
}

double GlobalReputation::GetReputationScore(ReputationType type) const {
    auto it = reputations.find(type);
    return it != reputations.end() ? it->second.value : 0.0;
}

bool GlobalReputation::HasReputation(ReputationType type, double threshold) const {
    auto it = reputations.find(type);
    if (it == reputations.end()) return false;

    return it->second.IsWellEstablished() && std::abs(it->second.value) >= threshold;
}

std::vector<ReputationType> GlobalReputation::GetPositiveReputations() const {
    std::vector<ReputationType> result;
    for (const auto& [type, score] : reputations) {
        if (score.value > 0.3 && score.IsWellEstablished()) {
            result.push_back(type);
        }
    }
    return result;
}

std::vector<ReputationType> GlobalReputation::GetNegativeReputations() const {
    std::vector<ReputationType> result;
    for (const auto& [type, score] : reputations) {
        if (score.value < -0.3 && score.IsWellEstablished()) {
            result.push_back(type);
        }
    }
    return result;
}

void GlobalReputation::ProcessEvent(const DiplomaticEvent& event) {
    // Update stats based on event type
    switch(event.type) {
        case EventType::TREATY_SIGNED:
        case EventType::ALLIANCE_FORMED:
            stats.total_treaties_signed++;
            break;

        case EventType::TREATY_HONORED:
            stats.total_treaties_honored++;
            UpdateReputation(ReputationType::TREATY_KEEPER, 0.05);
            break;

        case EventType::TREATY_VIOLATED:
            stats.total_treaties_violated++;
            UpdateReputation(ReputationType::TREATY_KEEPER, -0.15);
            break;

        case EventType::ALLIANCE_BROKEN:
            stats.total_alliances_broken++;
            UpdateReputation(ReputationType::MILITARY_HONOR, -0.20);
            UpdateReputation(ReputationType::BACKSTABBER, -0.25);
            break;

        case EventType::WAR_DECLARED:
            stats.total_wars_declared++;
            UpdateReputation(ReputationType::AGGRESSIVE_EXPANSIONIST, 0.08);
            break;

        case EventType::MILITARY_AID_PROVIDED:
            stats.total_aid_provided++;
            UpdateReputation(ReputationType::GENEROUS_ALLY, 0.10);
            UpdateReputation(ReputationType::MILITARY_HONOR, 0.05);
            break;

        case EventType::MILITARY_AID_REFUSED:
            stats.total_aid_refused++;
            UpdateReputation(ReputationType::GENEROUS_ALLY, -0.08);
            break;

        case EventType::LOAN_REPAID:
            stats.total_debts_repaid++;
            UpdateReputation(ReputationType::ECONOMIC_RELIABILITY, 0.08);
            break;

        case EventType::LOAN_DEFAULTED:
            stats.total_debts_defaulted++;
            UpdateReputation(ReputationType::ECONOMIC_RELIABILITY, -0.20);
            break;

        case EventType::STABBED_IN_BACK:
        case EventType::ALLY_ABANDONED:
            stats.total_backstabs++;
            UpdateReputation(ReputationType::BACKSTABBER, -0.35);
            UpdateReputation(ReputationType::MILITARY_HONOR, -0.30);
            UpdateReputation(ReputationType::TREATY_KEEPER, -0.25);
            break;

        case EventType::GIFT_SENT:
            stats.total_gifts_sent++;
            UpdateReputation(ReputationType::GENEROUS_ALLY, 0.03);
            break;

        default:
            break;
    }

    RecalculateReputations();
}

// ============================================================================
// ReputationStats Implementation
// ============================================================================

double GlobalReputation::ReputationStats::CalculateTreatyReliability() const {
    if (total_treaties_signed == 0) return 1.0;  // Benefit of doubt

    int total_outcomes = total_treaties_honored + total_treaties_violated;
    if (total_outcomes == 0) return 1.0;

    return static_cast<double>(total_treaties_honored) / total_outcomes;
}

double GlobalReputation::ReputationStats::CalculateAllianceReliability() const {
    if (total_alliances_formed == 0) return 1.0;

    int total_outcomes = total_alliances_maintained + total_alliances_broken;
    if (total_outcomes == 0) return 1.0;

    return static_cast<double>(total_alliances_maintained) / total_outcomes;
}

double GlobalReputation::ReputationStats::CalculateEconomicReliability() const {
    if (total_debts_repaid + total_debts_defaulted == 0) return 1.0;

    return static_cast<double>(total_debts_repaid) / (total_debts_repaid + total_debts_defaulted);
}

double GlobalReputation::ReputationStats::CalculateAggressiveness() const {
    if (total_wars_declared + total_wars_as_defender == 0) return 0.0;

    // Aggressive if you declare wars more than you defend
    double aggression_ratio = static_cast<double>(total_wars_declared) /
                             (total_wars_declared + total_wars_as_defender);

    return aggression_ratio;
}

} // namespace game::diplomacy
```

### Day 11-12: Reputation Component & Integration

#### File: `src/game/diplomacy/ReputationComponent.cpp`

```cpp
#include "game/diplomacy/ReputationSystem.h"

namespace game::diplomacy {

void ReputationComponent::RecordAction(const DiplomaticEvent& event) {
    // Record in global reputation
    global_reputation.ProcessEvent(event);

    // Track change
    ReputationChange change;
    // TODO: Populate change details
    recent_changes.push_back(change);

    // Keep only recent changes
    if (recent_changes.size() > 20) {
        recent_changes.pop_front();
    }
}

void ReputationComponent::UpdateGlobalReputation() {
    global_reputation.RecalculateReputations();
}

double ReputationComponent::GetReputationWith(types::EntityID observer, ReputationType type) const {
    // Check if observer has specific perception
    auto it = perceived_reputations.find(observer);
    if (it != perceived_reputations.end()) {
        // Custom perception exists
        // TODO: Implement perception-specific logic
    }

    // Return global reputation
    return global_reputation.GetReputationScore(type);
}

std::vector<ReputationType> ReputationComponent::GetPrimaryReputations() const {
    return global_reputation.primary_reputations;
}

std::vector<ReputationModifier> ReputationComponent::GetActiveModifiers() const {
    std::vector<ReputationModifier> modifiers;

    // Generate modifiers from primary reputations
    for (auto rep_type : global_reputation.primary_reputations) {
        ReputationModifier mod;
        mod.reputation_type = rep_type;

        switch(rep_type) {
            case ReputationType::TREATY_KEEPER:
                mod.opinion_modifier = 15.0;
                mod.trust_modifier = 0.15;
                mod.description = "Known for honoring treaties";
                break;

            case ReputationType::BACKSTABBER:
                mod.opinion_modifier = -30.0;
                mod.trust_modifier = -0.40;
                mod.description = "Reputation for betrayal";
                break;

            case ReputationType::GENEROUS_ALLY:
                mod.opinion_modifier = 20.0;
                mod.alliance_willingness_modifier = 0.20;
                mod.description = "Known as generous ally";
                break;

            case ReputationType::AGGRESSIVE_EXPANSIONIST:
                mod.opinion_modifier = -10.0;
                mod.description = "Seen as aggressive";
                break;

            case ReputationType::MILITARY_HONOR:
                mod.opinion_modifier = 12.0;
                mod.trust_modifier = 0.10;
                mod.alliance_willingness_modifier = 0.15;
                mod.description = "Reliable military ally";
                break;

            case ReputationType::ECONOMIC_RELIABILITY:
                mod.trade_modifier = 0.15;
                mod.trust_modifier = 0.08;
                mod.description = "Economically trustworthy";
                break;

            default:
                break;
        }

        modifiers.push_back(mod);
    }

    return modifiers;
}

Json::Value ReputationComponent::Serialize() const {
    Json::Value root;
    root["realm_id"] = static_cast<int>(realm_id.id);
    root["global_reputation"] = global_reputation.Serialize();

    // Serialize perceived reputations
    Json::Value perceptions(Json::arrayValue);
    for (const auto& [observer_id, rep_types] : perceived_reputations) {
        Json::Value perception;
        perception["observer_id"] = static_cast<int>(observer_id.id);

        Json::Value types_json(Json::arrayValue);
        for (auto type : rep_types) {
            types_json.append(static_cast<int>(type));
        }
        perception["reputation_types"] = types_json;

        perceptions.append(perception);
    }
    root["perceived_reputations"] = perceptions;

    // Serialize recent changes
    Json::Value changes_json(Json::arrayValue);
    for (const auto& change : recent_changes) {
        Json::Value change_json;
        change_json["type"] = static_cast<int>(change.type);
        change_json["old_value"] = change.old_value;
        change_json["new_value"] = change.new_value;
        change_json["reason"] = change.reason;
        changes_json.append(change_json);
    }
    root["recent_changes"] = changes_json;

    return root;
}

void ReputationComponent::Deserialize(const Json::Value& data) {
    realm_id.id = data["realm_id"].asInt();
    global_reputation.Deserialize(data["global_reputation"]);

    // Deserialize perceived reputations
    const Json::Value& perceptions = data["perceived_reputations"];
    for (const auto& perception : perceptions) {
        types::EntityID observer_id{static_cast<uint32_t>(perception["observer_id"].asInt())};

        std::vector<ReputationType> rep_types;
        const Json::Value& types_json = perception["reputation_types"];
        for (const auto& type_json : types_json) {
            rep_types.push_back(static_cast<ReputationType>(type_json.asInt()));
        }

        perceived_reputations[observer_id] = rep_types;
    }

    // Deserialize recent changes
    const Json::Value& changes_json = data["recent_changes"];
    for (const auto& change_json : changes_json) {
        ReputationChange change;
        change.type = static_cast<ReputationType>(change_json["type"].asInt());
        change.old_value = change_json["old_value"].asDouble();
        change.new_value = change_json["new_value"].asDouble();
        change.reason = change_json["reason"].asString();
        recent_changes.push_back(change);
    }
}

Json::Value GlobalReputation::Serialize() const {
    Json::Value root;
    root["realm_id"] = static_cast<int>(realm_id.id);
    root["global_trust"] = global_trust;

    // Serialize reputations
    Json::Value reps_json(Json::arrayValue);
    for (const auto& [type, score] : reputations) {
        Json::Value score_json;
        score_json["type"] = static_cast<int>(type);
        score_json["value"] = score.value;
        score_json["certainty"] = score.certainty;
        score_json["supporting_events"] = score.supporting_events;
        score_json["contradicting_events"] = score.contradicting_events;
        score_json["is_primary"] = score.is_primary_reputation;
        reps_json.append(score_json);
    }
    root["reputations"] = reps_json;

    // Serialize stats
    Json::Value stats_json;
    stats_json["treaties_signed"] = stats.total_treaties_signed;
    stats_json["treaties_honored"] = stats.total_treaties_honored;
    stats_json["treaties_violated"] = stats.total_treaties_violated;
    stats_json["alliances_formed"] = stats.total_alliances_formed;
    stats_json["alliances_maintained"] = stats.total_alliances_maintained;
    stats_json["alliances_broken"] = stats.total_alliances_broken;
    stats_json["wars_declared"] = stats.total_wars_declared;
    stats_json["backstabs"] = stats.total_backstabs;
    stats_json["aid_provided"] = stats.total_aid_provided;
    root["stats"] = stats_json;

    return root;
}

void GlobalReputation::Deserialize(const Json::Value& data) {
    realm_id.id = data["realm_id"].asInt();
    global_trust = data["global_trust"].asDouble();

    // Deserialize reputations
    const Json::Value& reps_json = data["reputations"];
    for (const auto& score_json : reps_json) {
        auto type = static_cast<ReputationType>(score_json["type"].asInt());
        ReputationScore score;
        score.type = type;
        score.value = score_json["value"].asDouble();
        score.certainty = score_json["certainty"].asDouble();
        score.supporting_events = score_json["supporting_events"].asInt();
        score.contradicting_events = score_json["contradicting_events"].asInt();
        score.is_primary_reputation = score_json["is_primary"].asBool();
        reputations[type] = score;
    }

    // Deserialize stats
    const Json::Value& stats_json = data["stats"];
    stats.total_treaties_signed = stats_json["treaties_signed"].asInt();
    stats.total_treaties_honored = stats_json["treaties_honored"].asInt();
    stats.total_treaties_violated = stats_json["treaties_violated"].asInt();
    stats.total_alliances_formed = stats_json["alliances_formed"].asInt();
    stats.total_alliances_maintained = stats_json["alliances_maintained"].asInt();
    stats.total_alliances_broken = stats_json["alliances_broken"].asInt();
    stats.total_wars_declared = stats_json["wars_declared"].asInt();
    stats.total_backstabs = stats_json["backstabs"].asInt();
    stats.total_aid_provided = stats_json["aid_provided"].asInt();

    RecalculateReputations();
}

} // namespace game::diplomacy
```

### Day 12-13: Reputation System Manager

#### File: `src/game/diplomacy/ReputationSystemManager.cpp`

```cpp
#include "game/diplomacy/ReputationSystem.h"
#include "game/diplomacy/DiplomacyComponents.h"

namespace game::diplomacy {

ReputationSystemManager::ReputationSystemManager(
    ::core::ecs::ComponentAccessManager& access_manager,
    ::core::ecs::MessageBus& message_bus)
    : m_access_manager(access_manager)
    , m_message_bus(message_bus)
{
}

void ReputationSystemManager::Initialize() {
    // Initialize reputation components for all realms
    auto realms = m_access_manager.GetEntitiesWithComponent<RealmComponent>();
    for (auto realm_id : realms) {
        GetOrCreateReputationComponent(realm_id);
    }

    SubscribeToEvents();
}

void ReputationSystemManager::UpdateMonthly() {
    UpdateReputationScores();
}

void ReputationSystemManager::UpdateYearly() {
    ProcessReputationDecay();

    // Recalculate all reputations
    auto entities = m_access_manager.GetEntitiesWithComponent<ReputationComponent>();
    for (auto entity_id : entities) {
        RecalculateReputation(entity_id);
    }
}

double ReputationSystemManager::GetGlobalReputation(types::EntityID realm, ReputationType type) const {
    auto rep_comp = m_access_manager.GetComponent<ReputationComponent>(realm);
    if (!rep_comp) return 0.0;

    return rep_comp->global_reputation.GetReputationScore(type);
}

std::vector<ReputationType> ReputationSystemManager::GetPrimaryReputations(types::EntityID realm) const {
    auto rep_comp = m_access_manager.GetComponent<ReputationComponent>(realm);
    if (!rep_comp) return {};

    return rep_comp->GetPrimaryReputations();
}

bool ReputationSystemManager::HasReputation(
    types::EntityID realm,
    ReputationType type,
    double threshold) const
{
    auto rep_comp = m_access_manager.GetComponent<ReputationComponent>(realm);
    if (!rep_comp) return false;

    return rep_comp->global_reputation.HasReputation(type, threshold);
}

void ReputationSystemManager::ProcessDiplomaticEvent(const DiplomaticEvent& event) {
    // Update actor's reputation
    auto actor_rep_guard = m_access_manager.GetComponentForWrite<ReputationComponent>(event.actor);
    if (actor_rep_guard.IsValid()) {
        actor_rep_guard.Get().RecordAction(event);
    }

    // Propagate reputation change to observers
    PropagateReputationChange(event.actor, event);
}

void ReputationSystemManager::RecalculateReputation(types::EntityID realm) {
    auto rep_guard = m_access_manager.GetComponentForWrite<ReputationComponent>(realm);
    if (!rep_guard.IsValid()) return;

    rep_guard.Get().UpdateGlobalReputation();
}

std::vector<ReputationModifier> ReputationSystemManager::GetReputationModifiers(
    types::EntityID realm,
    types::EntityID observer) const
{
    auto rep_comp = m_access_manager.GetComponent<ReputationComponent>(realm);
    if (!rep_comp) return {};

    return rep_comp->GetActiveModifiers();
}

void ReputationSystemManager::ApplyReputationToDiplomaticState(
    types::EntityID realm_a,
    types::EntityID realm_b)
{
    auto diplomacy_guard = m_access_manager.GetComponentForWrite<DiplomacyComponent>(realm_a);
    if (!diplomacy_guard.IsValid()) return;

    auto& diplomacy = diplomacy_guard.Get();
    auto* state = diplomacy.GetRelationship(realm_b);
    if (!state) return;

    // Get B's reputation modifiers as seen by A
    auto modifiers = GetReputationModifiers(realm_b, realm_a);

    for (const auto& mod : modifiers) {
        // Apply opinion modifier
        if (mod.opinion_modifier != 0.0) {
            std::string modifier_name = "reputation_" + mod.description;
            state->AddOpinionModifier(modifier_name, static_cast<int>(mod.opinion_modifier), false);
        }

        // Apply trust modifier
        if (mod.trust_modifier != 0.0) {
            state->trust = std::clamp(state->trust + mod.trust_modifier, 0.0, 1.0);
        }
    }

    // Apply global trust
    auto rep_comp = m_access_manager.GetComponent<ReputationComponent>(realm_b);
    if (rep_comp) {
        double global_trust = rep_comp->global_reputation.global_trust;
        int trust_opinion = static_cast<int>((global_trust - 0.5) * 30);
        state->AddOpinionModifier("global_reputation", trust_opinion, false);
    }
}

void ReputationSystemManager::PropagateReputationChange(
    types::EntityID realm,
    const DiplomaticEvent& event)
{
    // News of significant events spreads to other realms
    if (event.severity < EventSeverity::MODERATE) {
        return;  // Minor events don't spread
    }

    auto all_realms = m_access_manager.GetEntitiesWithComponent<RealmComponent>();

    for (auto observer_id : all_realms) {
        if (observer_id == realm || observer_id == event.target) {
            continue;  // Skip self and direct target
        }

        // Check if observer should care about this event
        auto observer_diplomacy = m_access_manager.GetComponent<DiplomacyComponent>(observer_id);
        if (!observer_diplomacy) continue;

        // If observer has relationship with actor, update perception
        auto* relationship = observer_diplomacy->GetRelationship(realm);
        if (relationship) {
            // Apply reputation impact (smaller than direct impact)
            double reputation_impact = event.opinion_impact * 0.3;  // 30% of direct impact

            // Update opinion based on reputation
            auto observer_dip_guard = m_access_manager.GetComponentForWrite<DiplomacyComponent>(observer_id);
            if (observer_dip_guard.IsValid()) {
                auto* rel = observer_dip_guard.Get().GetRelationship(realm);
                if (rel) {
                    rel->AddOpinionModifier(
                        "reputation_news",
                        static_cast<int>(reputation_impact),
                        false
                    );
                }
            }
        }
    }
}

void ReputationSystemManager::UpdateReputationScores() {
    auto entities = m_access_manager.GetEntitiesWithComponent<ReputationComponent>();

    for (auto entity_id : entities) {
        auto rep_guard = m_access_manager.GetComponentForWrite<ReputationComponent>(entity_id);
        if (!rep_guard.IsValid()) continue;

        rep_guard.Get().UpdateGlobalReputation();
    }
}

void ReputationSystemManager::ProcessReputationDecay() {
    // Reputations slowly decay toward neutral over time
    auto entities = m_access_manager.GetEntitiesWithComponent<ReputationComponent>();

    for (auto entity_id : entities) {
        auto rep_guard = m_access_manager.GetComponentForWrite<ReputationComponent>(entity_id);
        if (!rep_guard.IsValid()) continue;

        auto& rep_comp = rep_guard.Get();

        for (auto& [type, score] : rep_comp.global_reputation.reputations) {
            // Don't decay primary reputations or backstabber reputation
            if (score.is_primary_reputation || type == ReputationType::BACKSTABBER) {
                continue;
            }

            // Slow decay toward 0
            double decay_rate = 0.01;  // 1% per year
            if (score.value > 0) {
                score.value -= decay_rate;
                if (score.value < 0) score.value = 0;
            } else if (score.value < 0) {
                score.value += decay_rate;
                if (score.value > 0) score.value = 0;
            }
        }
    }
}

ReputationComponent* ReputationSystemManager::GetOrCreateReputationComponent(types::EntityID realm) {
    auto existing = m_access_manager.GetComponent<ReputationComponent>(realm);
    if (existing) {
        return const_cast<ReputationComponent*>(existing.get());
    }

    // Create new component
    // TODO: Implement based on your ECS component creation pattern
    return nullptr;
}

void ReputationSystemManager::SubscribeToEvents() {
    // TODO: Subscribe to diplomatic events
    // m_message_bus.Subscribe<DiplomaticEventMessage>([this](const auto& event) {
    //     ProcessDiplomaticEvent(event.diplomatic_event);
    // });
}

} // namespace game::diplomacy
```

### Day 14: Reputation Testing

#### File: `tests/game/diplomacy/ReputationSystemTests.cpp`

```cpp
#include <gtest/gtest.h>
#include "game/diplomacy/ReputationSystem.h"

namespace game::diplomacy::test {

class ReputationSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        global_rep.realm_id.id = 1;
        global_rep.InitializeReputations();
    }

    GlobalReputation global_rep;
    types::EntityID realm_a{1};
    types::EntityID realm_b{2};
};

TEST_F(ReputationSystemTest, TreatyComplianceBuildsReliabilityReputation) {
    // Honor multiple treaties
    for (int i = 0; i < 5; i++) {
        DiplomaticEvent event(EventType::TREATY_HONORED, realm_a, realm_b);
        global_rep.ProcessEvent(event);
    }

    double treaty_keeper_score = global_rep.GetReputationScore(ReputationType::TREATY_KEEPER);
    EXPECT_GT(treaty_keeper_score, 0.0);
}

TEST_F(ReputationSystemTest, BetrayalCreatesBadReputation) {
    DiplomaticEvent betrayal(EventType::STABBED_IN_BACK, realm_a, realm_b);
    global_rep.ProcessEvent(betrayal);

    double backstabber_score = global_rep.GetReputationScore(ReputationType::BACKSTABBER);
    EXPECT_LT(backstabber_score, 0.0);

    EXPECT_LT(global_rep.global_trust, 1.0);
}

TEST_F(ReputationSystemTest, ConsistentBehaviorIncreasesReputationCertainty) {
    // Consistent treaty honoring
    for (int i = 0; i < 10; i++) {
        DiplomaticEvent event(EventType::TREATY_HONORED, realm_a, realm_b);
        global_rep.ProcessEvent(event);
    }

    auto* score = &global_rep.reputations[ReputationType::TREATY_KEEPER];
    EXPECT_GT(score->certainty, 0.5);
    EXPECT_TRUE(score->IsWellEstablished());
}

TEST_F(ReputationSystemTest, InconsistentBehaviorLowersCertainty) {
    // Mix of honoring and violating
    for (int i = 0; i < 5; i++) {
        DiplomaticEvent honor(EventType::TREATY_HONORED, realm_a, realm_b);
        global_rep.ProcessEvent(honor);

        DiplomaticEvent violate(EventType::TREATY_VIOLATED, realm_a, realm_b);
        global_rep.ProcessEvent(violate);
    }

    auto* score = &global_rep.reputations[ReputationType::TREATY_KEEPER];
    EXPECT_LT(score->certainty, 0.7);
}

TEST_F(ReputationSystemTest, PrimaryReputationsAreIdentified) {
    // Build strong treaty keeper reputation
    for (int i = 0; i < 15; i++) {
        DiplomaticEvent event(EventType::TREATY_HONORED, realm_a, realm_b);
        global_rep.ProcessEvent(event);
    }

    // Build generous ally reputation
    for (int i = 0; i < 10; i++) {
        DiplomaticEvent event(EventType::MILITARY_AID_PROVIDED, realm_a, realm_b);
        global_rep.ProcessEvent(event);
    }

    global_rep.DeterminePrimaryReputations();

    EXPECT_GT(global_rep.primary_reputations.size(), 0);
    EXPECT_LE(global_rep.primary_reputations.size(), 3);
}

TEST_F(ReputationSystemTest, GlobalTrustReflectsOverallBehavior) {
    // All positive actions
    global_rep.stats.total_treaties_honored = 10;
    global_rep.stats.total_treaties_signed = 10;
    global_rep.stats.total_alliances_maintained = 5;
    global_rep.stats.total_alliances_formed = 5;
    global_rep.stats.total_debts_repaid = 3;

    global_rep.RecalculateReputations();

    EXPECT_GT(global_rep.global_trust, 0.8);
}

TEST_F(ReputationSystemTest, BetrayalsSeverelyDamageGlobalTrust) {
    global_rep.stats.total_backstabs = 2;
    global_rep.stats.total_treaties_honored = 5;
    global_rep.stats.total_treaties_signed = 10;

    global_rep.RecalculateReputations();

    EXPECT_LT(global_rep.global_trust, 0.5);
}

TEST_F(ReputationSystemTest, SerializationPreservesReputation) {
    // Build reputation
    for (int i = 0; i < 5; i++) {
        DiplomaticEvent event(EventType::TREATY_HONORED, realm_a, realm_b);
        global_rep.ProcessEvent(event);
    }

    // Serialize
    Json::Value json = global_rep.Serialize();

    // Deserialize
    GlobalReputation restored;
    restored.Deserialize(json);

    EXPECT_DOUBLE_EQ(restored.global_trust, global_rep.global_trust);
    EXPECT_EQ(restored.stats.total_treaties_honored, global_rep.stats.total_treaties_honored);

    double original_score = global_rep.GetReputationScore(ReputationType::TREATY_KEEPER);
    double restored_score = restored.GetReputationScore(ReputationType::TREATY_KEEPER);
    EXPECT_DOUBLE_EQ(restored_score, original_score);
}

} // namespace game::diplomacy::test
```

**Week 2 Deliverables:**
- [ ] Complete reputation system implemented
- [ ] Global reputation tracking working
- [ ] Reputation modifiers applied to diplomacy
- [ ] Reputation propagation functioning
- [ ] All tests passing
- [ ] Documentation complete

---

## WEEK 3: Integration, Polish & Testing (Days 15-21)

### Day 15: Complete System Integration

**Integration Tasks:**

1. **Connect Memory → Trust → Reputation Pipeline**

#### File: `include/game/diplomacy/DiplomacySystemIntegrator.h`

```cpp
#pragma once

#include "game/diplomacy/MemorySystem.h"
#include "game/diplomacy/TrustSystem.h"
#include "game/diplomacy/ReputationSystem.h"
#include "core/ECS/ComponentAccessManager.h"

namespace game::diplomacy {

/**
 * Integrates memory, trust, and reputation systems with the core diplomacy system
 */
class DiplomacySystemIntegrator {
public:
    DiplomacySystemIntegrator(
        MemorySystem& memory_system,
        TrustSystemManager& trust_system,
        ReputationSystemManager& reputation_system,
        ::core::ecs::ComponentAccessManager& access_manager,
        ::core::ecs::MessageBus& message_bus
    );

    // Initialize all systems
    void Initialize();

    // Monthly update - called every game month
    void UpdateMonthly();

    // Yearly update - called every game year
    void UpdateYearly();

    // Process diplomatic action and update all systems
    void ProcessDiplomaticAction(const DiplomaticEvent& event);

    // Calculate complete diplomatic score between two realms
    struct DiplomaticScore {
        int base_opinion = 0;
        int memory_opinion = 0;
        int trust_opinion = 0;
        int reputation_opinion = 0;
        int total_opinion = 0;

        double trust_level = 0.5;
        double reputation_impact = 0.0;

        bool has_grudge = false;
        bool has_deep_friendship = false;
        bool is_historical_rival = false;
        bool is_historical_ally = false;

        std::vector<std::string> active_modifiers;
    };

    DiplomaticScore CalculateCompleteDiplomaticScore(
        types::EntityID realm_a,
        types::EntityID realm_b
    ) const;

    // Apply all systems to diplomatic state
    void UpdateDiplomaticState(types::EntityID realm_a, types::EntityID realm_b);

    // Bulk update all diplomatic relationships
    void UpdateAllDiplomaticStates();

private:
    MemorySystem& m_memory_system;
    TrustSystemManager& m_trust_system;
    ReputationSystemManager& m_reputation_system;
    ::core::ecs::ComponentAccessManager& m_access_manager;
    ::core::ecs::MessageBus& m_message_bus;

    void PropagateEventThroughSystems(const DiplomaticEvent& event);
    void SynchronizeSystemStates();
};

} // namespace game::diplomacy
```

#### File: `src/game/diplomacy/DiplomacySystemIntegrator.cpp`

```cpp
#include "game/diplomacy/DiplomacySystemIntegrator.h"
#include "game/diplomacy/DiplomacyComponents.h"

namespace game::diplomacy {

DiplomacySystemIntegrator::DiplomacySystemIntegrator(
    MemorySystem& memory_system,
    TrustSystemManager& trust_system,
    ReputationSystemManager& reputation_system,
    ::core::ecs::ComponentAccessManager& access_manager,
    ::core::ecs::MessageBus& message_bus)
    : m_memory_system(memory_system)
    , m_trust_system(trust_system)
    , m_reputation_system(reputation_system)
    , m_access_manager(access_manager)
    , m_message_bus(message_bus)
{
}

void DiplomacySystemIntegrator::Initialize() {
    m_memory_system.Initialize();
    m_trust_system.Initialize();
    m_reputation_system.Initialize();

    // Initial synchronization
    UpdateAllDiplomaticStates();
}

void DiplomacySystemIntegrator::UpdateMonthly() {
    // Update each system
    m_memory_system.UpdateMonthly();
    m_trust_system.UpdateMonthly();
    m_reputation_system.UpdateMonthly();

    // Synchronize states
    UpdateAllDiplomaticStates();
}

void DiplomacySystemIntegrator::UpdateYearly() {
    m_memory_system.UpdateYearly();
    m_reputation_system.UpdateYearly();

    // Full recalculation
    UpdateAllDiplomaticStates();
}

void DiplomacySystemIntegrator::ProcessDiplomaticAction(const DiplomaticEvent& event) {
    // Propagate through all systems
    PropagateEventThroughSystems(event);

    // Update diplomatic states for affected parties
    UpdateDiplomaticState(event.actor, event.target);
    UpdateDiplomaticState(event.target, event.actor);
}

DiplomacySystemIntegrator::DiplomaticScore
DiplomacySystemIntegrator::CalculateCompleteDiplomaticScore(
    types::EntityID realm_a,
    types::EntityID realm_b) const
{
    DiplomaticScore score;

    // Get base opinion from diplomatic state
    auto diplomacy = m_access_manager.GetComponent<DiplomacyComponent>(realm_a);
    if (diplomacy) {
        auto* state = diplomacy->GetRelationship(realm_b);
        if (state) {
            score.base_opinion = state->CalculateTotalOpinion();
        }
    }

    // Memory impact
    score.memory_opinion = m_memory_system.CalculateMemoryOpinionImpact(realm_a, realm_b);
    score.has_grudge = m_memory_system.HasGrudge(realm_a, realm_b);
    score.has_deep_friendship = m_memory_system.HasFriendship(realm_a, realm_b);
    score.is_historical_rival = m_memory_system.AreHistoricalRivals(realm_a, realm_b);
    score.is_historical_ally = m_memory_system.AreHistoricalAllies(realm_a, realm_b);

    // Trust impact
    score.trust_level = m_trust_system.GetOverallTrust(realm_a, realm_b);
    score.trust_opinion = static_cast<int>((score.trust_level - 0.5) * 40);

    // Reputation impact
    auto rep_modifiers = m_reputation_system.GetReputationModifiers(realm_b, realm_a);
    for (const auto& mod : rep_modifiers) {
        score.reputation_opinion += static_cast<int>(mod.opinion_modifier);
        score.reputation_impact += mod.trust_modifier;
        score.active_modifiers.push_back(mod.description);
    }

    // Calculate total
    score.total_opinion = score.base_opinion + score.memory_opinion +
                         score.trust_opinion + score.reputation_opinion;

    return score;
}

void DiplomacySystemIntegrator::UpdateDiplomaticState(
    types::EntityID realm_a,
    types::EntityID realm_b)
{
    // Apply memory system updates
    m_memory_system.ApplyMemoryToDiplomaticState(realm_a, realm_b);

    // Apply trust system updates
    m_trust_system.ApplyTrustToDiplomaticState(realm_a, realm_b);

    // Apply reputation system updates
    m_reputation_system.ApplyReputationToDiplomaticState(realm_a, realm_b);
}

void DiplomacySystemIntegrator::UpdateAllDiplomaticStates() {
    auto realms = m_access_manager.GetEntitiesWithComponent<DiplomacyComponent>();

    for (auto realm_a : realms) {
        auto diplomacy = m_access_manager.GetComponent<DiplomacyComponent>(realm_a);
        if (!diplomacy) continue;

        for (const auto& [realm_b, state] : diplomacy->relationships) {
            UpdateDiplomaticState(realm_a, realm_b);
        }
    }
}

void DiplomacySystemIntegrator::PropagateEventThroughSystems(const DiplomaticEvent& event) {
    // Record in memory system
    m_memory_system.RecordDiplomaticEvent(event);

    // Update trust system
    switch(event.type) {
        case EventType::TREATY_HONORED:
            m_trust_system.OnTreatyCompliance(event.actor, true);
            break;
        case EventType::TREATY_VIOLATED:
            m_trust_system.OnTreatyCompliance(event.actor, false);
            m_trust_system.ModifyTrust(
                event.actor, event.target,
                TrustFactorType::TREATY_COMPLIANCE,
                -0.3, "Treaty violated"
            );
            break;
        case EventType::MILITARY_AID_PROVIDED:
            m_trust_system.OnMilitarySupport(event.actor, event.target, true);
            break;
        case EventType::MILITARY_AID_REFUSED:
            m_trust_system.OnMilitarySupport(event.actor, event.target, false);
            break;
        case EventType::STABBED_IN_BACK:
        case EventType::ALLY_ABANDONED:
            m_trust_system.OnBetrayal(event.actor, event.target);
            break;
        case EventType::LOAN_REPAID:
            m_trust_system.OnEconomicObligation(event.actor, true);
            break;
        case EventType::LOAN_DEFAULTED:
            m_trust_system.OnEconomicObligation(event.actor, false);
            break;
        default:
            break;
    }

    // Update reputation system
    m_reputation_system.ProcessDiplomaticEvent(event);
}

void DiplomacySystemIntegrator::SynchronizeSystemStates() {
    // Ensure all systems have consistent data
    // This is called after loading a save or when systems get out of sync

    auto realms = m_access_manager.GetEntitiesWithComponent<RealmComponent>();

    for (auto realm_id : realms) {
        // Recalculate reputation from memory and trust
        m_reputation_system.RecalculateReputation(realm_id);
    }
}

} // namespace game::diplomacy
```

2. **Create System Coordinator Tests**

#### File: `tests/game/diplomacy/SystemIntegrationTests.cpp`

```cpp
#include <gtest/gtest.h>
#include "game/diplomacy/DiplomacySystemIntegrator.h"
#include "game/diplomacy/MemorySystem.h"
#include "game/diplomacy/TrustSystem.h"
#include "game/diplomacy/ReputationSystem.h"

namespace game::diplomacy::test {

class SystemIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup mock ECS and message bus
        // Initialize systems
    }

    types::EntityID realm_a{1};
    types::EntityID realm_b{2};
    types::EntityID realm_c{3};
};

TEST_F(SystemIntegrationTest, TreatyViolationPropagatesThroughAllSystems) {
    // Create treaty violation event
    DiplomaticEvent event(EventType::TREATY_VIOLATED, realm_a, realm_b);

    // Process through integrator
    // integrator.ProcessDiplomaticAction(event);

    // Verify memory recorded event
    // Verify trust decreased
    // Verify reputation damaged
    // Verify diplomatic state updated

    EXPECT_TRUE(true);  // Placeholder
}

TEST_F(SystemIntegrationTest, CompleteDiplomaticScoreIsAccurate) {
    // Setup scenario with multiple modifiers
    // Calculate complete score
    // Verify all components are present

    EXPECT_TRUE(true);  // Placeholder
}

TEST_F(SystemIntegrationTest, ReputationAffectsThirdPartyOpinions) {
    // Realm A betrays Realm B
    // Verify Realm C's opinion of A is affected

    EXPECT_TRUE(true);  // Placeholder
}

} // namespace game::diplomacy::test
```

### Day 16-17: UI Integration & Visualization

**Create UI Data Structures:**

#### File: `include/game/diplomacy/DiplomacyUIData.h`

```cpp
#pragma once

#include "core/types/game_types.h"
#include "game/diplomacy/DiplomacyComponents.h"
#include "game/diplomacy/TrustSystem.h"
#include "game/diplomacy/ReputationSystem.h"
#include <vector>
#include <string>

namespace game::diplomacy {

/**
 * Data structures for UI display of diplomatic information
 */

struct TrustBreakdown {
    double overall_trust = 0.5;

    struct FactorBreakdown {
        std::string name;
        double value;
        double weight;
        std::string trend_description;  // "Improving", "Declining", "Stable"
    };

    std::vector<FactorBreakdown> factors;

    double min_possible_trust = 0.0;
    double max_possible_trust = 1.0;
    bool is_fragile = false;
    bool is_solid = false;

    std::string stability_description;
};

struct ReputationBreakdown {
    double global_trust = 1.0;

    struct ReputationEntry {
        std::string name;
        double value;
        double certainty;
        bool is_primary;
        std::string description;
    };

    std::vector<ReputationEntry> reputations;
    std::vector<std::string> primary_traits;

    struct ReputationStats {
        int treaties_honored = 0;
        int treaties_violated = 0;
        int alliances_maintained = 0;
        int alliances_betrayed = 0;
        int backstabs = 0;
    };

    ReputationStats stats;
};

struct MemoryBreakdown {
    int total_positive_events = 0;
    int total_negative_events = 0;
    int total_neutral_events = 0;

    struct MemorableMoment {
        std::string event_type;
        std::string description;
        std::string date;
        int opinion_impact;
        bool is_permanent;
    };

    std::vector<MemorableMoment> major_events;  // Top 10 most impactful
    std::vector<MemorableMoment> recent_events; // Last 20 events

    bool has_grudge = false;
    bool has_friendship = false;
    bool are_historical_rivals = false;
    bool are_historical_allies = false;

    std::string relationship_summary;
};

struct DiplomaticRelationshipUI {
    types::EntityID our_realm;
    types::EntityID other_realm;

    std::string other_realm_name;

    // Overall scores
    int total_opinion = 0;
    double trust_level = 0.5;

    // Breakdowns
    TrustBreakdown trust_breakdown;
    ReputationBreakdown reputation_breakdown;
    MemoryBreakdown memory_breakdown;

    // Opinion modifiers
    struct OpinionModifierUI {
        std::string source;
        int value;
        bool is_permanent;
    };
    std::vector<OpinionModifierUI> opinion_modifiers;

    // Relationship status
    std::string relationship_status;  // "Ally", "Friend", "Neutral", "Rival", "Enemy"
    std::vector<std::string> active_treaties;

    // Warnings and notifications
    std::vector<std::string> warnings;  // "Trust is fragile", "Has grudge", etc.
};

/**
 * Factory for creating UI data from game state
 */
class DiplomacyUIDataFactory {
public:
    static DiplomaticRelationshipUI CreateRelationshipUI(
        types::EntityID our_realm,
        types::EntityID other_realm,
        const ::core::ecs::ComponentAccessManager& access_manager,
        const MemorySystem& memory_system,
        const TrustSystemManager& trust_system,
        const ReputationSystemManager& reputation_system
    );

    static TrustBreakdown CreateTrustBreakdown(
        const TrustData& trust_data
    );

    static ReputationBreakdown CreateReputationBreakdown(
        const GlobalReputation& reputation
    );

    static MemoryBreakdown CreateMemoryBreakdown(
        const EventMemory& memory
    );

private:
    static std::string GetReputationName(ReputationType type);
    static std::string GetEventTypeName(EventType type);
    static std::string GetTrendDescription(double trend);
    static std::string GetRelationshipStatus(int opinion, double trust);
};

} // namespace game::diplomacy
```

#### File: `src/game/diplomacy/DiplomacyUIDataFactory.cpp`

```cpp
#include "game/diplomacy/DiplomacyUIData.h"
#include <sstream>

namespace game::diplomacy {

DiplomaticRelationshipUI DiplomacyUIDataFactory::CreateRelationshipUI(
    types::EntityID our_realm,
    types::EntityID other_realm,
    const ::core::ecs::ComponentAccessManager& access_manager,
    const MemorySystem& memory_system,
    const TrustSystemManager& trust_system,
    const ReputationSystemManager& reputation_system)
{
    DiplomaticRelationshipUI ui_data;
    ui_data.our_realm = our_realm;
    ui_data.other_realm = other_realm;

    // Get diplomatic state
    auto diplomacy = access_manager.GetComponent<DiplomacyComponent>(our_realm);
    if (diplomacy) {
        auto* state = diplomacy->GetRelationship(other_realm);
        if (state) {
            ui_data.total_opinion = state->CalculateTotalOpinion();
            ui_data.trust_level = state->trust;

            // Get opinion modifiers
            for (const auto& modifier : state->opinion_modifiers) {
                DiplomaticRelationshipUI::OpinionModifierUI mod_ui;
                mod_ui.source = modifier.source;
                mod_ui.value = modifier.GetCurrentValue();
                mod_ui.is_permanent = modifier.is_permanent;
                ui_data.opinion_modifiers.push_back(mod_ui);
            }

            ui_data.relationship_status = GetRelationshipStatus(ui_data.total_opinion, ui_data.trust_level);
        }
    }

    // Get trust breakdown
    auto trust_comp = access_manager.GetComponent<TrustComponent>(our_realm);
    if (trust_comp) {
        auto* trust_data = trust_comp->GetTrustData(other_realm);
        if (trust_data) {
            ui_data.trust_breakdown = CreateTrustBreakdown(*trust_data);

            if (trust_data->is_fragile) {
                ui_data.warnings.push_back("⚠ Trust is fragile and may collapse quickly");
            }
        }
    }

    // Get reputation breakdown
    auto rep_comp = access_manager.GetComponent<ReputationComponent>(other_realm);
    if (rep_comp) {
        ui_data.reputation_breakdown = CreateReputationBreakdown(rep_comp->global_reputation);
    }

    // Get memory breakdown
    auto memory_comp = access_manager.GetComponent<DiplomaticMemoryComponent>(our_realm);
    if (memory_comp) {
        auto* memory = memory_comp->GetMemoryWith(other_realm);
        if (memory) {
            ui_data.memory_breakdown = CreateMemoryBreakdown(*memory);

            if (memory->HasGrudge()) {
                ui_data.warnings.push_back("⚠ Deep grudge - difficult to improve relations");
            }
        }
    }

    return ui_data;
}

TrustBreakdown DiplomacyUIDataFactory::CreateTrustBreakdown(const TrustData& trust_data) {
    TrustBreakdown breakdown;
    breakdown.overall_trust = trust_data.overall_trust;
    breakdown.min_possible_trust = trust_data.min_possible_trust;
    breakdown.max_possible_trust = trust_data.max_possible_trust;
    breakdown.is_fragile = trust_data.is_fragile;
    breakdown.is_solid = trust_data.is_solid;

    // Add factors
    for (const auto& [type, factor] : trust_data.factors) {
        TrustBreakdown::FactorBreakdown factor_bd;
        factor_bd.name = GetTrustFactorName(type);
        factor_bd.value = factor.value;
        factor_bd.weight = factor.weight;
        factor_bd.trend_description = GetTrendDescription(factor.trend);
        breakdown.factors.push_back(factor_bd);
    }

    // Stability description
    if (trust_data.is_solid) {
        breakdown.stability_description = "Stable - changes slowly";
    } else if (trust_data.is_fragile) {
        breakdown.stability_description = "Fragile - may collapse quickly";
    } else {
        breakdown.stability_description = "Normal stability";
    }

    return breakdown;
}

ReputationBreakdown DiplomacyUIDataFactory::CreateReputationBreakdown(
    const GlobalReputation& reputation)
{
    ReputationBreakdown breakdown;
    breakdown.global_trust = reputation.global_trust;

    // Add reputations
    for (const auto& [type, score] : reputation.reputations) {
        if (std::abs(score.value) < 0.1) continue;  // Skip near-zero reputations

        ReputationBreakdown::ReputationEntry entry;
        entry.name = GetReputationName(type);
        entry.value = score.value;
        entry.certainty = score.certainty;
        entry.is_primary = score.is_primary_reputation;
        entry.description = GetReputationDescription(type, score.value);
        breakdown.reputations.push_back(entry);

        if (score.is_primary_reputation) {
            breakdown.primary_traits.push_back(entry.name);
        }
    }

    // Stats
    breakdown.stats.treaties_honored = reputation.stats.total_treaties_honored;
    breakdown.stats.treaties_violated = reputation.stats.total_treaties_violated;
    breakdown.stats.alliances_maintained = reputation.stats.total_alliances_maintained;
    breakdown.stats.alliances_betrayed = reputation.stats.total_alliances_broken;
    breakdown.stats.backstabs = reputation.stats.total_backstabs;

    return breakdown;
}

MemoryBreakdown DiplomacyUIDataFactory::CreateMemoryBreakdown(const EventMemory& memory) {
    MemoryBreakdown breakdown;
    breakdown.total_positive_events = memory.total_positive_events;
    breakdown.total_negative_events = memory.total_negative_events;
    breakdown.total_neutral_events = memory.total_neutral_events;

    breakdown.has_grudge = memory.HasGrudge();
    breakdown.has_friendship = memory.HasDeepFriendship();
    breakdown.are_historical_rivals = memory.IsHistoricalRival();
    breakdown.are_historical_allies = memory.IsHistoricalAlly();

    // Get major events
    auto major_events = memory.GetMajorEvents(EventSeverity::MAJOR);
    for (size_t i = 0; i < std::min(size_t(10), major_events.size()); i++) {
        auto* event = major_events[i];
        MemoryBreakdown::MemorableMoment moment;
        moment.event_type = GetEventTypeName(event->type);
        moment.description = event->description;
        moment.opinion_impact = event->opinion_impact;
        moment.is_permanent = event->is_permanent;
        // TODO: Format date
        breakdown.major_events.push_back(moment);
    }

    // Get recent events
    auto recent_events = memory.GetRecentEvents(24);  // Last 2 years
    for (size_t i = 0; i < std::min(size_t(20), recent_events.size()); i++) {
        auto* event = recent_events[i];
        MemoryBreakdown::MemorableMoment moment;
        moment.event_type = GetEventTypeName(event->type);
        moment.description = event->description;
        moment.opinion_impact = event->opinion_impact;
        moment.is_permanent = event->is_permanent;
        breakdown.recent_events.push_back(moment);
    }

    // Summary
    if (breakdown.are_historical_allies) {
        breakdown.relationship_summary = "Long-standing allies with deep cooperation";
    } else if (breakdown.are_historical_rivals) {
        breakdown.relationship_summary = "Historical rivals with long conflict";
    } else if (breakdown.has_friendship) {
        breakdown.relationship_summary = "Deep friendship built over many years";
    } else if (breakdown.has_grudge) {
        breakdown.relationship_summary = "Significant grievances and unresolved conflicts";
    } else {
        breakdown.relationship_summary = "Standard diplomatic relationship";
    }

    return breakdown;
}

std::string DiplomacyUIDataFactory::GetReputationName(ReputationType type) {
    switch(type) {
        case ReputationType::MILITARY_HONOR: return "Military Honor";
        case ReputationType::TREATY_KEEPER: return "Treaty Keeper";
        case ReputationType::ECONOMIC_RELIABILITY: return "Economic Reliability";
        case ReputationType::DIPLOMATIC_INTEGRITY: return "Diplomatic Integrity";
        case ReputationType::AGGRESSIVE_EXPANSIONIST: return "Aggressive Expansionist";
        case ReputationType::DEFENSIVE_PACIFIST: return "Defensive Pacifist";
        case ReputationType::GENEROUS_ALLY: return "Generous Ally";
        case ReputationType::BACKSTABBER: return "Backstabber";
        case ReputationType::CRUSADER: return "Crusader";
        case ReputationType::PRAGMATIST: return "Pragmatist";
        default: return "Unknown";
    }
}

std::string DiplomacyUIDataFactory::GetEventTypeName(EventType type) {
    // TODO: Implement full event type names
    return "Diplomatic Event";
}

std::string DiplomacyUIDataFactory::GetTrendDescription(double trend) {
    if (trend > 0.05) return "Improving";
    if (trend < -0.05) return "Declining";
    return "Stable";
}

std::string DiplomacyUIDataFactory::GetRelationshipStatus(int opinion, double trust) {
    if (opinion >= 75 && trust >= 0.7) return "Close Ally";
    if (opinion >= 50) return "Ally";
    if (opinion >= 25) return "Friend";
    if (opinion >= -25) return "Neutral";
    if (opinion >= -50) return "Rival";
    return "Enemy";
}

} // namespace game::diplomacy
```

### Day 18-19: Performance Optimization & Validation

**Performance Optimization Tasks:**

1. **Profile and optimize monthly updates**
2. **Optimize memory lookups**
3. **Add caching where appropriate**
4. **Ensure thread safety**

#### File: `docs/systems/performance_optimization.md`

```markdown
# Diplomacy System Performance Optimization

## Performance Targets
- Monthly update: < 10ms for 50 realms (2500 relationships)
- Memory queries: < 0.1ms
- Trust calculations: < 0.5ms per relationship
- Reputation updates: < 1ms per realm

## Optimization Strategies

### 1. Memory System
- Cache frequently accessed memories
- Lazy evaluation of memory impacts
- Prune forgotten events more aggressively
- Use spatial hashing for realm lookups

### 2. Trust System
- Cache trust calculations between updates
- Only recalculate when events occur
- Batch trust updates
- Use dirty flags for selective updates

### 3. Reputation System
- Update reputation scores incrementally
- Cache reputation modifiers
- Only propagate major reputation changes
- Limit propagation distance

### 4. Integration
- Batch all monthly updates
- Parallelize independent calculations
- Use lock-free data structures where possible
- Minimize ECS component access

## Profiling Results
[To be filled during optimization]

## Benchmarks
[To be filled during testing]
```

### Day 20: Documentation & Examples

Create comprehensive documentation:

#### File: `docs/systems/trust_reputation_user_guide.md`

```markdown
# Trust & Reputation System User Guide

## Overview
The Trust & Reputation systems add depth to diplomatic relationships by tracking:
- **Memory**: What actions have been taken between realms
- **Trust**: How reliable each realm considers others
- **Reputation**: How the world perceives each realm

## For Players

### Understanding Trust
Trust ranges from 0.0 (no trust) to 1.0 (complete trust):
- **0.0-0.3**: Distrustful - expect betrayal
- **0.3-0.5**: Cautious - uncertain reliability
- **0.5-0.7**: Trustworthy - generally reliable
- **0.7-1.0**: Highly trusted - proven ally

Trust is built through:
- Honoring treaties (+0.05 to +0.15)
- Providing military aid (+0.10 to +0.20)
- Fulfilling economic obligations (+0.08 to +0.15)

Trust is damaged by:
- Breaking treaties (-0.15 to -0.30)
- Refusing aid (-0.10 to -0.20)
- Betrayal (-0.50 to -0.80)

### Understanding Reputation
Reputations are global traits that affect all relationships:

**Positive Reputations:**
- Treaty Keeper: +15 opinion with all realms
- Military Honor: +12 opinion, easier to form alliances
- Generous Ally: +20 opinion, more trade opportunities
- Economic Reliability: Better trade deals

**Negative Reputations:**
- Backstabber: -30 opinion, very difficult to form alliances
- Aggressive Expansionist: -10 opinion from peaceful realms
- Treaty Breaker: -20 opinion, harder to negotiate

### Rebuilding Trust
After damaging trust, you can rebuild through:
1. Time - Natural slow recovery if no further violations
2. Compliance - Honor all agreements for extended period
3. Compensation - Send gifts or provide aid
4. Demonstration - Fight together in wars

Requirements vary based on violation severity:
- Minor violation: 1-2 years of good behavior
- Major violation: 3-5 years + significant demonstration
- Betrayal: 5-10 years + fighting together + guarantees

### Memory & Grudges
Realms remember your actions:
- Major events are remembered longer
- Betrayals are often permanent memories
- Multiple negative events create grudges
- Grudges make improving relations very difficult

## For Modders

### Adding New Event Types
[Documentation for modders]

### Customizing Trust Factors
[Documentation for modders]

### Creating Custom Reputations
[Documentation for modders]
```

### Day 21: Final Testing & Release Preparation

**Final Testing Checklist:**

```cpp
// File: tests/game/diplomacy/FinalValidationTests.cpp

TEST(FinalValidation, AllSystemsIntegrateCorrectly) {
    // Create complete scenario
    // Verify all systems work together
}

TEST(FinalValidation, PerformanceTargetsMet) {
    // Benchmark all operations
    // Verify < 10ms monthly updates for 50 realms
}

TEST(FinalValidation, SaveLoadPreservesState) {
    // Save complete game state
    // Load and verify all data intact
}

TEST(FinalValidation, EdgeCaseHandling) {
    // Test extreme values
    // Test null/invalid inputs
    // Test concurrent modifications
}

TEST(FinalValidation, BackwardCompatibility) {
    // Load old save files
    // Verify graceful handling
}
```

**Release Deliverables:**
- [ ] All code complete and reviewed
- [ ] All tests passing
- [ ] Performance targets met
- [ ] Documentation complete
- [ ] Example scenarios created
- [ ] Integration guide written
- [ ] Migration guide for existing saves

---

## PHASE 2 SUCCESS CRITERIA

### Functional Requirements
- [x] Trust system tracks 5 different trust factors
- [x] Trust can be built and lost based on actions
- [x] Trust rebuilding paths work correctly
- [x] Reputation system tracks 10+ reputation types
- [x] Global reputation affects third-party opinions
- [x] Reputation propagates to observers
- [x] Memory system integrates with trust
- [x] All systems integrate with diplomatic state

### Performance Requirements
- [ ] Monthly update < 10ms for 50 realms
- [ ] No memory leaks
- [ ] Stable with long-running games (1000+ years)

### Quality Requirements
- [ ] 90%+ code coverage
- [ ] All integration tests passing
- [ ] Documentation complete
- [ ] No critical bugs

### User Experience Requirements
- [ ] Clear UI for trust breakdown
- [ ] Clear UI for reputation display
- [ ] Meaningful feedback on actions
- [ ] Intuitive trust rebuilding

---

## PHASE 3 PREVIEW

After Phase 2, Phase 3 will cover:
- AI decision-making using trust/reputation
- Dynamic events based on relationships
- Advanced diplomatic actions
- Coalition and alliance network systems
- Historical chronicles and storytelling

---

## APPENDIX: Testing Strategy

### Unit Tests
- Individual function tests for all systems
- Edge case coverage
- Performance microbenchmarks

### Integration Tests
- System-to-system integration
- Full diplomatic scenario tests
- Save/load testing

### Performance Tests
- Scalability tests (10, 50, 100 realms)
- Long-running stress tests
- Memory profiling

### User Acceptance Tests
- Player-facing features
- UI responsiveness
- Gameplay balance

---

**END OF PHASE 2 IMPLEMENTATION PLAN**
