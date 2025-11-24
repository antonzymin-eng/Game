// ============================================================================
// Mechanica Imperii - Trade Repository Implementation
// Component Access Layer for Trade System
// ============================================================================

#include "game/trade/TradeRepository.h"
#include "core/logging/Logger.h"

namespace game::trade {

    TradeRepository::TradeRepository(::core::ecs::ComponentAccessManager& access_manager)
        : m_access_manager(access_manager) {
    }

    // ========================================================================
    // TradeRouteComponent Access
    // ========================================================================

    std::shared_ptr<TradeRouteComponent> TradeRepository::GetRouteComponent(types::EntityID province_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            // IMPROVEMENT (Issue #6): Add logging for silent failures
            CORE_STREAM_WARN("TradeRepository") << "GetRouteComponent: EntityManager not available for province " << province_id;
            return nullptr;
        }
        return entity_manager->GetComponent<TradeRouteComponent>(::core::ecs::EntityID{province_id});
    }

    std::shared_ptr<const TradeRouteComponent> TradeRepository::GetRouteComponent(types::EntityID province_id) const {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return nullptr;
        }
        return entity_manager->GetComponent<TradeRouteComponent>(::core::ecs::EntityID{province_id});
    }

    std::shared_ptr<TradeRouteComponent> TradeRepository::GetOrCreateRouteComponent(types::EntityID province_id) {
        auto component = GetRouteComponent(province_id);
        if (!component) {
            component = CreateRouteComponent(province_id);
        }
        return component;
    }

    bool TradeRepository::HasRouteComponent(types::EntityID province_id) const {
        auto component = GetRouteComponent(province_id);
        return component != nullptr;
    }

    // ========================================================================
    // TradeHubComponent Access
    // ========================================================================

    std::shared_ptr<TradeHubComponent> TradeRepository::GetHubComponent(types::EntityID province_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            // IMPROVEMENT (Issue #6): Add logging for silent failures
            CORE_STREAM_WARN("TradeRepository") << "GetHubComponent: EntityManager not available for province " << province_id;
            return nullptr;
        }
        return entity_manager->GetComponent<TradeHubComponent>(::core::ecs::EntityID{province_id});
    }

    std::shared_ptr<const TradeHubComponent> TradeRepository::GetHubComponent(types::EntityID province_id) const {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return nullptr;
        }
        return entity_manager->GetComponent<TradeHubComponent>(::core::ecs::EntityID{province_id});
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
        auto component = GetHubComponent(province_id);
        return component != nullptr;
    }

    // ========================================================================
    // TradeInventoryComponent Access
    // ========================================================================

    std::shared_ptr<TradeInventoryComponent> TradeRepository::GetInventoryComponent(types::EntityID province_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            // IMPROVEMENT (Issue #6): Add logging for silent failures
            CORE_STREAM_WARN("TradeRepository") << "GetInventoryComponent: EntityManager not available for province " << province_id;
            return nullptr;
        }
        return entity_manager->GetComponent<TradeInventoryComponent>(::core::ecs::EntityID{province_id});
    }

    std::shared_ptr<const TradeInventoryComponent> TradeRepository::GetInventoryComponent(types::EntityID province_id) const {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return nullptr;
        }
        return entity_manager->GetComponent<TradeInventoryComponent>(::core::ecs::EntityID{province_id});
    }

    std::shared_ptr<TradeInventoryComponent> TradeRepository::GetOrCreateInventoryComponent(types::EntityID province_id) {
        auto component = GetInventoryComponent(province_id);
        if (!component) {
            component = CreateInventoryComponent(province_id);
        }
        return component;
    }

    bool TradeRepository::HasInventoryComponent(types::EntityID province_id) const {
        auto component = GetInventoryComponent(province_id);
        return component != nullptr;
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
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return {};
        }
        auto ecs_entities = entity_manager->GetEntitiesWithComponent<TradeRouteComponent>();
        std::vector<types::EntityID> result;
        result.reserve(ecs_entities.size());
        for (const auto& entity : ecs_entities) {
            result.push_back(static_cast<types::EntityID>(entity.id));
        }
        return result;
    }

    std::vector<types::EntityID> TradeRepository::GetAllHubProvinces() const {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            return {};
        }
        auto ecs_entities = entity_manager->GetEntitiesWithComponent<TradeHubComponent>();
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
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return nullptr;
        
        ::core::ecs::EntityID ecs_id{province_id};
        auto component = entity_manager->AddComponent<TradeRouteComponent>(ecs_id);
        if (component) {
            component->total_monthly_volume = 0.0;
            component->total_monthly_profit = 0.0;
        }
        return component;
    }

    std::shared_ptr<TradeHubComponent> TradeRepository::CreateHubComponent(types::EntityID province_id, const TradeHub& hub_data) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return nullptr;
        
        ::core::ecs::EntityID ecs_id{province_id};
        auto component = entity_manager->AddComponent<TradeHubComponent>(ecs_id);
        if (component) {
            component->hub_data = hub_data;
            component->monthly_throughput = 0.0;
            component->merchant_count = 0;
        }
        return component;
    }

    std::shared_ptr<TradeInventoryComponent> TradeRepository::CreateInventoryComponent(types::EntityID province_id, double capacity) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return nullptr;
        
        ::core::ecs::EntityID ecs_id{province_id};
        auto component = entity_manager->AddComponent<TradeInventoryComponent>(ecs_id);
        if (component) {
            component->total_storage_capacity = capacity;
            component->current_utilization = 0.0;
        }
        return component;
    }

    // ========================================================================
    // Validation
    // ========================================================================

    bool TradeRepository::ProvinceExists(types::EntityID province_id) const {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return false;
        
        ::core::ecs::EntityID ecs_id{province_id};
        return entity_manager->IsEntityValid(ecs_id);
    }

    bool TradeRepository::ResourceExists(types::ResourceType resource, 
                                        const std::unordered_map<types::ResourceType, TradeGoodProperties>& trade_goods) {
        return trade_goods.find(resource) != trade_goods.end();
    }

    // ========================================================================
    // Component Removal
    // ========================================================================

    void TradeRepository::RemoveAllTradeComponents(types::EntityID province_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;
        
        ::core::ecs::EntityID ecs_id{province_id};
        
        if (HasRouteComponent(province_id)) {
            entity_manager->RemoveComponent<TradeRouteComponent>(ecs_id);
        }
        if (HasHubComponent(province_id)) {
            entity_manager->RemoveComponent<TradeHubComponent>(ecs_id);
        }
        if (HasInventoryComponent(province_id)) {
            entity_manager->RemoveComponent<TradeInventoryComponent>(ecs_id);
        }
    }

} // namespace game::trade
