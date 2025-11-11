#include "game/diplomacy/DiplomaticMemory.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace game::diplomacy {

// ============================================================================
// DiplomaticEvent Implementation
// ============================================================================

DiplomaticEvent::DiplomaticEvent(EventType evt_type, types::EntityID from, types::EntityID to)
    : type(evt_type)
    , actor(from)
    , target(to)
    , event_date(std::chrono::system_clock::now())
{
    // Generate unique ID
    auto timestamp = std::chrono::system_clock::to_time_t(event_date);
    event_id = std::to_string(from.id) + "_" + std::to_string(to.id) + "_" + std::to_string(timestamp);

    // Determine category from type
    int type_value = static_cast<int>(evt_type);
    if (type_value < 100) category = EventCategory::MILITARY;
    else if (type_value < 200) category = EventCategory::ECONOMIC;
    else if (type_value < 300) category = EventCategory::DIPLOMATIC;
    else if (type_value < 400) category = EventCategory::PERSONAL;
    else if (type_value < 500) category = EventCategory::DYNASTIC;
    else if (type_value < 600) category = EventCategory::TERRITORIAL;
    else if (type_value < 700) category = EventCategory::RELIGIOUS;
    else category = EventCategory::BETRAYAL;

    // Set default severity and impacts based on type
    AssignDefaultImpacts();
}

void DiplomaticEvent::AssignDefaultImpacts() {
    switch(type) {
        // Major positive military events
        case EventType::BATTLE_WON_TOGETHER:
            severity = EventSeverity::MAJOR;
            opinion_impact = 30;
            trust_impact = 0.15;
            decay_rate = 0.03;  // Slow decay
            break;

        case EventType::MILITARY_AID_PROVIDED:
            severity = EventSeverity::MODERATE;
            opinion_impact = 20;
            trust_impact = 0.10;
            decay_rate = 0.04;
            break;

        // Major negative military events
        case EventType::WAR_DECLARED:
            severity = EventSeverity::CRITICAL;
            opinion_impact = -75;
            trust_impact = -0.50;
            decay_rate = 0.02;  // Very slow decay
            is_permanent = true;  // Wars are remembered
            break;

        case EventType::MILITARY_AID_REFUSED:
            severity = EventSeverity::MAJOR;
            opinion_impact = -40;
            trust_impact = -0.25;
            decay_rate = 0.03;
            break;

        // Economic positive
        case EventType::TRADE_AGREEMENT_SIGNED:
            severity = EventSeverity::MODERATE;
            opinion_impact = 15;
            trust_impact = 0.08;
            decay_rate = 0.05;
            break;

        case EventType::GIFT_SENT:
            severity = EventSeverity::MINOR;
            opinion_impact = 10;
            trust_impact = 0.05;
            decay_rate = 0.08;  // Faster decay
            break;

        case EventType::LOAN_GRANTED:
            severity = EventSeverity::MODERATE;
            opinion_impact = 25;
            trust_impact = 0.12;
            decay_rate = 0.04;
            break;

        // Economic negative
        case EventType::LOAN_DEFAULTED:
            severity = EventSeverity::MAJOR;
            opinion_impact = -45;
            trust_impact = -0.35;
            decay_rate = 0.02;
            break;

        case EventType::TRADE_EMBARGO_IMPOSED:
            severity = EventSeverity::MODERATE;
            opinion_impact = -30;
            trust_impact = -0.15;
            decay_rate = 0.04;
            break;

        // Diplomatic positive
        case EventType::ALLIANCE_FORMED:
            severity = EventSeverity::MAJOR;
            opinion_impact = 40;
            trust_impact = 0.20;
            decay_rate = 0.02;
            break;

        case EventType::TREATY_HONORED:
            severity = EventSeverity::MINOR;
            opinion_impact = 8;
            trust_impact = 0.06;
            decay_rate = 0.06;
            break;

        case EventType::MEDIATION_SUCCESSFUL:
            severity = EventSeverity::MODERATE;
            opinion_impact = 22;
            trust_impact = 0.10;
            prestige_impact = 5.0;
            decay_rate = 0.04;
            break;

        // Diplomatic negative
        case EventType::ALLIANCE_BROKEN:
            severity = EventSeverity::CRITICAL;
            opinion_impact = -80;
            trust_impact = -0.60;
            decay_rate = 0.01;  // Almost never forgotten
            is_permanent = true;
            break;

        case EventType::TREATY_VIOLATED:
            severity = EventSeverity::MAJOR;
            opinion_impact = -50;
            trust_impact = -0.40;
            decay_rate = 0.02;
            break;

        case EventType::DIPLOMATIC_INSULT:
            severity = EventSeverity::MODERATE;
            opinion_impact = -25;
            trust_impact = -0.10;
            decay_rate = 0.05;
            break;

        // Personal events
        case EventType::RULER_FRIENDSHIP_FORMED:
            severity = EventSeverity::MAJOR;
            opinion_impact = 35;
            trust_impact = 0.25;
            decay_rate = 0.03;
            break;

        case EventType::RULER_SAVED_LIFE:
            severity = EventSeverity::CRITICAL;
            opinion_impact = 90;
            trust_impact = 0.70;
            decay_rate = 0.01;
            is_permanent = true;
            break;

        case EventType::PERSONAL_BETRAYAL:
            severity = EventSeverity::CRITICAL;
            opinion_impact = -85;
            trust_impact = -0.65;
            decay_rate = 0.01;
            is_permanent = true;
            break;

        // Dynastic events
        case EventType::MARRIAGE_ARRANGED:
            severity = EventSeverity::MAJOR;
            opinion_impact = 30;
            trust_impact = 0.18;
            decay_rate = 0.02;
            break;

        case EventType::HEIR_BORN_FROM_MARRIAGE:
            severity = EventSeverity::MODERATE;
            opinion_impact = 20;
            trust_impact = 0.12;
            decay_rate = 0.03;
            break;

        // Territorial events
        case EventType::TERRITORY_CEDED:
            severity = EventSeverity::MAJOR;
            opinion_impact = 45;  // Grateful for territory
            trust_impact = 0.20;
            decay_rate = 0.03;
            break;

        case EventType::TERRITORY_SEIZED:
            severity = EventSeverity::CRITICAL;
            opinion_impact = -70;
            trust_impact = -0.50;
            decay_rate = 0.01;
            is_permanent = true;
            break;

        // Betrayal events (all severe)
        case EventType::STABBED_IN_BACK:
            severity = EventSeverity::CRITICAL;
            opinion_impact = -95;
            trust_impact = -0.80;
            decay_rate = 0.005;  // Extremely slow decay
            is_permanent = true;
            break;

        case EventType::ALLY_ABANDONED:
            severity = EventSeverity::CRITICAL;
            opinion_impact = -88;
            trust_impact = -0.75;
            decay_rate = 0.008;
            is_permanent = true;
            break;

        case EventType::ASSASSINATION_ATTEMPTED:
            severity = EventSeverity::CRITICAL;
            opinion_impact = -100;
            trust_impact = -0.90;
            decay_rate = 0.003;
            is_permanent = true;
            break;

        default:
            severity = EventSeverity::MINOR;
            opinion_impact = 0;
            trust_impact = 0.0;
            decay_rate = 0.10;
            break;
    }
}

int DiplomaticEvent::GetCurrentOpinionImpact(std::chrono::system_clock::time_point current_time) const {
    if (is_permanent) {
        return opinion_impact;  // Permanent events don't decay
    }

    return static_cast<int>(opinion_impact * current_weight);
}

double DiplomaticEvent::GetCurrentTrustImpact(std::chrono::system_clock::time_point current_time) const {
    if (is_permanent) {
        return trust_impact;
    }

    return trust_impact * current_weight;
}

void DiplomaticEvent::ApplyDecay(float months_elapsed) {
    if (is_permanent) return;

    // Exponential decay: weight = weight * (1 - decay_rate)^months
    current_weight *= std::pow(1.0 - decay_rate, months_elapsed);

    // Clamp to prevent negative weights
    if (current_weight < 0.01) {
        current_weight = 0.0;
    }
}

bool DiplomaticEvent::IsEffectivelyForgotten() const {
    if (is_permanent) return false;
    return current_weight < 0.05;  // Less than 5% impact = forgotten
}

// ============================================================================
// EventMemory Implementation
// ============================================================================

EventMemory::EventMemory(types::EntityID us, types::EntityID them)
    : our_realm(us)
    , other_realm(them)
{
}

void EventMemory::RecordEvent(const DiplomaticEvent& event) {
    // Add to chronological history
    event_history.push_back(event);

    // Add to category index
    events_by_category[event.category].push_back(&event_history.back());

    // Add to permanent memories if applicable
    if (event.is_permanent) {
        permanent_memories.push_back(event);
    }

    // Update statistics
    if (event.opinion_impact > 0) {
        total_positive_events++;
    } else if (event.opinion_impact < 0) {
        total_negative_events++;
    } else {
        total_neutral_events++;
    }

    // Update special tracking
    if (event.category == EventCategory::BETRAYAL) {
        betrayals_count++;
    }

    switch(event.type) {
        case EventType::BATTLE_WON_TOGETHER:
        case EventType::MILITARY_AID_PROVIDED:
            wars_fought_together++;
            break;
        case EventType::WAR_DECLARED:
        case EventType::BATTLE_LOST_TOGETHER:
            wars_fought_against++;
            break;
        case EventType::TREATY_SIGNED:
        case EventType::ALLIANCE_FORMED:
            treaties_signed++;
            break;
        case EventType::TREATY_VIOLATED:
        case EventType::ALLIANCE_BROKEN:
            treaties_broken++;
            break;
        default:
            break;
    }

    // Prune if we exceed max events
    if (event_history.size() > MAX_EVENTS) {
        PruneMemory();
    }
}

std::vector<DiplomaticEvent*> EventMemory::GetEventsByCategory(EventCategory category) {
    return events_by_category[category];
}

std::vector<DiplomaticEvent*> EventMemory::GetEventsByType(EventType type) {
    std::vector<DiplomaticEvent*> results;
    for (auto& event : event_history) {
        if (event.type == type) {
            results.push_back(&event);
        }
    }
    return results;
}

std::vector<DiplomaticEvent*> EventMemory::GetRecentEvents(int months) {
    std::vector<DiplomaticEvent*> results;
    auto now = std::chrono::system_clock::now();
    auto cutoff = now - std::chrono::hours(months * 30 * 24);  // Approximate

    for (auto& event : event_history) {
        if (event.event_date >= cutoff) {
            results.push_back(&event);
        }
    }
    return results;
}

std::vector<DiplomaticEvent*> EventMemory::GetMajorEvents(EventSeverity min_severity) {
    std::vector<DiplomaticEvent*> results;
    for (auto& event : event_history) {
        if (event.severity >= min_severity) {
            results.push_back(&event);
        }
    }
    return results;
}

int EventMemory::CalculateTotalOpinionImpact(std::chrono::system_clock::time_point current_time) const {
    int total = 0;
    for (const auto& event : event_history) {
        total += event.GetCurrentOpinionImpact(current_time);
    }
    return total;
}

double EventMemory::CalculateTotalTrustImpact(std::chrono::system_clock::time_point current_time) const {
    double total = 0.0;
    for (const auto& event : event_history) {
        total += event.GetCurrentTrustImpact(current_time);
    }
    return std::clamp(total, -1.0, 1.0);
}

bool EventMemory::HasGrudge() const {
    // Grudge if: 3+ betrayals OR 5+ major negative events OR permanent negative event
    if (betrayals_count >= 3) return true;

    int major_negative = 0;
    for (const auto& event : event_history) {
        if (event.severity >= EventSeverity::MAJOR && event.opinion_impact < -30) {
            major_negative++;
        }
        if (event.is_permanent && event.opinion_impact < -50) {
            return true;
        }
    }

    return major_negative >= 5;
}

bool EventMemory::HasDeepFriendship() const {
    // Friendship if: 10+ positive events over 20+ years OR permanent positive event
    if (total_positive_events >= 10 && event_history.size() > 0) {
        auto oldest = event_history.front().event_date;
        auto newest = event_history.back().event_date;
        auto duration = std::chrono::duration_cast<std::chrono::hours>(newest - oldest).count() / (24 * 365);
        if (duration >= 20) return true;
    }

    for (const auto& event : event_history) {
        if (event.is_permanent && event.opinion_impact > 50) {
            return true;
        }
    }

    return false;
}

bool EventMemory::IsHistoricalRival() const {
    return wars_fought_against >= 3 && wars_fought_together == 0;
}

bool EventMemory::IsHistoricalAlly() const {
    return wars_fought_together >= 3 && wars_fought_against == 0;
}

void EventMemory::ApplyMonthlyDecay() {
    for (auto& event : event_history) {
        event.ApplyDecay(1.0f);  // 1 month
    }
}

void EventMemory::PruneMemory() {
    // Remove forgotten non-permanent events
    auto it = std::remove_if(event_history.begin(), event_history.end(),
        [](const DiplomaticEvent& e) {
            return !e.is_permanent && e.IsEffectivelyForgotten();
        });
    event_history.erase(it, event_history.end());

    // Rebuild category index
    events_by_category.clear();
    for (auto& event : event_history) {
        events_by_category[event.category].push_back(&event);
    }

    // If still too many, remove oldest non-permanent events
    while (event_history.size() > MAX_EVENTS) {
        // Find oldest non-permanent event
        auto oldest_non_perm = std::find_if(event_history.begin(), event_history.end(),
            [](const DiplomaticEvent& e) { return !e.is_permanent; });

        if (oldest_non_perm != event_history.end()) {
            event_history.erase(oldest_non_perm);
        } else {
            break;  // All remaining are permanent
        }
    }
}

Json::Value EventMemory::Serialize() const {
    Json::Value root;
    root["our_realm"] = static_cast<int>(our_realm.id);
    root["other_realm"] = static_cast<int>(other_realm.id);
    root["total_positive_events"] = total_positive_events;
    root["total_negative_events"] = total_negative_events;
    root["total_neutral_events"] = total_neutral_events;
    root["betrayals_count"] = betrayals_count;
    root["wars_fought_together"] = wars_fought_together;
    root["wars_fought_against"] = wars_fought_against;
    root["treaties_signed"] = treaties_signed;
    root["treaties_broken"] = treaties_broken;

    // Serialize events (simplified - full implementation would serialize all event details)
    Json::Value events_array(Json::arrayValue);
    for (const auto& event : event_history) {
        Json::Value event_json;
        event_json["id"] = event.event_id;
        event_json["type"] = static_cast<int>(event.type);
        event_json["opinion_impact"] = event.opinion_impact;
        event_json["trust_impact"] = event.trust_impact;
        event_json["is_permanent"] = event.is_permanent;
        events_array.append(event_json);
    }
    root["events"] = events_array;

    return root;
}

void EventMemory::Deserialize(const Json::Value& data) {
    if (data.isMember("our_realm")) {
        our_realm.id = data["our_realm"].asUInt();
    }
    if (data.isMember("other_realm")) {
        other_realm.id = data["other_realm"].asUInt();
    }
    if (data.isMember("total_positive_events")) {
        total_positive_events = data["total_positive_events"].asInt();
    }
    if (data.isMember("total_negative_events")) {
        total_negative_events = data["total_negative_events"].asInt();
    }
    if (data.isMember("betrayals_count")) {
        betrayals_count = data["betrayals_count"].asInt();
    }
    // Additional fields can be deserialized as needed
}

// ============================================================================
// RelationshipMilestone Implementation
// ============================================================================

RelationshipMilestone::RelationshipMilestone(MilestoneType milestone_type)
    : type(milestone_type)
    , achieved_date(std::chrono::system_clock::now())
{
    switch(type) {
        case MilestoneType::FIRST_CONTACT:
            description = "First diplomatic contact established";
            opinion_modifier = 0.0;
            break;
        case MilestoneType::HUNDREDTH_YEAR_PEACE:
            description = "A century of peace";
            opinion_modifier = 25.0;
            trust_modifier = 0.15;
            break;
        case MilestoneType::ETERNAL_ALLIANCE:
            description = "Century-long alliance";
            opinion_modifier = 40.0;
            trust_modifier = 0.25;
            break;
        case MilestoneType::DYNASTIC_UNION:
            description = "Dynasties united through marriage";
            opinion_modifier = 30.0;
            trust_modifier = 0.20;
            break;
        default:
            description = "Milestone achieved";
            break;
    }
}

// ============================================================================
// MilestoneTracker Implementation
// ============================================================================

std::vector<MilestoneType> MilestoneTracker::CheckForNewMilestones(
    const DiplomaticState& current_state,
    int current_year)
{
    std::vector<MilestoneType> new_milestones;

    // Check 100 year peace
    if (consecutive_peace_years >= 100 && !HasMilestone(MilestoneType::HUNDREDTH_YEAR_PEACE)) {
        new_milestones.push_back(MilestoneType::HUNDREDTH_YEAR_PEACE);
    }

    // Check eternal alliance
    if (consecutive_alliance_years >= 100 && !HasMilestone(MilestoneType::ETERNAL_ALLIANCE)) {
        new_milestones.push_back(MilestoneType::ETERNAL_ALLIANCE);
    }

    // Check dynastic union (5+ marriages)
    if (total_marriage_count >= 5 && !HasMilestone(MilestoneType::DYNASTIC_UNION)) {
        new_milestones.push_back(MilestoneType::DYNASTIC_UNION);
    }

    return new_milestones;
}

void MilestoneTracker::AddMilestone(const RelationshipMilestone& milestone) {
    achieved_milestones.push_back(milestone);
}

double MilestoneTracker::GetTotalOpinionModifier() const {
    double total = 0.0;
    for (const auto& milestone : achieved_milestones) {
        if (milestone.is_active) {
            total += milestone.opinion_modifier;
        }
    }
    return total;
}

double MilestoneTracker::GetTotalTrustModifier() const {
    double total = 0.0;
    for (const auto& milestone : achieved_milestones) {
        if (milestone.is_active) {
            total += milestone.trust_modifier;
        }
    }
    return total;
}

bool MilestoneTracker::HasMilestone(MilestoneType type) const {
    return std::any_of(achieved_milestones.begin(), achieved_milestones.end(),
        [type](const RelationshipMilestone& m) { return m.type == type && m.is_active; });
}

const RelationshipMilestone* MilestoneTracker::GetMilestone(MilestoneType type) const {
    auto it = std::find_if(achieved_milestones.begin(), achieved_milestones.end(),
        [type](const RelationshipMilestone& m) { return m.type == type && m.is_active; });

    return (it != achieved_milestones.end()) ? &(*it) : nullptr;
}

// ============================================================================
// DiplomaticMemoryComponent Implementation
// ============================================================================

EventMemory* DiplomaticMemoryComponent::GetMemoryWith(types::EntityID other_realm) {
    auto it = memories.find(other_realm);
    if (it != memories.end()) {
        return &it->second;
    }

    // Create new memory
    memories[other_realm] = EventMemory(realm_id, other_realm);
    return &memories[other_realm];
}

const EventMemory* DiplomaticMemoryComponent::GetMemoryWith(types::EntityID other_realm) const {
    auto it = memories.find(other_realm);
    if (it != memories.end()) {
        return &it->second;
    }
    return nullptr;
}

void DiplomaticMemoryComponent::RecordEvent(const DiplomaticEvent& event) {
    // Record in bilateral memory
    auto* memory = GetMemoryWith(event.target);
    if (memory) {
        memory->RecordEvent(event);
    }

    // If affects reputation, add to reputation events
    if (std::abs(event.prestige_impact) > 5.0) {
        reputation_events.push_back(event);
    }
}

void DiplomaticMemoryComponent::ApplyMonthlyDecay() {
    for (auto& [realm_id, memory] : memories) {
        memory.ApplyMonthlyDecay();
    }
}

std::string DiplomaticMemoryComponent::Serialize() const {
    Json::Value root;
    root["realm_id"] = static_cast<int>(realm_id.id);

    Json::Value memories_array(Json::arrayValue);
    for (const auto& [other_id, memory] : memories) {
        memories_array.append(memory.Serialize());
    }
    root["memories"] = memories_array;

    Json::StreamWriterBuilder writer;
    return Json::writeString(writer, root);
}

bool DiplomaticMemoryComponent::Deserialize(const std::string& json_str) {
    Json::CharReaderBuilder reader;
    Json::Value data;
    std::string errs;
    std::istringstream stream(json_str);

    if (!Json::parseFromStream(reader, stream, &data, &errs)) {
        return false;
    }

    if (data.isMember("realm_id")) {
        realm_id.id = data["realm_id"].asUInt();
    }

    if (data.isMember("memories") && data["memories"].isArray()) {
        for (const auto& memory_data : data["memories"]) {
            EventMemory memory;
            memory.Deserialize(memory_data);
            memories[memory.other_realm] = memory;
        }
    }

    return true;
}

} // namespace game::diplomacy
