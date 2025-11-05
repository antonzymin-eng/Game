// ============================================================================
// DiplomacyEconomicBridge.h - Integration Bridge Between Diplomacy and Economic Systems
// Created: October 31, 2025
// Location: include/game/bridge/DiplomacyEconomicBridge.h
// ============================================================================

#pragma once

#include "core/ECS/ComponentAccessManager.h"
#include "core/ECS/MessageBus.h"
#include "core/ECS/ISystem.h"
#include "core/threading/ThreadedSystemManager.h"
#include "core/threading/ThreadingTypes.h"
#include "core/types/game_types.h"
#include "game/diplomacy/DiplomacyComponents.h"
#include "game/economy/EconomicComponents.h"
#include "game/trade/TradeSystem.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <shared_mutex>

// Forward declarations
namespace Json {
    class Value;
}

namespace game::bridge {

    // ============================================================================
    // Embargo and Sanctions System
    // ============================================================================

    enum class SanctionType {
        TRADE_EMBARGO = 0,       // Complete trade ban
        PARTIAL_EMBARGO,         // Specific goods only
        FINANCIAL_SANCTIONS,     // Restrict financial flows
        TARIFF_INCREASE,         // Increased trade costs
        ASSET_FREEZE,            // Freeze financial assets
        DIPLOMATIC_ISOLATION     // Diplomatic restrictions
    };

    enum class SanctionSeverity {
        MILD = 0,
        MODERATE,
        SEVERE,
        TOTAL
    };

    struct Sanction {
        std::string sanction_id;
        types::EntityID imposer;
        types::EntityID target;
        SanctionType type = SanctionType::TRADE_EMBARGO;
        SanctionSeverity severity = SanctionSeverity::MODERATE;

        // Economic impact
        double trade_reduction_factor = 0.5;    // 0.0-1.0, how much trade is reduced
        double cost_increase_factor = 1.5;      // Multiplier for trade costs
        int monthly_economic_damage = 0;        // Direct economic damage per month

        // Timing
        std::chrono::system_clock::time_point start_time;
        int duration_months = 12;               // -1 for indefinite
        int months_elapsed = 0;

        // Affected resources (empty = all resources)
        std::unordered_set<types::ResourceType> affected_resources;

        // Political impact
        int opinion_modifier = -50;
        double prestige_cost = 10.0;            // Prestige cost to imposer

        // Metadata
        std::string reason;
        bool is_active = true;
        bool requires_enforcement = true;

        Sanction() = default;
        Sanction(const std::string& id, types::EntityID imposer_id, types::EntityID target_id);

        bool IsExpired() const;
        double GetEffectiveTradeReduction() const;
    };

    // ============================================================================
    // Economic Dependency System
    // ============================================================================

    struct EconomicDependency {
        types::EntityID realm_id;
        types::EntityID trading_partner;

        // Dependency metrics
        double trade_dependency = 0.0;          // 0.0-1.0, % of total trade
        double resource_dependency = 0.0;       // 0.0-1.0, critical resources
        double financial_dependency = 0.0;      // 0.0-1.0, loans/tribute
        double overall_dependency = 0.0;        // Weighted average

        // Critical resources this realm needs from partner
        std::unordered_map<types::ResourceType, double> critical_imports;

        // Economic vulnerability
        double vulnerability_to_disruption = 0.0; // 0.0-1.0
        int estimated_months_to_collapse = 24;    // If trade cut off

        EconomicDependency() = default;
        EconomicDependency(types::EntityID realm, types::EntityID partner);

        void CalculateOverallDependency();
        bool IsHighlyDependent() const { return overall_dependency > 0.6; }
        bool IsCriticallyDependent() const { return overall_dependency > 0.8; }
    };

    // ============================================================================
    // Trade Agreement Data
    // ============================================================================

    struct TradeAgreement {
        std::string agreement_id;
        types::EntityID realm_a;
        types::EntityID realm_b;

        // Economic benefits
        double trade_bonus_multiplier = 1.2;    // Trade volume multiplier
        double tariff_reduction = 0.5;          // Reduced trade costs
        int monthly_revenue_bonus = 0;          // Direct income bonus

        // Special provisions
        bool preferential_access = false;        // Priority in markets
        bool most_favored_nation = false;        // Best terms available
        bool exclusive_trade_rights = false;     // Monopoly on certain goods

        // Affected goods (empty = all goods)
        std::unordered_set<types::ResourceType> covered_resources;

        // Duration and status
        int duration_years = 10;
        int years_remaining = 10;
        bool is_active = true;
        bool auto_renew = true;

        // Political ties
        int opinion_bonus = 10;
        std::string linked_treaty_id;           // Diplomatic treaty this is tied to

        TradeAgreement() = default;
        TradeAgreement(const std::string& id, types::EntityID a, types::EntityID b);

        double GetEffectiveTradeBonus(types::ResourceType resource) const;
        bool CoversResource(types::ResourceType resource) const;
    };

    // ============================================================================
    // War Economic Impact Tracking
    // ============================================================================

    struct WarEconomicImpact {
        types::EntityID aggressor;
        types::EntityID defender;
        std::chrono::system_clock::time_point war_start;

        // Economic costs
        int total_military_spending = 0;
        int total_trade_losses = 0;
        int total_infrastructure_damage = 0;
        int total_population_loss = 0;

        // Monthly costs
        int monthly_war_cost = 0;
        int monthly_trade_disruption = 0;

        // Trade route disruptions
        std::vector<std::string> disrupted_trade_routes;
        std::unordered_set<types::EntityID> affected_neutral_parties;

        // Economic recovery estimates
        int estimated_recovery_months = 12;
        double economic_devastation = 0.0;      // 0.0-1.0

        WarEconomicImpact() = default;
        WarEconomicImpact(types::EntityID agg, types::EntityID def);

        void UpdateMonthlyCosts();
        int GetTotalWarCost() const;
    };

    // ============================================================================
    // Bridge Messages
    // ============================================================================

    namespace messages {
        struct SanctionImposed {
            std::string sanction_id;
            types::EntityID imposer;
            types::EntityID target;
            SanctionType type;
            SanctionSeverity severity;
            std::string reason;
            int estimated_economic_damage;
        };

        struct SanctionLifted {
            std::string sanction_id;
            types::EntityID imposer;
            types::EntityID target;
            int total_economic_damage_dealt;
            int months_active;
        };

        struct TradeAgreementEstablished {
            std::string agreement_id;
            types::EntityID realm_a;
            types::EntityID realm_b;
            double expected_trade_increase;
            int duration_years;
        };

        struct TradeAgreementExpired {
            std::string agreement_id;
            types::EntityID realm_a;
            types::EntityID realm_b;
            double total_trade_value_generated;
        };

        struct EconomicDependencyChanged {
            types::EntityID realm_id;
            types::EntityID trading_partner;
            double old_dependency;
            double new_dependency;
            bool is_now_critical;
        };

        struct WarEconomicDamage {
            types::EntityID aggressor;
            types::EntityID defender;
            int monthly_cost_aggressor;
            int monthly_cost_defender;
            int neutral_trade_losses;
        };

        struct EconomicCrisisDiplomatic {
            types::EntityID realm_id;
            std::string crisis_type;
            double severity;
            std::vector<types::EntityID> affected_trading_partners;
            bool likely_to_cause_instability;
        };
    }

    // ============================================================================
    // Main Bridge System
    // ============================================================================

    class DiplomacyEconomicBridge : public game::core::ISystem {
    public:
        explicit DiplomacyEconomicBridge(::core::ecs::ComponentAccessManager& access_manager,
                                        ::core::ecs::MessageBus& message_bus);
        virtual ~DiplomacyEconomicBridge() = default;

        // ISystem interface
        void Initialize() override;
        void Update(float delta_time) override;
        void Shutdown() override;

        // Threading configuration
        ::core::threading::ThreadingStrategy GetThreadingStrategy() const override;

        // Serialization
        Json::Value Serialize(int version) const override;
        bool Deserialize(const Json::Value& data, int version) override;
        std::string GetSystemName() const override;

        // ====================================================================
        // Sanctions and Embargoes
        // ====================================================================

        // Impose sanctions
        std::string ImposeSanction(types::EntityID imposer, types::EntityID target,
                                  SanctionType type, SanctionSeverity severity,
                                  const std::string& reason);
        std::string ImposeTradeEmbargo(types::EntityID imposer, types::EntityID target,
                                      const std::vector<types::ResourceType>& resources);
        void LiftSanction(const std::string& sanction_id);
        void LiftAllSanctions(types::EntityID imposer, types::EntityID target);

        // Sanction queries
        std::vector<Sanction> GetActiveSanctionsAgainst(types::EntityID target) const;
        std::vector<Sanction> GetSanctionsImposedBy(types::EntityID imposer) const;
        bool IsUnderSanction(types::EntityID realm_id) const;
        double GetTotalSanctionImpact(types::EntityID realm_id) const;

        // ====================================================================
        // Trade Agreements
        // ====================================================================

        // Create and manage agreements
        std::string CreateTradeAgreement(types::EntityID realm_a, types::EntityID realm_b,
                                        double trade_bonus, int duration_years);
        void TerminateTradeAgreement(const std::string& agreement_id);
        void RenewTradeAgreement(const std::string& agreement_id, int additional_years);

        // Agreement queries
        std::vector<TradeAgreement> GetTradeAgreements(types::EntityID realm_id) const;
        std::optional<TradeAgreement> GetTradeAgreement(types::EntityID realm_a, types::EntityID realm_b) const;
        bool HasTradeAgreement(types::EntityID realm_a, types::EntityID realm_b) const;
        double GetTradeAgreementBonus(types::EntityID realm_a, types::EntityID realm_b,
                                     types::ResourceType resource) const;

        // ====================================================================
        // Economic Dependency Analysis
        // ====================================================================

        // Calculate dependencies
        EconomicDependency CalculateDependency(types::EntityID realm_id, types::EntityID partner) const;
        void UpdateAllDependencies();
        void UpdateDependenciesForRealm(types::EntityID realm_id);

        // Dependency queries
        std::vector<EconomicDependency> GetDependencies(types::EntityID realm_id) const;
        std::vector<types::EntityID> GetCriticalTradingPartners(types::EntityID realm_id) const;
        bool IsDependentOn(types::EntityID realm_id, types::EntityID partner, double threshold = 0.6) const;
        double GetDependencyLevel(types::EntityID realm_id, types::EntityID partner) const;

        // ====================================================================
        // War Economic Integration
        // ====================================================================

        // War impact tracking
        void OnWarDeclared(types::EntityID aggressor, types::EntityID defender);
        void OnPeaceTreaty(types::EntityID realm_a, types::EntityID realm_b);
        void ProcessWarEconomics();

        // War economic queries
        WarEconomicImpact* GetWarImpact(types::EntityID aggressor, types::EntityID defender);
        int GetMonthlyWarCost(types::EntityID realm_id) const;
        std::vector<std::string> GetDisruptedTradeRoutes(types::EntityID realm_id) const;

        // ====================================================================
        // Diplomatic Event -> Economic Impact
        // ====================================================================

        // Process diplomatic events
        void OnAllianceFormed(types::EntityID realm_a, types::EntityID realm_b);
        void OnAllianceBroken(types::EntityID realm_a, types::EntityID realm_b);
        void OnTreatyViolation(types::EntityID violator, types::EntityID victim);
        void OnDiplomaticGift(types::EntityID sender, types::EntityID recipient, int value);

        // Apply diplomatic effects to economy
        void ApplyTreatyEconomicEffects(types::EntityID realm_id, diplomacy::TreatyType treaty_type);
        void RemoveTreatyEconomicEffects(types::EntityID realm_id, diplomacy::TreatyType treaty_type);

        // ====================================================================
        // Economic Event -> Diplomatic Impact
        // ====================================================================

        // Process economic events
        void OnTradeRouteDisrupted(types::EntityID source, types::EntityID destination,
                                  types::ResourceType resource);
        void OnEconomicCrisis(types::EntityID realm_id, const std::string& crisis_type, double severity);
        void OnResourceShortage(types::EntityID realm_id, types::ResourceType resource, double severity);

        // Apply economic effects to diplomacy
        void AdjustOpinionBasedOnTrade(types::EntityID realm_a, types::EntityID realm_b);
        void ProcessEconomicInfluenceOnRelations();

        // ====================================================================
        // Integration Utilities
        // ====================================================================

        // Cross-system queries
        double CalculateTradeValue(types::EntityID realm_a, types::EntityID realm_b) const;
        double CalculateEconomicLeverage(types::EntityID realm_id, types::EntityID target) const;
        bool WouldWarHurtEconomy(types::EntityID aggressor, types::EntityID target) const;
        int EstimateTributePotential(types::EntityID stronger, types::EntityID weaker) const;

        // Decision support
        bool ShouldAvoidWarForEconomicReasons(types::EntityID realm_id, types::EntityID target) const;
        bool ShouldFormAllianceForTrade(types::EntityID realm_id, types::EntityID potential_ally) const;
        std::vector<types::EntityID> GetBestTradePartnerCandidates(types::EntityID realm_id, int count = 5) const;

        // Statistics and reporting
        double GetTotalTradeRevenue(types::EntityID realm_id) const;
        int GetTradePartnerCount(types::EntityID realm_id) const;
        double GetAverageDependencyLevel(types::EntityID realm_id) const;

    private:
        ::core::ecs::ComponentAccessManager& m_access_manager;
        ::core::ecs::MessageBus& m_message_bus;

        bool m_initialized = false;
        float m_accumulated_time = 0.0f;
        float m_update_interval = 1.0f;
        float m_monthly_timer = 0.0f;

        // Sanctions and embargoes
        std::unordered_map<std::string, Sanction> m_active_sanctions;
        std::unordered_map<types::EntityID, std::vector<std::string>> m_sanctions_by_target;
        std::unordered_map<types::EntityID, std::vector<std::string>> m_sanctions_by_imposer;

        // Trade agreements
        std::unordered_map<std::string, TradeAgreement> m_trade_agreements;
        std::unordered_map<types::EntityID, std::vector<std::string>> m_agreements_by_realm;

        // Economic dependencies
        std::unordered_map<types::EntityID, std::vector<EconomicDependency>> m_dependencies;

        // War economics
        std::vector<WarEconomicImpact> m_active_wars;

        // Sanction baseline tracking (prevents overcorrection on removal)
        std::unordered_map<types::EntityID, double> m_sanction_baselines;

        // Thread safety for concurrent access from AI systems
        mutable std::shared_mutex m_sanctions_mutex;
        mutable std::shared_mutex m_agreements_mutex;
        mutable std::shared_mutex m_dependencies_mutex;
        mutable std::shared_mutex m_wars_mutex;

        // Configuration
        double m_dependency_threshold_high = 0.6;
        double m_dependency_threshold_critical = 0.8;
        double m_trade_opinion_modifier = 0.5;     // Opinion bonus per 100 trade value
        double m_war_trade_disruption = 0.7;       // % of trade routes disrupted in war

        // Initialization
        void SubscribeToEvents();
        void LoadConfiguration();

        // Update processing
        void ProcessRegularUpdates(float delta_time);
        void ProcessMonthlyUpdates();
        void UpdateSanctions();
        void UpdateTradeAgreements();
        void UpdateDependencies();

        // Internal helpers
        std::string GenerateSanctionId(types::EntityID imposer, types::EntityID target) const;
        std::string GenerateAgreementId(types::EntityID realm_a, types::EntityID realm_b) const;
        void ApplySanctionEffects(const Sanction& sanction);
        void RemoveSanctionEffects(const Sanction& sanction);
        void ApplyTradeAgreementEffects(const TradeAgreement& agreement);
        void RemoveTradeAgreementEffects(const TradeAgreement& agreement);

        // Cross-system integration
        economy::EconomicComponent* GetEconomicComponent(types::EntityID realm_id);
        const economy::EconomicComponent* GetEconomicComponent(types::EntityID realm_id) const;
        diplomacy::DiplomacyComponent* GetDiplomacyComponent(types::EntityID realm_id);
        const diplomacy::DiplomacyComponent* GetDiplomacyComponent(types::EntityID realm_id) const;

        // Economic calculations
        double CalculateTradeVolume(types::EntityID realm_a, types::EntityID realm_b) const;
        double CalculateResourceDependency(types::EntityID realm_id, types::EntityID partner) const;
        double CalculateFinancialDependency(types::EntityID realm_id, types::EntityID partner) const;
        int CalculateMonthlyTradeRevenue(types::EntityID realm_id, types::EntityID partner) const;

        // War economics
        void CalculateWarCosts(WarEconomicImpact& war);
        void DisruptTradeRoutesForWar(types::EntityID aggressor, types::EntityID defender);
        void RestoreTradeRoutesAfterPeace(types::EntityID realm_a, types::EntityID realm_b);
        std::vector<types::EntityID> GetAffectedNeutralParties(types::EntityID realm_a, types::EntityID realm_b) const;

        // Message handlers
        void HandleDiplomaticEvent(const std::string& event_type, types::EntityID realm_a, types::EntityID realm_b);
        void HandleEconomicEvent(const std::string& event_type, types::EntityID realm_id);

        // Logging and debugging
        void LogBridgeEvent(const std::string& message) const;
        void ValidateBridgeState() const;
    };

} // namespace game::bridge
