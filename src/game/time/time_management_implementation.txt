// ============================================================================
// Mechanica Imperii - Time Management System Implementation
// Created: September 22, 2025, 10:45 AM
// Location: src/game/time/TimeManagementSystem.cpp
// ============================================================================

#include "game/time/TimeManagementSystem.h"
#include "game/gameplay/GameplayCoordinator.h"
#include "core/Types/TypeRegistry.h"
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <cassert>
#include <sstream>
#include <iomanip>

namespace game::time {

    // ========================================================================
    // GameDate Implementation
    // ========================================================================

    GameDate::GameDate(int y, int m, int d, int h) : year(y), month(m), day(d), hour(h) {}

    bool GameDate::operator<(const GameDate& other) const {
        if (year != other.year) return year < other.year;
        if (month != other.month) return month < other.month;
        if (day != other.day) return day < other.day;
        return hour < other.hour;
    }

    bool GameDate::operator==(const GameDate& other) const {
        return year == other.year && month == other.month && 
               day == other.day && hour == other.hour;
    }

    bool GameDate::operator!=(const GameDate& other) const {
        return !(*this == other);
    }

    bool GameDate::operator<=(const GameDate& other) const {
        return *this < other || *this == other;
    }

    bool GameDate::operator>(const GameDate& other) const {
        return !(*this <= other);
    }

    bool GameDate::operator>=(const GameDate& other) const {
        return !(*this < other);
    }

    GameDate GameDate::AddHours(int hours) const {
        GameDate result = *this;
        result.hour += hours;
        
        while (result.hour >= 24) {
            result.hour -= 24;
            result.day++;
        }
        while (result.hour < 0) {
            result.hour += 24;
            result.day--;
        }
        
        // Handle day overflow/underflow
        while (result.day > GetDaysInMonth()) {
            result.day -= GetDaysInMonth();
            result.month++;
            if (result.month > 12) {
                result.month = 1;
                result.year++;
            }
        }
        while (result.day < 1) {
            result.month--;
            if (result.month < 1) {
                result.month = 12;
                result.year--;
            }
            result.day += GetDaysInMonth();
        }
        
        return result;
    }

    GameDate GameDate::AddDays(int days) const {
        return AddHours(days * 24);
    }

    GameDate GameDate::AddMonths(int months) const {
        GameDate result = *this;
        result.month += months;
        
        while (result.month > 12) {
            result.month -= 12;
            result.year++;
        }
        while (result.month < 1) {
            result.month += 12;
            result.year--;
        }
        
        // Adjust day if it exceeds days in new month
        int days_in_month = result.GetDaysInMonth();
        if (result.day > days_in_month) {
            result.day = days_in_month;
        }
        
        return result;
    }

    GameDate GameDate::AddYears(int years) const {
        GameDate result = *this;
        result.year += years;
        
        // Handle leap year edge case for Feb 29
        if (result.month == 2 && result.day == 29 && !result.IsLeapYear()) {
            result.day = 28;
        }
        
        return result;
    }

    std::chrono::system_clock::time_point GameDate::ToTimePoint() const {
        // Convert game date to time point for scheduling
        // Base year 1066, simplified calendar
        int64_t total_hours = static_cast<int64_t>(year - 1066) * 365 * 24 +
                             static_cast<int64_t>(month - 1) * 30 * 24 +
                             static_cast<int64_t>(day - 1) * 24 + 
                             static_cast<int64_t>(hour);
        
        auto duration = std::chrono::hours(total_hours);
        return std::chrono::system_clock::time_point(duration);
    }

    GameDate GameDate::FromTimePoint(const std::chrono::system_clock::time_point& tp) {
        auto duration = tp.time_since_epoch();
        auto hours = std::chrono::duration_cast<std::chrono::hours>(duration).count();

        int year = 1066 + static_cast<int>(hours / (365 * 24));
        hours %= (365 * 24);

        int month = 1 + static_cast<int>(hours / (30 * 24));
        hours %= (30 * 24);

        int day = 1 + static_cast<int>(hours / 24);
        int hour_val = static_cast<int>(hours % 24);

        return GameDate(year, month, day, hour_val);
    }

    std::string GameDate::ToString() const {
        static const char* month_names[] = {
            "", "January", "February", "March", "April", "May", "June",
            "July", "August", "September", "October", "November", "December"
        };
        
        std::ostringstream oss;
        oss << day;
        
        // Add ordinal suffix
        if (day >= 10 && day <= 20) oss << "th";
        else if (day % 10 == 1) oss << "st";
        else if (day % 10 == 2) oss << "nd";
        else if (day % 10 == 3) oss << "rd";
        else oss << "th";
        
        oss << " " << month_names[month] << " " << year;
        
        if (hour != 0) {
            oss << ", " << std::setfill('0') << std::setw(2) << hour << ":00";
        }
        
        return oss.str();
    }

    std::string GameDate::ToShortString() const {
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(2) << day << "/"
            << std::setw(2) << month << "/" << year;
        return oss.str();
    }

    int GameDate::GetDaysInMonth() const {
        static const int days[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if (month == 2 && IsLeapYear()) {
            return 29;
        }
        return days[month];
    }

    int GameDate::GetDayOfYear() const {
        int day_of_year = day;
        for (int m = 1; m < month; ++m) {
            GameDate temp_date(year, m, 1);
            day_of_year += temp_date.GetDaysInMonth();
        }
        return day_of_year;
    }

    bool GameDate::IsLeapYear() const {
        // Simplified leap year calculation for medieval period
        return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    }

    std::string GameDate::GetMonthName() const {
        static const char* month_names[] = {
            "", "January", "February", "March", "April", "May", "June",
            "July", "August", "September", "October", "November", "December"
        };
        return month_names[month];
    }

    std::string GameDate::GetSeasonName() const {
        if (month >= 3 && month <= 5) return "Spring";
        if (month >= 6 && month <= 8) return "Summer";
        if (month >= 9 && month <= 11) return "Autumn";
        return "Winter";
    }

    // ========================================================================
    // TimeEvent Implementation
    // ========================================================================

    bool TimeEvent::operator<(const TimeEvent& other) const {
        if (scheduled_time == other.scheduled_time) {
            return priority < other.priority; // Higher priority first (reverse for min-heap)
        }
        return scheduled_time > other.scheduled_time; // Earlier time first (reverse for min-heap)
    }

    // ========================================================================
    // Component Implementations
    // ========================================================================

    ComponentTypeID TimeComponent::GetTypeID() const {
        return GetStaticTypeID();
    }

    ComponentTypeID TimeComponent::GetStaticTypeID() {
        return core::types::TypeRegistry::GetComponentTypeID<TimeComponent>();
    }

    std::unique_ptr<core::ecs::IComponent> TimeComponent::Clone() const {
        return std::make_unique<TimeComponent>(*this);
    }

    ComponentTypeID ScheduledEventComponent::GetTypeID() const {
        return GetStaticTypeID();
    }

    ComponentTypeID ScheduledEventComponent::GetStaticTypeID() {
        return core::types::TypeRegistry::GetComponentTypeID<ScheduledEventComponent>();
    }

    std::unique_ptr<core::ecs::IComponent> ScheduledEventComponent::Clone() const {
        return std::make_unique<ScheduledEventComponent>(*this);
    }

    ComponentTypeID MessageComponent::GetTypeID() const {
        return GetStaticTypeID();
    }

    ComponentTypeID MessageComponent::GetStaticTypeID() {
        return core::types::TypeRegistry::GetComponentTypeID<MessageComponent>();
    }

    std::unique_ptr<core::ecs::IComponent> MessageComponent::Clone() const {
        return std::make_unique<MessageComponent>(*this);
    }

    // ========================================================================
    // RouteNetwork Implementation
    // ========================================================================

    RouteNetwork::RouteNetwork() {
        // Initialize with basic European routes for historical accuracy
        // Major medieval trade and communication routes
        
        // English Channel routes
        AddRoute("London", "Calais", 180);
        AddRoute("Dover", "Calais", 35);
        
        // French internal routes
        AddRoute("Paris", "Orleans", 130);
        AddRoute("Paris", "Rheims", 145);
        AddRoute("Orleans", "Poitiers", 150);
        
        // Germanic routes
        AddRoute("Cologne", "Mainz", 190);
        AddRoute("Mainz", "Nuremberg", 220);
        AddRoute("Nuremberg", "Vienna", 430);
        
        // Mediterranean routes
        AddRoute("Barcelona", "Marseilles", 350);
        AddRoute("Genoa", "Pisa", 160);
        AddRoute("Rome", "Naples", 230);
        
        // Eastern European routes
        AddRoute("Vienna", "Prague", 290);
        AddRoute("Prague", "Krakow", 540);
        AddRoute("Kiev", "Novgorod", 860);
    }

    void RouteNetwork::AddRoute(const std::string& from, const std::string& to, double distance_km) {
        std::lock_guard<std::mutex> lock(m_route_mutex);
        
        m_route_network[from].push_back(to);
        m_route_network[to].push_back(from); // Bidirectional
        
        std::string route_key1 = from + "_" + to;
        std::string route_key2 = to + "_" + from;
        
        m_distances[route_key1] = distance_km;
        m_distances[route_key2] = distance_km;
        
        // Default quality and availability
        m_route_quality[route_key1] = 1.0;
        m_route_quality[route_key2] = 1.0;
        m_route_availability[route_key1] = {1000, 9999}; // Available throughout game period
        m_route_availability[route_key2] = {1000, 9999};
    }

    void RouteNetwork::RemoveRoute(const std::string& from, const std::string& to) {
        std::lock_guard<std::mutex> lock(m_route_mutex);
        
        // Remove from network
        auto& from_routes = m_route_network[from];
        from_routes.erase(std::remove(from_routes.begin(), from_routes.end(), to), from_routes.end());
        
        auto& to_routes = m_route_network[to];
        to_routes.erase(std::remove(to_routes.begin(), to_routes.end(), from), to_routes.end());
        
        // Remove distance and quality data
        std::string route_key1 = from + "_" + to;
        std::string route_key2 = to + "_" + from;
        
        m_distances.erase(route_key1);
        m_distances.erase(route_key2);
        m_route_quality.erase(route_key1);
        m_route_quality.erase(route_key2);
        m_route_availability.erase(route_key1);
        m_route_availability.erase(route_key2);
    }

    double RouteNetwork::GetDistance(const std::string& from, const std::string& to) const {
        std::lock_guard<std::mutex> lock(m_route_mutex);
        
        std::string route_key = from + "_" + to;
        auto it = m_distances.find(route_key);
        
        if (it != m_distances.end()) {
            return it->second;
        }
        
        // If direct route doesn't exist, try to find path through network
        auto route = FindRoute(from, to);
        if (route.size() > 1) {
            double total_distance = 0.0;
            for (size_t i = 0; i < route.size() - 1; ++i) {
                std::string segment_key = route[i] + "_" + route[i + 1];
                auto segment_it = m_distances.find(segment_key);
                if (segment_it != m_distances.end()) {
                    total_distance += segment_it->second;
                }
            }
            return total_distance;
        }
        
        // Default distance if no route found (as the crow flies estimate)
        return 500.0;
    }

    std::vector<std::string> RouteNetwork::FindRoute(const std::string& from, const std::string& to) const {
        std::lock_guard<std::mutex> lock(m_route_mutex);
        
        // Simple breadth-first search for shortest path
        std::queue<std::vector<std::string>> paths;
        std::unordered_set<std::string> visited;
        
        paths.push({from});
        visited.insert(from);
        
        while (!paths.empty()) {
            auto current_path = paths.front();
            paths.pop();
            
            const std::string& current_location = current_path.back();
            
            if (current_location == to) {
                return current_path;
            }
            
            auto network_it = m_route_network.find(current_location);
            if (network_it != m_route_network.end()) {
                for (const std::string& neighbor : network_it->second) {
                    if (visited.find(neighbor) == visited.end()) {
                        visited.insert(neighbor);
                        auto new_path = current_path;
                        new_path.push_back(neighbor);
                        paths.push(new_path);
                    }
                }
            }
        }
        
        return {}; // No route found
    }

    void RouteNetwork::SetRouteQuality(const std::string& from, const std::string& to, double quality) {
        std::lock_guard<std::mutex> lock(m_route_mutex);
        
        quality = std::clamp(quality, 0.1, 2.0); // Clamp to reasonable range
        
        std::string route_key1 = from + "_" + to;
        std::string route_key2 = to + "_" + from;
        
        m_route_quality[route_key1] = quality;
        m_route_quality[route_key2] = quality;
    }

    void RouteNetwork::SetSeasonalModifier(const std::string& route_id, double winter_modifier) {
        // This would be implemented with seasonal data storage
        // For now, just validate the input
        winter_modifier = std::clamp(winter_modifier, 0.1, 3.0);
    }

    void RouteNetwork::SetRouteAvailability(const std::string& from, const std::string& to, int start_year, int end_year) {
        std::lock_guard<std::mutex> lock(m_route_mutex);
        
        std::string route_key1 = from + "_" + to;
        std::string route_key2 = to + "_" + from;
        
        m_route_availability[route_key1] = {start_year, end_year};
        m_route_availability[route_key2] = {start_year, end_year};
    }

    bool RouteNetwork::IsRouteAvailable(const std::string& from, const std::string& to, int year) const {
        std::lock_guard<std::mutex> lock(m_route_mutex);
        
        std::string route_key = from + "_" + to;
        auto it = m_route_availability.find(route_key);
        
        if (it != m_route_availability.end()) {
            return year >= it->second.first && year <= it->second.second;
        }
        
        return true; // Default to available if not specified
    }

    // ========================================================================
    // MessageDeliverySystem Implementation
    // ========================================================================

    MessageDeliverySystem::MessageDeliverySystem() {
        // Initialize type multipliers for different message types
        m_type_multipliers[Message::Type::DIPLOMATIC] = 0.8;    // Faster (priority)
        m_type_multipliers[Message::Type::TRADE] = 1.2;         // Slower (merchants)
        m_type_multipliers[Message::Type::MILITARY] = 0.6;      // Fastest (couriers)
        m_type_multipliers[Message::Type::INTELLIGENCE] = 0.7;  // Fast (spies)
        m_type_multipliers[Message::Type::PERSONAL] = 1.5;      // Slowest (regular mail)
        m_type_multipliers[Message::Type::ADMINISTRATIVE] = 1.0; // Standard
        m_type_multipliers[Message::Type::RELIGIOUS] = 1.3;     // Slow (pilgrims)
    }

    void MessageDeliverySystem::SendMessage(const Message& message) {
        std::lock_guard<std::mutex> lock(m_message_mutex);
        
        Message msg = message;
        
        // Calculate delivery time
        double distance = 0.0;
        if (m_route_network) {
            distance = m_route_network->GetDistance(message.from_location, message.to_location);
        } else {
            distance = 500.0; // Default distance
        }
        
        // Base time calculation
        double base_time_days = distance / m_base_delivery_speed;
        
        // Apply type multiplier
        auto type_it = m_type_multipliers.find(message.type);
        double type_multiplier = (type_it != m_type_multipliers.end()) ? type_it->second : 1.0;
        
        double delivery_days = base_time_days * type_multiplier * m_seasonal_modifier;
        
        // Apply urgency modifier
        if (message.is_urgent) {
            delivery_days *= 0.5; // Urgent messages travel at double speed
        }
        
        // Add some randomness for realism
        utils::RandomGenerator& rng = utils::RandomGenerator::Instance();
        double variance = rng.GetFloat(0.8f, 1.2f);
        delivery_days *= variance;
        
        // Calculate arrival date
        auto sent_tp = message.sent_date.ToTimePoint();
        auto arrival_tp = sent_tp + std::chrono::hours(static_cast<int64_t>(delivery_days * 24));
        msg.arrival_date = GameDate::FromTimePoint(arrival_tp);
        
        m_messages_in_transit.push_back(msg);
    }

    std::vector<Message> MessageDeliverySystem::ProcessDeliveries(const GameDate& current_date) {
        std::lock_guard<std::mutex> lock(m_message_mutex);
        
        std::vector<Message> delivered_messages;
        
        auto it = m_messages_in_transit.begin();
        while (it != m_messages_in_transit.end()) {
            if (current_date >= it->arrival_date) {
                delivered_messages.push_back(*it);
                
                // Execute delivery callback if present
                if (it->on_delivery) {
                    it->on_delivery();
                }
                
                it = m_messages_in_transit.erase(it);
            } else {
                ++it;
            }
        }
        
        return delivered_messages;
    }

    void MessageDeliverySystem::CancelMessage(const std::string& message_id) {
        std::lock_guard<std::mutex> lock(m_message_mutex);
        
        m_messages_in_transit.erase(
            std::remove_if(m_messages_in_transit.begin(), m_messages_in_transit.end(),
                          [&message_id](const Message& msg) { return msg.id == message_id; }),
            m_messages_in_transit.end());
    }

    void MessageDeliverySystem::SetBaseDeliverySpeed(double km_per_day) {
        m_base_delivery_speed = std::clamp(km_per_day, 10.0, 200.0); // Reasonable range
    }

    void MessageDeliverySystem::SetTypeMultiplier(Message::Type type, double multiplier) {
        m_type_multipliers[type] = std::clamp(multiplier, 0.1, 5.0);
    }

    void MessageDeliverySystem::SetSeasonalModifiers(const GameDate& date) {
        // Winter travel is slower
        std::string season = date.GetSeasonName();
        if (season == "Winter") {
            m_seasonal_modifier = 1.5; // 50% slower
        } else if (season == "Spring") {
            m_seasonal_modifier = 1.2; // Muddy roads
        } else {
            m_seasonal_modifier = 1.0; // Normal speed
        }
    }

    void MessageDeliverySystem::SetRouteNetwork(std::shared_ptr<RouteNetwork> network) {
        m_route_network = network;
    }

    std::vector<Message> MessageDeliverySystem::GetMessagesInTransit() const {
        std::lock_guard<std::mutex> lock(m_message_mutex);
        return m_messages_in_transit;
    }

    int MessageDeliverySystem::GetMessageCount() const {
        std::lock_guard<std::mutex> lock(m_message_mutex);
        return static_cast<int>(m_messages_in_transit.size());
    }

    double MessageDeliverySystem::GetAverageDeliveryTime() const {
        // This would calculate based on historical delivery data
        // For now, return estimated average
        return m_base_delivery_speed > 0 ? (500.0 / m_base_delivery_speed) : 10.0;
    }

// ========================================================================
    // GameClock Implementation
    // ========================================================================

    GameClock::GameClock(const GameDate& start_date) : m_current_date(start_date) {
        auto now = std::chrono::steady_clock::now();
        m_last_update = now;
        m_last_hourly_tick = now;
        m_last_daily_tick = now;
        m_last_monthly_tick = now;
        m_last_yearly_tick = now;

        // Initialize speed multipliers
        m_speed_multipliers = {
            {TimeScale::PAUSED, 0.0},
            {TimeScale::SLOW, 0.5},
            {TimeScale::NORMAL, 1.0},
            {TimeScale::FAST, 3.0},
            {TimeScale::VERY_FAST, 7.0},
            {TimeScale::ULTRA_FAST, 15.0}
        };
    }

    void GameClock::SetTimeScale(TimeScale scale) {
        if (m_time_scale != scale) {
            TimeScale old_scale = m_time_scale;
            m_time_scale = scale;
            
            // Reset timing when changing scales to avoid jumps
            auto now = std::chrono::steady_clock::now();
            m_last_update = now;
        }
    }

    TimeScale GameClock::GetTimeScale() const {
        return m_time_scale;
    }

    void GameClock::Pause() {
        SetTimeScale(TimeScale::PAUSED);
    }

    void GameClock::Resume() {
        if (m_time_scale == TimeScale::PAUSED) {
            SetTimeScale(TimeScale::NORMAL);
        }
    }

    bool GameClock::IsPaused() const {
        return m_time_scale == TimeScale::PAUSED;
    }

    GameDate GameClock::GetCurrentDate() const {
        return m_current_date;
    }

    void GameClock::SetCurrentDate(const GameDate& date) {
        m_current_date = date;
        
        // Reset all tick timers when manually setting date
        auto now = std::chrono::steady_clock::now();
        m_last_update = now;
        m_last_hourly_tick = now;
        m_last_daily_tick = now;
        m_last_monthly_tick = now;
        m_last_yearly_tick = now;
    }

    GameDate GameClock::GetFutureDate(int hours, int days, int months, int years) const {
        return m_current_date.AddHours(hours).AddDays(days).AddMonths(months).AddYears(years);
    }

    GameClock::TickResult GameClock::Update() {
        TickResult result;
        result.current_date = m_current_date;

        if (m_time_scale == TimeScale::PAUSED) {
            return result;
        }

        auto now = std::chrono::steady_clock::now();
        auto delta = now - m_last_update;
        m_last_update = now;

        result.delta_time_ms = std::chrono::duration_cast<std::chrono::microseconds>(delta).count() / 1000.0;

        // Get current speed multiplier
        double speed_multiplier = GetCurrentSpeedMultiplier();
        if (speed_multiplier <= 0.0) {
            return result;
        }

        // Check for hourly tick
        auto hourly_threshold = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            m_hourly_interval / speed_multiplier);
        if (now - m_last_hourly_tick >= hourly_threshold) {
            result.hourly_tick = true;
            m_last_hourly_tick = now;
            AdvanceTime(1, 0, 0, 0); // 1 hour
        }

        // Check for daily tick
        auto daily_threshold = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            m_daily_interval / speed_multiplier);
        if (now - m_last_daily_tick >= daily_threshold) {
            result.daily_tick = true;
            m_last_daily_tick = now;
            if (!result.hourly_tick) {
                AdvanceTime(0, 1, 0, 0); // 1 day
            }
        }

        // Check for monthly tick
        auto monthly_threshold = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            m_monthly_interval / speed_multiplier);
        if (now - m_last_monthly_tick >= monthly_threshold) {
            result.monthly_tick = true;
            m_last_monthly_tick = now;
            if (!result.daily_tick && !result.hourly_tick) {
                AdvanceTime(0, 0, 1, 0); // 1 month
            }
        }

        // Check for yearly tick
        auto yearly_threshold = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            m_yearly_interval / speed_multiplier);
        if (now - m_last_yearly_tick >= yearly_threshold) {
            result.yearly_tick = true;
            m_last_yearly_tick = now;
            if (!result.monthly_tick && !result.daily_tick && !result.hourly_tick) {
                AdvanceTime(0, 0, 0, 1); // 1 year
            }
        }

        result.current_date = m_current_date;
        return result;
    }

    void GameClock::ForceAdvanceTime(int hours, int days, int months, int years) {
        AdvanceTime(hours, days, months, years);
    }

    void GameClock::SetTickIntervals(std::chrono::milliseconds hourly, 
                                    std::chrono::milliseconds daily,
                                    std::chrono::milliseconds monthly,
                                    std::chrono::milliseconds yearly) {
        m_hourly_interval = hourly;
        m_daily_interval = daily;
        m_monthly_interval = monthly;
        m_yearly_interval = yearly;
    }

    void GameClock::SetGameplayCoordinator(game::gameplay::GameplayCoordinator* coordinator) {
        m_gameplay_coordinator = coordinator;
    }

    void GameClock::AdvanceTime(int hours, int days, int months, int years) {
        if (hours > 0) m_current_date = m_current_date.AddHours(hours);
        if (days > 0) m_current_date = m_current_date.AddDays(days);
        if (months > 0) m_current_date = m_current_date.AddMonths(months);
        if (years > 0) m_current_date = m_current_date.AddYears(years);
    }

    double GameClock::GetCurrentSpeedMultiplier() const {
        auto it = m_speed_multipliers.find(m_time_scale);
        double base_multiplier = (it != m_speed_multipliers.end()) ? it->second : 1.0;

        // Integrate with gameplay coordinator for dynamic speed adjustment
        if (m_gameplay_coordinator) {
            // This would call into your gameplay coordinator to get
            // the current time acceleration factor from quiet period management
            // For now, just return the base multiplier
        }

        return base_multiplier;
    }

    // ========================================================================
    // TimeEventScheduler Implementation
    // ========================================================================

    void TimeEventScheduler::ScheduleEvent(const TimeEvent& event) {
        std::lock_guard<std::mutex> lock(m_event_mutex);
        m_scheduled_events.push(event);
    }

    void TimeEventScheduler::ScheduleEvent(const std::string& id, const GameDate& when,
                                          std::function<void()> callback, TickType tick_type,
                                          bool requires_player_attention) {
        TimeEvent event;
        event.id = id;
        event.scheduled_time = when.ToTimePoint();
        event.callback = callback;
        event.required_tick_type = tick_type;
        event.requires_player_attention = requires_player_attention;
        event.priority = requires_player_attention ? 100 : 0; // High priority for player events

        ScheduleEvent(event);
    }

    void TimeEventScheduler::ScheduleRecurringEvent(const std::string& id, const GameDate& first_occurrence,
                                                   std::chrono::duration<int64_t> interval,
                                                   std::function<void()> callback, TickType tick_type) {
        TimeEvent event;
        event.id = id;
        event.scheduled_time = first_occurrence.ToTimePoint();
        event.callback = callback;
        event.required_tick_type = tick_type;
        event.repeating = true;
        event.repeat_interval = interval;
        event.category = "recurring";

        ScheduleEvent(event);
    }

    void TimeEventScheduler::CancelEvent(const std::string& event_id) {
        std::lock_guard<std::mutex> lock(m_event_mutex);
        
        // Create a new priority queue without the cancelled event
        std::priority_queue<TimeEvent> new_queue;
        std::priority_queue<TimeEvent> temp_queue = m_scheduled_events;

        while (!temp_queue.empty()) {
            const auto& event = temp_queue.top();
            if (event.id != event_id) {
                new_queue.push(event);
            }
            temp_queue.pop();
        }

        m_scheduled_events = new_queue;
    }

    void TimeEventScheduler::CancelEventsByCategory(const std::string& category) {
        std::lock_guard<std::mutex> lock(m_event_mutex);
        
        std::priority_queue<TimeEvent> new_queue;
        std::priority_queue<TimeEvent> temp_queue = m_scheduled_events;

        while (!temp_queue.empty()) {
            const auto& event = temp_queue.top();
            if (event.category != category) {
                new_queue.push(event);
            }
            temp_queue.pop();
        }

        m_scheduled_events = new_queue;
    }

    void TimeEventScheduler::ModifyEvent(const std::string& event_id, const GameDate& new_time) {
        std::lock_guard<std::mutex> lock(m_event_mutex);
        
        // Find and reschedule the event
        std::priority_queue<TimeEvent> new_queue;
        std::priority_queue<TimeEvent> temp_queue = m_scheduled_events;
        bool found = false;

        while (!temp_queue.empty()) {
            auto event = temp_queue.top();
            if (event.id == event_id && !found) {
                event.scheduled_time = new_time.ToTimePoint();
                found = true;
            }
            new_queue.push(event);
            temp_queue.pop();
        }

        m_scheduled_events = new_queue;
    }

    void TimeEventScheduler::RegisterTickCallback(TickType tick_type, std::function<void()> callback) {
        std::lock_guard<std::mutex> lock(m_event_mutex);
        m_tick_callbacks[tick_type].push_back(callback);
    }

    void TimeEventScheduler::UnregisterTickCallback(TickType tick_type, const std::string& callback_id) {
        std::lock_guard<std::mutex> lock(m_event_mutex);
        
        // Remove callback from registry
        m_callback_registry.erase(callback_id);
        
        // Note: This is a simplified implementation
        // A full implementation would track callback IDs properly
    }

    void TimeEventScheduler::ProcessEvents(TickType tick_type, const GameDate& current_date) {
        std::vector<TimeEvent> events_to_reschedule;
        
        {
            std::lock_guard<std::mutex> lock(m_event_mutex);
            
            // Process scheduled events
            while (!m_scheduled_events.empty()) {
                const auto& event = m_scheduled_events.top();
                auto event_date = GameDate::FromTimePoint(event.scheduled_time);

                // Check if event should trigger
                if (event_date <= current_date) {
                    if (event.required_tick_type == tick_type) {
                        // Check if event should be delegated
                        if (ShouldDelegateEvent(event)) {
                            HandleDelegatedEvent(event);
                        } else {
                            ExecuteEvent(event);
                        }

                        // Handle repeating events
                        if (event.repeating) {
                            TimeEvent repeat_event = event;
                            repeat_event.scheduled_time += event.repeat_interval;
                            events_to_reschedule.push_back(repeat_event);
                        }
                    }

                    m_scheduled_events.pop();
                } else {
                    break; // Events are sorted by time
                }
            }
        }

        // Reschedule repeating events (outside of lock)
        for (const auto& event : events_to_reschedule) {
            ScheduleEvent(event);
        }

        // Execute tick callbacks
        {
            std::lock_guard<std::mutex> lock(m_event_mutex);
            auto callback_it = m_tick_callbacks.find(tick_type);
            if (callback_it != m_tick_callbacks.end()) {
                for (const auto& callback : callback_it->second) {
                    try {
                        callback();
                    } catch (const std::exception& e) {
                        std::cerr << "Error in tick callback: " << e.what() << std::endl;
                    }
                }
            }
        }

        // Process message deliveries for daily ticks
        if (tick_type == TickType::DAILY && m_message_system) {
            auto delivered_messages = m_message_system->ProcessDeliveries(current_date);
            for (const auto& message : delivered_messages) {
                ProcessDeliveredMessage(message);
            }
        }
    }

    std::vector<TimeEvent> TimeEventScheduler::GetUpcomingEvents(int count) const {
        std::lock_guard<std::mutex> lock(m_event_mutex);
        
        std::vector<TimeEvent> upcoming;
        std::priority_queue<TimeEvent> temp_queue = m_scheduled_events;

        for (int i = 0; i < count && !temp_queue.empty(); ++i) {
            upcoming.push_back(temp_queue.top());
            temp_queue.pop();
        }

        return upcoming;
    }

    std::vector<TimeEvent> TimeEventScheduler::GetEventsByCategory(const std::string& category) const {
        std::lock_guard<std::mutex> lock(m_event_mutex);
        
        std::vector<TimeEvent> category_events;
        std::priority_queue<TimeEvent> temp_queue = m_scheduled_events;

        while (!temp_queue.empty()) {
            const auto& event = temp_queue.top();
            if (event.category == category) {
                category_events.push_back(event);
            }
            temp_queue.pop();
        }

        return category_events;
    }

    bool TimeEventScheduler::HasPendingEvents() const {
        std::lock_guard<std::mutex> lock(m_event_mutex);
        return !m_scheduled_events.empty();
    }

    int TimeEventScheduler::GetEventCount() const {
        std::lock_guard<std::mutex> lock(m_event_mutex);
        return static_cast<int>(m_scheduled_events.size());
    }

    void TimeEventScheduler::SetGameplayCoordinator(game::gameplay::GameplayCoordinator* coordinator) {
        m_gameplay_coordinator = coordinator;
    }

    void TimeEventScheduler::SetMessageSystem(MessageDeliverySystem* message_system) {
        m_message_system = message_system;
    }

    bool TimeEventScheduler::ShouldDelegateEvent(const TimeEvent& event) const {
        if (!event.can_be_delegated || !m_gameplay_coordinator) {
            return false;
        }

        // Check with gameplay coordinator if this event should be delegated
        // This would integrate with your delegation system
        return false; // Placeholder - implement based on your GameplayCoordinator
    }

    void TimeEventScheduler::HandleDelegatedEvent(const TimeEvent& event) {
        // Handle event through delegation system
        // This would integrate with your gameplay coordinator
        if (m_gameplay_coordinator) {
            // Delegate the event to AI systems
            ExecuteEvent(event); // For now, just execute normally
        }
    }

    void TimeEventScheduler::ExecuteEvent(const TimeEvent& event) {
        try {
            if (event.callback) {
                event.callback();
            }
        } catch (const std::exception& e) {
            std::cerr << "Error executing event " << event.id << ": " << e.what() << std::endl;
        }
    }

    void TimeEventScheduler::ProcessDeliveredMessage(const Message& message) {
        // Process delivered message - trigger diplomatic events, trade notifications, etc.
        if (message.requires_response) {
            // Schedule a response decision event
            ScheduleEvent("response_to_" + message.id,
                         GameDate::FromTimePoint(std::chrono::system_clock::now()),
                         [=]() { HandleMessageResponse(message); },
                         TickType::DAILY, true);
        }
    }

    void TimeEventScheduler::HandleMessageResponse(const Message& message) {
        // Create a decision for the player about how to respond to the message
        // This would integrate with your decision system
        // For now, just log the message response requirement
        std::cout << "Message from " << message.sender_name << " requires response: " << message.content << std::endl;
    }

// ========================================================================
    // TimeManagementSystem Implementation
    // ========================================================================

    TimeManagementSystem::TimeManagementSystem(core::ecs::ComponentAccessManager& access_manager,
                                              core::messaging::ThreadSafeMessageBus& message_bus,
                                              const GameDate& start_date)
        : m_access_manager(access_manager)
        , m_message_bus(message_bus)
        , m_clock(start_date)
        , m_route_network(std::make_shared<RouteNetwork>()) {
        
        m_last_performance_check = std::chrono::steady_clock::now();
        
        // Initialize message system with route network
        m_message_system.SetRouteNetwork(m_route_network);
        m_scheduler.SetMessageSystem(&m_message_system);
        
        // Set up default routes
        SetupDefaultRoutes();
        
        // Register component types with ECS system
        m_access_manager.RegisterComponent<TimeComponent>();
        m_access_manager.RegisterComponent<ScheduledEventComponent>();
        m_access_manager.RegisterComponent<MessageComponent>();
    }

    void TimeManagementSystem::Initialize() {
        // Subscribe to relevant message bus events
        m_message_bus.Subscribe<messages::TimeScaleChanged>([this](const messages::TimeScaleChanged& event) {
            // Handle time scale changes from other systems
            m_clock.SetTimeScale(event.new_scale);
        });
        
        // Set up initial time components for existing entities
        auto entities = m_access_manager.GetEntitiesWithComponent<TimeComponent>();
        for (auto entity_id : entities) {
            auto time_comp = m_access_manager.GetComponent<TimeComponent>(entity_id);
            if (time_comp) {
                time_comp->last_updated = m_clock.GetCurrentDate();
            }
        }
        
        // Schedule initial recurring events
        ScheduleRecurringEvent("monthly_maintenance", m_clock.GetCurrentDate().AddDays(30),
                              std::chrono::hours(24 * 30), // Every 30 days
                              [this]() { PerformMonthlyMaintenance(); },
                              TickType::MONTHLY);
        
        // Set seasonal modifiers
        m_message_system.SetSeasonalModifiers(m_clock.GetCurrentDate());
        
        std::cout << "TimeManagementSystem initialized for date: " 
                  << m_clock.GetCurrentDate().ToString() << std::endl;
    }

    void TimeManagementSystem::Update(float deltaTime) {
        auto tick_result = m_clock.Update();

        // Process ticks in order of frequency
        if (tick_result.hourly_tick) {
            ProcessTick(TickType::HOURLY, tick_result.current_date);
        }

        if (tick_result.daily_tick) {
            ProcessTick(TickType::DAILY, tick_result.current_date);
        }

        if (tick_result.monthly_tick) {
            ProcessTick(TickType::MONTHLY, tick_result.current_date);
            
            // Update seasonal modifiers monthly
            m_message_system.SetSeasonalModifiers(tick_result.current_date);
        }

        if (tick_result.yearly_tick) {
            ProcessTick(TickType::YEARLY, tick_result.current_date);
        }

        // Update age for entities with time components
        UpdateEntityAges(tick_result);
        
        // Update message delivery progress
        UpdateMessageProgress();
        
        // Update performance monitoring
        UpdatePerformanceMetrics();
        
        // Publish date change event if time advanced
        if (tick_result.hourly_tick || tick_result.daily_tick || 
            tick_result.monthly_tick || tick_result.yearly_tick) {
            
            messages::DateChanged date_event;
            date_event.old_date = m_clock.GetCurrentDate(); // This would need previous date tracking
            date_event.new_date = tick_result.current_date;
            date_event.reason = "natural_progression";
            
            m_message_bus.Publish(date_event);
        }
    }

    void TimeManagementSystem::Shutdown() {
        // Cancel all pending events
        while (m_scheduler.HasPendingEvents()) {
            // Clear events by category
            m_scheduler.CancelEventsByCategory("system");
            m_scheduler.CancelEventsByCategory("recurring");
        }
        
        // Clear message queue
        auto messages = m_message_system.GetMessagesInTransit();
        for (const auto& msg : messages) {
            m_message_system.CancelMessage(msg.id);
        }
        
        std::cout << "TimeManagementSystem shutdown complete" << std::endl;
    }

    core::threading::ThreadingStrategy TimeManagementSystem::GetThreadingStrategy() const {
        return core::threading::ThreadingStrategy::MAIN_THREAD;
    }

    std::string TimeManagementSystem::GetThreadingRationale() const {
        return "Time Management requires main thread execution for UI synchronization, "
               "event scheduling coordination, and message bus integration. "
               "Time progression affects all other systems and needs deterministic ordering.";
    }

    void TimeManagementSystem::Pause() {
        m_clock.Pause();
        
        messages::TimeScaleChanged event;
        event.old_scale = TimeScale::NORMAL; // Would need to track previous
        event.new_scale = TimeScale::PAUSED;
        event.change_time = m_clock.GetCurrentDate();
        
        m_message_bus.Publish(event);
    }

    void TimeManagementSystem::Resume() {
        TimeScale old_scale = m_clock.GetTimeScale();
        m_clock.Resume();
        
        messages::TimeScaleChanged event;
        event.old_scale = old_scale;
        event.new_scale = m_clock.GetTimeScale();
        event.change_time = m_clock.GetCurrentDate();
        
        m_message_bus.Publish(event);
    }

    void TimeManagementSystem::SetTimeScale(TimeScale scale) {
        TimeScale old_scale = m_clock.GetTimeScale();
        m_clock.SetTimeScale(scale);
        
        messages::TimeScaleChanged event;
        event.old_scale = old_scale;
        event.new_scale = scale;
        event.change_time = m_clock.GetCurrentDate();
        
        m_message_bus.Publish(event);
    }

    TimeScale TimeManagementSystem::GetTimeScale() const {
        return m_clock.GetTimeScale();
    }

    bool TimeManagementSystem::IsPaused() const {
        return m_clock.IsPaused();
    }

    GameDate TimeManagementSystem::GetCurrentDate() const {
        return m_clock.GetCurrentDate();
    }

    void TimeManagementSystem::SetCurrentDate(const GameDate& date) {
        GameDate old_date = m_clock.GetCurrentDate();
        m_clock.SetCurrentDate(date);
        
        messages::DateChanged event;
        event.old_date = old_date;
        event.new_date = date;
        event.reason = "manual_set";
        
        m_message_bus.Publish(event);
    }

    GameDate TimeManagementSystem::GetFutureDate(int hours, int days, int months, int years) const {
        return m_clock.GetFutureDate(hours, days, months, years);
    }

    void TimeManagementSystem::ScheduleEvent(const std::string& id, const GameDate& when,
                                            std::function<void()> callback, TickType tick_type) {
        m_scheduler.ScheduleEvent(id, when, callback, tick_type);
        
        messages::EventScheduled event;
        event.event_id = id;
        event.scheduled_date = when;
        event.tick_type = tick_type;
        event.category = "system";
        
        m_message_bus.Publish(event);
    }

    void TimeManagementSystem::ScheduleRecurringEvent(const std::string& id, const GameDate& first_occurrence,
                                                     std::chrono::hours interval_hours,
                                                     std::function<void()> callback, TickType tick_type) {
        auto interval = std::chrono::duration_cast<std::chrono::duration<int64_t>>(interval_hours);
        m_scheduler.ScheduleRecurringEvent(id, first_occurrence, interval, callback, tick_type);
        
        messages::EventScheduled event;
        event.event_id = id;
        event.scheduled_date = first_occurrence;
        event.tick_type = tick_type;
        event.category = "recurring";
        
        m_message_bus.Publish(event);
    }

    void TimeManagementSystem::CancelEvent(const std::string& event_id) {
        m_scheduler.CancelEvent(event_id);
    }

    void TimeManagementSystem::RegisterHourlyCallback(std::function<void()> callback) {
        m_scheduler.RegisterTickCallback(TickType::HOURLY, callback);
    }

    void TimeManagementSystem::RegisterDailyCallback(std::function<void()> callback) {
        m_scheduler.RegisterTickCallback(TickType::DAILY, callback);
    }

    void TimeManagementSystem::RegisterMonthlyCallback(std::function<void()> callback) {
        m_scheduler.RegisterTickCallback(TickType::MONTHLY, callback);
    }

    void TimeManagementSystem::RegisterYearlyCallback(std::function<void()> callback) {
        m_scheduler.RegisterTickCallback(TickType::YEARLY, callback);
    }

    void TimeManagementSystem::SendMessage(const std::string& from, const std::string& to,
                                          const std::string& content, Message::Type type,
                                          bool urgent, std::function<void()> on_delivery) {
        Message msg;
        msg.id = "msg_" + std::to_string(utils::RandomGenerator::Instance().GetInt(100000, 999999));
        msg.from_location = from;
        msg.to_location = to;
        msg.content = content;
        msg.sent_date = GetCurrentDate();
        msg.type = type;
        msg.is_urgent = urgent;
        msg.on_delivery = on_delivery;

        m_message_system.SendMessage(msg);
        
        // Create message entity in ECS
        auto message_entity = m_access_manager.CreateEntity();
        auto message_comp = std::make_unique<MessageComponent>();
        message_comp->message_data = msg;
        message_comp->in_transit = true;
        message_comp->progress = 0.0;
        
        m_access_manager.AddComponent(message_entity, std::move(message_comp));
    }

    void TimeManagementSystem::AddRoute(const std::string& from, const std::string& to, double distance_km) {
        m_route_network->AddRoute(from, to, distance_km);
    }

    TimeManagementSystem::PerformanceReport TimeManagementSystem::GetPerformanceReport() const {
        PerformanceReport report;

        auto hourly_it = m_tick_performance_ms.find(TickType::HOURLY);
        if (hourly_it != m_tick_performance_ms.end()) {
            report.hourly_tick_ms = hourly_it->second;
        }

        auto daily_it = m_tick_performance_ms.find(TickType::DAILY);
        if (daily_it != m_tick_performance_ms.end()) {
            report.daily_tick_ms = daily_it->second;
        }

        auto monthly_it = m_tick_performance_ms.find(TickType::MONTHLY);
        if (monthly_it != m_tick_performance_ms.end()) {
            report.monthly_tick_ms = monthly_it->second;
        }

        auto yearly_it = m_tick_performance_ms.find(TickType::YEARLY);
        if (yearly_it != m_tick_performance_ms.end()) {
            report.yearly_tick_ms = yearly_it->second;
        }

        report.total_update_ms = report.hourly_tick_ms + report.daily_tick_ms + 
                                report.monthly_tick_ms + report.yearly_tick_ms;

        // Performance warning if any tick takes longer than 16ms (60 FPS target)
        report.performance_warning = report.total_update_ms > 16.0;
        
        report.active_events = m_scheduler.GetEventCount();
        report.messages_in_transit = m_message_system.GetMessageCount();

        return report;
    }

    std::vector<TimeEvent> TimeManagementSystem::GetUpcomingEvents(int count) const {
        return m_scheduler.GetUpcomingEvents(count);
    }

    void TimeManagementSystem::SetGameplayCoordinator(game::gameplay::GameplayCoordinator* coordinator) {
        m_gameplay_coordinator = coordinator;
        m_clock.SetGameplayCoordinator(coordinator);
        m_scheduler.SetGameplayCoordinator(coordinator);
    }

    void TimeManagementSystem::SaveState(Json::Value& state) const {
        // Save current date
        state["current_date"]["year"] = m_clock.GetCurrentDate().year;
        state["current_date"]["month"] = m_clock.GetCurrentDate().month;
        state["current_date"]["day"] = m_clock.GetCurrentDate().day;
        state["current_date"]["hour"] = m_clock.GetCurrentDate().hour;
        
        // Save time scale
        state["time_scale"] = static_cast<int>(m_clock.GetTimeScale());
        
        // Save scheduled events (simplified - would need full serialization)
        auto upcoming_events = m_scheduler.GetUpcomingEvents(100);
        for (size_t i = 0; i < upcoming_events.size(); ++i) {
            const auto& event = upcoming_events[i];
            Json::Value event_data;
            event_data["id"] = event.id;
            event_data["category"] = event.category;
            event_data["tick_type"] = static_cast<int>(event.required_tick_type);
            event_data["priority"] = event.priority;
            event_data["repeating"] = event.repeating;
            event_data["serializable_data"] = event.serializable_data;
            
            // Convert scheduled time to string
            auto event_date = GameDate::FromTimePoint(event.scheduled_time);
            event_data["scheduled_year"] = event_date.year;
            event_data["scheduled_month"] = event_date.month;
            event_data["scheduled_day"] = event_date.day;
            event_data["scheduled_hour"] = event_date.hour;
            
            state["scheduled_events"][static_cast<int>(i)] = event_data;
        }
        
        // Save messages in transit
        auto messages = m_message_system.GetMessagesInTransit();
        for (size_t i = 0; i < messages.size(); ++i) {
            const auto& msg = messages[i];
            Json::Value msg_data;
            msg_data["id"] = msg.id;
            msg_data["from"] = msg.from_location;
            msg_data["to"] = msg.to_location;
            msg_data["content"] = msg.content;
            msg_data["type"] = static_cast<int>(msg.type);
            msg_data["urgent"] = msg.is_urgent;
            msg_data["requires_response"] = msg.requires_response;
            
            state["messages_in_transit"][static_cast<int>(i)] = msg_data;
        }
    }

    void TimeManagementSystem::LoadState(const Json::Value& state) {
        // Load current date
        if (state.isMember("current_date")) {
            GameDate loaded_date;
            loaded_date.year = state["current_date"]["year"].asInt();
            loaded_date.month = state["current_date"]["month"].asInt();
            loaded_date.day = state["current_date"]["day"].asInt();
            loaded_date.hour = state["current_date"]["hour"].asInt();
            
            m_clock.SetCurrentDate(loaded_date);
        }
        
        // Load time scale
        if (state.isMember("time_scale")) {
            auto time_scale = static_cast<TimeScale>(state["time_scale"].asInt());
            m_clock.SetTimeScale(time_scale);
        }
        
        // Load scheduled events (simplified - would need callback restoration)
        if (state.isMember("scheduled_events")) {
            const auto& events_array = state["scheduled_events"];
            for (const auto& event_data : events_array) {
                // Create placeholder callback - full implementation would restore callbacks
                auto placeholder_callback = []() {
                    std::cout << "Loaded event executed (placeholder)" << std::endl;
                };
                
                GameDate event_date(
                    event_data["scheduled_year"].asInt(),
                    event_data["scheduled_month"].asInt(),
                    event_data["scheduled_day"].asInt(),
                    event_data["scheduled_hour"].asInt()
                );
                
                auto tick_type = static_cast<TickType>(event_data["tick_type"].asInt());
                
                m_scheduler.ScheduleEvent(
                    event_data["id"].asString(),
                    event_date,
                    placeholder_callback,
                    tick_type
                );
            }
        }
        
        // Note: Messages in transit would be restored similarly
        // but require more complex state management
        
        messages::DateChanged event;
        event.old_date = m_clock.GetCurrentDate(); // Approximation
        event.new_date = m_clock.GetCurrentDate();
        event.reason = "save_load";
        
        m_message_bus.Publish(event);
    }

    void TimeManagementSystem::ProcessTick(TickType tick_type, const GameDate& current_date) {
        auto start_time = std::chrono::steady_clock::now();

        // Process events for this tick type
        m_scheduler.ProcessEvents(tick_type, current_date);

        auto end_time = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() / 1000.0;

        // Update performance tracking
        m_tick_performance_ms[tick_type] = duration_ms;
        
        // Publish tick event
        PublishTimeEvent(messages::TickOccurred{tick_type, current_date, duration_ms});
    }

    void TimeManagementSystem::UpdatePerformanceMetrics() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_last_performance_check);

        // Report performance every 30 seconds
        if (elapsed.count() >= 30) {
            auto report = GetPerformanceReport();

            if (report.performance_warning && m_gameplay_coordinator) {
                // Notify gameplay coordinator of performance issues
                // This could trigger automatic time scale reduction or system simplification
                std::cout << "Performance warning: Time management taking " 
                          << report.total_update_ms << "ms per update" << std::endl;
            }

            m_last_performance_check = now;
        }
    }

    void TimeManagementSystem::SetupDefaultRoutes() {
        // Historical European routes - major medieval trade and communication paths
        
        // British Isles
        AddRoute("London", "York", 320);
        AddRoute("London", "Winchester", 110);
        AddRoute("York", "Edinburgh", 340);
        
        // France
        AddRoute("Paris", "Lyon", 460);
        AddRoute("Paris", "Bordeaux", 580);
        AddRoute("Lyon", "Marseilles", 320);
        
        // Holy Roman Empire
        AddRoute("Cologne", "Frankfurt", 190);
        AddRoute("Frankfurt", "Munich", 390);
        AddRoute("Munich", "Vienna", 430);
        
        // Italy
        AddRoute("Milan", "Florence", 300);
        AddRoute("Florence", "Rome", 270);
        AddRoute("Rome", "Naples", 230);
        
        // Iberia
        AddRoute("Toledo", "Sevilla", 340);
        AddRoute("Barcelona", "Valencia", 350);
        AddRoute("Santiago", "Toledo", 610);
        
        // Cross-border major routes
        AddRoute("London", "Paris", 350);     // Via Channel crossing
        AddRoute("Paris", "Cologne", 340);    // Major trade route
        AddRoute("Cologne", "Milan", 520);    // Alpine passes
        AddRoute("Vienna", "Venice", 380);    // Eastern trade
        AddRoute("Barcelona", "Toulouse", 250); // Pyrenees crossing
    }

    void TimeManagementSystem::PublishTimeEvent(const messages::TickOccurred& event) {
        m_message_bus.Publish(event);
    }

    void TimeManagementSystem::UpdateEntityAges(const GameClock::TickResult& tick_result) {
        if (tick_result.monthly_tick) {
            auto entities = m_access_manager.GetEntitiesWithComponent<TimeComponent>();
            for (auto entity_id : entities) {
                auto time_comp = m_access_manager.GetComponent<TimeComponent>(entity_id);
                if (time_comp && !time_comp->paused) {
                    time_comp->age_in_months++;
                    time_comp->last_updated = tick_result.current_date;
                }
            }
        }
    }

    void TimeManagementSystem::UpdateMessageProgress() {
        auto entities = m_access_manager.GetEntitiesWithComponent<MessageComponent>();
        GameDate current_date = m_clock.GetCurrentDate();
        
        for (auto entity_id : entities) {
            auto msg_comp = m_access_manager.GetComponent<MessageComponent>(entity_id);
            if (msg_comp && msg_comp->in_transit) {
                const auto& msg = msg_comp->message_data;
                
                // Calculate progress (0.0 to 1.0)
                auto sent_tp = msg.sent_date.ToTimePoint();
                auto arrival_tp = msg.arrival_date.ToTimePoint();
                auto current_tp = current_date.ToTimePoint();
                
                auto total_duration = arrival_tp - sent_tp;
                auto elapsed_duration = current_tp - sent_tp;
                
                if (total_duration.count() > 0) {
                    double progress = static_cast<double>(elapsed_duration.count()) / 
                                    static_cast<double>(total_duration.count());
                    msg_comp->progress = std::clamp(progress, 0.0, 1.0);
                    
                    if (progress >= 1.0) {
                        msg_comp->in_transit = false;
                        
                        // Publish message delivered event
                        messages::MessageDelivered delivered_event;
                        delivered_event.message_id = msg.id;
                        delivered_event.delivery_date = current_date;
                        delivered_event.from_location = msg.from_location;
                        delivered_event.to_location = msg.to_location;
                        
                        m_message_bus.Publish(delivered_event);
                    }
                }
            }
        }
    }

    void TimeManagementSystem::PerformMonthlyMaintenance() {
        // Clean up old completed events
        // Update route conditions based on season
        // Recalculate seasonal message delivery modifiers
        
        std::cout << "Monthly time system maintenance performed for " 
                  << m_clock.GetCurrentDate().ToString() << std::endl;
    }

} // namespace game::time

} // namespace game::time
