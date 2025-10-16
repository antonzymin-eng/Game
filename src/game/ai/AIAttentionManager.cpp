// Created: September 25, 2025, 11:15 AM
// Location: src/game/ai/AIAttentionManager.cpp
    }

std::vector<uint32_t> AIAttentionManager::GetInterestedActors(
#include "game/ai/AIDirector.h"
#include "game/ai/CharacterAI.h"
#include "core/ECS/ComponentAccessManager.h"
#include "core/ECS/EntityManager.h"
#include "core/types/game_types.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <string>
#include <cmath>

namespace AI {

// ============================================================================
// AIAttentionManager Implementation
// ============================================================================

AIAttentionManager::AIAttentionManager(
    std::shared_ptr<ECS::ComponentAccessManager> componentAccess)
    : m_componentAccess(componentAccess)
    , m_enableDetailedLogging(false)
    , m_globalAttentionMultiplier(1.0f) {
  }

AIAttentionManager::~AIAttentionManager() {
    Shutdown();
  }

void AIAttentionManager::Initialize() {
    std::cout << "[AIAttentionManager] Initializing attention system" << std::endl;
    
    // Initialize archetype templates
    InitializeArchetypeTemplates();
    
    // Reset statistics
    ResetStatistics();
    
    std::cout << "[AIAttentionManager] Initialized with " 
              << m_archetypeTemplates.size() << " archetype templates" << std::endl;
  }

void AIAttentionManager::Shutdown() {
    std::lock_guard<std::mutex> lock(m_actorMutex);
    
    m_nationActors.clear();
    m_characterActors.clear();
    m_archetypeTemplates.clear();
    m_personalityTemplates.clear();
    
    std::cout << "[AIAttentionManager] Shutdown complete" << std::endl;
}

// ============================================================================
// Actor Management
// ============================================================================

uint32_t AIAttentionManager::RegisterNationActor(
    uint32_t nationId, 
    const std::string& name,
    CharacterArchetype rulerArchetype) {
    
    std::lock_guard<std::mutex> lock(m_actorMutex);
    
    auto actor = std::make_unique<AIActor>(nationId, name, true);
    
    // Create attention profile based on ruler's archetype
    actor->attentionProfile = CreateProfileFromArchetype(rulerArchetype);
    actor->attentionProfile.nationPersonality = DerivePersonalityFromArchetype(rulerArchetype);
    
    m_nationActors[nationId] = std::move(actor);
    
    if (m_enableDetailedLogging) {
        std::cout << "[AIAttentionManager] Registered nation: " << name 
                  << " (ID: " << nationId << ") with " 
                  << AttentionUtils::ArchetypeToString(rulerArchetype) 
                  << " personality" << std::endl;
    }
    
    return nationId;
}

uint32_t AIAttentionManager::RegisterCharacterActor(
    uint32_t characterId,
    const std::string& name,
    CharacterArchetype archetype) {
    
    std::lock_guard<std::mutex> lock(m_actorMutex);
    
    auto actor = std::make_unique<AIActor>(characterId, name, false);
    actor->attentionProfile = CreateProfileFromArchetype(archetype);
    
    m_characterActors[characterId] = std::move(actor);
    if (m_enableDetailedLogging) {
        std::cout << "[AIAttentionManager] Registered character: " << name 
                  << " (ID: " << characterId << ") with " 
                  << AttentionUtils::ArchetypeToString(archetype) 
                  << " archetype" << std::endl;
    }
    
    return characterId;
}

void AIAttentionManager::UnregisterActor(uint32_t actorId, bool isNation) {
    std::lock_guard<std::mutex> lock(m_actorMutex);
    
    if (isNation) {
        m_nationActors.erase(actorId);
    } else {
        m_characterActors.erase(actorId);
    }
}

// ============================================================================
// Core Attention Filtering
// ============================================================================

AttentionResult AIAttentionManager::FilterInformation(
    const InformationPacket& packet,
    uint32_t actorId,
    bool isNation) {
    
    AttentionResult result;
    result.shouldReceive = false;
    result.adjustedRelevance = packet.baseRelevance;
    
    // Get actor
    const AIActor* actor = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_actorMutex);
        
        if (isNation) {
            auto it = m_nationActors.find(actorId);
            if (it != m_nationActors.end()) {
                actor = it->second.get();
            }
        } else {
            auto it = m_characterActors.find(actorId);
            if (it != m_characterActors.end()) {
                actor = it->second.get();
            }
        }
    }

    if (!actor) {
        result.filterReason = "Actor not found";
        return result;
    }
    
    const auto& profile = actor->attentionProfile;
    {
    // Check special interests first (always pass)
        if (!IsSpecialInterest(packet, profile)) {
        result.shouldReceive = true;
        result.adjustedRelevance = InformationRelevance::CRITICAL;
        result.attentionScore = 1.0f;
        result.filterReason = "Special interest";
        return result;
            }
    
    // Distance filter
        if (!PassesDistanceFilter(packet, profile)) {
        result.filterReason = "Too distant";
        return result;
        }
    
    // Type filter
        if (!PassesTypeFilter(packet, profile)) {
        result.filterReason = "Type not relevant";
        return result;
        }
    
    // Calculate attention score
    float score = CalculateAttentionScore(packet, profile);
    result.attentionScore = score;
    
    // Apply threshold
        if (score < profile.lowThreshold) {
        result.filterReason = "Below attention threshold";
        return result;
       }
    
    // Passed all filters
    result.shouldReceive = true;
    result.adjustedRelevance = AdjustRelevanceByProfile(
        packet.baseRelevance, profile, score);
    
    // Calculate processing delay based on attention
    if (score >= profile.criticalThreshold) {
        result.processingDelay = 0.0f; // Immediate
    } else if (score >= profile.highThreshold) {
        result.processingDelay = 1.0f; // 1 day
    } else if (score >= profile.mediumThreshold) {
        result.processingDelay = 3.0f; // 3 days
    } else {
        result.processingDelay = 7.0f; // 1 week
    }
    
    result.filterReason = "Passed";
    {
    // Update statistics
     {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        m_stats.totalFilters++;
        m_stats.totalPassed++;
     }
    }
    return result;
}
}

std::vector<uint32_t> AIAttentionManager::GetInterestedActors(
    const InformationPacket& packet,
    bool nationsOnly) {
    std::vector<uint32_t> interested;
    std::lock_guard<std::mutex> lock(m_actorMutex);
    // Check nations
    for (const auto& [nationId, nation] : m_nationActors) {
        AttentionResult result = FilterInformation(packet, nationId, true);
        if (result.shouldReceive) {
            interested.push_back(nationId);
        }
    }
    
    // Check characters if requested
    if (!nationsOnly) {
        for (const auto& [charId, character] : m_characterActors) {
            AttentionResult result = FilterInformation(packet, charId, false);
            if (result.shouldReceive) {
                interested.push_back(charId);
            }
        }
    }
    
    return interested;
}

// ============================================================================
// Profile Management
// ============================================================================

void AIAttentionManager::SetActorProfile(
    uint32_t actorId,
    bool isNation,
    const AttentionProfile& profile) {
    
    std::lock_guard<std::mutex> lock(m_actorMutex);
    
    AIActor* actor = nullptr;
    
    if (isNation) {
        auto it = m_nationActors.find(actorId);
        if (it != m_nationActors.end()) {
            actor = it->second.get();
        }
    } else {
        auto it = m_characterActors.find(actorId);
        if (it != m_characterActors.end()) {
            actor = it->second.get();
        }
    }
    
    if (actor) {
        actor->attentionProfile = profile;
    }
}

AttentionProfile* AIAttentionManager::GetActorProfile(
    uint32_t actorId,
    bool isNation) {
    
    std::lock_guard<std::mutex> lock(m_actorMutex);
    
    if (isNation) {
        auto it = m_nationActors.find(actorId);
        if (it != m_nationActors.end()) {
            return &it->second->attentionProfile;
        }
    } else {
        auto it = m_characterActors.find(actorId);
        if (it != m_characterActors.end()) {
            return &it->second->attentionProfile;
        }
    }
    
    return nullptr;
}

// ============================================================================
// Relationship Management
// ============================================================================

void AIAttentionManager::SetRivalry(uint32_t actor1, uint32_t actor2) {
    std::lock_guard<std::mutex> lock(m_actorMutex);
    
    // Add to both actors' rival lists
    auto it1 = m_nationActors.find(actor1);
    auto it2 = m_nationActors.find(actor2);
    
    if (it1 != m_nationActors.end()) {
        auto& rivals = it1->second->attentionProfile.rivalNations;
        if (std::find(rivals.begin(), rivals.end(), actor2) == rivals.end()) {
            rivals.push_back(actor2);
        }
    }
    
    if (it2 != m_nationActors.end()) {
        auto& rivals = it2->second->attentionProfile.rivalNations;
        if (std::find(rivals.begin(), rivals.end(), actor1) == rivals.end()) {
            rivals.push_back(actor1);
        }
    }
}

void AIAttentionManager::SetAlliance(uint32_t actor1, uint32_t actor2) {
    std::lock_guard<std::mutex> lock(m_actorMutex);
    
    // Add to both actors' ally lists
    auto it1 = m_nationActors.find(actor1);
    auto it2 = m_nationActors.find(actor2);
    
    if (it1 != m_nationActors.end()) {
        auto& allies = it1->second->attentionProfile.alliedNations;
        if (std::find(allies.begin(), allies.end(), actor2) == allies.end()) {
            allies.push_back(actor2);
        }
    }
    
    if (it2 != m_nationActors.end()) {
        auto& allies = it2->second->attentionProfile.alliedNations;
        if (std::find(allies.begin(), allies.end(), actor1) == allies.end()) {
            allies.push_back(actor1);
        }
    }
}

void AIAttentionManager::AddWatchedProvince(uint32_t actorId, uint32_t provinceId) {
    std::lock_guard<std::mutex> lock(m_actorMutex);
    
    auto it = m_nationActors.find(actorId);
    if (it != m_nationActors.end()) {
        auto& watched = it->second->attentionProfile.watchedProvinces;
        if (std::find(watched.begin(), watched.end(), provinceId) == watched.end()) {
            watched.push_back(provinceId);
        }
    }
}

// ============================================================================
// Template Initialization
// ============================================================================

void AIAttentionManager::InitializeArchetypeTemplates() {
    // Clear existing templates
    m_archetypeTemplates.clear();
    
    // Initialize each archetype
    for (int i = 0; i < static_cast<int>(CharacterArchetype::COUNT); ++i) {
    AI::CharacterArchetype archetype = static_cast<AI::CharacterArchetype>(i);
    AI::AttentionProfile profile;
        
        switch (archetype) {
            case CharacterArchetype::THE_CONQUEROR:
            case CharacterArchetype::WARRIOR_KING:
                InitializeConquerorTemplate(profile);
                break;
                
            case CharacterArchetype::THE_DIPLOMAT:
                InitializeDiplomatTemplate(profile);
                break;
                
            case CharacterArchetype::THE_MERCHANT:
                InitializeMerchantTemplate(profile);
                break;
                
            case CharacterArchetype::THE_SCHOLAR:
                InitializeScholarTemplate(profile);
                break;
                
            case CharacterArchetype::THE_BUILDER:
            case CharacterArchetype::THE_ADMINISTRATOR:
                InitializeBuilderTemplate(profile);
                break;
                
            case CharacterArchetype::THE_ZEALOT:
                profile.typeWeights[InformationType::RELIGIOUS_EVENT] = 1.0f;
                profile.typeWeights[InformationType::CULTURAL_SHIFT] = 0.8f;
                profile.typeWeights[InformationType::SUCCESSION_CRISIS] = 0.6f;
                break;
                
            case CharacterArchetype::THE_TYRANT:
                profile.typeWeights[InformationType::REBELLION] = 1.0f;
                profile.typeWeights[InformationType::MILITARY_ACTION] = 0.9f;
                profile.typeWeights[InformationType::SUCCESSION_CRISIS] = 0.8f;
                break;
                
            case CharacterArchetype::THE_REFORMER:
                profile.typeWeights[InformationType::TECHNOLOGY_ADVANCE] = 0.9f;
                profile.typeWeights[InformationType::CULTURAL_SHIFT] = 0.8f;
                profile.typeWeights[InformationType::ECONOMIC_CRISIS] = 0.7f;
                break;
                
            default:
                // Balanced profile
                for (int t = 0; t < static_cast<int>(InformationType::CULTURAL_SHIFT); ++t) {
                    profile.typeWeights[static_cast<InformationType>(t)] = 0.5f;
                }
                break;
        }
        
        profile.characterType = archetype;
        m_archetypeTemplates[archetype] = profile;
    }
}


void AIAttentionManager::InitializeConquerorTemplate(AttentionProfile& profile) {
    // Conquerors care most about military and territorial matters
    profile.typeWeights[AI::InformationType::MILITARY_ACTION] = 1.0f;
    profile.typeWeights[AI::InformationType::REBELLION] = 0.9f;
    profile.typeWeights[AI::InformationType::SUCCESSION_CRISIS] = 0.8f;
    profile.typeWeights[AI::InformationType::ALLIANCE_FORMATION] = 0.7f;
    profile.typeWeights[AI::InformationType::DIPLOMATIC_CHANGE] = 0.6f;
    profile.typeWeights[AI::InformationType::ECONOMIC_CRISIS] = 0.3f;
    profile.typeWeights[AI::InformationType::TECHNOLOGY_ADVANCE] = 0.4f;
    profile.typeWeights[AI::InformationType::RELIGIOUS_EVENT] = 0.2f;
    
    // Conquerors have wide attention range
    profile.maxAttentionDistance = 4000.0f;
    profile.attentionFalloffRate = 0.3f;
    
    // Lower thresholds - they care about more things
    profile.criticalThreshold = 0.8f;
    profile.highThreshold = 0.6f;
    profile.mediumThreshold = 0.3f;
    profile.lowThreshold = 0.1f;
}


void AIAttentionManager::InitializeDiplomatTemplate(AttentionProfile& profile) {
    // Diplomats focus on political relationships
    profile.typeWeights[AI::InformationType::DIPLOMATIC_CHANGE] = 1.0f;
    profile.typeWeights[AI::InformationType::ALLIANCE_FORMATION] = 1.0f;
    profile.typeWeights[AI::InformationType::SUCCESSION_CRISIS] = 0.8f;
    profile.typeWeights[AI::InformationType::TRADE_DISRUPTION] = 0.7f;
    profile.typeWeights[AI::InformationType::MILITARY_ACTION] = 0.5f;
    profile.typeWeights[AI::InformationType::CULTURAL_SHIFT] = 0.6f;
    profile.typeWeights[AI::InformationType::REBELLION] = 0.4f;
    
    // Moderate range but low falloff
    profile.maxAttentionDistance = 3000.0f;
    profile.attentionFalloffRate = 0.4f;
}


void AIAttentionManager::InitializeMerchantTemplate(AttentionProfile& profile) {
    // Merchants prioritize economic information
    profile.typeWeights[AI::InformationType::ECONOMIC_CRISIS] = 1.0f;
    profile.typeWeights[AI::InformationType::TRADE_DISRUPTION] = 1.0f;
    profile.typeWeights[AI::InformationType::TECHNOLOGY_ADVANCE] = 0.6f;
    profile.typeWeights[AI::InformationType::DIPLOMATIC_CHANGE] = 0.5f;
    profile.typeWeights[AI::InformationType::NATURAL_DISASTER] = 0.7f;
    profile.typeWeights[AI::InformationType::PLAGUE_OUTBREAK] = 0.8f;
    profile.typeWeights[AI::InformationType::MILITARY_ACTION] = 0.3f;
    
    // Wide range for trade networks
    profile.maxAttentionDistance = 5000.0f;
    profile.attentionFalloffRate = 0.5f;
}


void AIAttentionManager::InitializeScholarTemplate(AttentionProfile& profile) {
    // Scholars focus on technology and culture
    profile.typeWeights[AI::InformationType::TECHNOLOGY_ADVANCE] = 1.0f;
    profile.typeWeights[AI::InformationType::CULTURAL_SHIFT] = 0.9f;
    profile.typeWeights[AI::InformationType::RELIGIOUS_EVENT] = 0.7f;
    profile.typeWeights[AI::InformationType::PLAGUE_OUTBREAK] = 0.6f;
    profile.typeWeights[AI::InformationType::NATURAL_DISASTER] = 0.5f;
    profile.typeWeights[AI::InformationType::MILITARY_ACTION] = 0.2f;
    
    // Moderate range
    profile.maxAttentionDistance = 2500.0f;
    profile.attentionFalloffRate = 0.6f;
}


void AIAttentionManager::InitializeBuilderTemplate(AttentionProfile& profile) {
    // Builders care about internal development
    profile.typeWeights[AI::InformationType::ECONOMIC_CRISIS] = 0.8f;
    profile.typeWeights[AI::InformationType::NATURAL_DISASTER] = 0.9f;
    profile.typeWeights[AI::InformationType::PLAGUE_OUTBREAK] = 0.9f;
    profile.typeWeights[AI::InformationType::TECHNOLOGY_ADVANCE] = 0.7f;
    profile.typeWeights[AI::InformationType::REBELLION] = 0.6f;
    profile.typeWeights[AI::InformationType::TRADE_DISRUPTION] = 0.5f;
    profile.typeWeights[AI::InformationType::MILITARY_ACTION] = 0.3f;
    
    // Shorter range - more internal focus
    profile.maxAttentionDistance = 2000.0f;
    profile.attentionFalloffRate = 0.7f;
}
}

// ============================================================================
// Profile Creation
// ============================================================================

AttentionProfile AIAttentionManager::CreateProfileFromArchetype(
    CharacterArchetype archetype) const {
    
    auto it = m_archetypeTemplates.find(archetype);
    if (it != m_archetypeTemplates.end()) {
        return it->second;
    }
    
    // Return default balanced profile
    AttentionProfile defaultProfile;
    for (int t = 0; t < static_cast<int>(InformationType::CULTURAL_SHIFT); ++t) {
        defaultProfile.typeWeights[static_cast<InformationType>(t)] = 0.5f;
    }
    return defaultProfile;
}


AttentionProfile AIAttentionManager::CreateProfileFromPersonality(
    NationPersonality personality) const {
    
    // Map personality to archetype for simplicity
    CharacterArchetype mappedArchetype = CharacterArchetype::THE_ADMINISTRATOR;
    
    switch (personality) {
        case NationPersonality::EXPANSIONIST:
            mappedArchetype = CharacterArchetype::THE_CONQUEROR;
            break;
        case NationPersonality::DIPLOMATIC:
            mappedArchetype = CharacterArchetype::THE_DIPLOMAT;
            break;
        case NationPersonality::ECONOMIC:
            mappedArchetype = CharacterArchetype::THE_MERCHANT;
            break;
        case NationPersonality::TECHNOLOGICAL:
            mappedArchetype = CharacterArchetype::THE_SCHOLAR;
            break;
        case NationPersonality::RELIGIOUS:
            mappedArchetype = CharacterArchetype::THE_ZEALOT;
            break;
        case NationPersonality::DEVELOPMENTAL:
            mappedArchetype = CharacterArchetype::THE_BUILDER;
            break;
        case NationPersonality::AGGRESSIVE:
            mappedArchetype = CharacterArchetype::THE_TYRANT;
            break;
        case NationPersonality::PROGRESSIVE:
            mappedArchetype = CharacterArchetype::THE_REFORMER;
            break;
        default:
            break;
    }
    
    return CreateProfileFromArchetype(mappedArchetype);
}
}

// ============================================================================
// Utility Functions
// ============================================================================

NationPersonality AIAttentionManager::DerivePersonalityFromArchetype(
    CharacterArchetype archetype) const {
    
    switch (archetype) {
        case CharacterArchetype::THE_CONQUEROR:
        case CharacterArchetype::WARRIOR_KING:
            return NationPersonality::EXPANSIONIST;
            
        case CharacterArchetype::THE_DIPLOMAT:
            return NationPersonality::DIPLOMATIC;
            
        case CharacterArchetype::THE_MERCHANT:
            return NationPersonality::ECONOMIC;
            
        case CharacterArchetype::THE_SCHOLAR:
            return NationPersonality::TECHNOLOGICAL;
            
        case CharacterArchetype::THE_ZEALOT:
            return NationPersonality::RELIGIOUS;
            
        case CharacterArchetype::THE_BUILDER:
        case CharacterArchetype::THE_ADMINISTRATOR:
            return NationPersonality::DEVELOPMENTAL;
            
        case CharacterArchetype::THE_TYRANT:
            return NationPersonality::AGGRESSIVE;
            
        case CharacterArchetype::THE_REFORMER:
            return NationPersonality::PROGRESSIVE;
            
        default:
            return NationPersonality::BALANCED;
    }
}


float AIAttentionManager::CalculateAttentionScore(
    const InformationPacket& packet,
    const AttentionProfile& profile) const {
    
    float score = 0.0f;
    
    // Type weight component (40%)
    auto typeIt = profile.typeWeights.find(packet.type);
    if (typeIt != profile.typeWeights.end()) {
        score += typeIt->second * 0.4f;
    }
    
    // Severity component (30%)
    score += packet.severity * 0.3f;
    
    // Accuracy component (20%)
    score += packet.GetDegradedAccuracy() * 0.2f;
    
    // Relevance component (10%)
    float relevanceScore = 0.0f;
    switch (packet.baseRelevance) {
        case InformationRelevance::CRITICAL:
            relevanceScore = 1.0f;
            break;
        case InformationRelevance::HIGH:
            relevanceScore = 0.7f;
            break;
        case InformationRelevance::MEDIUM:
            relevanceScore = 0.4f;
            break;
        case InformationRelevance::LOW:
            relevanceScore = 0.2f;
            break;
        default:
            relevanceScore = 0.0f;
    }
    score += relevanceScore * 0.1f;
    
    // Apply global multiplier
    score *= m_globalAttentionMultiplier;
    
    return std::min(1.0f, score);
}
}
    
// ============================================================================
// Internal Filtering Logic
// ============================================================================

bool AIAttentionManager::PassesDistanceFilter(
    const InformationPacket& packet,
    const AttentionProfile& profile) const {
    
    // TODO: Calculate actual distance when province positions available
    // For now, use hop count as proxy for distance
    float estimatedDistance = packet.hopCount * 200.0f; // 200km per hop estimate
    
    return estimatedDistance <= profile.maxAttentionDistance;
   }

bool AIAttentionManager::PassesTypeFilter(
    const InformationPacket& packet,
    const AttentionProfile& profile) const {
    
    auto it = profile.typeWeights.find(packet.type);
    if (it == profile.typeWeights.end()) {
        return false; // Type not in profile
    }
    
    // Pass if weight is above minimum threshold
    return it->second > 0.1f;
   }

bool AIAttentionManager::IsSpecialInterest(
    const InformationPacket& packet,
    const AttentionProfile& profile) const {
    
    // Check if from rival nation
    if (packet.originatorEntityId != 0) {
        if (std::find(profile.rivalNations.begin(), 
                     profile.rivalNations.end(),
                     packet.originatorEntityId) != profile.rivalNations.end()) {
            return true;
        }
        
        // Check if from allied nation
        if (std::find(profile.alliedNations.begin(),
                     profile.alliedNations.end(),
                     packet.originatorEntityId) != profile.alliedNations.end()) {
            return true;
        }
    }
    
    // Check if from watched province
    if (std::find(profile.watchedProvinces.begin(),
                 profile.watchedProvinces.end(),
                 packet.sourceProvinceId) != profile.watchedProvinces.end()) {
        return true;
    }
    
    return false;
}

InformationRelevance AIAttentionManager::AdjustRelevanceByProfile(
    InformationRelevance base,
    const AttentionProfile& profile,
    float attentionScore) const {
    
    // Upgrade relevance based on attention score
    if (attentionScore >= profile.criticalThreshold) {
        return InformationRelevance::CRITICAL;
    } else if (attentionScore >= profile.highThreshold) {
        return std::max(base, InformationRelevance::HIGH);
    } else if (attentionScore >= profile.mediumThreshold) {
        return std::max(base, InformationRelevance::MEDIUM);
    } else if (attentionScore >= profile.lowThreshold) {
        return std::max(base, InformationRelevance::LOW);
    }
    
    return base;
}

// ============================================================================
// Statistics and Debugging
// ============================================================================

AIAttentionManager::PerformanceStats AIAttentionManager::GetStatistics() const {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    return m_stats;
}

void AIAttentionManager::ResetStatistics() {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    m_stats = PerformanceStats{};
}

std::vector<std::string> AIAttentionManager::GetActorList() const {
    std::vector<std::string> actors;
    
    std::lock_guard<std::mutex> lock(m_actorMutex);
    
    for (const auto& [id, actor] : m_nationActors) {
        actors.push_back("Nation: " + actor->actorName + " (ID: " + 
                        std::to_string(id) + ")");
    }
    
    for (const auto& [id, actor] : m_characterActors) {
        actors.push_back("Character: " + actor->actorName + " (ID: " + 
                        std::to_string(id) + ")");
    }
    
    return actors;
}

void AIAttentionManager::LogFilterDecision(
    uint32_t actorId,
    const std::string& reason) const {
    
    if (m_enableDetailedLogging) {
        std::cout << "[AIAttentionManager] Actor " << actorId 
                  << " filter: " << reason << std::endl;
    }
}

// ============================================================================
// Utility Functions Implementation
// ============================================================================

namespace AttentionUtils {

std::string ArchetypeToString(CharacterArchetype archetype) {
    switch (archetype) {
        case CharacterArchetype::WARRIOR_KING:
            return "Warrior King";
        case CharacterArchetype::THE_CONQUEROR:
            return "The Conqueror";
        case CharacterArchetype::THE_DIPLOMAT:
            return "The Diplomat";
        case CharacterArchetype::THE_ADMINISTRATOR:
            return "The Administrator";
        case CharacterArchetype::THE_MERCHANT:
            return "The Merchant";
        case CharacterArchetype::THE_SCHOLAR:
            return "The Scholar";
        case CharacterArchetype::THE_ZEALOT:
            return "The Zealot";
        case CharacterArchetype::THE_BUILDER:
            return "The Builder";
        case CharacterArchetype::THE_TYRANT:
            return "The Tyrant";
        case CharacterArchetype::THE_REFORMER:
            return "The Reformer";
        default:
            return "Unknown";
    }
}

std::string PersonalityToString(NationPersonality personality) {
    switch (personality) {
        case NationPersonality::EXPANSIONIST:
            return "Expansionist";
        case NationPersonality::DIPLOMATIC:
            return "Diplomatic";
        case NationPersonality::ECONOMIC:
            return "Economic";
        case NationPersonality::TECHNOLOGICAL:
            return "Technological";
        case NationPersonality::RELIGIOUS:
            return "Religious";
        case NationPersonality::DEVELOPMENTAL:
            return "Developmental";
        case NationPersonality::AGGRESSIVE:
            return "Aggressive";
        case NationPersonality::PROGRESSIVE:
            return "Progressive";
        case NationPersonality::BALANCED:
            return "Balanced";
        default:
            return "Unknown";
    }
}

CharacterArchetype StringToArchetype(const std::string& str) {
    if (str == "Warrior King") return CharacterArchetype::WARRIOR_KING;
    if (str == "The Conqueror") return CharacterArchetype::THE_CONQUEROR;
    if (str == "The Diplomat") return CharacterArchetype::THE_DIPLOMAT;
    if (str == "The Administrator") return CharacterArchetype::THE_ADMINISTRATOR;
    if (str == "The Merchant") return CharacterArchetype::THE_MERCHANT;
    if (str == "The Scholar") return CharacterArchetype::THE_SCHOLAR;
    if (str == "The Zealot") return CharacterArchetype::THE_ZEALOT;
    if (str == "The Builder") return CharacterArchetype::THE_BUILDER;
    if (str == "The Tyrant") return CharacterArchetype::THE_TYRANT;
    if (str == "The Reformer") return CharacterArchetype::THE_REFORMER;
    return CharacterArchetype::WARRIOR_KING; // Default
}

NationPersonality StringToPersonality(const std::string& str) {
    if (str == "Expansionist") return NationPersonality::EXPANSIONIST;
    if (str == "Diplomatic") return NationPersonality::DIPLOMATIC;
    if (str == "Economic") return NationPersonality::ECONOMIC;
    if (str == "Technological") return NationPersonality::TECHNOLOGICAL;
    if (str == "Religious") return NationPersonality::RELIGIOUS;
    if (str == "Developmental") return NationPersonality::DEVELOPMENTAL;
    if (str == "Aggressive") return NationPersonality::AGGRESSIVE;
    if (str == "Progressive") return NationPersonality::PROGRESSIVE;
    return NationPersonality::BALANCED;
}

} // namespace AttentionUtils

} // namespace AI
