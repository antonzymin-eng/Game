// Created: September 25, 2025, 11:15 AM
// Location: include/game/ai/AIAttentionManager.h

#ifndef AI_ATTENTION_MANAGER_H
#define AI_ATTENTION_MANAGER_H

#include "core/types/game_types.h"

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>
#include <mutex>

// Forward declarations
namespace ECS {
    class ComponentAccessManager;
}

namespace AI {

// Forward declarations
struct InformationPacket;
enum class InformationRelevance;
enum class InformationType;

// Character archetype types (from existing character system)
enum class CharacterArchetype {
    WARRIOR_KING,
    THE_CONQUEROR,
    THE_DIPLOMAT,
    THE_ADMINISTRATOR,
    THE_MERCHANT,
    THE_SCHOLAR,
    THE_ZEALOT,
    THE_BUILDER,
    THE_TYRANT,
    THE_REFORMER,
    BALANCED,
    COUNT
};

// Nation AI personality (derived from ruler's archetype)
enum class NationPersonality {
    EXPANSIONIST,     // Conqueror, Warrior King
    DIPLOMATIC,       // Diplomat
    ECONOMIC,         // Merchant
    TECHNOLOGICAL,    // Scholar
    RELIGIOUS,        // Zealot
    DEVELOPMENTAL,    // Builder, Administrator
    AGGRESSIVE,       // Tyrant
    PROGRESSIVE,      // Reformer
    BALANCED,         // Default/mixed
    COUNT
};

// Attention profile defines what an AI actor cares about
struct AttentionProfile {
    // Base attention weights by information type
    std::unordered_map<InformationType, float> typeWeights;
    
    // Distance-based attention falloff
    float maxAttentionDistance = 3000.0f;
    float attentionFalloffRate = 0.5f;
    
    // Relevance thresholds
    float criticalThreshold = 0.9f;
    float highThreshold = 0.7f;
    float mediumThreshold = 0.4f;
    float lowThreshold = 0.2f;
    
    // Special interests (nations/regions of particular concern)
    std::vector<uint32_t> rivalNations;
    std::vector<uint32_t> alliedNations;
    std::vector<uint32_t> watchedProvinces;
    
    // Personality modifiers
    CharacterArchetype characterType = CharacterArchetype::BALANCED;
    NationPersonality nationPersonality = NationPersonality::BALANCED;
    
    AttentionProfile() = default;
};

// Attention filter result
struct AttentionResult {
    bool shouldReceive = false;
    InformationRelevance adjustedRelevance;
    float attentionScore = 0.0f;
    float processingDelay = 0.0f;  // Additional delay based on attention
    std::string filterReason;
};

// AI Actor represents either a nation or character
struct AIActor {
    uint32_t actorId;
    std::string actorName;
    bool isNation;  // true = NationAI, false = CharacterAI
    
    AttentionProfile attentionProfile;
    
    // Performance metrics
    uint32_t messagesReceived = 0;
    uint32_t messagesFiltered = 0;
    double averageAttentionScore = 0.0;
    
    AIActor(uint32_t id, const std::string& name, bool nation)
        : actorId(id), actorName(name), isNation(nation) {}
};

// Main attention management system
class AIAttentionManager {
private:
    // Actor registry
    std::unordered_map<uint32_t, std::unique_ptr<AIActor>> m_nationActors;
    std::unordered_map<uint32_t, std::unique_ptr<AIActor>> m_characterActors;
    mutable std::mutex m_actorMutex;
    
    // Archetype attention templates
    std::unordered_map<CharacterArchetype, AttentionProfile> m_archetypeTemplates;
    std::unordered_map<NationPersonality, AttentionProfile> m_personalityTemplates;
    
    // Reference to ECS
    std::shared_ptr<ECS::ComponentAccessManager> m_componentAccess;
    
    // Performance tracking
    struct PerformanceStats {
        uint64_t totalFilters = 0;
        uint64_t totalPassed = 0;
        uint64_t totalBlocked = 0;
        double averageFilterTime = 0.0;
    } m_stats;
    mutable std::mutex m_statsMutex;
    
    // Configuration
    bool m_enableDetailedLogging = false;
    float m_globalAttentionMultiplier = 1.0f;
    
public:
    AIAttentionManager(std::shared_ptr<ECS::ComponentAccessManager> componentAccess);
    ~AIAttentionManager();
    
    // System lifecycle
    void Initialize();
    void Shutdown();
    
    // Actor management
    uint32_t RegisterNationActor(uint32_t nationId, const std::string& name, 
                                 CharacterArchetype rulerArchetype);
    uint32_t RegisterCharacterActor(uint32_t characterId, const std::string& name,
                                    CharacterArchetype archetype);
    void UnregisterActor(uint32_t actorId, bool isNation);
    
    // Attention filtering - core functionality
    AttentionResult FilterInformation(const InformationPacket& packet, 
                                     uint32_t actorId, bool isNation);
    
    // Batch filtering for efficiency
    std::vector<uint32_t> GetInterestedActors(const InformationPacket& packet,
                                              bool nationsOnly = false);
    
    // Profile customization
    void SetActorProfile(uint32_t actorId, bool isNation, 
                         const AttentionProfile& profile);
    AttentionProfile* GetActorProfile(uint32_t actorId, bool isNation);
    
    // Relationship management
    void SetRivalry(uint32_t actor1, uint32_t actor2);
    void SetAlliance(uint32_t actor1, uint32_t actor2);
    void AddWatchedProvince(uint32_t actorId, uint32_t provinceId);
    
    // Template management
    void InitializeArchetypeTemplates();
    AttentionProfile CreateProfileFromArchetype(CharacterArchetype archetype) const;
    AttentionProfile CreateProfileFromPersonality(NationPersonality personality) const;
    
    // Utility functions
    NationPersonality DerivePersonalityFromArchetype(CharacterArchetype archetype) const;
    float CalculateAttentionScore(const InformationPacket& packet,
                                  const AttentionProfile& profile) const;
    
    // Statistics and debugging
    PerformanceStats GetStatistics() const;
    void ResetStatistics();
    std::vector<std::string> GetActorList() const;
    void EnableDetailedLogging(bool enable) { m_enableDetailedLogging = enable; }
    
private:
    // Internal filtering logic
    bool PassesDistanceFilter(const InformationPacket& packet,
                             const AttentionProfile& profile) const;
    bool PassesTypeFilter(const InformationPacket& packet,
                         const AttentionProfile& profile) const;
    bool IsSpecialInterest(const InformationPacket& packet,
                          const AttentionProfile& profile) const;
    
    InformationRelevance AdjustRelevanceByProfile(InformationRelevance base,
                                                  const AttentionProfile& profile,
                                                  float attentionScore) const;
    
    // Template initialization helpers
    void InitializeConquerorTemplate(AttentionProfile& profile);
    void InitializeDiplomatTemplate(AttentionProfile& profile);
    void InitializeMerchantTemplate(AttentionProfile& profile);
    void InitializeScholarTemplate(AttentionProfile& profile);
    void InitializeBuilderTemplate(AttentionProfile& profile);
    
    // Logging
    void LogFilterDecision(uint32_t actorId, const std::string& reason) const;
};

// Utility functions
namespace AttentionUtils {
    std::string ArchetypeToString(CharacterArchetype archetype);
    std::string PersonalityToString(NationPersonality personality);
    CharacterArchetype StringToArchetype(const std::string& str);
    NationPersonality StringToPersonality(const std::string& str);
}

} // namespace AI

#endif // AI_ATTENTION_MANAGER_H
