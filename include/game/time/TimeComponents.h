// ============================================================================
// TimeComponents.h - ECS Components for Time Management System
// Created: October 14, 2025 - Modern ECS Architecture Rewrite
// Location: include/game/time/TimeComponents.h
// ============================================================================

#pragma once

#include "utils/PlatformMacros.h"
#include "core/ECS/IComponent.h"
#include "core/types/game_types.h"
#include <chrono>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace game::time {

    // ============================================================================
    // Time System Enums and Data Structures
    // ============================================================================

    enum class TimeScale {
        PAUSED = 0,
        SLOW = 1,      // 0.5x speed - detailed observation
        NORMAL = 2,    // 1.0x speed - standard gameplay
        FAST = 3,      // 3.0x speed - quiet periods
        VERY_FAST = 4, // 7.0x speed - peaceful development
        ULTRA_FAST = 5 // 15.0x speed - long-term observation
    };

    enum class TickType {
        HOURLY = 0,    // Combat, urgent events, immediate actions
        DAILY = 1,     // Message delivery, court events, character actions
        MONTHLY = 2,   // Economy, population, diplomacy, development
        YEARLY = 3     // Technology, long-term trends, aging
    };

    struct GameDate {
        int year = 1066;
        int month = 1;   // 1-12
        int day = 1;     // 1-28/29/30/31 (realistic calendar with leap years)
        int hour = 0;    // 0-23

        GameDate() = default;
        GameDate(int y, int m = 1, int d = 1, int h = 0) : year(y), month(m), day(d), hour(h) {}

        // Comparison operators
        bool operator<(const GameDate& other) const {
            if (year != other.year) return year < other.year;
            if (month != other.month) return month < other.month;
            if (day != other.day) return day < other.day;
            return hour < other.hour;
        }

        bool operator==(const GameDate& other) const {
            return year == other.year && month == other.month && 
                   day == other.day && hour == other.hour;
        }

        bool operator!=(const GameDate& other) const { return !(*this == other); }
        bool operator<=(const GameDate& other) const { return *this < other || *this == other; }
        bool operator>(const GameDate& other) const { return !(*this <= other); }
        bool operator>=(const GameDate& other) const { return !(*this < other); }

        // Date arithmetic
        GameDate AddHours(int hours) const;
        GameDate AddDays(int days) const;
        GameDate AddMonths(int months) const;
        GameDate AddYears(int years) const;

        // Utility methods
        std::string ToString() const;
        std::string ToShortString() const;
        int GetDaysInMonth() const;
    };

    // ============================================================================
    // Entity Time Component - Basic time tracking for entities
    // ============================================================================

    struct EntityTimeComponent : public game::core::Component<EntityTimeComponent> {
        GameDate creation_date;
        GameDate last_updated;
        int age_in_months = 0;
        bool paused = false;
        
        EntityTimeComponent() : creation_date(1066, 10, 14), last_updated(1066, 10, 14) {}
        explicit EntityTimeComponent(const GameDate& start_date) 
            : creation_date(start_date), last_updated(start_date) {}

        void UpdateAge(const GameDate& current_date);
        int GetAgeInYears() const { return age_in_months / 12; }
        
        std::string GetComponentTypeName() const override {
            return "EntityTimeComponent";
        }
    };

    // ============================================================================
    // Scheduled Event Component - Events scheduled for specific times
    // ============================================================================

    struct ScheduledEventComponent : public game::core::Component<ScheduledEventComponent> {
        std::string event_id;
        GameDate scheduled_date;
        TickType tick_type = TickType::DAILY;
        std::string event_category;
        std::string event_data; // Serializable event information
        bool repeating = false;
        int repeat_interval_hours = 0;
        int priority = 0; // Higher = more important
        
        ScheduledEventComponent() = default;
        ScheduledEventComponent(const std::string& id, const GameDate& when, TickType type = TickType::DAILY)
            : event_id(id), scheduled_date(when), tick_type(type) {}

        bool IsReady(const GameDate& current_date) const;
        GameDate GetNextOccurrence() const;
        
        std::string GetComponentTypeName() const override {
            return "ScheduledEventComponent";
        }
    };

    // ============================================================================
    // Message Transit Component - Messages traveling between locations
    // ============================================================================

    enum class MessageType {
        DIPLOMATIC = 0,
        TRADE = 1,
        MILITARY = 2,
        INTELLIGENCE = 3,
        PERSONAL = 4,
        ADMINISTRATIVE = 5,
        RELIGIOUS = 6
    };

    struct MessageTransitComponent : public game::core::Component<MessageTransitComponent> {
        std::string message_id;
        std::string from_location;
        std::string to_location;
        std::string sender_name;
        std::string recipient_name;
        std::string content;
        
        GameDate sent_date;
        GameDate expected_arrival;
        MessageType message_type = MessageType::PERSONAL;
        bool is_urgent = false;
        bool requires_response = false;
        
        double progress = 0.0; // 0.0 to 1.0
        double travel_distance_km = 0.0;
        double travel_speed_kmh = 2.0; // Historical travel speed
        
        MessageTransitComponent() = default;
        MessageTransitComponent(const std::string& id, const std::string& from, const std::string& to)
            : message_id(id), from_location(from), to_location(to) {}

        bool IsDelivered() const { return progress >= 1.0; }
        void UpdateProgress(double hours_passed);
        
        std::string GetComponentTypeName() const override {
            return "MessageTransitComponent";
        }
    };

    // ============================================================================
    // Time Clock Component - Global time state (singleton-like)
    // ============================================================================

    struct TimeClockComponent : public game::core::Component<TimeClockComponent> {
        GameDate current_date;
        TimeScale time_scale = TimeScale::NORMAL;
        bool is_paused = false;
        
        // Timing intervals (in milliseconds)
        int hourly_interval_ms = 1000;   // 1 second = 1 game hour
        int daily_interval_ms = 24000;   // 24 seconds = 1 game day
        int monthly_interval_ms = 720000; // 12 minutes = 1 game month
        int yearly_interval_ms = 8640000; // 2.4 hours = 1 game year
        
        // Last tick times (for internal timing)
        std::chrono::steady_clock::time_point last_hourly_tick = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point last_daily_tick = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point last_monthly_tick = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point last_yearly_tick = std::chrono::steady_clock::now();
        
        TimeClockComponent() : current_date(1066, 10, 14) {}
        explicit TimeClockComponent(const GameDate& start_date) : current_date(start_date) {}

        double GetSpeedMultiplier() const;
        bool ShouldTick(TickType tick_type, std::chrono::steady_clock::time_point now) const;
        void UpdateLastTick(TickType tick_type, std::chrono::steady_clock::time_point now);
        
        std::string GetComponentTypeName() const override {
            return "TimeClockComponent";
        }
    };

    // ============================================================================
    // Route Network Component - Travel routes between locations
    // ============================================================================

    struct RouteNetworkComponent : public game::core::Component<RouteNetworkComponent> {
        // Route definitions (from -> to -> distance in km)
        std::unordered_map<std::string, std::unordered_map<std::string, double>> routes;
        
        // Route quality modifiers (affects travel speed)
        std::unordered_map<std::string, double> route_quality; // 0.1-2.0
        
        // Seasonal modifiers
        double current_seasonal_modifier = 1.0;
        
        RouteNetworkComponent() = default;

        void AddRoute(const std::string& from, const std::string& to, double distance_km);
        void RemoveRoute(const std::string& from, const std::string& to);
        double GetDistance(const std::string& from, const std::string& to) const;
        std::vector<std::string> FindRoute(const std::string& from, const std::string& to) const;
        double GetRouteQuality(const std::string& from, const std::string& to) const;
        
        std::string GetComponentTypeName() const override {
            return "RouteNetworkComponent";
        }
    };

    // ============================================================================
    // Time Performance Component - Performance monitoring
    // ============================================================================

    struct TimePerformanceComponent : public game::core::Component<TimePerformanceComponent> {
        double hourly_tick_ms = 0.0;
        double daily_tick_ms = 0.0;
        double monthly_tick_ms = 0.0;
        double yearly_tick_ms = 0.0;
        double total_update_ms = 0.0;
        
        int active_events = 0;
        int messages_in_transit = 0;
        int entities_with_time = 0;
        
        bool performance_warning = false;
        std::string performance_issues;
        
        TimePerformanceComponent() = default;

        void UpdateTickPerformance(TickType tick_type, double processing_ms);
        void ResetMetrics();
        bool HasPerformanceIssues() const;
        
        std::string GetComponentTypeName() const override {
            return "TimePerformanceComponent";
        }
    };

} // namespace game::time