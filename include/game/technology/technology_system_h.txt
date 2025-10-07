// ============================================================================
// TechnologySystem.h - Modernized Technology Research and Innovation System
// Created: September 22, 2025, 17:55 UTC
// Location: include/game/technology/TechnologySystem.h
// Modernized to align with completed systems architecture
// ============================================================================

#pragma once

#include "core/ECS/ComponentAccessManager.h"
#include "core/Threading/ThreadSafeMessageBus.h"
#include "core/Threading/ThreadSafeSystem.h"
#include "core/Types/game_types.h"
#include "game/province/EnhancedProvinceSystem.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <string>
#include <chrono>

namespace game::technology {

    // Forward declarations
    class TechnologySystem;

    // ============================================================================
    // Technology Types & Enums
    // ============================================================================

    enum class TechnologyType : uint32_t {
        // Agricultural Technologies (1000-1099)
        THREE_FIELD_SYSTEM = 1001,
        HEAVY_PLOW = 1002,
        HORSE_COLLAR = 1003,
        WINDMILL = 1004,
        WATERMILL = 1005,
        CROP_ROTATION = 1006,
        SELECTIVE_BREEDING = 1007,
        AGRICULTURAL_MANUAL = 1008,
        IRRIGATION_SYSTEMS = 1009,
        NEW_WORLD_CROPS = 1010,

        // Military Technologies (1100-1199)
        CHAINMAIL_ARMOR = 1101,
        PLATE_ARMOR = 1102,
        CROSSBOW = 1103,
        LONGBOW = 1104,
        GUNPOWDER = 1105,
        CANNONS = 1106,
        ARQUEBUS = 1107,
        MUSKET = 1108,
        STAR_FORTRESS = 1109,
        MILITARY_ENGINEERING = 1110,

        // Craft Technologies (1200-1299)
        BLAST_FURNACE = 1201,
        WATER_POWERED_MACHINERY = 1202,
        MECHANICAL_CLOCK = 1203,
        PRINTING_PRESS = 1204,
        DOUBLE_ENTRY_BOOKKEEPING = 1205,
        PAPER_MAKING = 1206,
        GLASS_MAKING = 1207,
        TEXTILE_MACHINERY = 1208,
        ADVANCED_METALLURGY = 1209,
        PRECISION_INSTRUMENTS = 1210,

        // Administrative Technologies (1300-1399)
        WRITTEN_LAW_CODES = 1301,
        BUREAUCRATIC_ADMINISTRATION = 1302,
        CENSUS_TECHNIQUES = 1303,
        TAX_COLLECTION_SYSTEMS = 1304,
        DIPLOMATIC_PROTOCOLS = 1305,
        RECORD_KEEPING = 1306,
        STANDARDIZED_WEIGHTS = 1307,
        POSTAL_SYSTEMS = 1308,
        PROFESSIONAL_ARMY = 1309,
        STATE_MONOPOLIES = 1310,

        // Academic Technologies (1400-1499)
        SCHOLASTIC_METHOD = 1401,
        UNIVERSITY_SYSTEM = 1402,
        VERNACULAR_WRITING = 1403,
        NATURAL_PHILOSOPHY = 1404,
        MATHEMATICAL_NOTATION = 1405,
        EXPERIMENTAL_METHOD = 1406,
        HUMANIST_EDUCATION = 1407,
        SCIENTIFIC_INSTRUMENTS = 1408,
        OPTICAL_DEVICES = 1409,
        CARTOGRAPHY = 1410,

        // Naval Technologies (1500-1599)
        IMPROVED_SHIP_DESIGN = 1501,
        NAVIGATION_INSTRUMENTS = 1502,
        COMPASS_NAVIGATION = 1503,
        NAVAL_ARTILLERY = 1504,
        OCEAN_NAVIGATION = 1505,
        SHIPYARD_TECHNIQUES = 1506,
        MARITIME_LAW = 1507,
        NAVAL_TACTICS = 1508,
        LIGHTHOUSE_SYSTEMS = 1509,
        HARBOR_ENGINEERING = 1510,

        COUNT,
        INVALID = 9999
    };

    enum class TechnologyCategory : uint8_t {
        AGRICULTURAL = 0,
        MILITARY = 1,
        CRAFT = 2,
        ADMINISTRATIVE = 3,
        ACADEMIC = 4,
        NAVAL = 5,
        COUNT
    };

    enum class ResearchState : uint8_t {
        UNKNOWN = 0,
        AVAILABLE = 1,
        RESEARCHING = 2,
        DISCOVERED = 3,
        IMPLEMENTING = 4,
        IMPLEMENTED = 5,
        COUNT
    };

    enum class DiscoveryMethod : uint8_t {
        RESEARCH = 0,
        TRADE = 1,
        DIPLOMACY = 2,
        WARFARE = 3,
        MIGRATION = 4,
        ACCIDENT = 5,
        COUNT
    };

    // ============================================================================
    // Technology Components
    // ============================================================================

    struct TechnologyComponent : public core::ecs::IComponent<TechnologyComponent> {
        // Current research state
        std::unordered_map<TechnologyType, ResearchState> technology_states;
        std::unordered_map<TechnologyType, double> research_progress; // 0.0 to 1.0
        std::unordered_map<TechnologyType, double> implementation_level; // 0.0 to 1.0

        // Research investment
        double monthly_research_budget = 100.0;
        std::unordered_map<TechnologyCategory, double> category_investment; // Investment per category
        TechnologyType current_focus = TechnologyType::INVALID;
        double focus_bonus = 0.5; // 50% bonus to focused research

        // Research infrastructure
        int universities = 0;
        int monasteries = 1;
        int libraries = 0;
        int workshops = 2;
        int scholar_population = 10;

        // Research modifiers
        double base_research_efficiency = 1.0;
        double literacy_bonus = 0.0;
        double trade_network_bonus = 0.0;
        double war_military_bonus = 0.0; // Bonus to military research during war
        double stability_bonus = 0.0;

        // Historical context
        int current_year = 1066;
        bool historical_accuracy_enabled = true;

        TechnologyComponent() = default;
    };

    struct ResearchInvestmentComponent : public core::ecs::IComponent<ResearchInvestmentComponent> {
        // Investment sources
        double player_investment = 0.0;
        double noble_patronage = 0.0;
        double church_funding = 0.0;
        double guild_funding = 0.0;
        double merchant_funding = 0.0;
        double foreign_investment = 0.0;

        // Investment allocation
        std::unordered_map<TechnologyType, double> specific_investments;
        std::unordered_map<TechnologyCategory, double> category_allocations;

        // Investment efficiency
        double investment_efficiency = 1.0;
        double corruption_penalty = 0.0;
        double war_disruption = 0.0;

        ResearchInvestmentComponent() = default;
    };

    struct KnowledgeNetworkComponent : public core::ecs::IComponent<KnowledgeNetworkComponent> {
        // Knowledge connections
        std::unordered_map<types::EntityID, double> knowledge_connections;
        std::vector<types::EntityID> trade_partners;
        std::vector<types::EntityID> diplomatic_contacts;
        std::vector<types::EntityID> scholarly_exchanges;

        // Network properties
        double knowledge_preservation_rate = 0.5;
        double knowledge_transmission_rate = 0.2;
        double cultural_openness = 0.5;
        double innovation_encouragement = 0.5;

        KnowledgeNetworkComponent() = default;
    };

    // ============================================================================
    // Technology Events
    // ============================================================================

    namespace messages {

        struct TechnologyDiscovered {
            types::EntityID province_id{ 0 };
            TechnologyType technology = TechnologyType::INVALID;
            DiscoveryMethod method = DiscoveryMethod::RESEARCH;
            int discovery_year = 1066;
            double research_investment = 0.0;
            std::chrono::system_clock::time_point timestamp;
        };

        struct TechnologyImplemented {
            types::EntityID province_id{ 0 };
            TechnologyType technology = TechnologyType::INVALID;
            double implementation_level = 1.0;
            int implementation_year = 1066;
            std::vector<std::string> effects_applied;
            std::chrono::system_clock::time_point timestamp;
        };

        struct ResearchBreakthrough {
            types::EntityID province_id{ 0 };
            TechnologyCategory category = TechnologyCategory::COUNT;
            double breakthrough_magnitude = 1.0;
            std::vector<TechnologyType> technologies_unlocked;
            std::string breakthrough_description;
            std::chrono::system_clock::time_point timestamp;
        };

        struct KnowledgeTransfer {
            types::EntityID source_province{ 0 };
            types::EntityID target_province{ 0 };
            TechnologyType technology = TechnologyType::INVALID;
            std::string transfer_method; // "trade", "diplomacy", "migration"
            double transfer_rate = 0.0;
            std::chrono::system_clock::time_point timestamp;
        };

        struct ResearchFocusChanged {
            types::EntityID province_id{ 0 };
            TechnologyType old_focus = TechnologyType::INVALID;
            TechnologyType new_focus = TechnologyType::INVALID;
            double investment_level = 0.0;
            std::string change_reason;
            std::chrono::system_clock::time_point timestamp;
        };

    } // namespace messages

    // ============================================================================
    // Technology Database Entry
    // ============================================================================

    struct TechnologyDefinition {
        TechnologyType type = TechnologyType::INVALID;
        TechnologyCategory category = TechnologyCategory::COUNT;
        std::string name;
        std::string description;

        // Research requirements
        double base_research_cost = 1000.0;
        double literacy_requirement = 0.1;
        std::vector<TechnologyType> prerequisites;

        // Historical timing
        int historical_emergence_year = 1066;
        int historical_spread_duration = 50;
        double historical_discovery_chance = 0.01; // 1% per year base

        // Effects (stored as key-value pairs)
        std::unordered_map<std::string, double> effects;

        TechnologyDefinition() = default;
        TechnologyDefinition(TechnologyType t, TechnologyCategory cat, 
                           const std::string& n, const std::string& desc, 
                           int hist_year = 1066);
    };

    // ============================================================================
    // Main Technology System
    // ============================================================================

    class TechnologySystem : public core::threading::ThreadSafeSystem {
    private:
        core::ecs::ComponentAccessManager& m_access_manager;
        core::threading::ThreadSafeMessageBus& m_message_bus;
        
        // Technology database
        std::unordered_map<TechnologyType, TechnologyDefinition> m_technology_database;
        std::unordered_map<TechnologyCategory, std::vector<TechnologyType>> m_technologies_by_category;
        
        // Update timing
        std::chrono::steady_clock::time_point m_last_update;
        double m_update_frequency = 1.0; // 1 update per second (research calculations)
        
        // Current game year
        int m_current_year = 1066;
        
        // Configuration
        double m_global_research_modifier = 1.0;
        double m_global_spread_modifier = 1.0;
        bool m_historical_accuracy = true;

    public:
        explicit TechnologySystem(core::ecs::ComponentAccessManager& access_manager,
            core::threading::ThreadSafeMessageBus& message_bus);
        ~TechnologySystem() override;

        // ThreadSafeSystem interface
        void Initialize() override;
        void Update(float delta_time,
            core::ecs::ComponentAccessManager& access_manager,
            core::threading::ThreadSafeMessageBus& message_bus) override;
        void Shutdown() override;

        std::string GetSystemName() const override { return "TechnologySystem"; }
        core::threading::ThreadingStrategy GetPreferredStrategy() const override;
        bool CanRunInParallel() const override { return true; }
        double GetTargetUpdateRate() const override { return m_update_frequency; }

        // Research management
        bool StartResearch(types::EntityID province_id, TechnologyType technology);
        bool StopResearch(types::EntityID province_id, TechnologyType technology);
        bool SetResearchFocus(types::EntityID province_id, TechnologyType technology);
        bool InvestInResearch(types::EntityID province_id, TechnologyCategory category, double amount);

        // Technology queries
        std::vector<TechnologyType> GetAvailableResearch(types::EntityID province_id) const;
        std::vector<TechnologyType> GetDiscoveredTechnologies(types::EntityID province_id) const;
        std::vector<TechnologyType> GetImplementedTechnologies(types::EntityID province_id) const;
        double GetResearchProgress(types::EntityID province_id, TechnologyType technology) const;
        double GetImplementationLevel(types::EntityID province_id, TechnologyType technology) const;

        // Technology information
        const TechnologyDefinition* GetTechnologyDefinition(TechnologyType technology) const;
        std::vector<TechnologyType> GetTechnologiesInCategory(TechnologyCategory category) const;
        bool HasPrerequisites(types::EntityID province_id, TechnologyType technology) const;
        std::vector<TechnologyType> GetMissingPrerequisites(types::EntityID province_id, TechnologyType technology) const;

        // Integration with other systems
        void SetCurrentYear(int year);
        void OnEconomicCrisis(types::EntityID province_id);
        void OnWarStateChanged(types::EntityID province_id, bool at_war);
        void OnBuildingConstructed(types::EntityID province_id, const std::string& building_type);

        // Configuration
        void SetGlobalResearchModifier(double modifier);
        void SetHistoricalAccuracy(bool enable);

    private:
        // Technology database initialization
        void InitializeTechnologyDatabase();
        void LoadTechnologyDefinitions();
        void SetupTechnologyPrerequisites();

        // Research processing
        void ProcessResearch(types::EntityID province_id);
        void ProcessTechnologyDiscovery(types::EntityID province_id, TechnologyType technology);
        void ProcessTechnologyImplementation(types::EntityID province_id, TechnologyType technology);
        void ProcessKnowledgeTransfer();

        // Research calculations
        double CalculateResearchRate(types::EntityID province_id, TechnologyType technology) const;
        double CalculateDiscoveryChance(types::EntityID province_id, TechnologyType technology) const;
        double CalculateImplementationRate(types::EntityID province_id, TechnologyType technology) const;

        // Historical processing
        void ProcessHistoricalEmergence();
        bool IsHistoricallyAvailable(TechnologyType technology, int year) const;
        double GetHistoricalDiscoveryChance(TechnologyType technology, int year) const;

        // Technology effects
        void ApplyTechnologyEffects(types::EntityID province_id, TechnologyType technology);
        void RemoveTechnologyEffects(types::EntityID province_id, TechnologyType technology);

        // Knowledge network
        void UpdateKnowledgeNetworks();
        void TransferKnowledge(types::EntityID source, types::EntityID target, TechnologyType technology);
        double CalculateKnowledgeTransferRate(types::EntityID source, types::EntityID target, TechnologyType technology) const;

        // Event handlers
        void OnProvinceCreated(const province::messages::ProvinceCreated& message);
        void OnPopulationChanged(const population::messages::PopulationChanged& message);
        void OnTradeRouteEstablished(const trade::messages::TradeRouteEstablished& message);

        // Helper methods
        std::vector<types::EntityID> GetAllProvinces() const;
        bool IsValidProvince(types::EntityID province_id) const;
        void LogTechnologyEvent(types::EntityID province_id, const std::string& message);
        void PublishTechnologyEvent(const messages::TechnologyDiscovered& event);
        void PublishTechnologyEvent(const messages::TechnologyImplemented& event);
        void PublishTechnologyEvent(const messages::ResearchBreakthrough& event);
        void PublishTechnologyEvent(const messages::KnowledgeTransfer& event);
        void PublishTechnologyEvent(const messages::ResearchFocusChanged& event);

        // Integration with other systems
        double GetProvinceLiteracy(types::EntityID province_id) const;
        double GetProvinceStability(types::EntityID province_id) const;
        std::vector<types::EntityID> GetTradePartners(types::EntityID province_id) const;
        bool IsAtWar(types::EntityID province_id) const;
    };

    // ============================================================================
    // Utility Functions
    // ============================================================================

    namespace utils {

        // String conversion utilities
        std::string TechnologyTypeToString(TechnologyType type);
        std::string TechnologyCategoryToString(TechnologyCategory category);
        std::string ResearchStateToString(ResearchState state);
        std::string DiscoveryMethodToString(DiscoveryMethod method);

        // Technology type conversions
        TechnologyType StringToTechnologyType(const std::string& str);
        TechnologyCategory StringToTechnologyCategory(const std::string& str);

        // Factory methods
        TechnologyComponent CreateTechnologyComponent(int starting_year = 1066);
        ResearchInvestmentComponent CreateResearchInvestment(double initial_budget = 100.0);
        KnowledgeNetworkComponent CreateKnowledgeNetwork();

        // Technology database queries
        std::vector<TechnologyType> GetTechnologiesAvailableInYear(int year);
        std::vector<TechnologyType> GetTechnologiesRequiring(TechnologyType prerequisite);
        bool IsValidTechnology(TechnologyType type);

        // Research calculations
        double CalculateOptimalInvestment(TechnologyType technology, double available_budget);
        double EstimateResearchTime(TechnologyType technology, double monthly_investment, double efficiency);
        std::vector<TechnologyType> GetResearchRecommendations(types::EntityID province_id, TechnologyCategory focus);

    } // namespace utils

} // namespace game::technology