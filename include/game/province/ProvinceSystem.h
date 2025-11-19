// ============================================================================
// ProvinceSystem.h - Core Province Management System (ECS-based)
// Created: October 30, 2025
// Location: include/game/province/ProvinceSystem.h
// Purpose: Core province functionality for the game's strategic layer
// ============================================================================

#pragma once

#include "core/ECS/ComponentAccessManager.h"
#include "core/ECS/MessageBus.h"
#include "core/ECS/EntityManager.h"
#include "core/ECS/ISystem.h"
#include "core/threading/ThreadedSystemManager.h"
#include "core/types/game_types.h"

#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <shared_mutex>
#include <algorithm>

namespace game::province {

    // ============================================================================
    // Province Building Types
    // ============================================================================

    enum class ProductionBuilding : uint8_t {
        FARM = 0,
        MARKET = 1,
        SMITHY = 2,
        WORKSHOP = 3,
        MINE = 4,
        TEMPLE = 5,
        COUNT
    };

    enum class InfrastructureBuilding : uint8_t {
        ROAD = 0,
        PORT = 1,
        FORTRESS = 2,
        UNIVERSITY = 3,
        COUNT
    };

    // ============================================================================
    // Province Messages for Event System
    // ============================================================================

    namespace messages {

        struct ProvinceCreated {
            types::EntityID province_id;
            std::string province_name;
        };

        struct ProvinceDestroyed {
            types::EntityID province_id;
        };

        struct EconomicCrisis {
            types::EntityID province_id;
            double severity; // 0.0 - 1.0
            std::string reason;
        };

        struct ResourceShortage {
            types::EntityID province_id;
            std::string resource_type;
            double shortage_amount;
        };

        struct BuildingConstructed {
            types::EntityID province_id;
            ProductionBuilding building_type;
            int new_level;
        };

        struct ProvinceOwnerChanged {
            types::EntityID province_id;
            types::EntityID old_owner;
            types::EntityID new_owner;
        };

    } // namespace messages

    // ============================================================================
    // Province Components
    // ============================================================================
    //
    // NOTE: This is the CANONICAL province component for the game system.
    //
    // IMPORTANT: There are multiple province representations in the codebase:
    //   1. game::province::ProvinceDataComponent (THIS ONE) - Full ECS component
    //      Use this for: Game systems, province management, economic simulation
    //   2. AI::ProvinceComponent (in game/components/ProvinceComponent.h) - Minimal AI component
    //      Use this for: AI decision making, pathfinding (being phased out)
    //   3. game::Province (in game/gameplay/Province.h) - Legacy non-ECS struct
    //      Use this for: Legacy code compatibility only (deprecated)
    //
    // For new code: ALWAYS use game::province::ProvinceDataComponent
    //
    // ============================================================================

    struct ProvinceDataComponent : public game::core::Component<ProvinceDataComponent> {
        std::string name;
        types::EntityID owner_nation{0};

        // Geographic data
        double x_coordinate = 0.0;
        double y_coordinate = 0.0;
        double area = 100.0; // in square km

        // Administrative data
        double autonomy = 0.0; // 0.0 - 1.0
        double stability = 0.5; // 0.0 - 1.0
        double war_exhaustion = 0.0; // 0.0 - 1.0

        // Development
        int development_level = 1;
        int max_development = 100;

        ProvinceDataComponent() = default;
        explicit ProvinceDataComponent(const std::string& province_name) : name(province_name) {}

        std::string GetComponentTypeName() const override { return "ProvinceDataComponent"; }

        // Validation and setter methods to enforce valid ranges
        void SetAutonomy(double value) {
            autonomy = std::clamp(value, 0.0, 1.0);
        }

        void SetStability(double value) {
            stability = std::clamp(value, 0.0, 1.0);
        }

        void SetWarExhaustion(double value) {
            war_exhaustion = std::clamp(value, 0.0, 1.0);
        }

        void SetDevelopmentLevel(int level) {
            development_level = std::clamp(level, 0, max_development);
        }

        bool IsValid() const {
            return !name.empty() &&
                   autonomy >= 0.0 && autonomy <= 1.0 &&
                   stability >= 0.0 && stability <= 1.0 &&
                   war_exhaustion >= 0.0 && war_exhaustion <= 1.0 &&
                   development_level >= 0 && development_level <= max_development &&
                   area > 0.0;
        }
    };

    struct ProvinceBuildingsComponent : public game::core::Component<ProvinceBuildingsComponent> {
        // Building levels (0 = not built)
        std::unordered_map<ProductionBuilding, int> production_buildings;
        std::unordered_map<InfrastructureBuilding, int> infrastructure_buildings;

        // Building capacity
        int max_buildings = 10;
        int current_buildings = 0;

        // Construction queue
        std::vector<ProductionBuilding> construction_queue;
        double construction_progress = 0.0;

        ProvinceBuildingsComponent() {
            // Initialize all building types to level 0
            // Explicit initialization to avoid unsafe enum casting
            production_buildings[ProductionBuilding::FARM] = 0;
            production_buildings[ProductionBuilding::MARKET] = 0;
            production_buildings[ProductionBuilding::SMITHY] = 0;
            production_buildings[ProductionBuilding::WORKSHOP] = 0;
            production_buildings[ProductionBuilding::MINE] = 0;
            production_buildings[ProductionBuilding::TEMPLE] = 0;

            infrastructure_buildings[InfrastructureBuilding::ROAD] = 0;
            infrastructure_buildings[InfrastructureBuilding::PORT] = 0;
            infrastructure_buildings[InfrastructureBuilding::FORTRESS] = 0;
            infrastructure_buildings[InfrastructureBuilding::UNIVERSITY] = 0;
        }

        std::string GetComponentTypeName() const override { return "ProvinceBuildingsComponent"; }
    };

    struct ProvinceResourcesComponent : public game::core::Component<ProvinceResourcesComponent> {
        // Resource production per month
        std::unordered_map<std::string, double> resource_production;

        // Resource consumption per month
        std::unordered_map<std::string, double> resource_consumption;

        // Resource storage
        std::unordered_map<std::string, double> resource_stockpile;
        double storage_capacity = 1000.0;

        // Resource modifiers
        double production_efficiency = 1.0;
        double harvest_modifier = 1.0;

        ProvinceResourcesComponent() = default;

        std::string GetComponentTypeName() const override { return "ProvinceResourcesComponent"; }
    };

    struct ProvinceProsperityComponent : public game::core::Component<ProvinceProsperityComponent> {
        // Prosperity metrics (0.0 - 1.0)
        double prosperity_level = 0.5;
        double growth_rate = 0.0;

        // Factors affecting prosperity
        double economic_factor = 0.5;
        double security_factor = 0.5;
        double infrastructure_factor = 0.5;
        double population_happiness = 0.5;

        // Historical tracking
        std::vector<double> prosperity_history;
        int max_history = 24; // months

        ProvinceProsperityComponent() = default;

        std::string GetComponentTypeName() const override { return "ProvinceProsperityComponent"; }
    };

    // ============================================================================
    // Province System - Core Province Management
    // ============================================================================

    class ProvinceSystem : public game::core::ISystem {
    private:
        ::core::ecs::ComponentAccessManager& m_access_manager;

        // Note: Using regular MessageBus (not ThreadSafeMessageBus) because this system
        // runs on MAIN_THREAD (see GetThreadingStrategy()). If threading strategy changes
        // to allow multi-threaded execution, switch to ThreadSafeMessageBus.
        ::core::ecs::MessageBus& m_message_bus;

        // Thread safety for province tracking
        // Note: Currently runs on MAIN_THREAD, but adding mutex for future-proofing
        mutable std::shared_mutex m_provinces_mutex;

        // Province tracking (protected by m_provinces_mutex)
        std::vector<types::EntityID> m_provinces;
        std::unordered_map<types::EntityID, std::string> m_province_names;

        // Building costs and requirements
        std::unordered_map<ProductionBuilding, double> m_building_base_costs;

        // Update timing
        std::chrono::steady_clock::time_point m_last_update;
        double m_update_frequency = 1.0; // 1 update per second

    public:
        explicit ProvinceSystem(::core::ecs::ComponentAccessManager& access_manager,
                               ::core::ecs::MessageBus& message_bus);
        ~ProvinceSystem() override;

        // ISystem interface
        void Initialize() override;
        void Update(float delta_time) override;
        void Shutdown() override;

        std::string GetSystemName() const override { return "ProvinceSystem"; }
        ::core::threading::ThreadingStrategy GetThreadingStrategy() const override;

        // Province lifecycle
        types::EntityID CreateProvince(const std::string& name, double x, double y);
        bool DestroyProvince(types::EntityID province_id);
        bool IsValidProvince(types::EntityID province_id) const;

        // Province queries
        std::vector<types::EntityID> GetAllProvinces() const {
            std::shared_lock<std::shared_mutex> read_lock(m_provinces_mutex);
            return m_provinces;
        }
        std::string GetProvinceName(types::EntityID province_id) const;

        /**
         * Get province data component for modification
         * @param province_id The province entity ID
         * @return Pointer to ProvinceDataComponent, or nullptr if province doesn't exist
         * @warning CALLER MUST CHECK FOR NULL! Always verify return value before dereferencing.
         * @example
         *   auto* data = GetProvinceData(id);
         *   if (data) { data->stability = 0.5; }
         */
        ProvinceDataComponent* GetProvinceData(types::EntityID province_id);

        // Building management
        bool CanConstructBuilding(types::EntityID province_id, ProductionBuilding building_type) const;
        bool ConstructBuilding(types::EntityID province_id, ProductionBuilding building_type);
        int GetBuildingLevel(types::EntityID province_id, ProductionBuilding building_type) const;
        double CalculateBuildingCost(ProductionBuilding building_type, int current_level) const;

        // Economic queries
        double GetProsperityLevel(types::EntityID province_id) const;
        double GetTreasuryBalance(types::EntityID province_id) const;
        double GetProductionOutput(types::EntityID province_id, const std::string& resource_type) const;

        // Administrative operations
        bool SetOwner(types::EntityID province_id, types::EntityID nation_id);
        bool SetDevelopmentLevel(types::EntityID province_id, int level);
        bool ModifyStability(types::EntityID province_id, double change);

        // Economic operations
        bool InvestInDevelopment(types::EntityID province_id, double investment);
        bool ModifyProsperity(types::EntityID province_id, double change);

    private:
        // Update logic
        void UpdateProvinces(float delta_time);
        void UpdateBuildingConstruction(types::EntityID province_id, float delta_time);
        void UpdateProsperity(types::EntityID province_id);
        void UpdateResources(types::EntityID province_id);

        // Event generation
        void CheckEconomicCrisis(types::EntityID province_id);
        void CheckResourceShortages(types::EntityID province_id);

        // Helper methods
        void InitializeBuildingCosts();
        void LogProvinceAction(types::EntityID province_id, const std::string& action);
        bool AddProvinceComponents(types::EntityID province_id);
    };

    // ============================================================================
    // Utility Functions
    // ============================================================================

    namespace utils {

        // String conversion utilities
        std::string ProductionBuildingToString(ProductionBuilding building);
        std::string InfrastructureBuildingToString(InfrastructureBuilding building);

        // Building utilities
        ProductionBuilding StringToProductionBuilding(const std::string& str);
        bool IsProductionBuilding(const std::string& str);

        // Cost calculations
        double CalculateConstructionTime(ProductionBuilding building, int level);
        double CalculateMaintenanceCost(ProductionBuilding building, int level);

        // Province validation
        bool ValidateProvinceData(const ProvinceDataComponent& data);

    } // namespace utils

} // namespace game::province
