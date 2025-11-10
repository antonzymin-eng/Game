// Created: September 25, 2025, 11:45 AM
// Updated: October 30, 2025 - Applied critical fixes for deadlocks and namespace issues
// Updated: November 10, 2025 - CRITICAL FIX: Changed from BACKGROUND_THREAD to MAIN_THREAD strategy
//                               Removed dedicated worker thread to eliminate race conditions
//                               with shared game state access (ComponentAccessManager)
// Location: src/game/ai/AIDirector.cpp

#include "game/ai/AIDirector.h"
#include "game/ai/NationAI.h"
#include "game/ai/CharacterAI.h"
#include "game/ai/CouncilAI.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace AI {

// ============================================================================
// AIMessageQueue Implementation
// ============================================================================

AIMessageQueue::AIMessageQueue() {
    for (auto& counter : m_messagesByPriority) {
        counter.store(0);
    }
}

void AIMessageQueue::PushMessage(AIMessage&& message) {
    std::lock_guard<std::mutex> lock(m_queueMutex);

    uint8_t priorityIndex = static_cast<uint8_t>(message.priority);
    m_priorityQueues[priorityIndex].push(std::move(message));

    m_totalMessages.fetch_add(1);
    m_messagesByPriority[priorityIndex].fetch_add(1);

    m_dataAvailable.notify_one();
}

// FIX 1: Deadlock fix - use inline lambda instead of calling HasMessages()
bool AIMessageQueue::PopMessage(AIMessage& message, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(m_queueMutex);

    // FIXED: Inline the check to avoid deadlock - don't call HasMessages()
    auto hasAnyMessages = [this]() {
        for (const auto& queue : m_priorityQueues) {
            if (!queue.empty()) return true;
        }
        return false;
    };

    if (!m_dataAvailable.wait_for(lock, timeout, hasAnyMessages)) {
        return false; // Timeout
    }

    // Get highest priority message
    for (int p = 0; p < static_cast<int>(MessagePriority::COUNT); ++p) {
        if (!m_priorityQueues[p].empty()) {
            message = std::move(m_priorityQueues[p].front());
            m_priorityQueues[p].pop();
            m_processedMessages.fetch_add(1);
            return true;
        }
    }

    return false;
}

// FIX 2: Add thread safety to HasMessages()
bool AIMessageQueue::HasMessages() const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    for (const auto& queue : m_priorityQueues) {
        if (!queue.empty()) return true;
    }
    return false;
}

size_t AIMessageQueue::GetQueueSize() const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    size_t total = 0;
    for (const auto& queue : m_priorityQueues) {
        total += queue.size();
    }
    return total;
}

size_t AIMessageQueue::GetQueueSize(MessagePriority priority) const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    uint8_t index = static_cast<uint8_t>(priority);
    return m_priorityQueues[index].size();
}

void AIMessageQueue::ResetStatistics() {
    m_totalMessages.store(0);
    m_processedMessages.store(0);
    for (auto& counter : m_messagesByPriority) {
        counter.store(0);
    }
}

// ============================================================================
// AIDirector Implementation
// ============================================================================

AIDirector::AIDirector(
    std::shared_ptr<core::ecs::ComponentAccessManager> componentAccess,
    std::shared_ptr<core::threading::ThreadSafeMessageBus> messageBus)
    : m_componentAccess(componentAccess)
    , m_messageBus(messageBus) {
}

// FIX 8: Add GetMetrics() implementation
AIDirector::PerformanceMetricsSnapshot AIDirector::GetMetrics() const {
    PerformanceMetricsSnapshot snap;
    snap.totalDecisions = m_metrics.totalDecisions.load();
    snap.totalFrames = m_metrics.totalFrames.load();
    snap.averageDecisionTime = m_metrics.averageDecisionTime.load();
    snap.averageFrameTime = m_metrics.averageFrameTime.load();
    snap.activeActors = m_metrics.activeActors.load();
    snap.lastUpdate = m_metrics.lastUpdate;
    return snap;
}

AIDirector::~AIDirector() {
    Shutdown();
}

void AIDirector::Initialize() {
    std::cout << "[AIDirector] Initializing AI Director system" << std::endl;

    m_state = AIDirectorState::INITIALIZING;

    // Initialize subsystems
    if (m_propagationSystem) {
        m_propagationSystem->Initialize();
    }

    if (m_attentionManager) {
        m_attentionManager->Initialize();
    }

    // Reset metrics
    ResetMetrics();

    m_state = AIDirectorState::STOPPED;

    std::cout << "[AIDirector] Initialization complete" << std::endl;
}

void AIDirector::Start() {
    if (m_state != AIDirectorState::STOPPED) {
        std::cerr << "[AIDirector] Cannot start - not in stopped state" << std::endl;
        return;
    }

    std::cout << "[AIDirector] Starting AI Director (MAIN_THREAD strategy)" << std::endl;

    m_shouldStop.store(false);
    m_state = AIDirectorState::RUNNING;

    // NO dedicated worker thread - Update() will be called from main thread

    std::cout << "[AIDirector] AI Director started - will run on main thread" << std::endl;
}

void AIDirector::Stop() {
    if (m_state != AIDirectorState::RUNNING && m_state != AIDirectorState::PAUSED) {
        return;
    }

    std::cout << "[AIDirector] Stopping AI Director" << std::endl;

    m_shouldStop.store(true);
    m_state = AIDirectorState::SHUTTING_DOWN;

    // No worker thread to join - runs on main thread

    m_state = AIDirectorState::STOPPED;

    std::cout << "[AIDirector] AI Director stopped" << std::endl;
}

void AIDirector::Pause() {
    if (m_state == AIDirectorState::RUNNING) {
        m_state = AIDirectorState::PAUSED;
        std::cout << "[AIDirector] AI Director paused" << std::endl;
    }
}

void AIDirector::Resume() {
    if (m_state == AIDirectorState::PAUSED) {
        m_state = AIDirectorState::RUNNING;
        std::cout << "[AIDirector] AI Director resumed" << std::endl;
    }
}

void AIDirector::Shutdown() {
    Stop();

    std::lock_guard<std::mutex> lock(m_actorMutex);

    m_nationActors.clear();
    m_characterActors.clear();
    m_councilActors.clear();
    m_actorQueues.clear();

    std::cout << "[AIDirector] AI Director shutdown complete" << std::endl;
}

// ============================================================================
// Main Thread Update (MAIN_THREAD strategy)
// ============================================================================

void AIDirector::Update(float deltaTime) {
    // Only process if running (not paused or stopped)
    if (m_state != AIDirectorState::RUNNING) {
        return;
    }

    // Process one frame of AI updates on MAIN_THREAD
    // This eliminates race conditions with shared game state
    ProcessFrame();
}

// ============================================================================
// System Setup
// ============================================================================

void AIDirector::SetPropagationSystem(std::shared_ptr<InformationPropagationSystem> system) {
    m_propagationSystem = system;
}

void AIDirector::SetAttentionManager(std::shared_ptr<AIAttentionManager> manager) {
    m_attentionManager = manager;
}

// ============================================================================
// Actor Management
// ============================================================================

uint32_t AIDirector::CreateNationAI(
    types::EntityID realmId,
    const std::string& name,
    CharacterArchetype personality) {

    static std::atomic<uint32_t> s_nextActorId{1000}; // Nation IDs start at 1000
    uint32_t actorId = s_nextActorId.fetch_add(1);

    std::lock_guard<std::mutex> lock(m_actorMutex);

    // FIX 3: Fully qualify namespace with global scope
    auto nationAI = std::make_unique<::game::ai::NationAI>(actorId, realmId, name, personality);
    m_nationActors[actorId] = std::move(nationAI);

    // Register with attention manager
    if (m_attentionManager) {
        m_attentionManager->RegisterNationActor(actorId, name, personality);
    }

    // Create message queue
    CreateActorQueue(actorId);

    m_metrics.activeActors.fetch_add(1);

    std::cout << "[AIDirector] Created NationAI: " << name
              << " (Actor ID: " << actorId << ")" << std::endl;

    return actorId;
}

uint32_t AIDirector::CreateCharacterAI(
    types::EntityID characterId,
    const std::string& name,
    CharacterArchetype archetype) {

    static std::atomic<uint32_t> s_nextActorId{5000}; // Character IDs start at 5000
    uint32_t actorId = s_nextActorId.fetch_add(1);

    std::lock_guard<std::mutex> lock(m_actorMutex);

    // CharacterAI is in AI namespace (same as AIDirector)
    auto characterAI = std::make_unique<CharacterAI>(actorId, characterId, name, archetype);
    m_characterActors[actorId] = std::move(characterAI);

    // Register with attention manager
    if (m_attentionManager) {
        m_attentionManager->RegisterCharacterActor(actorId, name, archetype);
    }

    // Create message queue
    CreateActorQueue(actorId);

    m_metrics.activeActors.fetch_add(1);

    std::cout << "[AIDirector] Created CharacterAI: " << name
              << " (Actor ID: " << actorId << ")" << std::endl;

    return actorId;
}

uint32_t AIDirector::CreateCouncilAI(
    types::EntityID realmId,
    const std::string& realmName) {

    // TODO: CouncilAI not yet implemented
    std::cerr << "[AIDirector] ERROR: CouncilAI not yet implemented" << std::endl;
    return 0;

    /* UNCOMMENT when CouncilAI is ready:
    static std::atomic<uint32_t> s_nextActorId{9000}; // Council IDs start at 9000
    uint32_t actorId = s_nextActorId.fetch_add(1);

    std::lock_guard<std::mutex> lock(m_actorMutex);

    // Create council AI instance
    auto councilAI = std::make_unique<CouncilAI>(actorId, realmId, realmName + " Council");
    m_councilActors[actorId] = std::move(councilAI);

    // Council doesn't need attention manager registration (processes differently)

    // Create message queue
    CreateActorQueue(actorId);

    m_metrics.activeActors.fetch_add(1);

    std::cout << "[AIDirector] Created CouncilAI for " << realmName
              << " (Actor ID: " << actorId << ")" << std::endl;

    return actorId;
    */
}

bool AIDirector::DestroyActor(uint32_t actorId) {
    std::lock_guard<std::mutex> lock(m_actorMutex);

    bool destroyed = false;

    // Remove from appropriate collection
    if (m_nationActors.erase(actorId) > 0) {
        destroyed = true;
    } else if (m_characterActors.erase(actorId) > 0) {
        destroyed = true;
    } else if (m_councilActors.erase(actorId) > 0) {
        destroyed = true;
    }

    if (destroyed) {
        // Remove message queue
        m_actorQueues.erase(actorId);

        // Unregister from attention manager
        if (m_attentionManager) {
            m_attentionManager->UnregisterActor(actorId, IsNationActor(actorId));
        }

        m_metrics.activeActors.fetch_sub(1);

        std::cout << "[AIDirector] Destroyed actor: " << actorId << std::endl;
    }

    return destroyed;
}

// ============================================================================
// Information Delivery
// ============================================================================

void AIDirector::DeliverInformation(
    const InformationPacket& packet,
    uint32_t actorId,
    MessagePriority priority) {

    auto* queue = GetActorQueue(actorId);
    if (!queue) return;

    AIMessage message;
    message.information = std::make_unique<InformationPacket>(packet);
    message.targetActorId = actorId;
    message.isNationActor = IsNationActor(actorId);
    message.priority = priority;
    message.receivedTime = std::chrono::system_clock::now();

    // Calculate scheduled processing time based on priority
    switch (priority) {
        case MessagePriority::CRITICAL:
            message.scheduledProcessing = message.receivedTime;
            break;
        case MessagePriority::HIGH:
            message.scheduledProcessing = message.receivedTime + std::chrono::hours(24);
            break;
        case MessagePriority::MEDIUM:
            message.scheduledProcessing = message.receivedTime + std::chrono::hours(24 * 7);
            break;
        case MessagePriority::LOW:
            message.scheduledProcessing = message.receivedTime + std::chrono::hours(24 * 14);
            break;
        default:
            message.scheduledProcessing = message.receivedTime;
    }

    queue->PushMessage(std::move(message));
}

void AIDirector::BroadcastInformation(const InformationPacket& packet) {
    // Route through attention manager to filter interested actors
    RouteInformationToActors(packet);
}

// ============================================================================
// Worker Thread Implementation
// ============================================================================

// REMOVED: WorkerThreadMain() - No longer needed with MAIN_THREAD strategy
// AIDirector now runs on main thread via Update() method

// ProcessFrame() - Now runs on MAIN_THREAD (called from Update())
void AIDirector::ProcessFrame() {
    uint32_t decisionsThisFrame = 0;
    auto frameStart = std::chrono::steady_clock::now();

    // 1. Process high-priority messages first
    {
        std::lock_guard<std::mutex> lock(m_actorMutex);

        // Process critical messages immediately
        for (const auto& [actorId, queue] : m_actorQueues) {
            if (queue->GetQueueSize(MessagePriority::CRITICAL) > 0) {
                // FIX 4: Capture returned count
                uint32_t processed = ProcessActorMessages(actorId, 1);
                decisionsThisFrame += processed;
            }
        }
    }

    // 2. Select actors for regular processing
    auto selectedActors = SelectActorsForProcessing();
    uint32_t maxActors = m_maxActorsPerFrame.load();
    uint32_t processedActors = 0;

    for (uint32_t actorId : selectedActors) {
        if (processedActors >= maxActors) break;

        uint32_t maxMessages = m_maxMessagesPerActor.load();

        // FIX 4: Capture returned count
        uint32_t messagesProcessed = ProcessActorMessages(actorId, maxMessages);
        decisionsThisFrame += messagesProcessed;
        processedActors++;
    }

    // 3. Process background tasks when idle
    if (decisionsThisFrame < maxActors / 2) {
        ProcessBackgroundTasks();
    }

    // 4. Periodic load balancing
    static uint32_t frameCounter = 0;
    if (++frameCounter % 300 == 0) { // Every 5 seconds at 60 FPS
        BalanceActorLoad();
    }

    auto frameEnd = std::chrono::steady_clock::now();
    auto frameDuration = std::chrono::duration<double, std::milli>(frameEnd - frameStart).count();

    UpdateMetrics(frameDuration, decisionsThisFrame);
    m_metrics.totalFrames.fetch_add(1);
    m_metrics.averageFrameTime.store(frameDuration);
}

// FIX 4: Return count of messages processed
uint32_t AIDirector::ProcessActorMessages(uint32_t actorId, uint32_t maxMessages) {
    auto* queue = GetActorQueue(actorId);
    if (!queue) return 0;

    uint32_t processed = 0;
    AIMessage message;

    while (processed < maxMessages && queue->PopMessage(message, std::chrono::milliseconds(0))) {
        // Check if message is ready for processing
        auto now = std::chrono::system_clock::now();
        if (message.scheduledProcessing > now) {
            // Not ready yet, push back
            queue->PushMessage(std::move(message));
            continue;
        }

        // Execute appropriate AI type
        std::lock_guard<std::mutex> lock(m_actorMutex);

        if (IsNationActor(actorId)) {
            auto it = m_nationActors.find(actorId);
            if (it != m_nationActors.end()) {
                ExecuteNationAI(it->second.get(), message);
            }
        } else if (IsCharacterActor(actorId)) {
            auto it = m_characterActors.find(actorId);
            if (it != m_characterActors.end()) {
                ExecuteCharacterAI(it->second.get(), message);
            }
        } else if (IsCouncilActor(actorId)) {
            auto it = m_councilActors.find(actorId);
            if (it != m_councilActors.end()) {
                ExecuteCouncilAI(it->second.get(), message);
            }
        }

        processed++;
        m_metrics.totalDecisions.fetch_add(1);
    }

    return processed;
}

// FIX 5: Separate lock acquisition to avoid deadlock
void AIDirector::ProcessBackgroundTasks() {
    // Execute existing tasks first (hold background mutex only)
    {
        std::lock_guard<std::mutex> lock(m_backgroundMutex);

        int tasksProcessed = 0;
        while (!m_backgroundTasks.empty() && tasksProcessed < 5) {
            auto task = m_backgroundTasks.front();
            m_backgroundTasks.pop();

            task(); // Execute background task
            tasksProcessed++;
        }
    } // Release background mutex

    // Now schedule new tasks (requires actor mutex)
    std::vector<std::function<void()>> newTasks;

    {
        std::lock_guard<std::mutex> actorLock(m_actorMutex);

        // Update a few nation AIs
        int nationsUpdated = 0;
        for (const auto& [actorId, nationAI] : m_nationActors) {
            if (nationsUpdated >= 2) break;

            newTasks.push_back([this, nation = nationAI.get()]() {
                UpdateNationBackground(nation);
            });
            nationsUpdated++;
        }

        // Update a few character AIs
        int charactersUpdated = 0;
        for (const auto& [actorId, characterAI] : m_characterActors) {
            if (charactersUpdated >= 3) break;

            newTasks.push_back([this, character = characterAI.get()]() {
                UpdateCharacterBackground(character);
            });
            charactersUpdated++;
        }
    } // Release actor mutex

    // Add new tasks (hold background mutex again)
    {
        std::lock_guard<std::mutex> lock(m_backgroundMutex);
        for (auto& task : newTasks) {
            m_backgroundTasks.push(std::move(task));
        }
    }
}

// ============================================================================
// Event-driven Processing
// ============================================================================

void AIDirector::OnInformationReceived(const InformationPacket& packet) {
    RouteInformationToActors(packet);
}

void AIDirector::RouteInformationToActors(const InformationPacket& packet) {
    if (!m_attentionManager) return;

    // Get all interested actors from attention manager
    auto interestedActors = m_attentionManager->GetInterestedActors(packet);

    for (uint32_t actorId : interestedActors) {
        // Get attention result for priority determination
        AttentionResult attention = m_attentionManager->FilterInformation(
            packet, actorId, IsNationActor(actorId)
        );

        if (!attention.shouldReceive) continue;

        // Map attention relevance to message priority
        MessagePriority priority = MessagePriority::LOW;
        switch (attention.adjustedRelevance) {
            case InformationRelevance::CRITICAL:
                priority = MessagePriority::CRITICAL;
                break;
            case InformationRelevance::HIGH:
                priority = MessagePriority::HIGH;
                break;
            case InformationRelevance::MEDIUM:
                priority = MessagePriority::MEDIUM;
                break;
            default:
                priority = MessagePriority::LOW;
                break;
        }

        // Deliver to actor
        DeliverInformation(packet, actorId, priority);
    }
}

// ============================================================================
// Actor Execution
// ============================================================================

// FIX 6: Fully qualify namespace
void AIDirector::ExecuteNationAI(::game::ai::NationAI* nation, const AIMessage& message) {
    if (!nation || !message.information) return;

    // Nation AI processes strategic information
    nation->ProcessInformation(*message.information);

    // Update last activity
    nation->SetLastActivityTime(std::chrono::system_clock::now());
}

void AIDirector::ExecuteCharacterAI(CharacterAI* character, const AIMessage& message) {
    if (!character || !message.information) return;

    // Character AI processes personal information
    character->ProcessInformation(*message.information);

    // Update last activity
    character->SetLastActivityTime(std::chrono::system_clock::now());
}

void AIDirector::ExecuteCouncilAI(CouncilAI* council, const AIMessage& message) {
    // TODO: CouncilAI not yet implemented
    (void)council;
    (void)message;
    std::cerr << "[AIDirector] ExecuteCouncilAI called but CouncilAI not implemented" << std::endl;
}

// FIX 6: Fully qualify namespace
void AIDirector::UpdateNationBackground(::game::ai::NationAI* nation) {
    if (!nation) return;

    // Background updates for nations (economy, diplomacy checks)
    nation->UpdateEconomy();
    nation->UpdateDiplomacy();
    nation->UpdateMilitary();
}

void AIDirector::UpdateCharacterBackground(CharacterAI* character) {
    if (!character) return;

    // Background updates for characters (ambitions, relationships)
    character->UpdateAmbitions();
    character->UpdateRelationships();
}

// ============================================================================
// Load Balancing
// ============================================================================

std::vector<uint32_t> AIDirector::SelectActorsForProcessing() {
    std::vector<uint32_t> selected;

    std::lock_guard<std::mutex> lock(m_actorMutex);

    // Priority 1: Actors with critical messages
    for (const auto& [actorId, queue] : m_actorQueues) {
        if (queue->GetQueueSize(MessagePriority::CRITICAL) > 0) {
            selected.push_back(actorId);
        }
    }

    // Priority 2: Actors with high priority messages
    for (const auto& [actorId, queue] : m_actorQueues) {
        if (queue->GetQueueSize(MessagePriority::HIGH) > 0) {
            selected.push_back(actorId);
        }
    }

    // Priority 3: Nations (strategic importance)
    for (const auto& [actorId, nation] : m_nationActors) {
        if (std::find(selected.begin(), selected.end(), actorId) == selected.end()) {
            selected.push_back(actorId);
        }
    }

    // Priority 4: Characters
    for (const auto& [actorId, character] : m_characterActors) {
        if (std::find(selected.begin(), selected.end(), actorId) == selected.end()) {
            selected.push_back(actorId);
        }
    }

    return selected;
}

void AIDirector::BalanceActorLoad() {
    // Analyze queue depths and adjust processing priorities
    std::lock_guard<std::mutex> lock(m_actorMutex);

    uint32_t totalQueued = 0;
    uint32_t maxQueueDepth = 0;
    uint32_t overloadedActors = 0;

    for (const auto& [actorId, queue] : m_actorQueues) {
        size_t queueSize = queue->GetQueueSize();
        totalQueued += queueSize;
        maxQueueDepth = std::max(maxQueueDepth, static_cast<uint32_t>(queueSize));

        if (queueSize > 50) {
            overloadedActors++;
        }
    }

    // Adjust processing rates if overloaded
    if (overloadedActors > 5) {
        // Increase actors per frame
        uint32_t current = m_maxActorsPerFrame.load();
        m_maxActorsPerFrame.store(std::min(20u, current + 2));
    } else if (overloadedActors == 0 && totalQueued < 100) {
        // Decrease actors per frame to save CPU
        uint32_t current = m_maxActorsPerFrame.load();
        m_maxActorsPerFrame.store(std::max(5u, current - 1));
    }
}

// ============================================================================
// Utility Methods
// ============================================================================

bool AIDirector::IsNationActor(uint32_t actorId) const {
    return actorId >= 1000 && actorId < 5000;
}

bool AIDirector::IsCharacterActor(uint32_t actorId) const {
    return actorId >= 5000 && actorId < 9000;
}

bool AIDirector::IsCouncilActor(uint32_t actorId) const {
    return actorId >= 9000;
}

AIMessageQueue* AIDirector::GetActorQueue(uint32_t actorId) {
    std::lock_guard<std::mutex> lock(m_actorMutex);

    auto it = m_actorQueues.find(actorId);
    if (it != m_actorQueues.end()) {
        return it->second.get();
    }
    return nullptr;
}

void AIDirector::CreateActorQueue(uint32_t actorId) {
    m_actorQueues[actorId] = std::make_unique<AIMessageQueue>();
}

// ============================================================================
// Statistics and Debugging
// ============================================================================

void AIDirector::UpdateMetrics(double frameTime, uint32_t decisionsThisFrame) {
    m_metrics.totalDecisions.fetch_add(decisionsThisFrame);

    // Update average decision time
    uint64_t totalDecisions = m_metrics.totalDecisions.load();
    if (totalDecisions > 0 && decisionsThisFrame > 0) {
        double avgDecisionTime = frameTime / decisionsThisFrame;
        m_metrics.averageDecisionTime.store(avgDecisionTime);
    }

    // Update average frame time with exponential moving average
    double currentAvg = m_metrics.averageFrameTime.load();
    double alpha = 0.1; // Smoothing factor
    double newAvg = alpha * frameTime + (1.0 - alpha) * currentAvg;
    m_metrics.averageFrameTime.store(newAvg);

    m_metrics.lastUpdate = std::chrono::steady_clock::now();
}

uint32_t AIDirector::GetActiveActorCount() const {
    return m_metrics.activeActors.load();
}

uint32_t AIDirector::GetTotalQueuedMessages() const {
    std::lock_guard<std::mutex> lock(m_actorMutex);

    uint32_t total = 0;
    for (const auto& [actorId, queue] : m_actorQueues) {
        total += queue->GetQueueSize();
    }
    return total;
}

std::vector<std::string> AIDirector::GetPerformanceReport() const {
    std::vector<std::string> report;

    std::stringstream ss;

    ss << "=== AI Director Performance ===";
    report.push_back(ss.str());
    ss.str("");

    ss << "State: ";
    switch (m_state.load()) {
        case AIDirectorState::RUNNING: ss << "RUNNING"; break;
        case AIDirectorState::PAUSED: ss << "PAUSED"; break;
        case AIDirectorState::STOPPED: ss << "STOPPED"; break;
        default: ss << "UNKNOWN"; break;
    }
    report.push_back(ss.str());
    ss.str("");

    ss << "Active Actors: " << GetActiveActorCount();
    report.push_back(ss.str());
    ss.str("");

    ss << "Total Decisions: " << m_metrics.totalDecisions.load();
    report.push_back(ss.str());
    ss.str("");

    ss << "Total Frames: " << m_metrics.totalFrames.load();
    report.push_back(ss.str());
    ss.str("");

    ss << std::fixed << std::setprecision(2);
    ss << "Avg Decision Time: " << m_metrics.averageDecisionTime.load() << " ms";
    report.push_back(ss.str());
    ss.str("");

    ss << "Avg Frame Time: " << m_metrics.averageFrameTime.load() << " ms";
    report.push_back(ss.str());
    ss.str("");

    ss << "Target Frame Time: " << m_targetFrameTime.load() << " ms";
    report.push_back(ss.str());
    ss.str("");

    ss << "Queued Messages: " << GetTotalQueuedMessages();
    report.push_back(ss.str());
    ss.str("");

    ss << "Max Actors/Frame: " << m_maxActorsPerFrame.load();
    report.push_back(ss.str());
    ss.str("");

    ss << "Max Messages/Actor: " << m_maxMessagesPerActor.load();
    report.push_back(ss.str());

    return report;
}

void AIDirector::ResetMetrics() {
    m_metrics.totalDecisions.store(0);
    m_metrics.totalFrames.store(0);
    m_metrics.averageDecisionTime.store(0.0);
    m_metrics.averageFrameTime.store(0.0);
    m_metrics.lastUpdate = std::chrono::steady_clock::now();
}

void AIDirector::DumpActorList() const {
    std::lock_guard<std::mutex> lock(m_actorMutex);

    std::cout << "=== AI Actor List ===" << std::endl;
    std::cout << "Nations: " << m_nationActors.size() << std::endl;
    for (const auto& [id, nation] : m_nationActors) {
        std::cout << "  " << id << ": " << nation->GetName() << std::endl;
    }

    std::cout << "Characters: " << m_characterActors.size() << std::endl;
    for (const auto& [id, character] : m_characterActors) {
        std::cout << "  " << id << ": " << character->GetName() << std::endl;
    }

    std::cout << "Councils: " << m_councilActors.size() << std::endl;
    for (const auto& [id, council] : m_councilActors) {
        std::cout << "  " << id << ": Council" << id << std::endl;
    }
}

// FIX 7: Add missing DumpQueueStatistics() implementation
void AIDirector::DumpQueueStatistics() const {
    std::lock_guard<std::mutex> lock(m_actorMutex);

    std::cout << "\n=== AI Message Queue Statistics ===" << std::endl;
    std::cout << "Total Actors: " << m_actorQueues.size() << std::endl;

    size_t totalMessages = 0;
    std::array<size_t, 4> totalByPriority = {0, 0, 0, 0};

    for (const auto& [actorId, queue] : m_actorQueues) {
        size_t queueSize = queue->GetQueueSize();
        if (queueSize > 0) {
            std::cout << "Actor " << actorId << ": " << queueSize << " messages";

            // Break down by priority
            std::cout << " (C:" << queue->GetQueueSize(MessagePriority::CRITICAL)
                      << " H:" << queue->GetQueueSize(MessagePriority::HIGH)
                      << " M:" << queue->GetQueueSize(MessagePriority::MEDIUM)
                      << " L:" << queue->GetQueueSize(MessagePriority::LOW)
                      << ")" << std::endl;

            totalMessages += queueSize;
            totalByPriority[0] += queue->GetQueueSize(MessagePriority::CRITICAL);
            totalByPriority[1] += queue->GetQueueSize(MessagePriority::HIGH);
            totalByPriority[2] += queue->GetQueueSize(MessagePriority::MEDIUM);
            totalByPriority[3] += queue->GetQueueSize(MessagePriority::LOW);
        }
    }

    std::cout << "\nTotal Messages Queued: " << totalMessages << std::endl;
    std::cout << "  Critical: " << totalByPriority[0] << std::endl;
    std::cout << "  High: " << totalByPriority[1] << std::endl;
    std::cout << "  Medium: " << totalByPriority[2] << std::endl;
    std::cout << "  Low: " << totalByPriority[3] << std::endl;
    std::cout << "================================\n" << std::endl;
}

} // namespace AI
