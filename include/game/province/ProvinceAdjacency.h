// ============================================================================
// ProvinceAdjacency.h - Province Adjacency and Border Management
// Created: November 15, 2025 - Phase 3: Influence System Integration
// Location: include/game/province/ProvinceAdjacency.h
// ============================================================================

#pragma once

#include "core/ECS/IComponent.h"
#include "core/types/game_types.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace game {
namespace province {

// ============================================================================
// Border Type Definitions
// ============================================================================

enum class BorderType : uint8_t {
    LAND,              // Land border
    RIVER,             // Border across a river
    MOUNTAIN,          // Border across mountains
    SEA,               // Sea border (coastal provinces)
    STRAIT,            // Strait connection
    COUNT
};

struct ProvinceConnection {
    types::EntityID connected_province{0};
    BorderType border_type = BorderType::LAND;
    double border_length = 0.0;  // Length of shared border
    bool is_passable = true;      // Can armies/influence cross?

    ProvinceConnection() = default;
    ProvinceConnection(types::EntityID province, BorderType type = BorderType::LAND)
        : connected_province(province)
        , border_type(type)
    {}
};

// ============================================================================
// Province Adjacency Component (ECS)
// ============================================================================

class ProvinceAdjacencyComponent : public ::core::ecs::Component<ProvinceAdjacencyComponent> {
public:
    types::EntityID province_id{0};

    // Direct neighbors
    std::vector<ProvinceConnection> adjacent_provinces;

    // Cached realm neighbors (updated when province ownership changes)
    std::unordered_set<types::EntityID> neighboring_realms;

    ProvinceAdjacencyComponent() = default;
    explicit ProvinceAdjacencyComponent(types::EntityID prov_id)
        : province_id(prov_id)
    {}

    // ========================================================================
    // Adjacency Management
    // ========================================================================

    /**
     * Add an adjacent province
     */
    void AddAdjacentProvince(types::EntityID adjacent_prov,
                            BorderType border = BorderType::LAND,
                            double border_len = 1.0) {
        // Check if already exists
        for (const auto& conn : adjacent_provinces) {
            if (conn.connected_province == adjacent_prov) {
                return;  // Already added
            }
        }

        ProvinceConnection conn(adjacent_prov, border);
        conn.border_length = border_len;
        adjacent_provinces.push_back(conn);
    }

    /**
     * Check if a province is adjacent
     */
    bool IsAdjacentTo(types::EntityID other_province) const {
        for (const auto& conn : adjacent_provinces) {
            if (conn.connected_province == other_province) {
                return true;
            }
        }
        return false;
    }

    /**
     * Get all adjacent province IDs
     */
    std::vector<types::EntityID> GetAdjacentProvinces() const {
        std::vector<types::EntityID> result;
        result.reserve(adjacent_provinces.size());
        for (const auto& conn : adjacent_provinces) {
            result.push_back(conn.connected_province);
        }
        return result;
    }

    /**
     * Get all passable adjacent provinces
     */
    std::vector<types::EntityID> GetPassableAdjacentProvinces() const {
        std::vector<types::EntityID> result;
        for (const auto& conn : adjacent_provinces) {
            if (conn.is_passable) {
                result.push_back(conn.connected_province);
            }
        }
        return result;
    }

    /**
     * Get border type with another province
     */
    BorderType GetBorderType(types::EntityID other_province) const {
        for (const auto& conn : adjacent_provinces) {
            if (conn.connected_province == other_province) {
                return conn.border_type;
            }
        }
        return BorderType::LAND;  // Default
    }

    /**
     * Set whether border with another province is passable
     */
    void SetBorderPassable(types::EntityID other_province, bool passable) {
        for (auto& conn : adjacent_provinces) {
            if (conn.connected_province == other_province) {
                conn.is_passable = passable;
                return;
            }
        }
    }

    /**
     * Add a neighboring realm (cached for performance)
     */
    void AddNeighboringRealm(types::EntityID realm_id) {
        neighboring_realms.insert(realm_id);
    }

    /**
     * Remove a neighboring realm
     */
    void RemoveNeighboringRealm(types::EntityID realm_id) {
        neighboring_realms.erase(realm_id);
    }

    /**
     * Get all neighboring realms
     */
    const std::unordered_set<types::EntityID>& GetNeighboringRealms() const {
        return neighboring_realms;
    }

    /**
     * Clear neighboring realm cache (should recalculate)
     */
    void ClearNeighboringRealms() {
        neighboring_realms.clear();
    }
};

// ============================================================================
// Province Adjacency Manager
// ============================================================================

/**
 * System-level manager for province adjacency
 * Handles building and maintaining the adjacency graph
 */
class ProvinceAdjacencyManager {
private:
    // Province ID -> Adjacency Component
    std::unordered_map<types::EntityID, ProvinceAdjacencyComponent> m_adjacencies;

    // Province ID -> Realm ID (ownership cache)
    std::unordered_map<types::EntityID, types::EntityID> m_province_owners;

public:
    /**
     * Register a province in the adjacency system
     */
    void RegisterProvince(types::EntityID province_id) {
        if (m_adjacencies.find(province_id) == m_adjacencies.end()) {
            m_adjacencies[province_id] = ProvinceAdjacencyComponent(province_id);
        }
    }

    /**
     * Add bidirectional adjacency between two provinces
     */
    void AddAdjacency(types::EntityID province1, types::EntityID province2,
                     BorderType border = BorderType::LAND, double border_length = 1.0) {
        RegisterProvince(province1);
        RegisterProvince(province2);

        m_adjacencies[province1].AddAdjacentProvince(province2, border, border_length);
        m_adjacencies[province2].AddAdjacentProvince(province1, border, border_length);
    }

    /**
     * Get adjacency component for a province
     */
    ProvinceAdjacencyComponent* GetAdjacency(types::EntityID province_id) {
        auto it = m_adjacencies.find(province_id);
        return (it != m_adjacencies.end()) ? &it->second : nullptr;
    }

    /**
     * Get adjacency component (const)
     */
    const ProvinceAdjacencyComponent* GetAdjacency(types::EntityID province_id) const {
        auto it = m_adjacencies.find(province_id);
        return (it != m_adjacencies.end()) ? &it->second : nullptr;
    }

    /**
     * Update province ownership (used to recalculate realm neighbors)
     */
    void UpdateProvinceOwnership(types::EntityID province_id, types::EntityID new_owner) {
        m_province_owners[province_id] = new_owner;
        RebuildRealmNeighbors(province_id);
    }

    /**
     * Get realms that border a specific province
     */
    std::vector<types::EntityID> GetBorderingRealms(types::EntityID province_id) const {
        auto adj = GetAdjacency(province_id);
        if (!adj) return {};

        std::unordered_set<types::EntityID> realms;
        for (types::EntityID adj_prov : adj->GetAdjacentProvinces()) {
            auto owner_it = m_province_owners.find(adj_prov);
            if (owner_it != m_province_owners.end() && owner_it->second != 0) {
                realms.insert(owner_it->second);
            }
        }

        return std::vector<types::EntityID>(realms.begin(), realms.end());
    }

    /**
     * Check if two realms share a border
     */
    bool RealmsShareBorder(types::EntityID realm1, types::EntityID realm2) const {
        // Find all provinces owned by realm1
        for (const auto& [province_id, owner] : m_province_owners) {
            if (owner != realm1) continue;

            // Check if any adjacent province is owned by realm2
            auto adj = GetAdjacency(province_id);
            if (!adj) continue;

            for (types::EntityID adj_prov : adj->GetAdjacentProvinces()) {
                auto adj_owner_it = m_province_owners.find(adj_prov);
                if (adj_owner_it != m_province_owners.end() && adj_owner_it->second == realm2) {
                    return true;
                }
            }
        }

        return false;
    }

    /**
     * Get all realms that border a specific realm
     */
    std::vector<types::EntityID> GetNeighboringRealms(types::EntityID realm_id) const {
        std::unordered_set<types::EntityID> neighbors;

        // Find all provinces owned by this realm
        for (const auto& [province_id, owner] : m_province_owners) {
            if (owner != realm_id) continue;

            // Get bordering realms for this province
            auto bordering = GetBorderingRealms(province_id);
            for (types::EntityID neighbor : bordering) {
                if (neighbor != realm_id) {
                    neighbors.insert(neighbor);
                }
            }
        }

        return std::vector<types::EntityID>(neighbors.begin(), neighbors.end());
    }

private:
    /**
     * Rebuild realm neighbor cache for a province
     */
    void RebuildRealmNeighbors(types::EntityID province_id) {
        auto adj = GetAdjacency(province_id);
        if (!adj) return;

        adj->ClearNeighboringRealms();

        for (types::EntityID adj_prov : adj->GetAdjacentProvinces()) {
            auto owner_it = m_province_owners.find(adj_prov);
            if (owner_it != m_province_owners.end() && owner_it->second != 0) {
                adj->AddNeighboringRealm(owner_it->second);
            }
        }
    }
};

} // namespace province
} // namespace game
