// ============================================================================
// FactionComponents.h - ECS Components for Faction System
// Created: November 18, 2025 - Faction System Implementation
// Location: include/game/faction/FactionComponents.h
// ============================================================================

#pragma once

#include "utils/PlatformMacros.h"
#include "core/ECS/IComponent.h"
#include "core/ECS/MessageBus.h"
#include "core/types/game_types.h"
#include <json/json.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <chrono>
#include <algorithm>
#include <typeindex>

namespace game::faction {

    using namespace game::core;
    using namespace game::types;

    // ============================================================================
    // Faction Data Structure - Represents a single faction's state
    // ============================================================================

    struct FactionData {
        FactionID faction_id{ 0 };
        FactionType type = FactionType::INVALID;
        std::string faction_name;

        // Power and influence metrics (0.0-1.0 normalized)
        double influence = 0.3;              // Political power/sway in province
        double loyalty = 0.7;                // Loyalty to the current ruler
        double satisfaction = 0.6;           // Satisfaction with current policies
        double power_base = 0.5;             // Economic/military strength

        // Faction demands and priorities
        std::vector<std::string> active_demands;
        std::vector<std::string> fulfilled_demands;
        double urgency_level = 0.3;          // How urgent their demands are

        // Faction characteristics
        double militancy = 0.2;              // Tendency towards rebellion/violence
        double cohesion = 0.7;               // Internal unity of faction
        double popular_support = 0.5;        // Support among general population

        // Economic factors
        double wealth_level = 0.5;           // Economic resources
        double tax_burden_tolerance = 0.5;   // Willingness to pay taxes
        double economic_contribution = 100.0; // Economic output to province

        // Status and timing
        uint32_t months_since_demand = 0;
        uint32_t months_since_concession = 999;
        bool has_pending_crisis = false;
        bool is_in_revolt = false;

        // Faction leader (if applicable)
        EntityID leader_id = INVALID_ENTITY;
        std::string leader_name;
        double leader_competence = 0.6;
        double leader_charisma = 0.5;

        // Constructors
        FactionData() = default;
        FactionData(FactionID id, FactionType faction_type, const std::string& name);

        // Behavioral methods
        double GetEffectivePower() const;
        double GetRevoltRisk() const;
        bool IsAngry() const;
        bool IsContent() const;
        void ProcessMonthlyUpdate();
        void AdjustSatisfaction(double change);
        void AdjustInfluence(double change);
        void AdjustLoyalty(double change);

        // Serialization
        Json::Value ToJson() const;
        static FactionData FromJson(const Json::Value& data);
    };

    // ============================================================================
    // Provincial Factions Component - Tracks all factions in a province
    // ============================================================================

    struct ProvincialFactionsComponent : public game::core::Component<ProvincialFactionsComponent> {
        // All factions operating in this province
        std::vector<FactionData> factions;
        std::unordered_map<FactionType, size_t> faction_indices; // Quick lookup

        // Aggregate metrics
        double total_faction_influence = 0.0;
        double average_faction_loyalty = 0.7;
        double average_faction_satisfaction = 0.6;
        double faction_stability = 0.8;      // 1.0 = stable, 0.0 = chaos

        // Political dynamics
        double political_tension = 0.2;       // Conflict between factions
        double revolt_risk = 0.1;             // Overall risk of uprising
        FactionType dominant_faction = FactionType::NOBILITY;
        FactionType rising_faction = FactionType::INVALID;

        // Faction balance (proportion of power)
        std::unordered_map<FactionType, double> power_distribution;

        // Faction relationships (-1.0 to 1.0)
        std::unordered_map<std::string, double> inter_faction_relations;

        // Recent faction events
        std::vector<std::string> recent_faction_events;
        uint32_t max_event_history = 20;

        // Timing
        uint32_t months_since_last_crisis = 999;
        uint32_t months_since_last_concession = 999;

        // Component interface
        std::string GetComponentTypeName() const override;

        // Utility methods
        FactionData* GetFaction(FactionType type);
        const FactionData* GetFaction(FactionType type) const;
        bool HasFaction(FactionType type) const;
        void AddFaction(const FactionData& faction);
        void RemoveFaction(FactionType type);
        void RecalculateMetrics();
        void UpdatePowerDistribution();
        std::vector<FactionType> GetAngryFactions() const;
        FactionType GetMostPowerfulFaction() const;
    };

    // ============================================================================
    // National Factions Component - Tracks faction state at nation level
    // ============================================================================

    struct NationalFactionsComponent : public game::core::Component<NationalFactionsComponent> {
        // National faction leaders and movements
        std::unordered_map<FactionType, EntityID> national_leaders;
        std::unordered_map<FactionType, std::string> faction_ideologies;

        // National-level faction metrics (aggregated from provinces)
        std::unordered_map<FactionType, double> national_influence;
        std::unordered_map<FactionType, double> national_loyalty;
        std::unordered_map<FactionType, double> national_satisfaction;

        // National faction demands
        std::unordered_map<FactionType, std::vector<std::string>> national_demands;
        std::unordered_map<FactionType, uint32_t> demands_pending_months;

        // Political coalitions
        std::vector<std::pair<FactionType, FactionType>> active_coalitions;
        std::vector<std::pair<FactionType, FactionType>> hostile_relations;

        // Royal council faction representation
        std::unordered_map<FactionType, uint32_t> council_seats;
        double faction_representation_balance = 0.5; // How balanced is representation

        // National faction events
        std::vector<std::string> national_faction_events;
        uint32_t max_event_history = 50;

        // Crisis tracking
        bool national_faction_crisis = false;
        FactionType crisis_faction = FactionType::INVALID;
        uint32_t months_since_crisis = 999;

        // Component interface
        std::string GetComponentTypeName() const override;

        // Utility methods
        void RecalculateNationalMetrics();
        void UpdateCoalitions();
        bool IsInCoalition(FactionType faction) const;
        std::vector<FactionType> GetHostileFactions(FactionType faction) const;
        FactionType GetMostInfluentialFaction() const;
        FactionType GetMostDissatisfiedFaction() const;
    };

    // ============================================================================
    // Faction Demands Component - Tracks specific demands and policies
    // ============================================================================

    struct FactionDemandsComponent : public game::core::Component<FactionDemandsComponent> {
        // Active demands by faction
        struct Demand {
            FactionType faction = FactionType::INVALID;
            std::string demand_type;         // "tax_reduction", "council_seat", etc.
            std::string demand_description;
            double urgency = 0.5;
            double importance = 0.5;
            uint32_t months_pending = 0;
            bool is_ultimatum = false;

            // Effects of accepting/rejecting
            double satisfaction_if_accepted = 0.2;
            double satisfaction_if_rejected = -0.3;
            double loyalty_if_accepted = 0.1;
            double loyalty_if_rejected = -0.2;

            Json::Value ToJson() const;
            static Demand FromJson(const Json::Value& data);
        };

        std::vector<Demand> pending_demands;
        std::vector<Demand> fulfilled_demands;
        std::vector<Demand> rejected_demands;

        uint32_t max_simultaneous_demands = 10;
        uint32_t max_history_size = 30;

        // Demand generation parameters
        double demand_generation_rate = 0.1; // Per month per faction
        uint32_t months_since_last_demand = 0;

        // Component interface
        std::string GetComponentTypeName() const override;

        // Utility methods
        void AddDemand(const Demand& demand);
        void FulfillDemand(size_t demand_index);
        void RejectDemand(size_t demand_index);
        std::vector<Demand> GetDemandsByFaction(FactionType type) const;
        bool HasUrgentDemands() const;
        bool HasUltimatums() const;
    };

    // ============================================================================
    // Faction Events - MessageBus integration
    // ============================================================================

    struct FactionInfluenceChangeEvent : public ::core::ecs::IMessage {
        EntityID province_id = INVALID_ENTITY;
        FactionType faction = FactionType::INVALID;
        double old_influence = 0.0;
        double new_influence = 0.0;
        std::string reason;

        FactionInfluenceChangeEvent() = default;
        FactionInfluenceChangeEvent(EntityID pid, FactionType ft, double old_inf, double new_inf, const std::string& r)
            : province_id(pid), faction(ft), old_influence(old_inf), new_influence(new_inf), reason(r) {}

        std::type_index GetTypeIndex() const override { return typeid(FactionInfluenceChangeEvent); }
        ::core::ecs::MessagePriority GetPriority() const override { return ::core::ecs::MessagePriority::NORMAL; }
    };

    struct FactionDemandEvent : public ::core::ecs::IMessage {
        EntityID province_id = INVALID_ENTITY;
        FactionType faction = FactionType::INVALID;
        std::string demand_type;
        std::string demand_description;
        bool is_ultimatum = false;

        FactionDemandEvent() = default;
        FactionDemandEvent(EntityID pid, FactionType ft, const std::string& dtype, const std::string& desc, bool ult)
            : province_id(pid), faction(ft), demand_type(dtype), demand_description(desc), is_ultimatum(ult) {}

        std::type_index GetTypeIndex() const override { return typeid(FactionDemandEvent); }
        ::core::ecs::MessagePriority GetPriority() const override { return ::core::ecs::MessagePriority::NORMAL; }
    };

    struct FactionRevoltEvent : public ::core::ecs::IMessage {
        EntityID province_id = INVALID_ENTITY;
        FactionType faction = FactionType::INVALID;
        double revolt_strength = 0.5;
        std::string revolt_reason;
        std::vector<FactionType> supporting_factions;

        FactionRevoltEvent() = default;
        FactionRevoltEvent(EntityID pid, FactionType ft, double strength, const std::string& reason)
            : province_id(pid), faction(ft), revolt_strength(strength), revolt_reason(reason) {}

        std::type_index GetTypeIndex() const override { return typeid(FactionRevoltEvent); }
    };

    struct FactionCoalitionEvent : public ::core::ecs::IMessage {
        FactionType faction1 = FactionType::INVALID;
        FactionType faction2 = FactionType::INVALID;
        bool is_forming = true; // true = forming, false = dissolving
        std::string reason;

        FactionCoalitionEvent() = default;
        FactionCoalitionEvent(FactionType f1, FactionType f2, bool forming, const std::string& r)
            : faction1(f1), faction2(f2), is_forming(forming), reason(r) {}

        std::type_index GetTypeIndex() const override { return typeid(FactionCoalitionEvent); }
    };

    struct FactionSatisfactionChangeEvent : public ::core::ecs::IMessage {
        EntityID province_id = INVALID_ENTITY;
        FactionType faction = FactionType::INVALID;
        double old_satisfaction = 0.0;
        double new_satisfaction = 0.0;
        std::string reason;

        FactionSatisfactionChangeEvent() = default;
        FactionSatisfactionChangeEvent(EntityID pid, FactionType ft, double old_sat, double new_sat, const std::string& r)
            : province_id(pid), faction(ft), old_satisfaction(old_sat), new_satisfaction(new_sat), reason(r) {}

        std::type_index GetTypeIndex() const override { return typeid(FactionSatisfactionChangeEvent); }
        ::core::ecs::MessagePriority GetPriority() const override { return ::core::ecs::MessagePriority::NORMAL; }
    };

} // namespace game::faction
