// ============================================================================
// InfluenceComponents.cpp - Sphere of Influence Implementation
// Created: November 11, 2025 - Phase 3: Sphere of Influence System
// Location: src/game/diplomacy/InfluenceComponents.cpp
// ============================================================================

#include "game/diplomacy/InfluenceComponents.h"
#include <algorithm>
#include <cmath>
#include <json/json.h>
#include <sstream>

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

Json::Value InfluenceComponent::SerializeToJson() const {
    Json::Value root;

    // Basic data
    root["realm_id"] = static_cast<int>(realm_id);
    root["sphere_size"] = sphere_size;
    root["sphere_strength"] = sphere_strength;

    // Serialize influence projection
    Json::Value projection;
    for (const auto& [type, strength] : influence_projection) {
        projection[InfluenceTypeToString(type)] = strength;
    }
    root["influence_projection"] = projection;

    // Serialize influenced realms
    Json::Value influenced(Json::arrayValue);
    for (const auto& [realm, state] : influenced_realms) {
        Json::Value state_json;
        state_json["realm_id"] = static_cast<int>(realm);
        state_json["total_influence"] = state.total_influence_received;
        state_json["autonomy"] = state.autonomy;
        state_json["diplomatic_freedom"] = state.diplomatic_freedom;
        state_json["resistance"] = state.resistance_strength;
        state_json["actively_resisting"] = state.actively_resisting;

        // Serialize influences by type
        Json::Value influences_by_type;
        for (const auto& [type, sources] : state.influences_by_type) {
            Json::Value sources_array(Json::arrayValue);
            for (const auto& source : sources) {
                Json::Value source_json;
                source_json["source_realm"] = static_cast<int>(source.source_realm);
                source_json["type"] = static_cast<int>(source.type);
                source_json["base_strength"] = source.base_strength;
                source_json["distance_mod"] = source.distance_modifier;
                source_json["relationship_mod"] = source.relationship_modifier;
                source_json["effective_strength"] = source.effective_strength;
                source_json["hops"] = source.hops_from_source;
                source_json["targets_whole_realm"] = source.targets_whole_realm;

                // Serialize path
                Json::Value path_array(Json::arrayValue);
                for (const auto& realm_id : source.path) {
                    path_array.append(static_cast<int>(realm_id));
                }
                source_json["path"] = path_array;

                // Serialize timestamps
                auto established_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    source.established_date.time_since_epoch()).count();
                auto update_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    source.last_update.time_since_epoch()).count();
                source_json["established_ms"] = static_cast<Json::Int64>(established_ms);
                source_json["last_update_ms"] = static_cast<Json::Int64>(update_ms);

                sources_array.append(source_json);
            }
            influences_by_type[InfluenceTypeToString(type)] = sources_array;
        }
        state_json["influences_by_type"] = influences_by_type;

        influenced.append(state_json);
    }
    root["influenced_realms"] = influenced;

    // Serialize incoming influence (CRITICAL for save/load)
    Json::Value incoming;
    incoming["realm_id"] = static_cast<int>(incoming_influence.affected_realm);
    incoming["total_influence"] = incoming_influence.total_influence_received;
    incoming["autonomy"] = incoming_influence.autonomy;
    incoming["diplomatic_freedom"] = incoming_influence.diplomatic_freedom;
    incoming["resistance"] = incoming_influence.resistance_strength;
    incoming["actively_resisting"] = incoming_influence.actively_resisting;

    // Serialize incoming influences by type
    Json::Value incoming_influences_by_type;
    for (const auto& [type, sources] : incoming_influence.influences_by_type) {
        Json::Value sources_array(Json::arrayValue);
        for (const auto& source : sources) {
            Json::Value source_json;
            source_json["source_realm"] = static_cast<int>(source.source_realm);
            source_json["type"] = static_cast<int>(source.type);
            source_json["base_strength"] = source.base_strength;
            source_json["distance_mod"] = source.distance_modifier;
            source_json["relationship_mod"] = source.relationship_modifier;
            source_json["effective_strength"] = source.effective_strength;
            source_json["hops"] = source.hops_from_source;
            source_json["targets_whole_realm"] = source.targets_whole_realm;

            // Serialize path
            Json::Value path_array(Json::arrayValue);
            for (const auto& realm_id : source.path) {
                path_array.append(static_cast<int>(realm_id));
            }
            source_json["path"] = path_array;

            auto established_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                source.established_date.time_since_epoch()).count();
            auto update_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                source.last_update.time_since_epoch()).count();
            source_json["established_ms"] = static_cast<Json::Int64>(established_ms);
            source_json["last_update_ms"] = static_cast<Json::Int64>(update_ms);

            sources_array.append(source_json);
        }
        incoming_influences_by_type[InfluenceTypeToString(type)] = sources_array;
    }
    incoming["influences_by_type"] = incoming_influences_by_type;
    root["incoming_influence"] = incoming;

    // TODO: Serialize influenced_vassals (not yet actively used)
    // TODO: Serialize foreign_vassals (not yet actively used)
    // TODO: Serialize influenced_characters (not yet actively used)

    // Serialize core/peripheral/contested spheres
    Json::Value core_array(Json::arrayValue);
    for (const auto& id : core_sphere) {
        core_array.append(static_cast<int>(id));
    }
    root["core_sphere"] = core_array;

    Json::Value peripheral_array(Json::arrayValue);
    for (const auto& id : peripheral_sphere) {
        peripheral_array.append(static_cast<int>(id));
    }
    root["peripheral_sphere"] = peripheral_array;

    Json::Value contested_array(Json::arrayValue);
    for (const auto& id : contested_sphere) {
        contested_array.append(static_cast<int>(id));
    }
    root["contested_sphere"] = contested_array;

    // Serialize conflicts
    Json::Value conflicts_array(Json::arrayValue);
    for (const auto& conflict : sphere_conflicts) {
        Json::Value conflict_json;
        conflict_json["conflict_id"] = conflict.conflict_id;
        conflict_json["contested_realm"] = static_cast<int>(conflict.contested_realm);
        conflict_json["primary"] = static_cast<int>(conflict.primary_influencer);
        conflict_json["challenger"] = static_cast<int>(conflict.challenging_influencer);
        conflict_json["type"] = static_cast<int>(conflict.conflict_type);
        conflict_json["primary_strength"] = conflict.primary_strength;
        conflict_json["challenger_strength"] = conflict.challenger_strength;
        conflict_json["tension"] = conflict.tension_level;
        conflict_json["is_flashpoint"] = conflict.is_flashpoint;
        conflict_json["escalation_risk"] = conflict.escalation_risk;

        auto conflict_start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            conflict.conflict_start.time_since_epoch()).count();
        conflict_json["conflict_start_ms"] = static_cast<Json::Int64>(conflict_start_ms);

        Json::Value incidents_array(Json::arrayValue);
        for (const auto& incident : conflict.incidents) {
            incidents_array.append(incident);
        }
        conflict_json["incidents"] = incidents_array;

        conflicts_array.append(conflict_json);
    }
    root["sphere_conflicts"] = conflicts_array;

    return root;
}

std::string InfluenceComponent::Serialize() const {
    Json::Value root = SerializeToJson();
    Json::StreamWriterBuilder writer;
    return Json::writeString(writer, root);
}

void InfluenceComponent::DeserializeFromJson(const Json::Value& root) {
    // Basic data
    if (root.isMember("realm_id")) {
        realm_id = static_cast<types::EntityID>(root["realm_id"].asUInt());
    }
    if (root.isMember("sphere_size")) {
        sphere_size = root["sphere_size"].asDouble();
    }
    if (root.isMember("sphere_strength")) {
        sphere_strength = root["sphere_strength"].asDouble();
    }

    // Deserialize influence projection
    if (root.isMember("influence_projection")) {
        influence_projection.clear();
        const Json::Value& projection = root["influence_projection"];
        for (const auto& key : projection.getMemberNames()) {
            // Convert string back to InfluenceType
            for (int i = 0; i < static_cast<int>(InfluenceType::COUNT); ++i) {
                InfluenceType type = static_cast<InfluenceType>(i);
                if (key == InfluenceTypeToString(type)) {
                    influence_projection[type] = projection[key].asDouble();
                    break;
                }
            }
        }
    }

    // Deserialize influenced realms
    if (root.isMember("influenced_realms") && root["influenced_realms"].isArray()) {
        influenced_realms.clear();
        const Json::Value& influenced = root["influenced_realms"];
        for (const auto& state_json : influenced) {
            types::EntityID realm = static_cast<types::EntityID>(state_json["realm_id"].asUInt());
            InfluenceState state(realm);

            state.total_influence_received = state_json.get("total_influence", 0.0).asDouble();
            state.autonomy = state_json.get("autonomy", 1.0).asDouble();
            state.diplomatic_freedom = state_json.get("diplomatic_freedom", 1.0).asDouble();
            state.resistance_strength = state_json.get("resistance", 0.0).asDouble();
            state.actively_resisting = state_json.get("actively_resisting", false).asBool();

            // Deserialize influences by type
            if (state_json.isMember("influences_by_type")) {
                const Json::Value& influences_by_type = state_json["influences_by_type"];
                for (const auto& type_key : influences_by_type.getMemberNames()) {
                    // Convert string to InfluenceType
                    for (int i = 0; i < static_cast<int>(InfluenceType::COUNT); ++i) {
                        InfluenceType type = static_cast<InfluenceType>(i);
                        if (type_key == InfluenceTypeToString(type)) {
                            const Json::Value& sources_array = influences_by_type[type_key];
                            for (const auto& source_json : sources_array) {
                                InfluenceSource source;
                                source.source_realm = static_cast<types::EntityID>(source_json["source_realm"].asUInt());
                                source.type = static_cast<InfluenceType>(source_json["type"].asInt());
                                source.base_strength = source_json.get("base_strength", 0.0).asDouble();
                                source.distance_modifier = source_json.get("distance_mod", 1.0).asDouble();
                                source.relationship_modifier = source_json.get("relationship_mod", 1.0).asDouble();
                                source.effective_strength = source_json.get("effective_strength", 0.0).asDouble();
                                source.hops_from_source = source_json.get("hops", 0).asInt();
                                source.targets_whole_realm = source_json.get("targets_whole_realm", true).asBool();

                                // Deserialize path
                                if (source_json.isMember("path")) {
                                    const Json::Value& path_array = source_json["path"];
                                    source.path.clear();
                                    for (const auto& realm_id : path_array) {
                                        source.path.push_back(static_cast<types::EntityID>(realm_id.asUInt()));
                                    }
                                }

                                // Deserialize timestamps
                                if (source_json.isMember("established_ms")) {
                                    auto ms = source_json["established_ms"].asInt64();
                                    source.established_date = std::chrono::system_clock::time_point(
                                        std::chrono::milliseconds(ms));
                                }
                                if (source_json.isMember("last_update_ms")) {
                                    auto ms = source_json["last_update_ms"].asInt64();
                                    source.last_update = std::chrono::system_clock::time_point(
                                        std::chrono::milliseconds(ms));
                                }

                                state.influences_by_type[type].push_back(source);
                            }
                            break;
                        }
                    }
                }
            }

            influenced_realms[realm] = state;
        }
    }

    // Deserialize incoming influence (CRITICAL for save/load)
    if (root.isMember("incoming_influence")) {
        const Json::Value& incoming = root["incoming_influence"];

        if (incoming.isMember("realm_id")) {
            incoming_influence.affected_realm = static_cast<types::EntityID>(incoming["realm_id"].asUInt());
        }
        incoming_influence.total_influence_received = incoming.get("total_influence", 0.0).asDouble();
        incoming_influence.autonomy = incoming.get("autonomy", 1.0).asDouble();
        incoming_influence.diplomatic_freedom = incoming.get("diplomatic_freedom", 1.0).asDouble();
        incoming_influence.resistance_strength = incoming.get("resistance", 0.0).asDouble();
        incoming_influence.actively_resisting = incoming.get("actively_resisting", false).asBool();

        // Deserialize incoming influences by type
        if (incoming.isMember("influences_by_type")) {
            incoming_influence.influences_by_type.clear();
            const Json::Value& influences_by_type = incoming["influences_by_type"];
            for (const auto& type_key : influences_by_type.getMemberNames()) {
                // Convert string to InfluenceType
                for (int i = 0; i < static_cast<int>(InfluenceType::COUNT); ++i) {
                    InfluenceType type = static_cast<InfluenceType>(i);
                    if (type_key == InfluenceTypeToString(type)) {
                        const Json::Value& sources_array = influences_by_type[type_key];
                        for (const auto& source_json : sources_array) {
                            InfluenceSource source;
                            source.source_realm = static_cast<types::EntityID>(source_json["source_realm"].asUInt());
                            source.type = static_cast<InfluenceType>(source_json["type"].asInt());
                            source.base_strength = source_json.get("base_strength", 0.0).asDouble();
                            source.distance_modifier = source_json.get("distance_mod", 1.0).asDouble();
                            source.relationship_modifier = source_json.get("relationship_mod", 1.0).asDouble();
                            source.effective_strength = source_json.get("effective_strength", 0.0).asDouble();
                            source.hops_from_source = source_json.get("hops", 0).asInt();
                            source.targets_whole_realm = source_json.get("targets_whole_realm", true).asBool();

                            // Deserialize path
                            if (source_json.isMember("path")) {
                                const Json::Value& path_array = source_json["path"];
                                source.path.clear();
                                for (const auto& realm_id : path_array) {
                                    source.path.push_back(static_cast<types::EntityID>(realm_id.asUInt()));
                                }
                            }

                            // Deserialize timestamps
                            if (source_json.isMember("established_ms")) {
                                auto ms = source_json["established_ms"].asInt64();
                                source.established_date = std::chrono::system_clock::time_point(
                                    std::chrono::milliseconds(ms));
                            }
                            if (source_json.isMember("last_update_ms")) {
                                auto ms = source_json["last_update_ms"].asInt64();
                                source.last_update = std::chrono::system_clock::time_point(
                                    std::chrono::milliseconds(ms));
                            }

                            incoming_influence.influences_by_type[type].push_back(source);
                        }
                        break;
                    }
                }
            }
        }

        // Recalculate dominant influencers after deserialization
        incoming_influence.UpdateDominantInfluencers();
    }

    // TODO: Deserialize influenced_vassals (not yet actively used)
    // TODO: Deserialize foreign_vassals (not yet actively used)
    // TODO: Deserialize influenced_characters (not yet actively used)

    // Deserialize spheres
    if (root.isMember("core_sphere") && root["core_sphere"].isArray()) {
        core_sphere.clear();
        for (const auto& id : root["core_sphere"]) {
            core_sphere.push_back(static_cast<types::EntityID>(id.asUInt()));
        }
    }

    if (root.isMember("peripheral_sphere") && root["peripheral_sphere"].isArray()) {
        peripheral_sphere.clear();
        for (const auto& id : root["peripheral_sphere"]) {
            peripheral_sphere.push_back(static_cast<types::EntityID>(id.asUInt()));
        }
    }

    if (root.isMember("contested_sphere") && root["contested_sphere"].isArray()) {
        contested_sphere.clear();
        for (const auto& id : root["contested_sphere"]) {
            contested_sphere.push_back(static_cast<types::EntityID>(id.asUInt()));
        }
    }

    // Deserialize conflicts
    if (root.isMember("sphere_conflicts") && root["sphere_conflicts"].isArray()) {
        sphere_conflicts.clear();
        const Json::Value& conflicts_array = root["sphere_conflicts"];
        for (const auto& conflict_json : conflicts_array) {
            InfluenceConflict conflict;
            conflict.conflict_id = conflict_json.get("conflict_id", "").asString();
            conflict.contested_realm = static_cast<types::EntityID>(conflict_json["contested_realm"].asUInt());
            conflict.primary_influencer = static_cast<types::EntityID>(conflict_json["primary"].asUInt());
            conflict.challenging_influencer = static_cast<types::EntityID>(conflict_json["challenger"].asUInt());
            conflict.conflict_type = static_cast<InfluenceType>(conflict_json["type"].asInt());
            conflict.primary_strength = conflict_json.get("primary_strength", 0.0).asDouble();
            conflict.challenger_strength = conflict_json.get("challenger_strength", 0.0).asDouble();
            conflict.tension_level = conflict_json.get("tension", 0.0).asDouble();
            conflict.is_flashpoint = conflict_json.get("is_flashpoint", false).asBool();
            conflict.escalation_risk = conflict_json.get("escalation_risk", 0.0).asDouble();

            if (conflict_json.isMember("conflict_start_ms")) {
                auto ms = conflict_json["conflict_start_ms"].asInt64();
                conflict.conflict_start = std::chrono::system_clock::time_point(
                    std::chrono::milliseconds(ms));
            }

            if (conflict_json.isMember("incidents") && conflict_json["incidents"].isArray()) {
                for (const auto& incident : conflict_json["incidents"]) {
                    conflict.incidents.push_back(incident.asString());
                }
            }

            sphere_conflicts.push_back(conflict);
        }
    }
}

bool InfluenceComponent::Deserialize(const std::string& data) {
    Json::CharReaderBuilder reader;
    Json::Value root;
    std::string errs;
    std::istringstream stream(data);

    if (!Json::parseFromStream(reader, stream, &root, &errs)) {
        return false;
    }

    DeserializeFromJson(root);
    return true;
}

} // namespace diplomacy
} // namespace game
