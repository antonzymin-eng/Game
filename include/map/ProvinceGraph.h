// ============================================================================
// ProvinceGraph.h - Efficient Province Lookup and Navigation
// Mechanica Imperii - High-performance map graph structure
// ============================================================================

#pragma once

#include "map/MapData.h"
#include <vector>
#include <unordered_map>
#include <memory>

namespace game::map {

    /// High-performance graph structure for province navigation
    /// Provides O(1) province lookups and efficient neighbor iteration
    ///
    /// THREAD SAFETY:
    ///   - SAFE for concurrent reads (multiple threads calling const methods)
    ///   - NOT SAFE for concurrent writes (Build, Clear, or mutable GetProvince)
    ///   - NOT SAFE if any thread is writing while others are reading
    ///   - Recommended: Build once, then use immutably across threads
    ///   - If modification needed: Use external synchronization (mutex/RW lock)
    class ProvinceGraph {
    public:
        ProvinceGraph() = default;
        ~ProvinceGraph() = default;

        // Non-copyable (holds large data structures)
        ProvinceGraph(const ProvinceGraph&) = delete;
        ProvinceGraph& operator=(const ProvinceGraph&) = delete;

        // Movable
        ProvinceGraph(ProvinceGraph&&) = default;
        ProvinceGraph& operator=(ProvinceGraph&&) = default;

        // ====================================================================
        // Construction
        // ====================================================================

        /// Build graph from province list
        /// Complexity: O(n) where n = number of provinces
        void Build(const std::vector<ProvinceData>& provinces);

        /// Build graph from province list (move semantics)
        void Build(std::vector<ProvinceData>&& provinces);

        // ====================================================================
        // Queries - O(1) Lookups
        // ====================================================================

        /// Get province by ID (O(1) lookup)
        /// Returns nullptr if not found
        /// Thread-safe for concurrent reads
        const ProvinceData* GetProvince(uint32_t province_id) const;

        /// Get province by ID (mutable)
        /// Returns nullptr if not found
        /// WARNING: NOT thread-safe - can cause data races if used concurrently
        /// Only use when you have exclusive write access to the graph
        ProvinceData* GetProvince(uint32_t province_id);

        /// Check if province exists
        bool HasProvince(uint32_t province_id) const;

        /// Get all provinces
        const std::vector<ProvinceData>& GetAllProvinces() const { return provinces_; }

        /// Get province count
        size_t GetProvinceCount() const { return provinces_.size(); }

        // ====================================================================
        // Neighbor Queries
        // ====================================================================

        /// Get detailed neighbors (with border lengths) - O(1) access
        /// Returns empty span if province not found
        const std::vector<NeighborWithBorder>& GetNeighbors(uint32_t province_id) const;

        /// Check if two provinces are neighbors - O(k) where k = avg neighbors per province
        bool AreNeighbors(uint32_t province_a, uint32_t province_b) const;

        /// Get border length between two provinces
        /// Returns 0.0 if not neighbors or province not found
        double GetBorderLength(uint32_t province_a, uint32_t province_b) const;

        // ====================================================================
        // Graph Statistics
        // ====================================================================

        /// Get total number of adjacencies (bidirectional, so counted twice)
        size_t GetTotalAdjacencies() const;

        /// Get average neighbors per province
        double GetAverageNeighbors() const;

        /// Get province with most neighbors
        /// Returns 0 if graph is empty (use HasProvince(0) to distinguish from valid ID 0)
        /// Better alternative: Check IsEmpty() before calling
        uint32_t GetMostConnectedProvince() const;

        /// Validate graph integrity (all neighbor relationships are bidirectional)
        bool ValidateGraph() const;

        // ====================================================================
        // Clear
        // ====================================================================

        /// Clear all data
        void Clear();

        /// Check if graph is empty
        bool IsEmpty() const { return provinces_.empty(); }

    private:
        // Province data storage (indexed by insertion order)
        std::vector<ProvinceData> provinces_;

        // Fast O(1) lookup: province_id → index in provinces_ vector
        std::unordered_map<uint32_t, size_t> province_id_to_index_;

        // Empty neighbor list for invalid queries
        static const std::vector<NeighborWithBorder> empty_neighbors_;
    };

} // namespace game::map
