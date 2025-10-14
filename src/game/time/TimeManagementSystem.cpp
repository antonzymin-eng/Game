#include "game/time/TimeManagementSystem.h"
#include "game/time/TimeComponents.h"
#include "core/logging/Logger.h"
#include <algorithm>
#include <chrono>

namespace game::time {

    // ========================================================================
    // Modern ECS-Based TimeManagementSystem Implementation
    // ========================================================================

    TimeManagementSystem::TimeManagementSystem(core::ecs::ComponentAccessManager& access_manager,
                                             core::threading::ThreadSafeMessageBus& message_bus,
                                             const GameDate& start_date)
        : m_access_manager(access_manager)
        , m_message_bus(message_bus)
        , m_last_update(std::chrono::steady_clock::now()) {
        
        CreateSystemEntities(start_date);
        core::logging::Logger::Info("[TimeManagementSystem] Modern ECS architecture initialized");
    }

    void TimeManagementSystem::Initialize() {
        core::logging::Logger::Info("[TimeManagementSystem] System initialized");
        SetupDefaultRoutes();
    }

    void TimeManagementSystem::Update(float deltaTime) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_update);
        
        TimeClockComponent* clock = GetTimeClockComponent();
        if (!clock || clock->is_paused) return;
        
        // Check for ticks based on intervals and speed multiplier
        if (clock->ShouldTick(TickType::HOURLY, now)) {
            ProcessTick(TickType::HOURLY, clock->current_date);
            clock->UpdateLastTick(TickType::HOURLY, now);
            clock->current_date = clock->current_date.AddHours(1);
        }
        
        if (clock->ShouldTick(TickType::DAILY, now)) {
            ProcessTick(TickType::DAILY, clock->current_date);
            clock->UpdateLastTick(TickType::DAILY, now);
        }
        
        if (clock->ShouldTick(TickType::MONTHLY, now)) {
            ProcessTick(TickType::MONTHLY, clock->current_date);
            clock->UpdateLastTick(TickType::MONTHLY, now);
        }
        
        if (clock->ShouldTick(TickType::YEARLY, now)) {
            ProcessTick(TickType::YEARLY, clock->current_date);
            clock->UpdateLastTick(TickType::YEARLY, now);
        }
        
        m_last_update = now;
    }

    void TimeManagementSystem::Shutdown() {
        core::logging::Logger::Info("[TimeManagementSystem] System shutdown");
        DestroySystemEntities();
    }

    core::threading::ThreadingStrategy TimeManagementSystem::GetThreadingStrategy() const {
        return core::threading::ThreadingStrategy::MAIN_THREAD_ONLY;
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

    core::ecs::EntityID TimeManagementSystem::ScheduleEvent(const std::string& event_id, const GameDate& when,
                                                           TickType tick_type, const std::string& event_data,
                                                           bool repeating, int repeat_hours) {
        core::ecs::EntityID entity = m_access_manager.CreateEntity();
        
        ScheduledEventComponent event;
        event.event_id = event_id;
        event.scheduled_date = when;
        event.tick_type = tick_type;
        event.event_data = event_data;
        event.repeating = repeating;
        event.repeat_interval_hours = repeat_hours;
        
        m_access_manager.AddComponent(entity, event);
        
        messages::EventScheduled msg;
        msg.event_id = event_id;
        msg.scheduled_date = when;
        msg.tick_type = tick_type;
        m_message_bus.Publish(msg);
        
        return entity;
    }

    void TimeManagementSystem::CancelEvent(const std::string& event_id) {
        auto entities = m_access_manager.GetEntitiesWithComponent<ScheduledEventComponent>();
        for (core::ecs::EntityID entity : entities) {
            auto* event = m_access_manager.GetComponent<ScheduledEventComponent>(entity);
            if (event && event->event_id == event_id) {
                m_access_manager.RemoveComponent<ScheduledEventComponent>(entity);
                break;
            }
        }
    }

    void TimeManagementSystem::CancelEvent(core::ecs::EntityID entity_id) {
        m_access_manager.RemoveComponent<ScheduledEventComponent>(entity_id);
    }

    std::vector<core::ecs::EntityID> TimeManagementSystem::GetScheduledEvents() const {
        return m_access_manager.GetEntitiesWithComponent<ScheduledEventComponent>();
    }

    std::vector<core::ecs::EntityID> TimeManagementSystem::GetReadyEvents(const GameDate& current_date) const {
        std::vector<core::ecs::EntityID> ready_events;
        auto entities = m_access_manager.GetEntitiesWithComponent<ScheduledEventComponent>();
        
        for (core::ecs::EntityID entity : entities) {
            auto* event = m_access_manager.GetComponent<ScheduledEventComponent>(entity);
            if (event && event->IsReady(current_date)) {
                ready_events.push_back(entity);
            }
        }
        
        return ready_events;
    }

    // ====================================================================
    // Message System Methods
    // ====================================================================

    core::ecs::EntityID TimeManagementSystem::SendMessage(const std::string& message_id,
                                                         const std::string& from, const std::string& to,
                                                         const std::string& content, MessageType type, bool urgent) {
        core::ecs::EntityID entity = m_access_manager.CreateEntity();
        
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
            message.travel_speed_kmh = urgent ? 4.0 : 2.0; // Historical travel speeds
            
            double hours_needed = message.travel_distance_km / message.travel_speed_kmh;
            message.expected_arrival = message.sent_date.AddHours(static_cast<int>(hours_needed));
        } else {
            message.expected_arrival = message.sent_date.AddDays(urgent ? 1 : 3);
        }
        
        m_access_manager.AddComponent(entity, message);
        return entity;
    }

    std::vector<core::ecs::EntityID> TimeManagementSystem::GetMessagesInTransit() const {
        return m_access_manager.GetEntitiesWithComponent<MessageTransitComponent>();
    }

    std::vector<core::ecs::EntityID> TimeManagementSystem::GetDeliveredMessages(const GameDate& current_date) const {
        std::vector<core::ecs::EntityID> delivered;
        auto entities = m_access_manager.GetEntitiesWithComponent<MessageTransitComponent>();
        
        for (core::ecs::EntityID entity : entities) {
            auto* message = m_access_manager.GetComponent<MessageTransitComponent>(entity);
            if (message && message->IsDelivered()) {
                delivered.push_back(entity);
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
        RouteNetworkComponent* network = GetRouteNetworkComponent();
        return network ? network->GetDistance(from, to) : 0.0;
    }

    // ====================================================================
    // Entity Time Tracking Methods
    // ====================================================================

    void TimeManagementSystem::AddTimeTracking(core::ecs::EntityID entity, const GameDate& creation_date) {
        EntityTimeComponent time_comp(creation_date);
        m_access_manager.AddComponent(entity, time_comp);
    }

    void TimeManagementSystem::RemoveTimeTracking(core::ecs::EntityID entity) {
        m_access_manager.RemoveComponent<EntityTimeComponent>(entity);
    }

    void TimeManagementSystem::UpdateEntityAges() {
        GameDate current_date = GetCurrentDate();
        auto entities = m_access_manager.GetEntitiesWithComponent<EntityTimeComponent>();
        
        for (core::ecs::EntityID entity : entities) {
            auto* time_comp = m_access_manager.GetComponent<EntityTimeComponent>(entity);
            if (time_comp) {
                time_comp->UpdateAge(current_date);
            }
        }
    }

    std::vector<core::ecs::EntityID> TimeManagementSystem::GetTimeTrackedEntities() const {
        return m_access_manager.GetEntitiesWithComponent<EntityTimeComponent>();
    }

    // ====================================================================
    // Performance Monitoring Methods
    // ====================================================================

    TimeManagementSystem::PerformanceReport TimeManagementSystem::GetPerformanceReport() const {
        PerformanceReport report;
        
        TimePerformanceComponent* perf = GetPerformanceComponent();
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
        state["time_scale"] = static_cast<int>(GetTimeScale());
        state["is_paused"] = IsPaused();
    }

    void TimeManagementSystem::LoadState(const Json::Value& state) {
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
        // Create time clock entity
        m_time_clock_entity = m_access_manager.CreateEntity();
        TimeClockComponent clock_component(start_date);
        m_access_manager.AddComponent(m_time_clock_entity, clock_component);

        // Create route network entity
        m_route_network_entity = m_access_manager.CreateEntity();
        RouteNetworkComponent network_component;
        m_access_manager.AddComponent(m_route_network_entity, network_component);
        
        // Create performance tracking entity
        m_performance_entity = m_access_manager.CreateEntity();
        TimePerformanceComponent perf_component;
        m_access_manager.AddComponent(m_performance_entity, perf_component);
    }

    void TimeManagementSystem::DestroySystemEntities() {
        if (m_time_clock_entity != 0) {
            m_access_manager.RemoveEntity(m_time_clock_entity);
        }
        if (m_route_network_entity != 0) {
            m_access_manager.RemoveEntity(m_route_network_entity);
        }
        if (m_performance_entity != 0) {
            m_access_manager.RemoveEntity(m_performance_entity);
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
        
        for (core::ecs::EntityID entity : ready_events) {
            auto* event = m_access_manager.GetComponent<ScheduledEventComponent>(entity);
            if (event) {
                ExecuteEvent(*event);
                
                if (event->repeating && event->repeat_interval_hours > 0) {
                    // Reschedule repeating event
                    event->scheduled_date = event->scheduled_date.AddHours(event->repeat_interval_hours);
                } else {
                    // Remove one-time event
                    m_access_manager.RemoveComponent<ScheduledEventComponent>(entity);
                }
            }
        }
    }

    void TimeManagementSystem::ProcessMessageTransit() {
        GameDate current_date = GetCurrentDate();
        auto message_entities = GetMessagesInTransit();
        
        for (core::ecs::EntityID entity : message_entities) {
            auto* message = m_access_manager.GetComponent<MessageTransitComponent>(entity);
            if (message && current_date >= message->expected_arrival) {
                DeliverMessage(*message);
                m_access_manager.RemoveComponent<MessageTransitComponent>(entity);
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
        
        core::logging::Logger::Info("[TimeManagementSystem] Event executed: " + event.event_id);
    }

    void TimeManagementSystem::DeliverMessage(const MessageTransitComponent& message) {
        messages::MessageDelivered msg;
        msg.message_id = message.message_id;
        msg.delivery_date = GetCurrentDate();
        msg.from_location = message.from_location;
        msg.to_location = message.to_location;
        m_message_bus.Publish(msg);
        
        core::logging::Logger::Info("[TimeManagementSystem] Message delivered: " + message.message_id);
    }

    TimeClockComponent* TimeManagementSystem::GetTimeClockComponent() {
        return m_access_manager.GetComponent<TimeClockComponent>(m_time_clock_entity);
    }

    const TimeClockComponent* TimeManagementSystem::GetTimeClockComponent() const {
        return m_access_manager.GetComponent<TimeClockComponent>(m_time_clock_entity);
    }

    RouteNetworkComponent* TimeManagementSystem::GetRouteNetworkComponent() {
        return m_access_manager.GetComponent<RouteNetworkComponent>(m_route_network_entity);
    }

    TimePerformanceComponent* TimeManagementSystem::GetPerformanceComponent() {
        return m_access_manager.GetComponent<TimePerformanceComponent>(m_performance_entity);
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
