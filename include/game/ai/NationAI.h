// Created: September 25, 2025, 12:00 PM
// Location: include/game/ai/NationAI.h

#ifndef NATION_AI_H
#define NATION_AI_H

#include "game/ai/InformationPropagationSystem.h"
#include "game/realm/RealmComponents.h"
#include "core/ECS/ComponentAccessManager.h"
#include <memory>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <queue>

namespace AI {

// ============================================================================
// Strategic Goals for Nations
// ============================================================================

enum class StrategicGoal {
    EXPANSION,           // Conquer new territory
    CONSOLIDATION,       // Strengthen existing holdings
    ECONOMIC_GROWTH,     // Build wealth
    DIPLOMATIC_DOMINANCE,// Form alliances, isolate rivals
    CULTURAL_SUPREMACY,  // Spread religion/culture
    SURVIVAL,           // Defensive focus
    TECHNOLOGICAL_ADVANCEMENT,
    NONE
};

enum class ThreatLevel {
    EXISTENTIAL,
    SEVERE,
    MODERATE,
    LOW,
    MINIMAL
};

// ============================================================================
// Nation Decision Types
// ============================================================================

struct WarDecision {
    types::EntityID targetRealm{0};
    game::realm::CasusBelli justification;
    float expectedSuccess = 0.5f;
    float expectedCost = 0.0f;
    bool shouldDeclare = false;
};

struct DiplomaticDecision {
    types::EntityID targetRealm{0};
    enum ActionType {
        FORM_ALLIANCE,
        BREAK_ALLIANCE,
        IMPROVE_RELATIONS,
        DENOUNCE,
        OFFER_TRADE,
        DEMAND_TRIBUTE
    } action;
    float expectedBenefit = 0.0f;
};

struct EconomicDecision {
    enum ActionType {
        ADJUST_TAXES,
        BUILD_INFRASTRUCTURE,
        PROMOTE_TRADE,
        DEBASE_CURRENCY,
        STOCKPILE_RESOURCES
    } action;
    float parameter = 0.0f; // Tax rate, spending amount, etc.
    float expectedImpact = 0.0f;
};

struct MilitaryDecision {
    enum ActionType {
        RAISE_LEVIES,
        HIRE_MERCENARIES,
        BUILD_FORTIFICATIONS,
        DISBAND_TROOPS,
        MOVE_ARMIES
    } action;
    uint32_t targetSize = 0;
    types::EntityID targetLocation{0};
};

// ============================================================================
// NationAI - Strategic AI for realms
// ============================================================================

class NationAI {
private:
    // Identity
    uint32_t m_actorId;
    types::EntityID m_realmId;
    std::string m_name;
    CharacterArchetype m_personality;
    
    // Strategic state
    StrategicGoal m_primaryGoal = StrategicGoal::NONE;
    StrategicGoal m_secondaryGoal = StrategicGoal::NONE;
    float m_aggressiveness = 0.5f;
    float m_riskTolerance = 0.5f;
    
    // Threat assessment
    std::unordered_map<types::EntityID, ThreatLevel> m_threatAssessment;
    std::unordered_map<types::EntityID, float> m_relationshipScores;
    
    // Decision queues
    std::queue<WarDecision> m_warDecisions;
    std::queue<DiplomaticDecision> m_diplomaticDecisions;
    std::queue<EconomicDecision> m_economicDecisions;
    std::queue<MilitaryDecision> m_militaryDecisions;
    
    // Memory of recent events
    struct EventMemory {
        InformationType type;
        float severity;
        std::chrono::system_clock::time_point timestamp;
        types::EntityID source{0};
    };
    std::vector<EventMemory> m_recentEvents;
    static constexpr size_t MAX_EVENT_MEMORY = 50;
    
    // Performance tracking
    std::chrono::system_clock::time_point m_lastActivityTime;
    std::chrono::system_clock::time_point m_lastStrategicReview;
    uint32_t m_decisionsExecuted = 0;
    
    // Component access
    std::shared_ptr<ECS::ComponentAccessManager> m_componentAccess;
    
public:
    NationAI(
        uint32_t actorId,
        types::EntityID realmId,
        const std::string& name,
        CharacterArchetype personality
    );
    ~NationAI() = default;
    
    // Core AI processing
    void ProcessInformation(const InformationPacket& packet);
    void UpdateStrategy();
    void ExecuteDecisions();
    
    // Background updates
    void UpdateEconomy();
    void UpdateDiplomacy();
    void UpdateMilitary();
    void UpdateThreats();
    
    // Decision making
    WarDecision EvaluateWarDecision(types::EntityID target);
    DiplomaticDecision EvaluateDiplomacy(types::EntityID target);
    EconomicDecision EvaluateEconomicPolicy();
    MilitaryDecision EvaluateMilitaryNeeds();
    
    // Strategic planning
    void SetStrategicGoals();
    void AdjustPersonalityWeights();
    bool ShouldExpandTerritory() const;
    bool ShouldSeekAlliance(types::EntityID target) const;
    
    // Threat evaluation
    ThreatLevel AssessThreat(types::EntityID realm) const;
    float CalculateMilitaryStrength() const;
    float CalculateRelativeStrength(types::EntityID other) const;
    
    // Utility methods
    void SetComponentAccess(std::shared_ptr<ECS::ComponentAccessManager> access) {
        m_componentAccess = access;
    }
    
    void SetLastActivityTime(std::chrono::system_clock::time_point time) {
        m_lastActivityTime = time;
    }
    
    std::chrono::system_clock::time_point GetLastActivityTime() const {
        return m_lastActivityTime;
    }
    
    const std::string& GetName() const { return m_name; }
    uint32_t GetActorId() const { return m_actorId; }
    types::EntityID GetRealmId() const { return m_realmId; }
    
    // Personality-based modifiers
    float GetAggressionModifier() const;
    float GetDiplomacyModifier() const;
    float GetEconomicModifier() const;
    
private:
    // Internal decision helpers
    void RememberEvent(const InformationPacket& packet);
    void PruneOldMemories();
    
    game::realm::RealmComponent* GetRealmComponent();
    game::realm::DiplomaticRelationsComponent* GetDiplomacyComponent();
    
    float CalculateWarDesirability(types::EntityID target) const;
    float CalculateAllianceValue(types::EntityID target) const;
    float CalculateTradeValue(types::EntityID target) const;
    
    void QueueWarDecision(const WarDecision& decision);
    void QueueDiplomaticDecision(const DiplomaticDecision& decision);
    void QueueEconomicDecision(const EconomicDecision& decision);
    void QueueMilitaryDecision(const MilitaryDecision& decision);
    
    void ExecuteWarDeclaration(const WarDecision& decision);
    void ExecuteDiplomaticAction(const DiplomaticDecision& decision);
    void ExecuteEconomicPolicy(const EconomicDecision& decision);
    void ExecuteMilitaryAction(const MilitaryDecision& decision);
};

// ============================================================================
// Nation AI Factory
// ============================================================================

class NationAIFactory {
public:
    static std::unique_ptr<NationAI> CreateConquerorAI(
        uint32_t actorId,
        types::EntityID realmId,
        const std::string& name
    );
    
    static std::unique_ptr<NationAI> CreateDiplomatAI(
        uint32_t actorId,
        types::EntityID realmId,
        const std::string& name
    );
    
    static std::unique_ptr<NationAI> CreateMerchantAI(
        uint32_t actorId,
        types::EntityID realmId,
        const std::string& name
    );
    
    static std::unique_ptr<NationAI> CreateScholarAI(
        uint32_t actorId,
        types::EntityID realmId,
        const std::string& name
    );
};

} // namespace AI

#endif // NATION_AI_H
