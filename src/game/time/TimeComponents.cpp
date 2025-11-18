// ============================================================================
// TimeComponents.cpp - ECS Components Implementation for Time System
// Created: October 14, 2025 - Modern ECS Architecture Rewrite
// Location: src/game/time/TimeComponents.cpp
// ============================================================================

#include "game/time/TimeComponents.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

namespace game::time {

    // ============================================================================
    // GameDate Implementation
    // ============================================================================

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
        while (result.day > result.GetDaysInMonth()) {
            result.day -= result.GetDaysInMonth();
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
            result.day += result.GetDaysInMonth();
        }
        
        return result;
    }

    GameDate GameDate::AddDays(int days) const {
        // Direct implementation to avoid int overflow from days * 24 in AddHours
        GameDate result = *this;
        result.day += days;
        
        // Handle day overflow/underflow
        while (result.day > result.GetDaysInMonth()) {
            result.day -= result.GetDaysInMonth();
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
            result.day += result.GetDaysInMonth();
        }
        
        return result;
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
        
        // Ensure day is valid for the new month
        int max_days = result.GetDaysInMonth();
        if (result.day > max_days) {
            result.day = max_days;
        }
        
        return result;
    }

    GameDate GameDate::AddYears(int years) const {
        GameDate result = *this;
        result.year += years;
        
        // Handle February 29th in non-leap years
        if (result.month == 2 && result.day == 29) {
            int max_days = result.GetDaysInMonth();
            if (result.day > max_days) {
                result.day = max_days;
            }
        }
        
        return result;
    }

    std::string GameDate::ToString() const {
        std::stringstream ss;
        ss << day << " ";

        const char* months[] = {"", "January", "February", "March", "April", "May", "June",
                                "July", "August", "September", "October", "November", "December"};

        // FIXED: Bounds checking to prevent array out-of-bounds access
        int safe_month = (month >= 1 && month <= 12) ? month : 1;
        ss << months[safe_month] << " " << year;
        
        // Always show time for consistency (including midnight)
        ss << " at " << std::setfill('0') << std::setw(2) << hour << ":00";
        
        return ss.str();
    }

    std::string GameDate::ToShortString() const {
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(2) << day << "/"
           << std::setfill('0') << std::setw(2) << month << "/" << year;
        return ss.str();
    }

    int GameDate::GetDaysInMonth() const {
        const int days_per_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

        // FIXED: Bounds checking to prevent array out-of-bounds access
        if (month < 1 || month > 12) {
            return 30;  // Safe default if month is invalid
        }

        if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) {
            return 29; // Leap year February
        }

        return days_per_month[month];
    }

    void GameDate::Normalize() {
        // FIXED: Normalize hours
        while (hour >= 24) {
            hour -= 24;
            day++;
        }
        while (hour < 0) {
            hour += 24;
            day--;
        }

        // FIXED: Normalize months
        while (month > 12) {
            month -= 12;
            year++;
        }
        while (month < 1) {
            month += 12;
            year--;
        }

        // FIXED: Normalize days with safety check against infinite loops
        int max_iterations = 120;  // Max iterations to prevent infinite loop (10 years worth)
        int iterations = 0;

        while (day > GetDaysInMonth() && iterations < max_iterations) {
            day -= GetDaysInMonth();
            month++;
            if (month > 12) {
                month = 1;
                year++;
            }
            iterations++;
        }

        iterations = 0;
        while (day < 1 && iterations < max_iterations) {
            month--;
            if (month < 1) {
                month = 12;
                year--;
            }
            day += GetDaysInMonth();
            iterations++;
        }

        // Safety clamp if normalization failed
        if (day < 1) day = 1;
        if (day > GetDaysInMonth()) day = GetDaysInMonth();
        if (month < 1) month = 1;
        if (month > 12) month = 12;
    }

    // ============================================================================
    // EntityTimeComponent Implementation
    // ============================================================================

    void EntityTimeComponent::UpdateAge(const GameDate& current_date) {
        // Calculate age in months since creation
        int years_diff = current_date.year - creation_date.year;
        int months_diff = current_date.month - creation_date.month;
        
        age_in_months = years_diff * 12 + months_diff;
        
        // Adjust for day differences
        if (current_date.day < creation_date.day) {
            age_in_months--;
        }
        
        // Ensure non-negative
        age_in_months = std::max(0, age_in_months);
        
        last_updated = current_date;
    }

    // ============================================================================
    // ScheduledEventComponent Implementation
    // ============================================================================

    bool ScheduledEventComponent::IsReady(const GameDate& current_date) const {
        return current_date >= scheduled_date;
    }

    GameDate ScheduledEventComponent::GetNextOccurrence() const {
        if (!repeating || repeat_interval_hours <= 0) {
            return scheduled_date;
        }
        
        return scheduled_date.AddHours(repeat_interval_hours);
    }

    // ============================================================================
    // MessageTransitComponent Implementation
    // ============================================================================

    void MessageTransitComponent::UpdateProgress(double hours_passed) {
        if (travel_distance_km <= 0.0) {
            progress = 1.0; // Instant delivery for zero distance
            return;
        }
        
        double distance_traveled = travel_speed_kmh * hours_passed;
        progress = std::min(1.0, progress + (distance_traveled / travel_distance_km));
    }

    // ============================================================================
    // TimeClockComponent Implementation
    // ============================================================================

    double TimeClockComponent::GetSpeedMultiplier() const {
        if (is_paused) return 0.0;
        
        switch (time_scale) {
            case TimeScale::PAUSED: return 0.0;
            case TimeScale::SLOW: return 0.5;
            case TimeScale::NORMAL: return 1.0;
            case TimeScale::FAST: return 3.0;
            case TimeScale::VERY_FAST: return 7.0;
            case TimeScale::ULTRA_FAST: return 15.0;
            default: return 1.0;
        }
    }

    bool TimeClockComponent::ShouldTick(TickType tick_type, std::chrono::steady_clock::time_point now) const {
        if (is_paused) return false;
        
        double multiplier = GetSpeedMultiplier();
        if (multiplier <= 0.0) return false;
        
        std::chrono::steady_clock::time_point last_tick;
        int interval_ms;
        
        switch (tick_type) {
            case TickType::HOURLY:
                last_tick = last_hourly_tick;
                interval_ms = hourly_interval_ms;
                break;
            case TickType::DAILY:
                last_tick = last_daily_tick;
                interval_ms = daily_interval_ms;
                break;
            case TickType::MONTHLY:
                last_tick = last_monthly_tick;
                interval_ms = monthly_interval_ms;
                break;
            case TickType::YEARLY:
                last_tick = last_yearly_tick;
                interval_ms = yearly_interval_ms;
                break;
            default:
                return false;
        }
        
        auto adjusted_interval = std::chrono::milliseconds(static_cast<int>(interval_ms / multiplier));
        return (now - last_tick) >= adjusted_interval;
    }

    void TimeClockComponent::UpdateLastTick(TickType tick_type, std::chrono::steady_clock::time_point now) {
        switch (tick_type) {
            case TickType::HOURLY:
                last_hourly_tick = now;
                break;
            case TickType::DAILY:
                last_daily_tick = now;
                break;
            case TickType::MONTHLY:
                last_monthly_tick = now;
                break;
            case TickType::YEARLY:
                last_yearly_tick = now;
                break;
        }
    }

    // ============================================================================
    // RouteNetworkComponent Implementation
    // ============================================================================

    void RouteNetworkComponent::AddRoute(const std::string& from, const std::string& to, double distance_km) {
        routes[from][to] = distance_km;
        routes[to][from] = distance_km; // Bidirectional by default
        
        // Set default quality
        std::string route_key = from + "->" + to;
        if (route_quality.find(route_key) == route_quality.end()) {
            route_quality[route_key] = 1.0;
        }
        route_key = to + "->" + from;
        if (route_quality.find(route_key) == route_quality.end()) {
            route_quality[route_key] = 1.0;
        }
    }

    void RouteNetworkComponent::RemoveRoute(const std::string& from, const std::string& to) {
        if (routes.find(from) != routes.end()) {
            routes[from].erase(to);
        }
        if (routes.find(to) != routes.end()) {
            routes[to].erase(from);
        }
        
        route_quality.erase(from + "->" + to);
        route_quality.erase(to + "->" + from);
    }

    double RouteNetworkComponent::GetDistance(const std::string& from, const std::string& to) const {
        auto from_it = routes.find(from);
        if (from_it != routes.end()) {
            auto to_it = from_it->second.find(to);
            if (to_it != from_it->second.end()) {
                return to_it->second;
            }
        }
        return 0.0; // No route found
    }

    std::vector<std::string> RouteNetworkComponent::FindRoute(const std::string& from, const std::string& to) const {
        // Simple direct route check for now
        // Could be expanded to use pathfinding algorithms like Dijkstra
        if (GetDistance(from, to) > 0.0) {
            return {from, to};
        }
        return {}; // No route found
    }

    double RouteNetworkComponent::GetRouteQuality(const std::string& from, const std::string& to) const {
        std::string route_key = from + "->" + to;
        auto it = route_quality.find(route_key);
        if (it != route_quality.end()) {
            return it->second * current_seasonal_modifier;
        }
        return 1.0; // Default quality
    }

    // ============================================================================
    // TimePerformanceComponent Implementation
    // ============================================================================

    void TimePerformanceComponent::UpdateTickPerformance(TickType tick_type, double processing_ms) {
        switch (tick_type) {
            case TickType::HOURLY:
                hourly_tick_ms = processing_ms;
                break;
            case TickType::DAILY:
                daily_tick_ms = processing_ms;
                break;
            case TickType::MONTHLY:
                monthly_tick_ms = processing_ms;
                break;
            case TickType::YEARLY:
                yearly_tick_ms = processing_ms;
                break;
        }
        
        total_update_ms = hourly_tick_ms + daily_tick_ms + monthly_tick_ms + yearly_tick_ms;
        
        // Check for performance issues
        performance_warning = (total_update_ms > 100.0); // More than 100ms is concerning
        
        if (performance_warning) {
            performance_issues = "High tick processing time: " + std::to_string(total_update_ms) + "ms";
        } else {
            performance_issues.clear();
        }
    }

    void TimePerformanceComponent::ResetMetrics() {
        hourly_tick_ms = 0.0;
        daily_tick_ms = 0.0;
        monthly_tick_ms = 0.0;
        yearly_tick_ms = 0.0;
        total_update_ms = 0.0;
        active_events = 0;
        messages_in_transit = 0;
        entities_with_time = 0;
        performance_warning = false;
        performance_issues.clear();
    }

    bool TimePerformanceComponent::HasPerformanceIssues() const {
        return performance_warning || total_update_ms > 50.0;
    }

} // namespace game::time