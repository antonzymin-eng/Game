// ============================================================================
// SpatialIndex.h - Spatial Indexing for Fast Queries
// Mechanica Imperii - Quadtree-based spatial indexing
// ============================================================================

#pragma once

#include "MapData.h"
#include <vector>
#include <memory>

namespace game {
    namespace map {

        // ============================================================================
        // Spatial Index - Quadtree Implementation
        // ============================================================================

        class SpatialIndex {
        public:
            SpatialIndex();
            explicit SpatialIndex(const BoundingBox& bounds, int max_depth = 8);
            ~SpatialIndex();

            // Index management
            void Clear();
            void Build(const std::vector<ProvinceData>& provinces);
            void Insert(uint32_t province_id, const BoundingBox& bounds);
            void Remove(uint32_t province_id);

            // Spatial queries
            std::vector<uint32_t> QueryPoint(double x, double y) const;
            std::vector<uint32_t> QueryRegion(const BoundingBox& bounds) const;
            std::vector<uint32_t> QueryRadius(const Coordinate& center, double radius) const;

            // Nearest neighbor
            uint32_t FindNearest(const Coordinate& point) const;
            std::vector<uint32_t> FindNNearest(const Coordinate& point, int n) const;

            // Statistics
            int GetNodeCount() const;
            int GetMaxDepth() const;
            int GetProvinceCount() const;

        private:
            struct QuadTreeNode;
            std::unique_ptr<QuadTreeNode> m_root;
            BoundingBox m_bounds;
            int m_max_depth;
            int m_node_count;

            void QueryPointRecursive(const QuadTreeNode* node, double x, double y,
                                    std::vector<uint32_t>& results) const;
            void QueryRegionRecursive(const QuadTreeNode* node, const BoundingBox& bounds,
                                     std::vector<uint32_t>& results) const;
        };

    } // namespace map
} // namespace game
