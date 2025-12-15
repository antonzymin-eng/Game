// Created: November 10, 2025
// Location: include/game/components/CharacterComponent.h

#pragma once

#include "utils/PlatformMacros.h"
#include "core/ECS/IComponent.h"
#include "core/types/game_types.h"
#include <memory>
#include <string>

namespace game {
namespace character {

// Character component for character entities
// Represents a character in the game world with basic attributes
class CharacterComponent : public game::core::Component<CharacterComponent> {
public:
    CharacterComponent()
        : m_name("")
        , m_age(18)
        , m_health(100.0f)
        , m_prestige(0.0f)
        , m_gold(0.0f)
        , m_diplomacy(5)
        , m_martial(5)
        , m_stewardship(5)
        , m_intrigue(5)
        , m_learning(5)
        , m_primaryTitle(0)
        , m_liegeId(0)
        , m_dynastyId(0)
        , m_isDead(false) {}

    ~CharacterComponent() = default;

    // Basic info accessors
    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }

    uint32_t GetAge() const { return m_age; }
    void SetAge(uint32_t age) { m_age = age; }

    // Status
    float GetHealth() const { return m_health; }
    void SetHealth(float health) { m_health = health; }

    float GetPrestige() const { return m_prestige; }
    void SetPrestige(float prestige) { m_prestige = prestige; }

    float GetGold() const { return m_gold; }
    void SetGold(float gold) { m_gold = gold; }

    bool IsDead() const { return m_isDead; }
    void SetDead(bool dead) { m_isDead = dead; }

    // Attributes (0-20 scale typical for grand strategy games)
    uint8_t GetDiplomacy() const { return m_diplomacy; }
    void SetDiplomacy(uint8_t value) { m_diplomacy = value; }

    uint8_t GetMartial() const { return m_martial; }
    void SetMartial(uint8_t value) { m_martial = value; }

    uint8_t GetStewardship() const { return m_stewardship; }
    void SetStewardship(uint8_t value) { m_stewardship = value; }

    uint8_t GetIntrigue() const { return m_intrigue; }
    void SetIntrigue(uint8_t value) { m_intrigue = value; }

    uint8_t GetLearning() const { return m_learning; }
    void SetLearning(uint8_t value) { m_learning = value; }

    // Relationships
    ::game::types::EntityID GetPrimaryTitle() const { return m_primaryTitle; }
    void SetPrimaryTitle(::game::types::EntityID titleId) { m_primaryTitle = titleId; }

    ::game::types::EntityID GetLiegeId() const { return m_liegeId; }
    void SetLiegeId(::game::types::EntityID liegeId) { m_liegeId = liegeId; }

    ::game::types::EntityID GetDynastyId() const { return m_dynastyId; }
    void SetDynastyId(::game::types::EntityID dynastyId) { m_dynastyId = dynastyId; }

    // Component interface
    std::unique_ptr<game::core::IComponent> Clone() const override {
        auto clone = std::make_unique<CharacterComponent>();
        clone->m_name = m_name;
        clone->m_age = m_age;
        clone->m_health = m_health;
        clone->m_prestige = m_prestige;
        clone->m_gold = m_gold;
        clone->m_diplomacy = m_diplomacy;
        clone->m_martial = m_martial;
        clone->m_stewardship = m_stewardship;
        clone->m_intrigue = m_intrigue;
        clone->m_learning = m_learning;
        clone->m_primaryTitle = m_primaryTitle;
        clone->m_liegeId = m_liegeId;
        clone->m_dynastyId = m_dynastyId;
        clone->m_isDead = m_isDead;
        return clone;
    }

    // Serialization (Phase 6)
    std::string Serialize() const override;
    bool Deserialize(const std::string& data) override;

private:
    std::string m_name;
    uint32_t m_age;
    float m_health;
    float m_prestige;
    float m_gold;

    // Character attributes
    uint8_t m_diplomacy;
    uint8_t m_martial;
    uint8_t m_stewardship;
    uint8_t m_intrigue;
    uint8_t m_learning;

    // Relationships
    ::game::types::EntityID m_primaryTitle;
    ::game::types::EntityID m_liegeId;
    ::game::types::EntityID m_dynastyId;

    bool m_isDead;
};

} // namespace character
} // namespace game
