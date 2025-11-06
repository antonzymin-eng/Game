// ============================================================================
// DiplomacyRepository.h - Component Access Layer for Diplomacy
// Created: 2025-10-28 - Refactoring DiplomacySystem
// Location: include/game/diplomacy/DiplomacyRepository.h
// ============================================================================

#pragma once

#include "core/ECS/ComponentAccessManager.h"
#include "game/diplomacy/DiplomacyComponents.h"
#include "core/types/game_types.h"
#include <memory>
#include <vector>
#include <optional>

namespace game::diplomacy {

/// Repository pattern for accessing DiplomacyComponent instances
/// Encapsulates all ECS component access logic
class DiplomacyRepository {
public:
    explicit DiplomacyRepository(::core::ecs::ComponentAccessManager& access_manager);

    /// Get diplomacy component for a realm (returns nullptr if not found)
    std::shared_ptr<DiplomacyComponent> Get(types::EntityID realm_id);

    /// Get diplomacy component (const version)
    std::shared_ptr<const DiplomacyComponent> Get(types::EntityID realm_id) const;

    /// Get or create diplomacy component for a realm
    std::shared_ptr<DiplomacyComponent> GetOrCreate(types::EntityID realm_id);

    /// Check if a realm has a diplomacy component
    bool Exists(types::EntityID realm_id) const;

    /// Get all realms with diplomacy components
    std::vector<types::EntityID> GetAllRealms() const;

    /// Create a new diplomacy component with default values
    std::shared_ptr<DiplomacyComponent> Create(
        types::EntityID realm_id,
        DiplomaticPersonality personality = DiplomaticPersonality::DIPLOMATIC
    );

    /// Remove diplomacy component from a realm
    bool Remove(types::EntityID realm_id);

    /// Get two realms' components at once (for bilateral operations)
    struct RealmPair {
        std::shared_ptr<DiplomacyComponent> first;
        std::shared_ptr<DiplomacyComponent> second;
        bool both_valid() const { return first && second; }
    };

    // Read/write pair
    RealmPair GetPair(types::EntityID realm1, types::EntityID realm2);

    // Read-only pair for const callers
    struct RealmPairConst {
        std::shared_ptr<const DiplomacyComponent> first;
        std::shared_ptr<const DiplomacyComponent> second;
        bool both_valid() const { return first && second; }
    };

    RealmPairConst GetPair(types::EntityID realm1, types::EntityID realm2) const;

private:
    ::core::ecs::ComponentAccessManager& m_access_manager;

    /// Convert realm ID to ECS entity handle
    ::core::ecs::EntityID ToEntityHandle(types::EntityID realm_id) const;
};

} // namespace game::diplomacy
