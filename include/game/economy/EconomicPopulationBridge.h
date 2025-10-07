// Created: September 11, 2025 - 3:30 PM PST
// Economic-Population Bridge System - Header Declarations
// Mechanica Imperii - Critical Integration System

#pragma once

#include "../core/ECS.h"
#include "../core/MessageBus.h"
#include "../population/PopulationTypes.h"
#include "../technology/TechnologyTypes.h"
#include "EnhancedProvinceSystem.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <chrono>

namespace economicbridge {

    // ============================================================================
    // Resource & Economic Types
    // ============================================================================

    enum class ResourceType {
        // Primary Resources (Extractable)
        GRAIN = 0, LIVESTOCK, FISH, SALT, TIMBER, STONE, IRON_ORE, CLAY,

        // Processed Resources (Manufacturable)  
        BREAD, MEAT, TOOLS, WEAPONS, CLOTH, POTTERY, LEATHER, FURNITURE,

        // Luxury Resources
        WINE, SPICES, JEWELRY, SILK, BOOKS, PRECIOUS_METALS,

        // Special Resources
        LABOR_PEASANT, LABOR_CRAFTSMAN, LABOR_MERCHANT, EDUCATION,

        COUNT
    };

    enum class ProductionBuilding {
        FARM = 0, MILL, PASTURE, FISHERY, MINE, QUARRY, LUMBER_CAMP,
        WORKSHOP, SMITHY, WEAVER, TANNER, BREWERY, MARKET, DOCK,
        CHURCH, SCHOOL, BARRACKS, WALLS,
        COUNT
    };

    enum class TradeStatus {
        SURPLUS,    // Export available
        BALANCED,   // Local production meets demand
        DEFICIT,    // Import needed
        CRITICAL    // Severe shortage
    };

    // ============================================================================
    // Economic Integration Components
    // ============================================================================

    struct EconomicFlowComponent {
        // Resource production by type
        std::unordered_map<ResourceType, double> production_rates;
        std::unordered_map<ResourceType, double> consumption_rates;
        std::unordered_map<ResourceType, double> current_stockpile;

        // Trade flows
        std::unordered_map<ResourceType, double> import_volume;
        std::unordered_map<ResourceType, double> export_volume;
        std::unordered_map<ResourceType, TradeStatus> trade_status;

        // Economic metrics
        double monthly_income = 0.0;
        double monthly_expenses = 0.0;
        double trade_balance = 0.0;
        double economic_prosperity = 0.5; // 0.0 to 1.0

        EconomicFlowComponent() {
            // Initialize resource maps
            for (int i = 0; i < static_cast<int>(ResourceType::COUNT); ++i) {
                ResourceType resource = static_cast<ResourceType>(i);
                production_rates[resource] = 0.0;
                consumption_rates[resource] = 0.0;
                current_stockpile[resource] = 100.0; // Basic starting stockpile
                import_volume[resource] = 0.0;
                export_volume[resource] = 0.0;
                trade_status[resource] = TradeStatus::BALANCED;
            }
        }
    };

    struct EmploymentComponent {
        // Workers by social class
        std::unordered_map<types::SocialClass, int> available_workers;
        std::unordered_map<types::SocialClass, int> employed_workers;
        std::unordered_map<types::SocialClass, int> unemployed_workers;

        // Employment by building type
        std::unordered_map<ProductionBuilding, int> workers_assigned;
        std::unordered_map<ProductionBuilding, int> workers_needed;

        // Employment metrics
        double overall_employment_rate = 0.0;
        double productivity_efficiency = 1.0; // Modifier based on employment
        double wage_pressure = 0.0; // Economic pressure from unemployment

        EmploymentComponent() {
            // Initialize with basic employment structure
            available_workers[types::SocialClass::PEASANTS] = 0;
            available_workers[types::SocialClass::CRAFTSMEN] = 0;
            available_workers[types::SocialClass::MERCHANTS] = 0;
            available_workers[types::SocialClass::NOBILITY] = 0;
            available_workers[types::SocialClass::CLERGY] = 0;
        }
    };

    struct ConsumptionComponent {
        // Consumption needs by social class
        std::unordered_map<types::SocialClass, std::unordered_map<ResourceType, double>> class_consumption_rates;

        // Market demand calculations
        std::unordered_map<ResourceType, double> total_demand;
        std::unordered_map<ResourceType, double> demand_satisfaction; // 0.0 to 1.0
        std::unordered_map<ResourceType, double> local_prices;

        // Economic pressure metrics
        double cost_of_living = 1.0;
        double quality_of_life = 0.5; // Average satisfaction across all classes
        std::unordered_map<types::SocialClass, double> class_satisfaction;

        ConsumptionComponent() {
            // Initialize consumption patterns for different social classes
            InitializeConsumptionPatterns();
        }

    private:
        void InitializeConsumptionPatterns();
    };

    // ============================================================================
    // Economic Bridge System
    // ============================================================================

    class EconomicPopulationBridge : public ISystem {
    public:
        EconomicPopulationBridge(MessageBus& message_bus);
        virtual ~EconomicPopulationBridge() = default;

        // ISystem interface
        void Update(double delta_time) override;
        void HandleMessage(const Message& message) override;
        const std::string& GetName() const override { return system_name_; }

        // Economic calculation methods
        void CalculateEmployment(types::EntityID province_id);
        void CalculateProduction(types::EntityID province_id);
        void CalculateConsumption(types::EntityID province_id);
        void CalculateTradeFlows(types::EntityID province_id);
        void ProcessMigrationPressure(types::EntityID province_id);

        // Economic queries
        double GetResourceProduction(types::EntityID province_id, ResourceType resource) const;
        double GetResourceConsumption(types::EntityID province_id, ResourceType resource) const;
        double GetEmploymentRate(types::EntityID province_id, types::SocialClass social_class) const;
        TradeStatus GetTradeStatus(types::EntityID province_id, ResourceType resource) const;

        // Economic metrics
        double GetEconomicProsperity(types::EntityID province_id) const;
        double GetCostOfLiving(types::EntityID province_id) const;
        std::vector<types::EntityID> GetTradePartners(types::EntityID province_id) const;

        // Configuration
        void SetUpdateFrequency(double frequency) { update_frequency_ = frequency; }
        void EnableDetailedLogging(bool enable) { detailed_logging_ = enable; }

    private:
        // Core calculation methods
        void UpdateProvinceEconomics(types::EntityID province_id, double delta_time);
        void CalculateResourceBalance(types::EntityID province_id);
        void UpdatePricesFromSupplyDemand(types::EntityID province_id);
        void ProcessEconomicEvents(types::EntityID province_id);

        // Employment calculation helpers
        int CalculateWorkersNeeded(ProductionBuilding building, int building_level) const;
        types::SocialClass GetRequiredWorkerClass(ProductionBuilding building) const;
        double CalculateProductivityEfficiency(const EmploymentComponent& employment) const;

        // Production calculation helpers
        double CalculateBaseProduction(ProductionBuilding building, int level) const;
        double ApplyProductionModifiers(types::EntityID province_id, ResourceType resource, double base_production) const;
        ResourceType GetBuildingOutput(ProductionBuilding building) const;

        // Consumption calculation helpers
        double CalculateClassConsumption(types::SocialClass social_class, ResourceType resource, int population) const;
        void UpdateDemandSatisfaction(types::EntityID province_id);
        double CalculateResourcePrice(ResourceType resource, double supply, double demand) const;

        // Migration calculation helpers
        double CalculateMigrationPressure(types::EntityID province_id) const;
        std::vector<types::EntityID> FindMigrationTargets(types::EntityID source_province) const;
        void ProcessMigrationBetweenProvinces(types::EntityID source, types::EntityID target, int migrants, types::SocialClass social_class);

        // System state
        MessageBus& message_bus_;
        std::string system_name_ = "EconomicPopulationBridge";
        double accumulated_time_ = 0.0;
        double update_frequency_ = 1.0; // Update every second
        bool detailed_logging_ = false;

        // Economic configuration
        struct EconomicConfig {
            double base_consumption_rate = 1.0;
            double employment_efficiency_factor = 1.5;
            double migration_threshold = 0.3;
            double price_volatility = 0.1;
            double trade_distance_factor = 0.1;
        } config_;

        // Performance tracking
        mutable double last_update_duration_ = 0.0;
        mutable int provinces_processed_ = 0;
    };

    // ============================================================================
    // Economic Event Messages
    // ============================================================================

    namespace messages {

        struct EconomicCrisis {
            types::EntityID province_id;
            ResourceType critical_resource;
            double shortage_severity; // 0.0 to 1.0
            std::string crisis_description;
            int estimated_duration_months;
        };

        struct TradeRouteEstablished {
            types::EntityID source_province;
            types::EntityID target_province;
            ResourceType resource;
            double monthly_volume;
            double profit_margin;
            int route_length_km;
        };

        struct EmploymentCrisis {
            types::EntityID province_id;
            types::SocialClass affected_class;
            double unemployment_rate;
            int unemployed_count;
            std::vector<std::string> contributing_factors;
        };

        struct ResourceDiscovered {
            types::EntityID province_id;
            ResourceType resource;
            double estimated_yield;
            ProductionBuilding required_building;
            int discovery_bonus_months;
        };

        struct EconomicBoom {
            types::EntityID province_id;
            double prosperity_increase;
            std::vector<ResourceType> contributing_resources;
            int estimated_duration_months;
            double population_growth_bonus;
        };

        struct MigrationWave {
            types::EntityID source_province;
            types::EntityID target_province;
            types::SocialClass migrating_class;
            int migrant_count;
            std::string migration_reason;
            int estimated_arrival_time_months;
        };

    } // namespace messages

    // ============================================================================
    // Economic Utility Functions
    // ============================================================================

    namespace economic_utils {

        // Resource classification helpers
        bool IsBasicNeed(ResourceType resource);
        bool IsLuxuryGood(ResourceType resource);
        bool IsRawMaterial(ResourceType resource);
        bool IsProcessedGood(ResourceType resource);

        // Economic calculation helpers
        double CalculateDistanceFactor(types::EntityID province1, types::EntityID province2);
        double CalculateTradeProfit(ResourceType resource, double price_source, double price_target, double distance);
        std::vector<ResourceType> GetResourceDependencies(ResourceType resource);
        std::vector<ProductionBuilding> GetCompatibleBuildings(types::SocialClass worker_class);

        // String conversion utilities
        std::string ResourceTypeToString(ResourceType resource);
        std::string ProductionBuildingToString(ProductionBuilding building);
        std::string TradeStatusToString(TradeStatus status);

        ResourceType StringToResourceType(const std::string& str);
        ProductionBuilding StringToProductionBuilding(const std::string& str);
        TradeStatus StringToTradeStatus(const std::string& str);

    } // namespace economic_utils

} // namespace economicbridge