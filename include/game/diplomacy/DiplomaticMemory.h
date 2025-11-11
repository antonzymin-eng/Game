#pragma once

#include "core/ECS/IComponent.h"
#include "core/types/game_types.h"
#include "game/diplomacy/DiplomacyComponents.h"
#include "utils/PlatformCompat.h"
#include <vector>
#include <deque>
#include <chrono>
#include <string>
#include <unordered_map>
#include <cstdint>

namespace game::diplomacy {

// ============================================================================
// Event Categories and Types
// ============================================================================

enum class EventCategory : uint8_t {
    MILITARY,       // Wars, battles, military aid
    ECONOMIC,       // Trade, gifts, loans
    DIPLOMATIC,     // Treaties, alliances, proposals
    PERSONAL,       // Ruler interactions, insults, friendships
    DYNASTIC,       // Marriages, successions, claims
    TERRITORIAL,    // Border disputes, territorial exchanges
    RELIGIOUS,      // Religious matters, conversions
    BETRAYAL,       // Treaty violations, backstabs
    COUNT
};

enum class EventType : uint16_t {
    // Military Events (0-99)
    WAR_DECLARED = 0,
    WAR_WON = 1,
    WAR_LOST = 2,
    BATTLE_WON_TOGETHER = 3,
    BATTLE_LOST_TOGETHER = 4,
    MILITARY_AID_PROVIDED = 5,
    MILITARY_AID_REFUSED = 6,
    SIEGE_ASSISTED = 7,
    TROOPS_GRANTED_PASSAGE = 8,
    TROOPS_DENIED_PASSAGE = 9,

    // Economic Events (100-199)
    TRADE_AGREEMENT_SIGNED = 100,
    TRADE_AGREEMENT_BROKEN = 101,
    GIFT_SENT = 102,
    GIFT_RECEIVED = 103,
    LOAN_GRANTED = 104,
    LOAN_REPAID = 105,
    LOAN_DEFAULTED = 106,
    TRADE_EMBARGO_IMPOSED = 107,
    ECONOMIC_AID_PROVIDED = 108,

    // Diplomatic Events (200-299)
    ALLIANCE_FORMED = 200,
    ALLIANCE_BROKEN = 201,
    TREATY_SIGNED = 202,
    TREATY_VIOLATED = 203,
    TREATY_HONORED = 204,
    EMBASSY_ESTABLISHED = 205,
    EMBASSY_CLOSED = 206,
    DIPLOMATIC_INSULT = 207,
    APOLOGY_GIVEN = 208,
    MEDIATION_SUCCESSFUL = 209,

    // Personal Events (300-399)
    RULER_FRIENDSHIP_FORMED = 300,
    RULER_RIVALRY_FORMED = 301,
    PERSONAL_FAVOR_GRANTED = 302,
    PERSONAL_BETRAYAL = 303,
    RULER_SAVED_LIFE = 304,
    RULER_HUMILIATED = 305,

    // Dynastic Events (400-499)
    MARRIAGE_ARRANGED = 400,
    MARRIAGE_REFUSED = 401,
    HEIR_BORN_FROM_MARRIAGE = 402,
    SUCCESSION_SUPPORTED = 403,
    SUCCESSION_OPPOSED = 404,
    DYNASTIC_CLAIM_PRESSED = 405,
    DYNASTIC_CLAIM_RENOUNCED = 406,

    // Territorial Events (500-599)
    TERRITORY_CEDED = 500,
    TERRITORY_SEIZED = 501,
    BORDER_AGREEMENT_SIGNED = 502,
    BORDER_VIOLATED = 503,
    TERRITORIAL_CLAIM_MADE = 504,
    TERRITORIAL_CLAIM_DROPPED = 505,

    // Religious Events (600-699)
    RELIGIOUS_CONVERSION_SUPPORTED = 600,
    RELIGIOUS_PERSECUTION = 601,
    HOLY_SITE_RETURNED = 602,
    HOLY_SITE_SEIZED = 603,
    CRUSADE_ALLY = 604,
    CRUSADE_ENEMY = 605,

    // Betrayal Events (700-799)
    STABBED_IN_BACK = 700,
    ALLY_ABANDONED = 701,
    SECRET_ALLIANCE_REVEALED = 702,
    SPY_CAUGHT = 703,
    ASSASSINATION_ATTEMPTED = 704,

    COUNT
};

// ============================================================================
// Event Severity
// ============================================================================

enum class EventSeverity : uint8_t {
    TRIVIAL,        // +/- 1-5 impact
    MINOR,          // +/- 5-15 impact
    MODERATE,       // +/- 15-35 impact
    MAJOR,          // +/- 35-60 impact
    CRITICAL,       // +/- 60-100 impact
    COUNT
};

// ============================================================================
// Diplomatic Event Record
// ============================================================================

struct DiplomaticEvent {
    std::string event_id;
    EventType type;
    EventCategory category;
    EventSeverity severity;

    types::EntityID actor;          // Who did the action
    types::EntityID target;         // Who received the action

    int opinion_impact = 0;         // -100 to +100
    double trust_impact = 0.0;      // -1.0 to +1.0
    double prestige_impact = 0.0;   // Can be negative or positive

    std::chrono::system_clock::time_point event_date;
    int game_date_year = 0;
    int game_date_month = 0;

    // Event details
    std::string description;
    std::unordered_map<std::string, double> metadata;

    // Memory decay
    double decay_rate = 0.05;       // Monthly decay rate (0.0 = never forget, 1.0 = instant forget)
    double current_weight = 1.0;    // Current impact weight (decays over time)
    bool is_permanent = false;      // Grudges and lasting memories

    // Related events
    std::vector<std::string> related_event_ids;

    DiplomaticEvent() = default;
    DiplomaticEvent(EventType evt_type, types::EntityID from, types::EntityID to);

    // Assign default impacts based on event type
    void AssignDefaultImpacts();

    // Calculate current impact after decay
    int GetCurrentOpinionImpact(std::chrono::system_clock::time_point current_time) const;
    double GetCurrentTrustImpact(std::chrono::system_clock::time_point current_time) const;

    // Apply time-based decay
    void ApplyDecay(float months_elapsed);

    // Check if event is effectively forgotten
    bool IsEffectivelyForgotten() const;
};

// ============================================================================
// Event Memory Storage
// ============================================================================

struct EventMemory {
    types::EntityID our_realm;
    types::EntityID other_realm;

    // All events in chronological order
    std::deque<DiplomaticEvent> event_history;

    // Events by category for quick lookup
    std::unordered_map<EventCategory, std::vector<DiplomaticEvent*>> events_by_category;

    // Permanent memories (never decay)
    std::vector<DiplomaticEvent> permanent_memories;

    // Statistics
    int total_positive_events = 0;
    int total_negative_events = 0;
    int total_neutral_events = 0;

    // Aggregated impacts
    int cumulative_opinion_impact = 0;
    double cumulative_trust_impact = 0.0;

    // Special tracking
    int betrayals_count = 0;
    int wars_fought_together = 0;
    int wars_fought_against = 0;
    int treaties_signed = 0;
    int treaties_broken = 0;

    // Configuration
    static constexpr size_t MAX_EVENTS = 200;           // Keep last 200 events
    static constexpr double FORGIVENESS_RATE = 0.05;    // Base monthly decay

    EventMemory() = default;
    EventMemory(types::EntityID us, types::EntityID them);

    // Add new event
    void RecordEvent(const DiplomaticEvent& event);

    // Query events
    std::vector<DiplomaticEvent*> GetEventsByCategory(EventCategory category);
    std::vector<DiplomaticEvent*> GetEventsByType(EventType type);
    std::vector<DiplomaticEvent*> GetRecentEvents(int months = 12);
    std::vector<DiplomaticEvent*> GetMajorEvents(EventSeverity min_severity = EventSeverity::MAJOR);

    // Calculate impacts
    int CalculateTotalOpinionImpact(std::chrono::system_clock::time_point current_time) const;
    double CalculateTotalTrustImpact(std::chrono::system_clock::time_point current_time) const;

    // Memory patterns
    bool HasGrudge() const;  // Multiple betrayals or major negative events
    bool HasDeepFriendship() const;  // Multiple positive events over long time
    bool IsHistoricalRival() const;  // Long history of conflict
    bool IsHistoricalAlly() const;   // Long history of cooperation

    // Apply decay to all events
    void ApplyMonthlyDecay();

    // Prune old/forgotten events
    void PruneMemory();

    // Serialization
    Json::Value Serialize() const;
    void Deserialize(const Json::Value& data);
};

// ============================================================================
// Relationship Milestone Tracking
// ============================================================================

enum class MilestoneType : uint8_t {
    FIRST_CONTACT,
    FIRST_TRADE,
    FIRST_ALLIANCE,
    FIRST_WAR,
    FIRST_MARRIAGE,
    HUNDREDTH_YEAR_PEACE,
    HUNDREDTH_YEAR_WAR,
    ETERNAL_ALLIANCE,       // 100+ years of alliance
    BITTER_RIVALS,          // 100+ years of conflict
    TRADE_PARTNERSHIP,      // 50+ years continuous trade
    DYNASTIC_UNION,         // Multiple royal marriages
    COUNT
};

struct RelationshipMilestone {
    MilestoneType type;
    std::chrono::system_clock::time_point achieved_date;
    int game_year = 0;

    std::string description;
    double opinion_modifier = 0.0;      // Permanent opinion bonus/penalty
    double trust_modifier = 0.0;        // Permanent trust modifier

    bool is_active = true;

    RelationshipMilestone() = default;
    RelationshipMilestone(MilestoneType milestone_type);
};

struct MilestoneTracker {
    types::EntityID our_realm;
    types::EntityID other_realm;

    std::vector<RelationshipMilestone> achieved_milestones;

    // Tracking for milestone triggers
    std::chrono::system_clock::time_point relationship_start;
    std::chrono::system_clock::time_point last_war_end;
    std::chrono::system_clock::time_point alliance_start;
    std::chrono::system_clock::time_point trade_start;

    int consecutive_peace_years = 0;
    int consecutive_war_years = 0;
    int consecutive_alliance_years = 0;
    int total_marriage_count = 0;

    // Check for new milestones
    std::vector<MilestoneType> CheckForNewMilestones(
        const DiplomaticState& current_state,
        int current_year
    );

    // Add milestone
    void AddMilestone(const RelationshipMilestone& milestone);

    // Get total modifiers from milestones
    double GetTotalOpinionModifier() const;
    double GetTotalTrustModifier() const;

    // Query
    bool HasMilestone(MilestoneType type) const;
    const RelationshipMilestone* GetMilestone(MilestoneType type) const;
};

// ============================================================================
// Memory Component (ECS)
// ============================================================================

struct DiplomaticMemoryComponent : public game::core::Component<DiplomaticMemoryComponent> {
    types::EntityID realm_id;

    // Event memories with all other realms
    std::unordered_map<types::EntityID, EventMemory> memories;

    // Milestone tracking with all other realms
    std::unordered_map<types::EntityID, MilestoneTracker> milestones;

    // Global reputation memory (how others remember us)
    std::vector<DiplomaticEvent> reputation_events;  // Events that affected our global reputation

    std::string GetComponentTypeName() const override {
        return "DiplomaticMemoryComponent";
    }

    // Helper methods
    EventMemory* GetMemoryWith(types::EntityID other_realm);
    const EventMemory* GetMemoryWith(types::EntityID other_realm) const;
    void RecordEvent(const DiplomaticEvent& event);
    void ApplyMonthlyDecay();

    // Serialization
    std::string Serialize() const override;
    bool Deserialize(const std::string& data) override;
};

} // namespace game::diplomacy
