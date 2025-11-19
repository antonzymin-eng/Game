// Created: October 17, 2025, 2:15 PM
// Location: src/game/ai/CharacterAI.cpp
// Part 1 of 2 - Lines 1-500

#include "game/ai/CharacterAI.h"
#include "game/ai/CouncilAI.h"
#include "game/ai/InformationPropagationSystem.h"
#include "game/ai/AIDirector.h"
#include "game/ai/NationAI.h"
#include "game/ai/AIAttentionManager.h"
#include "game/ai/CharacterAIConstants.h"
#include "game/components/CharacterComponent.h"
#include "game/components/NobleArtsComponent.h"
#include "core/ECS/ComponentAccessManager.h"
#include "utils/Random.h"
#include <algorithm>
#include <iostream>
#include <cmath>
#include "core/logging/Logger.h"

using namespace AI::CharacterAIConstants;

namespace AI {

// ============================================================================
// CharacterAI Implementation
// ============================================================================

CharacterAI::CharacterAI(
    uint32_t actorId,
    ::game::types::EntityID characterId,
    const std::string& name,
    CharacterArchetype archetype)
    : m_actorId(actorId)
    , m_characterId(characterId)
    , m_name(name)
    , m_archetype(archetype)
    , m_componentAccess(nullptr) {
    
    // Set personality traits based on archetype
    switch (archetype) {
        case AI::CharacterArchetype::THE_CONQUEROR:
        case AI::CharacterArchetype::WARRIOR_KING:
            m_ambition = 0.9f;
            m_loyalty = 0.4f;
            m_honor = 0.6f;
            m_greed = 0.5f;
            m_boldness = 0.9f;
            m_compassion = 0.3f;
            m_primaryAmbition = CharacterAmbition::GAIN_LAND;
            m_secondaryAmbition = CharacterAmbition::INCREASE_PRESTIGE;
            break;
            
        case AI::CharacterArchetype::THE_DIPLOMAT:
            m_ambition = 0.6f;
            m_loyalty = 0.7f;
            m_honor = 0.7f;
            m_greed = 0.3f;
            m_boldness = 0.4f;
            m_compassion = 0.6f;
            m_primaryAmbition = CharacterAmbition::POWER;
            m_secondaryAmbition = CharacterAmbition::GAIN_TITLE;
            break;
            
        case AI::CharacterArchetype::THE_MERCHANT:
            m_ambition = 0.7f;
            m_loyalty = 0.5f;
            m_honor = 0.5f;
            m_greed = 0.9f;
            m_boldness = 0.6f;
            m_compassion = 0.4f;
            m_primaryAmbition = CharacterAmbition::ACCUMULATE_WEALTH;
            m_secondaryAmbition = CharacterAmbition::POWER;
            break;
            
        case AI::CharacterArchetype::THE_SCHOLAR:
            m_ambition = 0.5f;
            m_loyalty = 0.6f;
            m_honor = 0.8f;
            m_greed = 0.2f;
            m_boldness = 0.3f;
            m_compassion = 0.7f;
            m_primaryAmbition = CharacterAmbition::KNOWLEDGE;
            m_secondaryAmbition = CharacterAmbition::LEGACY;
            break;
            
        case AI::CharacterArchetype::THE_ZEALOT:
            m_ambition = 0.8f;
            m_loyalty = 0.9f;
            m_honor = 0.9f;
            m_greed = 0.1f;
            m_boldness = 0.8f;
            m_compassion = 0.3f;
            m_primaryAmbition = CharacterAmbition::PIETY;
            m_secondaryAmbition = CharacterAmbition::POWER;
            break;
            
        case AI::CharacterArchetype::THE_BUILDER:
        case AI::CharacterArchetype::THE_ADMINISTRATOR:
            m_ambition = 0.6f;
            m_loyalty = 0.8f;
            m_honor = 0.7f;
            m_greed = 0.4f;
            m_boldness = 0.5f;
            m_compassion = 0.6f;
            m_primaryAmbition = CharacterAmbition::LEGACY;
            m_secondaryAmbition = CharacterAmbition::GAIN_TITLE;
            break;
            
        case AI::CharacterArchetype::THE_TYRANT:
            m_ambition = 0.9f;
            m_loyalty = 0.2f;
            m_honor = 0.2f;
            m_greed = 0.8f;
            m_boldness = 0.9f;
            m_compassion = 0.1f;
            m_primaryAmbition = CharacterAmbition::POWER;
            m_secondaryAmbition = CharacterAmbition::REVENGE;
            break;
            
        case AI::CharacterArchetype::THE_REFORMER:
            m_ambition = 0.7f;
            m_loyalty = 0.6f;
            m_honor = 0.8f;
            m_greed = 0.3f;
            m_boldness = 0.7f;
            m_compassion = 0.8f;
            m_primaryAmbition = CharacterAmbition::LEGACY;
            m_secondaryAmbition = CharacterAmbition::POWER;
            break;
            
        default:
            m_ambition = 0.5f;
            m_loyalty = 0.5f;
            m_honor = 0.5f;
            m_greed = 0.5f;
            m_boldness = 0.5f;
            m_compassion = 0.5f;
            m_primaryAmbition = CharacterAmbition::NONE;
            m_secondaryAmbition = CharacterAmbition::NONE;
            break;
    }
    
    m_currentMood = CharacterMood::CONTENT;
    m_lastActivityTime = std::chrono::system_clock::now();
    m_lastAmbitionReview = m_lastActivityTime;
}

// ============================================================================
// Core AI Processing
// ============================================================================

void CharacterAI::ProcessInformation(const AI::InformationPacket& packet) {
    std::lock_guard<std::mutex> lock(m_stateMutex);

    RememberInteraction(packet.originatorEntityId,
                       packet.eventDescription,
                       packet.severity);

    // React based on information type
    switch (packet.type) {
        case AI::InformationType::SUCCESSION_CRISIS:
            // Opportunity for ambitious characters
            if (m_ambition > 0.6f && packet.originatorEntityId != m_characterId) {
                auto plot = EvaluatePlot(packet.originatorEntityId);
                if (plot.shouldExecute && plot.successChance > 0.4f) {
                    StartPlot(plot);
                }
            }
            break;
            
        case AI::InformationType::DIPLOMATIC_CHANGE:
            // Relationship updates handled by AIDirector's UpdateCharacterBackground
            // (removed direct call to avoid deadlock)
            break;
            
        case AI::InformationType::MILITARY_ACTION:
            // Assess if this affects our interests
            if (packet.severity > 0.7f) {
                UpdateMood();
                if (m_currentMood == CharacterMood::AFRAID) {
                    // Consider fleeing or defensive plots
                    auto proposal = EvaluateProposal();
                    if (proposal.acceptanceChance > 0.5f) {
                        m_proposalDecisions.push(proposal);
                    }
                }
            }
            break;
            
        case AI::InformationType::ECONOMIC_CRISIS:
            // Opportunity for merchants, concern for others
            if (m_archetype == CharacterArchetype::THE_MERCHANT) {
                if (packet.severity > 0.5f) {
                    auto personal = EvaluatePersonalAction();
                    if (personal.expectedBenefit > personal.cost) {
                        m_personalDecisions.push(personal);
                    }
                }
            }
            break;
            
        case AI::InformationType::REBELLION:
            // Major concern - affects loyalty and ambition
            if (m_loyalty < 0.5f) {
                // Low loyalty - might join rebellion
                auto plot = EvaluatePlot(0);
                if (plot.type == PlotDecision::COUP) {
                    StartPlot(plot);
                }
            } else {
                // High loyalty - support ruler
                UpdateOpinion(packet.originatorEntityId, -20.0f, "Rebellion");
            }
            break;
            
        default:
            break;
    }
    
    // Update mood based on events
    UpdateMood();

    // Ambition reviews handled by AIDirector's UpdateCharacterBackground
    // (removed direct call to avoid deadlock)
}

void CharacterAI::UpdateAmbitions() {
    std::lock_guard<std::mutex> lock(m_stateMutex);

    // Check if current ambition is achieved
    if (IsAmbitionAchieved()) {
        // Choose new ambition
        CharacterAmbition newAmbition = ChooseNewAmbition();
        if (newAmbition != CharacterAmbition::NONE) {
            m_secondaryAmbition = m_primaryAmbition;
            m_primaryAmbition = newAmbition;
        }
    }
    
    // Pursue current ambition
    PursueAmbition();
}

void CharacterAI::UpdateRelationships() {
    std::lock_guard<std::mutex> lock(m_stateMutex);

    // Decay old relationships
    for (auto& [characterId, opinion] : m_relationships) {
        if (opinion > 0.0f) {
            opinion = std::max(Relationship::MIN_OPINION,
                             opinion + Relationship::POSITIVE_DECAY_RATE);
        } else if (opinion < 0.0f) {
            opinion = std::min(Relationship::MAX_OPINION,
                             opinion + Relationship::NEGATIVE_RECOVERY_RATE);
        }
    }

    // Forget old memories
    ForgetOldMemories();

    // Evaluate new relationships
    for (const auto& [characterId, opinion] : m_relationships) {
        if (opinion < Relationship::RIVAL_THRESHOLD && !IsRival(characterId)) {
            m_rival = characterId;
            m_relationshipTypes[characterId] = "rival";
        } else if (opinion > Relationship::FRIEND_THRESHOLD && !IsFriend(characterId)) {
            m_relationshipTypes[characterId] = "friend";
        }
    }
}

void CharacterAI::ExecuteDecisions() {
    std::lock_guard<std::mutex> lock(m_stateMutex);

    // Execute plot decisions (highest priority)
    if (!m_plotDecisions.empty()) {
        auto plot = m_plotDecisions.front();
        m_plotDecisions.pop();
        ExecutePlot(plot);
    }
    
    // Execute relationship decisions
    if (!m_relationshipDecisions.empty()) {
        auto decision = m_relationshipDecisions.front();
        m_relationshipDecisions.pop();
        ExecuteRelationshipAction(decision);
    }
    
    // Execute proposals
    if (!m_proposalDecisions.empty()) {
        auto proposal = m_proposalDecisions.front();
        m_proposalDecisions.pop();
        ExecuteProposal(proposal);
    }
    
    // Execute personal actions
    if (!m_personalDecisions.empty()) {
        auto action = m_personalDecisions.front();
        m_personalDecisions.pop();
        ExecutePersonalAction(action);
    }
}

// ============================================================================
// Decision Evaluation
// ============================================================================

PlotDecision CharacterAI::EvaluatePlot(::game::types::EntityID target) {
    PlotDecision plot;
    plot.targetCharacter = target;
    
    // Determine plot type based on personality and situation
    if (m_ambition > Personality::HIGH_AMBITION && m_honor < Personality::LOW_HONOR) {
        // Ambitious and dishonorable - consider assassination
        plot.type = PlotDecision::ASSASSINATION;
        plot.riskLevel = Plot::ASSASSINATION_RISK;
        plot.successChance = Plot::ASSASSINATION_BASE_SUCCESS * (1.0f - m_honor) * m_boldness;
    } else if (m_ambition > Personality::HIGH_BOLDNESS && m_loyalty < Personality::LOW_LOYALTY) {
        // Ambitious and disloyal - consider coup
        plot.type = PlotDecision::COUP;
        plot.riskLevel = Plot::COUP_RISK;
        plot.successChance = Plot::COUP_BASE_SUCCESS * m_boldness * (1.0f - m_loyalty);
    } else if (m_greed > Personality::HIGH_GREED) {
        // Greedy - consider blackmail or theft
        plot.type = PlotDecision::BLACKMAIL;
        plot.riskLevel = Plot::BLACKMAIL_RISK;
        plot.successChance = Plot::BLACKMAIL_BASE_SUCCESS * m_boldness;
    } else {
        // Default - fabricate claim
        plot.type = PlotDecision::FABRICATE_CLAIM;
        plot.riskLevel = Plot::FABRICATE_CLAIM_RISK;
        plot.successChance = Plot::FABRICATE_CLAIM_SUCCESS;
    }
    
    // Calculate desirability
    float desirability = CalculatePlotDesirability(plot);
    plot.shouldExecute = (desirability > Plot::MIN_DESIRABILITY_THRESHOLD &&
                         plot.successChance > Plot::MIN_SUCCESS_THRESHOLD &&
                         m_boldness > plot.riskLevel * Plot::RISK_MULTIPLIER);
    
    return plot;
}

ProposalDecision CharacterAI::EvaluateProposal() {
    ProposalDecision proposal;
    
    // Choose proposal type based on ambition
    switch (m_primaryAmbition) {
        case CharacterAmbition::GAIN_TITLE:
            proposal.type = ProposalDecision::REQUEST_TITLE;
            proposal.acceptanceChance = Proposal::TITLE_BASE_ACCEPTANCE +
                                       (m_loyalty * Proposal::TITLE_LOYALTY_MODIFIER);
            break;

        case CharacterAmbition::ACCUMULATE_WEALTH:
            proposal.type = ProposalDecision::REQUEST_GOLD;
            proposal.acceptanceChance = Proposal::GOLD_BASE_ACCEPTANCE +
                                       (m_loyalty * Proposal::GOLD_LOYALTY_MODIFIER);
            break;

        case CharacterAmbition::FIND_LOVE:
            proposal.type = ProposalDecision::REQUEST_MARRIAGE;
            proposal.acceptanceChance = Proposal::MARRIAGE_BASE_ACCEPTANCE;
            break;

        case CharacterAmbition::POWER:
            proposal.type = ProposalDecision::REQUEST_COUNCIL_POSITION;
            proposal.acceptanceChance = Proposal::COUNCIL_BASE_ACCEPTANCE +
                                       (m_loyalty * Proposal::COUNCIL_LOYALTY_MODIFIER);
            break;

        default:
            proposal.type = ProposalDecision::SUGGEST_WAR;
            proposal.acceptanceChance = Proposal::WAR_BASE_ACCEPTANCE;
            break;
    }

    // Adjust acceptance chance based on personality
    proposal.acceptanceChance *= (1.0f + m_compassion * Proposal::COMPASSION_MODIFIER);
    proposal.acceptanceChance = std::clamp(proposal.acceptanceChance, 0.0f, 1.0f);
    
    return proposal;
}

RelationshipDecision CharacterAI::EvaluateRelationship(::game::types::EntityID target) {
    RelationshipDecision decision;
    decision.targetCharacter = target;
    
    float opinion = GetOpinion(target);

    // Choose action based on opinion and personality
    if (opinion > Relationship::FRIEND_THRESHOLD) {
        if (m_primaryAmbition == CharacterAmbition::FIND_LOVE && !m_lover) {
            decision.action = RelationshipDecision::SEDUCE;
            decision.desirability = RelationshipDesire::SEDUCTION_DESIRABILITY;
        } else {
            decision.action = RelationshipDecision::BEFRIEND;
            decision.desirability = RelationshipDesire::BEFRIEND_DESIRABILITY;
        }
    } else if (opinion < Relationship::RIVAL_THRESHOLD) {
        if (m_boldness > Personality::HIGH_BOLDNESS && m_honor < Personality::LOW_HONOR) {
            decision.action = RelationshipDecision::BLACKMAIL;
            decision.desirability = RelationshipDesire::BLACKMAIL_DESIRABILITY;
        } else {
            decision.action = RelationshipDecision::RIVAL;
            decision.desirability = RelationshipDesire::RIVAL_DESIRABILITY;
        }
    } else if (opinion > Relationship::NEUTRAL_LOWER && opinion < Relationship::FRIEND_THRESHOLD) {
        decision.action = RelationshipDecision::MENTOR;
        decision.desirability = RelationshipDesire::MENTOR_DESIRABILITY;
    } else {
        decision.action = RelationshipDecision::BEFRIEND;
        decision.desirability = RelationshipDesire::NEUTRAL_DESIRABILITY;
    }
    
    decision.desirability *= CalculateRelationshipValue(target);
    
    return decision;
}

PersonalDecision CharacterAI::EvaluatePersonalAction() {
    PersonalDecision action;
    
    // Choose action based on archetype and current situation
    switch (m_archetype) {
        case AI::CharacterArchetype::THE_SCHOLAR:
            action.action = PersonalDecision::IMPROVE_SKILL;
            action.expectedBenefit = 0.8f;
            action.cost = 50.0f;
            break;
            
        case AI::CharacterArchetype::THE_MERCHANT:
            action.action = PersonalDecision::MANAGE_ESTATE;
            action.expectedBenefit = 0.7f;
            action.cost = 100.0f;
            break;
            
        case AI::CharacterArchetype::THE_ZEALOT:
            action.action = PersonalDecision::GO_ON_PILGRIMAGE;
            action.expectedBenefit = 0.6f;
            action.cost = 200.0f;
            break;
            
        case AI::CharacterArchetype::THE_BUILDER:
            action.action = PersonalDecision::COMMISSION_ARTIFACT;
            action.expectedBenefit = 0.5f;
            action.cost = 500.0f;
            break;
            
        default:
            action.action = PersonalDecision::HOST_FEAST;
            action.expectedBenefit = 0.4f;
            action.cost = 150.0f;
            break;
    }
    
    return action;
}

// ============================================================================
// Ambition System
// ============================================================================

void CharacterAI::SetPrimaryAmbition(CharacterAmbition ambition) {
    m_primaryAmbition = ambition;
}

void CharacterAI::PursueAmbition() {
    switch (m_primaryAmbition) {
        case CharacterAmbition::GAIN_TITLE:
            // Request title from liege
            {
                auto proposal = EvaluateProposal();
                if (proposal.type == ProposalDecision::REQUEST_TITLE) {
                    m_proposalDecisions.push(proposal);
                }
            }
            break;
            
        case CharacterAmbition::ACCUMULATE_WEALTH:
            // Focus on economic actions
            {
                auto action = EvaluatePersonalAction();
                if (action.action == PersonalDecision::MANAGE_ESTATE) {
                    m_personalDecisions.push(action);
                }
            }
            break;
            
        case CharacterAmbition::GAIN_LAND:
            // Consider plots or proposals for land
            if (m_boldness > 0.6f) {
                auto plot = EvaluatePlot(0);
                if (plot.type == PlotDecision::FABRICATE_CLAIM) {
                    m_plotDecisions.push(plot);
                }
            }
            break;
            
        case CharacterAmbition::POWER:
            // Seek council position or political influence
            {
                auto proposal = EvaluateProposal();
                if (proposal.type == ProposalDecision::REQUEST_COUNCIL_POSITION) {
                    m_proposalDecisions.push(proposal);
                }
            }
            break;
            
        case CharacterAmbition::KNOWLEDGE:
            // Improve skills
            {
                PersonalDecision action;
                action.action = PersonalDecision::IMPROVE_SKILL;
                action.expectedBenefit = 1.0f;
                action.cost = 50.0f;
                m_personalDecisions.push(action);
            }
            break;
            
        case CharacterAmbition::REVENGE:
            // Plot against rival
            if (m_rival != 0) {
                auto plot = EvaluatePlot(m_rival);
                if (plot.shouldExecute) {
                    m_plotDecisions.push(plot);
                }
            }
            break;
            
        default:
            break;
    }
}

bool CharacterAI::IsAmbitionAchieved() const {
    // Stub implementation - would check actual game state
    // For now, use time-based achievement simulation
    auto now = std::chrono::system_clock::now();
    auto timeSinceReview = std::chrono::duration_cast<std::chrono::hours>(
        now - m_lastAmbitionReview).count();
    
    // Ambitions take time to achieve
    return timeSinceReview > 720; // 30 days
}

CharacterAmbition CharacterAI::ChooseNewAmbition() const {
    // Choose based on personality traits
    if (m_greed > 0.7f) {
        return CharacterAmbition::ACCUMULATE_WEALTH;
    } else if (m_ambition > 0.8f) {
        return CharacterAmbition::GAIN_TITLE;
    } else if (m_boldness > 0.7f) {
        return CharacterAmbition::GAIN_LAND;
    } else if (m_compassion > 0.7f) {
        return CharacterAmbition::FIND_LOVE;
    } else {
        return CharacterAmbition::LEGACY;
    }
}

// ============================================================================
// Relationship Management
// ============================================================================

void CharacterAI::UpdateOpinion(::game::types::EntityID character, float change, 
                                const std::string& reason) {
    m_relationships[character] += change;
    m_relationships[character] = std::clamp(m_relationships[character], -100.0f, 100.0f);
    
    RememberInteraction(character, reason, std::abs(change) / 100.0f);
}

float CharacterAI::GetOpinion(::game::types::EntityID character) const {
    auto it = m_relationships.find(character);
    if (it != m_relationships.end()) {
        return it->second;
    }
    return 0.0f;
}

bool CharacterAI::IsRival(::game::types::EntityID character) const {
    return m_rival == character;
}

bool CharacterAI::IsFriend(::game::types::EntityID character) const {
    auto it = m_relationshipTypes.find(character);
    if (it != m_relationshipTypes.end()) {
        return it->second == "friend";
    }
    return false;
}

// ============================================================================
// Plot Management
// ============================================================================

void CharacterAI::StartPlot(const PlotDecision& plot) {
    m_activePlots.push_back(plot);
    m_schemesExecuted++;
    
    CORE_STREAM_INFO("CharacterAI") << "" << m_name << " starting plot: " 
              << static_cast<int>(plot.type) << " against " 
              << plot.targetCharacter;
}

void CharacterAI::JoinPlot(::game::types::EntityID plotLeader) {
    CORE_STREAM_INFO("CharacterAI") << "" << m_name << " joining plot led by " 
              << plotLeader;
}

void CharacterAI::AbandonPlot(size_t plotIndex) {
    if (plotIndex < m_activePlots.size()) {
        m_activePlots.erase(m_activePlots.begin() + plotIndex);
    }
}

bool CharacterAI::IsPlottingAgainst(::game::types::EntityID character) const {
    return std::any_of(m_activePlots.begin(), m_activePlots.end(),
        [character](const PlotDecision& plot) {
            return plot.targetCharacter == character;
        });
}

// CharacterAI.cpp - Continuation from line 501
// Location: src/game/ai/character/CharacterAI.cpp

// ============================================================================
// Mood and Stress System
// ============================================================================

void CharacterAI::UpdateMood() {
    m_currentMood = CalculateMood();
}

void CharacterAI::AddStress(float amount) {
    // Stress affects mood negatively
    if (m_currentMood == CharacterMood::CONTENT) {
        m_currentMood = CharacterMood::STRESSED;
    } else if (m_currentMood == CharacterMood::HAPPY) {
        m_currentMood = CharacterMood::CONTENT;
    }
}

void CharacterAI::ReduceStress(float amount) {
    // Stress relief improves mood
    if (m_currentMood == CharacterMood::STRESSED) {
        m_currentMood = CharacterMood::CONTENT;
    } else if (m_currentMood == CharacterMood::CONTENT) {
        m_currentMood = CharacterMood::HAPPY;
    }
}

CharacterMood CharacterAI::CalculateMood() const {
    // Calculate mood based on relationships, ambitions, and memories
    float moodScore = 0.0f;
    
    // Relationships affect mood
    float totalOpinion = 0.0f;
    int relationshipCount = 0;
    for (const auto& [characterId, opinion] : m_relationships) {
        totalOpinion += opinion;
        relationshipCount++;
    }
    if (relationshipCount > 0) {
        moodScore += (totalOpinion / relationshipCount) * 0.01f;
    }
    
    // Ambition progress affects mood
    if (IsAmbitionAchieved()) {
        moodScore += 0.5f;
    } else {
        moodScore -= 0.2f * m_ambition;
    }
    
    // Recent negative events affect mood
    int negativeEvents = 0;
    for (const auto& memory : m_memories) {
        if (memory.impact < 0.0f) {
            negativeEvents++;
        }
    }
    moodScore -= negativeEvents * 0.1f;
    
    // Active plots increase stress
    moodScore -= m_activePlots.size() * 0.15f;
    
    // Rivals cause stress
    if (m_rival != 0) {
        moodScore -= 0.3f;
    }
    
    // Determine mood from score
    if (moodScore > Mood::HAPPY_THRESHOLD) return CharacterMood::HAPPY;
    if (moodScore > Mood::CONTENT_THRESHOLD) return CharacterMood::CONTENT;
    if (moodScore > Mood::STRESSED_THRESHOLD) return CharacterMood::STRESSED;
    if (moodScore > Mood::ANGRY_THRESHOLD) return CharacterMood::ANGRY;
    return CharacterMood::AFRAID;
}

// ============================================================================
// Internal Helper Methods
// ============================================================================

void CharacterAI::RememberInteraction(::game::types::EntityID character, 
                                     const std::string& event, 
                                     float impact) {
    CharacterMemory memory;
    memory.character = character;
    memory.event = event;
    memory.impact = impact;
    memory.when = std::chrono::system_clock::now();
    
    m_memories.push_back(memory);
    
    // Prune old memories
    if (m_memories.size() > MAX_MEMORIES) {
        ForgetOldMemories();
    }
}

void CharacterAI::ForgetOldMemories() {
    if (m_memories.size() <= MAX_MEMORIES) return;
    
    auto now = std::chrono::system_clock::now();
    
    // Remove memories older than 1 year
    m_memories.erase(
        std::remove_if(m_memories.begin(), m_memories.end(),
            [&now](const CharacterMemory& memory) {
                auto age = std::chrono::duration_cast<std::chrono::hours>(
                    now - memory.when).count();
                return age > 8760; // 365 days
            }),
        m_memories.end()
    );
    
    // If still too many, remove least impactful
    if (m_memories.size() > MAX_MEMORIES) {
        std::sort(m_memories.begin(), m_memories.end(),
            [](const CharacterMemory& a, const CharacterMemory& b) {
                return std::abs(a.impact) < std::abs(b.impact);
            });
        
        m_memories.erase(m_memories.begin(), 
                        m_memories.begin() + (m_memories.size() - MAX_MEMORIES));
    }
}

// ============================================================================
// Component Access Helpers
// ============================================================================

const ::game::character::CharacterComponent* CharacterAI::GetCharacterComponent() {
    if (!m_componentAccess) return nullptr;

    try {
        auto result = m_componentAccess->GetComponent<::game::character::CharacterComponent>(
            m_characterId);
        return result.Get();
    } catch (const std::exception& e) {
        // Character component not found or error accessing it
        return nullptr;
    }
}

const ::game::character::NobleArtsComponent* CharacterAI::GetNobleArtsComponent() {
    if (!m_componentAccess) return nullptr;

    try {
        auto result = m_componentAccess->GetComponent<::game::character::NobleArtsComponent>(
            m_characterId);
        return result.Get();
    } catch (const std::exception& e) {
        // NobleArts component not found or error accessing it
        return nullptr;
    }
}

// ============================================================================
// Personality-based Decision Calculations
// ============================================================================

float CharacterAI::CalculatePlotDesirability(const PlotDecision& plot) const {
    float desirability = 0.0f;
    
    // Base desirability on ambition
    desirability += m_ambition * DecisionModifiers::AMBITION_BASE_WEIGHT;

    // Adjust for honor (dishonorable plots less appealing to honorable characters)
    switch (plot.type) {
        case PlotDecision::ASSASSINATION:
        case PlotDecision::BLACKMAIL:
            desirability -= m_honor * DecisionModifiers::HONOR_PENALTY_ASSASSINATION;
            break;
        case PlotDecision::COUP:
            desirability -= m_loyalty * DecisionModifiers::LOYALTY_PENALTY_COUP;
            break;
        default:
            break;
    }

    // Adjust for boldness
    desirability += m_boldness * plot.riskLevel * DecisionModifiers::BOLDNESS_RISK_WEIGHT;

    // Adjust for success chance
    desirability += plot.successChance * DecisionModifiers::SUCCESS_WEIGHT;
    
    // Mood affects decision-making
    switch (m_currentMood) {
        case CharacterMood::DESPERATE:
            desirability += DecisionModifiers::DESPERATE_BONUS;
            break;
        case CharacterMood::ANGRY:
            desirability += DecisionModifiers::ANGRY_BONUS;
            break;
        case CharacterMood::AMBITIOUS:
            desirability += DecisionModifiers::AMBITIOUS_BONUS;
            break;
        case CharacterMood::AFRAID:
            desirability -= DecisionModifiers::AFRAID_PENALTY;
            break;
        default:
            break;
    }
    
    return std::clamp(desirability, 0.0f, 1.0f);
}

float CharacterAI::CalculateProposalSuccess(const ProposalDecision& proposal) const {
    float success = proposal.acceptanceChance;
    
    // Adjust for loyalty
    if (proposal.type == ProposalDecision::REQUEST_TITLE ||
        proposal.type == ProposalDecision::REQUEST_COUNCIL_POSITION) {
        success += m_loyalty * 0.3f;
    }
    
    // Adjust for compassion
    if (proposal.type == ProposalDecision::REQUEST_GOLD) {
        success += m_compassion * 0.2f;
    }
    
    // Honor affects all proposals
    success += m_honor * 0.15f;
    
    return std::clamp(success, 0.0f, 1.0f);
}

float CharacterAI::CalculateRelationshipValue(::game::types::EntityID character) const {
    float value = 0.5f; // Base value
    
    // Existing opinion matters
    float opinion = GetOpinion(character);
    value += opinion * 0.005f;
    
    // Personality affects value
    value += m_compassion * 0.2f;
    
    // Rivals have negative value
    if (IsRival(character)) {
        value -= 0.5f;
    }
    
    // Friends have positive value
    if (IsFriend(character)) {
        value += 0.3f;
    }
    
    return std::clamp(value, 0.0f, 1.0f);
}

// ============================================================================
// Decision Execution
// ============================================================================

void CharacterAI::ExecutePlot(const PlotDecision& plot) {
    CORE_STREAM_INFO("CharacterAI") << "" << m_name << " executing plot type " 
              << static_cast<int>(plot.type) << " against " 
              << plot.targetCharacter;
    
    // Would post PlotEvent to message bus
    // This integrates with character/realm systems
    
    // Update relationships
    if (plot.targetCharacter != 0) {
        UpdateOpinion(plot.targetCharacter, -30.0f, "Plotting");
    }
    
    // Add to active plots
    m_activePlots.push_back(plot);
}

void CharacterAI::ExecuteProposal(const ProposalDecision& proposal) {
    CORE_STREAM_INFO("CharacterAI") << "" << m_name << " making proposal type " 
              << static_cast<int>(proposal.type) << " to " 
              << proposal.targetRuler;
    
    // Calculate actual success
    float actualSuccess = CalculateProposalSuccess(proposal);

    // Random roll for success using modern random
    float roll = utils::RandomFloat();

    if (roll < actualSuccess) {
        CORE_STREAM_INFO("CharacterAI") << "Proposal SUCCEEDED";
        
        // Update mood positively
        if (m_currentMood == CharacterMood::STRESSED) {
            m_currentMood = CharacterMood::CONTENT;
        }
        
        // Update relationship with ruler
        UpdateOpinion(proposal.targetRuler, 10.0f, "Proposal granted");
    } else {
        CORE_STREAM_INFO("CharacterAI") << "Proposal FAILED";
        
        // Update mood negatively
        AddStress(0.2f);
        
        // Slight negative opinion
        UpdateOpinion(proposal.targetRuler, -5.0f, "Proposal denied");
    }
}

void CharacterAI::ExecuteRelationshipAction(const RelationshipDecision& decision) {
    CORE_STREAM_INFO("CharacterAI") << "" << m_name << " performing relationship action " 
              << static_cast<int>(decision.action) << " with " 
              << decision.targetCharacter;
    
    switch (decision.action) {
        case RelationshipDecision::BEFRIEND:
            UpdateOpinion(decision.targetCharacter, 15.0f, "Befriending");
            break;
            
        case RelationshipDecision::SEDUCE:
            if (decision.desirability > 0.7f) {
                m_lover = decision.targetCharacter;
                UpdateOpinion(decision.targetCharacter, 30.0f, "Romance");
            }
            break;
            
        case RelationshipDecision::RIVAL:
            m_rival = decision.targetCharacter;
            UpdateOpinion(decision.targetCharacter, -40.0f, "Declared rival");
            m_primaryAmbition = CharacterAmbition::REVENGE;
            break;
            
        case RelationshipDecision::MENTOR:
            UpdateOpinion(decision.targetCharacter, 20.0f, "Mentoring");
            m_mentor = decision.targetCharacter;
            break;
            
        case RelationshipDecision::BLACKMAIL:
            UpdateOpinion(decision.targetCharacter, -50.0f, "Blackmail");
            // Would create plot
            break;
            
        case RelationshipDecision::MARRY:
            m_lover = decision.targetCharacter;
            UpdateOpinion(decision.targetCharacter, 50.0f, "Marriage");
            break;
            
        case RelationshipDecision::DIVORCE:
            if (m_lover == decision.targetCharacter) {
                m_lover = 0;
                UpdateOpinion(decision.targetCharacter, -30.0f, "Divorce");
            }
            break;
    }
}

void CharacterAI::ExecutePersonalAction(const PersonalDecision& decision) {
    CORE_STREAM_INFO("CharacterAI") << "" << m_name << " performing personal action " 
              << static_cast<int>(decision.action);
    
    switch (decision.action) {
        case PersonalDecision::IMPROVE_SKILL:
            // Would increase character skills
            ReduceStress(0.1f);
            break;
            
        case PersonalDecision::CHANGE_LIFESTYLE:
            // Would modify character lifestyle
            m_currentMood = CharacterMood::CONTENT;
            break;
            
        case PersonalDecision::MANAGE_ESTATE:
            // Would increase wealth
            ReduceStress(0.05f);
            break;
            
        case PersonalDecision::HOST_FEAST:
            // Would improve relationships
            ReduceStress(0.3f);
            for (auto& [characterId, opinion] : m_relationships) {
                UpdateOpinion(characterId, 5.0f, "Feast invitation");
            }
            break;
            
        case PersonalDecision::GO_ON_PILGRIMAGE:
            // Would increase piety
            ReduceStress(0.4f);
            m_currentMood = CharacterMood::CONTENT;
            break;
            
        case PersonalDecision::COMMISSION_ARTIFACT:
            // Would create artifact
            UpdateOpinion(0, 10.0f, "Commissioned artifact");
            break;
    }
}

// ============================================================================
// Personality Modifiers
// ============================================================================

float CharacterAI::GetAmbitionModifier() const {
    float modifier = m_ambition;
    
    // Mood affects ambition
    switch (m_currentMood) {
        case CharacterMood::AMBITIOUS:
            modifier *= 1.3f;
            break;
        case CharacterMood::AFRAID:
            modifier *= 0.6f;
            break;
        case CharacterMood::DESPERATE:
            modifier *= 1.5f;
            break;
        default:
            break;
    }
    
    return std::clamp(modifier, 0.0f, 1.0f);
}

float CharacterAI::GetLoyaltyModifier() const {
    float modifier = m_loyalty;
    
    // Relationships with ruler affect loyalty
    // Would query ruler ID from character component
    
    // Mood affects loyalty
    switch (m_currentMood) {
        case CharacterMood::ANGRY:
            modifier *= 0.7f;
            break;
        case CharacterMood::DESPERATE:
            modifier *= 0.5f;
            break;
        case CharacterMood::CONTENT:
            modifier *= 1.1f;
            break;
        default:
            break;
    }
    
    return std::clamp(modifier, 0.0f, 1.0f);
}

float CharacterAI::GetBoldnessModifier() const {
    float modifier = m_boldness;
    
    // Mood affects boldness
    switch (m_currentMood) {
        case CharacterMood::AFRAID:
            modifier *= 0.5f;
            break;
        case CharacterMood::ANGRY:
            modifier *= 1.3f;
            break;
        case CharacterMood::DESPERATE:
            modifier *= 1.5f;
            break;
        default:
            break;
    }
    
    return std::clamp(modifier, 0.0f, 1.0f);
}

// ============================================================================
// CouncilAI Implementation
// ============================================================================

CouncilAI::CouncilAI(
    uint32_t actorId,
    ::game::types::EntityID realmId,
    const std::string& name)
    : m_actorId(actorId)
    , m_realmId(realmId)
    , m_name(name) {
}

void CouncilAI::ProcessInformation(const AI::InformationPacket& packet) {
    // Council responds to major realm events
    VoteRecord record;
    record.proposalType = packet.eventDescription;
    record.when = std::chrono::system_clock::now();
    
    switch (packet.type) {
        case AI::InformationType::MILITARY_ACTION:
            record.votedFor = ShouldApproveWar(packet.originatorEntityId);
            break;
            
        case AI::InformationType::DIPLOMATIC_CHANGE:
            record.votedFor = true; // Generally approve diplomacy
            break;
            
        case AI::InformationType::ECONOMIC_CRISIS:
            record.votedFor = ShouldApproveTaxIncrease(0.1f);
            break;
            
        default:
            record.votedFor = true;
            break;
    }
    
    m_votingHistory.push_back(record);
    
    // Limit voting history
    if (m_votingHistory.size() > 100) {
        m_votingHistory.erase(m_votingHistory.begin());
    }
}

bool CouncilAI::ShouldApproveWar(::game::types::EntityID target) const {
    // Conservative approach - only approve strong cases
    
    // Count recent war approvals
    int recentWars = 0;
    auto now = std::chrono::system_clock::now();
    
    for (const auto& record : m_votingHistory) {
        if (record.proposalType.find("war") != std::string::npos &&
            record.votedFor) {
            auto age = std::chrono::duration_cast<std::chrono::hours>(
                now - record.when).count();
            if (age < 720) { // Last 30 days
                recentWars++;
            }
        }
    }
    
    // Limit wars
    if (recentWars > 2) return false;
    
    // Would evaluate realm strength, treasury, etc.
    return true;
}

bool CouncilAI::ShouldApproveTaxIncrease(float newRate) const {
    // Check recent tax decisions
    int recentTaxIncreases = 0;
    auto now = std::chrono::system_clock::now();
    
    for (const auto& record : m_votingHistory) {
        if (record.proposalType.find("tax") != std::string::npos &&
            record.votedFor) {
            auto age = std::chrono::duration_cast<std::chrono::hours>(
                now - record.when).count();
            if (age < 720) {
                recentTaxIncreases++;
            }
        }
    }
    
    // Limit tax increases
    return recentTaxIncreases < 2;
}

bool CouncilAI::ShouldApproveAlliance(::game::types::EntityID ally) const {
    // Generally approve alliances
    return true;
}

bool CouncilAI::ShouldApproveSuccession(::game::types::EntityID heir) const {
    // Check legitimacy of heir
    // Would query character component
    return true;
}

std::vector<std::string> CouncilAI::GetEconomicAdvice() const {
    std::vector<std::string> advice;
    
    advice.push_back("Consider investing in infrastructure");
    advice.push_back("Monitor trade agreements");
    advice.push_back("Build treasury reserves");
    
    return advice;
}

std::vector<std::string> CouncilAI::GetMilitaryAdvice() const {
    std::vector<std::string> advice;
    
    advice.push_back("Maintain standing army");
    advice.push_back("Fortify border provinces");
    advice.push_back("Train levy forces");
    
    return advice;
}

std::vector<std::string> CouncilAI::GetDiplomaticAdvice() const {
    std::vector<std::string> advice;
    
    advice.push_back("Seek alliances with neighbors");
    advice.push_back("Improve relations with rivals");
    advice.push_back("Negotiate trade agreements");
    
    return advice;
}

// ============================================================================
// CharacterAI Factory Implementation
// ============================================================================

std::unique_ptr<CharacterAI> CharacterAIFactory::CreateAmbitiousNoble(
    uint32_t actorId,
    ::game::types::EntityID characterId,
    const std::string& name) {
    
    auto ai = std::make_unique<CharacterAI>(
        actorId, characterId, name, CharacterArchetype::THE_CONQUEROR);
    
    // Adjust personality for ambitious noble
    ai->m_ambition = 0.9f;
    ai->m_loyalty = 0.4f;
    ai->m_honor = 0.5f;
    ai->m_primaryAmbition = CharacterAmbition::GAIN_TITLE;
    
    return ai;
}

std::unique_ptr<CharacterAI> CharacterAIFactory::CreateLoyalVassal(
    uint32_t actorId,
    ::game::types::EntityID characterId,
    const std::string& name) {
    
    auto ai = std::make_unique<CharacterAI>(
        actorId, characterId, name, CharacterArchetype::THE_ADMINISTRATOR);
    
    // Adjust personality for loyal vassal
    ai->m_ambition = 0.4f;
    ai->m_loyalty = 0.9f;
    ai->m_honor = 0.8f;
    ai->m_primaryAmbition = CharacterAmbition::LEGACY;
    
    return ai;
}

std::unique_ptr<CharacterAI> CharacterAIFactory::CreateCunningSchemer(
    uint32_t actorId,
    ::game::types::EntityID characterId,
    const std::string& name) {
    
    auto ai = std::make_unique<CharacterAI>(
        actorId, characterId, name, CharacterArchetype::THE_TYRANT);
    
    // Adjust personality for schemer
    ai->m_ambition = 0.8f;
    ai->m_loyalty = 0.2f;
    ai->m_honor = 0.3f;
    ai->m_boldness = 0.8f;
    ai->m_primaryAmbition = CharacterAmbition::POWER;
    
    return ai;
}

std::unique_ptr<CharacterAI> CharacterAIFactory::CreatePiousPriest(
    uint32_t actorId,
    ::game::types::EntityID characterId,
    const std::string& name) {
    
    auto ai = std::make_unique<CharacterAI>(
        actorId, characterId, name, CharacterArchetype::THE_ZEALOT);
    
    // Adjust personality for pious priest
    ai->m_ambition = 0.5f;
    ai->m_loyalty = 0.8f;
    ai->m_honor = 0.9f;
    ai->m_greed = 0.1f;
    ai->m_compassion = 0.7f;
    ai->m_primaryAmbition = CharacterAmbition::PIETY;
    
    return ai;
}

} // namespace AI
