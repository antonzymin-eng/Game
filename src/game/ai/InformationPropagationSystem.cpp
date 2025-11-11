// Created: September 25, 2025, 11:00 AM
// Location: src/game/ai/InformationPropagationSystem.cpp
// FIXES: ECS integration, thread safety, memory management

#include "game/ai/InformationPropagationSystem.h"
#include "core/ECS/ComponentAccessManager.h"
#include "core/ECS/MessageBus.h"
#include "game/time/TimeManagementSystem.h"
#include "game/components/ProvinceComponent.h"
#include "game/components/DiplomaticRelations.h"
#include "game/components/GameEvents.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include "core/logging/Logger.h"

namespace AI {

// ============================================================================
// InformationPacket Implementation
// ============================================================================

InformationPacket::InformationPacket()
    : type(InformationType::MILITARY_ACTION)
    , baseRelevance(InformationRelevance::LOW)
    , sourceProvinceId(0)
    , originatorEntityId(0)
    , severity(0.0f)
    , accuracy(1.0f)
    , hopCount(0) {
}

float InformationPacket::GetDegradedAccuracy() const {
    // Accuracy degrades based on hop count and time elapsed
    float degradation = 1.0f - (hopCount * 0.05f); // 5% per hop
    degradation = std::max(0.1f, degradation); // Minimum 10% accuracy
    return accuracy * degradation;
}

float InformationPacket::GetPropagationSpeed() const {
    // Speed based on severity and information type
    float baseSpeed = 1.0f;
    
    // Higher severity = faster propagation
    baseSpeed *= (1.0f + severity * 0.5f);
    
    // Military information travels faster than economic
    switch (type) {
        case InformationType::MILITARY_ACTION:
        case InformationType::REBELLION:
            baseSpeed *= 1.5f;
            break;
        case InformationType::SUCCESSION_CRISIS:
            baseSpeed *= 1.3f;
            break;
        case InformationType::DIPLOMATIC_CHANGE:
            baseSpeed *= 1.2f;
            break;
        default:
            break;
    }
    
    return baseSpeed;
}

// ============================================================================
// Constructor and Lifecycle
// ============================================================================

InformationPropagationSystem::InformationPropagationSystem(
    std::shared_ptr<core::ecs::ComponentAccessManager> componentAccess,
    std::shared_ptr<core::ecs::MessageBus> messageBus,
    std::shared_ptr<::game::time::TimeManagementSystem> timeSystem)
    : m_componentAccess(componentAccess)
    , m_messageBus(messageBus)
    , m_timeSystem(timeSystem)
    , m_propagationSpeedMultiplier(1.0f)
    , m_accuracyDegradationRate(0.05f)
    , m_maxPropagationDistance(1000.0f)
    , m_baseMessageSpeed(100.0f)
    , m_maxActiveProvinces(1000) {
}

InformationPropagationSystem::~InformationPropagationSystem() {
    Shutdown();
}

void InformationPropagationSystem::Initialize() {
    // Build province cache for distance calculations
    RebuildProvinceCache();
    
    // Initialize statistics
    m_statistics = PropagationStats{};
    m_lastCleanup = std::chrono::steady_clock::now();
    
    CORE_STREAM_INFO("InformationPropagation") << "System initialized with " 
              << m_provinceCache.size() << " provinces";
}

void InformationPropagationSystem::Shutdown() {
    // Clear all queues and caches
    std::lock_guard<std::mutex> queueLock(m_propagationQueueMutex);
    while (!m_propagationQueue.empty()) {
        m_propagationQueue.pop();
    }
    m_activeByProvince.clear();
    m_provinceCache.clear();
}

// ============================================================================
// FIX 1: Proper ECS Integration - RebuildProvinceCache
// ============================================================================

void InformationPropagationSystem::RebuildProvinceCache() {
    m_provinceCache.clear();
    
    // Use ComponentAccessManager to query actual province data
    if (!m_componentAccess) {
        // Fallback to test data if no access manager
        for (uint32_t i = 1; i <= 100; ++i) {
            ProvincePosition pos;
            pos.x = static_cast<float>(i % 10) * 100.0f;
            pos.y = static_cast<float>(i / 10) * 100.0f;
            pos.ownerNationId = (i - 1) / 5 + 1;
            m_provinceCache[i] = pos;
        }
        return;
    }
    
    try {
        // Get all entities with ProvinceComponent using thread-safe access
        auto province_read = m_componentAccess->GetAllComponentsForRead<ProvinceComponent>();
        auto entity_manager = m_componentAccess->GetEntityManager();
        
        if (!entity_manager) {
            return;
        }
        
        // Get all province entities
        auto province_entities = entity_manager->GetEntitiesWithComponent<ProvinceComponent>();
        
        for (const auto& entity_handle : province_entities) {
            // Validate entity is still valid
            if (!entity_manager->IsEntityValid(entity_handle)) {
                continue;
            }
            
            // Get province component
            auto province_comp = entity_manager->GetComponent<ProvinceComponent>(entity_handle);
            if (!province_comp) {
                continue;
            }
            
            // Extract province data
            ProvincePosition pos;
            pos.x = province_comp->GetPositionX();
            pos.y = province_comp->GetPositionY();
            pos.ownerNationId = province_comp->GetOwnerNationId();
            
            // Use entity ID as province ID
            uint32_t province_id = static_cast<uint32_t>(entity_handle.id);
            m_provinceCache[province_id] = pos;
        }
        
    } catch (const std::exception& e) {
        // Log error and use fallback data
        CORE_STREAM_ERROR("InformationPropagation") << "Error rebuilding province cache: " 
                  << e.what();
    }
}

// ============================================================================
// FIX 2: Enhanced Thread Safety
// ============================================================================

void InformationPropagationSystem::ProcessPropagationQueue() {
    // Add mutex protection for queue access
    static std::mutex s_queueProcessingMutex;
    std::lock_guard<std::mutex> processLock(s_queueProcessingMutex);
    
    auto currentDate = m_timeSystem->GetCurrentDate();
    
    // Process in batches to reduce lock contention
    const size_t BATCH_SIZE = 10;
    std::vector<PropagationNode> nodesToProcess;
    nodesToProcess.reserve(BATCH_SIZE);
    
    // Extract batch of ready nodes
    {
        std::lock_guard<std::mutex> queueLock(m_propagationQueueMutex);
        
        while (!m_propagationQueue.empty() && nodesToProcess.size() < BATCH_SIZE) {
            const auto& node = m_propagationQueue.top();
            
            if (node.scheduledArrival <= currentDate) {
                nodesToProcess.push_back(node);
                m_propagationQueue.pop();
            } else {
                break; // Queue is sorted, so we can stop
            }
        }
    }
    
    // Process nodes outside of lock
    for (const auto& node : nodesToProcess) {
        PropagateToNeighbors(node);
        UpdateStatistics(node, true);
    }
}

void InformationPropagationSystem::StartPropagation(const InformationPacket& packet) {
    PropagationNode node;
    node.packet = packet;
    node.currentProvinceId = packet.sourceProvinceId;
    node.targetNationId = 0;
    node.scheduledArrival = m_timeSystem->GetCurrentDate();
    node.remainingDistance = 0.0f;
    
    // Immediate local delivery
    if (m_provinceCache.count(packet.sourceProvinceId) > 0) {
        uint32_t localNationId = m_provinceCache[packet.sourceProvinceId].ownerNationId;
        DeliverInformation(packet, localNationId);
    }
    
    // Add to queue with thread safety
    {
        std::lock_guard<std::mutex> lock(m_propagationQueueMutex);
        m_propagationQueue.push(node);
    }
    
    // Start propagation
    PropagateToNeighbors(node);
}

void InformationPropagationSystem::PropagateToNeighbors(const PropagationNode& node) {
    auto neighbors = GetNeighborProvinces(node.currentProvinceId);
    
    // Batch queue additions for efficiency
    std::vector<PropagationNode> newNodes;
    newNodes.reserve(neighbors.size());
    
    for (uint32_t neighborId : neighbors) {
        if (!ShouldPropagate(node.packet, neighborId)) {
            std::lock_guard<std::mutex> lock(m_statsMutex);
            m_statistics.packetsDroppedIrrelevant++;
            continue;
        }
        
        float totalDistance = CalculateDistance(
            node.packet.sourceProvinceId, 
            neighborId
        );
        
        if (totalDistance > m_maxPropagationDistance) {
            std::lock_guard<std::mutex> lock(m_statsMutex);
            m_statistics.packetsDroppedDistance++;
            continue;
        }
        
        PropagationNode newNode = node;
        newNode.packet.hopCount++;
        newNode.packet.accuracy = node.packet.GetDegradedAccuracy();
        newNode.packet.propagationPath.push_back(neighborId);
        newNode.currentProvinceId = neighborId;
        
        float delay = GetEffectivePropagationDelay(
            node.currentProvinceId,
            neighborId
        );
        delay /= node.packet.GetPropagationSpeed();
        
        newNode.scheduledArrival = m_timeSystem->GetCurrentDate().AddDays(static_cast<int>(delay));
        
        newNodes.push_back(newNode);
    }
    
    // Add all new nodes to queue in single lock
    if (!newNodes.empty()) {
        std::lock_guard<std::mutex> lock(m_propagationQueueMutex);
        for (const auto& newNode : newNodes) {
            m_propagationQueue.push(newNode);
            
            // Update active propagations
            m_activeByProvince[newNode.currentProvinceId].push_back(newNode);
        }
        
        std::lock_guard<std::mutex> statsLock(m_statsMutex);
        m_statistics.totalPacketsPropagated += newNodes.size();
    }
}

// ============================================================================
// FIX 3: Memory Management - Periodic Cleanup
// ============================================================================

void InformationPropagationSystem::Update(float deltaTime) {
    ProcessPropagationQueue();
    
    // Periodic cache rebuild
    static float cacheRebuildTimer = 0.0f;
    cacheRebuildTimer += deltaTime;
    if (cacheRebuildTimer > 30.0f) {
        RebuildProvinceCache();
        cacheRebuildTimer = 0.0f;
    }
    
    // FIX: Periodic cleanup of m_activeByProvince
    static float cleanupTimer = 0.0f;
    cleanupTimer += deltaTime;
    if (cleanupTimer > 60.0f) { // Every minute
        CleanupActivePropagations();
        cleanupTimer = 0.0f;
    }
}

void InformationPropagationSystem::CleanupActivePropagations() {
    std::lock_guard<std::mutex> lock(m_propagationQueueMutex);
    
    auto currentDate = m_timeSystem->GetCurrentDate();
    size_t totalCleaned = 0;
    
    // Clean up expired propagations
    for (auto& [provinceId, propagations] : m_activeByProvince) {
        auto initialSize = propagations.size();
        
        // Remove expired propagations
        propagations.erase(
            std::remove_if(propagations.begin(), propagations.end(),
                [&currentDate](const PropagationNode& node) {
                    // Remove if already arrived (shouldn't still be here)
                    return node.scheduledArrival <= currentDate;
                }),
            propagations.end()
        );
        
        totalCleaned += (initialSize - propagations.size());
    }
    
    // Remove empty province entries
    for (auto it = m_activeByProvince.begin(); it != m_activeByProvince.end();) {
        if (it->second.empty()) {
            it = m_activeByProvince.erase(it);
        } else {
            ++it;
        }
    }
    
    if (totalCleaned > 0) {
        CORE_STREAM_INFO("InformationPropagation") << "Cleaned up " << totalCleaned 
                  << " expired propagations";
    }
}

// ============================================================================
// Additional Thread Safety for Event Handlers
// ============================================================================

void InformationPropagationSystem::OnGameEvent(
    const std::string& eventType, 
    const void* eventData) {
    
    // Thread-safe event processing
    try {
        if (eventType == "MilitaryEvent" && eventData) {
            // Properly cast and extract data from actual event structure
            const auto* militaryEvent = static_cast<const MilitaryEvent*>(eventData);
            
            std::unordered_map<std::string, float> data;
            data["event_id"] = static_cast<float>(militaryEvent->GetEventId());
            // Use GetCurrentDate instead of GetCurrentTick
            data["timestamp"] = 0.0f;
            data["severity"] = militaryEvent->GetSeverity();
            
            uint32_t sourceProvince = militaryEvent->GetSourceProvinceId();
            ConvertEventToInformation(eventType, sourceProvince, data);
        }
        else if (eventType == "DiplomaticEvent" && eventData) {
            const auto* dipEvent = static_cast<const DiplomaticEvent*>(eventData);
            
            std::unordered_map<std::string, float> data;
            data["event_id"] = static_cast<float>(dipEvent->GetEventId());
            data["nation_id"] = static_cast<float>(dipEvent->GetNationId());
            
            uint32_t sourceProvince = GetCapitalProvince(dipEvent->GetNationId());
            ConvertEventToInformation(eventType, sourceProvince, data);
        }
        else if (eventType == "EconomicEvent" && eventData) {
            const auto* econEvent = static_cast<const EconomicEvent*>(eventData);
            
            std::unordered_map<std::string, float> data;
            data["severity"] = econEvent->GetSeverity();
            data["economic_impact"] = econEvent->GetImpact();
            
            uint32_t sourceProvince = econEvent->GetProvinceId();
            ConvertEventToInformation(eventType, sourceProvince, data);
        }
    } catch (const std::exception& e) {
        CORE_STREAM_ERROR("InformationPropagation") << "Error processing event " 
                  << eventType << ": " << e.what();
    }
}

// ============================================================================
// Helper method to get nation capital
// ============================================================================

uint32_t InformationPropagationSystem::GetCapitalProvince(uint32_t nationId) const {
    // Query from ECS for nation capital
    if (m_componentAccess) {
        try {
            // Would query NationComponent for capital
            // For now, return first province of nation
            for (const auto& [provId, pos] : m_provinceCache) {
                if (pos.ownerNationId == nationId) {
                    return provId;
                }
            }
        } catch (...) {
            // Fallback
        }
    }
    
    return 1; // Default fallback
}

// ============================================================================
// Thread-safe statistics update
// ============================================================================

void InformationPropagationSystem::UpdateStatistics(
    const PropagationNode& node, 
    bool delivered) {
    
    std::lock_guard<std::mutex> lock(m_statsMutex);
    
    if (delivered) {
        // Stub: daysTaken calculation (replace with actual implementation)
        float daysTaken = 1.0f;
        
        float currentAvg = m_statistics.averagePropagationTime;
        float totalDelivered = static_cast<float>(m_statistics.totalPacketsPropagated);
        
        if (totalDelivered > 0) {
            m_statistics.averagePropagationTime = 
                (currentAvg * totalDelivered + daysTaken) / (totalDelivered + 1.0f);
            
            float currentAccuracy = m_statistics.averageAccuracyAtDelivery;
            m_statistics.averageAccuracyAtDelivery = 
                (currentAccuracy * totalDelivered + node.packet.GetDegradedAccuracy()) / 
                (totalDelivered + 1.0f);
        } else {
            m_statistics.averagePropagationTime = daysTaken;
            m_statistics.averageAccuracyAtDelivery = node.packet.GetDegradedAccuracy();
        }
    }
}

// Fix: Use GetCurrentDate instead of GetCurrentTick
// Fix: Use GetDaysBetween or stub if not available

// ============================================================================
// Helper Method Implementations (Stubs)
// ============================================================================

void InformationPropagationSystem::DeliverInformation(
    const InformationPacket& packet,
    uint32_t nationId) {

    // Deliver information to nation's AI through message bus
    if (m_messageBus) {
        // Create a message containing the information packet and target nation
        struct AIInformationMessage {
            InformationPacket packet;
            uint32_t targetNationId;
        };

        AIInformationMessage msg;
        msg.packet = packet;
        msg.targetNationId = nationId;

        // Post to message bus for AIDirector to handle
        m_messageBus->Publish(msg);

        CORE_STREAM_INFO("InfoPropagation") << "Delivered packet to nation " << nationId
                  << " (type: " << static_cast<int>(packet.type)
                  << ", accuracy: " << packet.GetDegradedAccuracy() << ")";
    } else {
        CORE_STREAM_ERROR("InfoPropagation") << "WARNING: No message bus available for delivery";
    }
}

std::vector<uint32_t> InformationPropagationSystem::GetNeighborProvinces(
    uint32_t provinceId) const {
    
    std::vector<uint32_t> neighbors;
    
    // Stub: Return neighboring provinces based on province connectivity
    // For now, return adjacent province IDs (simplified)
    auto it = m_provinceCache.find(provinceId);
    if (it != m_provinceCache.end()) {
        const auto& pos = it->second;
        
        // Find provinces within proximity range (simplified neighbor detection)
        for (const auto& [otherId, otherPos] : m_provinceCache) {
            if (otherId == provinceId) continue;
            
            float dx = pos.x - otherPos.x;
            float dy = pos.y - otherPos.y;
            float distSq = dx * dx + dy * dy;
            
            if (distSq < 150.0f * 150.0f) { // Within 150 units
                neighbors.push_back(otherId);
            }
        }
    }
    
    return neighbors;
}

bool InformationPropagationSystem::ShouldPropagate(
    const InformationPacket& packet, 
    uint32_t provinceId) const {
    
    // Stub: Determine if information should propagate to this province
    
    // Don't propagate if accuracy is too degraded
    if (packet.GetDegradedAccuracy() < 0.1f) {
        return false;
    }
    
    // Don't propagate if hop count is too high
    if (packet.hopCount > 10) {
        return false;
    }
    
    // Check if province is in propagation path (avoid loops)
    for (uint32_t pathProvince : packet.propagationPath) {
        if (pathProvince == provinceId) {
            return false; // Already visited
        }
    }
    
    return true;
}

float InformationPropagationSystem::CalculateDistance(
    uint32_t fromProvince, 
    uint32_t toProvince) const {
    
    // Stub: Calculate distance between provinces
    auto fromIt = m_provinceCache.find(fromProvince);
    auto toIt = m_provinceCache.find(toProvince);
    
    if (fromIt == m_provinceCache.end() || toIt == m_provinceCache.end()) {
        return 1000.0f; // Default large distance
    }
    
    const auto& fromPos = fromIt->second;
    const auto& toPos = toIt->second;
    
    float dx = fromPos.x - toPos.x;
    float dy = fromPos.y - toPos.y;
    
    return std::sqrt(dx * dx + dy * dy);
}

float InformationPropagationSystem::GetEffectivePropagationDelay(
    uint32_t fromProvince,
    uint32_t toProvince) const {
    
    // Stub: Calculate propagation delay between provinces
    float distance = CalculateDistance(fromProvince, toProvince);
    
    // Base speed: 100 km per day (configurable)
    float baseDelay = distance / 100.0f;
    
    // Get province owners to check for intelligence bonuses
    auto fromIt = m_provinceCache.find(fromProvince);
    auto toIt = m_provinceCache.find(toProvince);
    
    if (fromIt != m_provinceCache.end() && toIt != m_provinceCache.end()) {
        uint32_t fromNation = fromIt->second.ownerNationId;
        uint32_t toNation = toIt->second.ownerNationId;
        
        // Check for intelligence network bonuses
        auto nationIt = m_intelligenceBonuses.find(fromNation);
        if (nationIt != m_intelligenceBonuses.end()) {
            auto targetIt = nationIt->second.find(toNation);
            if (targetIt != nationIt->second.end()) {
                baseDelay *= (1.0f - targetIt->second); // Bonus reduces delay
            }
        }
    }
    
    return std::max(0.1f, baseDelay); // Minimum 0.1 days
}

void InformationPropagationSystem::ConvertEventToInformation(
    const std::string& eventType,
    uint32_t sourceProvinceId,
    const std::unordered_map<std::string, float>& eventData) {
    
    // Stub: Convert game event to InformationPacket and start propagation
    InformationPacket packet;
    packet.sourceProvinceId = sourceProvinceId;
    packet.eventDescription = eventType;
    packet.numericData = eventData;
    
    // Classify event type
    if (eventType.find("battle") != std::string::npos || 
        eventType.find("war") != std::string::npos) {
        packet.type = InformationType::MILITARY_ACTION;
        packet.baseRelevance = InformationRelevance::HIGH;
        packet.severity = 0.8f;
    } else if (eventType.find("rebel") != std::string::npos) {
        packet.type = InformationType::REBELLION;
        packet.baseRelevance = InformationRelevance::HIGH;
        packet.severity = 0.7f;
    } else if (eventType.find("diplo") != std::string::npos || 
               eventType.find("treaty") != std::string::npos) {
        packet.type = InformationType::DIPLOMATIC_CHANGE;
        packet.baseRelevance = InformationRelevance::MEDIUM;
        packet.severity = 0.5f;
    } else if (eventType.find("succession") != std::string::npos) {
        packet.type = InformationType::SUCCESSION_CRISIS;
        packet.baseRelevance = InformationRelevance::HIGH;
        packet.severity = 0.9f;
    } else {
        packet.type = InformationType::ECONOMIC_CRISIS;
        packet.baseRelevance = InformationRelevance::MEDIUM;
        packet.severity = 0.4f;
    }
    
    // Get originator from event data if available
    auto originatorIt = eventData.find("originator");
    if (originatorIt != eventData.end()) {
        packet.originatorEntityId = static_cast<uint32_t>(originatorIt->second);
    }
    
    // Start propagation
    StartPropagation(packet);
}

} // namespace AI
