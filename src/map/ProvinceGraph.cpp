// ============================================================================
// ProvinceGraph.cpp - Efficient Province Lookup and Navigation
// Mechanica Imperii
// ============================================================================

#include "map/ProvinceGraph.h"
#include "core/logging/Logger.h"
#include <algorithm>
#include <unordered_set>

namespace game::map {

    // Static empty neighbors list for invalid queries
    const std::vector<NeighborWithBorder> ProvinceGraph::empty_neighbors_;

    // ========================================================================
    // Construction
    // ========================================================================

    void ProvinceGraph::Build(const std::vector<ProvinceData>& provinces) {
        Clear();

        provinces_.reserve(provinces.size());
        province_id_to_index_.reserve(provinces.size());

        for (size_t i = 0; i < provinces.size(); ++i) {
            const auto& province = provinces[i];

            // Check for duplicate IDs
            if (province_id_to_index_.count(province.id) > 0) {
                CORE_STREAM_ERROR("ProvinceGraph")
                    << "Duplicate province ID detected: " << province.id
                    << " (name: " << province.name << ")";
                continue;
            }

            // BUGFIX: Use actual index in provinces_ vector, not input index
            size_t actual_index = provinces_.size();
            provinces_.push_back(province);
            province_id_to_index_[province.id] = actual_index;
        }

        CORE_STREAM_INFO("ProvinceGraph")
            << "Built graph with " << provinces_.size() << " provinces";
    }

    void ProvinceGraph::Build(std::vector<ProvinceData>&& provinces) {
        Clear();

        // BUGFIX: Can't handle duplicates efficiently with move semantics
        // because we'd need to compact the vector. Instead, validate first.
        std::unordered_set<uint32_t> seen_ids;
        size_t duplicates = 0;

        for (const auto& province : provinces) {
            if (!seen_ids.insert(province.id).second) {
                CORE_STREAM_ERROR("ProvinceGraph")
                    << "Duplicate province ID detected: " << province.id
                    << " (name: " << province.name << ")";
                ++duplicates;
            }
        }

        if (duplicates > 0) {
            CORE_STREAM_ERROR("ProvinceGraph")
                << "Found " << duplicates << " duplicate province IDs. "
                << "Cannot use move semantics with duplicates. Falling back to copy.";

            // Remove duplicates and rebuild
            std::vector<ProvinceData> cleaned;
            cleaned.reserve(provinces.size() - duplicates);
            seen_ids.clear();

            for (auto& province : provinces) {
                if (seen_ids.insert(province.id).second) {
                    cleaned.push_back(std::move(province));
                }
            }

            provinces_ = std::move(cleaned);
        } else {
            // No duplicates, can move safely
            provinces_ = std::move(provinces);
        }

        // Build index map
        province_id_to_index_.reserve(provinces_.size());
        for (size_t i = 0; i < provinces_.size(); ++i) {
            province_id_to_index_[provinces_[i].id] = i;
        }

        CORE_STREAM_INFO("ProvinceGraph")
            << "Built graph with " << provinces_.size() << " provinces (moved)";
    }

    // ========================================================================
    // Queries
    // ========================================================================

    const ProvinceData* ProvinceGraph::GetProvince(uint32_t province_id) const {
        auto it = province_id_to_index_.find(province_id);
        if (it == province_id_to_index_.end()) {
            return nullptr;
        }
        return &provinces_[it->second];
    }

    ProvinceData* ProvinceGraph::GetProvince(uint32_t province_id) {
        auto it = province_id_to_index_.find(province_id);
        if (it == province_id_to_index_.end()) {
            return nullptr;
        }
        return &provinces_[it->second];
    }

    bool ProvinceGraph::HasProvince(uint32_t province_id) const {
        return province_id_to_index_.count(province_id) > 0;
    }

    // ========================================================================
    // Neighbor Queries
    // ========================================================================

    const std::vector<NeighborWithBorder>& ProvinceGraph::GetNeighbors(uint32_t province_id) const {
        const ProvinceData* province = GetProvince(province_id);
        if (!province) {
            return empty_neighbors_;
        }
        return province->detailed_neighbors;
    }

    bool ProvinceGraph::AreNeighbors(uint32_t province_a, uint32_t province_b) const {
        const ProvinceData* prov_a = GetProvince(province_a);
        if (!prov_a) {
            return false;
        }

        // Check if province_b is in province_a's neighbor list
        for (const auto& neighbor : prov_a->detailed_neighbors) {
            if (neighbor.neighbor_id == province_b) {
                return true;
            }
        }

        return false;
    }

    double ProvinceGraph::GetBorderLength(uint32_t province_a, uint32_t province_b) const {
        const ProvinceData* prov_a = GetProvince(province_a);
        if (!prov_a) {
            return 0.0;
        }

        // Find border length in neighbor list
        for (const auto& neighbor : prov_a->detailed_neighbors) {
            if (neighbor.neighbor_id == province_b) {
                return neighbor.border_length;
            }
        }

        return 0.0;
    }

    // ========================================================================
    // Graph Statistics
    // ========================================================================

    size_t ProvinceGraph::GetTotalAdjacencies() const {
        size_t total = 0;
        for (const auto& province : provinces_) {
            total += province.detailed_neighbors.size();
        }
        return total;
    }

    double ProvinceGraph::GetAverageNeighbors() const {
        if (provinces_.empty()) {
            return 0.0;
        }
        return static_cast<double>(GetTotalAdjacencies()) / provinces_.size();
    }

    uint32_t ProvinceGraph::GetMostConnectedProvince() const {
        if (provinces_.empty()) {
            return 0;
        }

        const ProvinceData* most_connected = &provinces_[0];
        size_t max_neighbors = provinces_[0].detailed_neighbors.size();

        for (const auto& province : provinces_) {
            if (province.detailed_neighbors.size() > max_neighbors) {
                max_neighbors = province.detailed_neighbors.size();
                most_connected = &province;
            }
        }

        return most_connected->id;
    }

    bool ProvinceGraph::ValidateGraph() const {
        bool valid = true;

        for (const auto& province : provinces_) {
            for (const auto& neighbor_data : province.detailed_neighbors) {
                // Check neighbor exists
                const ProvinceData* neighbor = GetProvince(neighbor_data.neighbor_id);
                if (!neighbor) {
                    CORE_STREAM_ERROR("ProvinceGraph")
                        << "Invalid neighbor: Province " << province.id
                        << " references non-existent neighbor " << neighbor_data.neighbor_id;
                    valid = false;
                    continue;
                }

                // OPTIMIZATION: Build hash map of neighbor's neighbors for O(1) lookup
                // instead of O(k) linear search per neighbor check.
                // NOTE: Still O(n×k²) overall (builds k maps per province), but hash
                // lookups are faster in practice than linear searches. For true O(n×k),
                // would need to build all neighbor sets once before validation.
                std::unordered_map<uint32_t, double> neighbor_map;
                neighbor_map.reserve(neighbor->detailed_neighbors.size());
                for (const auto& n : neighbor->detailed_neighbors) {
                    neighbor_map[n.neighbor_id] = n.border_length;
                }

                // Check bidirectional relationship - O(1) instead of O(k)
                auto reverse_it = neighbor_map.find(province.id);
                if (reverse_it == neighbor_map.end()) {
                    CORE_STREAM_ERROR("ProvinceGraph")
                        << "Non-bidirectional adjacency: " << province.id
                        << " -> " << neighbor_data.neighbor_id
                        << " but not reverse";
                    valid = false;
                } else {
                    // Check border lengths match (within epsilon)
                    double diff = std::abs(reverse_it->second - neighbor_data.border_length);
                    if (diff > 0.01) {
                        CORE_STREAM_WARN("ProvinceGraph")
                            << "Border length mismatch: " << province.id
                            << " <-> " << neighbor_data.neighbor_id
                            << " (" << neighbor_data.border_length << " vs "
                            << reverse_it->second << ")";
                    }
                }
            }
        }

        return valid;
    }

    // ========================================================================
    // Clear
    // ========================================================================

    void ProvinceGraph::Clear() {
        provinces_.clear();
        province_id_to_index_.clear();
    }

} // namespace game::map
