// ============================================================================
// Mechanica Imperii - Hub Manager Implementation
// ============================================================================

#include "game/trade/HubManager.h"
#include "game/trade/TradeCalculator.h"
#include <iostream>
#include <algorithm>

namespace game::trade {

    HubManager::HubManager(
        TradeRepository& repository,
        std::unordered_map<types::EntityID, TradeHub>& trade_hubs,
        const std::unordered_map<std::string, TradeRoute>& active_routes,
        ::core::threading::ThreadSafeMessageBus& message_bus,
        std::mutex& trade_mutex
    )
        : m_repository(repository)
        , m_trade_hubs(trade_hubs)
        , m_active_routes(active_routes)
        , m_message_bus(message_bus)
        , m_trade_mutex(trade_mutex) {
    }

    // ========================================================================
    // Hub Creation and Evolution
    // ========================================================================

    void HubManager::CreateHub(types::EntityID province_id, const std::string& hub_name, HubType initial_type) {
        std::lock_guard<std::mutex> lock(m_trade_mutex);

        // Check if hub already exists
        if (m_trade_hubs.find(province_id) != m_trade_hubs.end()) {
            std::cout << "[HubManager] Trade hub already exists at province " << province_id << std::endl;
            return;
        }

        // Create new trade hub
        TradeHub new_hub(province_id, hub_name);
        new_hub.hub_type = initial_type;
        new_hub.max_throughput_capacity = TradeCalculator::CalculateHubCapacity(province_id, initial_type);
        new_hub.infrastructure_bonus = TradeCalculator::DetermineInfrastructureBonus(province_id);
        new_hub.security_rating = 0.9; // Start with good security
        new_hub.reputation_rating = 1.0; // Start with neutral reputation

        m_trade_hubs[province_id] = new_hub;

        // Create ECS component for the hub
        m_repository.EnsureAllTradeComponents(province_id);
        auto hub_comp = m_repository.GetHubComponent(province_id);
        if (hub_comp) {
            hub_comp->hub_data = new_hub;
        }

        std::cout << "[HubManager] Created trade hub '" << hub_name << "' at province " << province_id << std::endl;
    }

    bool HubManager::EvolveHub(types::EntityID province_id) {
        std::lock_guard<std::mutex> lock(m_trade_mutex);

        auto hub_it = m_trade_hubs.find(province_id);
        if (hub_it == m_trade_hubs.end()) {
            return false;
        }

        TradeHub& hub = hub_it->second;
        HubType old_type = hub.hub_type;
        HubType optimal_type = DetermineOptimalType(province_id);

        if (optimal_type != old_type) {
            hub.hub_type = optimal_type;

            // Adjust capacity for new type
            double base_capacity = TradeCalculator::CalculateHubCapacity(province_id, optimal_type);
            hub.max_throughput_capacity = base_capacity;

            // Update specializations
            UpdateSpecializations(hub);

            // Publish evolution event
            PublishHubEvolution(hub, old_type, "Economic growth and trade volume");

            std::cout << "[HubManager] Hub at province " << province_id << " evolved from type "
                      << static_cast<int>(old_type) << " to " << static_cast<int>(optimal_type) << std::endl;

            return true;
        }

        return false;
    }

    bool HubManager::UpgradeHub(types::EntityID province_id, int new_level) {
        std::lock_guard<std::mutex> lock(m_trade_mutex);

        auto hub_it = m_trade_hubs.find(province_id);
        if (hub_it == m_trade_hubs.end()) {
            return false;
        }

        TradeHub& hub = hub_it->second;
        new_level = std::clamp(new_level, 1, 5);

        if (new_level > hub.upgrade_level) {
            hub.upgrade_level = new_level;

            // Improve hub capabilities
            hub.max_throughput_capacity *= (1.0 + (new_level - 1) * 0.25); // 25% per level
            hub.infrastructure_bonus = 1.0 + (new_level - 1) * 0.15; // 15% per level
            hub.security_rating = std::min(1.0, hub.security_rating + (new_level - 1) * 0.1);

            std::cout << "[HubManager] Upgraded hub at province " << province_id
                      << " to level " << new_level << std::endl;

            return true;
        }

        return false;
    }

    HubType HubManager::DetermineOptimalType(types::EntityID province_id) const {
        double total_volume = GetTotalTradeVolume(province_id);
        int route_count = GetRouteCount(province_id);

        // Determine optimal type based on volume and connectivity
        if (total_volume > 1000.0 && route_count > 20) {
            return HubType::INTERNATIONAL_PORT;
        } else if (total_volume > 500.0 && route_count > 10) {
            return HubType::MAJOR_TRADING_CENTER;
        } else if (route_count > 6) {
            return HubType::CROSSROADS;
        } else if (total_volume > 100.0 || route_count > 3) {
            return HubType::REGIONAL_HUB;
        } else {
            return HubType::LOCAL_MARKET;
        }
    }

    // ========================================================================
    // Hub Queries
    // ========================================================================

    TradeHub* HubManager::GetHub(types::EntityID province_id) {
        auto hub_it = m_trade_hubs.find(province_id);
        return (hub_it != m_trade_hubs.end()) ? &hub_it->second : nullptr;
    }

    const TradeHub* HubManager::GetHub(types::EntityID province_id) const {
        auto hub_it = m_trade_hubs.find(province_id);
        return (hub_it != m_trade_hubs.end()) ? &hub_it->second : nullptr;
    }

    bool HubManager::HasHub(types::EntityID province_id) const {
        return m_trade_hubs.find(province_id) != m_trade_hubs.end();
    }

    std::vector<types::EntityID> HubManager::GetTradingPartners(types::EntityID province_id) const {
        std::unordered_set<types::EntityID> partners;

        // Find all routes connected to this province
        for (const auto& [route_id, route] : m_active_routes) {
            if (route.source_province == province_id) {
                partners.insert(route.destination_province);
            } else if (route.destination_province == province_id) {
                partners.insert(route.source_province);
            }
        }

        return std::vector<types::EntityID>(partners.begin(), partners.end());
    }

    // ========================================================================
    // Hub Maintenance
    // ========================================================================

    void HubManager::UpdateUtilization(TradeHub& hub) {
        double total_volume = 0.0;

        // Calculate total volume passing through this hub
        for (const auto& route_id : hub.incoming_route_ids) {
            auto route_it = m_active_routes.find(route_id);
            if (route_it != m_active_routes.end()) {
                total_volume += route_it->second.GetEffectiveVolume();
            }
        }

        for (const auto& route_id : hub.outgoing_route_ids) {
            auto route_it = m_active_routes.find(route_id);
            if (route_it != m_active_routes.end()) {
                total_volume += route_it->second.GetEffectiveVolume();
            }
        }

        hub.current_utilization = TradeCalculator::CalculateHubUtilization(
            total_volume, hub.GetEffectiveCapacity());
    }

    void HubManager::UpdateSpecializations(TradeHub& hub) {
        // Analyze trade patterns to determine specializations
        std::unordered_map<types::ResourceType, double> resource_volumes;

        for (const auto& route_id : hub.incoming_route_ids) {
            auto route_it = m_active_routes.find(route_id);
            if (route_it != m_active_routes.end()) {
                resource_volumes[route_it->second.resource] += route_it->second.GetEffectiveVolume();
            }
        }

        // Clear old specializations
        hub.specialized_goods.clear();

        // Add specializations for high-volume resources
        for (const auto& [resource, volume] : resource_volumes) {
            if (volume > hub.GetEffectiveCapacity() * 0.2) { // 20% of capacity threshold
                hub.specialized_goods.insert(resource);
                hub.handling_efficiency[resource] = 1.3; // 30% efficiency bonus
            }
        }
    }

    void HubManager::CalculateReputation(TradeHub& hub) {
        double total_volume = GetTotalTradeVolume(hub.province_id);
        hub.reputation_rating = TradeCalculator::CalculateHubReputation(
            hub.current_utilization, hub.security_rating, hub.infrastructure_bonus);
    }

    void HubManager::UpdateAllHubs() {
        std::lock_guard<std::mutex> lock(m_trade_mutex);

        for (auto& [province_id, hub] : m_trade_hubs) {
            UpdateUtilization(hub);
            UpdateSpecializations(hub);
            CalculateReputation(hub);
        }
    }

    // ========================================================================
    // Private Helper Methods
    // ========================================================================

    void HubManager::PublishHubEvolution(const TradeHub& hub, HubType old_type, const std::string& trigger) {
        messages::TradeHubEvolved event;
        event.province_id = hub.province_id;
        event.old_type = old_type;
        event.new_type = hub.hub_type;
        event.new_capacity = hub.max_throughput_capacity;
        event.new_specializations = std::vector<types::ResourceType>(hub.specialized_goods.begin(),
                                                                    hub.specialized_goods.end());
        event.evolution_trigger = trigger;

        m_message_bus.Publish(event);
    }

    double HubManager::GetTotalTradeVolume(types::EntityID province_id) const {
        double total_volume = 0.0;

        for (const auto& [route_id, route] : m_active_routes) {
            if (route.source_province == province_id || route.destination_province == province_id) {
                total_volume += route.GetEffectiveVolume();
            }
        }

        return total_volume;
    }

    int HubManager::GetRouteCount(types::EntityID province_id) const {
        int count = 0;

        for (const auto& [route_id, route] : m_active_routes) {
            if (route.source_province == province_id || route.destination_province == province_id) {
                count++;
            }
        }

        return count;
    }

} // namespace game::trade
