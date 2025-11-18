// ============================================================================
// FleetCompositionCache.cpp - Performance Caching Implementation
// Created: 2025-11-18 - Fleet Performance Optimization
// Location: src/game/military/FleetCompositionCache.cpp
// ============================================================================

#include "game/military/FleetCompositionCache.h"
#include "game/military/NavalCombatLogger.h"
#include <functional>

namespace game::military {

    // Global cache instance
    FleetCompositionCache g_fleet_composition_cache(100);

    FleetComposition FleetCompositionCache::GetOrCompute(const ArmyComponent& fleet) {
        game::types::EntityID fleet_id = fleet.commander_id;  // Use commander_id as fleet ID
        uint32_t current_hash = ComputeFleetHash(fleet);

        // Check if we have a cached entry
        auto it = cache_.find(fleet_id);
        if (it != cache_.end()) {
            // Verify the hash to ensure fleet hasn't changed
            if (it->second->fleet_hash == current_hash) {
                // Cache hit - move to front of LRU list
                hits_++;
                NavalCombatLogger::RecordCacheAccess(true, false);

                lru_list_.splice(lru_list_.begin(), lru_list_, it->second);
                return it->second->composition;
            } else {
                // Fleet composition changed - remove stale entry
                lru_list_.erase(it->second);
                cache_.erase(it);
            }
        }

        // Cache miss - compute composition
        misses_++;
        NavalCombatLogger::RecordCacheAccess(false, false);

        FleetComposition composition = FleetManagementSystem::AnalyzeFleetComposition(fleet);

        // Add to cache
        if (cache_.size() >= max_size_) {
            EvictLRU();
        }

        CacheEntry entry{fleet_id, composition, current_hash};
        lru_list_.push_front(entry);
        cache_[fleet_id] = lru_list_.begin();

        return composition;
    }

    void FleetCompositionCache::Clear() {
        cache_.clear();
        lru_list_.clear();
        hits_ = 0;
        misses_ = 0;
    }

    double FleetCompositionCache::GetHitRate() const {
        uint32_t total = hits_ + misses_;
        if (total == 0) return 0.0;
        return static_cast<double>(hits_) / total;
    }

    uint32_t FleetCompositionCache::ComputeFleetHash(const ArmyComponent& fleet) const {
        // Simple hash based on number of ships of each type
        uint32_t hash = 0;

        for (const auto& unit : fleet.units) {
            // Combine unit type and quantity into hash
            hash = hash * 31 + static_cast<uint32_t>(unit.type);
            hash = hash * 31 + unit.current_strength;
        }

        return hash;
    }

    void FleetCompositionCache::EvictLRU() {
        if (lru_list_.empty()) return;

        // Remove least recently used entry
        auto& lru_entry = lru_list_.back();
        cache_.erase(lru_entry.fleet_id);
        lru_list_.pop_back();
    }

} // namespace game::military
