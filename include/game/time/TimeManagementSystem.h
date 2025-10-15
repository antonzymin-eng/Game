// ============================================================================
// Mechanica Imperii - Time Management System Header (Modern ECS Version)
// Created: October 14, 2025 - Complete ECS Component Rewrite
// Location: include/game/time/TimeManagementSystem.h
// ============================================================================

#pragma once

#include "game/time/TimeComponents.h"
#include "core/ECS/ComponentAccessManager.h" 
#include "core/threading/ThreadSafeMessageBus.h"
#include "core/threading/ThreadedSystemManager.h"
#include "core/types/game_types.h"
#include <chrono>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <jsoncpp/json/json.h>

// Forward declarations
namespace game::gameplay {
    class GameplayCoordinator;
}

namespace game::time {

    // ========================================================================
    // Time Events (Message Bus Integration)
    // ========================================================================

    namespace messages {
        struct TimeScaleChanged : public ::core::ecs::IMessage {
            TimeScale old_scale;
            TimeScale new_scale;
            GameDate change_time;
            std::type_index GetTypeIndex() const override { return typeid(TimeScaleChanged); }
        };

        struct TickOccurred : public ::core::ecs::IMessage {
            TickType tick_type;
            GameDate current_date;
            double processing_time_ms;
            std::type_index GetTypeIndex() const override { return typeid(TickOccurred); }
        };

        struct EventScheduled : public ::core::ecs::IMessage {
            std::string event_id;
            GameDate scheduled_date;
            TickType tick_type;
            std::string category;
            std::type_index GetTypeIndex() const override { return typeid(EventScheduled); }
        };

        struct EventExecuted : public ::core::ecs::IMessage {
            std::string event_id;
            GameDate execution_date;
            bool success;
            std::string result_data;
            std::type_index GetTypeIndex() const override { return typeid(EventExecuted); }
        };

        struct MessageDelivered : public ::core::ecs::IMessage {
            std::string message_id;
            GameDate delivery_date;
            std::string from_location;
            std::string to_location;
            std::type_index GetTypeIndex() const override { return typeid(MessageDelivered); }
        };

        struct DateChanged : public ::core::ecs::IMessage {
            GameDate old_date;
            GameDate new_date;
            std::string reason; // "natural_progression", "manual_set", "save_load"
            std::type_index GetTypeIndex() const override { return typeid(DateChanged); }
        };
    }

    // ========================================================================
    // Modern ECS-Based Time Management System
    // ========================================================================

    class TimeManagementSystem {
    public:
        struct PerformanceReport {
            double hourly_tick_ms = 0.0;
            double daily_tick_ms = 0.0;
            double monthly_tick_ms = 0.0;
            double yearly_tick_ms = 0.0;
            double total_update_ms = 0.0;
            bool performance_warning = false;
            int active_events = 0;
            int messages_in_transit = 0;
            int entities_with_time = 0;
        };

        explicit TimeManagementSystem(::core::ecs::ComponentAccessManager& access_manager,
                                     ::core::threading::ThreadSafeMessageBus& message_bus,
                                     const GameDate& start_date = GameDate(1066, 10, 14));
        ~TimeManagementSystem() = default;

        // ====================================================================
        // System Lifecycle (ThreadedSystem interface)
        // ====================================================================
        void Initialize();
        void Update(float deltaTime);
        void Shutdown();
        
        ::core::threading::ThreadingStrategy GetThreadingStrategy() const;
        std::string GetThreadingRationale() const;

        // ====================================================================
        // Time Control (operates on TimeClockComponent)
        // ====================================================================
        void Pause();
        void Resume();
        void SetTimeScale(TimeScale scale);
        TimeScale GetTimeScale() const;
        bool IsPaused() const;

        // ====================================================================
        // Date Management (via TimeClockComponent)
        // ====================================================================
        GameDate GetCurrentDate() const;
        void SetCurrentDate(const GameDate& date);
        GameDate GetFutureDate(int hours, int days = 0, int months = 0, int years = 0) const;

        // ====================================================================
        // Event Scheduling (creates ScheduledEventComponent entities)
        // ====================================================================
        game::types::EntityID ScheduleEvent(const std::string& event_id, const GameDate& when,
                                         TickType tick_type = TickType::DAILY,
                                         const std::string& event_data = "",
                                         bool repeating = false, int repeat_hours = 0);
        
        void CancelEvent(const std::string& event_id);
        void CancelEvent(game::types::EntityID entity_id);
        
        std::vector<game::types::EntityID> GetScheduledEvents() const;
        std::vector<game::types::EntityID> GetReadyEvents(const GameDate& current_date) const;

        // ====================================================================
        // Message System (creates MessageTransitComponent entities)
        // ====================================================================
        game::types::EntityID SendMessage(const std::string& message_id,
                                       const std::string& from, const std::string& to,
                                       const std::string& content, 
                                       MessageType type = MessageType::PERSONAL,
                                       bool urgent = false);
        
        std::vector<game::types::EntityID> GetMessagesInTransit() const;
        std::vector<game::types::EntityID> GetDeliveredMessages(const GameDate& current_date) const;

        // ====================================================================
        // Route Management (operates on RouteNetworkComponent)
        // ====================================================================
        void AddRoute(const std::string& from, const std::string& to, double distance_km);
        void RemoveRoute(const std::string& from, const std::string& to);
        double GetRouteDistance(const std::string& from, const std::string& to) const;

        // ====================================================================
        // Entity Time Tracking (creates EntityTimeComponent for entities)
        // ====================================================================
        void AddTimeTracking(game::types::EntityID entity, const GameDate& creation_date);
        void RemoveTimeTracking(game::types::EntityID entity);
        void UpdateEntityAges();
        
        std::vector<game::types::EntityID> GetTimeTrackedEntities() const;

        // ====================================================================
        // Performance Monitoring (operates on TimePerformanceComponent)
        // ====================================================================
        PerformanceReport GetPerformanceReport() const;
        void ResetPerformanceMetrics();

        // ====================================================================
        // Callback Registration (for tick notifications)
        // ====================================================================
        using TickCallback = std::function<void(const GameDate&, TickType)>;
        
        void RegisterTickCallback(TickType tick_type, const std::string& system_name, TickCallback callback);
        void UnregisterTickCallback(TickType tick_type, const std::string& system_name);

        // ====================================================================
        // Integration
        // ====================================================================
        void SetGameplayCoordinator(game::gameplay::GameplayCoordinator* coordinator);

        // ====================================================================
        // Save/Load Support
        // ====================================================================
        void SaveState(Json::Value& state) const;
        void LoadState(const Json::Value& state);

    private:
        // ====================================================================
        // ECS Integration
        // ====================================================================
        ::core::ecs::ComponentAccessManager& m_access_manager;
        ::core::threading::ThreadSafeMessageBus& m_message_bus;

        // ====================================================================
        // System Entities (ECS entities that hold singleton-like components)
        // ====================================================================
        game::types::EntityID m_time_clock_entity = game::types::INVALID_ENTITY;          // Holds TimeClockComponent
        game::types::EntityID m_route_network_entity = game::types::INVALID_ENTITY;       // Holds RouteNetworkComponent  
        game::types::EntityID m_performance_entity = game::types::INVALID_ENTITY;         // Holds TimePerformanceComponent

        // ====================================================================
        // Tick Callbacks
        // ====================================================================
        std::unordered_map<TickType, std::unordered_map<std::string, TickCallback>> m_tick_callbacks;

        // ====================================================================
        // Integration
        // ====================================================================
        game::gameplay::GameplayCoordinator* m_gameplay_coordinator = nullptr;

        // ====================================================================
        // Timing Control
        // ====================================================================
        std::chrono::steady_clock::time_point m_last_update;

        // ====================================================================
        // Internal Methods
        // ====================================================================
        void CreateSystemEntities(const GameDate& start_date);
        void DestroySystemEntities();
        
        void ProcessTick(TickType tick_type, const GameDate& current_date);
        void ProcessScheduledEvents();
        void ProcessMessageTransit();
        void UpdatePerformanceMetrics();
        
        void ExecuteEvent(const ScheduledEventComponent& event);
        void DeliverMessage(const MessageTransitComponent& message);
        
        // Component access helpers
        TimeClockComponent* GetTimeClockComponent();
        const TimeClockComponent* GetTimeClockComponent() const;
        RouteNetworkComponent* GetRouteNetworkComponent();
        const RouteNetworkComponent* GetRouteNetworkComponent() const;
        TimePerformanceComponent* GetPerformanceComponent();
        const TimePerformanceComponent* GetPerformanceComponent() const;
        
        // Setup
        void SetupDefaultRoutes();
        
        // Message publishing
        void PublishTimeEvent(const messages::TickOccurred& event);
        void PublishDateChange(const GameDate& old_date, const GameDate& new_date, const std::string& reason);
    };

} // namespace game::time