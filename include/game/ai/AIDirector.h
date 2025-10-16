// Created: September 25, 2025, 11:45 AM
// Location: include/game/ai/AIDirector.h

#ifndef AI_DIRECTOR_H
#define AI_DIRECTOR_H

#include "game/ai/InformationPropagationSystem.h"
#include "game/ai/AIAttentionManager.h"
#include "core/ECS/MessageBus.h"
#include "core/ECS/ComponentAccessManager.h"
#include "core/threading/ThreadedSystemManager.h"
#include "core/types/game_types.h"

#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <queue>
#include <chrono>

namespace AI {

// Forward declarations
class NationAI;
class CharacterAI;
class CouncilAI;

// ============================================================================
// AI Message Queue with Priority
// ============================================================================

enum class MessagePriority : uint8_t {
    CRITICAL = 0,   // Immediate processing
    HIGH = 1,       // 1-3 day delay
    MEDIUM = 2,     // 1-2 week delay
    LOW = 3,        // When idle
    COUNT
};

struct AIMessage {
    std::unique_ptr<InformationPacket> information;
    uint32_t targetActorId;
    bool isNationActor;
    MessagePriority priority;
    std::chrono::system_clock::time_point scheduledProcessing;
    std::chrono::system_clock::time_point receivedTime;
    
    bool operator>(const AIMessage& other) const {
        if (priority != other.priority) {
            return priority > other.priority;
        }
        return scheduledProcessing > other.scheduledProcessing;
    }
};

class AIMessageQueue {
private:
    std::array<std::queue<AIMessage>, 4> m_priorityQueues;
    mutable std::mutex m_queueMutex;
    std::condition_variable m_dataAvailable;
    
    // Statistics
    std::atomic<uint64_t> m_totalMessages{0};
    std::atomic<uint64_t> m_processedMessages{0};
    std::array<std::atomic<uint64_t>, 4> m_messagesByPriority;
    
public:
    AIMessageQueue();
    
    void PushMessage(AIMessage&& message);
    bool PopMessage(AIMessage& message, std::chrono::milliseconds timeout);
    bool HasMessages() const;
    size_t GetQueueSize() const;
    size_t GetQueueSize(MessagePriority priority) const;
    
    // Statistics
    uint64_t GetTotalMessages() const { return m_totalMessages.load(); }
    uint64_t GetProcessedMessages() const { return m_processedMessages.load(); }
    void ResetStatistics();
};

// ============================================================================
// AI Director - Master Coordinator
// ============================================================================

enum class AIDirectorState {
    STOPPED,
    INITIALIZING,
    RUNNING,
    PAUSED,
    SHUTTING_DOWN
};

class AIDirector {
private:
    // Core systems
    std::shared_ptr<ECS::ComponentAccessManager> m_componentAccess;
    std::shared_ptr<core::threading::ThreadSafeMessageBus> m_messageBus;
    std::shared_ptr<InformationPropagationSystem> m_propagationSystem;
    std::shared_ptr<AIAttentionManager> m_attentionManager;
    
    // AI Actors
    std::unordered_map<uint32_t, std::unique_ptr<NationAI>> m_nationActors;
    std::unordered_map<uint32_t, std::unique_ptr<CharacterAI>> m_characterActors;
    std::unordered_map<uint32_t, std::unique_ptr<CouncilAI>> m_councilActors;
    mutable std::mutex m_actorMutex;
    
    // Message queues per actor
    std::unordered_map<uint32_t, std::unique_ptr<AIMessageQueue>> m_actorQueues;
    
    // Dedicated thread management
    std::thread m_workerThread;
    std::atomic<AIDirectorState> m_state{AIDirectorState::STOPPED};
    std::atomic<bool> m_shouldStop{false};
    std::condition_variable m_stateCondition;
    mutable std::mutex m_stateMutex;
    
    // Performance configuration
    std::atomic<uint32_t> m_maxActorsPerFrame{10};
    std::atomic<uint32_t> m_maxMessagesPerActor{5};
    std::atomic<float> m_targetFrameTime{16.67f}; // 60 FPS target
    
    // Performance tracking
    struct PerformanceMetrics {
        std::atomic<uint64_t> totalDecisions{0};
        std::atomic<uint64_t> totalFrames{0};
        std::atomic<double> averageDecisionTime{0.0};
        std::atomic<double> averageFrameTime{0.0};
        std::atomic<uint32_t> activeActors{0};
        std::chrono::steady_clock::time_point lastUpdate;
    } m_metrics;
    
    // Background processing queue
    std::queue<std::function<void()>> m_backgroundTasks;
    mutable std::mutex m_backgroundMutex;
    
public:
    AIDirector(
        std::shared_ptr<ECS::ComponentAccessManager> componentAccess,
        std::shared_ptr<core::threading::ThreadSafeMessageBus> messageBus
    );
    ~AIDirector();
    
    // System lifecycle
    void Initialize();
    void Start();
    void Stop();
    void Pause();
    void Resume();
    void Shutdown();
    
    // System setup
    void SetPropagationSystem(std::shared_ptr<InformationPropagationSystem> system);
    void SetAttentionManager(std::shared_ptr<AIAttentionManager> manager);
    
    // Actor management
    uint32_t CreateNationAI(
        types::EntityID realmId,
        const std::string& name,
        CharacterArchetype personality
    );
    
    uint32_t CreateCharacterAI(
        types::EntityID characterId,
        const std::string& name,
        CharacterArchetype archetype
    );
    
    uint32_t CreateCouncilAI(
        types::EntityID realmId,
        const std::string& realmName
    );
    
    bool DestroyActor(uint32_t actorId);
    
    // Information delivery
    void DeliverInformation(
        const InformationPacket& packet,
        uint32_t actorId,
        MessagePriority priority
    );
    
    void BroadcastInformation(
        const InformationPacket& packet
    );
    
    // Configuration
    void SetMaxActorsPerFrame(uint32_t max) { m_maxActorsPerFrame = max; }
    void SetMaxMessagesPerActor(uint32_t max) { m_maxMessagesPerActor = max; }
    void SetTargetFrameTime(float ms) { m_targetFrameTime = ms; }
    
    // Queries
    AIDirectorState GetState() const { return m_state.load(); }
    bool IsRunning() const { return m_state == AIDirectorState::RUNNING; }
    uint32_t GetActiveActorCount() const;
    uint32_t GetTotalQueuedMessages() const;
    
    // Public snapshot of performance metrics (copyable)
    struct PerformanceMetricsSnapshot {
        uint64_t totalDecisions{0};
        uint64_t totalFrames{0};
        double averageDecisionTime{0.0};
        double averageFrameTime{0.0};
        uint32_t activeActors{0};
        std::chrono::steady_clock::time_point lastUpdate;
    };

    // Return a snapshot of current metrics (thread-safe)
    PerformanceMetricsSnapshot GetMetrics() const;
    std::vector<std::string> GetPerformanceReport() const;
    void ResetMetrics();
    
    // Debug
    void DumpActorList() const;
    void DumpQueueStatistics() const;
    
private:
    // Main worker thread function
    void WorkerThreadMain();
    
    // Processing functions
    void ProcessFrame();
    void ProcessActorMessages(uint32_t actorId, uint32_t maxMessages);
    void ProcessBackgroundTasks();
    
    // Event-driven processing
    void OnInformationReceived(const InformationPacket& packet);
    void RouteInformationToActors(const InformationPacket& packet);
    
    // Actor execution
    void ExecuteNationAI(NationAI* nation, const AIMessage& message);
    void ExecuteCharacterAI(CharacterAI* character, const AIMessage& message);
    void ExecuteCouncilAI(CouncilAI* council, const AIMessage& message);
    
    // Background AI updates
    void UpdateNationBackground(NationAI* nation);
    void UpdateCharacterBackground(CharacterAI* character);
    
    // Load balancing
    std::vector<uint32_t> SelectActorsForProcessing();
    void BalanceActorLoad();
    
    // Utility
    bool IsNationActor(uint32_t actorId) const;
    bool IsCharacterActor(uint32_t actorId) const;
    bool IsCouncilActor(uint32_t actorId) const;
    
    AIMessageQueue* GetActorQueue(uint32_t actorId);
    void CreateActorQueue(uint32_t actorId);
    
    // Performance tracking
    void UpdateMetrics(double frameTime, uint32_t decisionsThisFrame);
};

// ============================================================================
// AI Coordinator Interface (for external systems)
// ============================================================================

class AICoordinator {
private:
    std::shared_ptr<AIDirector> m_director;
    std::shared_ptr<InformationPropagationSystem> m_propagation;
    std::shared_ptr<AIAttentionManager> m_attention;
    
public:
    AICoordinator(
        std::shared_ptr<ECS::ComponentAccessManager> componentAccess,
        std::shared_ptr<core::threading::ThreadSafeMessageBus> messageBus
    );
    
    void Initialize();
    void Start();
    void Stop();
    void Shutdown();
    
    // High-level AI creation
    void CreateAIForRealm(
        types::EntityID realmId,
        const std::string& realmName,
        CharacterArchetype rulerPersonality
    );
    
    void CreateAIForCharacter(
        types::EntityID characterId,
        const std::string& name,
        CharacterArchetype archetype
    );
    
    // Event injection
    void NotifyWarDeclaration(
        types::EntityID aggressor,
        types::EntityID defender
    );
    
    void NotifyDiplomaticChange(
        types::EntityID realm1,
        types::EntityID realm2,
        const std::string& changeType
    );
    
    void NotifyEconomicEvent(
        types::EntityID provinceId,
        float severity,
        const std::string& description
    );
    
    // Performance monitoring
    std::string GetSystemStatus() const;
    void OptimizePerformance();
};

} // namespace AI

#endif // AI_DIRECTOR_H
