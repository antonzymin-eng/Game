// ============================================================================
// ProvinceSystem.cpp - Core Province Management Implementation
// Created: October 30, 2025
// Location: src/game/province/ProvinceSystem.cpp
// ============================================================================

#include "game/province/ProvinceSystem.h"
#include "core/logging/Logger.h"
#include "game/economy/EconomicComponents.h"
#include <algorithm>
#include <cmath>

namespace game::province {

    // ============================================================================
    // ProvinceSystem Implementation
    // ============================================================================

    ProvinceSystem::ProvinceSystem(::core::ecs::ComponentAccessManager& access_manager,
                                   ::core::ecs::MessageBus& message_bus)
        : m_access_manager(access_manager), m_message_bus(message_bus) {
        m_last_update = std::chrono::steady_clock::now();
    }

    ProvinceSystem::~ProvinceSystem() = default;

    void ProvinceSystem::Initialize() {
        CORE_LOG_INFO("ProvinceSystem", "Initializing Province System");

        InitializeBuildingCosts();

        CORE_LOG_INFO("ProvinceSystem", "Province System initialized");
    }

    void ProvinceSystem::Update(float delta_time) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<double>(now - m_last_update).count();

        if (elapsed >= (1.0 / m_update_frequency)) {
            UpdateProvinces(delta_time);
            m_last_update = now;
        }
    }

    void ProvinceSystem::Shutdown() {
        CORE_LOG_INFO("ProvinceSystem", "Shutting down Province System");

        std::unique_lock<std::shared_mutex> write_lock(m_provinces_mutex);
        m_provinces.clear();
        m_province_names.clear();
    }

    ::core::threading::ThreadingStrategy ProvinceSystem::GetThreadingStrategy() const {
        return ::core::threading::ThreadingStrategy::MAIN_THREAD;
    }

    // ============================================================================
    // Province Lifecycle
    // ============================================================================

    types::EntityID ProvinceSystem::CreateProvince(const std::string& name, double x, double y) {
        types::EntityID province_id;

        {
            // Lock for reading province count and generating new ID
            std::shared_lock<std::shared_mutex> read_lock(m_provinces_mutex);
            province_id = static_cast<types::EntityID>(m_provinces.size() + 1000);
        }

        // Add province components
        if (!AddProvinceComponents(province_id)) {
            CORE_LOG_ERROR("ProvinceSystem",
                "Failed to add components for province: " + name);
            return 0;
        }

        // Initialize province data
        auto data_result = m_access_manager.GetComponentForWrite<ProvinceDataComponent>(province_id);
        if (data_result.IsValid()) {
            data_result->name = name;
            data_result->x_coordinate = x;
            data_result->y_coordinate = y;
        }

        {
            // Lock for writing to province lists
            std::unique_lock<std::shared_mutex> write_lock(m_provinces_mutex);
            m_provinces.push_back(province_id);
            m_province_names[province_id] = name;
        }

        // Publish creation event
        messages::ProvinceCreated msg;
        msg.province_id = province_id;
        msg.province_name = name;
        m_message_bus.PublishMessage(msg);

        LogProvinceAction(province_id, "Province created: " + name);

        return province_id;
    }

    bool ProvinceSystem::DestroyProvince(types::EntityID province_id) {
        {
            std::unique_lock<std::shared_mutex> write_lock(m_provinces_mutex);

            auto it = std::find(m_provinces.begin(), m_provinces.end(), province_id);
            if (it == m_provinces.end()) {
                return false;
            }

            // Remove from tracking
            m_provinces.erase(it);
            m_province_names.erase(province_id);
        }

        // Publish destruction event
        messages::ProvinceDestroyed msg;
        msg.province_id = province_id;
        m_message_bus.PublishMessage(msg);

        LogProvinceAction(province_id, "Province destroyed");

        return true;
    }

    bool ProvinceSystem::IsValidProvince(types::EntityID province_id) const {
        std::shared_lock<std::shared_mutex> read_lock(m_provinces_mutex);
        return std::find(m_provinces.begin(), m_provinces.end(), province_id) != m_provinces.end();
    }

    std::string ProvinceSystem::GetProvinceName(types::EntityID province_id) const {
        std::shared_lock<std::shared_mutex> read_lock(m_provinces_mutex);
        auto it = m_province_names.find(province_id);
        if (it != m_province_names.end()) {
            return it->second;
        }
        return "Unknown Province";
    }

    ProvinceDataComponent* ProvinceSystem::GetProvinceData(types::EntityID province_id) {
        if (!IsValidProvince(province_id)) {
            return nullptr;
        }

        auto result = m_access_manager.GetComponentForWrite<ProvinceDataComponent>(province_id);
        return result.IsValid() ? result.Get() : nullptr;
    }

    // ============================================================================
    // Building Management
    // ============================================================================

    bool ProvinceSystem::CanConstructBuilding(types::EntityID province_id,
                                              ProductionBuilding building_type) const {
        if (!IsValidProvince(province_id)) {
            return false;
        }

        auto buildings_result = m_access_manager.GetComponent<ProvinceBuildingsComponent>(province_id);
        if (!buildings_result.IsValid()) {
            return false;
        }

        auto* buildings = buildings_result.Get();

        // Check building capacity
        if (buildings->current_buildings >= buildings->max_buildings) {
            return false;
        }

        // Check if building can be upgraded
        auto it = buildings->production_buildings.find(building_type);
        int current_level = (it != buildings->production_buildings.end()) ? it->second : 0;
        if (current_level >= 10) { // Max level is 10
            return false;
        }

        // Check treasury for construction cost
        double cost = CalculateBuildingCost(building_type, current_level);
        double treasury = GetTreasuryBalance(province_id);

        return treasury >= cost;
    }

    bool ProvinceSystem::ConstructBuilding(types::EntityID province_id,
                                          ProductionBuilding building_type) {
        if (!CanConstructBuilding(province_id, building_type)) {
            return false;
        }

        auto buildings_result = m_access_manager.GetComponentForWrite<ProvinceBuildingsComponent>(province_id);
        if (!buildings_result.IsValid()) {
            return false;
        }

        auto* buildings = buildings_result.Get();
        int current_level = buildings->production_buildings[building_type];

        // Deduct cost from treasury
        double cost = CalculateBuildingCost(building_type, current_level);
        auto economic_result = m_access_manager.GetComponentForWrite<game::economy::EconomicComponent>(province_id);
        if (economic_result.IsValid()) {
            economic_result.Get()->treasury -= static_cast<int>(cost);
        }

        // Upgrade building
        buildings->production_buildings[building_type] = current_level + 1;
        if (current_level == 0) {
            buildings->current_buildings++;
        }

        // Publish building constructed event
        messages::BuildingConstructed msg;
        msg.province_id = province_id;
        msg.building_type = building_type;
        msg.new_level = current_level + 1;
        m_message_bus.PublishMessage(msg);

        LogProvinceAction(province_id,
            "Constructed " + utils::ProductionBuildingToString(building_type) +
            " (Level " + std::to_string(current_level + 1) + ")");

        return true;
    }

    int ProvinceSystem::GetBuildingLevel(types::EntityID province_id,
                                        ProductionBuilding building_type) const {
        if (!IsValidProvince(province_id)) {
            return 0;
        }

        auto buildings_result = m_access_manager.GetComponent<ProvinceBuildingsComponent>(province_id);
        if (!buildings_result.IsValid()) {
            return 0;
        }

        return buildings_result->production_buildings.at(building_type);
    }

    double ProvinceSystem::CalculateBuildingCost(ProductionBuilding building_type,
                                                 int current_level) const {
        auto it = m_building_base_costs.find(building_type);
        if (it == m_building_base_costs.end()) {
            return 100.0; // Default cost
        }

        // Cost increases with level: base_cost * (1.5 ^ level)
        double base_cost = it->second;
        return base_cost * std::pow(1.5, current_level);
    }

    // ============================================================================
    // Economic Queries
    // ============================================================================

    double ProvinceSystem::GetProsperityLevel(types::EntityID province_id) const {
        if (!IsValidProvince(province_id)) {
            return 0.0;
        }

        auto prosperity_result = m_access_manager.GetComponent<ProvinceProsperityComponent>(province_id);
        if (prosperity_result.IsValid()) {
            return prosperity_result->prosperity_level;
        }

        return 0.5; // Default prosperity
    }

    double ProvinceSystem::GetTreasuryBalance(types::EntityID province_id) const {
        if (!IsValidProvince(province_id)) {
            return 0.0;
        }

        auto economic_result = m_access_manager.GetComponent<game::economy::EconomicComponent>(province_id);
        if (economic_result.IsValid()) {
            return static_cast<double>(economic_result->treasury);
        }

        return 1000.0; // Default treasury
    }

    double ProvinceSystem::GetProductionOutput(types::EntityID province_id,
                                               const std::string& resource_type) const {
        if (!IsValidProvince(province_id)) {
            return 0.0;
        }

        auto resources_result = m_access_manager.GetComponent<ProvinceResourcesComponent>(province_id);
        if (resources_result.IsValid()) {
            auto it = resources_result->resource_production.find(resource_type);
            if (it != resources_result->resource_production.end()) {
                return it->second;
            }
        }

        return 0.0;
    }

    // ============================================================================
    // Administrative Operations
    // ============================================================================

    bool ProvinceSystem::SetOwner(types::EntityID province_id, types::EntityID nation_id) {
        auto data_result = m_access_manager.GetComponentForWrite<ProvinceDataComponent>(province_id);
        if (!data_result.IsValid()) {
            return false;
        }

        types::EntityID old_owner = data_result->owner_nation;
        data_result.Get()->owner_nation = nation_id;

        // Publish owner change event
        messages::ProvinceOwnerChanged msg;
        msg.province_id = province_id;
        msg.old_owner = old_owner;
        msg.new_owner = nation_id;
        m_message_bus.PublishMessage(msg);

        LogProvinceAction(province_id,
            "Owner changed from " + std::to_string(old_owner) +
            " to " + std::to_string(nation_id));

        return true;
    }

    bool ProvinceSystem::SetDevelopmentLevel(types::EntityID province_id, int level) {
        auto data_result = m_access_manager.GetComponentForWrite<ProvinceDataComponent>(province_id);
        if (!data_result.IsValid()) {
            return false;
        }

        level = std::max(0, std::min(level, data_result->max_development));
        data_result.Get()->development_level = level;

        LogProvinceAction(province_id, "Development level set to " + std::to_string(level));

        return true;
    }

    bool ProvinceSystem::ModifyStability(types::EntityID province_id, double change) {
        auto data_result = m_access_manager.GetComponentForWrite<ProvinceDataComponent>(province_id);
        if (!data_result.IsValid()) {
            return false;
        }

        auto* data = data_result.Get();
        data->stability += change;
        data->stability = std::max(0.0, std::min(1.0, data->stability));

        LogProvinceAction(province_id,
            "Stability modified by " + std::to_string(change) +
            " to " + std::to_string(data->stability));

        return true;
    }

    // ============================================================================
    // Economic Operations
    // ============================================================================

    bool ProvinceSystem::InvestInDevelopment(types::EntityID province_id, double investment) {
        auto data_result = m_access_manager.GetComponentForWrite<ProvinceDataComponent>(province_id);
        if (!data_result.IsValid()) {
            return false;
        }

        // Check treasury
        double treasury = GetTreasuryBalance(province_id);
        if (treasury < investment) {
            return false;
        }

        // Deduct investment from treasury
        auto economic_result = m_access_manager.GetComponentForWrite<game::economy::EconomicComponent>(province_id);
        if (economic_result.IsValid()) {
            economic_result.Get()->treasury -= static_cast<int>(investment);
        }

        // Increase development
        auto* data = data_result.Get();
        int development_gain = static_cast<int>(investment / 100.0);
        int new_level = std::min(data->development_level + development_gain,
                                 data->max_development);
        data->development_level = new_level;

        LogProvinceAction(province_id,
            "Invested " + std::to_string(investment) + " in development");

        return true;
    }

    bool ProvinceSystem::ModifyProsperity(types::EntityID province_id, double change) {
        auto prosperity_result = m_access_manager.GetComponentForWrite<ProvinceProsperityComponent>(province_id);
        if (!prosperity_result.IsValid()) {
            return false;
        }

        auto* prosperity = prosperity_result.Get();
        prosperity->prosperity_level += change;
        prosperity->prosperity_level =
            std::max(0.0, std::min(1.0, prosperity->prosperity_level));

        LogProvinceAction(province_id,
            "Prosperity modified by " + std::to_string(change));

        return true;
    }

    // ============================================================================
    // Update Logic
    // ============================================================================

    void ProvinceSystem::UpdateProvinces(float delta_time) {
        // Create a copy of province IDs to iterate over
        std::vector<types::EntityID> provinces_copy;
        {
            std::shared_lock<std::shared_mutex> read_lock(m_provinces_mutex);
            provinces_copy = m_provinces;
        }

        // Iterate without holding the lock (components have their own locking)
        for (auto province_id : provinces_copy) {
            UpdateBuildingConstruction(province_id, delta_time);
            UpdateProsperity(province_id);
            UpdateResources(province_id);

            // Check for events
            CheckEconomicCrisis(province_id);
            CheckResourceShortages(province_id);
        }
    }

    void ProvinceSystem::UpdateBuildingConstruction(types::EntityID province_id, float delta_time) {
        auto buildings_result = m_access_manager.GetComponentForWrite<ProvinceBuildingsComponent>(province_id);
        if (!buildings_result.IsValid()) {
            return;
        }

        auto* buildings = buildings_result.Get();
        if (buildings->construction_queue.empty()) {
            return;
        }

        // Progress construction (simplified - instant for now)
        buildings->construction_progress += delta_time * 0.1; // 10 seconds per building

        if (buildings->construction_progress >= 1.0) {
            ProductionBuilding to_construct = buildings->construction_queue.front();
            buildings->construction_queue.erase(buildings->construction_queue.begin());
            buildings->construction_progress = 0.0;

            ConstructBuilding(province_id, to_construct);
        }
    }

    void ProvinceSystem::UpdateProsperity(types::EntityID province_id) {
        auto prosperity_result = m_access_manager.GetComponentForWrite<ProvinceProsperityComponent>(province_id);
        if (!prosperity_result.IsValid()) {
            return;
        }

        auto* prosperity = prosperity_result.Get();

        // Calculate prosperity from factors
        double new_prosperity = (prosperity->economic_factor * 0.3 +
                                prosperity->security_factor * 0.2 +
                                prosperity->infrastructure_factor * 0.2 +
                                prosperity->population_happiness * 0.3);

        // Gradual change towards target prosperity
        double change = (new_prosperity - prosperity->prosperity_level) * 0.01;
        prosperity->prosperity_level += change;
        prosperity->growth_rate = change;

        // Track history
        prosperity->prosperity_history.push_back(prosperity->prosperity_level);
        if (prosperity->prosperity_history.size() > static_cast<size_t>(prosperity->max_history)) {
            prosperity->prosperity_history.erase(prosperity->prosperity_history.begin());
        }
    }

    void ProvinceSystem::UpdateResources(types::EntityID province_id) {
        auto resources_result = m_access_manager.GetComponentForWrite<ProvinceResourcesComponent>(province_id);
        if (!resources_result.IsValid()) {
            return;
        }

        auto* resources = resources_result.Get();

        // Update resource stockpiles based on production and consumption
        for (const auto& [resource, production] : resources->resource_production) {
            double consumption = 0.0;
            auto cons_it = resources->resource_consumption.find(resource);
            if (cons_it != resources->resource_consumption.end()) {
                consumption = cons_it->second;
            }

            double net_production = (production - consumption) * resources->production_efficiency;
            resources->resource_stockpile[resource] += net_production;

            // Cap at storage capacity
            if (resources->resource_stockpile[resource] > resources->storage_capacity) {
                resources->resource_stockpile[resource] = resources->storage_capacity;
            }
        }
    }

    // ============================================================================
    // Event Generation
    // ============================================================================

    void ProvinceSystem::CheckEconomicCrisis(types::EntityID province_id) {
        double prosperity = GetProsperityLevel(province_id);
        double treasury = GetTreasuryBalance(province_id);

        // Economic crisis if both prosperity and treasury are low
        if (prosperity < 0.3 && treasury < 100.0) {
            messages::EconomicCrisis msg;
            msg.province_id = province_id;
            msg.severity = 1.0 - prosperity;
            msg.reason = "Low prosperity and depleted treasury";
            m_message_bus.PublishMessage(msg);

            LogProvinceAction(province_id, "Economic crisis detected");
        }
    }

    void ProvinceSystem::CheckResourceShortages(types::EntityID province_id) {
        auto resources_result = m_access_manager.GetComponent<ProvinceResourcesComponent>(province_id);
        if (!resources_result.IsValid()) {
            return;
        }

        auto* resources = resources_result.Get();

        for (const auto& [resource, consumption] : resources->resource_consumption) {
            double stockpile = 0.0;
            auto stock_it = resources->resource_stockpile.find(resource);
            if (stock_it != resources->resource_stockpile.end()) {
                stockpile = stock_it->second;
            }

            // Shortage if stockpile is below one month's consumption
            if (stockpile < consumption) {
                messages::ResourceShortage msg;
                msg.province_id = province_id;
                msg.resource_type = resource;
                msg.shortage_amount = consumption - stockpile;
                m_message_bus.PublishMessage(msg);

                LogProvinceAction(province_id,
                    "Resource shortage detected: " + resource);
            }
        }
    }

    // ============================================================================
    // Helper Methods
    // ============================================================================

    void ProvinceSystem::InitializeBuildingCosts() {
        m_building_base_costs[ProductionBuilding::FARM] = 100.0;
        m_building_base_costs[ProductionBuilding::MARKET] = 150.0;
        m_building_base_costs[ProductionBuilding::SMITHY] = 200.0;
        m_building_base_costs[ProductionBuilding::WORKSHOP] = 250.0;
        m_building_base_costs[ProductionBuilding::MINE] = 300.0;
        m_building_base_costs[ProductionBuilding::TEMPLE] = 180.0;
    }

    void ProvinceSystem::LogProvinceAction(types::EntityID province_id, const std::string& action) {
        std::string province_name = GetProvinceName(province_id);
        CORE_LOG_INFO("ProvinceSystem",
            "Province [" + province_name + " #" + std::to_string(province_id) + "]: " + action);
    }

    bool ProvinceSystem::AddProvinceComponents(types::EntityID province_id) {
        // Get EntityManager from ComponentAccessManager
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) {
            CORE_LOG_ERROR("ProvinceSystem",
                "Cannot add components: EntityManager not available");
            return false;
        }

        // Convert to core::ecs::EntityID
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(province_id), 1);

        // Add all required province components
        auto data_comp = entity_manager->AddComponent<ProvinceDataComponent>(entity_handle);
        if (!data_comp) {
            CORE_LOG_ERROR("ProvinceSystem",
                "Failed to add ProvinceDataComponent for province " + std::to_string(province_id));
            return false;
        }

        auto buildings_comp = entity_manager->AddComponent<ProvinceBuildingsComponent>(entity_handle);
        if (!buildings_comp) {
            CORE_LOG_ERROR("ProvinceSystem",
                "Failed to add ProvinceBuildingsComponent for province " + std::to_string(province_id));
            return false;
        }

        auto resources_comp = entity_manager->AddComponent<ProvinceResourcesComponent>(entity_handle);
        if (!resources_comp) {
            CORE_LOG_ERROR("ProvinceSystem",
                "Failed to add ProvinceResourcesComponent for province " + std::to_string(province_id));
            return false;
        }

        auto prosperity_comp = entity_manager->AddComponent<ProvinceProsperityComponent>(entity_handle);
        if (!prosperity_comp) {
            CORE_LOG_ERROR("ProvinceSystem",
                "Failed to add ProvinceProsperityComponent for province " + std::to_string(province_id));
            return false;
        }

        CORE_LOG_INFO("ProvinceSystem",
            "Successfully added all components for province " + std::to_string(province_id));

        return true;
    }

    // ============================================================================
    // Utility Functions
    // ============================================================================

    namespace utils {

        std::string ProductionBuildingToString(ProductionBuilding building) {
            switch (building) {
                case ProductionBuilding::FARM: return "Farm";
                case ProductionBuilding::MARKET: return "Market";
                case ProductionBuilding::SMITHY: return "Smithy";
                case ProductionBuilding::WORKSHOP: return "Workshop";
                case ProductionBuilding::MINE: return "Mine";
                case ProductionBuilding::TEMPLE: return "Temple";
                default: return "Unknown Building";
            }
        }

        std::string InfrastructureBuildingToString(InfrastructureBuilding building) {
            switch (building) {
                case InfrastructureBuilding::ROAD: return "Road";
                case InfrastructureBuilding::PORT: return "Port";
                case InfrastructureBuilding::FORTRESS: return "Fortress";
                case InfrastructureBuilding::UNIVERSITY: return "University";
                default: return "Unknown Building";
            }
        }

        ProductionBuilding StringToProductionBuilding(const std::string& str) {
            if (str == "Farm" || str == "FARM") return ProductionBuilding::FARM;
            if (str == "Market" || str == "MARKET") return ProductionBuilding::MARKET;
            if (str == "Smithy" || str == "SMITHY") return ProductionBuilding::SMITHY;
            if (str == "Workshop" || str == "WORKSHOP") return ProductionBuilding::WORKSHOP;
            if (str == "Mine" || str == "MINE") return ProductionBuilding::MINE;
            if (str == "Temple" || str == "TEMPLE") return ProductionBuilding::TEMPLE;
            return ProductionBuilding::FARM; // Default
        }

        bool IsProductionBuilding(const std::string& str) {
            return str == "Farm" || str == "Market" || str == "Smithy" ||
                   str == "Workshop" || str == "Mine" || str == "Temple";
        }

        double CalculateConstructionTime(ProductionBuilding building, int level) {
            // Base construction time in days
            double base_time = 30.0;

            switch (building) {
                case ProductionBuilding::FARM: base_time = 20.0; break;
                case ProductionBuilding::MARKET: base_time = 30.0; break;
                case ProductionBuilding::SMITHY: base_time = 40.0; break;
                case ProductionBuilding::WORKSHOP: base_time = 50.0; break;
                case ProductionBuilding::MINE: base_time = 60.0; break;
                case ProductionBuilding::TEMPLE: base_time = 45.0; break;
                default: break;
            }

            // Time increases with level
            return base_time * (1.0 + level * 0.2);
        }

        double CalculateMaintenanceCost(ProductionBuilding building, int level) {
            double base_cost = 5.0;

            switch (building) {
                case ProductionBuilding::FARM: base_cost = 3.0; break;
                case ProductionBuilding::MARKET: base_cost = 5.0; break;
                case ProductionBuilding::SMITHY: base_cost = 7.0; break;
                case ProductionBuilding::WORKSHOP: base_cost = 8.0; break;
                case ProductionBuilding::MINE: base_cost = 10.0; break;
                case ProductionBuilding::TEMPLE: base_cost = 6.0; break;
                default: break;
            }

            return base_cost * level;
        }

        bool ValidateProvinceData(const ProvinceDataComponent& data) {
            if (data.name.empty()) return false;
            if (data.autonomy < 0.0 || data.autonomy > 1.0) return false;
            if (data.stability < 0.0 || data.stability > 1.0) return false;
            if (data.war_exhaustion < 0.0 || data.war_exhaustion > 1.0) return false;
            if (data.development_level < 0 || data.development_level > data.max_development) return false;
            return true;
        }

    } // namespace utils

} // namespace game::province
