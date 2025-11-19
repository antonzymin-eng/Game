// Created: November 19, 2025
// Location: include/game/character/CharacterLifeEvents.h
// Purpose: Character life event system for tracking major life moments

#pragma once

#include "core/ECS/IComponent.h"
#include "core/types/game_types.h"
#include <vector>
#include <string>
#include <chrono>
#include <functional>

namespace game {
namespace character {

// ============================================================================
// Life Event Types
// ============================================================================

enum class LifeEventType : uint8_t {
    // Birth and childhood
    BIRTH,
    COMING_OF_AGE,
    CHILDHOOD_TRAIT_GAINED,

    // Education
    EDUCATION_STARTED,
    EDUCATION_COMPLETED,
    TUTOR_ASSIGNED,
    SKILL_LEARNED,

    // Relationships
    MARRIAGE,
    DIVORCE,
    CHILD_BORN,
    LOVER_TAKEN,
    FRIENDSHIP_FORMED,
    RIVAL_DECLARED,
    MENTOR_GAINED,
    MENTOR_LOST,

    // Achievements
    TITLE_GAINED,
    TITLE_LOST,
    LAND_CONQUERED,
    BATTLE_WON,
    BATTLE_LOST,
    TOURNAMENT_WON,
    ARTIFACT_CREATED,
    BUILDING_COMPLETED,

    // Religious
    PILGRIMAGE_COMPLETED,
    RELIGIOUS_CONVERSION,
    EXCOMMUNICATION,
    CANONIZATION,

    // Health
    WOUNDED_IN_BATTLE,
    ILLNESS_CONTRACTED,
    ILLNESS_RECOVERED,
    MAIMING,
    RECOVERY,

    // Political
    PLOT_DISCOVERED,
    PLOT_SUCCEEDED,
    PLOT_FAILED,
    COUP_ATTEMPTED,
    ASSASSINATION_SURVIVED,
    COUNCIL_POSITION_GAINED,
    COUNCIL_POSITION_LOST,
    IMPRISONED,
    RELEASED,
    EXILED,
    RETURNED_FROM_EXILE,

    // Social
    FEAST_HOSTED,
    GREAT_WORK_COMMISSIONED,
    SCANDAL,
    FAME_GAINED,
    INFAMY_GAINED,

    // Negative
    BETRAYED,
    HUMILIATED,
    DEFEATED,
    RANSOMED,

    // Death
    DEATH_NATURAL,
    DEATH_BATTLE,
    DEATH_ASSASSINATION,
    DEATH_EXECUTION,
    DEATH_ILLNESS,
    DEATH_OLD_AGE,

    COUNT
};

// ============================================================================
// Life Event
// ============================================================================

struct LifeEvent {
    LifeEventType type = LifeEventType::BIRTH;
    std::chrono::system_clock::time_point date;
    std::string description;

    // Related entities
    types::EntityID related_character{0};  // For relationships, deaths, etc.
    types::EntityID related_realm{0};      // For titles, battles, etc.
    types::EntityID related_title{0};      // For title gains/losses

    // Event details
    std::string location;                  // Where it happened
    int32_t age_at_event = 0;             // Character's age
    float impact_prestige = 0.0f;         // Prestige change
    float impact_health = 0.0f;           // Health change
    std::vector<std::string> traits_gained;
    std::vector<std::string> traits_lost;

    // Flags
    bool is_positive = true;              // Good or bad event
    bool is_major = false;                // Major life event
    bool is_secret = false;               // Hidden from others

    LifeEvent() = default;
    LifeEvent(LifeEventType event_type, const std::string& desc)
        : type(event_type)
        , date(std::chrono::system_clock::now())
        , description(desc)
    {}
};

// ============================================================================
// Character Life Events Component (ECS)
// ============================================================================

class CharacterLifeEventsComponent : public ::core::ecs::Component<CharacterLifeEventsComponent> {
public:
    types::EntityID character_id{0};

    // All life events for this character (chronologically ordered)
    std::vector<LifeEvent> life_events;

    // Quick access to important events
    std::chrono::system_clock::time_point birth_date;
    std::chrono::system_clock::time_point coming_of_age_date;
    std::chrono::system_clock::time_point death_date;

    CharacterLifeEventsComponent() = default;
    explicit CharacterLifeEventsComponent(types::EntityID char_id)
        : character_id(char_id)
    {}

    // ========================================================================
    // Event Recording
    // ========================================================================

    /**
     * Add a life event
     */
    void AddEvent(const LifeEvent& event) {
        life_events.push_back(event);

        // Update quick access dates
        switch (event.type) {
            case LifeEventType::BIRTH:
                birth_date = event.date;
                break;
            case LifeEventType::COMING_OF_AGE:
                coming_of_age_date = event.date;
                break;
            case LifeEventType::DEATH_NATURAL:
            case LifeEventType::DEATH_BATTLE:
            case LifeEventType::DEATH_ASSASSINATION:
            case LifeEventType::DEATH_EXECUTION:
            case LifeEventType::DEATH_ILLNESS:
            case LifeEventType::DEATH_OLD_AGE:
                death_date = event.date;
                break;
            default:
                break;
        }
    }

    /**
     * Add a simple event
     */
    void AddSimpleEvent(LifeEventType type, const std::string& description,
                       int age = 0, bool is_major = false) {
        LifeEvent event(type, description);
        event.age_at_event = age;
        event.is_major = is_major;
        event.is_positive = IsPositiveEvent(type);
        AddEvent(event);
    }

    /**
     * Get all events of a specific type
     */
    std::vector<LifeEvent> GetEventsByType(LifeEventType type) const {
        std::vector<LifeEvent> result;
        for (const auto& event : life_events) {
            if (event.type == type) {
                result.push_back(event);
            }
        }
        return result;
    }

    /**
     * Get events in a date range
     */
    std::vector<LifeEvent> GetEventsInRange(
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end) const {

        std::vector<LifeEvent> result;
        for (const auto& event : life_events) {
            if (event.date >= start && event.date <= end) {
                result.push_back(event);
            }
        }
        return result;
    }

    /**
     * Get major life events only
     */
    std::vector<LifeEvent> GetMajorEvents() const {
        std::vector<LifeEvent> result;
        for (const auto& event : life_events) {
            if (event.is_major) {
                result.push_back(event);
            }
        }
        return result;
    }

    /**
     * Get event count by type
     */
    size_t GetEventCount(LifeEventType type) const {
        return std::count_if(life_events.begin(), life_events.end(),
            [type](const LifeEvent& e) { return e.type == type; });
    }

    /**
     * Check if character has experienced an event type
     */
    bool HasExperienced(LifeEventType type) const {
        return GetEventCount(type) > 0;
    }

    /**
     * Get most recent event of a type
     */
    const LifeEvent* GetMostRecentEvent(LifeEventType type) const {
        for (auto it = life_events.rbegin(); it != life_events.rend(); ++it) {
            if (it->type == type) {
                return &(*it);
            }
        }
        return nullptr;
    }

    /**
     * Calculate character age in years
     */
    int GetAgeInYears() const {
        if (birth_date.time_since_epoch().count() == 0) {
            return 0;
        }

        auto now = std::chrono::system_clock::now();
        auto age_duration = std::chrono::duration_cast<std::chrono::hours>(
            now - birth_date);
        return static_cast<int>(age_duration.count() / 8760); // hours per year
    }

    /**
     * Check if character is adult (came of age)
     */
    bool IsAdult() const {
        return coming_of_age_date.time_since_epoch().count() > 0;
    }

    /**
     * Get biography string (major events)
     */
    std::string GetBiography() const {
        std::string bio;
        for (const auto& event : life_events) {
            if (event.is_major) {
                bio += event.description + "\n";
            }
        }
        return bio;
    }

private:
    /**
     * Determine if event type is generally positive
     */
    static bool IsPositiveEvent(LifeEventType type) {
        switch (type) {
            case LifeEventType::BIRTH:
            case LifeEventType::COMING_OF_AGE:
            case LifeEventType::EDUCATION_COMPLETED:
            case LifeEventType::MARRIAGE:
            case LifeEventType::CHILD_BORN:
            case LifeEventType::FRIENDSHIP_FORMED:
            case LifeEventType::MENTOR_GAINED:
            case LifeEventType::TITLE_GAINED:
            case LifeEventType::LAND_CONQUERED:
            case LifeEventType::BATTLE_WON:
            case LifeEventType::TOURNAMENT_WON:
            case LifeEventType::ARTIFACT_CREATED:
            case LifeEventType::BUILDING_COMPLETED:
            case LifeEventType::PILGRIMAGE_COMPLETED:
            case LifeEventType::ILLNESS_RECOVERED:
            case LifeEventType::RECOVERY:
            case LifeEventType::PLOT_SUCCEEDED:
            case LifeEventType::COUNCIL_POSITION_GAINED:
            case LifeEventType::RELEASED:
            case LifeEventType::RETURNED_FROM_EXILE:
            case LifeEventType::FEAST_HOSTED:
            case LifeEventType::FAME_GAINED:
                return true;
            default:
                return false;
        }
    }
};

// ============================================================================
// Life Event Generator
// ============================================================================

class LifeEventGenerator {
public:
    /**
     * Generate birth event
     */
    static LifeEvent CreateBirthEvent(const std::string& character_name,
                                     const std::string& location,
                                     types::EntityID mother = 0,
                                     types::EntityID father = 0) {
        LifeEvent event(LifeEventType::BIRTH,
            character_name + " was born in " + location);
        event.age_at_event = 0;
        event.is_major = true;
        event.location = location;
        return event;
    }

    /**
     * Generate coming of age event
     */
    static LifeEvent CreateComingOfAgeEvent(const std::string& character_name, int age = 16) {
        LifeEvent event(LifeEventType::COMING_OF_AGE,
            character_name + " came of age");
        event.age_at_event = age;
        event.is_major = true;
        return event;
    }

    /**
     * Generate marriage event
     */
    static LifeEvent CreateMarriageEvent(const std::string& character_name,
                                        const std::string& spouse_name,
                                        types::EntityID spouse_id,
                                        int age) {
        LifeEvent event(LifeEventType::MARRIAGE,
            character_name + " married " + spouse_name);
        event.age_at_event = age;
        event.related_character = spouse_id;
        event.is_major = true;
        event.impact_prestige = 50.0f;
        return event;
    }

    /**
     * Generate child birth event
     */
    static LifeEvent CreateChildBirthEvent(const std::string& parent_name,
                                          const std::string& child_name,
                                          types::EntityID child_id,
                                          int parent_age) {
        LifeEvent event(LifeEventType::CHILD_BORN,
            parent_name + "'s child " + child_name + " was born");
        event.age_at_event = parent_age;
        event.related_character = child_id;
        event.is_major = true;
        event.impact_prestige = 10.0f;
        return event;
    }

    /**
     * Generate battle event
     */
    static LifeEvent CreateBattleEvent(bool won, const std::string& battle_name,
                                      int age, float prestige_change = 0.0f) {
        LifeEventType type = won ? LifeEventType::BATTLE_WON : LifeEventType::BATTLE_LOST;
        std::string desc = won ?
            "Won the battle of " + battle_name :
            "Was defeated at the battle of " + battle_name;

        LifeEvent event(type, desc);
        event.age_at_event = age;
        event.is_major = true;
        event.impact_prestige = prestige_change;
        return event;
    }

    /**
     * Generate title gain event
     */
    static LifeEvent CreateTitleGainEvent(const std::string& title_name,
                                         types::EntityID title_id,
                                         int age) {
        LifeEvent event(LifeEventType::TITLE_GAINED,
            "Gained the title: " + title_name);
        event.age_at_event = age;
        event.related_title = title_id;
        event.is_major = true;
        event.impact_prestige = 100.0f;
        return event;
    }

    /**
     * Generate death event
     */
    static LifeEvent CreateDeathEvent(LifeEventType death_type,
                                     const std::string& description,
                                     int age,
                                     types::EntityID killer = 0) {
        LifeEvent event(death_type, description);
        event.age_at_event = age;
        event.is_major = true;
        event.is_positive = false;
        if (killer != 0) {
            event.related_character = killer;
        }
        return event;
    }
};

} // namespace character
} // namespace game
