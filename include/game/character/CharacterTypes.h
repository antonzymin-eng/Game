// ============================================================================
// CharacterTypes.h - Common types and structures for character system
// Created: December 3, 2025
// Location: include/game/character/CharacterTypes.h
// ============================================================================

#pragma once

#include "core/types/game_types.h"
#include "utils/Random.h"
#include <cstdint>

namespace game {
namespace character {

// ============================================================================
// Character Statistics Structure
// ============================================================================

/**
 * Core character statistics
 * All stats use 0-20 scale typical for grand strategy games
 */
struct CharacterStats {
    // Core attributes
    uint8_t diplomacy = 5;       // Diplomatic skill and negotiation
    uint8_t martial = 5;         // Military leadership and tactics
    uint8_t stewardship = 5;     // Administrative and economic management
    uint8_t intrigue = 5;        // Cunning, espionage, and plotting
    uint8_t learning = 5;        // Education, scholarship, and innovation

    // Status values
    float health = 100.0f;       // 0-100, affects mortality and effectiveness
    float prestige = 0.0f;       // Reputation and fame
    float gold = 0.0f;           // Personal wealth

    // Constructor with default values
    CharacterStats() = default;

    CharacterStats(uint8_t dip, uint8_t mar, uint8_t stew, uint8_t intr, uint8_t learn)
        : diplomacy(dip)
        , martial(mar)
        , stewardship(stew)
        , intrigue(intr)
        , learning(learn)
    {}

    // ========================================================================
    // Factory Methods
    // ========================================================================

    /**
     * Create stats for a capable ruler
     */
    static CharacterStats DefaultRuler() {
        CharacterStats stats;
        stats.diplomacy = 8;
        stats.martial = 7;
        stats.stewardship = 8;
        stats.intrigue = 6;
        stats.learning = 6;
        stats.health = 95.0f;
        stats.prestige = 100.0f;
        stats.gold = 1000.0f;
        return stats;
    }

    /**
     * Create stats for an exceptional ruler
     */
    static CharacterStats ExceptionalRuler() {
        CharacterStats stats;
        stats.diplomacy = 12;
        stats.martial = 11;
        stats.stewardship = 12;
        stats.intrigue = 10;
        stats.learning = 11;
        stats.health = 100.0f;
        stats.prestige = 500.0f;
        stats.gold = 5000.0f;
        return stats;
    }

    /**
     * Create stats for a military leader
     */
    static CharacterStats MilitaryLeader() {
        CharacterStats stats;
        stats.diplomacy = 5;
        stats.martial = 14;
        stats.stewardship = 6;
        stats.intrigue = 7;
        stats.learning = 5;
        stats.health = 100.0f;
        stats.prestige = 200.0f;
        stats.gold = 500.0f;
        return stats;
    }

    /**
     * Create stats for a diplomat
     */
    static CharacterStats Diplomat() {
        CharacterStats stats;
        stats.diplomacy = 14;
        stats.martial = 4;
        stats.stewardship = 8;
        stats.intrigue = 10;
        stats.learning = 9;
        stats.health = 95.0f;
        stats.prestige = 150.0f;
        stats.gold = 800.0f;
        return stats;
    }

    /**
     * Create stats for a scholar
     */
    static CharacterStats Scholar() {
        CharacterStats stats;
        stats.diplomacy = 7;
        stats.martial = 3;
        stats.stewardship = 7;
        stats.intrigue = 5;
        stats.learning = 15;
        stats.health = 90.0f;
        stats.prestige = 100.0f;
        stats.gold = 300.0f;
        return stats;
    }

    /**
     * Create average stats for a noble
     */
    static CharacterStats AverageNoble() {
        CharacterStats stats;
        stats.diplomacy = 6;
        stats.martial = 6;
        stats.stewardship = 6;
        stats.intrigue = 5;
        stats.learning = 5;
        stats.health = 95.0f;
        stats.prestige = 50.0f;
        stats.gold = 500.0f;
        return stats;
    }

    /**
     * Create random stats with variation
     * @param min_value Minimum stat value (default 3)
     * @param max_value Maximum stat value (default 15)
     */
    static CharacterStats Random(uint8_t min_value = 3, uint8_t max_value = 15) {
        CharacterStats stats;
        stats.diplomacy = static_cast<uint8_t>(utils::RandomInt(min_value, max_value));
        stats.martial = static_cast<uint8_t>(utils::RandomInt(min_value, max_value));
        stats.stewardship = static_cast<uint8_t>(utils::RandomInt(min_value, max_value));
        stats.intrigue = static_cast<uint8_t>(utils::RandomInt(min_value, max_value));
        stats.learning = static_cast<uint8_t>(utils::RandomInt(min_value, max_value));
        stats.health = utils::RandomFloat(80.0f, 100.0f);
        stats.prestige = utils::RandomFloat(0.0f, 100.0f);
        stats.gold = utils::RandomFloat(100.0f, 1000.0f);
        return stats;
    }

    /**
     * Create random stats weighted toward higher values
     */
    static CharacterStats RandomAboveAverage() {
        CharacterStats stats;
        stats.diplomacy = static_cast<uint8_t>(utils::RandomInt(6, 14));
        stats.martial = static_cast<uint8_t>(utils::RandomInt(6, 14));
        stats.stewardship = static_cast<uint8_t>(utils::RandomInt(6, 14));
        stats.intrigue = static_cast<uint8_t>(utils::RandomInt(6, 14));
        stats.learning = static_cast<uint8_t>(utils::RandomInt(6, 14));
        stats.health = utils::RandomFloat(90.0f, 100.0f);
        stats.prestige = utils::RandomFloat(50.0f, 200.0f);
        stats.gold = utils::RandomFloat(500.0f, 2000.0f);
        return stats;
    }

    // ========================================================================
    // Utility Methods
    // ========================================================================

    /**
     * Calculate total skill value (sum of all attributes)
     */
    int GetTotalSkill() const {
        return diplomacy + martial + stewardship + intrigue + learning;
    }

    /**
     * Get highest stat value
     */
    uint8_t GetHighestStat() const {
        uint8_t highest = diplomacy;
        if (martial > highest) highest = martial;
        if (stewardship > highest) highest = stewardship;
        if (intrigue > highest) highest = intrigue;
        if (learning > highest) highest = learning;
        return highest;
    }

    /**
     * Get lowest stat value
     */
    uint8_t GetLowestStat() const {
        uint8_t lowest = diplomacy;
        if (martial < lowest) lowest = martial;
        if (stewardship < lowest) lowest = stewardship;
        if (intrigue < lowest) lowest = intrigue;
        if (learning < lowest) lowest = learning;
        return lowest;
    }

    /**
     * Clamp all stats to valid range (0-20)
     */
    void ClampStats() {
        auto clamp = [](uint8_t& val, uint8_t min, uint8_t max) {
            if (val < min) val = min;
            if (val > max) val = max;
        };

        clamp(diplomacy, 0, 20);
        clamp(martial, 0, 20);
        clamp(stewardship, 0, 20);
        clamp(intrigue, 0, 20);
        clamp(learning, 0, 20);

        if (health < 0.0f) health = 0.0f;
        if (health > 100.0f) health = 100.0f;
    }
};

} // namespace character
} // namespace game
