// ============================================================================
// MapSystem.cpp - Main Map System Implementation
// Mechanica Imperii
// ============================================================================

#include "map/MapSystem.h"
#include "map/GeographicUtils.h"
#include <algorithm>

namespace game {
    namespace map {

        MapSystem::MapSystem(core::ComponentAccessManager& access_manager,
                           core::MessageBus& message_bus)
            : m_access_manager(access_manager)
            , m_message_bus(message_bus)
            , m_initialized(false)
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
        }

        void MapSystem::Update(float delta_time) {
            // Stub: Update map state if needed
        }

        void MapSystem::Shutdown() {
            m_provinces.clear();
            m_province_index_map.clear();
            m_initialized = false;
        }

        core::ThreadingStrategy MapSystem::GetThreadingStrategy() const {
            return core::ThreadingStrategy::MAIN_THREAD;
        }

        std::string MapSystem::GetSystemName() const {
            return "MapSystem";
        }

        Json::Value MapSystem::Serialize(int version) const {
            Json::Value root;
            root["system_name"] = GetSystemName();
            root["version"] = version;
            root["province_count"] = static_cast<Json::Value::UInt>(m_provinces.size());
            return root;
        }

        bool MapSystem::Deserialize(const Json::Value& data, int version) {
            // Stub: Deserialize map data
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
            Coordinate point(x, y);
            for (const auto& province : m_provinces) {
                if (GeoUtils::PointInPolygon(point, province.boundary)) {
                    return province.id;
                }
            }
            return 0;
        }

        std::vector<uint32_t> MapSystem::GetProvincesInBounds(const BoundingBox& bounds) const {
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
                return province->neighbors;
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
            m_provinces.push_back(province);
            BuildProvinceIndexMap();
            CalculateMapBounds();
            return true;
        }

        bool MapSystem::RemoveProvince(uint32_t province_id) {
            auto it = m_province_index_map.find(province_id);
            if (it != m_province_index_map.end()) {
                m_provinces.erase(m_provinces.begin() + it->second);
                BuildProvinceIndexMap();
                CalculateMapBounds();
                return true;
            }
            return false;
        }

        bool MapSystem::UpdateProvince(const ProvinceData& province) {
            auto it = m_province_index_map.find(province.id);
            if (it != m_province_index_map.end()) {
                m_provinces[it->second] = province;
                return true;
            }
            return false;
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

    } // namespace map
} // namespace game
