// ============================================================================
// InfluenceSystem.h - Sphere of Influence Management System
// Created: November 11, 2025 - Phase 3: Sphere of Influence System
// Location: include/game/diplomacy/InfluenceSystem.h
// ============================================================================

#pragma once

#include "game/diplomacy/InfluenceComponents.h"
#include "game/diplomacy/InfluenceCalculator.h"
#include "game/diplomacy/DiplomacyComponents.h"
#include "game/realm/RealmComponents.h"
#include "core/ECS/ISystem.h"
#include "core/types/game_types.h"
#include <unordered_map>
#include <vector>
#include <queue>
#include <memory>

namespace game {
namespace diplomacy {

// Forward declarations
class DiplomacySystem;
namespace realm = game::realm;

/**
 * @brief Main system for managing sphere of influence mechanics
 *
 * Responsibilities:
 * - Calculate influence projections for all realms
 * - Propagate influence through realm networks
 * - Detect sphere conflicts and flashpoints
 * - Update autonomy and diplomatic freedom
 * - Integrate with diplomacy system
 */
class InfluenceSystem {
public:
    InfluenceSystem();
    ~InfluenceSystem() = default;

    // ========================================================================
    // Initialization and Updates
    // ========================================================================

    /**
     * Initialize the influence system
     * Should be called after realm and diplomacy systems are initialized
     */
    void Initialize();

    /**
     * Monthly update - recalculate all influences
     * Called by main game loop
     */
    void MonthlyUpdate();

    /**
     * Update influence for a specific realm
     * Useful for immediate recalculation after major events
     */
    void UpdateRealmInfluence(types::EntityID realm_id);

    // ========================================================================
    // Influence Calculation
    // ========================================================================

    /**
     * Calculate all types of influence that a realm projects
     * Updates the realm's InfluenceComponent with projection values
     */
    void CalculateInfluenceProjection(types::EntityID realm_id);

    /**
     * Calculate influence between two specific realms
     * Returns InfluenceSource object with calculated values
     */
    InfluenceSource CalculateInfluenceBetween(
        types::EntityID source_realm,
        types::EntityID target_realm,
        InfluenceType type);

    /**
     * Calculate effective influence considering distance and relationship
     */
    void ApplyModifiersToInfluence(
        InfluenceSource& influence,
        int hops,
        const std::vector<types::EntityID>& path,
        int opinion);

    // ========================================================================
    // Influence Propagation
    // ========================================================================

    /**
     * Propagate influence from a realm to all reachable targets
     * Uses breadth-first search through realm network
     */
    void PropagateInfluence(types::EntityID source_realm);

    /**
     * Find shortest path between two realms
     * Returns path as vector of realm IDs, empty if no path exists
     */
    std::vector<types::EntityID> FindPathBetweenRealms(
        types::EntityID source,
        types::EntityID target,
        int max_hops = 10);

    /**
     * Get all realms within N hops of source realm
     * Used for determining influence range
     */
    std::vector<types::EntityID> GetRealmsWithinRange(
        types::EntityID source,
        int max_hops);

    /**
     * Check if two realms are connected through diplomatic/vassal network
     */
    bool AreRealmsConnected(types::EntityID realm1, types::EntityID realm2);

    // ========================================================================
    // Sphere of Influence Management
    // ========================================================================

    /**
     * Update sphere metrics for a realm
     * Categorizes influenced realms into core/peripheral/contested
     */
    void UpdateSphereMetrics(types::EntityID realm_id);

    /**
     * Detect conflicts between competing spheres
     * Returns list of new or updated conflicts
     */
    std::vector<InfluenceConflict> DetectSphereConflicts(types::EntityID realm_id);

    /**
     * Check for flashpoints (high-tension conflicts)
     * Returns list of conflicts that have become flashpoints
     */
    std::vector<InfluenceConflict> CheckForFlashpoints();

    /**
     * Update all sphere conflicts
     * Recalculates tension and escalation risk
     */
    void UpdateSphereConflicts();

    /**
     * Process conflict escalation for high-tension conflicts
     * Handles progression: tension → incidents → crisis → potential war
     */
    void ProcessConflictEscalation();

    /**
     * Resolve a specific sphere conflict
     * Determines outcomes: backing down, diplomatic resolution, crisis, or war
     */
    void ResolveSphereConflict(InfluenceConflict& conflict);

    /**
     * Generate diplomatic incident from sphere conflict
     * Adds incident to both realms' diplomatic relationships
     */
    void GenerateDiplomaticIncident(
        const InfluenceConflict& conflict,
        const std::string& incident_type);

    /**
     * Calculate AI response to sphere competition
     * Returns: 0 = back down, 1 = hold ground, 2 = escalate
     */
    int CalculateAICompetitionResponse(
        types::EntityID realm_id,
        const InfluenceConflict& conflict);

    /**
     * Apply conflict outcome effects
     * Modifies influence, opinion, prestige based on resolution
     */
    void ApplyConflictOutcome(
        const InfluenceConflict& conflict,
        types::EntityID winner,
        types::EntityID loser,
        bool peaceful_resolution);

    // ========================================================================
    // Vassal and Character Influence
    // ========================================================================

    /**
     * Update foreign influence on a realm's vassals
     * Detects when vassals are being influenced by external powers
     */
    void UpdateVassalInfluences(types::EntityID realm_id);

    /**
     * Update foreign influence on characters
     * Tracks characters who may be compromised
     */
    void UpdateCharacterInfluences(types::EntityID realm_id);

    /**
     * Check if a vassal is at risk of defection
     */
    bool IsVassalAtRiskOfDefection(const VassalInfluence& vassal_influence);

    /**
     * Check if a character is compromised by foreign influence
     */
    bool IsCharacterCompromised(const CharacterInfluence& character_influence);

    // ========================================================================
    // Query Functions
    // ========================================================================

    /**
     * Get influence component for a realm
     */
    InfluenceComponent* GetInfluenceComponent(types::EntityID realm_id);

    /**
     * Get all realms in a realm's sphere of influence
     */
    std::vector<types::EntityID> GetSphereOfInfluence(types::EntityID realm_id);

    /**
     * Get dominant influencer of a realm for a specific type
     */
    types::EntityID GetDominantInfluencer(types::EntityID realm_id, InfluenceType type);

    /**
     * Get total influence strength on a realm
     */
    double GetTotalInfluenceOn(types::EntityID realm_id);

    /**
     * Get autonomy level of a realm (0-1)
     */
    double GetRealmAutonomy(types::EntityID realm_id);

    /**
     * Get diplomatic freedom of a realm (0-1)
     */
    double GetRealmDiplomaticFreedom(types::EntityID realm_id);

    /**
     * Check if two realms are competing over a third
     */
    bool AreRealmsCompeting(
        types::EntityID realm1,
        types::EntityID realm2,
        types::EntityID contested_realm);

    // ========================================================================
    // Integration with Other Systems
    // ========================================================================

    /**
     * Set reference to diplomacy system for cross-system queries
     */
    void SetDiplomacySystem(DiplomacySystem* diplomacy_system);

    /**
     * Get diplomatic state between two realms
     * Helper function that queries diplomacy system
     */
    const DiplomaticState* GetDiplomaticState(
        types::EntityID realm1,
        types::EntityID realm2);

    /**
     * Notify diplomacy system of influence changes
     * E.g., when autonomy drops below threshold, may affect relations
     */
    void NotifyInfluenceChange(types::EntityID realm_id);

    // ========================================================================
    // Data Access
    // ========================================================================

    /**
     * Register an influence component
     */
    void RegisterInfluenceComponent(types::EntityID realm_id, InfluenceComponent* component);

    /**
     * Unregister an influence component
     */
    void UnregisterInfluenceComponent(types::EntityID realm_id);

    /**
     * Get all active influence components
     */
    const std::unordered_map<types::EntityID, InfluenceComponent*>& GetAllInfluenceComponents() const;

private:
    // ========================================================================
    // Internal Helper Functions
    // ========================================================================

    /**
     * Get realm component (helper function)
     */
    const realm::RealmComponent* GetRealmComponent(types::EntityID realm_id);

    /**
     * Get dynasty component (helper function)
     */
    const realm::DynastyComponent* GetDynastyComponent(types::EntityID dynasty_id);

    /**
     * Build adjacency graph for realm network
     * Includes neighbors, vassals, allies
     */
    void BuildRealmNetwork();

    /**
     * Get adjacent realms (neighbors, vassals, overlord, allies)
     * Includes propagation blocking logic (closed borders, hostility)
     */
    std::vector<types::EntityID> GetAdjacentRealms(types::EntityID realm_id);

    /**
     * Check if influence can propagate from source through intermediate to reach target
     * Blocks propagation based on: closed borders, at war, extreme hostility
     */
    bool CanInfluencePropagate(
        types::EntityID source,
        types::EntityID intermediate,
        types::EntityID target);

    /**
     * Get allies from diplomacy system for a realm
     */
    std::vector<types::EntityID> GetAllies(types::EntityID realm_id);

    /**
     * Calculate hop distance between realms
     */
    int CalculateHopDistance(types::EntityID source, types::EntityID target);

    /**
     * Update autonomy and diplomatic freedom for all influenced realms
     */
    void UpdateAutonomyAndFreedom();

    /**
     * Process monthly decay of influences
     */
    void ProcessInfluenceDecay();

    // ========================================================================
    // Member Variables
    // ========================================================================

    // Component storage
    std::unordered_map<types::EntityID, InfluenceComponent*> m_influence_components;

    // Realm network graph (adjacency list)
    std::unordered_map<types::EntityID, std::vector<types::EntityID>> m_realm_network;

    // Active sphere conflicts
    std::vector<InfluenceConflict> m_active_conflicts;

    // Reference to other systems
    DiplomacySystem* m_diplomacy_system = nullptr;

    // Temporary storage for realm components (cache)
    std::unordered_map<types::EntityID, const realm::RealmComponent*> m_realm_cache;
    std::unordered_map<types::EntityID, const realm::DynastyComponent*> m_dynasty_cache;

    // Update tracking
    int m_current_month = 0;
    bool m_initialized = false;

    // Constants
    static constexpr int MAX_INFLUENCE_HOPS = 10;
    static constexpr double MIN_INFLUENCE_THRESHOLD = 5.0;
    static constexpr double INFLUENCE_DECAY_RATE = 0.02;  // 2% per month
};

} // namespace diplomacy
} // namespace game
