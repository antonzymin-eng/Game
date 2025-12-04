// ============================================================================
// CharacterSystem.h - Character entity management system
// Created: December 3, 2025
// Location: include/game/systems/CharacterSystem.h
// Purpose: Manages character entity lifecycle, creation, and updates
// ============================================================================

#pragma once

#include "core/ECS/ComponentAccessManager.h"
#include "core/ECS/EntityManager.h"
#include "core/threading/ThreadSafeMessageBus.h"
#include "core/types/game_types.h"
#include "game/character/CharacterTypes.h"
#include "game/character/CharacterEvents.h"
#include "game/components/CharacterComponent.h"
#include "game/components/TraitsComponent.h"
#include "game/character/CharacterRelationships.h"
#include "game/character/CharacterEducation.h"
#include "game/character/CharacterLifeEvents.h"
#include "game/components/NobleArtsComponent.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>

namespace game {
namespace character {

// ============================================================================
// CharacterSystem - Main character management system
// ============================================================================

/**
 * Manages all character entities in the game world.
 *
 * Responsibilities:
 * - Create and destroy character entities
 * - Load historical characters from data files
 * - Update character lifecycles (aging, education, relationships)
 * - Track active characters and provide lookup
 *
 * Threading: BACKGROUND (independent calculations, no direct UI access)
 */
class CharacterSystem {
public:
    CharacterSystem(
        core::ecs::ComponentAccessManager& componentAccess,
        core::threading::ThreadSafeMessageBus& messageBus
    );

    ~CharacterSystem();

    // ========================================================================
    // Entity Creation and Management
    // ========================================================================

    /**
     * Create a new character entity
     * @param name Character's name
     * @param age Character's age
     * @param stats Character's attributes (diplomacy, martial, etc.)
     * @return EntityID of created character (invalid if failed)
     */
    core::ecs::EntityID CreateCharacter(
        const std::string& name,
        uint32_t age,
        const CharacterStats& stats
    );

    /**
     * Destroy a character entity
     * @param characterId Entity ID of character to destroy
     */
    void DestroyCharacter(core::ecs::EntityID characterId);

    // ========================================================================
    // Data Loading
    // ========================================================================

    /**
     * Load historical characters from JSON file
     * @param json_path Path to characters JSON file
     * @return true if load successful
     */
    bool LoadHistoricalCharacters(const std::string& json_path);

    // ========================================================================
    // Character Queries
    // ========================================================================

    /**
     * Find character by name
     * @param name Character name to search for
     * @return EntityID of character (invalid if not found)
     */
    core::ecs::EntityID GetCharacterByName(const std::string& name) const;

    /**
     * Get all active character entities
     * @return Const reference to vector of all character EntityIDs
     * @note Returns by const reference to avoid unnecessary vector copy
     */
    const std::vector<core::ecs::EntityID>& GetAllCharacters() const;

    /**
     * Get all characters belonging to a realm
     * @param realmId Entity ID of realm
     * @return Vector of character EntityIDs in that realm
     */
    std::vector<core::ecs::EntityID> GetCharactersByRealm(core::ecs::EntityID realmId) const;

    /**
     * Get total number of active characters
     */
    size_t GetCharacterCount() const { return m_allCharacters.size(); }

    // ========================================================================
    // System Lifecycle
    // ========================================================================

    /**
     * Update all character systems
     * Called each frame from main loop
     * @param deltaTime Time since last update (seconds)
     */
    void Update(float deltaTime);

    // ========================================================================
    // Integration Hooks
    // ========================================================================

    /**
     * Notification that a realm was created
     * @param realmId Entity ID of new realm (legacy types::EntityID from RealmManager)
     * @param rulerId Entity ID of ruler character (legacy types::EntityID, 0 if no ruler)
     *
     * Note: RealmManager uses legacy types::EntityID (uint32_t), so we accept that type
     * and convert internally to core::ecs::EntityID for character lookups.
     */
    void OnRealmCreated(game::types::EntityID realmId, game::types::EntityID rulerId);

    /**
     * Notification that a character died
     * @param characterId Entity ID of deceased character
     */
    void OnCharacterDeath(core::ecs::EntityID characterId);

private:
    // ========================================================================
    // Update Subsystems
    // ========================================================================

    /**
     * Update character aging (once per in-game year)
     */
    void UpdateAging(float deltaTime);

    /**
     * Update active education sessions
     */
    void UpdateEducation(float deltaTime);

    /**
     * Update relationship decay/growth
     */
    void UpdateRelationships(float deltaTime);

    /**
     * Process pending life events
     */
    void UpdateLifeEvents(float deltaTime);

    /**
     * Remove expired temporary traits
     */
    void UpdateTraits(float deltaTime);

    /**
     * Convert legacy types::EntityID to core::ecs::EntityID
     * Looks up the character in our tracking to get the versioned handle
     */
    core::ecs::EntityID LegacyToVersionedEntityID(game::types::EntityID legacy_id) const;

    // ========================================================================
    // Member Variables
    // ========================================================================

    // ECS and messaging
    core::ecs::ComponentAccessManager& m_componentAccess;
    core::threading::ThreadSafeMessageBus& m_messageBus;

    // Character tracking
    std::unordered_map<core::ecs::EntityID, std::string, core::ecs::EntityID::Hash> m_characterNames;
    std::unordered_map<std::string, core::ecs::EntityID> m_nameToEntity;
    std::unordered_map<game::types::EntityID, core::ecs::EntityID> m_legacyToVersioned;  // Legacy ID → Versioned ID mapping
    std::vector<core::ecs::EntityID> m_allCharacters;

    // Update timers
    float m_ageTimer = 0.0f;           // Timer for aging (yearly)
    float m_relationshipTimer = 0.0f;  // Timer for relationship updates

    // Shutdown flag - prevents processing events during destruction
    bool m_shuttingDown = false;
};

} // namespace character
} // namespace game
