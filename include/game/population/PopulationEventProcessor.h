// ============================================================================
// PopulationEventProcessor.h - Population Event Processing System
// Created: 2025-11-18
// Location: include/game/population/PopulationEventProcessor.h
// ============================================================================

#pragma once

#include "game/population/PopulationEvents.h"
#include "game/population/PopulationComponents.h"
#include "core/threading/ThreadSafeMessageBus.h"
#include "core/types/game_types.h"
#include <vector>
#include <array>
#include <unordered_map>
#include <chrono>

namespace game::population {

    /**
     * @brief Processes population events and triggers consequential events
     *
     * Handles event logging, state tracking, and cascading effects of
     * population changes.
     */
    class PopulationEventProcessor {
    public:
        PopulationEventProcessor();
        ~PopulationEventProcessor() = default;

        // Event processing
        void ProcessPopulationUpdate(const PopulationUpdateEvent& event,
                                     ::core::threading::ThreadSafeMessageBus& message_bus);

        void ProcessDemographicChange(const DemographicChangeEvent& event,
                                      ::core::threading::ThreadSafeMessageBus& message_bus);

        void ProcessHealthCrisis(const HealthCrisisEvent& event,
                                ::core::threading::ThreadSafeMessageBus& message_bus);

        void ProcessSocialMobility(const SocialMobilityEvent& event,
                                  ::core::threading::ThreadSafeMessageBus& message_bus);

        void ProcessMigration(const MigrationEvent& event,
                            ::core::threading::ThreadSafeMessageBus& message_bus);

        void ProcessCrisisEvent(game::types::EntityID entity_id,
                               const std::string& crisis_type,
                               double severity,
                               ::core::threading::ThreadSafeMessageBus& message_bus);

        // State tracking
        void RecordEvent(game::types::EntityID entity_id, const std::string& event_description);

        std::vector<std::string> GetRecentEvents(game::types::EntityID entity_id, int max_count = 10) const;

        void ClearEventHistory(game::types::EntityID entity_id);

        // Crisis detection and escalation
        bool IsCrisisActive(game::types::EntityID entity_id, const std::string& crisis_type) const;

        void ActivateCrisis(game::types::EntityID entity_id, const std::string& crisis_type, double severity);

        void DeactivateCrisis(game::types::EntityID entity_id, const std::string& crisis_type);

        double GetCrisisSeverity(game::types::EntityID entity_id, const std::string& crisis_type) const;

        // Demographic shift detection
        bool DetectSignificantShift(const PopulationComponent& before, const PopulationComponent& after) const;

        std::string AnalyzePopulationTrend(game::types::EntityID entity_id,
                                          const PopulationComponent& current) const;

    private:
        // Event history tracking
        struct EventRecord {
            std::string description;
            std::chrono::system_clock::time_point timestamp;
        };

        // ==================================================================
        // Performance Optimization: Circular Buffer for Event History (Priority 1.2)
        // ==================================================================
        // Replaces std::vector to eliminate O(n) erase operations
        // Provides O(1) insertion and automatic overwriting of old events

        template<typename T, size_t Capacity>
        class CircularBuffer {
        public:
            CircularBuffer() : m_head(0), m_size(0) {}

            void push_back(const T& item) {
                m_buffer[m_head] = item;
                m_head = (m_head + 1) % Capacity;
                if (m_size < Capacity) {
                    ++m_size;
                }
            }

            size_t size() const { return m_size; }
            bool empty() const { return m_size == 0; }

            void clear() {
                m_head = 0;
                m_size = 0;
            }

            // Get recent N items (newest first)
            std::vector<T> get_recent(size_t count) const {
                if (m_size == 0) return {};

                size_t items_to_get = std::min(count, m_size);
                std::vector<T> result;
                result.reserve(items_to_get);

                // Start from most recent and go backwards
                size_t index = (m_head + Capacity - 1) % Capacity;
                for (size_t i = 0; i < items_to_get; ++i) {
                    result.push_back(m_buffer[index]);
                    index = (index + Capacity - 1) % Capacity;
                }

                return result;
            }

        private:
            std::array<T, Capacity> m_buffer;
            size_t m_head;  // Next write position
            size_t m_size;  // Current number of elements
        };

        std::unordered_map<game::types::EntityID, CircularBuffer<EventRecord, MAX_EVENT_HISTORY>> m_event_history;

        // Crisis tracking
        struct CrisisState {
            std::string crisis_type;
            double severity;
            std::chrono::system_clock::time_point start_time;
            bool active;
        };

        std::unordered_map<game::types::EntityID, std::vector<CrisisState>> m_active_crises;

        // Constants
        static constexpr int MAX_EVENT_HISTORY = 100;
        static constexpr double SIGNIFICANT_CHANGE_THRESHOLD = 0.05; // 5% change

        // Helper methods
        void TriggerCascadingEvents(const HealthCrisisEvent& event,
                                   ::core::threading::ThreadSafeMessageBus& message_bus);

        void CheckCrisisEscalation(game::types::EntityID entity_id,
                                  ::core::threading::ThreadSafeMessageBus& message_bus);

        std::string FormatEventDescription(const std::string& event_type,
                                          game::types::EntityID entity_id,
                                          const std::string& details) const;
    };

} // namespace game::population
