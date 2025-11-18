// ============================================================================
// NewsSystem.h - News and Message Propagation System
// Created: November 18, 2025
// Description: Delayed news delivery system for military campaigns and
//              nation management with realistic information propagation
// ============================================================================

#pragma once

#include "utils/PlatformMacros.h"
#include "core/types/game_types.h"
#include "core/ECS/IComponent.h"
#include "map/ProvinceRenderComponent.h"
#include "game/military/CommandDelay.h"
#include <string>
#include <vector>
#include <queue>

namespace game::news {

    // ========================================================================
    // NewsCategory - Type of news/message
    // ========================================================================
    enum class NewsCategory {
        MILITARY = 0,           // Battle reports, army movements
        NAVAL,                  // Naval battles, fleet movements
        ECONOMIC,               // Economic reports, trade updates
        DIPLOMATIC,             // Diplomatic messages, treaties
        ADMINISTRATIVE,         // Provincial reports, administration
        REGENT_REPORT,          // Reports from regent/council
        EMERGENCY,              // Emergencies requiring immediate attention
        COURT,                  // Court events, character news
        ESPIONAGE,              // Intelligence reports
        TECHNOLOGY,             // Technology breakthroughs
        RELIGIOUS,              // Religious events
        CULTURAL,               // Cultural events
        POPULATION,             // Population events, unrest
        DISASTER,               // Natural disasters, plagues
        COUNT
    };

    // ========================================================================
    // NewsPriority - Priority level for news delivery
    // ========================================================================
    enum class NewsPriority {
        ROUTINE = 0,            // Regular updates, no rush
        IMPORTANT,              // Important but not urgent
        URGENT,                 // Needs attention soon
        CRITICAL,               // Requires immediate attention
        EMERGENCY,              // Use fastest messenger possible
        COUNT
    };

    // ========================================================================
    // NewsMessage - Individual news item
    // ========================================================================
    struct NewsMessage {
        game::types::EntityID message_id = 0;

        // Message metadata
        NewsCategory category = NewsCategory::MILITARY;
        NewsPriority priority = NewsPriority::ROUTINE;
        std::string title;
        std::string content;
        std::string sender_name;                        // Who sent the message

        // Location and timing
        map::Vector2 origin_position;                   // Where event occurred
        game::types::EntityID origin_province = 0;
        float event_time = 0.0f;                        // When event happened
        float send_time = 0.0f;                         // When message was sent
        float arrival_time = 0.0f;                      // When message arrives
        float total_delay = 0.0f;                       // Total delay in hours

        // Communication
        military::CommunicationType comm_type = military::CommunicationType::MESSENGER;
        bool has_arrived = false;
        bool has_been_read = false;

        // Related entities
        game::types::EntityID related_army = 0;
        game::types::EntityID related_character = 0;
        game::types::EntityID related_province = 0;
        game::types::EntityID related_nation = 0;

        // Importance and actions
        bool requires_response = false;
        bool requires_decision = false;
        std::vector<std::string> available_actions;     // Actions player can take

        NewsMessage() = default;

        // Check if arrived
        bool HasArrived(float current_time) const {
            return current_time >= arrival_time;
        }

        // Get progress (0.0 to 1.0)
        float GetProgress(float current_time) const {
            if (total_delay <= 0.0f) return 1.0f;
            float elapsed = current_time - send_time;
            return std::min(1.0f, elapsed / total_delay);
        }

        // Get time remaining
        float GetTimeRemaining(float current_time) const {
            return std::max(0.0f, arrival_time - current_time);
        }

        // Get age since event occurred
        float GetAge(float current_time) const {
            return current_time - event_time;
        }
    };

    // ========================================================================
    // RegentReport - Periodic report from regent
    // ========================================================================
    struct RegentReport {
        float report_time = 0.0f;
        float report_period_start = 0.0f;
        float report_period_end = 0.0f;

        // Summary statistics
        int provinces_managed = 0;
        int armies_deployed = 0;
        int battles_fought = 0;
        int diplomatic_actions = 0;
        int economic_decisions = 0;

        // Financial summary
        double income_this_period = 0.0;
        double expenses_this_period = 0.0;
        double treasury_balance = 0.0;

        // Important events
        std::vector<std::string> major_events;
        std::vector<std::string> problems_encountered;
        std::vector<std::string> recommendations;

        // Regent's assessment
        std::string overall_status;                     // "Stable", "Concerning", "Crisis"
        float stability_rating = 0.7f;                  // 0.0 to 1.0

        RegentReport() = default;
    };

    // ========================================================================
    // MessageInboxComponent - Player's message inbox
    // ========================================================================
    struct MessageInboxComponent : public game::core::Component<MessageInboxComponent> {
        // Pending messages (not yet arrived)
        std::vector<NewsMessage> pending_messages;

        // Arrived messages (in inbox, may be unread)
        std::vector<NewsMessage> inbox_messages;

        // Read messages (archive)
        std::vector<NewsMessage> archive_messages;

        // Regent reports
        std::vector<RegentReport> regent_reports;

        // Settings
        size_t max_inbox_size = 100;
        size_t max_archive_size = 500;
        bool auto_archive_read = true;
        bool filter_routine_when_in_field = true;       // Filter low priority when with army

        // Statistics
        int total_messages_received = 0;
        int unread_message_count = 0;
        float last_message_time = 0.0f;

        MessageInboxComponent() = default;

        // Add new message to pending queue
        void AddPendingMessage(const NewsMessage& message) {
            pending_messages.push_back(message);
        }

        // Move arrived message to inbox
        void MoveToInbox(const NewsMessage& message) {
            inbox_messages.push_back(message);
            unread_message_count++;
            total_messages_received++;
        }

        // Mark message as read
        void MarkAsRead(size_t inbox_index) {
            if (inbox_index < inbox_messages.size()) {
                inbox_messages[inbox_index].has_been_read = true;
                unread_message_count--;

                if (auto_archive_read) {
                    archive_messages.push_back(inbox_messages[inbox_index]);
                    inbox_messages.erase(inbox_messages.begin() + inbox_index);

                    // Limit archive size
                    if (archive_messages.size() > max_archive_size) {
                        archive_messages.erase(archive_messages.begin());
                    }
                }
            }
        }

        // Get unread count by category
        int GetUnreadCount(NewsCategory category) const {
            int count = 0;
            for (const auto& msg : inbox_messages) {
                if (!msg.has_been_read && msg.category == category) {
                    count++;
                }
            }
            return count;
        }

        // Get unread count by priority
        int GetUnreadCount(NewsPriority priority) const {
            int count = 0;
            for (const auto& msg : inbox_messages) {
                if (!msg.has_been_read && msg.priority == priority) {
                    count++;
                }
            }
            return count;
        }

        // Clear old archive
        void CleanArchive(float current_time, float max_age_hours) {
            archive_messages.erase(
                std::remove_if(archive_messages.begin(), archive_messages.end(),
                    [current_time, max_age_hours](const NewsMessage& msg) {
                        return (current_time - msg.event_time) > max_age_hours;
                    }),
                archive_messages.end()
            );
        }

        std::string GetComponentTypeName() const override {
            return "MessageInboxComponent";
        }
    };

    // ========================================================================
    // Helper Functions
    // ========================================================================

    inline const char* NewsCategoryToString(NewsCategory category) {
        switch (category) {
            case NewsCategory::MILITARY: return "Military";
            case NewsCategory::NAVAL: return "Naval";
            case NewsCategory::ECONOMIC: return "Economic";
            case NewsCategory::DIPLOMATIC: return "Diplomatic";
            case NewsCategory::ADMINISTRATIVE: return "Administrative";
            case NewsCategory::REGENT_REPORT: return "Regent Report";
            case NewsCategory::EMERGENCY: return "Emergency";
            case NewsCategory::COURT: return "Court";
            case NewsCategory::ESPIONAGE: return "Espionage";
            case NewsCategory::TECHNOLOGY: return "Technology";
            case NewsCategory::RELIGIOUS: return "Religious";
            case NewsCategory::CULTURAL: return "Cultural";
            case NewsCategory::POPULATION: return "Population";
            case NewsCategory::DISASTER: return "Disaster";
            default: return "Unknown";
        }
    }

    inline const char* NewsPriorityToString(NewsPriority priority) {
        switch (priority) {
            case NewsPriority::ROUTINE: return "Routine";
            case NewsPriority::IMPORTANT: return "Important";
            case NewsPriority::URGENT: return "Urgent";
            case NewsPriority::CRITICAL: return "Critical";
            case NewsPriority::EMERGENCY: return "Emergency";
            default: return "Unknown";
        }
    }

    // Get communication type based on priority
    inline military::CommunicationType GetCommTypeForPriority(NewsPriority priority) {
        switch (priority) {
            case NewsPriority::ROUTINE:
                return military::CommunicationType::MESSENGER;
            case NewsPriority::IMPORTANT:
                return military::CommunicationType::COURIER;
            case NewsPriority::URGENT:
                return military::CommunicationType::SIGNAL_FIRE;
            case NewsPriority::CRITICAL:
                return military::CommunicationType::TELEGRAPH;
            case NewsPriority::EMERGENCY:
                return military::CommunicationType::RADIO;
            default:
                return military::CommunicationType::MESSENGER;
        }
    }

} // namespace game::news
