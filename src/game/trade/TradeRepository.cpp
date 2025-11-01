// ============================================================================
// Mechanica Imperii - Trade Repository Implementation
// Component Access Layer for Trade System
// ============================================================================

#include "game/trade/TradeRepository.h"

namespace game::trade {

    TradeRepository::TradeRepository(core::ecs::ComponentAccessManager& access_manager)
        : m_access_manager(access_manager) {
    }

    // ========================================================================
    // TradeRouteComponent Access
    // ========================================================================

    std::shared_ptr<TradeRouteComponent> TradeRepository::GetRouteComponent(types::EntityID province_id) {
        return m_access_manager.GetComponent<TradeRouteComponent>(province_id);
    }

    std::shared_ptr<const TradeRouteComponent> TradeRepository::GetRouteComponent(types::EntityID province_id) const {
        return m_access_manager.GetComponent<TradeRouteComponent>(province_id);
    }

    std::shared_ptr<TradeRouteComponent> TradeRepository::GetOrCreateRouteComponent(types::EntityID province_id) {
        auto component = GetRouteComponent(province_id);
        if (!component) {
            component = CreateRouteComponent(province_id);
        }
        return component;
    }

    bool TradeRepository::HasRouteComponent(types::EntityID province_id) const {
        return m_access_manager.HasComponent<TradeRouteComponent>(province_id);
    }

    // ========================================================================
    // TradeHubComponent Access
    // ========================================================================

    std::shared_ptr<TradeHubComponent> TradeRepository::GetHubComponent(types::EntityID province_id) {
        return m_access_manager.GetComponent<TradeHubComponent>(province_id);
    }

    std::shared_ptr<const TradeHubComponent> TradeRepository::GetHubComponent(types::EntityID province_id) const {
        return m_access_manager.GetComponent<TradeHubComponent>(province_id);
    }

    std::shared_ptr<TradeHubComponent> TradeRepository::GetOrCreateHubComponent(types::EntityID province_id) {
        auto component = GetHubComponent(province_id);
        if (!component) {
            // Create with default hub data
            TradeHub default_hub(province_id, "Province_" + std::to_string(province_id));
            component = CreateHubComponent(province_id, default_hub);
        }
        return component;
    }

    bool TradeRepository::HasHubComponent(types::EntityID province_id) const {
        return m_access_manager.HasComponent<TradeHubComponent>(province_id);
    }

    // ========================================================================
    // TradeInventoryComponent Access
    // ========================================================================

    std::shared_ptr<TradeInventoryComponent> TradeRepository::GetInventoryComponent(types::EntityID province_id) {
        return m_access_manager.GetComponent<TradeInventoryComponent>(province_id);
    }

    std::shared_ptr<const TradeInventoryComponent> TradeRepository::GetInventoryComponent(types::EntityID province_id) const {
        return m_access_manager.GetComponent<TradeInventoryComponent>(province_id);
    }

    std::shared_ptr<TradeInventoryComponent> TradeRepository::GetOrCreateInventoryComponent(types::EntityID province_id) {
        auto component = GetInventoryComponent(province_id);
        if (!component) {
            component = CreateInventoryComponent(province_id);
        }
        return component;
    }

    bool TradeRepository::HasInventoryComponent(types::EntityID province_id) const {
        return m_access_manager.HasComponent<TradeInventoryComponent>(province_id);
    }

    // ========================================================================
    // Bulk Operations
    // ========================================================================

    void TradeRepository::EnsureAllTradeComponents(types::EntityID province_id) {
        if (!HasRouteComponent(province_id)) {
            CreateRouteComponent(province_id);
        }
        if (!HasHubComponent(province_id)) {
            TradeHub default_hub(province_id, "Province_" + std::to_string(province_id));
            CreateHubComponent(province_id, default_hub);
        }
        if (!HasInventoryComponent(province_id)) {
            CreateInventoryComponent(province_id);
        }
    }

    std::vector<types::EntityID> TradeRepository::GetAllTradeProvinces() const {
        auto ecs_entities = m_access_manager.GetEntityManager()->GetEntitiesWithComponent<TradeRouteComponent>();
        std::vector<types::EntityID> result;
        result.reserve(ecs_entities.size());
        for (const auto& entity : ecs_entities) {
            result.push_back(static_cast<types::EntityID>(entity.id));
        }
        return result;
    }

    std::vector<types::EntityID> TradeRepository::GetAllHubProvinces() const {
        auto ecs_entities = m_access_manager.GetEntityManager()->GetEntitiesWithComponent<TradeHubComponent>();
        std::vector<types::EntityID> result;
        result.reserve(ecs_entities.size());
        for (const auto& entity : ecs_entities) {
            result.push_back(static_cast<types::EntityID>(entity.id));
        }
        return result;
    }

    bool TradeRepository::HasAllTradeComponents(types::EntityID province_id) const {
        return HasRouteComponent(province_id) &&
               HasHubComponent(province_id) &&
               HasInventoryComponent(province_id);
    }

    // ========================================================================
    // Component Creation
    // ========================================================================

    std::shared_ptr<TradeRouteComponent> TradeRepository::CreateRouteComponent(types::EntityID province_id) {
        auto component = std::make_unique<TradeRouteComponent>();
        component->total_monthly_volume = 0.0;
        component->total_monthly_profit = 0.0;

        m_access_manager.AddComponent(province_id, std::move(component));
        return m_access_manager.GetComponent<TradeRouteComponent>(province_id);
    }

    std::shared_ptr<TradeHubComponent> TradeRepository::CreateHubComponent(types::EntityID province_id, const TradeHub& hub_data) {
        auto component = std::make_unique<TradeHubComponent>();
        component->hub_data = hub_data;
        component->monthly_throughput = 0.0;
        component->merchant_count = 0;

        m_access_manager.AddComponent(province_id, std::move(component));
        return m_access_manager.GetComponent<TradeHubComponent>(province_id);
    }

    std::shared_ptr<TradeInventoryComponent> TradeRepository::CreateInventoryComponent(types::EntityID province_id, double capacity) {
        auto component = std::make_unique<TradeInventoryComponent>();
        component->total_storage_capacity = capacity;
        component->current_utilization = 0.0;

        m_access_manager.AddComponent(province_id, std::move(component));
        return m_access_manager.GetComponent<TradeInventoryComponent>(province_id);
    }

    // ========================================================================
    // Component Removal
    // ========================================================================

    void TradeRepository::RemoveAllTradeComponents(types::EntityID province_id) {
        if (HasRouteComponent(province_id)) {
            m_access_manager.RemoveComponent<TradeRouteComponent>(province_id);
        }
        if (HasHubComponent(province_id)) {
            m_access_manager.RemoveComponent<TradeHubComponent>(province_id);
        }
        if (HasInventoryComponent(province_id)) {
            m_access_manager.RemoveComponent<TradeInventoryComponent>(province_id);
        }
    }

} // namespace game::trade
