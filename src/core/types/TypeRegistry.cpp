// ============================================================================
// TypeRegistry.cpp - Implementation of String ? Enum Conversion Utilities
// Eliminates all string parsing in favor of type-safe enum operations
// ============================================================================

#include "game_types.h"
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
        // System Type Mappings
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

            {SystemType::TRADE_ROUTES, "trade_routes"},
            {SystemType::NATURAL_EVENTS, "natural_events"},
            {SystemType::CLIMATE, "climate"},
            {SystemType::RESOURCES, "resources"}
        };

        // Create reverse mapping
        for (const auto& [type, str] : s_system_to_string) {
            s_string_to_system[str] = type;
        }

        // ========================================
        // Decision Type Mappings
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
        // Function Type Mappings
        // ========================================
        s_function_to_string = {
            {FunctionType::INVALID, "invalid"},

            {FunctionType::TAX_COLLECTION, "tax_collection"},
            {FunctionType::TAX_POLICY, "tax_policy"},
            {FunctionType::TRADE_ADMINISTRATION, "trade_administration"},
            {FunctionType::TRADE_AGREEMENTS, "trade_agreements"},
            {FunctionType::CURRENCY_MANAGEMENT, "currency_management"},
            {FunctionType::DEBT_ADMINISTRATION, "debt_administration"},

            {FunctionType::OFFICIAL_APPOINTMENTS, "official_appointments"},
            {FunctionType::CORRUPTION_HANDLING, "corruption_handling"},
            {FunctionType::BUREAUCRACY_MANAGEMENT, "bureaucracy_management"},
            {FunctionType::LAW_ENFORCEMENT, "law_enforcement"},
            {FunctionType::PROVINCIAL_ADMINISTRATION, "provincial_administration"},
            {FunctionType::CENSUS_ADMINISTRATION, "census_administration"},

            {FunctionType::RECRUITMENT, "recruitment"},
            {FunctionType::UNIT_MANAGEMENT, "unit_management"},
            {FunctionType::FORTIFICATION_PLANNING, "fortification_planning"},
            {FunctionType::MILITARY_LOGISTICS, "military_logistics"},
            {FunctionType::NAVAL_OPERATIONS, "naval_operations"},
            {FunctionType::SIEGE_WARFARE, "siege_warfare"},

            {FunctionType::ALLIANCE_NEGOTIATION, "alliance_negotiation"},
            {FunctionType::TREATY_NEGOTIATION, "treaty_negotiation"},
            {FunctionType::MARRIAGE_DIPLOMACY, "marriage_diplomacy"},
            {FunctionType::BORDER_NEGOTIATION, "border_negotiation"},
            {FunctionType::TRIBUTE_NEGOTIATION, "tribute_negotiation"},
            {FunctionType::EMBASSY_MANAGEMENT, "embassy_management"},

            {FunctionType::POPULATION_MANAGEMENT, "population_management"},
            {FunctionType::MIGRATION_POLICY, "migration_policy"},
            {FunctionType::RELIGIOUS_POLICY, "religious_policy"},
            {FunctionType::EDUCATION_POLICY, "education_policy"},
            {FunctionType::CULTURAL_POLICY, "cultural_policy"},
            {FunctionType::LABOR_POLICY, "labor_policy"},

            {FunctionType::BUILDING_CONSTRUCTION, "building_construction"},
            {FunctionType::INFRASTRUCTURE_DEVELOPMENT, "infrastructure_development"},
            {FunctionType::URBAN_PLANNING, "urban_planning"},
            {FunctionType::MAINTENANCE, "maintenance"},
            {FunctionType::PROJECT_MANAGEMENT, "project_management"},

            {FunctionType::RESEARCH_ADMINISTRATION, "research_administration"},
            {FunctionType::INNOVATION_POLICY, "innovation_policy"},
            {FunctionType::KNOWLEDGE_ACQUISITION, "knowledge_acquisition"},
            {FunctionType::CRAFT_REGULATION, "craft_regulation"},
            {FunctionType::TECHNICAL_EDUCATION, "technical_education"}
        };

        // Create reverse mapping
        for (const auto& [type, str] : s_function_to_string) {
            s_string_to_function[str] = type;
        }

        // ========================================
        // Decision ? System Mappings
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

            // And so on for all decision types...
        };

        // ========================================
        // System ? Functions Mappings
        // ========================================
        s_system_to_functions = {
            {SystemType::ECONOMICS, {
                FunctionType::TAX_COLLECTION, FunctionType::TAX_POLICY,
                FunctionType::TRADE_ADMINISTRATION, FunctionType::TRADE_AGREEMENTS,
                FunctionType::CURRENCY_MANAGEMENT, FunctionType::DEBT_ADMINISTRATION
            }},
            {SystemType::ADMINISTRATION, {
                FunctionType::OFFICIAL_APPOINTMENTS, FunctionType::CORRUPTION_HANDLING,
                FunctionType::BUREAUCRACY_MANAGEMENT, FunctionType::LAW_ENFORCEMENT,
                FunctionType::PROVINCIAL_ADMINISTRATION, FunctionType::CENSUS_ADMINISTRATION
            }},
            {SystemType::MILITARY, {
                FunctionType::RECRUITMENT, FunctionType::UNIT_MANAGEMENT,
                FunctionType::FORTIFICATION_PLANNING, FunctionType::MILITARY_LOGISTICS,
                FunctionType::NAVAL_OPERATIONS, FunctionType::SIEGE_WARFARE
            }},
            {SystemType::DIPLOMACY, {
                FunctionType::ALLIANCE_NEGOTIATION, FunctionType::TREATY_NEGOTIATION,
                FunctionType::MARRIAGE_DIPLOMACY, FunctionType::BORDER_NEGOTIATION,
                FunctionType::TRIBUTE_NEGOTIATION, FunctionType::EMBASSY_MANAGEMENT
            }},
            {SystemType::POPULATION, {
                FunctionType::POPULATION_MANAGEMENT, FunctionType::MIGRATION_POLICY,
                FunctionType::RELIGIOUS_POLICY, FunctionType::EDUCATION_POLICY,
                FunctionType::CULTURAL_POLICY, FunctionType::LABOR_POLICY
            }},
            {SystemType::CONSTRUCTION, {
                FunctionType::BUILDING_CONSTRUCTION, FunctionType::INFRASTRUCTURE_DEVELOPMENT,
                FunctionType::URBAN_PLANNING, FunctionType::MAINTENANCE, FunctionType::PROJECT_MANAGEMENT
            }},
            {SystemType::TECHNOLOGY, {
                FunctionType::RESEARCH_ADMINISTRATION, FunctionType::INNOVATION_POLICY,
                FunctionType::KNOWLEDGE_ACQUISITION, FunctionType::CRAFT_REGULATION,
                FunctionType::TECHNICAL_EDUCATION
            }}
        };

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

        for (const auto& pair : s_threading_strategy_to_string) {
            s_string_to_threading_strategy[pair.second] = pair.first;
        }

        // ========================================
        // Social Class Mappings
        // ========================================
        s_social_class_to_string = {
            {game::population::SocialClass::HIGH_NOBILITY, "high_nobility"},
            {game::population::SocialClass::LESSER_NOBILITY, "lesser_nobility"},
            {game::population::SocialClass::HIGH_CLERGY, "high_clergy"},
            {game::population::SocialClass::CLERGY, "clergy"},
            {game::population::SocialClass::WEALTHY_MERCHANTS, "wealthy_merchants"},
            {game::population::SocialClass::BURGHERS, "burghers"},
            {game::population::SocialClass::GUILD_MASTERS, "guild_masters"},
            {game::population::SocialClass::CRAFTSMEN, "craftsmen"},
            {game::population::SocialClass::SCHOLARS, "scholars"},
            {game::population::SocialClass::FREE_PEASANTS, "free_peasants"},
            {game::population::SocialClass::VILLEINS, "villeins"},
            {game::population::SocialClass::SERFS, "serfs"},
            {game::population::SocialClass::URBAN_LABORERS, "urban_laborers"},
            {game::population::SocialClass::SLAVES, "slaves"},
            {game::population::SocialClass::FOREIGNERS, "foreigners"},
            {game::population::SocialClass::OUTLAWS, "outlaws"},
            {game::population::SocialClass::RELIGIOUS_ORDERS, "religious_orders"}
        };

        for (const auto& pair : s_social_class_to_string) {
            s_string_to_social_class[pair.second] = pair.first;
        }

        s_initialized = true;
    }

    // ============================================================================
    // Public API Implementation
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
        // Determine technology category based on type
        int tech_value = static_cast<int>(tech);

        if (tech_value >= 100 && tech_value < 200) {
            return TechnologyCategory::MILITARY;
        }
        else if (tech_value >= 200 && tech_value < 300) {
            return TechnologyCategory::AGRICULTURAL;
        }
        else if (tech_value >= 300 && tech_value < 400) {
            return TechnologyCategory::CRAFT;
        }
        else if (tech_value >= 400 && tech_value < 500) {
            return TechnologyCategory::ADMINISTRATIVE;
        }
        else if (tech_value >= 500 && tech_value < 600) {
            return TechnologyCategory::RELIGIOUS;
        }
        else if (tech_value >= 600 && tech_value < 700) {
            return TechnologyCategory::NAVAL;
        }
        else {
            return TechnologyCategory::GENERAL;
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
        return ::core::threading::ThreadingStrategy::MAIN_THREAD; // Default fallback
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
        return game::population::SocialClass::FREE_PEASANTS; // Default fallback
    }

} // namespace game::types