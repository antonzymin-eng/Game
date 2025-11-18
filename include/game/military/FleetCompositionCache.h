// ============================================================================
// FleetCompositionCache.h - Performance Caching for Fleet Operations
// Created: 2025-11-18 - Fleet Performance Optimization
// Location: include/game/military/FleetCompositionCache.h
// ============================================================================

#pragma once

#include "game/military/FleetManagementSystem.h"
#include "core/types/game_types.h"
#include <unordered_map>
#include <list>

namespace game::military {

    /// LRU Cache for fleet composition analysis
    class FleetCompositionCache {
    public:
        explicit FleetCompositionCache(size_t max_size = 100)
            : max_size_(max_size) {}

        /// Get cached composition or compute and cache it
        FleetComposition GetOrCompute(const ArmyComponent& fleet);

        /// Clear the cache
        void Clear();

        /// Get cache statistics
        size_t GetSize() const { return cache_.size(); }
        size_t GetMaxSize() const { return max_size_; }
        double GetHitRate() const;

    private:
        struct CacheEntry {
            game::types::EntityID fleet_id;
            FleetComposition composition;
            uint32_t fleet_hash;  // Hash of fleet composition for invalidation
        };

        size_t max_size_;
        std::unordered_map<game::types::EntityID, std::list<CacheEntry>::iterator> cache_;
        std::list<CacheEntry> lru_list_;

        uint32_t hits_ = 0;
        uint32_t misses_ = 0;

        uint32_t ComputeFleetHash(const ArmyComponent& fleet) const;
        void EvictLRU();
    };

    /// Global fleet composition cache
    extern FleetCompositionCache g_fleet_composition_cache;

} // namespace game::military
