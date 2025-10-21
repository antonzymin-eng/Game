// ============================================================================
// game_types.h - Unified Type System Foundation for Mechanica Imperii
// Replaces all string-based IDs with type-safe enums and strong types
// ============================================================================

#pragma once

// NOTE: On Windows, Windows.h MUST be included BEFORE this file with:
//   #define NOMINMAX
//   #define WIN32_LEAN_AND_MEAN
//   #include <Windows.h>
// Then immediately undefine: INVALID, ERROR, DELETE, IN, OUT, min, max, etc.

#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <type_traits>
#include <atomic>
#include "core/ECS/IComponent.h"

// Include the actual enum definitions for the TypeRegistry
#include "core/threading/ThreadingTypes.h"
#include "game/population/PopulationTypes.h"

// Forward declarations
namespace game::types {
    using ComponentTypeID = std::uint32_t;
}
// Provide a convenient alias to allow legacy code to use 'types::EntityID' etc.
namespace types = game::types;

namespace core::ecs {
    // Use the type from game::types
    using ComponentTypeID = game::types::ComponentTypeID;
    
    // Global component type ID counter function
    inline ComponentTypeID GetNextComponentTypeID() {
        static std::atomic<ComponentTypeID> s_next_id{1};
        return s_next_id.fetch_add(1);
    }
    
    template<typename T>
    class Component : public game::core::IComponent {
    private:
        static ComponentTypeID s_type_id;

    public:
        // Get unique static type ID for this component type
        static ComponentTypeID GetStaticTypeID() {
            if (s_type_id == 0) {
                s_type_id = GetNextComponentTypeID();
            }
            return s_type_id;
        }

        // Instance method to get type ID
        ComponentTypeID GetTypeID() const override {
            return GetStaticTypeID();
        }

        // Clone implementation using CRTP
        std::unique_ptr<game::core::IComponent> Clone() const override {
            return std::make_unique<T>(static_cast<const T&>(*this));
        }

        // Get component type name
        std::string GetComponentTypeName() const override {
            return typeid(T).name();
        }

        // Serialization compatibility methods for EntityManager
        virtual bool HasSerialize() const {
            return false;  // Default: no serialization (can be overridden)
        }

        virtual bool HasDeserialize() const {
            return false;  // Default: no deserialization (can be overridden)
        }
    };

    // Static member definitions (header-only implementation)
    template<typename T>
    ComponentTypeID Component<T>::s_type_id = 0;
}

namespace game::types {

    // ============================================================================
    // Core Type Definitions
    // ============================================================================
    using TimePoint = std::chrono::system_clock::time_point;
    // ... rest of your types
    using EntityID = uint32_t;
    using ComponentTypeID = uint32_t;
    using SystemTypeID = uint32_t;

    constexpr EntityID INVALID_ENTITY = 0;

    // ============================================================================
    // Strong Type Template for Type Safety
    // ============================================================================

    template<typename T, typename Tag>
    struct StrongType {
        T value;

        explicit StrongType(T val) : value(val) {}
        StrongType() : value{} {}

        // Comparison operators
        bool operator==(const StrongType& other) const { return value == other.value; }
        bool operator!=(const StrongType& other) const { return value != other.value; }
        bool operator<(const StrongType& other) const { return value < other.value; }

        // Conversion
        explicit operator T() const { return value; }
        T get() const { return value; }
    };

    // ============================================================================
    // System Type Identification
    // ============================================================================

    enum class SystemType : uint8_t {
        INVALID = 0,

        // Core Systems
        ECS_FOUNDATION,
        MESSAGE_BUS,
        THREADING,
        SAVE_SYSTEM,
        BALANCE_MONITOR,

        // Game Systems
        ECONOMICS,
        MILITARY,
        DIPLOMACY,
        ADMINISTRATION,
        POPULATION,
        CONSTRUCTION,
        TECHNOLOGY,
        CULTURE,
        RELIGION,
        ESPIONAGE,

        // Character & Politics
        CHARACTERS,
        COURT_INTRIGUE,
        FACTIONS,
        SUCCESSION,

        // World Systems
        TRADE,
        NATURAL_EVENTS,
        CLIMATE,
        RESOURCES,

        MAX_SYSTEM_TYPE
    };

    // ============================================================================
    // Decision System Types
    // ============================================================================

    enum class DecisionType : uint16_t {
        INVALID = 0,

        // Economic Decisions
        ECONOMIC_TAX_RATE = 100,
        ECONOMIC_TRADE_POLICY,
        ECONOMIC_CURRENCY_DEBASEMENT,
        ECONOMIC_MERCHANT_PRIVILEGES,
        ECONOMIC_GUILD_REGULATION,
        ECONOMIC_INFRASTRUCTURE_INVESTMENT,
        ECONOMIC_DEBT_MANAGEMENT,

        // Administrative Decisions
        ADMIN_OFFICIAL_APPOINTMENT = 200,
        ADMIN_CORRUPTION_INVESTIGATION,
        ADMIN_BUREAUCRACY_REFORM,
        ADMIN_CENSUS_ORGANIZATION,
        ADMIN_LAW_CODIFICATION,
        ADMIN_COURT_ESTABLISHMENT,
        ADMIN_PROVINCIAL_AUTONOMY,

        // Military Decisions
        MILITARY_RECRUITMENT = 300,
        MILITARY_UNIT_DEPLOYMENT,
        MILITARY_FORTIFICATION_CONSTRUCTION,
        MILITARY_MERCENARY_HIRING,
        MILITARY_NAVAL_EXPANSION,
        MILITARY_SIEGE_TACTICS,
        MILITARY_ARMY_REORGANIZATION,

        // Diplomatic Decisions
        DIPLOMACY_ALLIANCE_PROPOSAL = 400,
        DIPLOMACY_TRADE_AGREEMENT,
        DIPLOMACY_MARRIAGE_NEGOTIATION,
        DIPLOMACY_BORDER_SETTLEMENT,
        DIPLOMACY_TRIBUTE_DEMAND,
        DIPLOMACY_EMBASSY_ESTABLISHMENT,
        DIPLOMACY_WAR_DECLARATION,

        // Population Decisions
        POPULATION_MIGRATION_POLICY = 500,
        POPULATION_RELIGIOUS_TOLERANCE,
        POPULATION_EDUCATION_FUNDING,
        POPULATION_SETTLEMENT_ENCOURAGEMENT,
        POPULATION_CULTURAL_INTEGRATION,
        POPULATION_LABOR_REGULATION,
        POPULATION_HEALTH_MEASURES,

        // Construction Decisions
        CONSTRUCTION_BUILDING_PROJECT = 600,
        CONSTRUCTION_ROAD_NETWORK,
        CONSTRUCTION_HARBOR_EXPANSION,
        CONSTRUCTION_CATHEDRAL_BUILDING,
        CONSTRUCTION_UNIVERSITY_FOUNDING,
        CONSTRUCTION_MARKET_ESTABLISHMENT,
        CONSTRUCTION_DEFENSIVE_WORKS,

        // Technology & Innovation
        TECHNOLOGY_RESEARCH_FUNDING = 700,
        TECHNOLOGY_SCHOLAR_PATRONAGE,
        TECHNOLOGY_INNOVATION_ENCOURAGEMENT,
        TECHNOLOGY_KNOWLEDGE_ACQUISITION,
        TECHNOLOGY_CRAFT_GUILD_SUPPORT,
        TECHNOLOGY_FOREIGN_EXPERTISE,
        TECHNOLOGY_PRINTING_PRESS_ADOPTION,

        // Character & Court
        CHARACTER_MARRIAGE_ARRANGEMENT = 800,
        CHARACTER_HEIR_EDUCATION,
        CHARACTER_COURTIER_APPOINTMENT,
        CHARACTER_NOBLE_PRIVILEGES,
        CHARACTER_SUCCESSION_PLANNING,
        CHARACTER_COURT_CEREMONY,
        CHARACTER_PERSONAL_QUEST,

        // Faction Politics
        FACTION_NOBLE_DEMANDS = 900,
        FACTION_MERCHANT_PRIVILEGES,
        FACTION_CLERGY_DISPUTES,
        FACTION_MILITARY_COMPLAINTS,
        FACTION_REGIONAL_AUTONOMY,
        FACTION_SUCCESSION_CRISIS,
        FACTION_CIVIL_UNREST,

        MAX_DECISION_TYPE = 9999
    };

    // ============================================================================
    // Function Type Classification
    // ============================================================================

    enum class FunctionType : uint16_t {
        INVALID = 0,

        // Economic Functions
        TAX_COLLECTION = 100,
        TRADE_REGULATION,
        CURRENCY_MANAGEMENT,
        DEBT_COLLECTION,
        MARKET_OVERSIGHT,
        GUILD_LICENSING,
        RESOURCE_ALLOCATION,

        // Administrative Functions
        OFFICIAL_APPOINTMENT = 200,
        CORRUPTION_MONITORING,
        BUREAUCRACY_MANAGEMENT,
        RECORD_KEEPING,
        LAW_ENFORCEMENT,
        CENSUS_TAKING,
        COURT_ADMINISTRATION,

        // Military Functions
        RECRUITMENT = 300,
        UNIT_TRAINING,
        DEPLOYMENT_PLANNING,
        LOGISTICS_MANAGEMENT,
        FORTIFICATION_MAINTENANCE,
        INTELLIGENCE_GATHERING,
        VETERAN_CARE,

        // Diplomatic Functions
        ALLIANCE_MAINTENANCE = 400,
        TRADE_NEGOTIATION,
        BORDER_MANAGEMENT,
        EMBASSY_OPERATIONS,
        INTELLIGENCE_EXCHANGE,
        CULTURAL_EXCHANGE,
        CONFLICT_MEDIATION,

        // Construction Functions
        PROJECT_PLANNING = 500,
        RESOURCE_PROCUREMENT,
        WORKER_COORDINATION,
        QUALITY_CONTROL,
        MAINTENANCE_SCHEDULING,
        INFRASTRUCTURE_PLANNING,
        URBAN_DEVELOPMENT,

        // Population Management
        MIGRATION_CONTROL = 600,
        CULTURAL_INTEGRATION,
        RELIGIOUS_AFFAIRS,
        EDUCATION_OVERSIGHT,
        HEALTH_ADMINISTRATION,
        SETTLEMENT_PLANNING,
        DEMOGRAPHIC_MONITORING,

        MAX_FUNCTION_TYPE = 9999
    };

    // ============================================================================
    // Regional Classification
    // ============================================================================

    enum class RegionType : uint8_t {
        INVALID = 0,

        // Geographic Regions
        CORE_PROVINCES,
        BORDER_PROVINCES,
        DISTANT_PROVINCES,
        OVERSEAS_TERRITORIES,
        VASSAL_LANDS,
        OCCUPIED_TERRITORIES,

        // Cultural Regions
        HOME_CULTURE,
        ACCEPTED_CULTURES,
        FOREIGN_CULTURES,
        HOSTILE_CULTURES,

        // Administrative Regions
        CAPITAL_REGION,
        DUCAL_REGIONS,
        COUNTY_REGIONS,
        FRONTIER_REGIONS,
        TRADE_ZONES,
        MILITARY_DISTRICTS,

        MAX_REGION_TYPE
    };

    // ============================================================================
    // Faction System Types
    // ============================================================================

    enum class FactionType : uint8_t {
        INVALID = 0,

        // Internal Factions
        NOBILITY,
        CLERGY,
        MERCHANTS,
        MILITARY,
        BURGHERS,
        PEASANTS,
        BUREAUCRATS,

        // Specialized Factions
        COURT_FACTION,
        REGIONAL_FACTION,
        RELIGIOUS_ORDER,
        TRADE_GUILD,
        MILITARY_ORDER,
        INTELLECTUAL_CIRCLE,
        FOREIGN_INFLUENCE,

        MAX_FACTION_TYPE
    };

    // Strong typed faction ID
    using FactionID = StrongType<uint32_t, struct FactionIDTag>;

    // ============================================================================
    // Event System Types
    // ============================================================================

    enum class EventType : uint16_t {
        INVALID = 0,

        // Economic Events
        ECONOMIC_CRISIS = 100,
        TRADE_DISRUPTION,
        CURRENCY_FLUCTUATION,
        MARKET_CRASH,
        RESOURCE_DISCOVERY,
        HARVEST_FAILURE,
        COMMERCIAL_BOOM,

        // Political Events
        SUCCESSION_CRISIS = 200,
        NOBLE_REBELLION,
        FACTION_DEMANDS,
        COURT_SCANDAL,
        DIPLOMATIC_INCIDENT,
        CIVIL_UNREST,
        ADMINISTRATIVE_CRISIS,

        // Military Events
        ENEMY_INVASION = 300,
        MILITARY_MUTINY,
        SIEGE_WARFARE,
        NAVAL_BATTLE,
        MERCENARY_DESERTION,
        FORTIFICATION_BREACH,
        STRATEGIC_VICTORY,

        // Social Events
        POPULATION_GROWTH = 400,
        CULTURAL_SHIFT,
        RELIGIOUS_MOVEMENT,
        PLAGUE_OUTBREAK,
        MIGRATION_WAVE,
        TECHNOLOGICAL_BREAKTHROUGH,
        EDUCATIONAL_ADVANCEMENT,

        // Natural Events
        NATURAL_DISASTER = 500,
        CLIMATE_CHANGE,
        RESOURCE_DEPLETION,
        GEOLOGICAL_EVENT,
        WEATHER_PATTERN,
        ECOLOGICAL_SHIFT,
        ASTRONOMICAL_EVENT,

        // Character Events
        CHARACTER_DEATH = 600,
        CHARACTER_MARRIAGE,
        CHARACTER_BIRTH,
        CHARACTER_COMING_OF_AGE,
        CHARACTER_SKILL_DEVELOPMENT,
        CHARACTER_RELATIONSHIP_CHANGE,
        CHARACTER_ACHIEVEMENT,

        MAX_EVENT_TYPE = 9999
    };

    // Strong typed event ID
    using EventID = StrongType<uint32_t, struct EventIDTag>;

    // ============================================================================
    // Technology System Types
    // ============================================================================

    enum class TechnologyCategory : uint8_t {
        INVALID = 0,

        MILITARY_TECHNOLOGY,
        AGRICULTURAL_TECHNIQUES,
        CRAFT_KNOWLEDGE,
        ADMINISTRATIVE_METHODS,
        RELIGIOUS_KNOWLEDGE,
        NAVAL_TECHNOLOGY,
        ARCHITECTURAL_TECHNIQUES,
        SCHOLARLY_PURSUITS,
        MEDICAL_KNOWLEDGE,
        ENGINEERING_SKILLS,

        MAX_TECHNOLOGY_CATEGORY
    };

    enum class TechnologyType : uint16_t {
        INVALID = 0,

        // Military Technologies
        HEAVY_CAVALRY = 100,
        CROSSBOW_TACTICS,
        SIEGE_ENGINES,
        PLATE_ARMOR,
        GUNPOWDER_WEAPONS,
        FORTIFICATION_DESIGN,
        NAVAL_ARTILLERY,

        // Agricultural Technologies
        THREE_FIELD_SYSTEM = 200,
        HEAVY_PLOW,
        WINDMILLS,
        CROP_ROTATION,
        SELECTIVE_BREEDING,
        AGRICULTURAL_TOOLS,
        IRRIGATION_SYSTEMS,

        // Craft Technologies
        IMPROVED_METALLURGY = 300,
        TEXTILE_PRODUCTION,
        PRECISION_TOOLS,
        GLASSMAKING,
        PRINTING_PRESS,
        MECHANICAL_CLOCKS,
        OPTICS,

        // Administrative Technologies
        DOUBLE_ENTRY_BOOKKEEPING = 400,
        BUREAUCRATIC_SYSTEMS,
        LEGAL_CODIFICATION,
        POSTAL_SYSTEMS,
        CENSUS_TECHNIQUES,
        DIPLOMATIC_PROTOCOLS,
        TAXATION_METHODS,

        MAX_TECHNOLOGY_TYPE = 9999
    };

    // ============================================================================
    // Character System Types
    // ============================================================================

    enum class CharacterType : uint8_t {
        INVALID = 0,

        PLAYER_CHARACTER,
        FAMILY_MEMBER,
        COURT_NOBLE,
        GOVERNMENT_OFFICIAL,
        MILITARY_COMMANDER,
        RELIGIOUS_LEADER,
        MERCHANT_LEADER,
        FOREIGN_DIPLOMAT,
        ADVISOR,
        SERVANT,

        MAX_CHARACTER_TYPE
    };

    enum class NobleArt : uint8_t {
        INVALID = 0,

        WARFARE,      // Military command and combat
        STEWARDSHIP,  // Economic and administrative management  
        INTRIGUE,     // Political maneuvering and espionage
        LEARNING,     // Scholarship and technological advancement
        DIPLOMACY,    // International relations and negotiation
        PIETY,        // Religious devotion and moral authority

        MAX_NOBLE_ART
    };

    // ============================================================================
    // Situation Classification
    // ============================================================================

    enum class SituationType : uint8_t {
        INVALID = 0,

        ROUTINE,         // Normal day-to-day operations
        IMPORTANT,       // Significant but not urgent
        URGENT,          // Time-sensitive decisions
        CRISIS,          // Emergency situations requiring immediate attention
        OPPORTUNITY,     // Positive situations that could be leveraged
        DIPLOMATIC,      // International relations context
        MILITARY,        // War or conflict context
        ECONOMIC,        // Financial or trade context

        MAX_SITUATION_TYPE
    };

    // ============================================================================
    // Delegation System Types
    // ============================================================================

    enum class DelegationLevel : uint8_t {
        INVALID = 0,

        FULL_CONTROL,     // Player makes all decisions
        ADVISORY,         // Council provides recommendations, player decides
        SUPERVISED,       // Council acts, player can intervene
        AUTONOMOUS,       // Council acts independently

        MAX_DELEGATION_LEVEL
    };

    enum class DelegationType : uint8_t {
        INVALID = 0,

        SYSTEM_WIDE,      // Entire system (all economics, all military, etc.)
        REGIONAL,         // Geographic regions
        FUNCTIONAL,       // Specific functions (tax collection, construction, etc.)
        SITUATIONAL,      // Based on situation type (crisis vs routine)

        MAX_DELEGATION_TYPE
    };

    // ============================================================================
    // Type Conversion Utilities
    // ============================================================================

    class TypeRegistry {
    public:
        // System type conversion
        static std::string SystemTypeToString(SystemType type);
        static SystemType StringToSystemType(const std::string& str);

        // Decision type conversion
        static std::string DecisionTypeToString(DecisionType type);
        static DecisionType StringToDecisionType(const std::string& str);

        // Function type conversion
        static std::string FunctionTypeToString(FunctionType type);
        static FunctionType StringToFunctionType(const std::string& str);

        // Region type conversion
        static std::string RegionTypeToString(RegionType type);
        static RegionType StringToRegionType(const std::string& str);

        // Event type conversion
        static std::string EventTypeToString(EventType type);
        static EventType StringToEventType(const std::string& str);

        // Technology type conversion
        static std::string TechnologyTypeToString(TechnologyType type);
        static TechnologyType StringToTechnologyType(const std::string& str);

        // Threading strategy conversion
        static std::string ThreadingStrategyToString(::core::threading::ThreadingStrategy type);
        static ::core::threading::ThreadingStrategy StringToThreadingStrategy(const std::string& str);

        // Social class conversion
        static std::string SocialClassToString(game::population::SocialClass type);
        static game::population::SocialClass StringToSocialClass(const std::string& str);

        // Validation
        static bool IsValidSystemType(SystemType type);
        static bool IsValidDecisionType(DecisionType type);
        static bool IsValidFunctionType(FunctionType type);
        static bool IsValidRegionType(RegionType type);

        // Category queries
        static SystemType GetSystemForDecision(DecisionType decision);
        static std::vector<FunctionType> GetFunctionsForSystem(SystemType system);
        static TechnologyCategory GetCategoryForTechnology(TechnologyType tech);

    private:
        static void InitializeMappings();
        static bool s_initialized;

        static std::unordered_map<SystemType, std::string> s_system_to_string;
        static std::unordered_map<std::string, SystemType> s_string_to_system;

        static std::unordered_map<DecisionType, std::string> s_decision_to_string;
        static std::unordered_map<std::string, DecisionType> s_string_to_decision;

        static std::unordered_map<FunctionType, std::string> s_function_to_string;
        static std::unordered_map<std::string, FunctionType> s_string_to_function;

        static std::unordered_map<RegionType, std::string> s_region_to_string;
        static std::unordered_map<std::string, RegionType> s_string_to_region;

        static std::unordered_map<EventType, std::string> s_event_to_string;
        static std::unordered_map<std::string, EventType> s_string_to_event;

        static std::unordered_map<TechnologyType, std::string> s_technology_to_string;
        static std::unordered_map<std::string, TechnologyType> s_string_to_technology;

        static std::unordered_map<::core::threading::ThreadingStrategy, std::string> s_threading_strategy_to_string;
        static std::unordered_map<std::string, ::core::threading::ThreadingStrategy> s_string_to_threading_strategy;

        static std::unordered_map<game::population::SocialClass, std::string> s_social_class_to_string;
        static std::unordered_map<std::string, game::population::SocialClass> s_string_to_social_class;

        static std::unordered_map<DecisionType, SystemType> s_decision_to_system;
        static std::unordered_map<SystemType, std::vector<FunctionType>> s_system_to_functions;
    };

    // ============================================================================
    // Decision Structure with Strong Types
    // ============================================================================

    struct Decision {
        DecisionType type = DecisionType::INVALID;
        SystemType system = SystemType::INVALID;
        FunctionType function = FunctionType::INVALID;
        RegionType region = RegionType::INVALID;
        SituationType situation = SituationType::ROUTINE;

        std::string title;
        std::string description;
        std::vector<std::string> options;

        EntityID target_entity = INVALID_ENTITY;
        double urgency = 0.5;
        double importance = 0.5;

        // Metadata
        uint64_t decision_id = 0;
        std::chrono::system_clock::time_point created_time;
        std::chrono::system_clock::time_point deadline;

        // Validation
        bool IsValid() const {
            return type != DecisionType::INVALID &&
                system != SystemType::INVALID &&
                !title.empty() &&
                !options.empty();
        }

        // Helper methods
        bool IsEconomicDecision() const { return system == SystemType::ECONOMICS; }
        bool IsMilitaryDecision() const { return system == SystemType::MILITARY; }
        bool IsDiplomaticDecision() const { return system == SystemType::DIPLOMACY; }
        bool IsUrgent() const { return situation == SituationType::CRISIS || urgency > 0.8; }
        bool IsRoutine() const { return situation == SituationType::ROUTINE && urgency < 0.3; }
    };

    // ============================================================================
    // Event Structure with Strong Types
    // ============================================================================

    struct GameEvent {
        EventType type = EventType::INVALID;
        EventID event_id{ 0 };

        std::string title;
        std::string description;

        EntityID source_entity = INVALID_ENTITY;
        EntityID target_entity = INVALID_ENTITY;

        std::chrono::system_clock::time_point timestamp;
        double severity = 0.5;  // 0.0 = minor, 1.0 = catastrophic

        // Metadata
        std::unordered_map<std::string, double> numeric_data;
        std::unordered_map<std::string, std::string> string_data;

        // Validation
        bool IsValid() const {
            return type != EventType::INVALID &&
                event_id.get() != 0 &&
                !title.empty();
        }

        // Helper methods
        bool IsEconomicEvent() const {
            return static_cast<uint16_t>(type) >= 100 && static_cast<uint16_t>(type) < 200;
        }
        bool IsPoliticalEvent() const {
            return static_cast<uint16_t>(type) >= 200 && static_cast<uint16_t>(type) < 300;
        }
        bool IsMilitaryEvent() const {
            return static_cast<uint16_t>(type) >= 300 && static_cast<uint16_t>(type) < 400;
        }
        bool IsSevere() const { return severity > 0.7; }
        bool IsMinor() const { return severity < 0.3; }
    };

    // ============================================================================
    // Resource System Types
    // ============================================================================

    enum class ResourceType : uint16_t {
        INVALID = 0,

        // Basic Resources
        FOOD = 100,
        WOOD,
        STONE,
        IRON,
        LEATHER,
        CLOTH,

        // Advanced Resources
        HORSES = 200,
        SALTPETER,
        GOLD,
        SILVER,
        SALT,
        SPICES,

        // Luxury Resources
        SILK = 300,
        WINE,
        FURS,
        IVORY,
        JEWELS,

        MAX_RESOURCE_TYPE = 9999
    };

} // namespace game::types

// ============================================================================
// Hash Functions for Strong Types (for use in unordered_map/set)
// ============================================================================

namespace std {
    template<>
    struct hash<game::types::FactionID> {
        size_t operator()(const game::types::FactionID& id) const {
            return std::hash<uint32_t>{}(id.get());
        }
    };

    template<>
    struct hash<game::types::EventID> {
        size_t operator()(const game::types::EventID& id) const {
            return std::hash<uint32_t>{}(id.get());
        }
    };
} // namespace game::types

#pragma once
