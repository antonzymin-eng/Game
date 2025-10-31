// ============================================================================
// SpatialIndex.cpp - Spatial Indexing Implementation
// Mechanica Imperii
// ============================================================================

#include "map/SpatialIndex.h"
#include "map/GeographicUtils.h"
#include <algorithm>

namespace game {
    namespace map {

        // QuadTree Node structure
        struct SpatialIndex::QuadTreeNode {
            BoundingBox bounds;
            std::vector<uint32_t> province_ids;
            std::unique_ptr<QuadTreeNode> children[4];
            int depth = 0;
            bool is_leaf = true;

            QuadTreeNode(const BoundingBox& b, int d) : bounds(b), depth(d) {}
        };

        SpatialIndex::SpatialIndex()
            : m_bounds(BoundingBox(-500.0, -500.0, 500.0, 500.0))
            , m_max_depth(8)
            , m_node_count(0)
        {
            m_root = std::make_unique<QuadTreeNode>(m_bounds, 0);
            m_node_count = 1;
        }

        SpatialIndex::SpatialIndex(const BoundingBox& bounds, int max_depth)
            : m_bounds(bounds)
            , m_max_depth(max_depth)
            , m_node_count(0)
        {
            m_root = std::make_unique<QuadTreeNode>(bounds, 0);
            m_node_count = 1;
        }

        SpatialIndex::~SpatialIndex() {
            Clear();
        }

        void SpatialIndex::Clear() {
            m_root = std::make_unique<QuadTreeNode>(m_bounds, 0);
            m_node_count = 1;
        }

        void SpatialIndex::Build(const std::vector<ProvinceData>& provinces) {
            Clear();
            for (const auto& province : provinces) {
                Insert(province.id, province.bounds);
            }
        }

        void SpatialIndex::Insert(uint32_t province_id, const BoundingBox& bounds) {
            if (m_root) {
                m_root->province_ids.push_back(province_id);
            }
        }

        void SpatialIndex::Remove(uint32_t province_id) {
            // Stub: Remove province from index
        }

        std::vector<uint32_t> SpatialIndex::QueryPoint(double x, double y) const {
            std::vector<uint32_t> results;
            if (m_root) {
                QueryPointRecursive(m_root.get(), x, y, results);
            }
            return results;
        }

        std::vector<uint32_t> SpatialIndex::QueryRegion(const BoundingBox& bounds) const {
            std::vector<uint32_t> results;
            if (m_root) {
                QueryRegionRecursive(m_root.get(), bounds, results);
            }
            return results;
        }

        std::vector<uint32_t> SpatialIndex::QueryRadius(const Coordinate& center, double radius) const {
            BoundingBox search_bounds(
                center.x - radius, center.y - radius,
                center.x + radius, center.y + radius
            );
            return QueryRegion(search_bounds);
        }

        uint32_t SpatialIndex::FindNearest(const Coordinate& point) const {
            // Stub: Find nearest province
            if (m_root && !m_root->province_ids.empty()) {
                return m_root->province_ids[0];
            }
            return 0;
        }

        std::vector<uint32_t> SpatialIndex::FindNNearest(const Coordinate& point, int n) const {
            // Stub: Find N nearest provinces
            std::vector<uint32_t> results;
            if (m_root) {
                for (size_t i = 0; i < std::min(static_cast<size_t>(n), m_root->province_ids.size()); ++i) {
                    results.push_back(m_root->province_ids[i]);
                }
            }
            return results;
        }

        int SpatialIndex::GetNodeCount() const {
            return m_node_count;
        }

        int SpatialIndex::GetMaxDepth() const {
            return m_max_depth;
        }

        int SpatialIndex::GetProvinceCount() const {
            return m_root ? static_cast<int>(m_root->province_ids.size()) : 0;
        }

        void SpatialIndex::QueryPointRecursive(const QuadTreeNode* node, double x, double y,
                                               std::vector<uint32_t>& results) const {
            if (!node || !node->bounds.Contains(x, y)) return;

            for (uint32_t id : node->province_ids) {
                results.push_back(id);
            }

            if (!node->is_leaf) {
                for (int i = 0; i < 4; ++i) {
                    if (node->children[i]) {
                        QueryPointRecursive(node->children[i].get(), x, y, results);
                    }
                }
            }
        }

        void SpatialIndex::QueryRegionRecursive(const QuadTreeNode* node, const BoundingBox& bounds,
                                                std::vector<uint32_t>& results) const {
            if (!node || !node->bounds.Intersects(bounds)) return;

            for (uint32_t id : node->province_ids) {
                results.push_back(id);
            }

            if (!node->is_leaf) {
                for (int i = 0; i < 4; ++i) {
                    if (node->children[i]) {
                        QueryRegionRecursive(node->children[i].get(), bounds, results);
                    }
                }
            }
        }

    } // namespace map
} // namespace game
