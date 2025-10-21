// ============================================================================
// TechnologyComponents.h - ECS Components for Technology System
// Created: October 11, 2025 - Technology System ECS Integration
// Location: include/game/technology/TechnologyComponents.h
// ============================================================================

#pragma once

#include "utils/PlatformMacros.h"
#include "core/ECS/IComponent.h"
#include "core/types/game_types.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <chrono>
#include <algorithm>

namespace game::technology {

    using namespace game::core;

    // ============================================================================
    // Technology Enums and Data Structures
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
    // Technology Definition Structure
    // ============================================================================

    struct TechnologyDefinition {
        TechnologyType type = TechnologyType::INVALID;
        TechnologyCategory category = TechnologyCategory::AGRICULTURAL;
        std::string name;
        std::string description;
        
        // Research requirements
        double base_research_cost = 1000.0;
        double literacy_requirement = 0.1;
        std::vector<TechnologyType> prerequisites;
        
        // Historical context
        uint32_t historical_emergence_year = 1066;
        uint32_t historical_spread_duration = 50;
        double historical_discovery_chance = 0.01;
        
        // Technology effects
        std::unordered_map<std::string, double> effects;
        
        TechnologyDefinition() = default;
        TechnologyDefinition(TechnologyType t, TechnologyCategory cat, const std::string& n, 
                           const std::string& desc, uint32_t hist_year);
    };

    // ============================================================================
    // Research Component - Technology research and development
    // ============================================================================

    struct ResearchComponent : public game::core::Component<ResearchComponent> {
        // Research state tracking
        std::unordered_map<TechnologyType, ResearchState> technology_states;
        std::unordered_map<TechnologyType, double> research_progress; // 0.0 to 1.0
        std::unordered_map<TechnologyType, double> implementation_level; // 0.0 to 1.0
        
        // Current research focus
        TechnologyType current_focus = TechnologyType::INVALID;
        double focus_bonus = 0.5; // 50% bonus to focused research
        
        // Research capacity and infrastructure
        uint32_t universities = 0;
        uint32_t monasteries = 1;
        uint32_t libraries = 0;
        uint32_t workshops = 2;
        uint32_t scholar_population = 10;
        
        // Research efficiency modifiers
        double base_research_efficiency = 1.0;
        double literacy_bonus = 0.0;
        double trade_network_bonus = 0.0;
        double stability_bonus = 0.0;
        double war_military_bonus = 0.0; // Bonus to military research during war
        
        // Investment and resources
        double monthly_research_budget = 100.0;
        std::unordered_map<TechnologyCategory, double> category_investment;
        double total_research_investment = 0.0;
        
        // Research specialization
        TechnologyCategory primary_specialization = TechnologyCategory::CRAFT;
        std::vector<TechnologyCategory> secondary_specializations;
        
        std::string GetComponentTypeName() const override {
            return "ResearchComponent";
        }
    };

    // ============================================================================
    // Innovation Component - Innovation, invention, and knowledge creation
    // ============================================================================

    struct InnovationComponent : public game::core::Component<InnovationComponent> {
        // Innovation capacity
        double innovation_rate = 0.1;
        double breakthrough_chance = 0.05;
        double invention_quality = 0.6;
        
        // Innovation sources
        uint32_t inventors = 0;
        uint32_t craftsmen_innovators = 5;
        uint32_t scholar_innovators = 2;
        uint32_t foreign_scholars = 0;
        
        // Innovation environment
        double cultural_openness = 0.5;
        double innovation_encouragement = 0.5;
        double knowledge_preservation_rate = 0.6;
        double experimentation_freedom = 0.4;
        
        // Recent innovations
        std::vector<TechnologyType> recent_discoveries;
        std::vector<std::string> innovation_attempts;
        std::vector<std::string> failed_experiments;
        
        // Innovation modifiers
        double guild_resistance = 0.2; // Craft guilds resist innovation
        double religious_restriction = 0.1; // Religious limitations on research
        double royal_patronage = 0.0; // Royal support for innovation
        double merchant_funding = 0.0; // Merchant investment in innovation
        
        // Innovation specialties
        std::unordered_map<TechnologyCategory, double> innovation_expertise;
        std::vector<std::string> local_innovations; // Unique provincial innovations
        
        std::string GetComponentTypeName() const override {
            return "InnovationComponent";
        }
    };

    // ============================================================================
    // Knowledge Component - Knowledge preservation, transmission, and networks
    // ============================================================================

    struct KnowledgeComponent : public game::core::Component<KnowledgeComponent> {
        // Knowledge infrastructure
        uint32_t manuscripts = 100;
        uint32_t scribes = 5;
        uint32_t translators = 1;
        uint32_t book_production_capacity = 20; // Books per year
        
        // Knowledge preservation
        double knowledge_preservation_quality = 0.5;
        double manuscript_durability = 0.6;
        double translation_accuracy = 0.7;
        double knowledge_loss_rate = 0.02; // Monthly knowledge decay
        
        // Knowledge networks
        std::unordered_map<game::types::EntityID, double> knowledge_connections;
        std::vector<game::types::EntityID> scholarly_exchanges;
        std::vector<game::types::EntityID> trade_knowledge_routes;
        std::vector<game::types::EntityID> diplomatic_knowledge_sharing;
        
        // Knowledge categories
        std::unordered_map<TechnologyCategory, double> knowledge_depth;
        std::unordered_map<TechnologyType, double> specific_knowledge;
        
        // Knowledge transmission
        double knowledge_transmission_rate = 0.2;
        double cultural_knowledge_absorption = 0.3;
        double foreign_knowledge_acceptance = 0.4;
        
        // Language and literacy
        std::vector<std::string> known_languages;
        double literacy_rate = 0.15;
        double scholarly_literacy_rate = 0.8;
        
        // Knowledge events
        std::vector<std::string> knowledge_acquisitions;
        std::vector<std::string> knowledge_losses;
        std::vector<std::string> translation_projects;
        
        std::string GetComponentTypeName() const override {
            return "KnowledgeComponent";
        }
    };

    // ============================================================================
    // Technology Events Component - Technology-related events and historical tracking
    // ============================================================================

    struct TechnologyEventsComponent : public game::core::Component<TechnologyEventsComponent> {
        // Discovery events
        std::vector<std::string> recent_discoveries;
        std::vector<std::string> research_breakthroughs;
        std::vector<std::string> innovation_successes;
        
        // Implementation events
        std::vector<std::string> technology_adoptions;
        std::vector<std::string> implementation_challenges;
        std::vector<std::string> technology_improvements;
        
        // Knowledge transfer events
        std::vector<std::string> knowledge_acquisitions;
        std::vector<std::string> scholarly_exchanges;
        std::vector<std::string> trade_knowledge_transfers;
        
        // Research setbacks
        std::vector<std::string> research_failures;
        std::vector<std::string> knowledge_losses;
        std::vector<std::string> innovation_resistance;
        
        // Historical tracking
        std::unordered_map<TechnologyType, std::chrono::system_clock::time_point> discovery_dates;
        std::unordered_map<TechnologyType, DiscoveryMethod> discovery_methods;
        std::unordered_map<TechnologyType, double> discovery_investments;
        
        // Event frequency and timing
        double event_frequency_modifier = 1.0;
        uint32_t months_since_last_discovery = 0;
        uint32_t months_since_last_innovation = 0;
        uint32_t months_since_last_breakthrough = 0;
        
        // Technology reputation and prestige
        double technological_reputation = 0.5;
        double innovation_prestige = 0.4;
        double scholarly_recognition = 0.3;
        
        // Research progress tracking
        std::unordered_map<TechnologyType, double> monthly_progress_history;
        std::vector<std::string> active_research_projects;
        
        // Maximum history tracking
        uint32_t max_history_size = 100;
        
        std::string GetComponentTypeName() const override {
            return "TechnologyEventsComponent";
        }
    };

    // ============================================================================
    // Technology Event Structures
    // ============================================================================

    struct TechnologyDiscoveryEvent {
        uint32_t event_id = 0;
        TechnologyType technology = TechnologyType::INVALID;
        DiscoveryMethod method = DiscoveryMethod::RESEARCH;
        
        // Discovery details
        game::types::EntityID discovering_province = 0;
        std::string discoverer_name;
        double research_investment = 0.0;
        uint32_t discovery_year = 1066;
        
        // Discovery circumstances
        std::string discovery_description;
        std::vector<std::string> contributing_factors;
        bool was_accidental = false;
        bool was_collaborative = false;
        
        // Effects and implications
        std::vector<std::string> immediate_effects;
        std::vector<std::string> potential_applications;
        double economic_impact_estimate = 0.0;
        double military_impact_estimate = 0.0;
        
        // Timing
        std::chrono::system_clock::time_point discovery_date;
        
        static std::string GetTypeName() { return "TechnologyDiscoveryEvent"; }
    };

    struct ResearchBreakthroughEvent {
        uint32_t event_id = 0;
        TechnologyCategory category = TechnologyCategory::CRAFT;
        double breakthrough_magnitude = 1.0;
        
        // Breakthrough details
        game::types::EntityID province_id = 0;
        std::string breakthrough_description;
        std::vector<TechnologyType> technologies_unlocked;
        std::vector<TechnologyType> technologies_accelerated;
        
        // Research context
        double total_investment = 0.0;
        std::vector<std::string> research_contributors;
        std::string breakthrough_method; // "systematic", "accidental", "collaborative"
        
        // Impact assessment
        double research_efficiency_boost = 0.0;
        double category_progress_boost = 0.0;
        std::vector<std::string> cascade_effects;
        
        // Timing
        std::chrono::system_clock::time_point breakthrough_date;
        
        static std::string GetTypeName() { return "ResearchBreakthroughEvent"; }
    };

    struct KnowledgeTransferEvent {
        uint32_t event_id = 0;
        game::types::EntityID source_province = 0;
        game::types::EntityID target_province = 0;
        
        // Transfer details
        TechnologyType technology = TechnologyType::INVALID;
        std::string transfer_method; // "trade", "diplomacy", "migration", "espionage"
        double transfer_completeness = 1.0;
        double transfer_accuracy = 0.9;
        
        // Transfer context
        std::string transfer_description;
        std::vector<std::string> transfer_facilitators;
        double transfer_cost = 0.0;
        double transfer_time_months = 1.0;
        
        // Transfer effects
        double knowledge_gained = 0.0;
        std::vector<std::string> adaptation_challenges;
        std::vector<std::string> local_modifications;
        
        // Timing
        std::chrono::system_clock::time_point transfer_date;
        
        static std::string GetTypeName() { return "KnowledgeTransferEvent"; }
    };

    // ============================================================================
    // Utility Functions for Component Creation
    // ============================================================================

    namespace utils {

        // Component factory functions
        inline ResearchComponent CreateTechnologyComponent(int starting_year = 1066) {
            ResearchComponent component;
            component.base_research_efficiency = 1.0;
            component.monthly_research_budget = 100.0;
            
            // Initialize category investments equally
            double per_category = component.monthly_research_budget / static_cast<int>(TechnologyCategory::COUNT);
            for (int i = 0; i < static_cast<int>(TechnologyCategory::COUNT); ++i) {
                auto category = static_cast<TechnologyCategory>(i);
                component.category_investment[category] = per_category;
            }
            
            // Set reasonable defaults
            component.scholar_population = 10;
            component.workshops = 2;
            component.monasteries = 1;
            
            return component;
        }

        inline InnovationComponent CreateInnovationComponent(double initial_rate = 0.1) {
            InnovationComponent component;
            component.innovation_rate = initial_rate;
            component.breakthrough_chance = 0.05;
            component.invention_quality = 0.6;
            component.craftsmen_innovators = 5;
            component.scholar_innovators = 2;
            component.cultural_openness = 0.5;
            component.innovation_encouragement = 0.5;
            
            // Initialize innovation expertise
            for (int i = 0; i < static_cast<int>(TechnologyCategory::COUNT); ++i) {
                auto category = static_cast<TechnologyCategory>(i);
                component.innovation_expertise[category] = 0.3; // Base expertise
            }
            
            return component;
        }

        inline KnowledgeComponent CreateKnowledgeNetwork() {
            KnowledgeComponent component;
            component.manuscripts = 100;
            component.scribes = 5;
            component.translators = 1;
            component.book_production_capacity = 20;
            component.knowledge_preservation_quality = 0.5;
            component.manuscript_durability = 0.6;
            component.translation_accuracy = 0.7;
            component.knowledge_loss_rate = 0.02;
            component.knowledge_transmission_rate = 0.2;
            component.literacy_rate = 0.15;
            component.scholarly_literacy_rate = 0.8;
            
            // Initialize basic language support
            component.known_languages.push_back("Latin");
            component.known_languages.push_back("Local");
            
            return component;
        }

        inline TechnologyEventsComponent CreateTechnologyEvents(size_t max_history = 100) {
            TechnologyEventsComponent component;
            component.max_history_size = static_cast<uint32_t>(max_history);
            component.event_frequency_modifier = 1.0;
            component.technological_reputation = 0.5;
            component.innovation_prestige = 0.4;
            component.scholarly_recognition = 0.3;
            
            return component;
        }

    } // namespace utils

} // namespace game::technology