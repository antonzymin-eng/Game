// ============================================================================
// InfluenceComponents.cpp - Sphere of Influence Implementation
// Created: November 11, 2025 - Phase 3: Sphere of Influence System
// Location: src/game/diplomacy/InfluenceComponents.cpp
// ============================================================================

#include "game/diplomacy/InfluenceComponents.h"
#include <algorithm>
#include <cmath>

namespace game {
namespace diplomacy {

// ============================================================================
// InfluenceSource Implementation
// ============================================================================

void InfluenceSource::CalculateEffectiveStrength() {
    effective_strength = base_strength * distance_modifier * relationship_modifier;
    effective_strength = std::max(0.0, effective_strength);
}

void InfluenceSource::UpdateDistanceModifier(int hops, const std::vector<types::EntityID>& influence_path) {
    hops_from_source = hops;
    path = influence_path;

    // Type-specific decay rates
    double decay_rate = 0.0;
    switch(type) {
        case InfluenceType::MILITARY:    decay_rate = 0.40; break;  // High decay, short range
        case InfluenceType::ECONOMIC:    decay_rate = 0.15; break;  // Low decay, long range
        case InfluenceType::DYNASTIC:    decay_rate = 0.05; break;  // Very low, family ties transcend distance
        case InfluenceType::PERSONAL:    decay_rate = 0.25; break;  // Medium decay
        case InfluenceType::RELIGIOUS:   decay_rate = 0.00; break;  // No decay within same faith
        case InfluenceType::CULTURAL:    decay_rate = 0.20; break;  // Medium decay
        case InfluenceType::PRESTIGE:    decay_rate = 0.10; break;  // Low decay, reputation travels far
        default: decay_rate = 0.30; break;
    }

    // Calculate modifier: modifier = (1 - decay_rate)^hops
    distance_modifier = std::pow(1.0 - decay_rate, static_cast<double>(hops));
    distance_modifier = std::clamp(distance_modifier, 0.0, 1.0);

    CalculateEffectiveStrength();
}

void InfluenceSource::UpdateRelationshipModifier(int opinion) {
    // Opinion from -100 to +100 affects effectiveness
    // -100 opinion = 0.5x effectiveness, +100 = 1.5x effectiveness
    relationship_modifier = 1.0 + (opinion / 200.0);
    relationship_modifier = std::clamp(relationship_modifier, 0.5, 1.5);

    CalculateEffectiveStrength();
}

// ============================================================================
// InfluenceState Implementation
// ============================================================================

void InfluenceState::AddInfluence(const InfluenceSource& source) {
    influences_by_type[source.type].push_back(source);
    CalculateTotalInfluence();
    UpdateDominantInfluencers();
    CalculateAutonomy();
    CalculateDiplomaticFreedom();
}

void InfluenceState::RemoveInfluence(types::EntityID source_realm, InfluenceType type) {
    auto& sources = influences_by_type[type];
    sources.erase(
        std::remove_if(sources.begin(), sources.end(),
            [source_realm](const InfluenceSource& s) {
                return s.source_realm == source_realm;
            }),
        sources.end()
    );

    CalculateTotalInfluence();
    UpdateDominantInfluencers();
    CalculateAutonomy();
    CalculateDiplomaticFreedom();
}

void InfluenceState::CalculateTotalInfluence() {
    total_influence_received = 0.0;

    for (auto& [type, sources] : influences_by_type) {
        for (const auto& source : sources) {
            total_influence_received += source.effective_strength;
        }
    }
}

void InfluenceState::UpdateDominantInfluencers() {
    dominant_influencer.clear();

    for (auto& [type, sources] : influences_by_type) {
        if (sources.empty()) continue;

        // Find strongest influence of this type
        auto strongest = std::max_element(sources.begin(), sources.end(),
            [](const InfluenceSource& a, const InfluenceSource& b) {
                return a.effective_strength < b.effective_strength;
            });

        if (strongest != sources.end() && strongest->effective_strength > 10.0) {
            dominant_influencer[type] = strongest->source_realm;
        }
    }
}

void InfluenceState::CalculateAutonomy() {
    // Autonomy reduced by total influence
    // Formula: autonomy = 1.0 - (total_influence / 200.0)
    autonomy = 1.0 - (total_influence_received / 200.0);
    autonomy = std::clamp(autonomy, 0.0, 1.0);
}

void InfluenceState::CalculateDiplomaticFreedom() {
    // Diplomatic freedom primarily affected by military + economic influence
    double military_inf = 0.0;
    double economic_inf = 0.0;

    auto mil_it = influences_by_type.find(InfluenceType::MILITARY);
    if (mil_it != influences_by_type.end()) {
        for (const auto& source : mil_it->second) {
            military_inf += source.effective_strength;
        }
    }

    auto econ_it = influences_by_type.find(InfluenceType::ECONOMIC);
    if (econ_it != influences_by_type.end()) {
        for (const auto& source : econ_it->second) {
            economic_inf += source.effective_strength;
        }
    }

    diplomatic_freedom = 1.0 - ((military_inf + economic_inf) / 150.0);
    diplomatic_freedom = std::clamp(diplomatic_freedom, 0.0, 1.0);
}

double InfluenceState::GetInfluenceStrength(types::EntityID source_realm, InfluenceType type) const {
    auto it = influences_by_type.find(type);
    if (it == influences_by_type.end()) return 0.0;

    for (const auto& source : it->second) {
        if (source.source_realm == source_realm) {
            return source.effective_strength;
        }
    }
    return 0.0;
}

types::EntityID InfluenceState::GetDominantInfluencer(InfluenceType type) const {
    auto it = dominant_influencer.find(type);
    return (it != dominant_influencer.end()) ? it->second : types::EntityID();
}

bool InfluenceState::IsInfluencedBy(types::EntityID source_realm) const {
    for (const auto& [type, sources] : influences_by_type) {
        for (const auto& source : sources) {
            if (source.source_realm == source_realm && source.effective_strength > 5.0) {
                return true;
            }
        }
    }
    return false;
}

// ============================================================================
// VassalInfluence Implementation
// ============================================================================

void VassalInfluence::CalculateEffects(double base_influence) {
    // Calculate loyalty shift based on influence strength
    loyalty_shift = -base_influence / 100.0;  // Negative = away from liege
    loyalty_shift = std::clamp(loyalty_shift, -1.0, 0.0);

    // Independence desire grows with influence
    independence_desire = base_influence / 80.0;
    independence_desire = std::clamp(independence_desire, 0.0, 1.0);

    // Allegiance shift (considering switching sides)
    allegiance_shift = base_influence / 120.0;
    allegiance_shift = std::clamp(allegiance_shift, 0.0, 1.0);

    CheckDefectionRisk(0.7);
}

void VassalInfluence::CheckDefectionRisk(double threshold) {
    may_defect = (allegiance_shift > threshold);
    may_revolt = (independence_desire > threshold && allegiance_shift < 0.5);
    may_request_protection = (influence_strength > 50.0 && months_under_influence > 12);
}

void VassalInfluence::UpdateMonthly() {
    months_under_influence++;
    CalculateEffects(influence_strength);
}

// ============================================================================
// CharacterInfluence Implementation
// ============================================================================

void CharacterInfluence::CalculateOpinionBias(double base_influence) {
    // Opinion bias toward influencer
    opinion_bias = base_influence / 2.0;  // Max +50 opinion bias
    opinion_bias = std::clamp(opinion_bias, 0.0, 50.0);

    // Personal loyalty to foreign power
    personal_loyalty = base_influence / 100.0;
    personal_loyalty = std::clamp(personal_loyalty, 0.0, 1.0);

    CheckCompromised(0.8);
}

void CharacterInfluence::CheckCompromised(double threshold) {
    compromised = (personal_loyalty > threshold);
}

bool CharacterInfluence::WouldSabotage() const {
    return compromised && personal_loyalty > 0.9;
}

bool CharacterInfluence::WouldLeak() const {
    return compromised && personal_loyalty > 0.8;
}

double CharacterInfluence::GetDecisionBias() const {
    return opinion_bias / 100.0;  // Return as 0-0.5 multiplier
}

// ============================================================================
// InfluenceConflict Implementation
// ============================================================================

void InfluenceConflict::CalculateTension() {
    // Tension higher when powers are evenly matched
    double strength_diff = std::abs(primary_strength - challenger_strength);
    double max_strength = std::max(primary_strength, challenger_strength);

    if (max_strength > 0) {
        // Closer strengths = higher tension
        tension_level = 100.0 * (1.0 - (strength_diff / max_strength));
    } else {
        tension_level = 0.0;
    }

    tension_level = std::clamp(tension_level, 0.0, 100.0);
    UpdateEscalationRisk();
}

void InfluenceConflict::UpdateEscalationRisk() {
    // Escalation risk based on tension and incident count
    double base_risk = tension_level / 200.0;  // Max 0.5 from tension
    double incident_risk = std::min(0.4, incidents.size() * 0.1);  // Max 0.4 from incidents

    escalation_risk = base_risk + incident_risk;
    escalation_risk = std::clamp(escalation_risk, 0.0, 1.0);

    // Check if this is a flashpoint
    is_flashpoint = CheckFlashpoint();
}

void InfluenceConflict::AddIncident(const std::string& incident) {
    incidents.push_back(incident);
    UpdateEscalationRisk();
}

bool InfluenceConflict::CheckFlashpoint() const {
    // Flashpoint if: high tension + multiple incidents + high escalation risk
    return (tension_level > 70.0 && incidents.size() >= 3 && escalation_risk > 0.6);
}

// ============================================================================
// InfluenceComponent Implementation
// ============================================================================

void InfluenceComponent::AddInfluenceSource(const InfluenceSource& source) {
    auto it = influenced_realms.find(source.source_realm);
    if (it != influenced_realms.end()) {
        it->second.AddInfluence(source);
    } else {
        InfluenceState state(source.source_realm);
        state.AddInfluence(source);
        influenced_realms[source.source_realm] = state;
    }

    UpdateSphereMetrics();
}

void InfluenceComponent::RemoveInfluenceSource(types::EntityID source_realm, InfluenceType type) {
    auto it = influenced_realms.find(source_realm);
    if (it != influenced_realms.end()) {
        it->second.RemoveInfluence(source_realm, type);

        // Remove if no more influences
        if (it->second.total_influence_received < 1.0) {
            influenced_realms.erase(it);
        }
    }

    UpdateSphereMetrics();
}

void InfluenceComponent::UpdateSphereMetrics() {
    core_sphere.clear();
    peripheral_sphere.clear();
    contested_sphere.clear();

    double total_strength = 0.0;
    int realm_count = 0;

    for (const auto& [realm_id, state] : influenced_realms) {
        total_strength += state.total_influence_received;
        realm_count++;

        // Categorize by influence strength
        if (state.total_influence_received > 80.0) {
            core_sphere.push_back(realm_id);
        } else if (state.total_influence_received > 30.0) {
            peripheral_sphere.push_back(realm_id);
        }

        // Check if contested
        bool has_competition = false;
        for (const auto& [type, sources] : state.influences_by_type) {
            if (sources.size() > 1) {
                has_competition = true;
                break;
            }
        }
        if (has_competition) {
            contested_sphere.push_back(realm_id);
        }
    }

    sphere_size = static_cast<double>(realm_count);
    sphere_strength = (realm_count > 0) ? (total_strength / realm_count) : 0.0;
}

void InfluenceComponent::UpdateInfluencedRealms() {
    for (auto& [realm_id, state] : influenced_realms) {
        state.CalculateTotalInfluence();
        state.UpdateDominantInfluencers();
        state.CalculateAutonomy();
        state.CalculateDiplomaticFreedom();
    }

    UpdateSphereMetrics();
}

double InfluenceComponent::GetProjectionStrength(InfluenceType type) const {
    auto it = influence_projection.find(type);
    return (it != influence_projection.end()) ? it->second : 0.0;
}

const InfluenceState* InfluenceComponent::GetInfluenceOn(types::EntityID target) const {
    auto it = influenced_realms.find(target);
    return (it != influenced_realms.end()) ? &it->second : nullptr;
}

Json::Value InfluenceComponent::Serialize() const {
    Json::Value root;
    // TODO: Implement serialization
    return root;
}

void InfluenceComponent::Deserialize(const Json::Value& data) {
    // TODO: Implement deserialization
}

} // namespace diplomacy
} // namespace game
