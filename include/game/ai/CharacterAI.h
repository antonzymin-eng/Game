// Created: September 25, 2025, 12:00 PM
// Location: include/game/ai/CharacterAI.h

#ifndef CHARACTER_AI_H
#define CHARACTER_AI_H

#include "game/ai/InformationPropagationSystem.h"
//#include "game/character/CharacterComponents.h"
#include "core/ECS/ComponentAccessManager.h"
#include "core/types/game_types.h"

#include <memory>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <queue>

namespace AI {

// Forward declarations to avoid pulling in heavy character/realm headers here.
// These are minimal declarations to allow AI headers to compile; full
// definitions are expected at implementation files that need them.
namespace game {
    namespace character {
        struct CharacterComponent;
        struct NobleArtsComponent;
    }
    namespace realm {
        enum class CouncilPosition : int;
    }
}

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
    
    types::EntityID targetCharacter{0};
    float successChance = 0.0f;
    float riskLevel = 0.0f;
    std::vector<types::EntityID> conspirators;
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
    
    types::EntityID targetRuler{0};
    float acceptanceChance = 0.0f;
    std::string proposalDetails;
};

struct RelationshipDecision {
    types::EntityID targetCharacter{0};
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
private:
    // Identity
    uint32_t m_actorId;
    types::EntityID m_characterId;
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
    std::unordered_map<types::EntityID, float> m_relationships; // -100 to 100
    std::unordered_map<types::EntityID, std::string> m_relationshipTypes;
    types::EntityID m_rival{0};
    types::EntityID m_lover{0};
    types::EntityID m_mentor{0};
    
    // Plots and schemes
    std::vector<PlotDecision> m_activePlots;
    std::vector<types::EntityID> m_plotsAgainstMe;
    
    // Decision queues
    std::queue<PlotDecision> m_plotDecisions;
    std::queue<ProposalDecision> m_proposalDecisions;
    std::queue<RelationshipDecision> m_relationshipDecisions;
    std::queue<PersonalDecision> m_personalDecisions;
    
    // Memory
    struct CharacterMemory {
        types::EntityID character{0};
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
    std::shared_ptr<ECS::ComponentAccessManager> m_componentAccess;
    
public:
    CharacterAI(
        uint32_t actorId,
        types::EntityID characterId,
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
    PlotDecision EvaluatePlot(types::EntityID target);
    ProposalDecision EvaluateProposal();
    RelationshipDecision EvaluateRelationship(types::EntityID target);
    PersonalDecision EvaluatePersonalAction();
    
    // Ambition system
    void SetPrimaryAmbition(CharacterAmbition ambition);
    void PursueAmbition();
    bool IsAmbitionAchieved() const;
    CharacterAmbition ChooseNewAmbition() const;
    
    // Relationship management
    void UpdateOpinion(types::EntityID character, float change, const std::string& reason);
    float GetOpinion(types::EntityID character) const;
    bool IsRival(types::EntityID character) const;
    bool IsFriend(types::EntityID character) const;
    
    // Plot management
    void StartPlot(const PlotDecision& plot);
    void JoinPlot(types::EntityID plotLeader);
    void AbandonPlot(size_t plotIndex);
    bool IsPlottingAgainst(types::EntityID character) const;
    
    // Mood and stress
    void UpdateMood();
    void AddStress(float amount);
    void ReduceStress(float amount);
    CharacterMood CalculateMood() const;
    
    // Utilities
    void SetComponentAccess(std::shared_ptr<ECS::ComponentAccessManager> access) {
        m_componentAccess = access;
    }
    
    void SetLastActivityTime(std::chrono::system_clock::time_point time) {
        m_lastActivityTime = time;
    }
    
    const std::string& GetName() const { return m_name; }
    uint32_t GetActorId() const { return m_actorId; }
    types::EntityID GetCharacterId() const { return m_characterId; }
    CharacterArchetype GetArchetype() const { return m_archetype; }
    
private:
    // Internal helpers
    void RememberInteraction(types::EntityID character, const std::string& event, float impact);
    void ForgetOldMemories();
    
    // Component access helpers
    game::character::CharacterComponent* GetCharacterComponent();
    game::character::NobleArtsComponent* GetNobleArtsComponent();
    
    // Personality-based decisions
    float CalculatePlotDesirability(const PlotDecision& plot) const;
    float CalculateProposalSuccess(const ProposalDecision& proposal) const;
    float CalculateRelationshipValue(types::EntityID character) const;
    
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
// Council AI - Advisor AI for realm councils
// ============================================================================

class CouncilAI {
private:
    uint32_t m_actorId;
    types::EntityID m_realmId;
    std::string m_name;
    
    // Council composition
    std::unordered_map<game::realm::CouncilPosition, types::EntityID> m_councilors;
    
    // Voting history
    struct VoteRecord {
        std::string proposalType;
        bool votedFor;
        std::chrono::system_clock::time_point when;
    };
    std::vector<VoteRecord> m_votingHistory;
    
public:
    CouncilAI(
        uint32_t actorId,
        types::EntityID realmId,
        const std::string& name
    );
    
    void ProcessInformation(const InformationPacket& packet);
    
    // Council decisions
    bool ShouldApproveWar(types::EntityID target) const;
    bool ShouldApproveTaxIncrease(float newRate) const;
    bool ShouldApproveAlliance(types::EntityID ally) const;
    bool ShouldApproveSuccession(types::EntityID heir) const;
    
    // Advisor recommendations
    std::vector<std::string> GetEconomicAdvice() const;
    std::vector<std::string> GetMilitaryAdvice() const;
    std::vector<std::string> GetDiplomaticAdvice() const;
    
    const std::string& GetName() const { return m_name; }
};

// ============================================================================
// Character AI Factory
// ============================================================================

class CharacterAIFactory {
public:
    static std::unique_ptr<CharacterAI> CreateAmbitiousNoble(
        uint32_t actorId,
        types::EntityID characterId,
        const std::string& name
    );
    
    static std::unique_ptr<CharacterAI> CreateLoyalVassal(
        uint32_t actorId,
        types::EntityID characterId,
        const std::string& name
    );
    
    static std::unique_ptr<CharacterAI> CreateCunningSchemer(
        uint32_t actorId,
        types::EntityID characterId,
        const std::string& name
    );
    
    static std::unique_ptr<CharacterAI> CreatePiousPriest(
        uint32_t actorId,
        types::EntityID characterId,
        const std::string& name
    );
};

} // namespace AI

#endif // CHARACTER_AI_H
