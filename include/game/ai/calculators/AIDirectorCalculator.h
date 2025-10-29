// ============================================================================
// Mechanica Imperii - AI Director Calculator Header
// Pure Calculation Functions for AI Director Coordination
// ============================================================================

#pragma once

#include "game/ai/AIDirector.h"
#include <chrono>
#include <cstdint>

namespace AI {

    /**
     * @brief Actor type classification
     */
    enum class ActorType {
        NATION,
        CHARACTER,
        COUNCIL,
        UNKNOWN
    };

    /**
     * @brief Load balancing recommendation
     */
    enum class LoadBalanceAction {
        INCREASE_PROCESSING,
        DECREASE_PROCESSING,
        MAINTAIN
    };

    /**
     * @brief Pure calculation functions for AI Director coordination
     * All functions are static with no side effects
     */
    class AIDirectorCalculator {
    public:
        // ====================================================================
        // Message Scheduling Calculations
        // ====================================================================

        /**
         * @brief Calculate scheduling delay based on message priority
         * CRITICAL: Immediate (0ms)
         * HIGH: 1 day (24 hours)
         * MEDIUM: 7 days (1 week)
         * LOW: 14 days (2 weeks)
         */
        static std::chrono::milliseconds CalculateSchedulingDelay(MessagePriority priority);

        /**
         * @brief Map information relevance to message priority
         */
        static MessagePriority MapRelevanceToPriority(InformationRelevance relevance);

        // ====================================================================
        // Load Balancing Calculations
        // ====================================================================

        /**
         * @brief Determine if actor queue is overloaded
         */
        static bool IsActorOverloaded(uint32_t queueSize, uint32_t threshold = 50);

        /**
         * @brief Calculate number of overloaded actors
         */
        static uint32_t CountOverloadedActors(
            const std::vector<uint32_t>& queueSizes,
            uint32_t threshold = 50
        );

        /**
         * @brief Determine load balancing action based on system state
         */
        static LoadBalanceAction DetermineLoadBalanceAction(
            uint32_t overloadedActors,
            uint32_t totalQueuedMessages,
            uint32_t overloadThreshold = 5,
            uint32_t lowLoadThreshold = 100
        );

        /**
         * @brief Calculate optimal actors per frame for load balancing
         */
        static uint32_t CalculateOptimalActorsPerFrame(
            uint32_t currentActorsPerFrame,
            LoadBalanceAction action,
            uint32_t minActors = 5,
            uint32_t maxActors = 20,
            uint32_t adjustmentStep = 2
        );

        /**
         * @brief Determine if system is idle (low workload)
         */
        static bool IsSystemIdle(
            uint32_t decisionsThisFrame,
            uint32_t maxActorsPerFrame
        );

        // ====================================================================
        // Actor Type Classification
        // ====================================================================

        /**
         * @brief Determine actor type from actor ID
         * Nations: 1000-4999
         * Characters: 5000-8999
         * Councils: 9000+
         */
        static ActorType GetActorType(uint32_t actorId);

        /**
         * @brief Check if actor is a nation AI
         */
        static bool IsNationActor(uint32_t actorId);

        /**
         * @brief Check if actor is a character AI
         */
        static bool IsCharacterActor(uint32_t actorId);

        /**
         * @brief Check if actor is a council AI
         */
        static bool IsCouncilActor(uint32_t actorId);

        // ====================================================================
        // Processing Priority Calculations
        // ====================================================================

        /**
         * @brief Calculate actor processing priority score
         * Higher score = higher priority
         */
        static float CalculateActorProcessingPriority(
            uint32_t criticalMessages,
            uint32_t highMessages,
            ActorType actorType
        );

        /**
         * @brief Compare two actors for priority ordering
         * Returns true if actor1 should be processed before actor2
         */
        static bool CompareActorPriority(
            uint32_t actor1Critical,
            uint32_t actor1High,
            ActorType actor1Type,
            uint32_t actor2Critical,
            uint32_t actor2High,
            ActorType actor2Type
        );

        // ====================================================================
        // Performance Metrics Calculations
        // ====================================================================

        /**
         * @brief Calculate exponential moving average
         * Used for smoothing frame time measurements
         */
        static double CalculateExponentialMovingAverage(
            double currentValue,
            double newValue,
            double alpha = 0.1
        );

        /**
         * @brief Calculate average decision time
         */
        static double CalculateAverageDecisionTime(
            double frameTime,
            uint32_t decisionsThisFrame
        );

        /**
         * @brief Calculate required sleep time to maintain target frame rate
         */
        static double CalculateFrameSleepTime(
            double frameDuration,
            double targetFrameTime
        );

        /**
         * @brief Determine optimal background task batch size
         */
        static int CalculateBackgroundTaskBatchSize(
            bool isSystemIdle,
            int maxBatchSize = 5
        );

        // ====================================================================
        // Utility Functions
        // ====================================================================

        /**
         * @brief Clamp value to range
         */
        static uint32_t Clamp(uint32_t value, uint32_t min_val, uint32_t max_val);

        /**
         * @brief Clamp value to range (double)
         */
        static double Clamp(double value, double min_val, double max_val);

        /**
         * @brief Calculate percentage
         */
        static float CalculatePercentage(uint32_t part, uint32_t total);
    };

} // namespace AI
