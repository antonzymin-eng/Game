// Created: September 25, 2025, 12:00 PM
// Updated: October 16, 2025, 3:45 PM - Fixed namespace and component access types
// Location: include/game/ai/NationAI.h

#ifndef NATION_AI_H
#define NATION_AI_H

#include "core/ECS/MessageBus.h"
#include "game/ai/InformationPropagationSystem.h"
#include "game/realm/RealmComponents.h"
#include "game/ai/AIAttentionManager.h"
#include "core/ECS/ComponentAccessManager.h"
#include "core/types/game_types.h"
#include <jsoncpp/json/json.h>
#include <memory>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <queue>

namespace game {
namespace ai {

// ============================================================================
// Strategic Goals for Nations
// ============================================================================

enum class StrategicGoal {
    EXPANSION,
    CONSOLIDATION,
    ECONOMIC_GROWTH,
    DIPLOMATIC_DOMINANCE,
    CULTURAL_SUPREMACY,
    SURVIVAL,
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
    game::types::EntityID targetRealm{0};
    realm::CasusBelli justification;
    float expectedSuccess = 0.5f;
    float expectedCost = 0.0f;
    bool shouldDeclare = false;
};

struct DiplomaticDecision {
    game::types::EntityID targetRealm{0};
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
    float parameter = 0.0f;
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
    game::types::EntityID targetLocation{0};
};

// ============================================================================
// NationAI - Strategic AI for realms
// ============================================================================

class NationAI {
private:
    // Identity
    uint32_t m_actorId;
    game::types::EntityID m_realmId;
    std::string m_name;
    AI::CharacterArchetype m_personality;
    
    // Strategic state
    StrategicGoal m_primaryGoal = StrategicGoal::NONE;
    StrategicGoal m_secondaryGoal = StrategicGoal::NONE;
    float m_aggressiveness = 0.5f;
    float m_riskTolerance = 0.5f;
    
    // Threat assessment
    std::map<game::types::EntityID, ThreatLevel> m_threatAssessment;
    std::map<game::types::EntityID, float> m_relationshipScores;
    
    // Decision queues
    std::queue<WarDecision> m_warDecisions;
    std::queue<DiplomaticDecision> m_diplomaticDecisions;
    std::queue<EconomicDecision> m_economicDecisions;
    std::queue<MilitaryDecision> m_militaryDecisions;
    
    // Memory of recent events
    struct EventMemory {
        AI::InformationType type;
        float severity;
        std::chrono::system_clock::time_point timestamp;
    game::types::EntityID source{0};
    };
    std::vector<EventMemory> m_recentEvents;
    static constexpr size_t MAX_EVENT_MEMORY = 50;
    
    // Performance tracking
    std::chrono::system_clock::time_point m_lastActivityTime;
    std::chrono::system_clock::time_point m_lastStrategicReview;
    uint64_t m_decisionsExecuted = 0;
    
    // Component access - FIXED: Use raw pointer to match .cpp
    ::core::ecs::ComponentAccessManager* m_componentAccess;
    
public:
    NationAI(
        uint32_t actorId,
        game::types::EntityID realmId,
        const std::string& name,
        AI::CharacterArchetype personality
    );
    ~NationAI() = default;
    
    // Core AI processing
    void ProcessInformation(const AI::InformationPacket& packet);
    void UpdateStrategy();
    void ExecuteDecisions();
    
    // Background updates
    void UpdateEconomy();
    void UpdateDiplomacy();
    void UpdateMilitary();
    void UpdateThreats();
    
    // Decision making
    WarDecision EvaluateWarDecision(game::types::EntityID target);
    DiplomaticDecision EvaluateDiplomacy(game::types::EntityID target);
    EconomicDecision EvaluateEconomicPolicy();
    MilitaryDecision EvaluateMilitaryNeeds();
    
    // Strategic planning
    void SetStrategicGoals();
    void AdjustPersonalityWeights();
    
    // Threat evaluation
    ThreatLevel AssessThreat(game::types::EntityID realm) const;
    float CalculateMilitaryStrength() const;
    float CalculateRelativeStrength(game::types::EntityID other) const;
    
    // Utility methods - FIXED: Match .cpp signature
    void SetComponentAccess(::core::ecs::ComponentAccessManager* access);
    
    void SetLastActivityTime(std::chrono::system_clock::time_point time) {
        m_lastActivityTime = time;
    }
    
    std::chrono::system_clock::time_point GetLastActivityTime() const;
    
    // State queries - match .cpp implementations
    uint32_t GetActorId() const;
    game::types::EntityID GetRealmId() const;
    std::string GetName() const;
    AI::CharacterArchetype GetPersonality() const;
    StrategicGoal GetPrimaryGoal() const;
    StrategicGoal GetSecondaryGoal() const;
    float GetAggressiveness() const;
    float GetRiskTolerance() const;
    size_t GetPendingDecisions() const;
    uint64_t GetDecisionsExecuted() const;
    const std::map<game::types::EntityID, ThreatLevel>& GetThreatAssessment() const;
    const std::map<game::types::EntityID, float>& GetRelationshipScores() const;
    
    // Activity tracking
    void UpdateActivity();
    bool IsActive() const;
    
    // Debug & statistics
    void PrintDebugInfo() const;
    Json::Value GetStatistics() const;
    
private:
    // Internal decision helpers
    void RememberEvent(const AI::InformationPacket& packet);
    
    // FIXED: const methods matching .cpp
    const realm::RealmComponent* GetRealmComponent() const;
    const realm::DiplomaticRelationsComponent* GetDiplomacyComponent() const;
    
    float CalculateWarDesirability(game::types::EntityID target) const;
    float CalculateAllianceValue(game::types::EntityID target) const;
    float CalculateTradeValue(game::types::EntityID target) const;
    
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

} // namespace ai
} // namespace game

#endif // NATION_AI_H
