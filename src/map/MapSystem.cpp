// ============================================================================
// MapSystem.cpp - Main Map System Implementation
// Mechanica Imperii
// ============================================================================

#include "map/MapSystem.h"
#include "map/SpatialIndex.h"
#include "map/GeographicUtils.h"
#include <json/json.h>
#include <algorithm>
#include <limits>

namespace game {
    namespace map {

        MapSystem::MapSystem(::core::ecs::ComponentAccessManager& access_manager,
                           ::core::ecs::MessageBus& message_bus)
            : m_access_manager(access_manager)
            , m_message_bus(message_bus)
            , m_initialized(false)
            , m_bounds_dirty(false)
            , m_spatial_index(nullptr)
        {
        }

        MapSystem::~MapSystem() {
            if (m_initialized) {
                Shutdown();
            }
        }

        void MapSystem::Initialize() {
            m_initialized = true;
            CalculateMapBounds();

            // Initialize spatial index with map bounds
            if (!m_provinces.empty()) {
                m_spatial_index = std::make_unique<SpatialIndex>(m_map_bounds, 8);
                RebuildSpatialIndex();
            }
        }

        void MapSystem::Update(float delta_time) {
            // Lazy recalculation of bounds if dirty
            if (m_bounds_dirty) {
                CalculateMapBounds();
                RebuildSpatialIndex();
                m_bounds_dirty = false;
            }
        }

        void MapSystem::Shutdown() {
            m_provinces.clear();
            m_province_index_map.clear();
            m_spatial_index.reset();
            m_initialized = false;
            m_bounds_dirty = false;
        }

        ::core::threading::ThreadingStrategy MapSystem::GetThreadingStrategy() const {
            return ::core::threading::ThreadingStrategy::MAIN_THREAD;
        }

        std::string MapSystem::GetSystemName() const {
            return "MapSystem";
        }

        Json::Value MapSystem::Serialize(int version) const {
            Json::Value root;
            root["system_name"] = GetSystemName();
            root["version"] = version;
            root["province_count"] = static_cast<Json::Value::UInt>(m_provinces.size());

            // Serialize map bounds
            Json::Value bounds;
            bounds["min_x"] = m_map_bounds.min_x;
            bounds["min_y"] = m_map_bounds.min_y;
            bounds["max_x"] = m_map_bounds.max_x;
            bounds["max_y"] = m_map_bounds.max_y;
            root["map_bounds"] = bounds;

            // Serialize all provinces
            Json::Value provinces_array(Json::arrayValue);
            for (const auto& province : m_provinces) {
                Json::Value prov_obj;
                prov_obj["id"] = province.id;
                prov_obj["name"] = province.name;
                prov_obj["owner_id"] = province.owner_id;
                prov_obj["terrain"] = static_cast<int>(province.terrain);
                prov_obj["climate"] = static_cast<int>(province.climate);
                prov_obj["is_coastal"] = province.is_coastal;
                prov_obj["has_river"] = province.has_river;

                // Center point
                Json::Value center;
                center["x"] = province.center.x;
                center["y"] = province.center.y;
                prov_obj["center"] = center;

                // Neighbors
                Json::Value neighbors_array(Json::arrayValue);
                for (uint32_t neighbor_id : province.GetNeighborIds()) {
                    neighbors_array.append(neighbor_id);
                }
                prov_obj["neighbors"] = neighbors_array;

                provinces_array.append(prov_obj);
            }
            root["provinces"] = provinces_array;

            return root;
        }

        bool MapSystem::Deserialize(const Json::Value& data, int version) {
            if (!data.isMember("provinces") || !data["provinces"].isArray()) {
                return false;
            }

            // Clear existing data
            m_provinces.clear();
            m_province_index_map.clear();
            m_spatial_index.reset();

            // Deserialize map bounds
            if (data.isMember("map_bounds")) {
                const Json::Value& bounds = data["map_bounds"];
                m_map_bounds.min_x = bounds["min_x"].asDouble();
                m_map_bounds.min_y = bounds["min_y"].asDouble();
                m_map_bounds.max_x = bounds["max_x"].asDouble();
                m_map_bounds.max_y = bounds["max_y"].asDouble();
            }

            // Deserialize provinces
            const Json::Value& provinces_array = data["provinces"];
            m_provinces.reserve(provinces_array.size());

            for (const auto& prov_obj : provinces_array) {
                ProvinceData province;
                province.id = prov_obj["id"].asUInt();
                province.name = prov_obj["name"].asString();
                province.owner_id = prov_obj["owner_id"].asUInt();
                province.terrain = static_cast<TerrainType>(prov_obj["terrain"].asInt());
                province.climate = static_cast<ClimateZone>(prov_obj["climate"].asInt());
                province.is_coastal = prov_obj["is_coastal"].asBool();
                province.has_river = prov_obj["has_river"].asBool();

                // Center point
                if (prov_obj.isMember("center")) {
                    province.center.x = prov_obj["center"]["x"].asDouble();
                    province.center.y = prov_obj["center"]["y"].asDouble();
                }

                // Neighbors
                if (prov_obj.isMember("neighbors")) {
                    const Json::Value& neighbors_array = prov_obj["neighbors"];
                    province.detailed_neighbors.reserve(neighbors_array.size());
                    for (const auto& neighbor_id : neighbors_array) {
                        province.detailed_neighbors.emplace_back(neighbor_id.asUInt(), 0.0);
                    }
                }

                m_provinces.push_back(std::move(province));
            }

            // Rebuild index and spatial index
            BuildProvinceIndexMap();
            if (!m_provinces.empty() && m_initialized) {
                m_spatial_index = std::make_unique<SpatialIndex>(m_map_bounds, 8);
                RebuildSpatialIndex();
            }

            return true;
        }

        const ProvinceData* MapSystem::GetProvince(uint32_t province_id) const {
            auto it = m_province_index_map.find(province_id);
            if (it != m_province_index_map.end()) {
                return &m_provinces[it->second];
            }
            return nullptr;
        }

        ProvinceData* MapSystem::GetProvinceMutable(uint32_t province_id) {
            auto it = m_province_index_map.find(province_id);
            if (it != m_province_index_map.end()) {
                return &m_provinces[it->second];
            }
            return nullptr;
        }

        uint32_t MapSystem::GetProvinceAtPosition(double x, double y) const {
            // Use spatial index for O(log n) query if available
            if (m_spatial_index) {
                auto candidates = m_spatial_index->QueryPoint(x, y);

                // Test point-in-polygon only for candidate provinces
                Coordinate point(x, y);
                for (uint32_t candidate_id : candidates) {
                    const ProvinceData* province = GetProvince(candidate_id);
                    if (province && GeoUtils::PointInPolygon(point, province->boundary)) {
                        return province->id;
                    }
                }
                return 0;
            }

            // Fallback to linear search if spatial index not available
            Coordinate point(x, y);
            for (const auto& province : m_provinces) {
                if (GeoUtils::PointInPolygon(point, province.boundary)) {
                    return province.id;
                }
            }
            return 0;
        }

        std::vector<uint32_t> MapSystem::GetProvincesInBounds(const BoundingBox& bounds) const {
            // Use spatial index for O(log n + k) query if available
            if (m_spatial_index) {
                return m_spatial_index->QueryRegion(bounds);
            }

            // Fallback to linear search if spatial index not available
            std::vector<uint32_t> result;
            for (const auto& province : m_provinces) {
                if (province.bounds.Intersects(bounds)) {
                    result.push_back(province.id);
                }
            }
            return result;
        }

        std::vector<uint32_t> MapSystem::GetNeighborProvinces(uint32_t province_id) const {
            const ProvinceData* province = GetProvince(province_id);
            if (province) {
                return province->GetNeighborIds();
            }
            return {};
        }

        double MapSystem::GetDistanceBetweenProvinces(uint32_t province1, uint32_t province2) const {
            const ProvinceData* p1 = GetProvince(province1);
            const ProvinceData* p2 = GetProvince(province2);
            if (p1 && p2) {
                return GeoUtils::CalculateDistance(p1->center, p2->center);
            }
            return 0.0;
        }

        bool MapSystem::AddProvince(const ProvinceData& province) {
            // Check for duplicate ID
            if (m_province_index_map.find(province.id) != m_province_index_map.end()) {
                return false;  // Province with this ID already exists
            }

            // Add province to vector
            size_t index = m_provinces.size();
            m_provinces.push_back(province);

            // O(1) index update instead of rebuilding entire map
            m_province_index_map[province.id] = index;

            // O(1) incremental bounds update instead of recalculating from all provinces
            UpdateMapBoundsIncremental(province.bounds);

            // Add to spatial index for O(log n) insertion
            if (m_spatial_index) {
                m_spatial_index->Insert(province.id, province.bounds);
            }

            return true;
        }

        bool MapSystem::RemoveProvince(uint32_t province_id) {
            auto it = m_province_index_map.find(province_id);
            if (it == m_province_index_map.end()) {
                return false;  // Province not found
            }

            size_t index = it->second;

            // Use swap-and-pop for O(1) removal instead of O(n) erase
            if (index != m_provinces.size() - 1) {
                // Swap with last element
                std::swap(m_provinces[index], m_provinces.back());
                // Update index of swapped element
                m_province_index_map[m_provinces[index].id] = index;
            }

            // Remove last element
            m_provinces.pop_back();
            m_province_index_map.erase(province_id);

            // Remove from spatial index
            if (m_spatial_index) {
                m_spatial_index->Remove(province_id);
            }

            // Mark bounds as dirty for lazy recalculation
            // (Removing a province might change map bounds, but we defer calculation)
            m_bounds_dirty = true;

            return true;
        }

        bool MapSystem::UpdateProvince(const ProvinceData& province) {
            auto it = m_province_index_map.find(province.id);
            if (it == m_province_index_map.end()) {
                return false;  // Province not found
            }

            size_t index = it->second;
            const ProvinceData& old_province = m_provinces[index];

            // Update province data
            m_provinces[index] = province;

            // Update spatial index if bounds changed
            if (m_spatial_index && old_province.bounds != province.bounds) {
                m_spatial_index->Remove(province.id);
                m_spatial_index->Insert(province.id, province.bounds);
            }

            // Update map bounds if province bounds changed
            if (old_province.bounds != province.bounds) {
                m_bounds_dirty = true;
            }

            return true;
        }

        void MapSystem::BuildProvinceIndexMap() {
            m_province_index_map.clear();
            for (size_t i = 0; i < m_provinces.size(); ++i) {
                m_province_index_map[m_provinces[i].id] = i;
            }
        }

        void MapSystem::CalculateMapBounds() {
            if (m_provinces.empty()) {
                m_map_bounds = BoundingBox(-500.0, -500.0, 500.0, 500.0);
                return;
            }

            std::vector<Coordinate> all_points;
            for (const auto& province : m_provinces) {
                all_points.insert(all_points.end(), province.boundary.begin(), province.boundary.end());
            }
            m_map_bounds = GeoUtils::CalculateBoundingBox(all_points);
        }

        void MapSystem::UpdateMapBoundsIncremental(const BoundingBox& province_bounds) {
            if (m_provinces.empty()) {
                m_map_bounds = province_bounds;
                return;
            }

            // Expand map bounds to include new province bounds (O(1) operation)
            m_map_bounds.min_x = std::min(m_map_bounds.min_x, province_bounds.min_x);
            m_map_bounds.min_y = std::min(m_map_bounds.min_y, province_bounds.min_y);
            m_map_bounds.max_x = std::max(m_map_bounds.max_x, province_bounds.max_x);
            m_map_bounds.max_y = std::max(m_map_bounds.max_y, province_bounds.max_y);
        }

        void MapSystem::RebuildSpatialIndex() {
            if (!m_spatial_index) {
                return;
            }

            m_spatial_index->Clear();
            m_spatial_index->Build(m_provinces);
        }

    } // namespace map
} // namespace game
