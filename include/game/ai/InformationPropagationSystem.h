// Created: September 25, 2025, 10:00 AM
// Location: include/game/ai/InformationPropagationSystem.h

#ifndef INFORMATION_PROPAGATION_SYSTEM_H
#define INFORMATION_PROPAGATION_SYSTEM_H

#include "core/types/game_types.h"
#include "game/time/TimeManagementSystem.h"

#include <memory>
#include <vector>
#include <unordered_map>
#include <queue>
#include <string>
#include <chrono>
#include <optional>
#include <mutex>
#include <cstdint>

// Forward declarations
namespace ECS {
    class Entity;
    class ComponentAccessManager;
}

class MessageBus;
class GameDate;
class TimeManagementSystem;
struct ProvinceComponent;
struct DiplomaticRelations;

namespace AI {

// Relevance categories for information filtering
enum class InformationRelevance {
    CRITICAL,      // Immediate neighbors, direct threats
    HIGH,          // Allies, trade partners, rivals  
    MEDIUM,        // Regional powers, indirect concerns
    LOW,           // Distant nations, general awareness
    IRRELEVANT     // Too far/unimportant to care
};

// Types of information that can propagate
enum class InformationType {
    MILITARY_ACTION,
    DIPLOMATIC_CHANGE,
    ECONOMIC_CRISIS,
    SUCCESSION_CRISIS,
    REBELLION,
    TECHNOLOGY_ADVANCE,
    RELIGIOUS_EVENT,
    TRADE_DISRUPTION,
    ALLIANCE_FORMATION,
    NATURAL_DISASTER,
    PLAGUE_OUTBREAK,
    CULTURAL_SHIFT
};

// Represents AI-consumable information derived from game events
struct InformationPacket {
    InformationType type;
    InformationRelevance baseRelevance;
    uint32_t sourceProvinceId;
    uint32_t originatorEntityId;  // Nation or character that triggered event
    
    // Core event data
    std::string eventDescription;
    float severity;  // 0.0-1.0, affects propagation speed
    float accuracy;  // 1.0 at source, degrades with distance
    
    // Temporal data
    GameDate eventOccurredDate;
    GameDate packetCreatedDate;
    
    // Propagation tracking
    uint32_t hopCount;  // How many relays from source
    std::vector<uint32_t> propagationPath;  // Province IDs traveled through
    
    // Payload for AI decision-making
    std::unordered_map<std::string, float> numericData;
    std::unordered_map<std::string, std::string> textData;
    
    InformationPacket();
    float GetDegradedAccuracy() const;
    float GetPropagationSpeed() const;
};

// Controls how information spreads through game world
class InformationPropagationSystem {
public:
    InformationPropagationSystem(
        std::shared_ptr<ECS::ComponentAccessManager> componentAccess,
        std::shared_ptr<MessageBus> messageBus,
        std::shared_ptr<TimeManagementSystem> timeSystem
    );
    
    ~InformationPropagationSystem();
    
    // System lifecycle
    void Initialize();
    void Update(float deltaTime);
    void Shutdown();
    
    // Event conversion - transforms game events into information
    void ConvertEventToInformation(
        const std::string& eventType,
        uint32_t sourceProvinceId,
        const std::unordered_map<std::string, float>& eventData
    );
    
    // Manual information injection for special cases
    void InjectInformation(const InformationPacket& packet);
    
    // Propagation control
    void StartPropagation(const InformationPacket& packet);
    void ProcessPropagationQueue();
    
    // Configuration
    void SetPropagationSpeedMultiplier(float multiplier);
    void SetAccuracyDegradationRate(float rate);
    void SetMaxPropagationDistance(float distance);
    
    // Intelligence network modifiers
    void SetIntelligenceBonus(uint32_t nationId, uint32_t targetNationId, float bonus);
    float GetEffectivePropagationDelay(uint32_t fromProvince, uint32_t toProvince) const;
    
    // Relevance calculation
    InformationRelevance CalculateRelevance(
        const InformationPacket& packet,
        uint32_t receiverNationId
    ) const;
    
    // Statistics and debugging
    struct PropagationStats {
        uint32_t totalPacketsCreated;
        uint32_t totalPacketsPropagated;
        uint32_t packetsDroppedIrrelevant;
        uint32_t packetsDroppedDistance;
        float averagePropagationTime;
        float averageAccuracyAtDelivery;
    };
    
    PropagationStats GetStatistics() const;
    void ResetStatistics();
    
    // Threading
    std::string GetThreadingStrategy() const { return "THREAD_POOL"; }
    std::string GetThreadingRationale() const { 
        return "Information propagation involves distance calculations and pathfinding";
    }
    
private:
    struct PropagationNode {
        InformationPacket packet;
        uint32_t currentProvinceId;
        uint32_t targetNationId;
        GameDate scheduledArrival;
        float remainingDistance;
        
        bool operator>(const PropagationNode& other) const {
            return scheduledArrival > other.scheduledArrival;
        }
    };
    
    // Core components
    std::shared_ptr<ECS::ComponentAccessManager> m_componentAccess;
    std::shared_ptr<MessageBus> m_messageBus;
    std::shared_ptr<TimeManagementSystem> m_timeSystem;
    
    // Propagation queue (priority queue by arrival time)
    std::priority_queue<
        PropagationNode,
        std::vector<PropagationNode>,
        std::greater<PropagationNode>
    > m_propagationQueue;
    
    // Active propagations indexed by province
    std::unordered_map<uint32_t, std::vector<PropagationNode>> m_activeByProvince;
    
    // Intelligence network bonuses (nationId -> targetNationId -> speedBonus)
    std::unordered_map<uint32_t, std::unordered_map<uint32_t, float>> m_intelligenceBonuses;
    
    // Configuration parameters
    float m_propagationSpeedMultiplier;
    float m_accuracyDegradationRate;
    float m_maxPropagationDistance;
    float m_baseMessageSpeed;  // km per day
    
    // Statistics tracking
    mutable std::mutex m_statsMutex;
    PropagationStats m_statistics;
    
    // Helper methods
    float CalculateDistance(uint32_t fromProvince, uint32_t toProvince) const;
    std::vector<uint32_t> FindPropagationPath(uint32_t from, uint32_t to) const;
    std::vector<uint32_t> GetNeighborProvinces(uint32_t provinceId) const;
    
    void PropagateToNeighbors(const PropagationNode& node);
    void DeliverInformation(const InformationPacket& packet, uint32_t nationId);
    
    bool ShouldPropagate(const InformationPacket& packet, uint32_t provinceId) const;
    float CalculatePropagationDelay(
        const InformationPacket& packet,
        float distance,
        uint32_t fromNation,
        uint32_t toNation
    ) const;
    
    // Event handlers
    void OnGameEvent(const std::string& eventType, const void* eventData);
    void OnTimeUpdate(const GameDate& currentDate);
    
    // Cache for province positions (populated on Initialize)
    struct ProvincePosition {
        float x, y;  // Map coordinates
        uint32_t ownerNationId;
    };
    std::unordered_map<uint32_t, ProvincePosition> m_provinceCache;
    
    void RebuildProvinceCache();
    void UpdateStatistics(const PropagationNode& node, bool delivered);
};

// Factory for creating information packets from events
class InformationFactory {
public:
    static InformationPacket CreateFromMilitaryEvent(
        uint32_t provinceId,
        const std::string& eventType,
        const std::unordered_map<std::string, float>& data
    );
    
    static InformationPacket CreateFromDiplomaticEvent(
        uint32_t nationId,
        const std::string& eventType,
        const std::unordered_map<std::string, std::string>& data
    );
    
    static InformationPacket CreateFromEconomicEvent(
        uint32_t provinceId,
        float severity,
        const std::string& description
    );
    
    static InformationPacket CreateFromSuccessionCrisis(
        uint32_t nationId,
        uint32_t characterId,
        const std::string& claimantName
    );
    
private:
    static InformationType ClassifyEventType(const std::string& eventType);
    static float CalculateSeverity(const std::string& eventType, 
                                  const std::unordered_map<std::string, float>& data);
};

} // namespace AI

#endif // INFORMATION_PROPAGATION_SYSTEM_H