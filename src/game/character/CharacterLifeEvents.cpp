// Created: December 5, 2025
// Location: src/game/character/CharacterLifeEvents.cpp
// Purpose: Character life events component serialization (Phase 6.5)

#include "game/character/CharacterLifeEvents.h"
#include "core/save/SerializationConstants.h"
#include <json/json.h>
#include <sstream>

namespace game {
namespace character {

// Helper: Serialize a single LifeEvent to Json::Value
static Json::Value SerializeLifeEvent(const LifeEvent& event) {
    Json::Value event_data;

    // Basic info
    event_data["type"] = static_cast<int>(event.type);
    event_data["description"] = event.description;

    // Serialize time_point as milliseconds since epoch
    auto date_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        event.date.time_since_epoch()).count();
    event_data["date"] = Json::Int64(date_ms);

    // Related entities
    event_data["related_character"] = event.related_character;
    event_data["related_realm"] = event.related_realm;
    event_data["related_title"] = event.related_title;

    // Event details
    event_data["location"] = event.location;
    event_data["age_at_event"] = event.age_at_event;
    event_data["impact_prestige"] = event.impact_prestige;
    event_data["impact_health"] = event.impact_health;

    // Traits gained/lost
    Json::Value traits_gained_array(Json::arrayValue);
    for (const auto& trait : event.traits_gained) {
        traits_gained_array.append(trait);
    }
    event_data["traits_gained"] = traits_gained_array;

    Json::Value traits_lost_array(Json::arrayValue);
    for (const auto& trait : event.traits_lost) {
        traits_lost_array.append(trait);
    }
    event_data["traits_lost"] = traits_lost_array;

    // Flags
    event_data["is_positive"] = event.is_positive;
    event_data["is_major"] = event.is_major;
    event_data["is_secret"] = event.is_secret;

    return event_data;
}

// Helper: Deserialize a single LifeEvent from Json::Value
static LifeEvent DeserializeLifeEvent(const Json::Value& event_data) {
    LifeEvent event;

    // Basic info
    if (event_data.isMember("type")) {
        int type_int = event_data["type"].asInt();
        if (type_int >= 0 && type_int < static_cast<int>(LifeEventType::COUNT)) {
            event.type = static_cast<LifeEventType>(type_int);
        }
    }

    if (event_data.isMember("description")) {
        event.description = event_data["description"].asString();
    }

    // Deserialize time_point from milliseconds
    if (event_data.isMember("date")) {
        auto date_ms = event_data["date"].asInt64();
        event.date = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(date_ms));
    }

    // Related entities
    if (event_data.isMember("related_character")) {
        event.related_character = event_data["related_character"].asUInt();
    }
    if (event_data.isMember("related_realm")) {
        event.related_realm = event_data["related_realm"].asUInt();
    }
    if (event_data.isMember("related_title")) {
        event.related_title = event_data["related_title"].asUInt();
    }

    // Event details
    if (event_data.isMember("location")) {
        event.location = event_data["location"].asString();
    }
    if (event_data.isMember("age_at_event")) {
        event.age_at_event = game::core::serialization::Clamp(
            event_data["age_at_event"].asInt(),
            game::core::serialization::MIN_AGE,
            game::core::serialization::MAX_AGE);
    }

    // Impact values with bounds checking
    if (event_data.isMember("impact_prestige")) {
        event.impact_prestige = game::core::serialization::Clamp(
            event_data["impact_prestige"].asFloat(),
            game::core::serialization::MIN_IMPACT_VALUE,
            game::core::serialization::MAX_IMPACT_VALUE);
    }
    if (event_data.isMember("impact_health")) {
        event.impact_health = game::core::serialization::Clamp(
            event_data["impact_health"].asFloat(),
            game::core::serialization::MIN_IMPACT_VALUE,
            game::core::serialization::MAX_IMPACT_VALUE);
    }

    // Traits gained/lost
    if (event_data.isMember("traits_gained") && event_data["traits_gained"].isArray()) {
        const Json::Value& traits_gained_array = event_data["traits_gained"];
        for (const auto& trait : traits_gained_array) {
            if (trait.isString()) {
                event.traits_gained.push_back(trait.asString());
            }
        }
    }

    if (event_data.isMember("traits_lost") && event_data["traits_lost"].isArray()) {
        const Json::Value& traits_lost_array = event_data["traits_lost"];
        for (const auto& trait : traits_lost_array) {
            if (trait.isString()) {
                event.traits_lost.push_back(trait.asString());
            }
        }
    }

    // Flags
    if (event_data.isMember("is_positive")) {
        event.is_positive = event_data["is_positive"].asBool();
    }
    if (event_data.isMember("is_major")) {
        event.is_major = event_data["is_major"].asBool();
    }
    if (event_data.isMember("is_secret")) {
        event.is_secret = event_data["is_secret"].asBool();
    }

    return event;
}

std::string CharacterLifeEventsComponent::Serialize() const {
    Json::Value data;

    // Schema version for future migration support
    data["schema_version"] = game::core::serialization::CHARACTER_LIFE_EVENTS_VERSION;

    // Character ID
    data["character_id"] = character_id;

    // Quick access dates
    auto birth_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        birth_date.time_since_epoch()).count();
    data["birth_date"] = Json::Int64(birth_ms);

    auto coming_of_age_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        coming_of_age_date.time_since_epoch()).count();
    data["coming_of_age_date"] = Json::Int64(coming_of_age_ms);

    auto death_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        death_date.time_since_epoch()).count();
    data["death_date"] = Json::Int64(death_ms);

    // Serialize all life events
    Json::Value events_array(Json::arrayValue);
    for (const auto& event : life_events) {
        events_array.append(SerializeLifeEvent(event));
    }
    data["life_events"] = events_array;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";  // Compact JSON
    return Json::writeString(builder, data);
}

bool CharacterLifeEventsComponent::Deserialize(const std::string& json_str) {
    Json::Value data;
    Json::CharReaderBuilder builder;
    std::stringstream ss(json_str);
    std::string errors;

    if (!Json::parseFromStream(builder, ss, &data, &errors)) {
        return false;
    }

    // Character ID
    if (data.isMember("character_id")) {
        character_id = data["character_id"].asUInt();
    }

    // Quick access dates
    if (data.isMember("birth_date")) {
        auto birth_ms = data["birth_date"].asInt64();
        birth_date = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(birth_ms));
    }

    if (data.isMember("coming_of_age_date")) {
        auto coming_of_age_ms = data["coming_of_age_date"].asInt64();
        coming_of_age_date = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(coming_of_age_ms));
    }

    if (data.isMember("death_date")) {
        auto death_ms = data["death_date"].asInt64();
        death_date = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(death_ms));
    }

    // Check schema version
    if (data.isMember("schema_version")) {
        int version = data["schema_version"].asInt();
        if (version > game::core::serialization::CHARACTER_LIFE_EVENTS_VERSION) {
            // Future: handle migration from older versions
        }
    }

    // Deserialize all life events with count limit
    if (data.isMember("life_events") && data["life_events"].isArray()) {
        life_events.clear();
        const Json::Value& events_array = data["life_events"];

        // Limit to prevent DoS from corrupted saves
        size_t max_events = std::min(events_array.size(),
            static_cast<size_t>(game::core::serialization::MAX_LIFE_EVENTS));

        for (Json::ArrayIndex i = 0; i < max_events; ++i) {
            life_events.push_back(DeserializeLifeEvent(events_array[i]));
        }

        // Sort events by date to ensure chronological order
        std::sort(life_events.begin(), life_events.end(),
            [](const LifeEvent& a, const LifeEvent& b) {
                return a.date < b.date;
            });
    }

    return true;
}

} // namespace character
} // namespace game
