#include "game/time/TimeManagementSystem.h"
#include "game/time/TimeComponents.h"
#include "core/logging/Logger.h"
#include <algorithm>
#include <chrono>
#include <cmath>

namespace game::time {

    // ========================================================================
    // Modern ECS-Based TimeManagementSystem Implementation
    // ========================================================================

    TimeManagementSystem::TimeManagementSystem(::core::ecs::ComponentAccessManager& access_manager,
                                             ::core::threading::ThreadSafeMessageBus& message_bus,
                                             const GameDate& start_date)
        : m_access_manager(access_manager)
        , m_message_bus(message_bus)
        , m_last_update(std::chrono::steady_clock::now()) {
        
        CreateSystemEntities(start_date);
        ::core::logging::LogInfo("TimeManagementSystem", "Modern ECS architecture initialized");
    }

    void TimeManagementSystem::Initialize() {
        ::core::logging::LogInfo("TimeManagementSystem", "System initialized");
        SetupDefaultRoutes();
    }

    void TimeManagementSystem::Update(float deltaTime) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_update);
        
        TimeClockComponent* clock = GetTimeClockComponent();
        if (!clock || clock->is_paused) return;
        
        // Check for hourly tick - this advances the date
        if (clock->ShouldTick(TickType::HOURLY, now)) {
            GameDate old_date = clock->current_date;
            clock->current_date = clock->current_date.AddHours(1);
            GameDate new_date = clock->current_date;
            
            ProcessTick(TickType::HOURLY, new_date);
            clock->UpdateLastTick(TickType::HOURLY, now);
            
            // Fire DAILY when hour wraps to 0 (new day boundary)
            if (new_date.hour == 0 && old_date.hour != 0) {
                ProcessTick(TickType::DAILY, new_date);
                clock->UpdateLastTick(TickType::DAILY, now);
                
                // Fire MONTHLY when day wraps to 1 (new month boundary)
                if (new_date.day == 1 && old_date.day != 1) {
                    ProcessTick(TickType::MONTHLY, new_date);
                    clock->UpdateLastTick(TickType::MONTHLY, now);
                    
                    // Fire YEARLY when month wraps to 1 (new year boundary)
                    if (new_date.month == 1 && old_date.month != 1) {
                        ProcessTick(TickType::YEARLY, new_date);
                        clock->UpdateLastTick(TickType::YEARLY, now);
                    }
                }
            }
        }
        
        // Update performance metrics once per frame
        UpdatePerformanceMetrics();
        
        m_last_update = now;
    }

    void TimeManagementSystem::Shutdown() {
        ::core::logging::LogInfo("TimeManagementSystem", "System shutdown");
        DestroySystemEntities();
    }

    ::core::threading::ThreadingStrategy TimeManagementSystem::GetThreadingStrategy() const {
        return ::core::threading::ThreadingStrategy::MAIN_THREAD;
    }

    std::string TimeManagementSystem::GetThreadingRationale() const {
        return "Time management requires strict sequencing and affects all other systems";
    }

    // ====================================================================
    // Time Control Methods
    // ====================================================================

    void TimeManagementSystem::Pause() {
        TimeClockComponent* clock = GetTimeClockComponent();
        if (clock) {
            TimeScale old_scale = clock->time_scale;
            clock->is_paused = true;
            PublishDateChange(clock->current_date, clock->current_date, "Game paused");
        }
    }

    void TimeManagementSystem::Resume() {
        TimeClockComponent* clock = GetTimeClockComponent();
        if (clock) {
            clock->is_paused = false;
            PublishDateChange(clock->current_date, clock->current_date, "Game resumed");
        }
    }

    void TimeManagementSystem::SetTimeScale(TimeScale scale) {
        TimeClockComponent* clock = GetTimeClockComponent();
        if (clock) {
            TimeScale old_scale = clock->time_scale;
            clock->time_scale = scale;
            
            // Reset tick timestamps to prevent clumping after speed changes
            auto now = std::chrono::steady_clock::now();
            clock->UpdateLastTick(TickType::HOURLY, now);
            clock->UpdateLastTick(TickType::DAILY, now);
            clock->UpdateLastTick(TickType::MONTHLY, now);
            clock->UpdateLastTick(TickType::YEARLY, now);
            
            messages::TimeScaleChanged msg;
            msg.old_scale = old_scale;
            msg.new_scale = scale;
            msg.change_time = clock->current_date;
            m_message_bus.Publish(msg);
        }
    }

    TimeScale TimeManagementSystem::GetTimeScale() const {
        const TimeClockComponent* clock = GetTimeClockComponent();
        return clock ? clock->time_scale : TimeScale::NORMAL;
    }

    bool TimeManagementSystem::IsPaused() const {
        const TimeClockComponent* clock = GetTimeClockComponent();
        return clock ? clock->is_paused : false;
    }

    // ====================================================================
    // Date Management Methods
    // ====================================================================

    GameDate TimeManagementSystem::GetCurrentDate() const {
        const TimeClockComponent* clock = GetTimeClockComponent();
        return clock ? clock->current_date : GameDate(1066, 10, 14);
    }

    void TimeManagementSystem::SetCurrentDate(const GameDate& date) {
        TimeClockComponent* clock = GetTimeClockComponent();
        if (clock) {
            GameDate old_date = clock->current_date;
            clock->current_date = date;
            PublishDateChange(old_date, date, "Manual date change");
        }
    }

    GameDate TimeManagementSystem::GetFutureDate(int hours, int days, int months, int years) const {
        GameDate current = GetCurrentDate();
        return current.AddYears(years).AddMonths(months).AddDays(days).AddHours(hours);
    }

    // ====================================================================
    // Event Scheduling Methods
    // ====================================================================

    game::types::EntityID TimeManagementSystem::ScheduleEvent(const std::string& event_id, const GameDate& when,
                                                           TickType tick_type, const std::string& event_data,
                                                           bool repeating, int repeat_hours) {
        // Get EntityManager from ComponentAccessManager (EconomicSystem pattern)
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            ::core::logging::LogError("TimeManagementSystem", "EntityManager not available");
            return game::types::INVALID_ENTITY;
        }
        
        // Create a new entity for this scheduled event
        auto event_entity = entity_manager->CreateEntity("ScheduledEvent_" + event_id);
        game::types::EntityID entity_id = static_cast<game::types::EntityID>(event_entity.id);
        
        // Create EntityID handle (EconomicSystem pattern)
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        
        // Create and configure the ScheduledEventComponent
        auto event_component = entity_manager->AddComponent<ScheduledEventComponent>(entity_handle);
        if (event_component) {
            event_component->event_id = event_id;
            event_component->scheduled_date = when;
            event_component->tick_type = tick_type;
            event_component->event_data = event_data;
            event_component->repeating = repeating;
            event_component->repeat_interval_hours = repeat_hours;
        }
        
        messages::EventScheduled msg;
        msg.event_id = event_id;
        msg.scheduled_date = when;
        msg.tick_type = tick_type;
        m_message_bus.Publish(msg);
        
        return entity_id;
    }

    void TimeManagementSystem::CancelEvent(const std::string& event_id) {
        // Get the EntityManager
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            ::core::logging::LogError("TimeManagementSystem", "EntityManager not available");
            return;
        }

        // Get all entities with ScheduledEventComponents and find the matching event
        auto scheduled_entities = entity_manager->GetEntitiesWithComponent<ScheduledEventComponent>();
        
        for (const auto& ecs_entity_id : scheduled_entities) {
            // Convert EntityID types
            game::types::EntityID game_entity_id = static_cast<game::types::EntityID>(ecs_entity_id.id);
            auto event_result = m_access_manager.GetComponent<ScheduledEventComponent>(game_entity_id);
            if (event_result.IsValid() && event_result.Get()->event_id == event_id) {
                // Found the event - remove the component and destroy the entity
                entity_manager->RemoveComponent<ScheduledEventComponent>(ecs_entity_id);
                entity_manager->DestroyEntity(ecs_entity_id);
                ::core::logging::LogInfo("TimeManagementSystem", "Canceled scheduled event: " + event_id);
                break;
            }
        }
    }

    void TimeManagementSystem::CancelEvent(game::types::EntityID entity_id) {
        // Get the EntityManager
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            ::core::logging::LogError("TimeManagementSystem", "EntityManager not available");
            return;
        }

        // Convert EntityID to EntityHandle and remove component
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        entity_manager->RemoveComponent<ScheduledEventComponent>(entity_handle);
        entity_manager->DestroyEntity(entity_handle);
    }

    std::vector<game::types::EntityID> TimeManagementSystem::GetScheduledEvents() const {
        // Get EntityManager 
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return {};
        }
        
        // Get all entities with ScheduledEventComponents and convert types
        auto ecs_entities = entity_manager->GetEntitiesWithComponent<ScheduledEventComponent>();
        std::vector<game::types::EntityID> entity_ids;
        entity_ids.reserve(ecs_entities.size());
        
        for (const auto& ecs_entity : ecs_entities) {
            entity_ids.push_back(static_cast<game::types::EntityID>(ecs_entity.id));
        }
        
        return entity_ids;
    }

    std::vector<game::types::EntityID> TimeManagementSystem::GetReadyEvents(const GameDate& current_date) const {
        std::vector<game::types::EntityID> ready_events;
        
        // Get EntityManager
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return ready_events;
        }
        
        // Get all entities with ScheduledEventComponents and check if they're ready
        auto scheduled_entities = entity_manager->GetEntitiesWithComponent<ScheduledEventComponent>();
        
        for (const auto& ecs_entity_id : scheduled_entities) {
            // Convert EntityID types
            game::types::EntityID entity_id = static_cast<game::types::EntityID>(ecs_entity_id.id);
            auto event_result = m_access_manager.GetComponent<ScheduledEventComponent>(entity_id);
            if (event_result.IsValid() && event_result.Get()->IsReady(current_date)) {
                ready_events.push_back(entity_id);
            }
        }
        
        return ready_events;
    }

    // ====================================================================
    // Message System Methods
    // ====================================================================

    game::types::EntityID TimeManagementSystem::SendMessage(const std::string& message_id,
                                                         const std::string& from, const std::string& to,
                                                         const std::string& content, MessageType type, bool urgent) {
        // Get the EntityManager
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            ::core::logging::LogError("TimeManagementSystem", "EntityManager not available");
            return game::types::INVALID_ENTITY;
        }

        // Create entity and get its ID
        ::core::ecs::EntityID entity_handle = entity_manager->CreateEntity();
        game::types::EntityID entity_id = static_cast<game::types::EntityID>(entity_handle.id);
        
        MessageTransitComponent message;
        message.message_id = message_id;
        message.from_location = from;
        message.to_location = to;
        message.content = content;
        message.message_type = type;
        message.is_urgent = urgent;
        message.sent_date = GetCurrentDate();
        
        // Calculate delivery time based on route distance
        RouteNetworkComponent* network = GetRouteNetworkComponent();
        if (network) {
            message.travel_distance_km = network->GetDistance(from, to);
            double base_speed_kmh = urgent ? 4.0 : 2.0; // Historical travel speeds
            
            // Apply route quality and seasonal modifiers to speed
            double route_quality = network->GetRouteQuality(from, to);
            double seasonal_modifier = network->current_seasonal_modifier;
            message.travel_speed_kmh = base_speed_kmh * route_quality * seasonal_modifier;
            
            double hours_needed = message.travel_distance_km / message.travel_speed_kmh;
            message.expected_arrival = message.sent_date.AddHours(static_cast<int>(std::ceil(hours_needed)));
        } else {
            message.expected_arrival = message.sent_date.AddDays(urgent ? 1 : 3);
        }
        
        entity_manager->AddComponent<MessageTransitComponent>(entity_handle, message);
        return entity_id;
    }

    std::vector<game::types::EntityID> TimeManagementSystem::GetMessagesInTransit() const {
        // Get EntityManager 
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return {};
        }
        
        // Get all entities with MessageTransitComponents and convert types
        auto ecs_entities = entity_manager->GetEntitiesWithComponent<MessageTransitComponent>();
        std::vector<game::types::EntityID> entity_ids;
        entity_ids.reserve(ecs_entities.size());
        
        for (const auto& ecs_entity : ecs_entities) {
            entity_ids.push_back(static_cast<game::types::EntityID>(ecs_entity.id));
        }
        
        return entity_ids;
    }

    std::vector<game::types::EntityID> TimeManagementSystem::GetDeliveredMessages(const GameDate& current_date) const {
        std::vector<game::types::EntityID> delivered;
        
        // Get EntityManager
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return delivered;
        }
        
        // Get all entities with MessageTransitComponents and check if they're delivered
        auto message_entities = entity_manager->GetEntitiesWithComponent<MessageTransitComponent>();
        
        for (const auto& ecs_entity_id : message_entities) {
            // Convert EntityID types
            game::types::EntityID entity_id = static_cast<game::types::EntityID>(ecs_entity_id.id);
            auto message_result = m_access_manager.GetComponent<MessageTransitComponent>(entity_id);
            if (message_result.IsValid() && message_result.Get()->IsDelivered()) {
                delivered.push_back(entity_id);
            }
        }
        
        return delivered;
    }

    // ====================================================================
    // Route Management Methods
    // ====================================================================

    void TimeManagementSystem::AddRoute(const std::string& from, const std::string& to, double distance_km) {
        RouteNetworkComponent* network = GetRouteNetworkComponent();
        if (network) {
            network->AddRoute(from, to, distance_km);
        }
    }

    void TimeManagementSystem::RemoveRoute(const std::string& from, const std::string& to) {
        RouteNetworkComponent* network = GetRouteNetworkComponent();
        if (network) {
            network->RemoveRoute(from, to);
        }
    }

    double TimeManagementSystem::GetRouteDistance(const std::string& from, const std::string& to) const {
        const RouteNetworkComponent* network = GetRouteNetworkComponent();
        return network ? network->GetDistance(from, to) : 0.0;
    }

    // ====================================================================
    // Entity Time Tracking Methods
    // ====================================================================

    void TimeManagementSystem::AddTimeTracking(game::types::EntityID entity, const GameDate& creation_date) {
        // Get the EntityManager
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            ::core::logging::LogError("TimeManagementSystem", "EntityManager not available");
            return;
        }

        EntityTimeComponent time_comp(creation_date);
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity), 1);
        entity_manager->AddComponent<EntityTimeComponent>(entity_handle, time_comp);
    }

    void TimeManagementSystem::RemoveTimeTracking(game::types::EntityID entity) {
        // Get the EntityManager
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            ::core::logging::LogError("TimeManagementSystem", "EntityManager not available");
            return;
        }

        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity), 1);
        entity_manager->RemoveComponent<EntityTimeComponent>(entity_handle);
    }

    void TimeManagementSystem::UpdateEntityAges() {
        GameDate current_date = GetCurrentDate();
        
        // Get EntityManager
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return;
        }
        
        // Get all entities with EntityTimeComponents and update their ages
        auto time_tracked_entities = entity_manager->GetEntitiesWithComponent<EntityTimeComponent>();
        
        for (const auto& ecs_entity_id : time_tracked_entities) {
            // Convert EntityID types
            game::types::EntityID entity_id = static_cast<game::types::EntityID>(ecs_entity_id.id);
            auto time_result = m_access_manager.GetComponentForWrite<EntityTimeComponent>(entity_id);
            if (time_result.IsValid()) {
                time_result.Get()->UpdateAge(current_date);
            }
        }
    }

    std::vector<game::types::EntityID> TimeManagementSystem::GetTimeTrackedEntities() const {
        // Get EntityManager 
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return {};
        }
        
        // Get all entities with EntityTimeComponents and convert types
        auto ecs_entities = entity_manager->GetEntitiesWithComponent<EntityTimeComponent>();
        std::vector<game::types::EntityID> entity_ids;
        entity_ids.reserve(ecs_entities.size());
        
        for (const auto& ecs_entity : ecs_entities) {
            entity_ids.push_back(static_cast<game::types::EntityID>(ecs_entity.id));
        }
        
        return entity_ids;
    }

    // ====================================================================
    // Performance Monitoring Methods
    // ====================================================================

    TimeManagementSystem::PerformanceReport TimeManagementSystem::GetPerformanceReport() const {
        PerformanceReport report;
        
        const TimePerformanceComponent* perf = GetPerformanceComponent();
        if (perf) {
            report.hourly_tick_ms = perf->hourly_tick_ms;
            report.daily_tick_ms = perf->daily_tick_ms;
            report.monthly_tick_ms = perf->monthly_tick_ms;
            report.yearly_tick_ms = perf->yearly_tick_ms;
            report.total_update_ms = perf->total_update_ms;
            report.performance_warning = perf->performance_warning;
            report.active_events = perf->active_events;
            report.messages_in_transit = perf->messages_in_transit;
            report.entities_with_time = perf->entities_with_time;
        }
        
        return report;
    }

    void TimeManagementSystem::ResetPerformanceMetrics() {
        TimePerformanceComponent* perf = GetPerformanceComponent();
        if (perf) {
            perf->ResetMetrics();
        }
    }

    // ====================================================================
    // Callback Registration Methods
    // ====================================================================

    void TimeManagementSystem::RegisterTickCallback(TickType tick_type, const std::string& system_name, TickCallback callback) {
        m_tick_callbacks[tick_type][system_name] = callback;
    }

    void TimeManagementSystem::UnregisterTickCallback(TickType tick_type, const std::string& system_name) {
        if (m_tick_callbacks.count(tick_type)) {
            m_tick_callbacks[tick_type].erase(system_name);
        }
    }

    // ====================================================================
    // Integration Methods
    // ====================================================================

    void TimeManagementSystem::SetGameplayCoordinator(game::gameplay::GameplayCoordinator* coordinator) {
        m_gameplay_coordinator = coordinator;
    }

    void TimeManagementSystem::SaveState(Json::Value& state) const {
        GameDate current_date = GetCurrentDate();
        state["current_date"] = current_date.ToString();
        
        // Store date as structured fields for reliable restoration
        state["date_year"] = current_date.year;
        state["date_month"] = current_date.month;
        state["date_day"] = current_date.day;
        state["date_hour"] = current_date.hour;
        
        state["time_scale"] = static_cast<int>(GetTimeScale());
        state["is_paused"] = IsPaused();
    }

    void TimeManagementSystem::LoadState(const Json::Value& state) {
        // Restore date from structured fields
        if (state.isMember("date_year") && state.isMember("date_month") && 
            state.isMember("date_day") && state.isMember("date_hour")) {
            GameDate loaded_date(
                state["date_year"].asInt(),
                state["date_month"].asInt(),
                state["date_day"].asInt(),
                state["date_hour"].asInt()
            );
            SetCurrentDate(loaded_date);
        }
        
        if (state.isMember("time_scale")) {
            SetTimeScale(static_cast<TimeScale>(state["time_scale"].asInt()));
        }
        if (state.isMember("is_paused") && state["is_paused"].asBool()) {
            Pause();
        }
    }

    // ====================================================================
    // Private Methods
    // ====================================================================

    void TimeManagementSystem::CreateSystemEntities(const GameDate& start_date) {
        // Get the EntityManager
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            ::core::logging::LogError("TimeManagementSystem", "EntityManager not available");
            return;
        }

        // Create time clock entity
        ::core::ecs::EntityID clock_handle = entity_manager->CreateEntity();
        m_time_clock_entity = static_cast<game::types::EntityID>(clock_handle.id);
        TimeClockComponent clock_component(start_date);
        entity_manager->AddComponent<TimeClockComponent>(clock_handle, clock_component);

        // Create route network entity
        ::core::ecs::EntityID network_handle = entity_manager->CreateEntity();
        m_route_network_entity = static_cast<game::types::EntityID>(network_handle.id);
        RouteNetworkComponent network_component;
        entity_manager->AddComponent<RouteNetworkComponent>(network_handle, network_component);
        
        // Create performance tracking entity
        ::core::ecs::EntityID perf_handle = entity_manager->CreateEntity();
        m_performance_entity = static_cast<game::types::EntityID>(perf_handle.id);
        TimePerformanceComponent perf_component;
        entity_manager->AddComponent<TimePerformanceComponent>(perf_handle, perf_component);
    }

    void TimeManagementSystem::DestroySystemEntities() {
        // Get the EntityManager
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            ::core::logging::LogError("TimeManagementSystem", "EntityManager not available");
            return;
        }

        if (m_time_clock_entity != game::types::INVALID_ENTITY) {
            ::core::ecs::EntityID clock_handle(static_cast<uint64_t>(m_time_clock_entity), 1);
            entity_manager->DestroyEntity(clock_handle);
        }
        if (m_route_network_entity != game::types::INVALID_ENTITY) {
            ::core::ecs::EntityID network_handle(static_cast<uint64_t>(m_route_network_entity), 1);
            entity_manager->DestroyEntity(network_handle);
        }
        if (m_performance_entity != game::types::INVALID_ENTITY) {
            ::core::ecs::EntityID perf_handle(static_cast<uint64_t>(m_performance_entity), 1);
            entity_manager->DestroyEntity(perf_handle);
        }
    }

    void TimeManagementSystem::ProcessTick(TickType tick_type, const GameDate& current_date) {
        auto start_time = std::chrono::steady_clock::now();
        
        // Process scheduled events for this tick type
        ProcessScheduledEvents();
        
        // Process message deliveries
        ProcessMessageTransit();
        
        // Update entity ages
        UpdateEntityAges();
        
        // Execute registered callbacks
        if (m_tick_callbacks.count(tick_type)) {
            for (const auto& [system_name, callback] : m_tick_callbacks[tick_type]) {
                if (callback) {
                    callback(current_date, tick_type);
                }
            }
        }
        
        // Update performance metrics
        auto end_time = std::chrono::steady_clock::now();
        double processing_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        
        TimePerformanceComponent* perf = GetPerformanceComponent();
        if (perf) {
            perf->UpdateTickPerformance(tick_type, processing_ms);
        }
        
        // Publish tick event
        messages::TickOccurred tick_event;
        tick_event.tick_type = tick_type;
        tick_event.current_date = current_date;
        tick_event.processing_time_ms = processing_ms;
        PublishTimeEvent(tick_event);
    }

    void TimeManagementSystem::ProcessScheduledEvents() {
        GameDate current_date = GetCurrentDate();
        auto ready_events = GetReadyEvents(current_date);
        
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return;
        }

        for (game::types::EntityID entity : ready_events) {
            auto event_result = m_access_manager.GetComponent<ScheduledEventComponent>(entity);
            if (event_result.IsValid()) {
                const auto* event = event_result.Get();
                ExecuteEvent(*event);
                
                if (event->repeating && event->repeat_interval_hours > 0) {
                    // Reschedule repeating event - need write access
                    auto write_result = m_access_manager.GetComponentForWrite<ScheduledEventComponent>(entity);
                    if (write_result.IsValid()) {
                        write_result.Get()->scheduled_date = event->scheduled_date.AddHours(event->repeat_interval_hours);
                    }
                } else {
                    // Remove one-time event and destroy entity
                    ::core::ecs::EntityID ecs_entity_id(static_cast<uint64_t>(entity), 1);
                    entity_manager->RemoveComponent<ScheduledEventComponent>(ecs_entity_id);
                    entity_manager->DestroyEntity(ecs_entity_id);
                }
            }
        }
    }

    void TimeManagementSystem::ProcessMessageTransit() {
        GameDate current_date = GetCurrentDate();
        auto message_entities = GetMessagesInTransit();
        
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return;
        }
        
        for (game::types::EntityID entity : message_entities) {
            auto message_result = m_access_manager.GetComponent<MessageTransitComponent>(entity);
            if (message_result.IsValid()) {
                const auto* message = message_result.Get();
                if (current_date >= message->expected_arrival) {
                    DeliverMessage(*message);
                    // Remove delivered message and destroy entity
                    ::core::ecs::EntityID ecs_entity_id(static_cast<uint64_t>(entity), 1);
                    entity_manager->RemoveComponent<MessageTransitComponent>(ecs_entity_id);
                    entity_manager->DestroyEntity(ecs_entity_id);
                }
            }
        }
    }

    void TimeManagementSystem::UpdatePerformanceMetrics() {
        TimePerformanceComponent* perf = GetPerformanceComponent();
        if (perf) {
            perf->active_events = static_cast<int>(GetScheduledEvents().size());
            perf->messages_in_transit = static_cast<int>(GetMessagesInTransit().size());
            perf->entities_with_time = static_cast<int>(GetTimeTrackedEntities().size());
        }
    }

    void TimeManagementSystem::ExecuteEvent(const ScheduledEventComponent& event) {
        messages::EventExecuted msg;
        msg.event_id = event.event_id;
        msg.execution_date = GetCurrentDate();
        msg.success = true;
        msg.result_data = event.event_data;
        m_message_bus.Publish(msg);
        
        ::core::logging::LogInfo("TimeManagementSystem", "Event executed: " + event.event_id);
    }

    void TimeManagementSystem::DeliverMessage(const MessageTransitComponent& message) {
        messages::MessageDelivered msg;
        msg.message_id = message.message_id;
        msg.delivery_date = GetCurrentDate();
        msg.from_location = message.from_location;
        msg.to_location = message.to_location;
        m_message_bus.Publish(msg);
        
        ::core::logging::LogInfo("TimeManagementSystem", "Message delivered: " + message.message_id);
    }

    TimeClockComponent* TimeManagementSystem::GetTimeClockComponent() {
        auto result = m_access_manager.GetComponentForWrite<TimeClockComponent>(m_time_clock_entity);
        return result.IsValid() ? result.Get() : nullptr;
    }

    const TimeClockComponent* TimeManagementSystem::GetTimeClockComponent() const {
        auto result = m_access_manager.GetComponent<TimeClockComponent>(m_time_clock_entity);
        return result.IsValid() ? result.Get() : nullptr;
    }

    RouteNetworkComponent* TimeManagementSystem::GetRouteNetworkComponent() {
        auto result = m_access_manager.GetComponentForWrite<RouteNetworkComponent>(m_route_network_entity);
        return result.IsValid() ? result.Get() : nullptr;
    }

    TimePerformanceComponent* TimeManagementSystem::GetPerformanceComponent() {
        auto result = m_access_manager.GetComponentForWrite<TimePerformanceComponent>(m_performance_entity);
        return result.IsValid() ? result.Get() : nullptr;
    }

    const RouteNetworkComponent* TimeManagementSystem::GetRouteNetworkComponent() const {
        auto result = m_access_manager.GetComponent<RouteNetworkComponent>(m_route_network_entity);
        return result.IsValid() ? result.Get() : nullptr;
    }

    const TimePerformanceComponent* TimeManagementSystem::GetPerformanceComponent() const {
        auto result = m_access_manager.GetComponent<TimePerformanceComponent>(m_performance_entity);
        return result.IsValid() ? result.Get() : nullptr;
    }

    void TimeManagementSystem::SetupDefaultRoutes() {
        AddRoute("London", "Winchester", 100.0);
        AddRoute("London", "Canterbury", 80.0);
        AddRoute("Winchester", "Canterbury", 120.0);
        AddRoute("London", "York", 300.0);
        AddRoute("Winchester", "Exeter", 150.0);
    }

    void TimeManagementSystem::PublishTimeEvent(const messages::TickOccurred& event) {
        m_message_bus.Publish(event);
    }

    void TimeManagementSystem::PublishDateChange(const GameDate& old_date, const GameDate& new_date, const std::string& reason) {
        messages::DateChanged msg;
        msg.old_date = old_date;
        msg.new_date = new_date;
        msg.reason = reason;
        m_message_bus.Publish(msg);
    }

} // namespace game::time
