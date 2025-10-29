// ============================================================================
// Mechanica Imperii - AI Director Calculator Implementation
// ============================================================================

#include "game/ai/calculators/AIDirectorCalculator.h"
#include <algorithm>

namespace AI {

    // ========================================================================
    // Message Scheduling Calculations
    // ========================================================================

    std::chrono::milliseconds AIDirectorCalculator::CalculateSchedulingDelay(
        MessagePriority priority) {

        switch (priority) {
        case MessagePriority::CRITICAL:
            return std::chrono::milliseconds(0); // Immediate
        case MessagePriority::HIGH:
            return std::chrono::hours(24); // 1 day
        case MessagePriority::MEDIUM:
            return std::chrono::hours(24 * 7); // 7 days
        case MessagePriority::LOW:
            return std::chrono::hours(24 * 14); // 14 days
        default:
            return std::chrono::hours(24 * 7); // Default: 7 days
        }
    }

    MessagePriority AIDirectorCalculator::MapRelevanceToPriority(
        InformationRelevance relevance) {

        switch (relevance) {
        case InformationRelevance::CRITICAL:
            return MessagePriority::CRITICAL;
        case InformationRelevance::HIGH:
            return MessagePriority::HIGH;
        case InformationRelevance::MEDIUM:
            return MessagePriority::MEDIUM;
        case InformationRelevance::LOW:
        default:
            return MessagePriority::LOW;
        }
    }

    // ========================================================================
    // Load Balancing Calculations
    // ========================================================================

    bool AIDirectorCalculator::IsActorOverloaded(
        uint32_t queueSize,
        uint32_t threshold) {

        return queueSize > threshold;
    }

    uint32_t AIDirectorCalculator::CountOverloadedActors(
        const std::vector<uint32_t>& queueSizes,
        uint32_t threshold) {

        uint32_t count = 0;
        for (uint32_t queueSize : queueSizes) {
            if (IsActorOverloaded(queueSize, threshold)) {
                count++;
            }
        }
        return count;
    }

    LoadBalanceAction AIDirectorCalculator::DetermineLoadBalanceAction(
        uint32_t overloadedActors,
        uint32_t totalQueuedMessages,
        uint32_t overloadThreshold,
        uint32_t lowLoadThreshold) {

        if (overloadedActors > overloadThreshold) {
            return LoadBalanceAction::INCREASE_PROCESSING;
        } else if (overloadedActors == 0 && totalQueuedMessages < lowLoadThreshold) {
            return LoadBalanceAction::DECREASE_PROCESSING;
        }
        return LoadBalanceAction::MAINTAIN;
    }

    uint32_t AIDirectorCalculator::CalculateOptimalActorsPerFrame(
        uint32_t currentActorsPerFrame,
        LoadBalanceAction action,
        uint32_t minActors,
        uint32_t maxActors,
        uint32_t adjustmentStep) {

        uint32_t newValue = currentActorsPerFrame;

        switch (action) {
        case LoadBalanceAction::INCREASE_PROCESSING:
            newValue = currentActorsPerFrame + adjustmentStep;
            break;
        case LoadBalanceAction::DECREASE_PROCESSING:
            newValue = (currentActorsPerFrame > adjustmentStep)
                ? currentActorsPerFrame - 1
                : currentActorsPerFrame;
            break;
        case LoadBalanceAction::MAINTAIN:
        default:
            newValue = currentActorsPerFrame;
            break;
        }

        return Clamp(newValue, minActors, maxActors);
    }

    bool AIDirectorCalculator::IsSystemIdle(
        uint32_t decisionsThisFrame,
        uint32_t maxActorsPerFrame) {

        return decisionsThisFrame < maxActorsPerFrame / 2;
    }

    // ========================================================================
    // Actor Type Classification
    // ========================================================================

    ActorType AIDirectorCalculator::GetActorType(uint32_t actorId) {
        if (IsNationActor(actorId)) {
            return ActorType::NATION;
        } else if (IsCharacterActor(actorId)) {
            return ActorType::CHARACTER;
        } else if (IsCouncilActor(actorId)) {
            return ActorType::COUNCIL;
        }
        return ActorType::UNKNOWN;
    }

    bool AIDirectorCalculator::IsNationActor(uint32_t actorId) {
        return actorId >= 1000 && actorId < 5000;
    }

    bool AIDirectorCalculator::IsCharacterActor(uint32_t actorId) {
        return actorId >= 5000 && actorId < 9000;
    }

    bool AIDirectorCalculator::IsCouncilActor(uint32_t actorId) {
        return actorId >= 9000;
    }

    // ========================================================================
    // Processing Priority Calculations
    // ========================================================================

    float AIDirectorCalculator::CalculateActorProcessingPriority(
        uint32_t criticalMessages,
        uint32_t highMessages,
        ActorType actorType) {

        float priority = 0.0f;

        // Critical messages have highest weight
        priority += criticalMessages * 100.0f;

        // High priority messages
        priority += highMessages * 10.0f;

        // Actor type importance
        switch (actorType) {
        case ActorType::NATION:
            priority += 5.0f; // Nations are strategically important
            break;
        case ActorType::CHARACTER:
            priority += 3.0f; // Characters less important
            break;
        case ActorType::COUNCIL:
            priority += 4.0f; // Councils moderately important
            break;
        case ActorType::UNKNOWN:
        default:
            priority += 0.0f;
            break;
        }

        return priority;
    }

    bool AIDirectorCalculator::CompareActorPriority(
        uint32_t actor1Critical,
        uint32_t actor1High,
        ActorType actor1Type,
        uint32_t actor2Critical,
        uint32_t actor2High,
        ActorType actor2Type) {

        float priority1 = CalculateActorProcessingPriority(
            actor1Critical, actor1High, actor1Type);
        float priority2 = CalculateActorProcessingPriority(
            actor2Critical, actor2High, actor2Type);

        return priority1 > priority2;
    }

    // ========================================================================
    // Performance Metrics Calculations
    // ========================================================================

    double AIDirectorCalculator::CalculateExponentialMovingAverage(
        double currentValue,
        double newValue,
        double alpha) {

        return alpha * newValue + (1.0 - alpha) * currentValue;
    }

    double AIDirectorCalculator::CalculateAverageDecisionTime(
        double frameTime,
        uint32_t decisionsThisFrame) {

        if (decisionsThisFrame == 0) return 0.0;
        return frameTime / decisionsThisFrame;
    }

    double AIDirectorCalculator::CalculateFrameSleepTime(
        double frameDuration,
        double targetFrameTime) {

        if (frameDuration < targetFrameTime) {
            return targetFrameTime - frameDuration;
        }
        return 0.0;
    }

    int AIDirectorCalculator::CalculateBackgroundTaskBatchSize(
        bool isSystemIdle,
        int maxBatchSize) {

        if (isSystemIdle) {
            return maxBatchSize; // Process more when idle
        }
        return maxBatchSize / 2; // Process less when busy
    }

    // ========================================================================
    // Utility Functions
    // ========================================================================

    uint32_t AIDirectorCalculator::Clamp(
        uint32_t value,
        uint32_t min_val,
        uint32_t max_val) {

        return std::max(min_val, std::min(value, max_val));
    }

    double AIDirectorCalculator::Clamp(
        double value,
        double min_val,
        double max_val) {

        return std::max(min_val, std::min(value, max_val));
    }

    float AIDirectorCalculator::CalculatePercentage(
        uint32_t part,
        uint32_t total) {

        if (total == 0) return 0.0f;
        return (static_cast<float>(part) / total) * 100.0f;
    }

} // namespace AI
