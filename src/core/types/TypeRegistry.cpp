// ============================================================================
// Date/Time Created: Sunday, October 26, 2025 - 2:45 PM PST
// Intended Folder Location: src/core/types/TypeRegistry.cpp
// TypeRegistry.cpp - String ↔ Enum Conversion Utilities (CORRECTED)
// Fixed to match actual enum values in game_types.h
// ============================================================================

#include "core/types/game_types.h"
#include "core/threading/ThreadingTypes.h"
#include "game/population/PopulationTypes.h"
#include <unordered_map>
#include <stdexcept>

namespace game::types {

    // ============================================================================
    // Static Member Initialization
    // ============================================================================

    bool TypeRegistry::s_initialized = false;

    std::unordered_map<SystemType, std::string> TypeRegistry::s_system_to_string;
    std::unordered_map<std::string, SystemType> TypeRegistry::s_string_to_system;

    std::unordered_map<DecisionType, std::string> TypeRegistry::s_decision_to_string;
    std::unordered_map<std::string, DecisionType> TypeRegistry::s_string_to_decision;

    std::unordered_map<FunctionType, std::string> TypeRegistry::s_function_to_string;
    std::unordered_map<std::string, FunctionType> TypeRegistry::s_string_to_function;

    std::unordered_map<RegionType, std::string> TypeRegistry::s_region_to_string;
    std::unordered_map<std::string, RegionType> TypeRegistry::s_string_to_region;

    std::unordered_map<EventType, std::string> TypeRegistry::s_event_to_string;
    std::unordered_map<std::string, EventType> TypeRegistry::s_string_to_event;

    std::unordered_map<TechnologyType, std::string> TypeRegistry::s_technology_to_string;
    std::unordered_map<std::string, TechnologyType> TypeRegistry::s_string_to_technology;

    std::unordered_map<::core::threading::ThreadingStrategy, std::string> TypeRegistry::s_threading_strategy_to_string;
    std::unordered_map<std::string, ::core::threading::ThreadingStrategy> TypeRegistry::s_string_to_threading_strategy;

    std::unordered_map<game::population::SocialClass, std::string> TypeRegistry::s_social_class_to_string;
    std::unordered_map<std::string, game::population::SocialClass> TypeRegistry::s_string_to_social_class;

    std::unordered_map<DecisionType, SystemType> TypeRegistry::s_decision_to_system;
    std::unordered_map<SystemType, std::vector<FunctionType>> TypeRegistry::s_system_to_functions;

    // ============================================================================
    // Initialization - Called once to populate all mapping tables
    // ============================================================================

    void TypeRegistry::InitializeMappings() {
        if (s_initialized) return;

        // ========================================
        // System Type Mappings (CORRECTED)
        // ========================================
        s_system_to_string = {
            {SystemType::INVALID, "invalid"},
            {SystemType::ECS_FOUNDATION, "ecs_foundation"},
            {SystemType::MESSAGE_BUS, "message_bus"},
            {SystemType::THREADING, "threading"},
            {SystemType::SAVE_SYSTEM, "save_system"},
            {SystemType::BALANCE_MONITOR, "balance_monitor"},

            {SystemType::ECONOMICS, "economics"},
            {SystemType::MILITARY, "military"},
            {SystemType::DIPLOMACY, "diplomacy"},
            {SystemType::ADMINISTRATION, "administration"},
            {SystemType::POPULATION, "population"},
            {SystemType::CONSTRUCTION, "construction"},
            {SystemType::TECHNOLOGY, "technology"},
            {SystemType::CULTURE, "culture"},
            {SystemType::RELIGION, "religion"},
            {SystemType::ESPIONAGE, "espionage"},

            {SystemType::CHARACTERS, "characters"},
            {SystemType::COURT_INTRIGUE, "court_intrigue"},
            {SystemType::FACTIONS, "factions"},
            {SystemType::SUCCESSION, "succession"},

            {SystemType::TRADE, "trade"},
            {SystemType::NATURAL_EVENTS, "natural_events"},
            {SystemType::CLIMATE, "climate"},
            {SystemType::RESOURCES, "resources"}
        };

        // Create reverse mapping
        for (const auto& [type, str] : s_system_to_string) {
            s_string_to_system[str] = type;
        }

        // ========================================
        // Decision Type Mappings (CORRECTED)
        // ========================================
        s_decision_to_string = {
            {DecisionType::INVALID, "invalid"},

            // Economic decisions
            {DecisionType::ECONOMIC_TAX_RATE, "economic_tax_rate"},
            {DecisionType::ECONOMIC_TRADE_POLICY, "economic_trade_policy"},
            {DecisionType::ECONOMIC_CURRENCY_DEBASEMENT, "economic_currency_debasement"},
            {DecisionType::ECONOMIC_MERCHANT_PRIVILEGES, "economic_merchant_privileges"},
            {DecisionType::ECONOMIC_GUILD_REGULATION, "economic_guild_regulation"},
            {DecisionType::ECONOMIC_INFRASTRUCTURE_INVESTMENT, "economic_infrastructure_investment"},
            {DecisionType::ECONOMIC_DEBT_MANAGEMENT, "economic_debt_management"},

            // Administrative decisions
            {DecisionType::ADMIN_OFFICIAL_APPOINTMENT, "admin_official_appointment"},
            {DecisionType::ADMIN_CORRUPTION_INVESTIGATION, "admin_corruption_investigation"},
            {DecisionType::ADMIN_BUREAUCRACY_REFORM, "admin_bureaucracy_reform"},
            {DecisionType::ADMIN_CENSUS_ORGANIZATION, "admin_census_organization"},
            {DecisionType::ADMIN_LAW_CODIFICATION, "admin_law_codification"},
            {DecisionType::ADMIN_COURT_ESTABLISHMENT, "admin_court_establishment"},
            {DecisionType::ADMIN_PROVINCIAL_AUTONOMY, "admin_provincial_autonomy"},

            // Military decisions
            {DecisionType::MILITARY_RECRUITMENT, "military_recruitment"},
            {DecisionType::MILITARY_UNIT_DEPLOYMENT, "military_unit_deployment"},
            {DecisionType::MILITARY_FORTIFICATION_CONSTRUCTION, "military_fortification_construction"},
            {DecisionType::MILITARY_MERCENARY_HIRING, "military_mercenary_hiring"},
            {DecisionType::MILITARY_NAVAL_EXPANSION, "military_naval_expansion"},
            {DecisionType::MILITARY_SIEGE_TACTICS, "military_siege_tactics"},
            {DecisionType::MILITARY_ARMY_REORGANIZATION, "military_army_reorganization"},

            // Diplomatic decisions
            {DecisionType::DIPLOMACY_ALLIANCE_PROPOSAL, "diplomacy_alliance_proposal"},
            {DecisionType::DIPLOMACY_TRADE_AGREEMENT, "diplomacy_trade_agreement"},
            {DecisionType::DIPLOMACY_MARRIAGE_NEGOTIATION, "diplomacy_marriage_negotiation"},
            {DecisionType::DIPLOMACY_BORDER_SETTLEMENT, "diplomacy_border_settlement"},
            {DecisionType::DIPLOMACY_TRIBUTE_DEMAND, "diplomacy_tribute_demand"},
            {DecisionType::DIPLOMACY_EMBASSY_ESTABLISHMENT, "diplomacy_embassy_establishment"},
            {DecisionType::DIPLOMACY_WAR_DECLARATION, "diplomacy_war_declaration"},

            // Population decisions
            {DecisionType::POPULATION_MIGRATION_POLICY, "population_migration_policy"},
            {DecisionType::POPULATION_RELIGIOUS_TOLERANCE, "population_religious_tolerance"},
            {DecisionType::POPULATION_EDUCATION_FUNDING, "population_education_funding"},
            {DecisionType::POPULATION_SETTLEMENT_ENCOURAGEMENT, "population_settlement_encouragement"},
            {DecisionType::POPULATION_CULTURAL_INTEGRATION, "population_cultural_integration"},
            {DecisionType::POPULATION_LABOR_REGULATION, "population_labor_regulation"},
            {DecisionType::POPULATION_HEALTH_MEASURES, "population_health_measures"},

            // Construction decisions
            {DecisionType::CONSTRUCTION_BUILDING_PROJECT, "construction_building_project"},
            {DecisionType::CONSTRUCTION_ROAD_NETWORK, "construction_road_network"},
            {DecisionType::CONSTRUCTION_HARBOR_EXPANSION, "construction_harbor_expansion"},
            {DecisionType::CONSTRUCTION_CATHEDRAL_BUILDING, "construction_cathedral_building"},
            {DecisionType::CONSTRUCTION_UNIVERSITY_FOUNDING, "construction_university_founding"},
            {DecisionType::CONSTRUCTION_MARKET_ESTABLISHMENT, "construction_market_establishment"},
            {DecisionType::CONSTRUCTION_DEFENSIVE_WORKS, "construction_defensive_works"},

            // Technology decisions
            {DecisionType::TECHNOLOGY_RESEARCH_FUNDING, "technology_research_funding"},
            {DecisionType::TECHNOLOGY_SCHOLAR_PATRONAGE, "technology_scholar_patronage"},
            {DecisionType::TECHNOLOGY_INNOVATION_ENCOURAGEMENT, "technology_innovation_encouragement"},
            {DecisionType::TECHNOLOGY_KNOWLEDGE_ACQUISITION, "technology_knowledge_acquisition"},
            {DecisionType::TECHNOLOGY_CRAFT_GUILD_SUPPORT, "technology_craft_guild_support"},
            {DecisionType::TECHNOLOGY_FOREIGN_EXPERTISE, "technology_foreign_expertise"}
        };

        // Create reverse mapping
        for (const auto& [type, str] : s_decision_to_string) {
            s_string_to_decision[str] = type;
        }

        // ========================================
        // Function Type Mappings (CORRECTED to match game_types.h)
        // ========================================
        s_function_to_string = {
            {FunctionType::INVALID, "invalid"},

            // Economic Functions (100-199)
            {FunctionType::TAX_COLLECTION, "tax_collection"},
            {FunctionType::TRADE_REGULATION, "trade_regulation"},
            {FunctionType::CURRENCY_MANAGEMENT, "currency_management"},
            {FunctionType::DEBT_COLLECTION, "debt_collection"},
            {FunctionType::MARKET_OVERSIGHT, "market_oversight"},
            {FunctionType::GUILD_LICENSING, "guild_licensing"},
            {FunctionType::RESOURCE_ALLOCATION, "resource_allocation"},

            // Administrative Functions (200-299)
            {FunctionType::OFFICIAL_APPOINTMENT, "official_appointment"},
            {FunctionType::CORRUPTION_MONITORING, "corruption_monitoring"},
            {FunctionType::BUREAUCRACY_MANAGEMENT, "bureaucracy_management"},
            {FunctionType::RECORD_KEEPING, "record_keeping"},
            {FunctionType::LAW_ENFORCEMENT, "law_enforcement"},
            {FunctionType::CENSUS_TAKING, "census_taking"},
            {FunctionType::COURT_ADMINISTRATION, "court_administration"},

            // Military Functions (300-399)
            {FunctionType::RECRUITMENT, "recruitment"},
            {FunctionType::UNIT_TRAINING, "unit_training"},
            {FunctionType::DEPLOYMENT_PLANNING, "deployment_planning"},
            {FunctionType::LOGISTICS_MANAGEMENT, "logistics_management"},
            {FunctionType::FORTIFICATION_MAINTENANCE, "fortification_maintenance"},
            {FunctionType::INTELLIGENCE_GATHERING, "intelligence_gathering"},
            {FunctionType::VETERAN_CARE, "veteran_care"},

            // Diplomatic Functions (400-499)
            {FunctionType::ALLIANCE_MAINTENANCE, "alliance_maintenance"},
            {FunctionType::TRADE_NEGOTIATION, "trade_negotiation"},
            {FunctionType::BORDER_MANAGEMENT, "border_management"},
            {FunctionType::EMBASSY_OPERATIONS, "embassy_operations"},
            {FunctionType::INTELLIGENCE_EXCHANGE, "intelligence_exchange"},
            {FunctionType::CULTURAL_EXCHANGE, "cultural_exchange"},
            {FunctionType::CONFLICT_MEDIATION, "conflict_mediation"},

            // Construction Functions (500-599)
            {FunctionType::PROJECT_PLANNING, "project_planning"},
            {FunctionType::RESOURCE_PROCUREMENT, "resource_procurement"},
            {FunctionType::WORKER_COORDINATION, "worker_coordination"},
            {FunctionType::QUALITY_CONTROL, "quality_control"},
            {FunctionType::MAINTENANCE_SCHEDULING, "maintenance_scheduling"},
            {FunctionType::INFRASTRUCTURE_PLANNING, "infrastructure_planning"},
            {FunctionType::URBAN_DEVELOPMENT, "urban_development"},

            // Population Management (600-699)
            {FunctionType::MIGRATION_CONTROL, "migration_control"},
            {FunctionType::CULTURAL_INTEGRATION, "cultural_integration"},
            {FunctionType::RELIGIOUS_AFFAIRS, "religious_affairs"},
            {FunctionType::EDUCATION_OVERSIGHT, "education_oversight"},
            {FunctionType::HEALTH_ADMINISTRATION, "health_administration"},
            {FunctionType::SETTLEMENT_PLANNING, "settlement_planning"},
            {FunctionType::DEMOGRAPHIC_MONITORING, "demographic_monitoring"}
        };

        // Create reverse mapping
        for (const auto& [type, str] : s_function_to_string) {
            s_string_to_function[str] = type;
        }

        // ========================================
        // Region Type Mappings (CORRECTED)
        // ========================================
        s_region_to_string = {
            {RegionType::INVALID, "invalid"},

            // Geographic Regions
            {RegionType::CORE_PROVINCES, "core_provinces"},
            {RegionType::BORDER_PROVINCES, "border_provinces"},
            {RegionType::DISTANT_PROVINCES, "distant_provinces"},
            {RegionType::OVERSEAS_TERRITORIES, "overseas_territories"},
            {RegionType::VASSAL_LANDS, "vassal_lands"},
            {RegionType::OCCUPIED_TERRITORIES, "occupied_territories"},

            // Cultural Regions
            {RegionType::HOME_CULTURE, "home_culture"},
            {RegionType::ACCEPTED_CULTURES, "accepted_cultures"},
            {RegionType::FOREIGN_CULTURES, "foreign_cultures"},
            {RegionType::HOSTILE_CULTURES, "hostile_cultures"},

            // Administrative Regions
            {RegionType::CAPITAL_REGION, "capital_region"},
            {RegionType::DUCAL_REGIONS, "ducal_regions"},
            {RegionType::COUNTY_REGIONS, "county_regions"},
            {RegionType::FRONTIER_REGIONS, "frontier_regions"},
            {RegionType::TRADE_ZONES, "trade_zones"},
            {RegionType::MILITARY_DISTRICTS, "military_districts"}
        };

        // Create reverse mapping
        for (const auto& [type, str] : s_region_to_string) {
            s_string_to_region[str] = type;
        }

        // ========================================
        // Event Type Mappings (CORRECTED)
        // ========================================
        s_event_to_string = {
            {EventType::INVALID, "invalid"},

            // Economic Events (100-199)
            {EventType::ECONOMIC_CRISIS, "economic_crisis"},
            {EventType::TRADE_DISRUPTION, "trade_disruption"},
            {EventType::CURRENCY_FLUCTUATION, "currency_fluctuation"},
            {EventType::MARKET_CRASH, "market_crash"},
            {EventType::RESOURCE_DISCOVERY, "resource_discovery"},
            {EventType::HARVEST_FAILURE, "harvest_failure"},
            {EventType::COMMERCIAL_BOOM, "commercial_boom"},

            // Political Events (200-299)
            {EventType::SUCCESSION_CRISIS, "succession_crisis"},
            {EventType::NOBLE_REBELLION, "noble_rebellion"},
            {EventType::FACTION_DEMANDS, "faction_demands"},
            {EventType::COURT_SCANDAL, "court_scandal"},
            {EventType::DIPLOMATIC_INCIDENT, "diplomatic_incident"},
            {EventType::CIVIL_UNREST, "civil_unrest"},
            {EventType::ADMINISTRATIVE_CRISIS, "administrative_crisis"},

            // Military Events (300-399)
            {EventType::ENEMY_INVASION, "enemy_invasion"},
            {EventType::MILITARY_MUTINY, "military_mutiny"},
            {EventType::SIEGE_WARFARE, "siege_warfare"},
            {EventType::NAVAL_BATTLE, "naval_battle"},
            {EventType::MERCENARY_DESERTION, "mercenary_desertion"},
            {EventType::FORTIFICATION_BREACH, "fortification_breach"},
            {EventType::STRATEGIC_VICTORY, "strategic_victory"},

            // Social Events (400-499)
            {EventType::POPULATION_GROWTH, "population_growth"},
            {EventType::CULTURAL_SHIFT, "cultural_shift"},
            {EventType::RELIGIOUS_MOVEMENT, "religious_movement"},
            {EventType::PLAGUE_OUTBREAK, "plague_outbreak"},
            {EventType::MIGRATION_WAVE, "migration_wave"},
            {EventType::TECHNOLOGICAL_BREAKTHROUGH, "technological_breakthrough"},
            {EventType::EDUCATIONAL_ADVANCEMENT, "educational_advancement"},

            // Natural Events (500-599)
            {EventType::NATURAL_DISASTER, "natural_disaster"},
            {EventType::CLIMATE_CHANGE, "climate_change"},
            {EventType::RESOURCE_DEPLETION, "resource_depletion"},
            {EventType::GEOLOGICAL_EVENT, "geological_event"},
            {EventType::WEATHER_PATTERN, "weather_pattern"},
            {EventType::ECOLOGICAL_SHIFT, "ecological_shift"},
            {EventType::ASTRONOMICAL_EVENT, "astronomical_event"},

            // Character Events (600-699)
            {EventType::CHARACTER_DEATH, "character_death"},
            {EventType::CHARACTER_MARRIAGE, "character_marriage"},
            {EventType::CHARACTER_BIRTH, "character_birth"},
            {EventType::CHARACTER_COMING_OF_AGE, "character_coming_of_age"},
            {EventType::CHARACTER_SKILL_DEVELOPMENT, "character_skill_development"},
            {EventType::CHARACTER_RELATIONSHIP_CHANGE, "character_relationship_change"},
            {EventType::CHARACTER_ACHIEVEMENT, "character_achievement"}
        };

        // Create reverse mapping
        for (const auto& [type, str] : s_event_to_string) {
            s_string_to_event[str] = type;
        }

        // ========================================
        // Technology Type Mappings (CORRECTED)
        // ========================================
        s_technology_to_string = {
            {TechnologyType::INVALID, "invalid"},

            // Military Technologies (100-199)
            {TechnologyType::HEAVY_CAVALRY, "heavy_cavalry"},
            {TechnologyType::CROSSBOW_TACTICS, "crossbow_tactics"},
            {TechnologyType::SIEGE_ENGINES, "siege_engines"},
            {TechnologyType::PLATE_ARMOR, "plate_armor"},
            {TechnologyType::GUNPOWDER_WEAPONS, "gunpowder_weapons"},
            {TechnologyType::FORTIFICATION_DESIGN, "fortification_design"},
            {TechnologyType::NAVAL_ARTILLERY, "naval_artillery"},

            // Agricultural Technologies (200-299)
            {TechnologyType::THREE_FIELD_SYSTEM, "three_field_system"},
            {TechnologyType::HEAVY_PLOW, "heavy_plow"},
            {TechnologyType::WINDMILLS, "windmills"},
            {TechnologyType::CROP_ROTATION, "crop_rotation"},
            {TechnologyType::SELECTIVE_BREEDING, "selective_breeding"},
            {TechnologyType::AGRICULTURAL_TOOLS, "agricultural_tools"},
            {TechnologyType::IRRIGATION_SYSTEMS, "irrigation_systems"},

            // Craft Technologies (300-399)
            {TechnologyType::IMPROVED_METALLURGY, "improved_metallurgy"},
            {TechnologyType::TEXTILE_PRODUCTION, "textile_production"},
            {TechnologyType::PRECISION_TOOLS, "precision_tools"},
            {TechnologyType::GLASSMAKING, "glassmaking"},
            {TechnologyType::PRINTING_PRESS, "printing_press"},
            {TechnologyType::MECHANICAL_CLOCKS, "mechanical_clocks"},
            {TechnologyType::OPTICS, "optics"},

            // Administrative Technologies (400-499)
            {TechnologyType::DOUBLE_ENTRY_BOOKKEEPING, "double_entry_bookkeeping"},
            {TechnologyType::BUREAUCRATIC_SYSTEMS, "bureaucratic_systems"},
            {TechnologyType::LEGAL_CODIFICATION, "legal_codification"},
            {TechnologyType::POSTAL_SYSTEMS, "postal_systems"},
            {TechnologyType::CENSUS_TECHNIQUES, "census_techniques"},
            {TechnologyType::DIPLOMATIC_PROTOCOLS, "diplomatic_protocols"},
            {TechnologyType::TAXATION_METHODS, "taxation_methods"}
        };

        // Create reverse mapping
        for (const auto& [type, str] : s_technology_to_string) {
            s_string_to_technology[str] = type;
        }

        // ========================================
        // Threading Strategy Mappings
        // ========================================
        s_threading_strategy_to_string = {
            {::core::threading::ThreadingStrategy::MAIN_THREAD, "main_thread"},
            {::core::threading::ThreadingStrategy::THREAD_POOL, "thread_pool"},
            {::core::threading::ThreadingStrategy::DEDICATED_THREAD, "dedicated_thread"},
            {::core::threading::ThreadingStrategy::BACKGROUND_THREAD, "background_thread"},
            {::core::threading::ThreadingStrategy::HYBRID, "hybrid"}
        };

        // Create reverse mapping
        for (const auto& [type, str] : s_threading_strategy_to_string) {
            s_string_to_threading_strategy[str] = type;
        }

        // ========================================
        // Social Class Mappings
        // ========================================
        s_social_class_to_string = {
            {game::population::SocialClass::NOBILITY, "nobility"},
            {game::population::SocialClass::FREE_PEASANTS, "free_peasants"},
            {game::population::SocialClass::SERFS, "serfs"},
            {game::population::SocialClass::CLERGY, "clergy"},
            {game::population::SocialClass::MERCHANTS, "merchants"},
            {game::population::SocialClass::ARTISANS, "artisans"},
            {game::population::SocialClass::URBAN_POOR, "urban_poor"}
        };

        // Create reverse mapping
        for (const auto& [type, str] : s_social_class_to_string) {
            s_string_to_social_class[str] = type;
        }

        // ========================================
        // Decision-to-System Mappings
        // ========================================
        s_decision_to_system = {
            // Economic decisions
            {DecisionType::ECONOMIC_TAX_RATE, SystemType::ECONOMICS},
            {DecisionType::ECONOMIC_TRADE_POLICY, SystemType::ECONOMICS},
            {DecisionType::ECONOMIC_CURRENCY_DEBASEMENT, SystemType::ECONOMICS},
            {DecisionType::ECONOMIC_MERCHANT_PRIVILEGES, SystemType::ECONOMICS},
            {DecisionType::ECONOMIC_GUILD_REGULATION, SystemType::ECONOMICS},
            {DecisionType::ECONOMIC_INFRASTRUCTURE_INVESTMENT, SystemType::ECONOMICS},
            {DecisionType::ECONOMIC_DEBT_MANAGEMENT, SystemType::ECONOMICS},

            // Administrative decisions
            {DecisionType::ADMIN_OFFICIAL_APPOINTMENT, SystemType::ADMINISTRATION},
            {DecisionType::ADMIN_CORRUPTION_INVESTIGATION, SystemType::ADMINISTRATION},
            {DecisionType::ADMIN_BUREAUCRACY_REFORM, SystemType::ADMINISTRATION},
            {DecisionType::ADMIN_CENSUS_ORGANIZATION, SystemType::ADMINISTRATION},
            {DecisionType::ADMIN_LAW_CODIFICATION, SystemType::ADMINISTRATION},
            {DecisionType::ADMIN_COURT_ESTABLISHMENT, SystemType::ADMINISTRATION},
            {DecisionType::ADMIN_PROVINCIAL_AUTONOMY, SystemType::ADMINISTRATION},

            // Military decisions
            {DecisionType::MILITARY_RECRUITMENT, SystemType::MILITARY},
            {DecisionType::MILITARY_UNIT_DEPLOYMENT, SystemType::MILITARY},
            {DecisionType::MILITARY_FORTIFICATION_CONSTRUCTION, SystemType::MILITARY},
            {DecisionType::MILITARY_MERCENARY_HIRING, SystemType::MILITARY},
            {DecisionType::MILITARY_NAVAL_EXPANSION, SystemType::MILITARY},
            {DecisionType::MILITARY_SIEGE_TACTICS, SystemType::MILITARY},
            {DecisionType::MILITARY_ARMY_REORGANIZATION, SystemType::MILITARY},

            // Diplomatic decisions
            {DecisionType::DIPLOMACY_ALLIANCE_PROPOSAL, SystemType::DIPLOMACY},
            {DecisionType::DIPLOMACY_TRADE_AGREEMENT, SystemType::DIPLOMACY},
            {DecisionType::DIPLOMACY_MARRIAGE_NEGOTIATION, SystemType::DIPLOMACY},
            {DecisionType::DIPLOMACY_BORDER_SETTLEMENT, SystemType::DIPLOMACY},
            {DecisionType::DIPLOMACY_TRIBUTE_DEMAND, SystemType::DIPLOMACY},
            {DecisionType::DIPLOMACY_EMBASSY_ESTABLISHMENT, SystemType::DIPLOMACY},
            {DecisionType::DIPLOMACY_WAR_DECLARATION, SystemType::DIPLOMACY},

            // Population decisions
            {DecisionType::POPULATION_MIGRATION_POLICY, SystemType::POPULATION},
            {DecisionType::POPULATION_RELIGIOUS_TOLERANCE, SystemType::POPULATION},
            {DecisionType::POPULATION_EDUCATION_FUNDING, SystemType::POPULATION},
            {DecisionType::POPULATION_SETTLEMENT_ENCOURAGEMENT, SystemType::POPULATION},
            {DecisionType::POPULATION_CULTURAL_INTEGRATION, SystemType::POPULATION},
            {DecisionType::POPULATION_LABOR_REGULATION, SystemType::POPULATION},
            {DecisionType::POPULATION_HEALTH_MEASURES, SystemType::POPULATION},

            // Construction decisions
            {DecisionType::CONSTRUCTION_BUILDING_PROJECT, SystemType::CONSTRUCTION},
            {DecisionType::CONSTRUCTION_ROAD_NETWORK, SystemType::CONSTRUCTION},
            {DecisionType::CONSTRUCTION_HARBOR_EXPANSION, SystemType::CONSTRUCTION},
            {DecisionType::CONSTRUCTION_CATHEDRAL_BUILDING, SystemType::CONSTRUCTION},
            {DecisionType::CONSTRUCTION_UNIVERSITY_FOUNDING, SystemType::CONSTRUCTION},
            {DecisionType::CONSTRUCTION_MARKET_ESTABLISHMENT, SystemType::CONSTRUCTION},
            {DecisionType::CONSTRUCTION_DEFENSIVE_WORKS, SystemType::CONSTRUCTION},

            // Technology decisions
            {DecisionType::TECHNOLOGY_RESEARCH_FUNDING, SystemType::TECHNOLOGY},
            {DecisionType::TECHNOLOGY_SCHOLAR_PATRONAGE, SystemType::TECHNOLOGY},
            {DecisionType::TECHNOLOGY_INNOVATION_ENCOURAGEMENT, SystemType::TECHNOLOGY},
            {DecisionType::TECHNOLOGY_KNOWLEDGE_ACQUISITION, SystemType::TECHNOLOGY},
            {DecisionType::TECHNOLOGY_CRAFT_GUILD_SUPPORT, SystemType::TECHNOLOGY},
            {DecisionType::TECHNOLOGY_FOREIGN_EXPERTISE, SystemType::TECHNOLOGY}
        };

        // ========================================
        // System-to-Functions Mappings
        // ========================================
        s_system_to_functions = {
            {SystemType::ECONOMICS, {
                FunctionType::TAX_COLLECTION,
                FunctionType::TRADE_REGULATION,
                FunctionType::CURRENCY_MANAGEMENT,
                FunctionType::DEBT_COLLECTION,
                FunctionType::MARKET_OVERSIGHT,
                FunctionType::GUILD_LICENSING,
                FunctionType::RESOURCE_ALLOCATION
            }},
            {SystemType::ADMINISTRATION, {
                FunctionType::OFFICIAL_APPOINTMENT,
                FunctionType::CORRUPTION_MONITORING,
                FunctionType::BUREAUCRACY_MANAGEMENT,
                FunctionType::RECORD_KEEPING,
                FunctionType::LAW_ENFORCEMENT,
                FunctionType::CENSUS_TAKING,
                FunctionType::COURT_ADMINISTRATION
            }},
            {SystemType::MILITARY, {
                FunctionType::RECRUITMENT,
                FunctionType::UNIT_TRAINING,
                FunctionType::DEPLOYMENT_PLANNING,
                FunctionType::LOGISTICS_MANAGEMENT,
                FunctionType::FORTIFICATION_MAINTENANCE,
                FunctionType::INTELLIGENCE_GATHERING,
                FunctionType::VETERAN_CARE
            }},
            {SystemType::DIPLOMACY, {
                FunctionType::ALLIANCE_MAINTENANCE,
                FunctionType::TRADE_NEGOTIATION,
                FunctionType::BORDER_MANAGEMENT,
                FunctionType::EMBASSY_OPERATIONS,
                FunctionType::INTELLIGENCE_EXCHANGE,
                FunctionType::CULTURAL_EXCHANGE,
                FunctionType::CONFLICT_MEDIATION
            }},
            {SystemType::CONSTRUCTION, {
                FunctionType::PROJECT_PLANNING,
                FunctionType::RESOURCE_PROCUREMENT,
                FunctionType::WORKER_COORDINATION,
                FunctionType::QUALITY_CONTROL,
                FunctionType::MAINTENANCE_SCHEDULING,
                FunctionType::INFRASTRUCTURE_PLANNING,
                FunctionType::URBAN_DEVELOPMENT
            }},
            {SystemType::POPULATION, {
                FunctionType::MIGRATION_CONTROL,
                FunctionType::CULTURAL_INTEGRATION,
                FunctionType::RELIGIOUS_AFFAIRS,
                FunctionType::EDUCATION_OVERSIGHT,
                FunctionType::HEALTH_ADMINISTRATION,
                FunctionType::SETTLEMENT_PLANNING,
                FunctionType::DEMOGRAPHIC_MONITORING
            }}
        };

        s_initialized = true;
    }

    // ============================================================================
    // Public Conversion Methods
    // ============================================================================

    std::string TypeRegistry::SystemTypeToString(SystemType type) {
        InitializeMappings();
        auto it = s_system_to_string.find(type);
        return (it != s_system_to_string.end()) ? it->second : "unknown";
    }

    SystemType TypeRegistry::StringToSystemType(const std::string& str) {
        InitializeMappings();
        auto it = s_string_to_system.find(str);
        return (it != s_string_to_system.end()) ? it->second : SystemType::INVALID;
    }

    std::string TypeRegistry::DecisionTypeToString(DecisionType type) {
        InitializeMappings();
        auto it = s_decision_to_string.find(type);
        return (it != s_decision_to_string.end()) ? it->second : "unknown";
    }

    DecisionType TypeRegistry::StringToDecisionType(const std::string& str) {
        InitializeMappings();
        auto it = s_string_to_decision.find(str);
        return (it != s_string_to_decision.end()) ? it->second : DecisionType::INVALID;
    }

    std::string TypeRegistry::FunctionTypeToString(FunctionType type) {
        InitializeMappings();
        auto it = s_function_to_string.find(type);
        return (it != s_function_to_string.end()) ? it->second : "unknown";
    }

    FunctionType TypeRegistry::StringToFunctionType(const std::string& str) {
        InitializeMappings();
        auto it = s_string_to_function.find(str);
        return (it != s_string_to_function.end()) ? it->second : FunctionType::INVALID;
    }

    std::string TypeRegistry::RegionTypeToString(RegionType type) {
        InitializeMappings();
        auto it = s_region_to_string.find(type);
        return (it != s_region_to_string.end()) ? it->second : "unknown";
    }

    RegionType TypeRegistry::StringToRegionType(const std::string& str) {
        InitializeMappings();
        auto it = s_string_to_region.find(str);
        return (it != s_string_to_region.end()) ? it->second : RegionType::INVALID;
    }

    std::string TypeRegistry::EventTypeToString(EventType type) {
        InitializeMappings();
        auto it = s_event_to_string.find(type);
        return (it != s_event_to_string.end()) ? it->second : "unknown";
    }

    EventType TypeRegistry::StringToEventType(const std::string& str) {
        InitializeMappings();
        auto it = s_string_to_event.find(str);
        return (it != s_string_to_event.end()) ? it->second : EventType::INVALID;
    }

    std::string TypeRegistry::TechnologyTypeToString(TechnologyType type) {
        InitializeMappings();
        auto it = s_technology_to_string.find(type);
        return (it != s_technology_to_string.end()) ? it->second : "unknown";
    }

    TechnologyType TypeRegistry::StringToTechnologyType(const std::string& str) {
        InitializeMappings();
        auto it = s_string_to_technology.find(str);
        return (it != s_string_to_technology.end()) ? it->second : TechnologyType::INVALID;
    }

    // ============================================================================
    // Validation Functions
    // ============================================================================

    bool TypeRegistry::IsValidSystemType(SystemType type) {
        return type != SystemType::INVALID && static_cast<int>(type) < static_cast<int>(SystemType::MAX_SYSTEM_TYPE);
    }

    bool TypeRegistry::IsValidDecisionType(DecisionType type) {
        return type != DecisionType::INVALID;
    }

    bool TypeRegistry::IsValidFunctionType(FunctionType type) {
        return type != FunctionType::INVALID && static_cast<int>(type) < static_cast<int>(FunctionType::MAX_FUNCTION_TYPE);
    }

    bool TypeRegistry::IsValidRegionType(RegionType type) {
        return type != RegionType::INVALID && static_cast<int>(type) < static_cast<int>(RegionType::MAX_REGION_TYPE);
    }

    // ============================================================================
    // Category Query Functions
    // ============================================================================

    SystemType TypeRegistry::GetSystemForDecision(DecisionType decision) {
        InitializeMappings();
        auto it = s_decision_to_system.find(decision);
        return (it != s_decision_to_system.end()) ? it->second : SystemType::INVALID;
    }

    std::vector<FunctionType> TypeRegistry::GetFunctionsForSystem(SystemType system) {
        InitializeMappings();
        auto it = s_system_to_functions.find(system);
        return (it != s_system_to_functions.end()) ? it->second : std::vector<FunctionType>{};
    }

    TechnologyCategory TypeRegistry::GetCategoryForTechnology(TechnologyType tech) {
        int tech_value = static_cast<int>(tech);

        if (tech_value >= 100 && tech_value < 200) {
            return TechnologyCategory::MILITARY_TECHNOLOGY;
        }
        else if (tech_value >= 200 && tech_value < 300) {
            return TechnologyCategory::AGRICULTURAL_TECHNIQUES;
        }
        else if (tech_value >= 300 && tech_value < 400) {
            return TechnologyCategory::CRAFT_KNOWLEDGE;
        }
        else if (tech_value >= 400 && tech_value < 500) {
            return TechnologyCategory::ADMINISTRATIVE_METHODS;
        }
        else {
            return TechnologyCategory::INVALID;
        }
    }

    // ============================================================================
    // Threading Strategy Conversion Methods
    // ============================================================================

    std::string TypeRegistry::ThreadingStrategyToString(::core::threading::ThreadingStrategy type) {
        InitializeMappings();
        
        auto it = s_threading_strategy_to_string.find(type);
        if (it != s_threading_strategy_to_string.end()) {
            return it->second;
        }
        return "unknown";
    }

    ::core::threading::ThreadingStrategy TypeRegistry::StringToThreadingStrategy(const std::string& str) {
        InitializeMappings();
        
        auto it = s_string_to_threading_strategy.find(str);
        if (it != s_string_to_threading_strategy.end()) {
            return it->second;
        }
        return ::core::threading::ThreadingStrategy::MAIN_THREAD;
    }

    // ============================================================================
    // Social Class Conversion Methods
    // ============================================================================

    std::string TypeRegistry::SocialClassToString(game::population::SocialClass type) {
        InitializeMappings();
        
        auto it = s_social_class_to_string.find(type);
        if (it != s_social_class_to_string.end()) {
            return it->second;
        }
        return "unknown";
    }

    game::population::SocialClass TypeRegistry::StringToSocialClass(const std::string& str) {
        InitializeMappings();
        
        auto it = s_string_to_social_class.find(str);
        if (it != s_string_to_social_class.end()) {
            return it->second;
        }
        return game::population::SocialClass::FREE_PEASANTS;
    }

} // namespace game::types
