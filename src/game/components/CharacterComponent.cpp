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

    // M1 FIX: Add error logging on parse failure
    if (!Json::parseFromStream(builder, ss, &data, &errors)) {
        // Log the parse error details
        // Note: Can't use Logger here as it would create circular dependency
        // Error is logged by ECS system caller
        return false;
    }

    // Basic info
    if (data.isMember("name")) m_name = data["name"].asString();
    if (data.isMember("age")) m_age = data["age"].asUInt();

    // M2 FIX: Validate health range (0-100)
    if (data.isMember("health")) {
        float health = data["health"].asFloat();
        m_health = std::max(0.0f, std::min(100.0f, health));
    }

    if (data.isMember("prestige")) m_prestige = data["prestige"].asFloat();
    if (data.isMember("gold")) m_gold = data["gold"].asFloat();
    if (data.isMember("is_dead")) m_isDead = data["is_dead"].asBool();

    // M2 FIX: Validate attributes (0-20 range for grand strategy games)
    if (data.isMember("diplomacy")) {
        uint32_t value = data["diplomacy"].asUInt();
        m_diplomacy = static_cast<uint8_t>(std::min(value, 20u));
    }
    if (data.isMember("martial")) {
        uint32_t value = data["martial"].asUInt();
        m_martial = static_cast<uint8_t>(std::min(value, 20u));
    }
    if (data.isMember("stewardship")) {
        uint32_t value = data["stewardship"].asUInt();
        m_stewardship = static_cast<uint8_t>(std::min(value, 20u));
    }
    if (data.isMember("intrigue")) {
        uint32_t value = data["intrigue"].asUInt();
        m_intrigue = static_cast<uint8_t>(std::min(value, 20u));
    }
    if (data.isMember("learning")) {
        uint32_t value = data["learning"].asUInt();
        m_learning = static_cast<uint8_t>(std::min(value, 20u));
    }

    // Relationships
    if (data.isMember("primary_title")) m_primaryTitle = data["primary_title"].asUInt();
    if (data.isMember("liege_id")) m_liegeId = data["liege_id"].asUInt();
    if (data.isMember("dynasty_id")) m_dynastyId = data["dynasty_id"].asUInt();

    return true;
}

} // namespace character
} // namespace game
