#pragma once

#include "core/types/game_types.h"
#include "game/diplomacy/DiplomacyComponents.h"
#include "core/ECS/IComponent.h"
#include "core/ECS/ComponentAccessManager.h"
#include "core/ECS/MessageBus.h"
#include "utils/PlatformCompat.h"
#include <unordered_map>
#include <deque>
#include <string>

namespace game::diplomacy {

// ============================================================================
// Trust Factor Types
// ============================================================================

enum class TrustFactorType : uint8_t {
    TREATY_COMPLIANCE,      // How well they honor treaties
    MILITARY_RELIABILITY,   // Reliability in wars/alliances
    ECONOMIC_RELIABILITY,   // Trade agreements, debt repayment
    PERSONAL_RELATIONSHIP,  // Ruler-to-ruler trust
    HISTORICAL_BEHAVIOR,    // Long-term pattern
    COUNT
};

// ============================================================================
// Individual Trust Factor
// ============================================================================

struct TrustFactor {
    TrustFactorType type;
    double value = 0.5;                 // 0.0 (complete distrust) to 1.0 (complete trust)
    double weight = 1.0;                // How much this factor matters

    // History
    std::deque<double> value_history;   // Track changes over time
    double trend = 0.0;                 // Positive = improving, negative = declining

    // Events affecting this factor
    int positive_events = 0;
    int negative_events = 0;

    // Update value
    void ModifyValue(double delta, const std::string& reason);
    void CalculateTrend();
    double GetWeightedValue() const { return value * weight; }
};

// ============================================================================
// Composite Trust Data
// ============================================================================

struct TrustData {
    types::EntityID our_realm;
    types::EntityID other_realm;

    // Individual trust factors
    std::unordered_map<TrustFactorType, TrustFactor> factors;

    // Composite trust score
    double overall_trust = 0.5;
    double previous_trust = 0.5;
    double trust_change_rate = 0.0;

    // Trust bounds
    double min_possible_trust = 0.0;    // Floor based on history
    double max_possible_trust = 1.0;    // Ceiling based on history

    // Trust volatility
    double volatility = 0.1;            // How quickly trust changes
    bool is_fragile = false;            // Can collapse quickly
    bool is_solid = false;              // Very stable trust

    TrustData();
    TrustData(types::EntityID us, types::EntityID them);

    void InitializeFactors();
    void CalculateOverallTrust();
    void UpdateTrustBounds();
    void AssessStability();

    // Get specific factor
    TrustFactor* GetFactor(TrustFactorType type);
    const TrustFactor* GetFactor(TrustFactorType type) const;

    // Modify trust
    void ModifyTrust(TrustFactorType factor_type, double delta, const std::string& reason);
    void SetTrustFloor(double floor);  // After betrayal, can't fully trust again
    void SetTrustCeiling(double ceiling);
};

// ============================================================================
// Trust Rebuilding
// ============================================================================

struct TrustRebuildingPath {
    types::EntityID realm_a;
    types::EntityID realm_b;

    double starting_trust = 0.0;
    double target_trust = 0.5;
    double current_progress = 0.0;

    // Requirements
    struct Requirement {
        std::string description;
        bool is_completed = false;
        double trust_gain_on_completion = 0.05;
    };

    std::vector<Requirement> requirements;

    // Time-based rebuilding
    int months_of_peace_required = 24;
    int months_of_peace_achieved = 0;

    bool compliance_required = true;
    bool gifts_required = false;
    int gifts_sent = 0;
    int gifts_needed = 3;

    double monthly_natural_recovery = 0.01;  // Slow natural healing

    void AddRequirement(const std::string& desc, double trust_gain);
    void CompleteRequirement(const std::string& desc);
    void UpdateProgress();
    bool IsComplete() const;
};

// ============================================================================
// Trust Component
// ============================================================================

struct TrustComponent : public game::core::Component<TrustComponent> {
    types::EntityID realm_id;

    // Trust data with all other realms
    std::unordered_map<types::EntityID, TrustData> trust_relationships;

    // Active rebuilding efforts
    std::unordered_map<types::EntityID, TrustRebuildingPath> rebuilding_paths;

    // Trust reputation (how trustworthy are we globally?)
    double global_trustworthiness = 1.0;
    int treaties_honored = 0;
    int treaties_violated = 0;
    int alliances_honored = 0;
    int alliances_betrayed = 0;

    std::string GetComponentTypeName() const override {
        return "TrustComponent";
    }

    // Helper methods
    TrustData* GetTrustData(types::EntityID other_realm);
    const TrustData* GetTrustData(types::EntityID other_realm) const;

    void ModifyTrust(types::EntityID other_realm, TrustFactorType factor, double delta, const std::string& reason);
    double GetTrustLevel(types::EntityID other_realm) const;

    void StartTrustRebuilding(types::EntityID other_realm, double target_trust = 0.5);
    void UpdateRebuildingProgress(float delta_time);

    void UpdateGlobalTrustworthiness();

    // Serialization
    Json::Value Serialize() const override;
    void Deserialize(const Json::Value& data) override;
};

// ============================================================================
// Trust System Manager
// ============================================================================

class TrustSystemManager {
public:
    explicit TrustSystemManager(
        ::core::ecs::ComponentAccessManager& access_manager,
        ::core::ecs::MessageBus& message_bus
    );

    void Initialize();
    void UpdateMonthly();

    // Trust queries
    double GetTrustLevel(types::EntityID realm_a, types::EntityID realm_b, TrustFactorType factor) const;
    double GetOverallTrust(types::EntityID realm_a, types::EntityID realm_b) const;

    // Trust modification
    void ModifyTrust(types::EntityID realm_a, types::EntityID realm_b, TrustFactorType factor, double delta, const std::string& reason);
    void OnTreatyCompliance(types::EntityID realm, bool complied);
    void OnMilitarySupport(types::EntityID supporter, types::EntityID supported, bool provided);
    void OnEconomicObligation(types::EntityID realm, bool fulfilled);
    void OnBetrayal(types::EntityID betrayer, types::EntityID victim);

    // Trust rebuilding
    void InitiateTrustRebuilding(types::EntityID realm_a, types::EntityID realm_b);
    void ProcessRebuildingPath(types::EntityID realm_a, types::EntityID realm_b);

    // Integration
    void ApplyTrustToDiplomaticState(types::EntityID realm_a, types::EntityID realm_b);

private:
    ::core::ecs::ComponentAccessManager& m_access_manager;
    ::core::ecs::MessageBus& m_message_bus;

    TrustComponent* GetOrCreateTrustComponent(types::EntityID realm);
    void SubscribeToEvents();
    void ProcessTrustDecay();
    void UpdateTrustBounds();
};

} // namespace game::diplomacy
