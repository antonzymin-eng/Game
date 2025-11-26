// ============================================================================
// AdministrativeComponents.h - ECS Components for Administrative System
// Created: October 11, 2025 - Administrative System ECS Integration
// Location: include/game/administration/AdministrativeComponents.h
// ============================================================================

#pragma once

#include "utils/PlatformMacros.h"
#include "core/ECS/IComponent.h"
#include "core/ECS/MessageBus.h"
#include "core/types/game_types.h"
#include "game/population/PopulationTypes.h"
#include <json/json.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <chrono>
#include <algorithm>
#include <typeindex>
#include <mutex>

namespace game::administration {

    using namespace game::core;

    // ============================================================================
    // Administrative Enums and Data Structures
    // ============================================================================

    enum class OfficialType {
        TAX_COLLECTOR = 0,
        TRADE_MINISTER,
        MILITARY_GOVERNOR,
        COURT_ADVISOR,
        PROVINCIAL_GOVERNOR,
        JUDGE,
        SCRIBE,
        CUSTOMS_OFFICER,
        COUNT
    };

    enum class OfficialTrait {
        NONE = 0,
        CORRUPT,
        EFFICIENT,
        LOYAL,
        AMBITIOUS,
        EXPERIENCED,
        YOUNG_TALENT,
        WELL_CONNECTED,
        STUBBORN,
        SCHOLARLY,
        COUNT
    };

    enum class GovernanceType {
        FEUDAL = 0,
        CENTRALIZED,
        BUREAUCRATIC,
        MERCHANT_REPUBLIC,
        THEOCRACY,
        TRIBAL,
        COUNT
    };

    enum class LawType {
        COMMON_LAW = 0,
        CIVIL_LAW,
        RELIGIOUS_LAW,
        TRIBAL_LAW,
        MERCHANT_LAW,
        MILITARY_LAW,
        COUNT
    };

    // ============================================================================
    // Administrative Official Data Structure
    // ============================================================================

    struct AdministrativeOfficial {
        uint32_t official_id = 0;
        std::string name;
        OfficialType type = OfficialType::COURT_ADVISOR;
        game::types::EntityID assigned_province = 0;

        // Core attributes (0.0-1.0 normalized range)
        double competence = 0.5;
        double loyalty = 0.8;
        double efficiency = 0.6;
        double corruption_resistance = 0.7;

        // Status and experience
        uint32_t age = 30;
        uint32_t months_in_position = 0;
        double satisfaction = 0.7; // 0.0-1.0
        double salary_cost = 100.0;

        // Traits and characteristics
        std::vector<OfficialTrait> traits;
        std::vector<std::string> specializations;

        // Performance metrics
        double administrative_effectiveness = 1.0;
        uint32_t corruption_suspicion = 0; // 0-100 scale
        bool has_pending_event = false;

        // Constructors
        AdministrativeOfficial() = default;
        AdministrativeOfficial(uint32_t id, const std::string& official_name, OfficialType official_type, 
                             game::types::EntityID province = 0);

        // Behavioral methods (thread-safe, deterministic)
        double GetEffectiveCompetence() const;
        double GetLoyaltyModifier() const;
        double GetMonthlyUpkeepCost() const;
        bool IsCorrupt() const;
        
        // Monthly simulation updates
        void ProcessMonthlyUpdate(double competence_drift_rate, double satisfaction_decay_rate);
        void AdjustSatisfaction(double change);
        
        // Trait management
        bool HasTrait(OfficialTrait trait) const;
        void AddTrait(OfficialTrait trait);
        std::string GetTraitDescription(OfficialTrait trait) const;
        
        // Serialization helpers
        Json::Value ToJson() const;
        static AdministrativeOfficial FromJson(const Json::Value& data);
        
        // Factory methods (for AI/event generation)
        static AdministrativeOfficial GenerateRandom(uint32_t id, OfficialType type, 
                                                    game::types::EntityID province = 0);
        static std::string GenerateRandomName();
    };

    // ============================================================================
    // Administrative Event Types (for MessageBus integration)
    // ============================================================================
    
    struct AdminAppointmentEvent : public ::core::ecs::IMessage {
        game::types::EntityID province_id;
        uint32_t official_id;
        OfficialType official_type;
        std::string official_name;
        
        AdminAppointmentEvent() = default;
        AdminAppointmentEvent(game::types::EntityID pid, uint32_t oid, OfficialType otype, const std::string& name)
            : province_id(pid), official_id(oid), official_type(otype), official_name(name) {}

        std::type_index GetTypeIndex() const override { return typeid(AdminAppointmentEvent); }
        ::core::ecs::MessagePriority GetPriority() const override { return ::core::ecs::MessagePriority::NORMAL; }
    };
    
    struct AdminCorruptionEvent : public ::core::ecs::IMessage {
        game::types::EntityID province_id;
        uint32_t official_id;
        double corruption_level;
        std::string incident_description;

        AdminCorruptionEvent() = default;
        AdminCorruptionEvent(game::types::EntityID pid, uint32_t oid, double level, const std::string& desc)
            : province_id(pid), official_id(oid), corruption_level(level), incident_description(desc) {}

        std::type_index GetTypeIndex() const override { return typeid(AdminCorruptionEvent); }
        ::core::ecs::MessagePriority GetPriority() const override { return ::core::ecs::MessagePriority::NORMAL; }
    };
    
    struct AdminDismissalEvent : public ::core::ecs::IMessage {
        game::types::EntityID province_id;
        uint32_t official_id;
        std::string reason;
        
        AdminDismissalEvent() = default;
        AdminDismissalEvent(game::types::EntityID pid, uint32_t oid, const std::string& r)
            : province_id(pid), official_id(oid), reason(r) {}

        std::type_index GetTypeIndex() const override { return typeid(AdminDismissalEvent); }
        ::core::ecs::MessagePriority GetPriority() const override { return ::core::ecs::MessagePriority::NORMAL; }
    };
    
    struct AdminReformEvent : public ::core::ecs::IMessage {
        game::types::EntityID province_id;
        std::string reform_type;
        double cost;
        double efficiency_change;
        
        AdminReformEvent() = default;
        AdminReformEvent(game::types::EntityID pid, const std::string& rtype, double c, double eff_change)
            : province_id(pid), reform_type(rtype), cost(c), efficiency_change(eff_change) {}

        std::type_index GetTypeIndex() const override { return typeid(AdminReformEvent); }
        ::core::ecs::MessagePriority GetPriority() const override { return ::core::ecs::MessagePriority::NORMAL; }
    };

    // ============================================================================
    // Governance Component - Provincial governance structure and policies
    // ============================================================================

    struct GovernanceComponent : public game::core::Component<GovernanceComponent> {
        // Governance structure
        GovernanceType governance_type = GovernanceType::FEUDAL;
        std::vector<AdministrativeOfficial> appointed_officials;
        mutable std::mutex officials_mutex;  // Thread-safe access to appointed_officials

        // Administrative efficiency
        double administrative_efficiency = 0.5;
        double bureaucratic_capacity = 100.0;
        double governance_stability = 0.8;

        // Tax and revenue system
        double tax_collection_efficiency = 0.6;
        double tax_rate = 0.15;
        double total_tax_revenue = 0.0;
        std::unordered_map<std::string, double> tax_sources;

        // Trade and economic administration
        double trade_administration_efficiency = 0.7;
        double customs_efficiency = 0.6;
        double market_regulation_level = 0.5;

        // Military administration
        double military_administration_efficiency = 0.5;
        double recruitment_administration = 0.6;
        double logistics_efficiency = 0.7;

        // Population administration
        double population_administration_efficiency = 0.6;
        double census_accuracy = 0.5;
        double public_order_maintenance = 0.8;

        // Administrative costs
        double monthly_administrative_costs = 0.0;
        double official_salaries = 0.0;
        double infrastructure_costs = 0.0;

        // Default constructor
        GovernanceComponent() = default;

        // Copy constructor (mutex cannot be copied)
        GovernanceComponent(const GovernanceComponent& other)
            : governance_type(other.governance_type),
              appointed_officials(other.appointed_officials),
              administrative_efficiency(other.administrative_efficiency),
              bureaucratic_capacity(other.bureaucratic_capacity),
              governance_stability(other.governance_stability),
              tax_collection_efficiency(other.tax_collection_efficiency),
              tax_rate(other.tax_rate),
              total_tax_revenue(other.total_tax_revenue),
              tax_sources(other.tax_sources),
              trade_administration_efficiency(other.trade_administration_efficiency),
              customs_efficiency(other.customs_efficiency),
              market_regulation_level(other.market_regulation_level),
              military_administration_efficiency(other.military_administration_efficiency),
              recruitment_administration(other.recruitment_administration),
              logistics_efficiency(other.logistics_efficiency),
              population_administration_efficiency(other.population_administration_efficiency),
              census_accuracy(other.census_accuracy),
              public_order_maintenance(other.public_order_maintenance),
              monthly_administrative_costs(other.monthly_administrative_costs),
              official_salaries(other.official_salaries),
              infrastructure_costs(other.infrastructure_costs)
        {
            // mutex is not copied (initialized with default constructor)
        }

        // Copy assignment operator
        GovernanceComponent& operator=(const GovernanceComponent& other) {
            if (this != &other) {
                governance_type = other.governance_type;
                appointed_officials = other.appointed_officials;
                administrative_efficiency = other.administrative_efficiency;
                bureaucratic_capacity = other.bureaucratic_capacity;
                governance_stability = other.governance_stability;
                tax_collection_efficiency = other.tax_collection_efficiency;
                tax_rate = other.tax_rate;
                total_tax_revenue = other.total_tax_revenue;
                tax_sources = other.tax_sources;
                trade_administration_efficiency = other.trade_administration_efficiency;
                customs_efficiency = other.customs_efficiency;
                market_regulation_level = other.market_regulation_level;
                military_administration_efficiency = other.military_administration_efficiency;
                recruitment_administration = other.recruitment_administration;
                logistics_efficiency = other.logistics_efficiency;
                population_administration_efficiency = other.population_administration_efficiency;
                census_accuracy = other.census_accuracy;
                public_order_maintenance = other.public_order_maintenance;
                monthly_administrative_costs = other.monthly_administrative_costs;
                official_salaries = other.official_salaries;
                infrastructure_costs = other.infrastructure_costs;
                // mutex is not copied
            }
            return *this;
        }

        std::string GetComponentTypeName() const override;

        // Serialization
        Json::Value ToJson() const;
        void FromJson(const Json::Value& data);
    };

    // ============================================================================
    // Bureaucracy Component - Administrative apparatus and processes
    // ============================================================================

    struct BureaucracyComponent : public game::core::Component<BureaucracyComponent> {
        // Bureaucratic structure
        uint32_t bureaucracy_level = 1;
        uint32_t scribes_employed = 5;
        uint32_t clerks_employed = 3;
        uint32_t administrators_employed = 1;
        
        // Record keeping and documentation
        double record_keeping_quality = 0.4;
        double document_accuracy = 0.6;
        double administrative_speed = 0.5;
        
        // Bureaucratic processes
        std::unordered_map<std::string, double> process_efficiency;
        std::vector<std::string> active_administrative_tasks;
        std::vector<std::string> pending_decisions;
        
        // Information flow
        double information_gathering_efficiency = 0.5;
        double communication_speed = 0.6;
        double inter_provincial_coordination = 0.4;
        
        // Corruption and oversight
        double corruption_level = 0.2;
        double oversight_effectiveness = 0.6;
        double audit_frequency = 0.1;
        
        // Innovation and reform
        double administrative_innovation = 0.3;
        std::vector<std::string> recent_reforms;
        std::vector<std::string> planned_improvements;
        
        // Performance metrics
        double citizen_satisfaction_with_services = 0.6;
        double administrative_response_time = 5.0; // days
        uint32_t documents_processed_monthly = 100;

        std::string GetComponentTypeName() const override;

        // Serialization
        Json::Value ToJson() const;
        void FromJson(const Json::Value& data);
    };

    // ============================================================================
    // Law Component - Legal system and enforcement
    // ============================================================================

    struct LawComponent : public game::core::Component<LawComponent> {
        // Legal system structure
        LawType primary_law_system = LawType::COMMON_LAW;
        std::vector<LawType> secondary_law_systems;
        
        // Law enforcement
        double law_enforcement_effectiveness = 0.6;
        uint32_t judges_appointed = 2;
        uint32_t bailiffs_employed = 10;
        uint32_t courts_established = 1;
        
        // Legal processes
        double legal_process_speed = 0.5;
        double justice_fairness = 0.7;
        double legal_accessibility = 0.4;
        
        // Crime and punishment
        double crime_rate = 0.3;
        std::unordered_map<std::string, double> crime_types;
        std::unordered_map<std::string, std::string> punishment_types;
        
        // Legal codes and regulations
        std::vector<std::string> active_laws;
        std::vector<std::string> legal_precedents;
        std::vector<std::string> pending_legislation;
        
        // Court system
        uint32_t cases_pending = 20;
        uint32_t cases_resolved_monthly = 15;
        double court_backlog_pressure = 0.3;
        
        // Legal expertise
        double legal_scholarship_level = 0.5;
        uint32_t legal_scholars = 1;
        std::vector<std::string> legal_specializations;
        
        // Social order
        double public_order = 0.8;
        double legal_compliance = 0.7;
        double respect_for_authority = 0.6;

        std::string GetComponentTypeName() const override;

        // Serialization
        Json::Value ToJson() const;
        void FromJson(const Json::Value& data);
    };

    // ============================================================================
    // Administrative Events Component - Administrative events and decisions
    // ============================================================================

    struct AdministrativeEventsComponent : public game::core::Component<AdministrativeEventsComponent> {
        // Active administrative events
        std::vector<std::string> active_appointments;
        std::vector<std::string> pending_dismissals;
        std::vector<std::string> corruption_investigations;
        
        // Official events
        std::vector<std::string> official_promotions;
        std::vector<std::string> official_scandals;
        std::vector<std::string> performance_reviews;
        
        // Policy events
        std::vector<std::string> policy_changes;
        std::vector<std::string> reform_initiatives;
        std::vector<std::string> legislative_proposals;
        
        // Administrative crises
        std::vector<std::string> bureaucratic_failures;
        std::vector<std::string> administrative_delays;
        std::vector<std::string> inter_departmental_conflicts;
        
        // Public relations
        std::vector<std::string> public_announcements;
        std::vector<std::string> citizen_complaints;
        std::vector<std::string> diplomatic_communications;
        
        // Event frequency and timing
        double event_frequency_modifier = 1.0;
        uint32_t months_since_last_appointment = 0;
        uint32_t months_since_last_reform = 0;
        
        // Administrative reputation
        double administrative_reputation = 0.6;
        double government_legitimacy = 0.8;
        double public_trust = 0.7;
        
        // Decision tracking
        std::vector<game::types::EntityID> pending_decisions;
        std::vector<std::string> recent_policy_decisions;

        // Maximum history tracking
        uint32_t max_history_size = 50;

        std::string GetComponentTypeName() const override;

        // Serialization
        Json::Value ToJson() const;
        void FromJson(const Json::Value& data);
    };

    // ============================================================================
    // Administrative Event Structure
    // ============================================================================

    struct AdministrativeEvent {
        uint32_t event_id = 0;
        std::string event_title;
        std::string event_description;
        
        // Event categorization
        std::string event_type; // "appointment", "corruption", "reform", "crisis"
        double urgency_level = 0.5;
        double importance_level = 0.5;
        
        // Event participants
        game::types::EntityID affected_province = 0;
        uint32_t affected_official_id = 0;
        std::vector<game::types::EntityID> involved_entities;
        
        // Decision options
        std::vector<std::string> available_options;
        std::vector<double> option_costs;
        std::vector<std::string> option_consequences;
        
        // Timing
        std::chrono::system_clock::time_point event_date;
        std::chrono::system_clock::time_point deadline;
        bool requires_immediate_attention = false;
        
        // Event effects
        std::unordered_map<std::string, double> administrative_effects;
        std::unordered_map<std::string, double> economic_effects;
        std::unordered_map<std::string, double> social_effects;
        
        static std::string GetTypeName() { return "AdministrativeEvent"; }
    };

} // namespace game::administration