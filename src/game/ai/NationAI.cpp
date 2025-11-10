// Created: October 7, 2025, 2:30 PM
// Location: src/game/ai/nation/NationAI.cpp
// Part 1 of 2 - Lines 1-500

#include "game/ai/NationAI.h"
#include "game/realm/RealmManager.h"
#include "core/ECS/ComponentAccessManager.h"
#include <algorithm>
#include <iostream>
#include <cmath>

namespace game {
namespace ai {

// ============================================================================
// NationAI Implementation
// ============================================================================

NationAI::NationAI(
    uint32_t actorId,
    game::types::EntityID realmId,
    const std::string& name,
    AI::CharacterArchetype personality)
    : m_actorId(actorId)
    , m_realmId(realmId)
    , m_name(name)
    , m_personality(personality)
    , m_aggressiveness(0.5f)
    , m_riskTolerance(0.5f)
    , m_decisionsExecuted(0)
    , m_componentAccess(nullptr) {
    
    // Set personality-based defaults
    switch (personality) {
        case AI::CharacterArchetype::THE_CONQUEROR:
        case AI::CharacterArchetype::WARRIOR_KING:
            m_aggressiveness = 0.8f;
            m_riskTolerance = 0.7f;
            m_primaryGoal = StrategicGoal::EXPANSION;
            m_secondaryGoal = StrategicGoal::CONSOLIDATION;
            break;
            
        case AI::CharacterArchetype::THE_DIPLOMAT:
            m_aggressiveness = 0.3f;
            m_riskTolerance = 0.4f;
            m_primaryGoal = StrategicGoal::DIPLOMATIC_DOMINANCE;
            m_secondaryGoal = StrategicGoal::ECONOMIC_GROWTH;
            break;
            
        case AI::CharacterArchetype::THE_MERCHANT:
            m_aggressiveness = 0.2f;
            m_riskTolerance = 0.5f;
            m_primaryGoal = StrategicGoal::ECONOMIC_GROWTH;
            m_secondaryGoal = StrategicGoal::TECHNOLOGICAL_ADVANCEMENT;
            break;
            
        case AI::CharacterArchetype::THE_SCHOLAR:
            m_aggressiveness = 0.1f;
            m_riskTolerance = 0.3f;
            m_primaryGoal = StrategicGoal::TECHNOLOGICAL_ADVANCEMENT;
            m_secondaryGoal = StrategicGoal::CULTURAL_SUPREMACY;
            break;
            
        case AI::CharacterArchetype::THE_BUILDER:
            m_aggressiveness = 0.3f;
            m_riskTolerance = 0.4f;
            m_primaryGoal = StrategicGoal::CONSOLIDATION;
            m_secondaryGoal = StrategicGoal::ECONOMIC_GROWTH;
            break;
            
        default:
            m_aggressiveness = 0.5f;
            m_riskTolerance = 0.5f;
            m_primaryGoal = StrategicGoal::CONSOLIDATION;
            m_secondaryGoal = StrategicGoal::ECONOMIC_GROWTH;
            break;
    }
    
    m_lastActivityTime = std::chrono::system_clock::now();
    m_lastStrategicReview = m_lastActivityTime;
}

void NationAI::SetComponentAccess(::core::ecs::ComponentAccessManager* access) {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    m_componentAccess = access;
}

// ============================================================================
// Core AI Processing
// ============================================================================

void NationAI::ProcessInformation(const AI::InformationPacket& packet) {
    std::lock_guard<std::mutex> lock(m_stateMutex);

    // Remember the event
    RememberEvent(packet);

    // React based on information type and relevance
    switch (packet.type) {
        case AI::InformationType::MILITARY_ACTION:
            // Potential threat or opportunity
            if (packet.originatorEntityId != m_realmId) {
                UpdateThreats();
                
                // If high severity, consider military response
                if (packet.severity > 0.7f) {
                    auto decision = EvaluateMilitaryNeeds();
                    if (decision.action == MilitaryDecision::RAISE_LEVIES) {
                        QueueMilitaryDecision(decision);
                    }
                }
            }
            break;
            
        case AI::InformationType::DIPLOMATIC_CHANGE:
            // Update relationship assessments
            UpdateDiplomacy();
            break;
            
        case AI::InformationType::ECONOMIC_CRISIS:
            // Economic opportunity or threat
            if (packet.severity > 0.6f) {
                auto decision = EvaluateEconomicPolicy();
                QueueEconomicDecision(decision);
            }
            break;
            
        case AI::InformationType::SUCCESSION_CRISIS:
            // Opportunity for expansion or alliance
            if (packet.originatorEntityId != m_realmId) {
                auto warDecision = EvaluateWarDecision(packet.originatorEntityId);
                if (warDecision.shouldDeclare && warDecision.expectedSuccess > 0.6f) {
                    QueueWarDecision(warDecision);
                }
            }
            break;
            
        case AI::InformationType::REBELLION:
            // Weakness to exploit or internal problem
            if (packet.sourceProvinceId != 0) {
                auto* realm = GetRealmComponent();
                if (realm) {
                    // Check if it's our province
                    auto& provinces = realm->ownedProvinces;
                    if (std::find(provinces.begin(), provinces.end(), 
                                 packet.sourceProvinceId) != provinces.end()) {
                        // Internal rebellion - military response
                        auto decision = EvaluateMilitaryNeeds();
                        decision.action = MilitaryDecision::MOVE_ARMIES;
                        decision.targetLocation = packet.sourceProvinceId;
                        QueueMilitaryDecision(decision);
                    }
                }
            }
            break;
            
        default:
            break;
    }
    
    // Periodic strategic review
    auto now = std::chrono::system_clock::now();
    auto timeSinceReview = std::chrono::duration_cast<std::chrono::hours>(
        now - m_lastStrategicReview).count();
    
    if (timeSinceReview > 24) { // Daily strategic review
        UpdateStrategy();
        m_lastStrategicReview = now;
    }
}

void NationAI::UpdateStrategy() {
    // NOTE: No lock here - called from ProcessInformation which holds the lock

    // Reassess strategic goals based on current situation
    auto* realm = GetRealmComponent();
    if (!realm) return;
    
    // Check survival conditions
    if (realm->stability < 0.3f || realm->treasury < 100.0) {
        m_primaryGoal = StrategicGoal::SURVIVAL;
        m_aggressiveness *= 0.5f; // Reduce aggression when struggling
        return;
    }
    
    // Evaluate goal achievement
    switch (m_primaryGoal) {
        case StrategicGoal::EXPANSION:
            // Check if we've expanded recently
            if (realm->ownedProvinces.size() > 10) {
                // Switch to consolidation after expansion
                m_primaryGoal = StrategicGoal::CONSOLIDATION;
                m_secondaryGoal = StrategicGoal::EXPANSION;
            }
            break;
            
        case StrategicGoal::ECONOMIC_GROWTH:
            // Check economic health
            if (realm->treasury > 10000.0) {
                // Wealthy enough, consider expansion
                if (m_aggressiveness > 0.5f) {
                    m_primaryGoal = StrategicGoal::EXPANSION;
                } else {
                    m_primaryGoal = StrategicGoal::TECHNOLOGICAL_ADVANCEMENT;
                }
            }
            break;
            
        case StrategicGoal::CONSOLIDATION:
            // Check if realm is stable
            if (realm->stability > 0.8f && realm->legitimacy > 0.8f) {
                // Well consolidated, pursue secondary goal
                std::swap(m_primaryGoal, m_secondaryGoal);
            }
            break;
            
        default:
            break;
    }
    
    // Adjust personality weights based on recent events
    AdjustPersonalityWeights();
    
    // Set new goals if needed
    SetStrategicGoals();
}

void NationAI::ExecuteDecisions() {
    std::lock_guard<std::mutex> lock(m_stateMutex);

    // Execute queued decisions in priority order

    // War decisions (highest priority)
    if (!m_warDecisions.empty()) {
        auto decision = m_warDecisions.front();
        m_warDecisions.pop();
        ExecuteWarDeclaration(decision);
    }
    
    // Military decisions
    if (!m_militaryDecisions.empty()) {
        auto decision = m_militaryDecisions.front();
        m_militaryDecisions.pop();
        ExecuteMilitaryAction(decision);
    }
    
    // Diplomatic decisions
    if (!m_diplomaticDecisions.empty()) {
        auto decision = m_diplomaticDecisions.front();
        m_diplomaticDecisions.pop();
        ExecuteDiplomaticAction(decision);
    }
    
    // Economic decisions (lowest priority)
    if (!m_economicDecisions.empty()) {
        auto decision = m_economicDecisions.front();
        m_economicDecisions.pop();
        ExecuteEconomicPolicy(decision);
    }
    
    m_decisionsExecuted++;
}

// ============================================================================
// Background Updates
// ============================================================================

void NationAI::UpdateEconomy() {
    std::lock_guard<std::mutex> lock(m_stateMutex);

    auto* realm = GetRealmComponent();
    if (!realm) return;
    
    // Evaluate economic health
    float economicHealth = realm->treasury / std::max(1.0, realm->monthlyExpenses * 12.0);
    
    if (economicHealth < 0.5f) {
        // Poor economy - need to act
        auto decision = EvaluateEconomicPolicy();
        
        if (realm->treasury < realm->monthlyExpenses * 3) {
            // Emergency measures
            decision.action = EconomicDecision::ADJUST_TAXES;
            decision.parameter = 0.15f; // Increase taxes
            decision.expectedImpact = realm->monthlyIncome * 0.2f;
            QueueEconomicDecision(decision);
        }
    } else if (economicHealth > 2.0f) {
        // Strong economy - consider investments
        if (m_primaryGoal == StrategicGoal::ECONOMIC_GROWTH) {
            EconomicDecision decision;
            decision.action = EconomicDecision::BUILD_INFRASTRUCTURE;
            decision.parameter = realm->treasury * 0.1f; // Invest 10%
            decision.expectedImpact = realm->monthlyIncome * 0.05f;
            QueueEconomicDecision(decision);
        }
    }
}

void NationAI::UpdateDiplomacy() {
    std::lock_guard<std::mutex> lock(m_stateMutex);

    auto* diplomacy = GetDiplomacyComponent();
    if (!diplomacy) return;
    
    // Update relationship scores
    for (auto& [realmId, relation] : diplomacy->relations) {
        float score = relation.opinion / 100.0f; // Normalize to -1 to 1
        
        // Adjust for treaties
        if (relation.hasAlliance) score += 0.5f;
        if (relation.atWar) score -= 1.0f;
        if (relation.hasTradeAgreement) score += 0.2f;
        
        m_relationshipScores[realmId] = score;
        
        // Consider diplomatic actions
        if (score < -0.5f && !relation.atWar) {
            // Hostile but not at war - prepare
            auto threat = AssessThreat(realmId);
            if (threat >= ThreatLevel::MODERATE) {
                // Seek allies against threat
                for (auto& [otherId, otherScore] : m_relationshipScores) {
                    if (otherId != realmId && otherScore > 0.3f) {
                        DiplomaticDecision decision;
                        decision.action = DiplomaticDecision::FORM_ALLIANCE;
                        decision.targetRealm = otherId;
                        decision.expectedBenefit = 0.5f;
                        QueueDiplomaticDecision(decision);
                        break;
                    }
                }
            }
        } else if (score > 0.7f && !relation.hasAlliance) {
            // Very friendly - consider alliance
            auto decision = EvaluateDiplomacy(realmId);
            if (decision.action == DiplomaticDecision::FORM_ALLIANCE) {
                QueueDiplomaticDecision(decision);
            }
        }
    }
}

void NationAI::UpdateMilitary() {
    std::lock_guard<std::mutex> lock(m_stateMutex);

    auto* realm = GetRealmComponent();
    if (!realm) return;
    
    // Calculate military readiness
    float totalForces = realm->levySize + realm->standingArmy;
    float militaryReadiness = totalForces / std::max(1.0f, realm->ownedProvinces.size() * 200.0f);
    
    // Update based on strategic goals
    if (m_primaryGoal == StrategicGoal::EXPANSION) {
        if (militaryReadiness < 0.7f) {
            // Need more troops for expansion
            MilitaryDecision decision;
            decision.action = MilitaryDecision::RAISE_LEVIES;
            decision.targetSize = realm->ownedProvinces.size() * 300;
            QueueMilitaryDecision(decision);
        }
    } else if (m_primaryGoal == StrategicGoal::SURVIVAL) {
        // Defensive preparations
        if (militaryReadiness < 0.5f) {
            MilitaryDecision decision;
            decision.action = MilitaryDecision::HIRE_MERCENARIES;
            decision.targetSize = 500;
            QueueMilitaryDecision(decision);
        }
    }
}

void NationAI::UpdateThreats() {
    // NOTE: No lock here - called from ProcessInformation/UpdateDiplomacy which hold the lock

    auto* diplomacy = GetDiplomacyComponent();
    if (!diplomacy) return;

    // Clear old assessments
    m_threatAssessment.clear();
    
    for (const auto& [realmId, relation] : diplomacy->relations) {
        ThreatLevel threat = AssessThreat(realmId);
        m_threatAssessment[realmId] = threat;
        
        // React to high threats
        if (threat >= ThreatLevel::SEVERE && !relation.atWar) {
            // Consider preemptive action
            if (m_aggressiveness > 0.6f && m_riskTolerance > 0.5f) {
                auto warDecision = EvaluateWarDecision(realmId);
                if (warDecision.shouldDeclare) {
                    QueueWarDecision(warDecision);
                }
            } else {
                // Defensive preparations
                MilitaryDecision military;
                military.action = MilitaryDecision::BUILD_FORTIFICATIONS;
                QueueMilitaryDecision(military);
            }
        }
    }
}

// ============================================================================
// Decision Making
// ============================================================================

WarDecision NationAI::EvaluateWarDecision(types::EntityID target) {
    WarDecision decision;
    decision.targetRealm = target;
    
    // Calculate relative strength
    float relativeStrength = CalculateRelativeStrength(target);
    decision.expectedSuccess = std::min(1.0f, relativeStrength * 0.7f);
    
    // Estimate costs
    auto* realm = GetRealmComponent();
    if (realm) {
        decision.expectedCost = realm->monthlyExpenses * 12.0f; // 1 year of war
    }
    
    // Determine if we should declare
    float desirability = CalculateWarDesirability(target);
    float threshold = 0.5f - (m_aggressiveness * 0.3f); // More aggressive = lower threshold
    
    decision.shouldDeclare = (desirability > threshold && 
                             decision.expectedSuccess > 0.4f &&
                             decision.expectedCost < realm->treasury * 0.5f);
    
    // Determine casus belli
    decision.justification = realm::CasusBelli::CONQUEST; // Default
    
    return decision;
}

DiplomaticDecision NationAI::EvaluateDiplomacy(types::EntityID target) {
    DiplomaticDecision decision;
    decision.targetRealm = target;
    
    // Get current relationship
    float relationshipScore = 0.0f;
    auto it = m_relationshipScores.find(target);
    if (it != m_relationshipScores.end()) {
        relationshipScore = it->second;
    }
    
    // Determine best action based on relationship and goals
    if (relationshipScore > 0.5f) {
        decision.action = DiplomaticDecision::FORM_ALLIANCE;
        decision.expectedBenefit = CalculateAllianceValue(target);
    } else if (relationshipScore > 0.0f) {
        decision.action = DiplomaticDecision::OFFER_TRADE;
        decision.expectedBenefit = CalculateTradeValue(target);
    } else if (relationshipScore < -0.5f) {
        decision.action = DiplomaticDecision::DENOUNCE;
        decision.expectedBenefit = 0.1f; // Minor benefit from denouncing enemies
    } else {
        decision.action = DiplomaticDecision::IMPROVE_RELATIONS;
        decision.expectedBenefit = 0.2f;
    }
    
    return decision;
}

EconomicDecision NationAI::EvaluateEconomicPolicy() {
    EconomicDecision decision;
    
    auto* realm = GetRealmComponent();
    if (!realm) return decision;
    
    float economicPressure = realm->monthlyExpenses / std::max(1.0, realm->monthlyIncome);
    
    if (economicPressure > 1.2f) {
        // Deficit - need revenue
        decision.action = EconomicDecision::ADJUST_TAXES;
        decision.parameter = 0.02f; // Increase by 2%
        decision.expectedImpact = realm->monthlyIncome * 0.1f;
    } else if (economicPressure < 0.5f && realm->treasury > 5000.0) {
        // Surplus - invest
        decision.action = EconomicDecision::BUILD_INFRASTRUCTURE;
        decision.parameter = 1000.0;
        decision.expectedImpact = realm->monthlyIncome * 0.05f;
    } else {
        // Balanced - promote trade
        decision.action = EconomicDecision::PROMOTE_TRADE;
        decision.parameter = 100.0;
        decision.expectedImpact = realm->monthlyIncome * 0.02f;
    }
    
    return decision;
}

MilitaryDecision NationAI::EvaluateMilitaryNeeds() {
    MilitaryDecision decision;
    
    auto* realm = GetRealmComponent();
    if (!realm) return decision;
    
    float totalThreats = 0.0f;
    for (const auto& [realmId, threat] : m_threatAssessment) {
        totalThreats += static_cast<float>(threat) / 4.0f;
    }
    
    float currentStrength = CalculateMilitaryStrength();
    float neededStrength = totalThreats * 1000.0f; // Scale threat to troops
    
    if (currentStrength < neededStrength * 0.7f) {
        // Need more troops
        decision.action = MilitaryDecision::RAISE_LEVIES;
        decision.targetSize = neededStrength - currentStrength;
    } else if (currentStrength > neededStrength * 1.5f) {
        // Too many troops - reduce costs
        decision.action = MilitaryDecision::DISBAND_TROOPS;
        decision.targetSize = currentStrength - neededStrength;
    } else {
        // Adequate troops - improve quality
        decision.action = MilitaryDecision::BUILD_FORTIFICATIONS;
    }
    
    return decision;
}

// ============================================================================
// Threat Evaluation
// ============================================================================

ThreatLevel NationAI::AssessThreat(types::EntityID realm) const {
    // Get relative military strength
    float relativeStrength = CalculateRelativeStrength(realm);
    
    // Get relationship
    float relationship = 0.0f;
    auto it = m_relationshipScores.find(realm);
    if (it != m_relationshipScores.end()) {
        relationship = it->second;
    }
    
    // Calculate threat
    float threatScore = (1.0f / std::max(0.1f, relativeStrength)) * (1.0f - relationship);
    
    if (threatScore > 2.0f) return ThreatLevel::EXISTENTIAL;
    if (threatScore > 1.5f) return ThreatLevel::SEVERE;
    if (threatScore > 1.0f) return ThreatLevel::MODERATE;
    if (threatScore > 0.5f) return ThreatLevel::LOW;
    return ThreatLevel::MINIMAL;
}

float NationAI::CalculateMilitaryStrength() const {
    auto* realm = GetRealmComponent();
    if (!realm) return 0.0f;
    
    float strength = 0.0f;
    strength += realm->levySize * 1.0f;
    strength += realm->standingArmy * 2.0f; // Professional troops worth more
    
    // Adjust for economic support
    float economicMultiplier = std::min(2.0f, static_cast<float>(realm->treasury) / 1000.0f);
    strength *= economicMultiplier;
    
    return strength;
}

float NationAI::CalculateRelativeStrength(game::types::EntityID other) const {
    float ourStrength = CalculateMilitaryStrength();
    
    // Get other realm's component
    if (m_componentAccess) {
        auto otherRealm = m_componentAccess->GetComponent<realm::RealmComponent>(other).Get();
        if (otherRealm) {
            float theirStrength = 0.0f;
            theirStrength += otherRealm->levySize * 1.0f;
            theirStrength += otherRealm->standingArmy * 2.0f;
            float economicMult = std::min(2.0f, static_cast<float>(otherRealm->treasury) / 1000.0f);
            theirStrength *= economicMult;
            
            return ourStrength / std::max(1.0f, theirStrength);
        }
    }
    
    // Fallback estimate
    return ourStrength / (ourStrength * 0.8f);
}

float NationAI::CalculateAllianceValue(game::types::EntityID target) const {
    float value = 0.0f;
    
    // Get target's military strength
    float theirStrength = 0.0f;
    if (m_componentAccess) {
        auto targetRealm = m_componentAccess->GetComponent<realm::RealmComponent>(target).Get();
        if (targetRealm) {
            theirStrength = targetRealm->levySize + (targetRealm->standingArmy * 2.0f);
        }
    }
    
    // Value based on their strength relative to ours
    float ourStrength = CalculateMilitaryStrength();
    value = theirStrength / std::max(1.0f, ourStrength);
    
    // Higher value if we have common enemies
    for (const auto& [realmId, threat] : m_threatAssessment) {
        if (threat >= ThreatLevel::MODERATE) {
            // Check if target also dislikes this realm
            auto relationIt = m_relationshipScores.find(realmId);
            if (relationIt != m_relationshipScores.end() && relationIt->second < 0.0f) {
                value += 0.3f;
            }
        }
    }
    
    return std::clamp(value, 0.0f, 1.0f);
}

float NationAI::CalculateTradeValue(types::EntityID target) const {
    float value = 0.0f;
    
    if (m_componentAccess) {
        auto* targetRealm = m_componentAccess->GetComponent<realm::RealmComponent>(target).Get();
        if (targetRealm) {
            // Value based on their economic strength
            value = targetRealm->monthlyIncome / 1000.0f;
            
            // Adjust for distance (would need province system)
            // For now, base value
            value *= 0.1f; // 10% of their income potential
        }
    }
    
    return std::clamp(value, 0.0f, 1.0f);
}

float NationAI::CalculateWarDesirability(types::EntityID target) const {
    float desirability = m_aggressiveness; // Base on personality
    
    // Adjust for strategic goals
    if (m_primaryGoal == StrategicGoal::EXPANSION) {
        desirability += 0.3f;
    } else if (m_primaryGoal == StrategicGoal::SURVIVAL) {
        desirability -= 0.5f;
    }
    
    // Adjust for threat level
    auto threatIt = m_threatAssessment.find(target);
    if (threatIt != m_threatAssessment.end()) {
        desirability += static_cast<float>(threatIt->second) * 0.1f;
    }
    
    // Adjust for relationship
    auto relationIt = m_relationshipScores.find(target);
    if (relationIt != m_relationshipScores.end()) {
        desirability -= relationIt->second * 0.3f; // Less desire to attack friends
    }
    
    return std::clamp(desirability, 0.0f, 1.0f);
}

// ============================================================================
// Decision Execution
// ============================================================================

void NationAI::ExecuteWarDeclaration(const WarDecision& decision) {
    // Send war declaration event through message bus
    // This would integrate with the diplomacy system
    
    std::cout << "[NationAI] " << m_name << " declares war on realm " 
              << decision.targetRealm << " (Success chance: " 
              << decision.expectedSuccess << ")" << std::endl;
    
    // Would post DiplomacyEvent::WAR_DECLARED to message bus
}

void NationAI::ExecuteMilitaryAction(const MilitaryDecision& decision) {
    auto* realm = GetRealmComponent();
    if (!realm) return;
    
    switch (decision.action) {
        case MilitaryDecision::RAISE_LEVIES:
            std::cout << "[NationAI] " << m_name << " raising levies (target: " 
                      << decision.targetSize << ")" << std::endl;
            // Would post MilitaryEvent::RAISE_LEVIES
            break;
            
        case MilitaryDecision::DISBAND_TROOPS:
            std::cout << "[NationAI] " << m_name << " disbanding troops (target: " 
                      << decision.targetSize << ")" << std::endl;
            // Would post MilitaryEvent::DISBAND_TROOPS
            break;
            
        case MilitaryDecision::HIRE_MERCENARIES:
            std::cout << "[NationAI] " << m_name << " hiring mercenaries (target: " 
                      << decision.targetSize << ")" << std::endl;
            // Would post MilitaryEvent::HIRE_MERCENARIES
            break;
            
        case MilitaryDecision::BUILD_FORTIFICATIONS:
            std::cout << "[NationAI] " << m_name << " building fortifications" << std::endl;
            // Would post ConstructionEvent::BUILD_FORTIFICATIONS
            break;
            
        case MilitaryDecision::MOVE_ARMIES:
            std::cout << "[NationAI] " << m_name << " moving armies to province " 
                      << decision.targetLocation << std::endl;
            // Would post MilitaryEvent::MOVE_ARMY
            break;
    }
}

void NationAI::ExecuteDiplomaticAction(const DiplomaticDecision& decision) {
    switch (decision.action) {
        case DiplomaticDecision::FORM_ALLIANCE:
            std::cout << "[NationAI] " << m_name << " proposing alliance with realm " 
                      << decision.targetRealm << std::endl;
            // Would post DiplomacyEvent::PROPOSE_ALLIANCE
            break;
            
        case DiplomaticDecision::OFFER_TRADE:
            std::cout << "[NationAI] " << m_name << " offering trade agreement to realm " 
                      << decision.targetRealm << std::endl;
            // Would post DiplomacyEvent::PROPOSE_TRADE
            break;
            
        case DiplomaticDecision::DENOUNCE:
            std::cout << "[NationAI] " << m_name << " denouncing realm " 
                      << decision.targetRealm << std::endl;
            // Would post DiplomacyEvent::DENOUNCE
            break;
            
        case DiplomaticDecision::IMPROVE_RELATIONS:
            std::cout << "[NationAI] " << m_name << " improving relations with realm " 
                      << decision.targetRealm << std::endl;
            // Would post DiplomacyEvent::IMPROVE_RELATIONS
            break;
    }
}

void NationAI::ExecuteEconomicPolicy(const EconomicDecision& decision) {
    auto* realm = GetRealmComponent();
    if (!realm) return;
    
    switch (decision.action) {
        case EconomicDecision::ADJUST_TAXES:
            std::cout << "[NationAI] " << m_name << " adjusting taxes by " 
                      << (decision.parameter * 100.0f) << "%" << std::endl;
            // Would post EconomicEvent::ADJUST_TAXES
            break;
            
        case EconomicDecision::BUILD_INFRASTRUCTURE:
            std::cout << "[NationAI] " << m_name << " investing " << decision.parameter 
                      << " in infrastructure" << std::endl;
            // Would post EconomicEvent::BUILD_INFRASTRUCTURE
            break;
            
        case EconomicDecision::PROMOTE_TRADE:
            std::cout << "[NationAI] " << m_name << " promoting trade (investment: " 
                      << decision.parameter << ")" << std::endl;
            // Would post EconomicEvent::PROMOTE_TRADE
            break;
    }
}

// ============================================================================
// Personality & Strategy Adjustment
// ============================================================================

void NationAI::AdjustPersonalityWeights() {
    // Adjust personality based on recent experiences
    int militaryEvents = 0;
    int economicCrises = 0;
    int diplomaticEvents = 0;
    
    for (const auto& memory : m_recentEvents) {
        switch (memory.type) {
            case AI::InformationType::MILITARY_ACTION:
            case AI::InformationType::REBELLION:
                militaryEvents++;
                break;
            case AI::InformationType::ECONOMIC_CRISIS:
                economicCrises++;
                break;
            case AI::InformationType::DIPLOMATIC_CHANGE:
                diplomaticEvents++;
                break;
            default:
                break;
        }
    }
    
    // Adjust aggressiveness based on military success/failure
    if (militaryEvents > 5) {
        // Frequent military action - become more cautious
        m_aggressiveness *= 0.95f;
        m_riskTolerance *= 0.95f;
    }
    
    // Adjust risk tolerance based on economic crises
    if (economicCrises > 3) {
        m_riskTolerance *= 0.9f; // More cautious when economy struggles
    }
    
    // Clamp values
    m_aggressiveness = std::clamp(m_aggressiveness, 0.1f, 0.9f);
    m_riskTolerance = std::clamp(m_riskTolerance, 0.1f, 0.9f);
}

void NationAI::SetStrategicGoals() {
    auto* realm = GetRealmComponent();
    if (!realm) return;
    
    // Analyze realm situation
    float militaryPower = CalculateMilitaryStrength();
    float economicPower = realm->treasury / 1000.0f;
    float stabilityLevel = realm->stability;
    
    // Determine appropriate goals based on situation and personality
    if (stabilityLevel < 0.3f) {
        // Critical stability - focus on survival
        m_primaryGoal = StrategicGoal::SURVIVAL;
        m_secondaryGoal = StrategicGoal::CONSOLIDATION;
        return;
    }
    
    // Normal goal setting based on personality
    switch (m_personality) {
        case AI::CharacterArchetype::THE_CONQUEROR:
        case AI::CharacterArchetype::WARRIOR_KING:
            if (militaryPower > economicPower) {
                m_primaryGoal = StrategicGoal::EXPANSION;
                m_secondaryGoal = StrategicGoal::CONSOLIDATION;
            } else {
                m_primaryGoal = StrategicGoal::ECONOMIC_GROWTH;
                m_secondaryGoal = StrategicGoal::EXPANSION;
            }
            break;
            
        case AI::CharacterArchetype::THE_DIPLOMAT:
            m_primaryGoal = StrategicGoal::DIPLOMATIC_DOMINANCE;
            m_secondaryGoal = (economicPower > militaryPower) ? 
                StrategicGoal::ECONOMIC_GROWTH : StrategicGoal::CONSOLIDATION;
            break;
            
        case AI::CharacterArchetype::THE_MERCHANT:
            m_primaryGoal = StrategicGoal::ECONOMIC_GROWTH;
            m_secondaryGoal = StrategicGoal::TECHNOLOGICAL_ADVANCEMENT;
            break;
            
        case AI::CharacterArchetype::THE_SCHOLAR:
            m_primaryGoal = StrategicGoal::TECHNOLOGICAL_ADVANCEMENT;
            m_secondaryGoal = StrategicGoal::CULTURAL_SUPREMACY;
            break;
            
        case AI::CharacterArchetype::THE_BUILDER:
            if (stabilityLevel < 0.6f) {
                m_primaryGoal = StrategicGoal::CONSOLIDATION;
                m_secondaryGoal = StrategicGoal::ECONOMIC_GROWTH;
            } else {
                m_primaryGoal = StrategicGoal::ECONOMIC_GROWTH;
                m_secondaryGoal = StrategicGoal::CONSOLIDATION;
            }
            break;
            
        default:
            m_primaryGoal = StrategicGoal::CONSOLIDATION;
            m_secondaryGoal = StrategicGoal::ECONOMIC_GROWTH;
            break;
    }
}

// ============================================================================
// Decision Queue Management
// ============================================================================

void NationAI::QueueWarDecision(const WarDecision& decision) {
    m_warDecisions.push(decision);
}

void NationAI::QueueMilitaryDecision(const MilitaryDecision& decision) {
    m_militaryDecisions.push(decision);
}

void NationAI::QueueDiplomaticDecision(const DiplomaticDecision& decision) {
    m_diplomaticDecisions.push(decision);
}

void NationAI::QueueEconomicDecision(const EconomicDecision& decision) {
    m_economicDecisions.push(decision);
}

// ============================================================================
// Helper Methods
// ============================================================================

void NationAI::RememberEvent(const AI::InformationPacket& packet) {
    EventMemory memory;
    memory.type = packet.type;
    memory.severity = packet.severity;
    memory.timestamp = std::chrono::system_clock::now();
    memory.source = packet.originatorEntityId;
    
    m_recentEvents.push_back(memory);
    
    // Prune old memories (keep last 50)
    if (m_recentEvents.size() > MAX_EVENT_MEMORY) {
        m_recentEvents.erase(m_recentEvents.begin());
    }
}

const realm::RealmComponent* NationAI::GetRealmComponent() const {
    if (!m_componentAccess) return nullptr;
    return m_componentAccess->GetComponent<realm::RealmComponent>(m_realmId).Get();
}

const realm::DiplomaticRelationsComponent* NationAI::GetDiplomacyComponent() const {
    if (!m_componentAccess) return nullptr;
    return m_componentAccess->GetComponent<realm::DiplomaticRelationsComponent>(m_realmId).Get();
}

// ============================================================================
// State Queries
// ============================================================================

uint32_t NationAI::GetActorId() const {
    return m_actorId;
}

game::types::EntityID NationAI::GetRealmId() const {
    return m_realmId;
}

std::string NationAI::GetName() const {
    return m_name;
}

AI::CharacterArchetype NationAI::GetPersonality() const {
    return m_personality;
}

StrategicGoal NationAI::GetPrimaryGoal() const {
    return m_primaryGoal;
}

StrategicGoal NationAI::GetSecondaryGoal() const {
    return m_secondaryGoal;
}

float NationAI::GetAggressiveness() const {
    return m_aggressiveness;
}

float NationAI::GetRiskTolerance() const {
    return m_riskTolerance;
}

size_t NationAI::GetPendingDecisions() const {
    return m_warDecisions.size() + m_militaryDecisions.size() + 
           m_diplomaticDecisions.size() + m_economicDecisions.size();
}

uint64_t NationAI::GetDecisionsExecuted() const {
    return m_decisionsExecuted;
}

std::chrono::system_clock::time_point NationAI::GetLastActivityTime() const {
    return m_lastActivityTime;
}

const std::map<types::EntityID, ThreatLevel>& NationAI::GetThreatAssessment() const {
    return m_threatAssessment;
}

const std::map<types::EntityID, float>& NationAI::GetRelationshipScores() const {
    return m_relationshipScores;
}

// ============================================================================
// Activity Tracking
// ============================================================================

void NationAI::UpdateActivity() {
    m_lastActivityTime = std::chrono::system_clock::now();
}

bool NationAI::IsActive() const {
    auto now = std::chrono::system_clock::now();
    auto timeSinceActivity = std::chrono::duration_cast<std::chrono::hours>(
        now - m_lastActivityTime).count();
    
    // Consider active if we've done something in the last 24 hours
    return timeSinceActivity < 24;
}

// ============================================================================
// Debug & Statistics
// ============================================================================

void NationAI::PrintDebugInfo() const {
    std::cout << "\n=== NationAI Debug Info ===" << std::endl;
    std::cout << "Name: " << m_name << std::endl;
    std::cout << "Actor ID: " << m_actorId << std::endl;
    std::cout << "Realm ID: " << m_realmId << std::endl;
    std::cout << "Personality: " << static_cast<int>(m_personality) << std::endl;
    std::cout << "Primary Goal: " << static_cast<int>(m_primaryGoal) << std::endl;
    std::cout << "Secondary Goal: " << static_cast<int>(m_secondaryGoal) << std::endl;
    std::cout << "Aggressiveness: " << m_aggressiveness << std::endl;
    std::cout << "Risk Tolerance: " << m_riskTolerance << std::endl;
    std::cout << "Decisions Executed: " << m_decisionsExecuted << std::endl;
    std::cout << "Pending Decisions: " << GetPendingDecisions() << std::endl;
    
    std::cout << "\nThreat Assessment:" << std::endl;
    for (const auto& [realmId, threat] : m_threatAssessment) {
        std::cout << "  Realm " << realmId << ": " << static_cast<int>(threat) << std::endl;
    }
    
    std::cout << "\nRelationship Scores:" << std::endl;
    for (const auto& [realmId, score] : m_relationshipScores) {
        std::cout << "  Realm " << realmId << ": " << score << std::endl;
    }
    
    std::cout << "\nRecent Events: " << m_recentEvents.size() << std::endl;
    std::cout << "========================\n" << std::endl;
}

Json::Value NationAI::GetStatistics() const {
    Json::Value stats;
    
    stats["actor_id"] = m_actorId;
    stats["realm_id"] = static_cast<Json::UInt64>(m_realmId);
    stats["name"] = m_name;
    stats["personality"] = static_cast<int>(m_personality);
    stats["primary_goal"] = static_cast<int>(m_primaryGoal);
    stats["secondary_goal"] = static_cast<int>(m_secondaryGoal);
    stats["aggressiveness"] = m_aggressiveness;
    stats["risk_tolerance"] = m_riskTolerance;
    stats["decisions_executed"] = static_cast<Json::UInt64>(m_decisionsExecuted);
    stats["pending_decisions"] = static_cast<Json::UInt>(GetPendingDecisions());
    stats["is_active"] = IsActive();
    
    // Queue sizes
    stats["war_decisions_queued"] = static_cast<Json::UInt>(m_warDecisions.size());
    stats["military_decisions_queued"] = static_cast<Json::UInt>(m_militaryDecisions.size());
    stats["diplomatic_decisions_queued"] = static_cast<Json::UInt>(m_diplomaticDecisions.size());
    stats["economic_decisions_queued"] = static_cast<Json::UInt>(m_economicDecisions.size());
    
    // Event memory
    stats["recent_events_count"] = static_cast<Json::UInt>(m_recentEvents.size());
    
    // Threat assessment summary
    stats["threats_tracked"] = static_cast<Json::UInt>(m_threatAssessment.size());
    stats["relationships_tracked"] = static_cast<Json::UInt>(m_relationshipScores.size());
    
    return stats;
}

} // namespace ai
} // namespace game
