// Created: December 5, 2025
// Location: src/game/components/CharacterComponent.cpp
// Purpose: Character component serialization implementation (Phase 6)

#include "game/components/CharacterComponent.h"
#include <json/json.h>
#include <sstream>

namespace game {
namespace character {

std::string CharacterComponent::Serialize() const {
    Json::Value data;

    // Basic info
    data["name"] = m_name;
    data["age"] = m_age;
    data["health"] = m_health;
    data["prestige"] = m_prestige;
    data["gold"] = m_gold;
    data["is_dead"] = m_isDead;

    // Attributes
    data["diplomacy"] = m_diplomacy;
    data["martial"] = m_martial;
    data["stewardship"] = m_stewardship;
    data["intrigue"] = m_intrigue;
    data["learning"] = m_learning;

    // Relationships (legacy EntityIDs)
    data["primary_title"] = m_primaryTitle;
    data["liege_id"] = m_liegeId;
    data["dynasty_id"] = m_dynastyId;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";  // Compact JSON
    return Json::writeString(builder, data);
}

bool CharacterComponent::Deserialize(const std::string& json_str) {
    Json::Value data;
    Json::CharReaderBuilder builder;
    std::stringstream ss(json_str);
    std::string errors;

    if (!Json::parseFromStream(builder, ss, &data, &errors)) {
        return false;
    }

    // Basic info
    if (data.isMember("name")) m_name = data["name"].asString();
    if (data.isMember("age")) m_age = data["age"].asUInt();
    if (data.isMember("health")) m_health = data["health"].asFloat();
    if (data.isMember("prestige")) m_prestige = data["prestige"].asFloat();
    if (data.isMember("gold")) m_gold = data["gold"].asFloat();
    if (data.isMember("is_dead")) m_isDead = data["is_dead"].asBool();

    // Attributes
    if (data.isMember("diplomacy")) m_diplomacy = static_cast<uint8_t>(data["diplomacy"].asUInt());
    if (data.isMember("martial")) m_martial = static_cast<uint8_t>(data["martial"].asUInt());
    if (data.isMember("stewardship")) m_stewardship = static_cast<uint8_t>(data["stewardship"].asUInt());
    if (data.isMember("intrigue")) m_intrigue = static_cast<uint8_t>(data["intrigue"].asUInt());
    if (data.isMember("learning")) m_learning = static_cast<uint8_t>(data["learning"].asUInt());

    // Relationships
    if (data.isMember("primary_title")) m_primaryTitle = data["primary_title"].asUInt();
    if (data.isMember("liege_id")) m_liegeId = data["liege_id"].asUInt();
    if (data.isMember("dynasty_id")) m_dynastyId = data["dynasty_id"].asUInt();

    return true;
}

} // namespace character
} // namespace game
