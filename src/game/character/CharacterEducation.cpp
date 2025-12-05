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
    }

    // Deserialize time_points from milliseconds
    if (data.isMember("education_start")) {
        auto start_ms = data["education_start"].asInt64();
        education_start = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(start_ms));
    }

    if (data.isMember("education_end")) {
        auto end_ms = data["education_end"].asInt64();
        education_end = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(end_ms));
    }

    // Skill experience
    if (data.isMember("skill_xp") && data["skill_xp"].isObject()) {
        const Json::Value& skill_data = data["skill_xp"];

        if (skill_data.isMember("diplomacy_xp")) {
            skill_xp.diplomacy_xp = skill_data["diplomacy_xp"].asInt();
        }
        if (skill_data.isMember("martial_xp")) {
            skill_xp.martial_xp = skill_data["martial_xp"].asInt();
        }
        if (skill_data.isMember("stewardship_xp")) {
            skill_xp.stewardship_xp = skill_data["stewardship_xp"].asInt();
        }
        if (skill_data.isMember("intrigue_xp")) {
            skill_xp.intrigue_xp = skill_data["intrigue_xp"].asInt();
        }
        if (skill_data.isMember("learning_xp")) {
            skill_xp.learning_xp = skill_data["learning_xp"].asInt();
        }
    }

    // Learning modifier
    if (data.isMember("learning_rate_modifier")) {
        learning_rate_modifier = data["learning_rate_modifier"].asFloat();
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
