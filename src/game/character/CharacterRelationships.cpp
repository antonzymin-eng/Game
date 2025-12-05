// Created: December 5, 2025
// Location: src/game/character/CharacterRelationships.cpp
// Purpose: Character relationships component serialization (Phase 6.5)

#include "game/character/CharacterRelationships.h"
#include <json/json.h>
#include <sstream>

namespace game {
namespace character {

// Helper: Serialize a single Marriage to Json::Value
static Json::Value SerializeMarriage(const Marriage& marriage) {
    Json::Value marriage_data;

    marriage_data["spouse"] = marriage.spouse;
    marriage_data["realm_of_spouse"] = marriage.realm_of_spouse;
    marriage_data["spouse_dynasty"] = marriage.spouse_dynasty;
    marriage_data["type"] = static_cast<int>(marriage.type);

    // Serialize time_point as milliseconds since epoch
    auto date_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        marriage.marriage_date.time_since_epoch()).count();
    marriage_data["marriage_date"] = Json::Int64(date_ms);

    marriage_data["is_alliance"] = marriage.is_alliance;

    // Serialize children
    Json::Value children_array(Json::arrayValue);
    for (const auto& child : marriage.children) {
        children_array.append(child);
    }
    marriage_data["children"] = children_array;

    return marriage_data;
}

// Helper: Deserialize a single Marriage from Json::Value
static Marriage DeserializeMarriage(const Json::Value& marriage_data) {
    Marriage marriage;

    if (marriage_data.isMember("spouse")) {
        marriage.spouse = marriage_data["spouse"].asUInt();
    }
    if (marriage_data.isMember("realm_of_spouse")) {
        marriage.realm_of_spouse = marriage_data["realm_of_spouse"].asUInt();
    }
    if (marriage_data.isMember("spouse_dynasty")) {
        marriage.spouse_dynasty = marriage_data["spouse_dynasty"].asUInt();
    }

    if (marriage_data.isMember("type")) {
        int type_int = marriage_data["type"].asInt();
        if (type_int >= 0 && type_int < static_cast<int>(MarriageType::COUNT)) {
            marriage.type = static_cast<MarriageType>(type_int);
        }
    }

    // Deserialize time_point from milliseconds
    if (marriage_data.isMember("marriage_date")) {
        auto date_ms = marriage_data["marriage_date"].asInt64();
        marriage.marriage_date = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(date_ms));
    }

    if (marriage_data.isMember("is_alliance")) {
        marriage.is_alliance = marriage_data["is_alliance"].asBool();
    }

    // Deserialize children
    if (marriage_data.isMember("children") && marriage_data["children"].isArray()) {
        const Json::Value& children_array = marriage_data["children"];
        for (const auto& child : children_array) {
            if (child.isUInt()) {
                marriage.children.push_back(child.asUInt());
            }
        }
    }

    return marriage;
}

// Helper: Serialize a single CharacterRelationship to Json::Value
static Json::Value SerializeRelationship(const CharacterRelationship& rel) {
    Json::Value rel_data;

    rel_data["other_character"] = rel.other_character;
    rel_data["type"] = static_cast<int>(rel.type);
    rel_data["opinion"] = rel.opinion;
    rel_data["bond_strength"] = rel.bond_strength;

    // Serialize time_points as milliseconds since epoch
    auto established_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        rel.established_date.time_since_epoch()).count();
    rel_data["established_date"] = Json::Int64(established_ms);

    auto interaction_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        rel.last_interaction.time_since_epoch()).count();
    rel_data["last_interaction"] = Json::Int64(interaction_ms);

    rel_data["is_active"] = rel.is_active;

    return rel_data;
}

// Helper: Deserialize a single CharacterRelationship from Json::Value
static CharacterRelationship DeserializeRelationship(const Json::Value& rel_data) {
    CharacterRelationship rel;

    if (rel_data.isMember("other_character")) {
        rel.other_character = rel_data["other_character"].asUInt();
    }

    if (rel_data.isMember("type")) {
        int type_int = rel_data["type"].asInt();
        if (type_int >= 0 && type_int < static_cast<int>(RelationshipType::COUNT)) {
            rel.type = static_cast<RelationshipType>(type_int);
        }
    }

    if (rel_data.isMember("opinion")) {
        rel.opinion = rel_data["opinion"].asInt();
    }
    if (rel_data.isMember("bond_strength")) {
        rel.bond_strength = rel_data["bond_strength"].asDouble();
    }

    // Deserialize time_points from milliseconds
    if (rel_data.isMember("established_date")) {
        auto established_ms = rel_data["established_date"].asInt64();
        rel.established_date = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(established_ms));
    }

    if (rel_data.isMember("last_interaction")) {
        auto interaction_ms = rel_data["last_interaction"].asInt64();
        rel.last_interaction = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(interaction_ms));
    }

    if (rel_data.isMember("is_active")) {
        rel.is_active = rel_data["is_active"].asBool();
    }

    return rel;
}

std::string CharacterRelationshipsComponent::Serialize() const {
    Json::Value data;

    // Character ID
    data["character_id"] = character_id;

    // Current spouse
    data["current_spouse"] = current_spouse;

    // Serialize marriages
    Json::Value marriages_array(Json::arrayValue);
    for (const auto& marriage : marriages) {
        marriages_array.append(SerializeMarriage(marriage));
    }
    data["marriages"] = marriages_array;

    // Serialize relationships (unordered_map)
    Json::Value relationships_array(Json::arrayValue);
    for (const auto& [entity_id, rel] : relationships) {
        Json::Value rel_entry = SerializeRelationship(rel);
        // Note: other_character is already in the relationship data
        relationships_array.append(rel_entry);
    }
    data["relationships"] = relationships_array;

    // Serialize family ties
    Json::Value children_array(Json::arrayValue);
    for (const auto& child : children) {
        children_array.append(child);
    }
    data["children"] = children_array;

    Json::Value siblings_array(Json::arrayValue);
    for (const auto& sibling : siblings) {
        siblings_array.append(sibling);
    }
    data["siblings"] = siblings_array;

    data["father"] = father;
    data["mother"] = mother;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";  // Compact JSON
    return Json::writeString(builder, data);
}

bool CharacterRelationshipsComponent::Deserialize(const std::string& json_str) {
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

    // Current spouse
    if (data.isMember("current_spouse")) {
        current_spouse = data["current_spouse"].asUInt();
    }

    // Deserialize marriages
    if (data.isMember("marriages") && data["marriages"].isArray()) {
        marriages.clear();
        const Json::Value& marriages_array = data["marriages"];
        for (const auto& marriage_data : marriages_array) {
            marriages.push_back(DeserializeMarriage(marriage_data));
        }
    }

    // Deserialize relationships
    if (data.isMember("relationships") && data["relationships"].isArray()) {
        relationships.clear();
        const Json::Value& relationships_array = data["relationships"];
        for (const auto& rel_data : relationships_array) {
            CharacterRelationship rel = DeserializeRelationship(rel_data);
            // Use other_character as the key in the map
            relationships[rel.other_character] = rel;
        }
    }

    // Deserialize family ties
    if (data.isMember("children") && data["children"].isArray()) {
        children.clear();
        const Json::Value& children_array = data["children"];
        for (const auto& child : children_array) {
            if (child.isUInt()) {
                children.push_back(child.asUInt());
            }
        }
    }

    if (data.isMember("siblings") && data["siblings"].isArray()) {
        siblings.clear();
        const Json::Value& siblings_array = data["siblings"];
        for (const auto& sibling : siblings_array) {
            if (sibling.isUInt()) {
                siblings.push_back(sibling.asUInt());
            }
        }
    }

    if (data.isMember("father")) {
        father = data["father"].asUInt();
    }
    if (data.isMember("mother")) {
        mother = data["mother"].asUInt();
    }

    return true;
}

} // namespace character
} // namespace game
