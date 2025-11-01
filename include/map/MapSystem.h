// ============================================================================
// MapSystem.h - Main Map Management System
// Mechanica Imperii - ECS-based map system
// ============================================================================

#pragma once

#include "MapData.h"
#include "core/ECS/ISystem.h"
#include "core/ECS/ComponentAccessManager.h"
#include "core/ECS/MessageBus.h"
#include <vector>
#include <memory>
#include <unordered_map>

namespace game {
    namespace map {

        // ============================================================================
        // MapSystem - Main ECS System for Map Management
        // ============================================================================

        class MapSystem : public core::ISystem {
        public:
            explicit MapSystem(::core::ecs::ComponentAccessManager& access_manager,
                             ::core::ecs::MessageBus& message_bus);
            virtual ~MapSystem();

            // ISystem interface
            void Initialize() override;
            void Update(float delta_time) override;
            void Shutdown() override;
            ::core::threading::ThreadingStrategy GetThreadingStrategy() const override;
            std::string GetSystemName() const override;
            Json::Value Serialize(int version) const override;
            bool Deserialize(const Json::Value& data, int version) override;

            // Map data access
            const std::vector<ProvinceData>& GetProvinces() const { return m_provinces; }
            const ProvinceData* GetProvince(uint32_t province_id) const;
            ProvinceData* GetProvinceMutable(uint32_t province_id);

            // Map queries
            uint32_t GetProvinceAtPosition(double x, double y) const;
            std::vector<uint32_t> GetProvincesInBounds(const BoundingBox& bounds) const;
            std::vector<uint32_t> GetNeighborProvinces(uint32_t province_id) const;
            double GetDistanceBetweenProvinces(uint32_t province1, uint32_t province2) const;

            // Map modifications
            bool AddProvince(const ProvinceData& province);
            bool RemoveProvince(uint32_t province_id);
            bool UpdateProvince(const ProvinceData& province);

            // Map bounds
            BoundingBox GetMapBounds() const { return m_map_bounds; }
            void SetMapBounds(const BoundingBox& bounds) { m_map_bounds = bounds; }

        private:
            ::core::ecs::ComponentAccessManager& m_access_manager;
            ::core::ecs::MessageBus& m_message_bus;

            std::vector<ProvinceData> m_provinces;
            std::unordered_map<uint32_t, size_t> m_province_index_map;
            BoundingBox m_map_bounds;
            bool m_initialized = false;

            void BuildProvinceIndexMap();
            void CalculateMapBounds();
        };

    } // namespace map
} // namespace game
