// Created: September 25, 2025, 12:00 PM
// Location: include/game/ai/CharacterAI.h

#ifndef CHARACTER_AI_H
#define CHARACTER_AI_H

#include "game/ai/InformationPropagationSystem.h"
#include "game/ai/AIAttentionManager.h"
#include "game/ai/CharacterAIConstants.h"
//#include "game/character/CharacterComponents.h"
#include "core/ECS/ComponentAccessManager.h"
#include "core/types/game_types.h"
#include "utils/Random.h"

#include <memory>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <queue>
#include <mutex>

// Forward declarations to avoid pulling in heavy character/realm headers here.
// These are minimal declarations to allow AI headers to compile; full
// definitions are expected at implementation files that need them.
namespace game {
    namespace character {
        struct CharacterComponent;
        struct NobleArtsComponent;
    }
    namespace realm {
        enum class CouncilPosition : uint8_t;
    }
}

namespace AI {

// ============================================================================
// Character Ambitions and Motivations
// ============================================================================

enum class CharacterAmbition {
    GAIN_TITLE,          // Acquire higher rank
    ACCUMULATE_WEALTH,   // Build fortune
    GAIN_LAND,          // Acquire provinces
    INCREASE_PRESTIGE,   // Fame and glory
    FIND_LOVE,          // Marriage/romance
    REVENGE,            // Against rival
    KNOWLEDGE,          // Learn and discover
    PIETY,              // Religious devotion
    POWER,              // Political influence
    LEGACY,             // Dynasty building
    NONE
};

enum class CharacterMood {
    CONTENT,
    HAPPY,
    STRESSED,
    ANGRY,
    AFRAID,
    AMBITIOUS,
    DESPERATE
};

// ============================================================================
// Character Decisions
// ============================================================================

struct PlotDecision {
    enum PlotType {
        ASSASSINATION,
        COUP,
        BLACKMAIL,
        FABRICATE_CLAIM,
        STEAL_SECRETS,
        SABOTAGE,
        SEDUCTION
    } type;
    
    ::game::types::EntityID targetCharacter{0};
    float successChance = 0.0f;
    float riskLevel = 0.0f;
    std::vector<::game::types::EntityID> conspirators;
    bool shouldExecute = false;
};

struct ProposalDecision {
    enum ProposalType {
        REQUEST_TITLE,
        REQUEST_GOLD,
        REQUEST_MARRIAGE,
        PROPOSE_ALLIANCE,
        SUGGEST_WAR,
        RECOMMEND_POLICY,
        REQUEST_COUNCIL_POSITION
    } type;
    
    ::game::types::EntityID targetRuler{0};
    float acceptanceChance = 0.0f;
    std::string proposalDetails;
};

struct RelationshipDecision {
    ::game::types::EntityID targetCharacter{0};
    enum ActionType {
        BEFRIEND,
        SEDUCE,
        RIVAL,
        MENTOR,
        BLACKMAIL,
        MARRY,
        DIVORCE
    } action;
    float desirability = 0.0f;
};

struct PersonalDecision {
    enum ActionType {
        IMPROVE_SKILL,
        CHANGE_LIFESTYLE,
        MANAGE_ESTATE,
        HOST_FEAST,
        GO_ON_PILGRIMAGE,
        COMMISSION_ARTIFACT
    } action;
    float expectedBenefit = 0.0f;
    float cost = 0.0f;
};

// ============================================================================
// CharacterAI - Personal AI for individual characters
// ============================================================================

class CharacterAI {
    // Allow factory to access private members
    friend class CharacterAIFactory;
    
private:
    // Identity
    uint32_t m_actorId;
    ::game::types::EntityID m_characterId;
    std::string m_name;
    CharacterArchetype m_archetype;
    
    // Personality traits
    float m_ambition = 0.5f;      // Drive to achieve goals
    float m_loyalty = 0.5f;        // To liege/realm
    float m_honor = 0.5f;          // Moral code
    float m_greed = 0.5f;          // Desire for wealth
    float m_boldness = 0.5f;       // Risk-taking
    float m_compassion = 0.5f;     // Care for others
    
    // Current state
    CharacterAmbition m_primaryAmbition = CharacterAmbition::NONE;
    CharacterAmbition m_secondaryAmbition = CharacterAmbition::NONE;
    CharacterMood m_currentMood = CharacterMood::CONTENT;
    
    // Relationships
    std::unordered_map<::game::types::EntityID, float> m_relationships; // -100 to 100
    std::unordered_map<::game::types::EntityID, std::string> m_relationshipTypes;
    ::game::types::EntityID m_rival{0};
    ::game::types::EntityID m_lover{0};
    ::game::types::EntityID m_mentor{0};
    
    // Plots and schemes
    std::vector<PlotDecision> m_activePlots;
    std::vector<::game::types::EntityID> m_plotsAgainstMe;
    
    // Decision queues
    std::queue<PlotDecision> m_plotDecisions;
    std::queue<ProposalDecision> m_proposalDecisions;
    std::queue<RelationshipDecision> m_relationshipDecisions;
    std::queue<PersonalDecision> m_personalDecisions;
    
    // Memory
    struct CharacterMemory {
        ::game::types::EntityID character{0};
        std::string event;
        float impact; // How much it affected opinion
        std::chrono::system_clock::time_point when;
    };
    std::vector<CharacterMemory> m_memories;
    static constexpr size_t MAX_MEMORIES = 30;
    
    // Activity tracking
    std::chrono::system_clock::time_point m_lastActivityTime;
    std::chrono::system_clock::time_point m_lastAmbitionReview;
    uint32_t m_schemesExecuted = 0;
    
    // Component access
    std::shared_ptr<::core::ecs::ComponentAccessManager> m_componentAccess;

    // Thread safety - CRITICAL FIX: Add mutex for concurrent access from AIDirector
    mutable std::mutex m_stateMutex;

public:
    CharacterAI(
        uint32_t actorId,
        ::game::types::EntityID characterId,
        const std::string& name,
        CharacterArchetype archetype
    );
    ~CharacterAI() = default;
    
    // Core AI processing
    void ProcessInformation(const InformationPacket& packet);
    void UpdateAmbitions();
    void UpdateRelationships();
    void ExecuteDecisions();
    
    // Decision evaluation
    PlotDecision EvaluatePlot(::game::types::EntityID target);
    ProposalDecision EvaluateProposal();
    RelationshipDecision EvaluateRelationship(::game::types::EntityID target);
    PersonalDecision EvaluatePersonalAction();
    
    // Ambition system
    void SetPrimaryAmbition(CharacterAmbition ambition);
    void PursueAmbition();
    bool IsAmbitionAchieved() const;
    CharacterAmbition ChooseNewAmbition() const;
    
    // Relationship management
    void UpdateOpinion(::game::types::EntityID character, float change, const std::string& reason);
    float GetOpinion(::game::types::EntityID character) const;
    bool IsRival(::game::types::EntityID character) const;
    bool IsFriend(::game::types::EntityID character) const;
    
    // Plot management
    void StartPlot(const PlotDecision& plot);
    void JoinPlot(::game::types::EntityID plotLeader);
    void AbandonPlot(size_t plotIndex);
    bool IsPlottingAgainst(::game::types::EntityID character) const;
    
    // Mood and stress
    void UpdateMood();
    void AddStress(float amount);
    void ReduceStress(float amount);
    CharacterMood CalculateMood() const;
    
    // Utilities
    void SetComponentAccess(std::shared_ptr<::core::ecs::ComponentAccessManager> access) {
        m_componentAccess = access;
    }
    
    void SetLastActivityTime(std::chrono::system_clock::time_point time) {
        m_lastActivityTime = time;
    }
    
    const std::string& GetName() const { return m_name; }
    uint32_t GetActorId() const { return m_actorId; }
    ::game::types::EntityID GetCharacterId() const { return m_characterId; }
    CharacterArchetype GetArchetype() const { return m_archetype; }
    
private:
    // Internal helpers
    void RememberInteraction(::game::types::EntityID character, const std::string& event, float impact);
    void ForgetOldMemories();
    
    // Component access helpers
    const ::game::character::CharacterComponent* GetCharacterComponent();
    const ::game::character::NobleArtsComponent* GetNobleArtsComponent();
    
    // Personality-based decisions
    float CalculatePlotDesirability(const PlotDecision& plot) const;
    float CalculateProposalSuccess(const ProposalDecision& proposal) const;
    float CalculateRelationshipValue(::game::types::EntityID character) const;
    
    // Decision execution
    void ExecutePlot(const PlotDecision& plot);
    void ExecuteProposal(const ProposalDecision& proposal);
    void ExecuteRelationshipAction(const RelationshipDecision& decision);
    void ExecutePersonalAction(const PersonalDecision& decision);
    
    // Personality modifiers
    float GetAmbitionModifier() const;
    float GetLoyaltyModifier() const;
    float GetBoldnessModifier() const;
};

// ============================================================================
// Character AI Factory
// ============================================================================

class CharacterAIFactory {
public:
    static std::unique_ptr<CharacterAI> CreateAmbitiousNoble(
        uint32_t actorId,
        ::game::types::EntityID characterId,
        const std::string& name
    );
    
    static std::unique_ptr<CharacterAI> CreateLoyalVassal(
        uint32_t actorId,
        ::game::types::EntityID characterId,
        const std::string& name
    );
    
    static std::unique_ptr<CharacterAI> CreateCunningSchemer(
        uint32_t actorId,
        ::game::types::EntityID characterId,
        const std::string& name
    );
    
    static std::unique_ptr<CharacterAI> CreatePiousPriest(
        uint32_t actorId,
        ::game::types::EntityID characterId,
        const std::string& name
    );
};

} // namespace AI

#endif // CHARACTER_AI_H
