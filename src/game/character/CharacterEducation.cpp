// Created: December 5, 2025
// Location: src/game/character/CharacterEducation.cpp
// Purpose: Character education component serialization (Phase 6.5)

#include "game/character/CharacterEducation.h"
#include <json/json.h>
#include <sstream>

namespace game {
namespace character {

std::string CharacterEducationComponent::Serialize() const {
    Json::Value data;

    // Schema version for future migration support
    data["schema_version"] = 1;

    // Character ID
    data["character_id"] = character_id;

    // Education status
    data["is_educated"] = is_educated;
    data["education_focus"] = static_cast<int>(education_focus);
    data["education_quality"] = static_cast<int>(education_quality);
    data["educator"] = educator;

    // Serialize time_points as milliseconds since epoch
    auto start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        education_start.time_since_epoch()).count();
    data["education_start"] = Json::Int64(start_ms);

    auto end_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        education_end.time_since_epoch()).count();
    data["education_end"] = Json::Int64(end_ms);

    // Skill experience
    Json::Value skill_data;
    skill_data["diplomacy_xp"] = skill_xp.diplomacy_xp;
    skill_data["martial_xp"] = skill_xp.martial_xp;
    skill_data["stewardship_xp"] = skill_xp.stewardship_xp;
    skill_data["intrigue_xp"] = skill_xp.intrigue_xp;
    skill_data["learning_xp"] = skill_xp.learning_xp;
    data["skill_xp"] = skill_data;

    // Learning modifier
    data["learning_rate_modifier"] = learning_rate_modifier;

    // Education traits
    Json::Value traits_array(Json::arrayValue);
    for (const auto& trait : education_traits) {
        traits_array.append(trait);
    }
    data["education_traits"] = traits_array;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";  // Compact JSON
    return Json::writeString(builder, data);
}

bool CharacterEducationComponent::Deserialize(const std::string& json_str) {
    Json::Value data;
    Json::CharReaderBuilder builder;
    std::stringstream ss(json_str);
    std::string errors;

    if (!Json::parseFromStream(builder, ss, &data, &errors)) {
        return false;
    }

    // Check schema version
    if (data.isMember("schema_version")) {
        int version = data["schema_version"].asInt();
        if (version > 1) {
            // Future: handle migration from older versions
        }
    }

    // Character ID
    if (data.isMember("character_id")) {
        character_id = data["character_id"].asUInt();
    }

    // Education status
    if (data.isMember("is_educated")) {
        is_educated = data["is_educated"].asBool();
    }

    if (data.isMember("education_focus")) {
        int focus_int = data["education_focus"].asInt();
        if (focus_int >= 0 && focus_int < static_cast<int>(EducationFocus::COUNT)) {
            education_focus = static_cast<EducationFocus>(focus_int);
        }
    }

    if (data.isMember("education_quality")) {
        int quality_int = data["education_quality"].asInt();
        if (quality_int >= 0 && quality_int < static_cast<int>(EducationQuality::COUNT)) {
            education_quality = static_cast<EducationQuality>(quality_int);
        }
    }

    if (data.isMember("educator")) {
        educator = data["educator"].asUInt();
        // Note: EntityID validation should be done at system level after all entities loaded
    }

    // Deserialize time_points from milliseconds with validation
    if (data.isMember("education_start")) {
        auto start_ms = data["education_start"].asInt64();
        if (start_ms >= 0 && start_ms <= 4102444800000) {
            education_start = std::chrono::system_clock::time_point(
                std::chrono::milliseconds(start_ms));
        }
    }

    if (data.isMember("education_end")) {
        auto end_ms = data["education_end"].asInt64();
        if (end_ms >= 0 && end_ms <= 4102444800000) {
            education_end = std::chrono::system_clock::time_point(
                std::chrono::milliseconds(end_ms));
        }
    }

    // Skill experience with bounds checking (0 to 100,000)
    if (data.isMember("skill_xp") && data["skill_xp"].isObject()) {
        const Json::Value& skill_data = data["skill_xp"];

        auto clamp_xp = [](int xp) -> int {
            return (xp < 0) ? 0 : (xp > 100000) ? 100000 : xp;
        };

        if (skill_data.isMember("diplomacy_xp")) {
            skill_xp.diplomacy_xp = clamp_xp(skill_data["diplomacy_xp"].asInt());
        }
        if (skill_data.isMember("martial_xp")) {
            skill_xp.martial_xp = clamp_xp(skill_data["martial_xp"].asInt());
        }
        if (skill_data.isMember("stewardship_xp")) {
            skill_xp.stewardship_xp = clamp_xp(skill_data["stewardship_xp"].asInt());
        }
        if (skill_data.isMember("intrigue_xp")) {
            skill_xp.intrigue_xp = clamp_xp(skill_data["intrigue_xp"].asInt());
        }
        if (skill_data.isMember("learning_xp")) {
            skill_xp.learning_xp = clamp_xp(skill_data["learning_xp"].asInt());
        }
    }

    // Learning modifier with bounds checking (0.1 to 5.0)
    if (data.isMember("learning_rate_modifier")) {
        float rate = data["learning_rate_modifier"].asFloat();
        learning_rate_modifier = (rate < 0.1f) ? 0.1f : (rate > 5.0f) ? 5.0f : rate;
    }

    // Education traits
    if (data.isMember("education_traits") && data["education_traits"].isArray()) {
        education_traits.clear();
        const Json::Value& traits_array = data["education_traits"];
        for (const auto& trait : traits_array) {
            if (trait.isString()) {
                education_traits.push_back(trait.asString());
            }
        }
    }

    return true;
}

} // namespace character
} // namespace game
