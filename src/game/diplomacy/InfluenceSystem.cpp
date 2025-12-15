// ============================================================================
// InfluenceSystem.cpp - Sphere of Influence Management Implementation
// Created: November 11, 2025 - Phase 3: Sphere of Influence System
// Location: src/game/diplomacy/InfluenceSystem.cpp
// ============================================================================

#include "game/diplomacy/InfluenceSystem.h"
#include "game/diplomacy/InfluenceSystemIntegration.h"
#include "game/diplomacy/DiplomacySystem.h"
#include "game/systems/CharacterSystem.h"
#include "game/components/CharacterComponent.h"
#include "game/character/CharacterRelationships.h"
#include "core/ECS/EntityManager.h"
#include <algorithm>
#include <queue>
#include <unordered_set>

namespace game {
namespace diplomacy {

// Namespace aliases (must match header)
namespace ecs = ::core::ecs;

// ============================================================================
// Configuration Constants
// ============================================================================

// Number of game months before stale character influences are removed
constexpr uint32_t INFLUENCE_DECAY_MONTHS = 12;

// ============================================================================
// Constructor and Initialization
// ============================================================================

InfluenceSystem::InfluenceSystem(ComponentAccessManager& componentAccess)
    : m_componentAccess(componentAccess)
    , m_diplomacy_system(nullptr)
    , m_current_month(0)
    , m_initialized(false)
{
}

void InfluenceSystem::Initialize() {
    if (m_initialized) return;

    // Build initial realm network
    BuildRealmNetwork();

    // Calculate initial influence projections for all realms
    for (auto& [realm_id, component] : m_influence_components) {
        CalculateInfluenceProjection(realm_id);
    }

    // Propagate influence through network
    for (auto& [realm_id, component] : m_influence_components) {
        PropagateInfluence(realm_id);
    }

    m_initialized = true;
}

// ============================================================================
// Monthly Updates
// ============================================================================

void InfluenceSystem::MonthlyUpdate() {
    m_current_month++;

    // Rebuild realm network (realms may have changed)
    BuildRealmNetwork();

    // Update all realm influence projections
    for (auto& [realm_id, component] : m_influence_components) {
        CalculateInfluenceProjection(realm_id);
    }

    // Propagate influence through network
    for (auto& [realm_id, component] : m_influence_components) {
        PropagateInfluence(realm_id);
    }

    // Update sphere metrics
    for (auto& [realm_id, component] : m_influence_components) {
        UpdateSphereMetrics(realm_id);
    }

    // Update vassal and character influences
    for (auto& [realm_id, component] : m_influence_components) {
        UpdateVassalInfluences(realm_id);
        UpdateCharacterInfluences(realm_id);
    }

    // Process influence decay
    ProcessInfluenceDecay();

    // Update autonomy and diplomatic freedom
    UpdateAutonomyAndFreedom();

    // Detect and update conflicts
    UpdateSphereConflicts();

    // Check for flashpoints
    CheckForFlashpoints();
}

void InfluenceSystem::UpdateRealmInfluence(types::EntityID realm_id) {
    if (!m_influence_components.count(realm_id)) return;

    CalculateInfluenceProjection(realm_id);
    PropagateInfluence(realm_id);
    UpdateSphereMetrics(realm_id);
}

// ============================================================================
// Influence Calculation
// ============================================================================

void InfluenceSystem::CalculateInfluenceProjection(types::EntityID realm_id) {
    auto* component = GetInfluenceComponent(realm_id);
    if (!component) return;

    const auto* realm = GetRealmComponent(realm_id);
    if (!realm) return;

    // Calculate base projection for each influence type
    component->influence_projection[InfluenceType::MILITARY] =
        InfluenceCalculator::CalculateMilitaryInfluence(*realm);

    component->influence_projection[InfluenceType::ECONOMIC] =
        InfluenceCalculator::CalculateEconomicInfluence(*realm);

    component->influence_projection[InfluenceType::PRESTIGE] =
        InfluenceCalculator::CalculatePrestigeInfluence(*realm);

    // Religious and cultural need target realm context, will be calculated in propagation
    // Dynastic and personal need diplomatic state, will be calculated in propagation
}

InfluenceSource InfluenceSystem::CalculateInfluenceBetween(
    types::EntityID source_realm,
    types::EntityID target_realm,
    InfluenceType type)
{
    InfluenceSource influence(source_realm, type);

    const auto* source = GetRealmComponent(source_realm);
    const auto* target = GetRealmComponent(target_realm);

    if (!source || !target) return influence;

    // Get diplomatic state
    const auto* diplo_state = GetDiplomaticState(source_realm, target_realm);

    // Calculate base strength based on type
    switch(type) {
        case InfluenceType::MILITARY:
            influence.base_strength = InfluenceCalculator::CalculateMilitaryInfluence(*source, diplo_state);
            break;

        case InfluenceType::ECONOMIC:
            influence.base_strength = InfluenceCalculator::CalculateEconomicInfluence(*source, diplo_state);
            break;

        case InfluenceType::DYNASTIC: {
            const auto* source_dynasty = GetDynastyComponent(source->currentRuler);
            const auto* target_dynasty = GetDynastyComponent(target->currentRuler);
            influence.base_strength = InfluenceCalculator::CalculateDynasticInfluence(
                *source, *target, source_dynasty, target_dynasty,
                &m_componentAccess, m_character_system);
            break;
        }

        case InfluenceType::PERSONAL:
            influence.base_strength = InfluenceCalculator::CalculatePersonalInfluence(
                *source, *target, diplo_state);
            break;

        case InfluenceType::RELIGIOUS:
            influence.base_strength = InfluenceCalculator::CalculateReligiousInfluence(*source, *target);
            break;

        case InfluenceType::CULTURAL:
            influence.base_strength = InfluenceCalculator::CalculateCulturalInfluence(*source, *target);
            break;

        case InfluenceType::PRESTIGE: {
            const auto* dynasty = GetDynastyComponent(source->currentRuler);
            influence.base_strength = InfluenceCalculator::CalculatePrestigeInfluence(*source, dynasty);
            break;
        }

        default:
            influence.base_strength = 0.0;
            break;
    }

    return influence;
}

void InfluenceSystem::ApplyModifiersToInfluence(
    InfluenceSource& influence,
    int hops,
    const std::vector<types::EntityID>& path,
    int opinion)
{
    // Apply distance modifier
    influence.UpdateDistanceModifier(hops, path);

    // Apply relationship modifier
    influence.UpdateRelationshipModifier(opinion);

    // Calculate final effective strength
    influence.CalculateEffectiveStrength();
}

// ============================================================================
// Influence Propagation
// ============================================================================

void InfluenceSystem::PropagateInfluence(types::EntityID source_realm) {
    auto* source_component = GetInfluenceComponent(source_realm);
    if (!source_component) return;

    // Clear existing influenced realms
    source_component->influenced_realms.clear();

    // Get all realms within range
    auto reachable_realms = GetRealmsWithinRange(source_realm, MAX_INFLUENCE_HOPS);

    // Calculate influence on each reachable realm
    for (const auto& target_realm : reachable_realms) {
        if (target_realm == source_realm) continue;  // Don't influence self

        // Find path to target
        auto path = FindPathBetweenRealms(source_realm, target_realm);
        if (path.empty()) continue;

        int hops = static_cast<int>(path.size()) - 1;

        // Get opinion for relationship modifier
        const auto* diplo_state = GetDiplomaticState(source_realm, target_realm);
        int opinion = diplo_state ? diplo_state->opinion : 0;

        // Calculate each type of influence
        for (int i = 0; i < static_cast<int>(InfluenceType::COUNT); ++i) {
            InfluenceType type = static_cast<InfluenceType>(i);

            // Calculate base influence
            InfluenceSource influence = CalculateInfluenceBetween(source_realm, target_realm, type);

            // Apply modifiers
            ApplyModifiersToInfluence(influence, hops, path, opinion);

            // Only add if effective strength is above threshold
            if (influence.effective_strength >= MIN_INFLUENCE_THRESHOLD) {
                // Update target realm's incoming influence
                auto* target_component = GetInfluenceComponent(target_realm);
                if (target_component) {
                    target_component->incoming_influence.AddInfluence(influence);
                }

                // Track in source realm's influenced realms
                auto& target_state = source_component->influenced_realms[target_realm];
                target_state.affected_realm = target_realm;
                target_state.AddInfluence(influence);
            }
        }
    }

    // Update sphere metrics after propagation
    source_component->UpdateSphereMetrics();
}

std::vector<types::EntityID> InfluenceSystem::FindPathBetweenRealms(
    types::EntityID source,
    types::EntityID target,
    int max_hops)
{
    if (source == target) return {source};

    // Breadth-first search with propagation blocking
    std::queue<std::vector<types::EntityID>> queue;
    std::unordered_set<types::EntityID> visited;

    queue.push({source});
    visited.insert(source);

    while (!queue.empty()) {
        auto path = queue.front();
        queue.pop();

        types::EntityID current = path.back();

        // Check hop limit
        if (static_cast<int>(path.size()) > max_hops) continue;

        // Get adjacent realms
        auto adjacent = GetAdjacentRealms(current);

        for (const auto& next : adjacent) {
            if (visited.count(next)) continue;

            // Check if influence can propagate through this connection
            if (!CanInfluencePropagate(source, current, target)) {
                continue;
            }

            auto new_path = path;
            new_path.push_back(next);

            // Found target
            if (next == target) {
                return new_path;
            }

            visited.insert(next);
            queue.push(new_path);
        }
    }

    // No path found
    return {};
}

std::vector<types::EntityID> InfluenceSystem::GetRealmsWithinRange(
    types::EntityID source,
    int max_hops)
{
    std::vector<types::EntityID> reachable;
    std::queue<std::pair<types::EntityID, int>> queue;
    std::unordered_set<types::EntityID> visited;

    queue.push({source, 0});
    visited.insert(source);
    reachable.push_back(source);

    while (!queue.empty()) {
        auto [current, hops] = queue.front();
        queue.pop();

        if (hops >= max_hops) continue;

        auto adjacent = GetAdjacentRealms(current);

        for (const auto& next : adjacent) {
            if (visited.count(next)) continue;

            // Check if influence can propagate to this realm
            // Note: for range calculation, we check if we can reach 'next' from 'current'
            if (!CanInfluencePropagate(source, current, next)) {
                continue;
            }

            visited.insert(next);
            reachable.push_back(next);
            queue.push({next, hops + 1});
        }
    }

    return reachable;
}

bool InfluenceSystem::AreRealmsConnected(types::EntityID realm1, types::EntityID realm2) {
    auto path = FindPathBetweenRealms(realm1, realm2);
    return !path.empty();
}

// ============================================================================
// Sphere of Influence Management
// ============================================================================

void InfluenceSystem::UpdateSphereMetrics(types::EntityID realm_id) {
    auto* component = GetInfluenceComponent(realm_id);
    if (!component) return;

    component->UpdateSphereMetrics();
}

std::vector<InfluenceConflict> InfluenceSystem::DetectSphereConflicts(types::EntityID realm_id) {
    std::vector<InfluenceConflict> conflicts;

    auto* component = GetInfluenceComponent(realm_id);
    if (!component) return conflicts;

    // Check each influenced realm for competition
    for (const auto& [target_realm, target_state] : component->influenced_realms) {
        // Look for other realms influencing the same target
        auto* target_component = GetInfluenceComponent(target_realm);
        if (!target_component) continue;

        // Check each influence type for competition
        for (const auto& [type, sources] : target_component->incoming_influence.influences_by_type) {
            if (sources.size() < 2) continue;  // No competition if only one influencer

            // Find our influence and strongest competitor
            const InfluenceSource* our_influence = nullptr;
            const InfluenceSource* competitor_influence = nullptr;

            for (const auto& source : sources) {
                if (source.source_realm == realm_id) {
                    our_influence = &source;
                } else {
                    if (!competitor_influence ||
                        source.effective_strength > competitor_influence->effective_strength) {
                        competitor_influence = &source;
                    }
                }
            }

            // Create conflict if both exist and are significant
            if (our_influence && competitor_influence &&
                our_influence->effective_strength >= MIN_INFLUENCE_THRESHOLD &&
                competitor_influence->effective_strength >= MIN_INFLUENCE_THRESHOLD)
            {
                InfluenceConflict conflict(target_realm, realm_id, competitor_influence->source_realm);
                conflict.conflict_type = type;
                conflict.primary_strength = our_influence->effective_strength;
                conflict.challenger_strength = competitor_influence->effective_strength;
                conflict.CalculateTension();

                conflicts.push_back(conflict);
            }
        }
    }

    return conflicts;
}

std::vector<InfluenceConflict> InfluenceSystem::CheckForFlashpoints() {
    std::vector<InfluenceConflict> flashpoints;

    for (auto& conflict : m_active_conflicts) {
        if (conflict.CheckFlashpoint()) {
            flashpoints.push_back(conflict);
        }
    }

    return flashpoints;
}

void InfluenceSystem::UpdateSphereConflicts() {
    // Clear old conflicts
    m_active_conflicts.clear();

    // Detect new conflicts for all realms
    for (const auto& [realm_id, component] : m_influence_components) {
        auto conflicts = DetectSphereConflicts(realm_id);
        m_active_conflicts.insert(m_active_conflicts.end(), conflicts.begin(), conflicts.end());
    }

    // Update tension and escalation risk
    for (auto& conflict : m_active_conflicts) {
        conflict.CalculateTension();
        conflict.UpdateEscalationRisk();
    }
}

void InfluenceSystem::ProcessConflictEscalation() {
    for (auto& conflict : m_active_conflicts) {
        // Only process high-tension conflicts
        if (conflict.tension_level < 50.0) continue;

        // Generate incidents based on tension level
        if (conflict.tension_level > 80.0 && conflict.incidents.size() < 3) {
            // High tension generates incidents
            if (rand() % 100 < static_cast<int>(conflict.tension_level - 50)) {
                GenerateDiplomaticIncident(conflict, "sphere_competition");
            }
        }

        // Check for escalation
        if (conflict.is_flashpoint) {
            ResolveSphereConflict(conflict);
        }
    }
}

void InfluenceSystem::ResolveSphereConflict(InfluenceConflict& conflict) {
    // Calculate AI responses for both sides
    int primary_response = CalculateAICompetitionResponse(
        conflict.primary_influencer, conflict);
    int challenger_response = CalculateAICompetitionResponse(
        conflict.challenging_influencer, conflict);

    // Determine outcome based on responses
    // 0 = back down, 1 = hold ground, 2 = escalate

    if (primary_response == 0 && challenger_response == 0) {
        // Both back down - peaceful resolution, contested realm gains autonomy
        ApplyConflictOutcome(conflict, types::INVALID_ENTITY, types::INVALID_ENTITY, true);
    }
    else if (primary_response == 0) {
        // Primary backs down, challenger wins
        ApplyConflictOutcome(conflict, conflict.challenging_influencer,
                           conflict.primary_influencer, true);
    }
    else if (challenger_response == 0) {
        // Challenger backs down, primary wins
        ApplyConflictOutcome(conflict, conflict.primary_influencer,
                           conflict.challenging_influencer, true);
    }
    else if (primary_response == 2 || challenger_response == 2) {
        // At least one side escalates - crisis/war
        GenerateDiplomaticIncident(conflict, "sphere_crisis");

        // Check if this leads to war (requires diplomacy system integration)
        if (m_diplomacy_system && conflict.escalation_risk > 0.8) {
            // Add opinion penalty for crisis
            if (auto primary_diplo = m_diplomacy_system->GetDiplomacyComponent(conflict.primary_influencer)) {
                primary_diplo->ModifyOpinion(conflict.challenging_influencer, -30, "Sphere of influence crisis");
            }
            if (auto challenger_diplo = m_diplomacy_system->GetDiplomacyComponent(conflict.challenging_influencer)) {
                challenger_diplo->ModifyOpinion(conflict.primary_influencer, -30, "Sphere of influence crisis");
            }

            // TODO: Trigger war if escalation_risk > 0.9 and relations already hostile
            // This requires fuller DiplomacySystem::DeclareWar() integration
        }

        // Stronger side wins the influence contest
        if (conflict.primary_strength > conflict.challenger_strength) {
            ApplyConflictOutcome(conflict, conflict.primary_influencer,
                               conflict.challenging_influencer, false);
        } else {
            ApplyConflictOutcome(conflict, conflict.challenging_influencer,
                               conflict.primary_influencer, false);
        }
    }
    else {
        // Both hold ground - ongoing competition, increase tension
        conflict.tension_level = std::min(100.0, conflict.tension_level + 10.0);
        GenerateDiplomaticIncident(conflict, "sphere_standoff");
    }
}

void InfluenceSystem::GenerateDiplomaticIncident(
    const InfluenceConflict& conflict,
    const std::string& incident_type)
{
    // Build incident description
    std::string incident_desc = incident_type + "_realm_" +
                                std::to_string(conflict.contested_realm);

    // Add to conflict's incident list
    const_cast<InfluenceConflict&>(conflict).AddIncident(incident_desc);

    // Add opinion penalties through diplomacy system
    if (m_diplomacy_system) {
        int opinion_penalty = 0;

        if (incident_type == "sphere_competition") {
            opinion_penalty = -5;
        } else if (incident_type == "sphere_crisis") {
            opinion_penalty = -20;
        } else if (incident_type == "sphere_standoff") {
            opinion_penalty = -10;
        }

        if (opinion_penalty != 0) {
            // Apply mutual opinion penalty
            if (auto primary_diplo = m_diplomacy_system->GetDiplomacyComponent(conflict.primary_influencer)) {
                primary_diplo->ModifyOpinion(conflict.challenging_influencer, opinion_penalty,
                                            "Sphere of influence " + incident_type);
            }
            if (auto challenger_diplo = m_diplomacy_system->GetDiplomacyComponent(conflict.challenging_influencer)) {
                challenger_diplo->ModifyOpinion(conflict.primary_influencer, opinion_penalty,
                                              "Sphere of influence " + incident_type);
            }
        }
    }
}

int InfluenceSystem::CalculateAICompetitionResponse(
    types::EntityID realm_id,
    const InfluenceConflict& conflict)
{
    // Determine if this realm is the primary or challenger
    bool is_primary = (realm_id == conflict.primary_influencer);
    double our_strength = is_primary ? conflict.primary_strength : conflict.challenger_strength;
    double their_strength = is_primary ? conflict.challenger_strength : conflict.primary_strength;

    // Factors influencing decision:
    // 1. Relative strength
    double strength_ratio = (their_strength > 0) ? (our_strength / their_strength) : 2.0;

    // 2. Realm power (military, economic)
    const auto* our_realm = GetRealmComponent(realm_id);
    double power_factor = 1.0;
    if (our_realm) {
        // Calculate military strength from standing army and levies
        double military_strength = (our_realm->standingArmy * 2.0) + (our_realm->levySize * 1.0);
        power_factor = (military_strength + our_realm->treasury) / 2000.0;
        power_factor = std::clamp(power_factor, 0.5, 2.0);
    }

    // 3. Diplomatic relationship
    double relationship_factor = 0.0;
    if (m_diplomacy_system) {
        types::EntityID opponent = is_primary ? conflict.challenging_influencer : conflict.primary_influencer;
        const auto* diplo_state = m_diplomacy_system->GetDiplomaticState(realm_id, opponent);
        if (diplo_state) {
            // More hostile = more likely to escalate
            relationship_factor = -diplo_state->opinion / 100.0;  // -1.0 to 1.0
        }
    }

    // 4. Strategic value of contested realm
    double strategic_value = 1.0;
    const auto* contested = GetRealmComponent(conflict.contested_realm);
    if (contested) {
        // More valuable realms worth fighting for
        // Use province count as proxy for population (assume ~1000 people per province)
        double estimated_population = contested->ownedProvinces.size() * 1000.0;
        strategic_value = (estimated_population / 10000.0) + (contested->treasury / 1000.0);
        strategic_value = std::clamp(strategic_value, 0.5, 3.0);
    }

    // Calculate decision score
    double escalate_score = (strength_ratio * 30.0) +
                           (power_factor * 20.0) +
                           (relationship_factor * 25.0) +
                           (strategic_value * 25.0);

    // Normalize to 0-100
    escalate_score = std::clamp(escalate_score, 0.0, 100.0);

    // Decision thresholds
    if (escalate_score < 30.0) {
        return 0;  // Back down - not worth the conflict
    } else if (escalate_score < 70.0) {
        return 1;  // Hold ground - maintain position
    } else {
        return 2;  // Escalate - push harder
    }
}

void InfluenceSystem::ApplyConflictOutcome(
    const InfluenceConflict& conflict,
    types::EntityID winner,
    types::EntityID loser,
    bool peaceful_resolution)
{
    // Apply influence changes
    if (winner != types::INVALID_ENTITY && loser != types::INVALID_ENTITY) {
        // Winner gains prestige, loser loses prestige
        if (m_diplomacy_system) {
            if (auto winner_diplo = m_diplomacy_system->GetDiplomacyComponent(winner)) {
                winner_diplo->prestige += peaceful_resolution ? 5.0 : 10.0;
            }
            if (auto loser_diplo = m_diplomacy_system->GetDiplomacyComponent(loser)) {
                loser_diplo->prestige -= peaceful_resolution ? 3.0 : 8.0;
            }
        }

        // Reduce loser's influence on contested realm
        auto* contested_component = GetInfluenceComponent(conflict.contested_realm);
        if (contested_component) {
            // Find and reduce loser's influence
            for (auto& [type, sources] : contested_component->incoming_influence.influences_by_type) {
                for (auto& source : sources) {
                    if (source.source_realm == loser) {
                        source.base_strength *= 0.7;  // 30% reduction
                        source.CalculateEffectiveStrength();
                    }
                }
            }

            // Recalculate metrics
            contested_component->incoming_influence.CalculateAutonomy();
            contested_component->incoming_influence.CalculateDiplomaticFreedom();
        }

        // Apply opinion changes
        if (m_diplomacy_system) {
            // Winner and loser become more hostile
            if (auto winner_diplo = m_diplomacy_system->GetDiplomacyComponent(winner)) {
                winner_diplo->ModifyOpinion(loser, peaceful_resolution ? -5 : -15,
                                          "Sphere of influence victory");
            }
            if (auto loser_diplo = m_diplomacy_system->GetDiplomacyComponent(loser)) {
                loser_diplo->ModifyOpinion(winner, peaceful_resolution ? -10 : -25,
                                         "Sphere of influence defeat");
            }

            // Contested realm's opinion changes
            if (auto contested_diplo = m_diplomacy_system->GetDiplomacyComponent(conflict.contested_realm)) {
                contested_diplo->ModifyOpinion(winner, peaceful_resolution ? -5 : -15,
                                            "Fought over our realm");
                contested_diplo->ModifyOpinion(loser, 5, "Backed down in sphere competition");
            }
        }
    }
    else {
        // Peaceful mutual withdrawal - contested realm gains autonomy
        auto* contested_component = GetInfluenceComponent(conflict.contested_realm);
        if (contested_component) {
            // Reduce all foreign influences slightly
            for (auto& [type, sources] : contested_component->incoming_influence.influences_by_type) {
                for (auto& source : sources) {
                    source.base_strength *= 0.85;  // 15% reduction
                    source.CalculateEffectiveStrength();
                }
            }

            contested_component->incoming_influence.CalculateAutonomy();
            contested_component->incoming_influence.CalculateDiplomaticFreedom();
        }

        // Slight opinion improvement between competitors
        if (m_diplomacy_system) {
            if (auto primary_diplo = m_diplomacy_system->GetDiplomacyComponent(conflict.primary_influencer)) {
                primary_diplo->ModifyOpinion(conflict.challenging_influencer, 5,
                                           "Peaceful sphere resolution");
            }
            if (auto challenger_diplo = m_diplomacy_system->GetDiplomacyComponent(conflict.challenging_influencer)) {
                challenger_diplo->ModifyOpinion(conflict.primary_influencer, 5,
                                             "Peaceful sphere resolution");
            }
        }
    }
}

// ============================================================================
// Vassal and Character Influence
// ============================================================================

void InfluenceSystem::UpdateVassalInfluences(types::EntityID realm_id) {
    auto* component = GetInfluenceComponent(realm_id);
    if (!component) return;

    const auto* realm = GetRealmComponent(realm_id);
    if (!realm) return;

    // Check each vassal for foreign influence
    for (const auto& vassal_id : realm->vassalRealms) {
        auto* vassal_component = GetInfluenceComponent(vassal_id);
        if (!vassal_component) continue;

        // Check for foreign influences (not from liege)
        for (const auto& [type, sources] : vassal_component->incoming_influence.influences_by_type) {
            for (const auto& source : sources) {
                if (source.source_realm == realm_id) continue;  // Skip liege's own influence

                // Create or update vassal influence record
                bool found = false;
                for (auto& vi : component->influenced_vassals) {
                    if (vi.vassal_id == vassal_id && vi.influencing_realm == source.source_realm) {
                        vi.influence_strength = source.effective_strength;
                        vi.primary_type = type;
                        vi.UpdateMonthly();
                        found = true;
                        break;
                    }
                }

                if (!found && source.effective_strength >= MIN_INFLUENCE_THRESHOLD) {
                    VassalInfluence vi(vassal_id, realm_id, source.source_realm);
                    vi.influence_strength = source.effective_strength;
                    vi.primary_type = type;
                    vi.CalculateEffects(source.effective_strength);
                    component->influenced_vassals.push_back(vi);
                }
            }
        }
    }
}

void InfluenceSystem::UpdateCharacterInfluences(types::EntityID realm_id) {
    auto* component = GetInfluenceComponent(realm_id);
    if (!component) return;

    // Update monthly for existing character influences
    for (auto& ci : component->influenced_characters) {
        ci.CalculateOpinionBias(ci.influence_strength);
    }

    // Detect new character influences if CharacterSystem is available
    if (!m_character_system) {
        return; // Character system not configured, skip character influence detection
    }

    auto* entity_manager = m_componentAccess.GetEntityManager();
    if (!entity_manager) {
        CORE_STREAM_WARN("InfluenceSystem")
            << "UpdateCharacterInfluences: EntityManager is null";
        return;
    }

    // Convert legacy realm ID to versioned EntityID for character system queries
    ecs::EntityID versioned_realm_id{realm_id, 0};

    // Get all characters in this realm
    std::vector<ecs::EntityID> realm_characters =
        m_character_system->GetCharactersByRealm(versioned_realm_id);

    // Scan each character for foreign relationships that could create influence
    for (const auto& char_id : realm_characters) {
        auto char_comp = entity_manager->GetComponent<game::character::CharacterComponent>(char_id);
        if (!char_comp) continue;

        // Verify character belongs to this realm
        if (char_comp->GetPrimaryTitle() != realm_id) continue;

        // Get character relationships
        auto rel_comp = entity_manager->GetComponent<game::character::CharacterRelationshipsComponent>(char_id);
        if (!rel_comp) continue;

        // Check friendships for foreign influence
        for (const auto& [friend_id, relationship] : rel_comp->friends) {
            auto friend_char = entity_manager->GetComponent<game::character::CharacterComponent>(ecs::EntityID{friend_id, 0});
            if (!friend_char) continue;

            types::EntityID foreign_realm = friend_char->GetPrimaryTitle();

            // Skip if friend is in same realm or has no realm
            if (foreign_realm == 0 || foreign_realm == realm_id) continue;

            // Calculate influence strength based on friendship bond strength
            float bond_strength = relationship.bond_strength;
            float influence_amount = (bond_strength / 100.0f) * 15.0f; // Max 15 influence from strong friendship

            if (influence_amount >= 1.0f) {
                // Check if this character influence already exists
                bool found = false;
                for (auto& ci : component->influenced_characters) {
                    if (ci.character_id == static_cast<uint32_t>(char_id.id) &&
                        ci.influencing_realm == foreign_realm) {
                        // Update existing influence
                        ci.influence_strength = std::max(ci.influence_strength, static_cast<double>(influence_amount));
                        ci.last_updated_month = m_current_month;
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    // Create new character influence entry
                    CharacterInfluence new_influence;
                    new_influence.character_id = static_cast<uint32_t>(char_id.id);
                    new_influence.influencing_realm = foreign_realm;
                    new_influence.influence_strength = influence_amount;
                    new_influence.primary_type = InfluenceType::PERSONAL;
                    new_influence.compromised = false;
                    new_influence.influence_start_month = m_current_month;
                    new_influence.last_updated_month = m_current_month;

                    component->influenced_characters.push_back(new_influence);

                    CORE_STREAM_DEBUG("InfluenceSystem")
                        << "Detected character influence: Character " << char_id.id
                        << " in realm " << realm_id << " influenced by realm " << foreign_realm
                        << " (strength: " << influence_amount << ")";
                }
            }
        }
    }

    // Clean up old character influences (decay over time if relationships end)
    component->influenced_characters.erase(
        std::remove_if(component->influenced_characters.begin(),
                      component->influenced_characters.end(),
                      [this](const CharacterInfluence& ci) {
                          // Remove influences that haven't been updated recently
                          return (m_current_month - ci.last_updated_month) > INFLUENCE_DECAY_MONTHS;
                      }),
        component->influenced_characters.end()
    );
}

bool InfluenceSystem::IsVassalAtRiskOfDefection(const VassalInfluence& vassal_influence) {
    return vassal_influence.may_defect || vassal_influence.may_revolt;
}

bool InfluenceSystem::IsCharacterCompromised(const CharacterInfluence& character_influence) {
    return character_influence.compromised;
}

// ============================================================================
// Query Functions
// ============================================================================

InfluenceComponent* InfluenceSystem::GetInfluenceComponent(types::EntityID realm_id) {
    auto it = m_influence_components.find(realm_id);
    return (it != m_influence_components.end()) ? it->second : nullptr;
}

std::vector<types::EntityID> InfluenceSystem::GetSphereOfInfluence(types::EntityID realm_id) {
    std::vector<types::EntityID> sphere;

    auto* component = GetInfluenceComponent(realm_id);
    if (!component) return sphere;

    // Combine core, peripheral, and contested spheres
    sphere.insert(sphere.end(), component->core_sphere.begin(), component->core_sphere.end());
    sphere.insert(sphere.end(), component->peripheral_sphere.begin(), component->peripheral_sphere.end());

    return sphere;
}

types::EntityID InfluenceSystem::GetDominantInfluencer(types::EntityID realm_id, InfluenceType type) {
    auto* component = GetInfluenceComponent(realm_id);
    if (!component) return types::EntityID();

    return component->incoming_influence.GetDominantInfluencer(type);
}

double InfluenceSystem::GetTotalInfluenceOn(types::EntityID realm_id) {
    auto* component = GetInfluenceComponent(realm_id);
    if (!component) return 0.0;

    return component->incoming_influence.total_influence_received;
}

double InfluenceSystem::GetRealmAutonomy(types::EntityID realm_id) {
    auto* component = GetInfluenceComponent(realm_id);
    if (!component) return 1.0;

    return component->incoming_influence.autonomy;
}

double InfluenceSystem::GetRealmDiplomaticFreedom(types::EntityID realm_id) {
    auto* component = GetInfluenceComponent(realm_id);
    if (!component) return 1.0;

    return component->incoming_influence.diplomatic_freedom;
}

bool InfluenceSystem::AreRealmsCompeting(
    types::EntityID realm1,
    types::EntityID realm2,
    types::EntityID contested_realm)
{
    auto* component = GetInfluenceComponent(contested_realm);
    if (!component) return false;

    bool realm1_influences = component->incoming_influence.IsInfluencedBy(realm1);
    bool realm2_influences = component->incoming_influence.IsInfluencedBy(realm2);

    return realm1_influences && realm2_influences;
}

// ============================================================================
// Integration with Other Systems
// ============================================================================

void InfluenceSystem::SetDiplomacySystem(DiplomacySystem* diplomacy_system) {
    m_diplomacy_system = diplomacy_system;
}

const DiplomaticState* InfluenceSystem::GetDiplomaticState(
    types::EntityID realm1,
    types::EntityID realm2)
{
    if (!m_diplomacy_system) return nullptr;

    return m_diplomacy_system->GetDiplomaticState(realm1, realm2);
}

void InfluenceSystem::NotifyInfluenceChange(types::EntityID realm_id) {
    // Notify diplomacy system if influence has significantly changed
    // Could trigger events like "losing independence" or "falling under sphere"

    auto* component = GetInfluenceComponent(realm_id);
    if (!component) return;

    // Check for critical thresholds
    double autonomy = component->incoming_influence.autonomy;
    double freedom = component->incoming_influence.diplomatic_freedom;

    // TODO: Trigger events when thresholds are crossed
    if (autonomy < 0.5) {
        // Realm is losing independence
    }

    if (freedom < 0.3) {
        // Realm has very limited diplomatic freedom
    }
}

// ============================================================================
// Integration Component Support (Phase 3)
// ============================================================================

void InfluenceSystem::EnableIntegration() {
    if (!m_integration_helper) {
        m_integration_helper = std::make_unique<InfluenceSystemIntegrationHelper>();
        m_integration_enabled = true;
    }
}

void InfluenceSystem::SetProvinceAdjacencyManager(game::province::ProvinceAdjacencyManager* manager) {
    if (!m_integration_helper) {
        EnableIntegration();
    }
    m_integration_helper->SetAdjacencyManager(manager);
}

void InfluenceSystem::SetReligionSystemData(game::religion::ReligionSystemData* data) {
    if (!m_integration_helper) {
        EnableIntegration();
    }
    m_integration_helper->SetReligionData(data);
}

void InfluenceSystem::SetCharacterSystem(game::character::CharacterSystem* character_system) {
    // Warn if called after initialization (risky)
    if (m_initialized) {
        CORE_STREAM_WARN("InfluenceSystem")
            << "SetCharacterSystem() called after Initialize() - this may cause inconsistent state";
    }

    m_character_system = character_system;

    if (character_system != nullptr) {
        CORE_STREAM_INFO("InfluenceSystem")
            << "CharacterSystem dependency configured - character influence features enabled";
    } else {
        CORE_STREAM_INFO("InfluenceSystem")
            << "CharacterSystem set to nullptr - character influence features disabled";
    }
}

void InfluenceSystem::RegisterCharacterRelationships(
    types::EntityID char_id,
    game::character::CharacterRelationshipsComponent* component)
{
    if (!m_integration_helper) {
        EnableIntegration();
    }
    m_integration_helper->RegisterCharacterRelationships(char_id, component);
}

void InfluenceSystem::RegisterCharacterReligion(
    types::EntityID char_id,
    game::religion::CharacterReligionComponent* component)
{
    if (!m_integration_helper) {
        EnableIntegration();
    }
    m_integration_helper->RegisterCharacterReligion(char_id, component);
}

void InfluenceSystem::RegisterRealmReligion(
    types::EntityID realm_id,
    game::religion::RealmReligionComponent* component)
{
    if (!m_integration_helper) {
        EnableIntegration();
    }
    m_integration_helper->RegisterRealmReligion(realm_id, component);
}

void InfluenceSystem::UnregisterCharacterRelationships(types::EntityID char_id) {
    if (m_integration_helper) {
        m_integration_helper->UnregisterCharacterRelationships(char_id);
    }
}

void InfluenceSystem::UnregisterCharacterReligion(types::EntityID char_id) {
    if (m_integration_helper) {
        m_integration_helper->UnregisterCharacterReligion(char_id);
    }
}

void InfluenceSystem::UnregisterRealmReligion(types::EntityID realm_id) {
    if (m_integration_helper) {
        m_integration_helper->UnregisterRealmReligion(realm_id);
    }
}

bool InfluenceSystem::IsIntegrationEnabled() const {
    return m_integration_enabled && m_integration_helper &&
           m_integration_helper->IsIntegrationEnabled();
}

// ============================================================================
// Data Access
// ============================================================================

void InfluenceSystem::RegisterInfluenceComponent(types::EntityID realm_id, InfluenceComponent* component) {
    m_influence_components[realm_id] = component;
}

void InfluenceSystem::UnregisterInfluenceComponent(types::EntityID realm_id) {
    m_influence_components.erase(realm_id);
}

const std::unordered_map<types::EntityID, InfluenceComponent*>&
InfluenceSystem::GetAllInfluenceComponents() const {
    return m_influence_components;
}

// ============================================================================
// Internal Helper Functions
// ============================================================================

const realm::RealmComponent* InfluenceSystem::GetRealmComponent(types::EntityID realm_id) {
    // Check cache first
    auto it = m_realm_cache.find(realm_id);
    if (it != m_realm_cache.end()) {
        return it->second;
    }

    // TODO: Query from ECS/component manager
    // For now, return nullptr
    return nullptr;
}

const realm::DynastyComponent* InfluenceSystem::GetDynastyComponent(types::EntityID dynasty_id) {
    // Check cache first
    auto it = m_dynasty_cache.find(dynasty_id);
    if (it != m_dynasty_cache.end()) {
        return it->second;
    }

    // TODO: Query from ECS/component manager
    // For now, return nullptr
    return nullptr;
}

void InfluenceSystem::BuildRealmNetwork() {
    m_realm_network.clear();

    // Build adjacency list for all realms
    for (const auto& [realm_id, component] : m_influence_components) {
        m_realm_network[realm_id] = GetAdjacentRealms(realm_id);
    }
}

std::vector<types::EntityID> InfluenceSystem::GetAdjacentRealms(types::EntityID realm_id) {
    std::vector<types::EntityID> adjacent;

    const auto* realm = GetRealmComponent(realm_id);
    if (!realm) return adjacent;

    // Add vassals
    adjacent.insert(adjacent.end(), realm->vassalRealms.begin(), realm->vassalRealms.end());

    // Add liege
    if (realm->liegeRealm != 0) {
        adjacent.push_back(realm->liegeRealm);
    }

    // Add allies from diplomacy system
    auto allies = GetAllies(realm_id);
    adjacent.insert(adjacent.end(), allies.begin(), allies.end());

    // TODO: Add neighbors (realms sharing borders) - awaiting Province system integration
    // Geographic neighbors will be added when province/map adjacency data is available

    return adjacent;
}

std::vector<types::EntityID> InfluenceSystem::GetAllies(types::EntityID realm_id) {
    std::vector<types::EntityID> allies;

    if (!m_diplomacy_system) return allies;

    // Get diplomacy component for this realm
    auto diplomacy_component = m_diplomacy_system->GetDiplomacyComponent(realm_id);
    if (!diplomacy_component) return allies;

    // Return the allies list
    return diplomacy_component->allies;
}

bool InfluenceSystem::CanInfluencePropagate(
    types::EntityID source,
    types::EntityID intermediate,
    types::EntityID target)
{
    // Influence always propagates to direct vassals and lieges
    const auto* realm = GetRealmComponent(intermediate);
    if (realm) {
        if (realm->liegeRealm == source || realm->liegeRealm == target) return true;

        auto& vassals = realm->vassalRealms;
        if (std::find(vassals.begin(), vassals.end(), source) != vassals.end()) return true;
        if (std::find(vassals.begin(), vassals.end(), target) != vassals.end()) return true;
    }

    // Check diplomatic relationship between source and intermediate
    if (m_diplomacy_system) {
        const auto* diplo_state = m_diplomacy_system->GetDiplomaticState(source, intermediate);
        if (diplo_state) {
            // Block if at war
            if (diplo_state->relation == DiplomaticRelation::AT_WAR) {
                return false;
            }

            // Block if extremely hostile (opinion < -75)
            if (diplo_state->opinion < -75) {
                return false;
            }

            // Block if explicitly closed borders (requires future treaty system enhancement)
            // if (diplo_state->borders_closed) return false;
        }
    }

    // Allow propagation by default
    return true;
}

int InfluenceSystem::CalculateHopDistance(types::EntityID source, types::EntityID target) {
    auto path = FindPathBetweenRealms(source, target);
    return path.empty() ? -1 : static_cast<int>(path.size()) - 1;
}

void InfluenceSystem::UpdateAutonomyAndFreedom() {
    for (auto& [realm_id, component] : m_influence_components) {
        component->incoming_influence.CalculateAutonomy();
        component->incoming_influence.CalculateDiplomaticFreedom();
    }
}

void InfluenceSystem::ProcessInfluenceDecay() {
    // Gradually decay influence over time if not maintained
    for (auto& [realm_id, component] : m_influence_components) {
        for (auto& [target_realm, state] : component->influenced_realms) {
            for (auto& [type, sources] : state.influences_by_type) {
                for (auto& source : sources) {
                    // Apply monthly decay
                    source.effective_strength *= (1.0 - INFLUENCE_DECAY_RATE);

                    // Remove if below threshold
                    if (source.effective_strength < MIN_INFLUENCE_THRESHOLD) {
                        // Will be cleaned up in next propagation
                    }
                }
            }
        }
    }
}

} // namespace diplomacy
} // namespace game
