#include "game/diplomacy/TrustSystem.h"
#include <algorithm>
#include <cmath>

namespace game::diplomacy {

// ============================================================================
// TrustFactor Implementation
// ============================================================================

void TrustFactor::ModifyValue(double delta, const std::string& reason) {
    value = std::clamp(value + delta, 0.0, 1.0);

    if (delta > 0) {
        positive_events++;
    } else if (delta < 0) {
        negative_events++;
    }

    // Record in history
    value_history.push_back(value);
    if (value_history.size() > 24) {  // Keep last 24 data points (2 years)
        value_history.pop_front();
    }

    CalculateTrend();
}

void TrustFactor::CalculateTrend() {
    if (value_history.size() < 2) {
        trend = 0.0;
        return;
    }

    // Calculate linear regression trend
    double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_xx = 0.0;
    int n = static_cast<int>(value_history.size());

    for (int i = 0; i < n; ++i) {
        double x = static_cast<double>(i);
        double y = value_history[i];
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_xx += x * x;
    }

    trend = (n * sum_xy - sum_x * sum_y) / (n * sum_xx - sum_x * sum_x);
}

// ============================================================================
// TrustData Implementation
// ============================================================================

TrustData::TrustData() {
    InitializeFactors();
}

TrustData::TrustData(types::EntityID us, types::EntityID them)
    : our_realm(us)
    , other_realm(them)
{
    InitializeFactors();
}

void TrustData::InitializeFactors() {
    for (int i = 0; i < static_cast<int>(TrustFactorType::COUNT); ++i) {
        TrustFactorType type = static_cast<TrustFactorType>(i);
        TrustFactor factor;
        factor.type = type;
        factor.value = 0.5;  // Start neutral

        // Set weights based on importance
        switch (type) {
            case TrustFactorType::TREATY_COMPLIANCE:
                factor.weight = 1.5;  // Very important
                break;
            case TrustFactorType::MILITARY_RELIABILITY:
                factor.weight = 1.3;
                break;
            case TrustFactorType::ECONOMIC_RELIABILITY:
                factor.weight = 1.0;
                break;
            case TrustFactorType::PERSONAL_RELATIONSHIP:
                factor.weight = 0.8;
                break;
            case TrustFactorType::HISTORICAL_BEHAVIOR:
                factor.weight = 1.2;
                break;
            default:
                factor.weight = 1.0;
                break;
        }

        factors[type] = factor;
    }

    CalculateOverallTrust();
}

void TrustData::CalculateOverallTrust() {
    previous_trust = overall_trust;

    double weighted_sum = 0.0;
    double weight_total = 0.0;

    for (auto& [type, factor] : factors) {
        weighted_sum += factor.GetWeightedValue();
        weight_total += factor.weight;
    }

    if (weight_total > 0.0) {
        overall_trust = weighted_sum / weight_total;
    } else {
        overall_trust = 0.5;
    }

    // Apply trust bounds
    overall_trust = std::clamp(overall_trust, min_possible_trust, max_possible_trust);

    // Calculate change rate
    trust_change_rate = overall_trust - previous_trust;

    AssessStability();
}

void TrustData::UpdateTrustBounds() {
    // Trust bounds are determined by history of the relationship
    // If there was a major betrayal, max trust is lowered
    // If there's consistent positive history, min trust is raised

    // Example logic (can be refined)
    if (overall_trust < 0.2 && trust_change_rate < -0.1) {
        max_possible_trust = 0.7;  // Can't reach full trust after severe betrayal
    }

    if (overall_trust > 0.8 && trust_change_rate > 0.0) {
        min_possible_trust = 0.3;  // Solid foundation, won't drop to zero easily
    }
}

void TrustData::AssessStability() {
    // Calculate volatility based on recent changes
    double abs_change = std::abs(trust_change_rate);
    volatility = 0.9 * volatility + 0.1 * abs_change;  // Exponential moving average

    // Assess if trust is fragile (high volatility, low trust)
    is_fragile = (volatility > 0.15 && overall_trust < 0.4);

    // Assess if trust is solid (low volatility, high trust)
    is_solid = (volatility < 0.05 && overall_trust > 0.7);
}

TrustFactor* TrustData::GetFactor(TrustFactorType type) {
    auto it = factors.find(type);
    return (it != factors.end()) ? &it->second : nullptr;
}

const TrustFactor* TrustData::GetFactor(TrustFactorType type) const {
    auto it = factors.find(type);
    return (it != factors.end()) ? &it->second : nullptr;
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
    overall_trust = std::max(overall_trust, min_possible_trust);
}

void TrustData::SetTrustCeiling(double ceiling) {
    max_possible_trust = std::clamp(ceiling, 0.0, 1.0);
    overall_trust = std::min(overall_trust, max_possible_trust);
}

// ============================================================================
// TrustRebuildingPath Implementation
// ============================================================================

void TrustRebuildingPath::AddRequirement(const std::string& desc, double trust_gain) {
    Requirement req;
    req.description = desc;
    req.is_completed = false;
    req.trust_gain_on_completion = trust_gain;
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

    // Clamp progress
    current_progress = std::clamp(current_progress, 0.0, 1.0);
}

bool TrustRebuildingPath::IsComplete() const {
    // Check if all requirements are met
    bool all_completed = std::all_of(requirements.begin(), requirements.end(),
        [](const Requirement& req) { return req.is_completed; });

    return all_completed && current_progress >= 1.0;
}

// ============================================================================
// TrustComponent Implementation
// ============================================================================

TrustData* TrustComponent::GetTrustData(types::EntityID other_realm) {
    auto it = trust_relationships.find(other_realm);
    if (it != trust_relationships.end()) {
        return &it->second;
    }

    // Create new trust data
    trust_relationships[other_realm] = TrustData(realm_id, other_realm);
    return &trust_relationships[other_realm];
}

const TrustData* TrustComponent::GetTrustData(types::EntityID other_realm) const {
    auto it = trust_relationships.find(other_realm);
    return (it != trust_relationships.end()) ? &it->second : nullptr;
}

void TrustComponent::ModifyTrust(types::EntityID other_realm, TrustFactorType factor, double delta, const std::string& reason) {
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
    TrustRebuildingPath path;
    path.realm_a = realm_id;
    path.realm_b = other_realm;

    auto* trust_data = GetTrustData(other_realm);
    if (trust_data) {
        path.starting_trust = trust_data->overall_trust;
    }

    path.target_trust = target_trust;

    // Add default requirements
    path.AddRequirement("Maintain peace for 2 years", 0.15);
    path.AddRequirement("Honor existing treaties", 0.20);
    path.AddRequirement("Provide economic aid", 0.10);

    rebuilding_paths[other_realm] = path;
}

void TrustComponent::UpdateRebuildingProgress(float delta_time) {
    for (auto& [realm_id, path] : rebuilding_paths) {
        path.UpdateProgress();

        // Apply progress to actual trust
        auto* trust_data = GetTrustData(realm_id);
        if (trust_data) {
            double trust_gain = path.monthly_natural_recovery * delta_time;
            trust_data->ModifyTrust(TrustFactorType::HISTORICAL_BEHAVIOR, trust_gain, "Trust rebuilding");
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
    if (treaties_honored + treaties_violated == 0) {
        global_trustworthiness = 1.0;
        return;
    }

    double compliance_rate = static_cast<double>(treaties_honored) /
                           (treaties_honored + treaties_violated);

    // Weight recent actions more heavily
    global_trustworthiness = 0.7 * compliance_rate + 0.3 * global_trustworthiness;
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
    Json::Value relationships_array(Json::arrayValue);
    for (const auto& [other_id, trust_data] : trust_relationships) {
        Json::Value trust_json;
        trust_json["other_realm"] = static_cast<int>(other_id.id);
        trust_json["overall_trust"] = trust_data.overall_trust;
        trust_json["min_possible_trust"] = trust_data.min_possible_trust;
        trust_json["max_possible_trust"] = trust_data.max_possible_trust;
        relationships_array.append(trust_json);
    }
    root["trust_relationships"] = relationships_array;

    return root;
}

void TrustComponent::Deserialize(const Json::Value& data) {
    if (data.isMember("realm_id")) {
        realm_id.id = data["realm_id"].asUInt();
    }
    if (data.isMember("global_trustworthiness")) {
        global_trustworthiness = data["global_trustworthiness"].asDouble();
    }
    if (data.isMember("treaties_honored")) {
        treaties_honored = data["treaties_honored"].asInt();
    }
    if (data.isMember("treaties_violated")) {
        treaties_violated = data["treaties_violated"].asInt();
    }
    // Additional fields can be deserialized as needed
}

// ============================================================================
// TrustSystemManager Implementation
// ============================================================================

TrustSystemManager::TrustSystemManager(
    ::core::ecs::ComponentAccessManager& access_manager,
    ::core::ecs::MessageBus& message_bus)
    : m_access_manager(access_manager)
    , m_message_bus(message_bus)
{
}

void TrustSystemManager::Initialize() {
    SubscribeToEvents();

    // Initialize trust components for all existing realms
    auto entities = m_access_manager.GetEntitiesWithComponent<DiplomacyComponent>();
    for (auto realm_id : entities) {
        GetOrCreateTrustComponent(realm_id);
    }
}

void TrustSystemManager::UpdateMonthly() {
    ProcessTrustDecay();
    UpdateTrustBounds();

    // Update rebuilding paths
    auto entities = m_access_manager.GetEntitiesWithComponent<TrustComponent>();
    for (auto entity_id : entities) {
        auto trust_guard = m_access_manager.GetComponentForWrite<TrustComponent>(entity_id);
        if (trust_guard.IsValid()) {
            trust_guard.Get().UpdateRebuildingProgress(1.0f);  // 1 month
            trust_guard.Get().UpdateGlobalTrustworthiness();
        }
    }

    // Apply trust to diplomatic states
    auto diplomacy_entities = m_access_manager.GetEntitiesWithComponent<DiplomacyComponent>();
    for (auto realm_id : diplomacy_entities) {
        auto diplomacy = m_access_manager.GetComponent<DiplomacyComponent>(realm_id);
        if (!diplomacy) continue;

        for (const auto& [other_id, state] : diplomacy->relationships) {
            ApplyTrustToDiplomaticState(realm_id, other_id);
        }
    }
}

double TrustSystemManager::GetTrustLevel(types::EntityID realm_a, types::EntityID realm_b, TrustFactorType factor) const {
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

    return trust_comp->GetTrustLevel(realm_b);
}

void TrustSystemManager::ModifyTrust(types::EntityID realm_a, types::EntityID realm_b, TrustFactorType factor, double delta, const std::string& reason) {
    auto trust_guard = m_access_manager.GetComponentForWrite<TrustComponent>(realm_a);
    if (trust_guard.IsValid()) {
        trust_guard.Get().ModifyTrust(realm_b, factor, delta, reason);
    }

    // Also update reciprocal trust
    auto trust_guard_b = m_access_manager.GetComponentForWrite<TrustComponent>(realm_b);
    if (trust_guard_b.IsValid()) {
        trust_guard_b.Get().ModifyTrust(realm_a, factor, delta, reason);
    }
}

void TrustSystemManager::OnTreatyCompliance(types::EntityID realm, bool complied) {
    auto trust_guard = m_access_manager.GetComponentForWrite<TrustComponent>(realm);
    if (!trust_guard.IsValid()) return;

    if (complied) {
        trust_guard.Get().treaties_honored++;
    } else {
        trust_guard.Get().treaties_violated++;
    }

    trust_guard.Get().UpdateGlobalTrustworthiness();
}

void TrustSystemManager::OnMilitarySupport(types::EntityID supporter, types::EntityID supported, bool provided) {
    double delta = provided ? 0.15 : -0.25;
    ModifyTrust(supported, supporter, TrustFactorType::MILITARY_RELIABILITY, delta,
                provided ? "Provided military support" : "Failed to provide military support");
}

void TrustSystemManager::OnEconomicObligation(types::EntityID realm, bool fulfilled) {
    auto trust_guard = m_access_manager.GetComponentForWrite<TrustComponent>(realm);
    if (!trust_guard.IsValid()) return;

    // Update trust for all relationships based on global reputation
    auto& trust_comp = trust_guard.Get();
    for (auto& [other_id, trust_data] : trust_comp.trust_relationships) {
        double delta = fulfilled ? 0.05 : -0.10;
        trust_data.ModifyTrust(TrustFactorType::ECONOMIC_RELIABILITY, delta,
                             fulfilled ? "Fulfilled obligation" : "Failed obligation");
    }
}

void TrustSystemManager::OnBetrayal(types::EntityID betrayer, types::EntityID victim) {
    // Severe trust damage
    ModifyTrust(victim, betrayer, TrustFactorType::TREATY_COMPLIANCE, -0.6, "Betrayed");
    ModifyTrust(victim, betrayer, TrustFactorType::MILITARY_RELIABILITY, -0.5, "Betrayed");

    // Set trust ceiling - can't fully trust again
    auto trust_guard = m_access_manager.GetComponentForWrite<TrustComponent>(victim);
    if (trust_guard.IsValid()) {
        auto* trust_data = trust_guard.Get().GetTrustData(betrayer);
        if (trust_data) {
            trust_data->SetTrustCeiling(0.6);  // Can never fully trust again
        }
    }
}

void TrustSystemManager::InitiateTrustRebuilding(types::EntityID realm_a, types::EntityID realm_b) {
    auto trust_guard = m_access_manager.GetComponentForWrite<TrustComponent>(realm_a);
    if (trust_guard.IsValid()) {
        trust_guard.Get().StartTrustRebuilding(realm_b, 0.5);
    }
}

void TrustSystemManager::ProcessRebuildingPath(types::EntityID realm_a, types::EntityID realm_b) {
    auto trust_guard = m_access_manager.GetComponentForWrite<TrustComponent>(realm_a);
    if (!trust_guard.IsValid()) return;

    auto& trust_comp = trust_guard.Get();
    auto it = trust_comp.rebuilding_paths.find(realm_b);
    if (it != trust_comp.rebuilding_paths.end()) {
        it->second.UpdateProgress();
    }
}

void TrustSystemManager::ApplyTrustToDiplomaticState(types::EntityID realm_a, types::EntityID realm_b) {
    auto trust_comp = m_access_manager.GetComponent<TrustComponent>(realm_a);
    if (!trust_comp) return;

    double trust_level = trust_comp->GetTrustLevel(realm_b);

    auto diplomacy_guard = m_access_manager.GetComponentForWrite<DiplomacyComponent>(realm_a);
    if (!diplomacy_guard.IsValid()) return;

    auto* state = diplomacy_guard.Get().GetRelationship(realm_b);
    if (state) {
        state->trust = trust_level;
    }
}

TrustComponent* TrustSystemManager::GetOrCreateTrustComponent(types::EntityID realm) {
    auto existing_guard = m_access_manager.GetComponentForWrite<TrustComponent>(realm);
    if (existing_guard.IsValid()) {
        return &existing_guard.Get();
    }

    // Component doesn't exist - would need to be created through proper ECS API
    return nullptr;
}

void TrustSystemManager::SubscribeToEvents() {
    // TODO: Subscribe to relevant events
    // This depends on your MessageBus implementation
}

void TrustSystemManager::ProcessTrustDecay() {
    // Trust naturally decays slightly toward neutral over time
    auto entities = m_access_manager.GetEntitiesWithComponent<TrustComponent>();

    for (auto entity_id : entities) {
        auto trust_guard = m_access_manager.GetComponentForWrite<TrustComponent>(entity_id);
        if (!trust_guard.IsValid()) continue;

        auto& trust_comp = trust_guard.Get();
        for (auto& [other_id, trust_data] : trust_comp.trust_relationships) {
            // Slight drift toward 0.5 (neutral)
            double drift = (0.5 - trust_data.overall_trust) * 0.01;  // 1% per month
            trust_data.overall_trust += drift;
            trust_data.overall_trust = std::clamp(trust_data.overall_trust, trust_data.min_possible_trust, trust_data.max_possible_trust);
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

} // namespace game::diplomacy
