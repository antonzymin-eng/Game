// ============================================================================
// DiplomacyEconomicBridge.cpp - Implementation of Diplomacy-Economic Integration Bridge
// Created: October 31, 2025
// Location: src/game/bridge/DiplomacyEconomicBridge.cpp
// ============================================================================

#include "game/bridge/DiplomacyEconomicBridge.h"
#include "core/types/game_types.h"
#include "game/diplomacy/DiplomacySystem.h"
#include "game/economy/EconomicSystem.h"
#include "game/time/TimeManagementSystem.h"
#include "utils/RandomGenerator.h"
#include <json/json.h>
#include <algorithm>
#include <sstream>
#include <cmath>
#include "core/logging/Logger.h"

namespace game::bridge {

    // ============================================================================
    // Sanction Implementation
    // ============================================================================

    Sanction::Sanction(const std::string& id, types::EntityID imposer_id, types::EntityID target_id)
        : sanction_id(id), imposer(imposer_id), target(target_id),
          start_time(std::chrono::system_clock::now()) {
    }

    bool Sanction::IsExpired() const {
        if (duration_months < 0) return false; // Indefinite
        return months_elapsed >= duration_months;
    }

    double Sanction::GetEffectiveTradeReduction() const {
        if (!is_active) return 0.0;

        // Sanctions become more effective over time (up to 3 months)
        double ramp_up = std::min(1.0, months_elapsed / 3.0);
        return trade_reduction_factor * ramp_up;
    }

    // ============================================================================
    // Economic Dependency Implementation
    // ============================================================================

    EconomicDependency::EconomicDependency(types::EntityID realm, types::EntityID partner)
        : realm_id(realm), trading_partner(partner) {
    }

    void EconomicDependency::CalculateOverallDependency() {
        // Weighted average of different dependency types
        overall_dependency = (trade_dependency * 0.4) +
                           (resource_dependency * 0.4) +
                           (financial_dependency * 0.2);

        // Calculate vulnerability
        vulnerability_to_disruption = overall_dependency *
                                     (1.0 + resource_dependency * 0.5);

        // Estimate time to collapse if trade cut off
        if (overall_dependency > 0.8) {
            estimated_months_to_collapse = 3;
        } else if (overall_dependency > 0.6) {
            estimated_months_to_collapse = 6;
        } else if (overall_dependency > 0.4) {
            estimated_months_to_collapse = 12;
        } else {
            estimated_months_to_collapse = 24;
        }
    }

    // ============================================================================
    // Trade Agreement Implementation
    // ============================================================================

    TradeAgreement::TradeAgreement(const std::string& id, types::EntityID a, types::EntityID b)
        : agreement_id(id), realm_a(a), realm_b(b) {
    }

    double TradeAgreement::GetEffectiveTradeBonus(types::ResourceType resource) const {
        if (!is_active) return 1.0;
        if (!CoversResource(resource)) return 1.0;
        return trade_bonus_multiplier;
    }

    bool TradeAgreement::CoversResource(types::ResourceType resource) const {
        // If no specific resources listed, covers all
        if (covered_resources.empty()) return true;
        return covered_resources.count(resource) > 0;
    }

    // ============================================================================
    // War Economic Impact Implementation
    // ============================================================================

    WarEconomicImpact::WarEconomicImpact(types::EntityID agg, types::EntityID def)
        : aggressor(agg), defender(def),
          war_start(std::chrono::system_clock::now()) {
    }

    void WarEconomicImpact::UpdateMonthlyCosts() {
        // War costs increase over time
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::hours>(now - war_start).count();
        int months_at_war = duration / (24 * 30); // Approximate months

        // Base costs
        monthly_war_cost = 100 + (months_at_war * 10);
        monthly_trade_disruption = 50 + (months_at_war * 5);

        // Accumulate totals
        total_military_spending += monthly_war_cost;
        total_trade_losses += monthly_trade_disruption;
    }

    int WarEconomicImpact::GetTotalWarCost() const {
        return total_military_spending + total_trade_losses +
               total_infrastructure_damage + (total_population_loss * 10);
    }

    // ============================================================================
    // Main Bridge System Implementation
    // ============================================================================

    DiplomacyEconomicBridge::DiplomacyEconomicBridge(
        ::core::ecs::ComponentAccessManager& access_manager,
        ::core::threading::ThreadSafeMessageBus& message_bus)
        : m_access_manager(access_manager), m_message_bus(message_bus) {
    }

    void DiplomacyEconomicBridge::Initialize() {
        if (m_initialized) return;

        LogBridgeEvent("Initializing Diplomacy-Economic Bridge");

        LoadConfiguration();
        SubscribeToEvents();

        m_initialized = true;
        LogBridgeEvent("Bridge initialization complete");
    }

    void DiplomacyEconomicBridge::Update(float delta_time) {
        if (!m_initialized) return;

        m_accumulated_time += delta_time;
        m_monthly_timer += delta_time;

        // Regular updates every second
        if (m_accumulated_time >= m_update_interval) {
            ProcessRegularUpdates(delta_time);
            m_accumulated_time = 0.0f;
        }

        // Monthly updates are now handled by subscribing to TickOccurred messages
        // with TickType::MONTHLY from the TimeManagementSystem (see SubscribeToEvents).
        // This ensures monthly updates happen at actual month boundaries in the game
        // calendar rather than on a fixed 30-second timer.
    }

    void DiplomacyEconomicBridge::Shutdown() {
        if (!m_initialized) return;

        LogBridgeEvent("Shutting down Diplomacy-Economic Bridge");

        m_active_sanctions.clear();
        m_trade_agreements.clear();
        m_dependencies.clear();
        m_active_wars.clear();

        m_initialized = false;
    }

    ::core::threading::ThreadingStrategy DiplomacyEconomicBridge::GetThreadingStrategy() const {
        return ::core::threading::ThreadingStrategy::MAIN_THREAD;
    }

    Json::Value DiplomacyEconomicBridge::Serialize(int version) const {
        Json::Value root;
        root["system_name"] = GetSystemName();
        root["version"] = version;
        root["initialized"] = m_initialized;
        root["sanction_count"] = static_cast<int>(m_active_sanctions.size());
        root["agreement_count"] = static_cast<int>(m_trade_agreements.size());
        return root;
    }

    bool DiplomacyEconomicBridge::Deserialize(const Json::Value& data, int version) {
        if (!data.isObject()) return false;
        // TODO: Implement full deserialization
        return true;
    }

    std::string DiplomacyEconomicBridge::GetSystemName() const {
        return "DiplomacyEconomicBridge";
    }

    void DiplomacyEconomicBridge::SetEconomicSystem(game::economy::EconomicSystem* economic_system) {
        m_economic_system = economic_system;
        if (m_economic_system) {
            LogBridgeEvent("EconomicSystem connected to DiplomacyEconomicBridge");
        }
    }

    // ====================================================================
    // Sanctions and Embargoes
    // ====================================================================

    std::string DiplomacyEconomicBridge::ImposeSanction(
        types::EntityID imposer, types::EntityID target,
        SanctionType type, SanctionSeverity severity,
        const std::string& reason) {

        std::string sanction_id = GenerateSanctionId(imposer, target);

        Sanction sanction(sanction_id, imposer, target);
        sanction.type = type;
        sanction.severity = severity;
        sanction.reason = reason;

        // Set parameters based on severity
        switch (severity) {
            case SanctionSeverity::MILD:
                sanction.trade_reduction_factor = 0.25;
                sanction.cost_increase_factor = 1.2;
                sanction.opinion_modifier = -20;
                sanction.monthly_economic_damage = 50;
                break;
            case SanctionSeverity::MODERATE:
                sanction.trade_reduction_factor = 0.5;
                sanction.cost_increase_factor = 1.5;
                sanction.opinion_modifier = -50;
                sanction.monthly_economic_damage = 150;
                break;
            case SanctionSeverity::SEVERE:
                sanction.trade_reduction_factor = 0.75;
                sanction.cost_increase_factor = 2.0;
                sanction.opinion_modifier = -100;
                sanction.monthly_economic_damage = 300;
                break;
            case SanctionSeverity::TOTAL:
                sanction.trade_reduction_factor = 1.0;
                sanction.cost_increase_factor = 3.0;
                sanction.opinion_modifier = -200;
                sanction.monthly_economic_damage = 500;
                break;
        }

        // Store sanction (with write lock)
        {
            std::unique_lock<std::shared_mutex> lock(m_sanctions_mutex);
            m_active_sanctions[sanction_id] = sanction;
            m_sanctions_by_target[target].push_back(sanction_id);
            m_sanctions_by_imposer[imposer].push_back(sanction_id);
        }

        // Apply effects
        ApplySanctionEffects(sanction);

        // Publish message
        messages::SanctionImposed msg;
        msg.sanction_id = sanction_id;
        msg.imposer = imposer;
        msg.target = target;
        msg.type = type;
        msg.severity = severity;
        msg.reason = reason;
        msg.estimated_economic_damage = sanction.monthly_economic_damage;
        m_message_bus.Publish(msg);

        LogBridgeEvent("Sanction imposed: " + sanction_id);
        return sanction_id;
    }

    std::string DiplomacyEconomicBridge::ImposeTradeEmbargo(
        types::EntityID imposer, types::EntityID target,
        const std::vector<types::ResourceType>& resources) {

        std::string sanction_id = ImposeSanction(imposer, target,
                                                SanctionType::TRADE_EMBARGO,
                                                SanctionSeverity::SEVERE,
                                                "Trade embargo");

        // Add specific resources
        if (m_active_sanctions.count(sanction_id) > 0) {
            auto& sanction = m_active_sanctions[sanction_id];
            sanction.affected_resources.insert(resources.begin(), resources.end());
        }

        return sanction_id;
    }

    void DiplomacyEconomicBridge::LiftSanction(const std::string& sanction_id) {
        auto it = m_active_sanctions.find(sanction_id);
        if (it == m_active_sanctions.end()) return;

        Sanction& sanction = it->second;

        // Remove effects
        RemoveSanctionEffects(sanction);

        // Publish message
        messages::SanctionLifted msg;
        msg.sanction_id = sanction_id;
        msg.imposer = sanction.imposer;
        msg.target = sanction.target;
        msg.total_economic_damage_dealt = sanction.monthly_economic_damage * sanction.months_elapsed;
        msg.months_active = sanction.months_elapsed;
        m_message_bus.Publish(msg);

        // Remove from tracking
        auto& target_sanctions = m_sanctions_by_target[sanction.target];
        target_sanctions.erase(std::remove(target_sanctions.begin(), target_sanctions.end(), sanction_id),
                              target_sanctions.end());

        auto& imposer_sanctions = m_sanctions_by_imposer[sanction.imposer];
        imposer_sanctions.erase(std::remove(imposer_sanctions.begin(), imposer_sanctions.end(), sanction_id),
                               imposer_sanctions.end());

        m_active_sanctions.erase(it);

        LogBridgeEvent("Sanction lifted: " + sanction_id);
    }

    void DiplomacyEconomicBridge::LiftAllSanctions(types::EntityID imposer, types::EntityID target) {
        auto sanctions = GetActiveSanctionsAgainst(target);
        for (const auto& sanction : sanctions) {
            if (sanction.imposer == imposer) {
                LiftSanction(sanction.sanction_id);
            }
        }
    }

    std::vector<Sanction> DiplomacyEconomicBridge::GetActiveSanctionsAgainst(types::EntityID target) const {
        std::shared_lock<std::shared_mutex> lock(m_sanctions_mutex);
        std::vector<Sanction> result;

        auto it = m_sanctions_by_target.find(target);
        if (it == m_sanctions_by_target.end()) return result;

        for (const auto& sanction_id : it->second) {
            auto sanction_it = m_active_sanctions.find(sanction_id);
            if (sanction_it != m_active_sanctions.end()) {
                result.push_back(sanction_it->second);
            }
        }

        return result;
    }

    std::vector<Sanction> DiplomacyEconomicBridge::GetSanctionsImposedBy(types::EntityID imposer) const {
        std::shared_lock<std::shared_mutex> lock(m_sanctions_mutex);
        std::vector<Sanction> result;

        auto it = m_sanctions_by_imposer.find(imposer);
        if (it == m_sanctions_by_imposer.end()) return result;

        for (const auto& sanction_id : it->second) {
            auto sanction_it = m_active_sanctions.find(sanction_id);
            if (sanction_it != m_active_sanctions.end()) {
                result.push_back(sanction_it->second);
            }
        }

        return result;
    }

    bool DiplomacyEconomicBridge::IsUnderSanction(types::EntityID realm_id) const {
        auto it = m_sanctions_by_target.find(realm_id);
        return it != m_sanctions_by_target.end() && !it->second.empty();
    }

    double DiplomacyEconomicBridge::GetTotalSanctionImpact(types::EntityID realm_id) const {
        double total_impact = 0.0;

        auto sanctions = GetActiveSanctionsAgainst(realm_id);
        for (const auto& sanction : sanctions) {
            total_impact += sanction.GetEffectiveTradeReduction();
        }

        return std::min(1.0, total_impact); // Cap at 100%
    }

    // ====================================================================
    // Trade Agreements
    // ====================================================================

    std::string DiplomacyEconomicBridge::CreateTradeAgreement(
        types::EntityID realm_a, types::EntityID realm_b,
        double trade_bonus, int duration_years) {

        std::string agreement_id = GenerateAgreementId(realm_a, realm_b);

        TradeAgreement agreement(agreement_id, realm_a, realm_b);
        agreement.trade_bonus_multiplier = trade_bonus;
        agreement.duration_years = duration_years;
        agreement.years_remaining = duration_years;
        agreement.tariff_reduction = 0.5;
        agreement.opinion_bonus = 10;

        // Calculate monthly revenue bonus (10% of trade value)
        int trade_value = CalculateMonthlyTradeRevenue(realm_a, realm_b);
        agreement.monthly_revenue_bonus = static_cast<int>(trade_value * 0.1);

        // Store agreement
        m_trade_agreements[agreement_id] = agreement;
        m_agreements_by_realm[realm_a].push_back(agreement_id);
        m_agreements_by_realm[realm_b].push_back(agreement_id);

        // Apply effects
        ApplyTradeAgreementEffects(agreement);

        // Publish message
        messages::TradeAgreementEstablished msg;
        msg.agreement_id = agreement_id;
        msg.realm_a = realm_a;
        msg.realm_b = realm_b;
        msg.expected_trade_increase = (trade_bonus - 1.0) * 100.0; // As percentage
        msg.duration_years = duration_years;
        m_message_bus.Publish(msg);

        LogBridgeEvent("Trade agreement created: " + agreement_id);
        return agreement_id;
    }

    void DiplomacyEconomicBridge::TerminateTradeAgreement(const std::string& agreement_id) {
        auto it = m_trade_agreements.find(agreement_id);
        if (it == m_trade_agreements.end()) return;

        TradeAgreement& agreement = it->second;

        // Remove effects
        RemoveTradeAgreementEffects(agreement);

        // Publish message
        messages::TradeAgreementExpired msg;
        msg.agreement_id = agreement_id;
        msg.realm_a = agreement.realm_a;
        msg.realm_b = agreement.realm_b;
        msg.total_trade_value_generated = agreement.monthly_revenue_bonus *
                                          (agreement.duration_years - agreement.years_remaining) * 12;
        m_message_bus.Publish(msg);

        // Remove from tracking
        auto& realm_a_agreements = m_agreements_by_realm[agreement.realm_a];
        realm_a_agreements.erase(std::remove(realm_a_agreements.begin(), realm_a_agreements.end(), agreement_id),
                                realm_a_agreements.end());

        auto& realm_b_agreements = m_agreements_by_realm[agreement.realm_b];
        realm_b_agreements.erase(std::remove(realm_b_agreements.begin(), realm_b_agreements.end(), agreement_id),
                                realm_b_agreements.end());

        m_trade_agreements.erase(it);

        LogBridgeEvent("Trade agreement terminated: " + agreement_id);
    }

    void DiplomacyEconomicBridge::RenewTradeAgreement(const std::string& agreement_id, int additional_years) {
        auto it = m_trade_agreements.find(agreement_id);
        if (it == m_trade_agreements.end()) return;

        TradeAgreement& agreement = it->second;
        agreement.years_remaining += additional_years;
        agreement.duration_years += additional_years;

        LogBridgeEvent("Trade agreement renewed: " + agreement_id);
    }

    std::vector<TradeAgreement> DiplomacyEconomicBridge::GetTradeAgreements(types::EntityID realm_id) const {
        std::vector<TradeAgreement> result;

        auto it = m_agreements_by_realm.find(realm_id);
        if (it == m_agreements_by_realm.end()) return result;

        for (const auto& agreement_id : it->second) {
            auto agreement_it = m_trade_agreements.find(agreement_id);
            if (agreement_it != m_trade_agreements.end()) {
                result.push_back(agreement_it->second);
            }
        }

        return result;
    }

    std::optional<TradeAgreement> DiplomacyEconomicBridge::GetTradeAgreement(
        types::EntityID realm_a, types::EntityID realm_b) const {

        std::string agreement_id = GenerateAgreementId(realm_a, realm_b);
        auto it = m_trade_agreements.find(agreement_id);

        if (it != m_trade_agreements.end()) {
            return it->second;
        }

        // Try reverse order
        agreement_id = GenerateAgreementId(realm_b, realm_a);
        it = m_trade_agreements.find(agreement_id);

        if (it != m_trade_agreements.end()) {
            return it->second;
        }

        return std::nullopt;
    }

    bool DiplomacyEconomicBridge::HasTradeAgreement(types::EntityID realm_a, types::EntityID realm_b) const {
        return GetTradeAgreement(realm_a, realm_b).has_value();
    }

    double DiplomacyEconomicBridge::GetTradeAgreementBonus(
        types::EntityID realm_a, types::EntityID realm_b,
        types::ResourceType resource) const {

        auto agreement = GetTradeAgreement(realm_a, realm_b);
        if (!agreement.has_value()) return 1.0;

        return agreement->GetEffectiveTradeBonus(resource);
    }

    // ====================================================================
    // Economic Dependency Analysis
    // ====================================================================

    EconomicDependency DiplomacyEconomicBridge::CalculateDependency(
        types::EntityID realm_id, types::EntityID partner) const {

        EconomicDependency dep(realm_id, partner);

        // Calculate trade dependency
        double total_trade = GetTotalTradeRevenue(realm_id);
        double partner_trade = CalculateMonthlyTradeRevenue(realm_id, partner);

        if (total_trade > 0.0 && partner_trade > 0.0) {
            dep.trade_dependency = std::min(1.0, partner_trade / total_trade);
        } else if (total_trade == 0.0 && partner_trade == 0.0) {
            dep.trade_dependency = 0.0; // No trade, no dependency
        } else {
            // Edge case: partner has trade but realm has none
            dep.trade_dependency = 0.0;
        }

        // Calculate resource dependency (simplified)
        dep.resource_dependency = dep.trade_dependency * 0.7; // Estimate

        // Calculate financial dependency
        dep.financial_dependency = CalculateFinancialDependency(realm_id, partner);

        // Calculate overall dependency
        dep.CalculateOverallDependency();

        return dep;
    }

    void DiplomacyEconomicBridge::UpdateAllDependencies() {
        m_dependencies.clear();

        // Collect all realm IDs from trade agreements (realms that trade are candidates for dependencies)
        std::unordered_set<types::EntityID> all_realms;
        
        for (const auto& [agreement_id, agreement] : m_trade_agreements) {
            all_realms.insert(agreement.realm_a);
            all_realms.insert(agreement.realm_b);
        }
        
        // Also include realms involved in sanctions
        for (const auto& [sanction_id, sanction] : m_active_sanctions) {
            all_realms.insert(sanction.imposer);
            all_realms.insert(sanction.target);
        }

        // Calculate dependencies for each realm
        for (types::EntityID realm_id : all_realms) {
            UpdateDependenciesForRealm(realm_id);
        }
        
        LogBridgeEvent("Updated dependencies for " + std::to_string(all_realms.size()) + " realms");
    }

    void DiplomacyEconomicBridge::UpdateDependenciesForRealm(types::EntityID realm_id) {
        std::vector<EconomicDependency> dependencies;

        // Calculate dependencies for all trading partners from trade agreements
        std::unordered_set<types::EntityID> partners;
        
        // Find trading partners from agreements
        for (const auto& [agreement_id, agreement] : m_trade_agreements) {
            if (agreement.realm_a == realm_id) {
                partners.insert(agreement.realm_b);
            } else if (agreement.realm_b == realm_id) {
                partners.insert(agreement.realm_a);
            }
        }

        // Calculate dependency on each partner
        for (types::EntityID partner : partners) {
            EconomicDependency dep = CalculateDependency(realm_id, partner);
            if (dep.overall_dependency > 0.1) { // Only track significant dependencies
                dependencies.push_back(dep);
            }
        }

        // Sort by dependency level (highest first)
        std::sort(dependencies.begin(), dependencies.end(),
            [](const EconomicDependency& a, const EconomicDependency& b) {
                return a.overall_dependency > b.overall_dependency;
            });

        m_dependencies[realm_id] = dependencies;
    }

    std::vector<EconomicDependency> DiplomacyEconomicBridge::GetDependencies(types::EntityID realm_id) const {
        auto it = m_dependencies.find(realm_id);
        if (it != m_dependencies.end()) {
            return it->second;
        }
        return {};
    }

    std::vector<types::EntityID> DiplomacyEconomicBridge::GetCriticalTradingPartners(types::EntityID realm_id) const {
        std::vector<types::EntityID> critical_partners;

        auto dependencies = GetDependencies(realm_id);
        for (const auto& dep : dependencies) {
            if (dep.IsCriticallyDependent()) {
                critical_partners.push_back(dep.trading_partner);
            }
        }

        return critical_partners;
    }

    bool DiplomacyEconomicBridge::IsDependentOn(
        types::EntityID realm_id, types::EntityID partner, double threshold) const {

        auto dep = CalculateDependency(realm_id, partner);
        return dep.overall_dependency >= threshold;
    }

    double DiplomacyEconomicBridge::GetDependencyLevel(types::EntityID realm_id, types::EntityID partner) const {
        auto dep = CalculateDependency(realm_id, partner);
        return dep.overall_dependency;
    }

    // ====================================================================
    // War Economic Integration
    // ====================================================================

    void DiplomacyEconomicBridge::OnWarDeclared(types::EntityID aggressor, types::EntityID defender) {
        WarEconomicImpact war(aggressor, defender);

        // Disrupt trade routes
        DisruptTradeRoutesForWar(aggressor, defender);

        // Calculate initial costs
        war.UpdateMonthlyCosts();

        m_active_wars.push_back(war);

        LogBridgeEvent("War economic impact tracking started");
    }

    void DiplomacyEconomicBridge::OnPeaceTreaty(types::EntityID realm_a, types::EntityID realm_b) {
        // Find and remove war
        auto it = std::find_if(m_active_wars.begin(), m_active_wars.end(),
            [realm_a, realm_b](const WarEconomicImpact& war) {
                return (war.aggressor == realm_a && war.defender == realm_b) ||
                       (war.aggressor == realm_b && war.defender == realm_a);
            });

        if (it != m_active_wars.end()) {
            // Restore trade routes
            RestoreTradeRoutesAfterPeace(realm_a, realm_b);

            LogBridgeEvent("War ended - total cost: " + std::to_string(it->GetTotalWarCost()));
            m_active_wars.erase(it);
        }
    }

    void DiplomacyEconomicBridge::ProcessWarEconomics() {
        if (!m_economic_system) {
            LogBridgeEvent("Warning: EconomicSystem not set, cannot process war economics");
            return;
        }

        for (auto& war : m_active_wars) {
            war.UpdateMonthlyCosts();

            // Apply costs to treasuries using EconomicSystem API
            auto* aggressor_econ = GetEconomicComponent(war.aggressor);
            auto* defender_econ = GetEconomicComponent(war.defender);

            if (aggressor_econ) {
                if (m_economic_system->SpendMoney(war.aggressor, war.monthly_war_cost)) {
                    aggressor_econ->monthly_expenses += war.monthly_war_cost;
                } else {
                    LogBridgeEvent("Warning: Aggressor cannot afford war costs");
                }
            }

            if (defender_econ) {
                if (m_economic_system->SpendMoney(war.defender, war.monthly_war_cost)) {
                    defender_econ->monthly_expenses += war.monthly_war_cost;
                } else {
                    LogBridgeEvent("Warning: Defender cannot afford war costs");
                }
            }
        }
    }

    WarEconomicImpact* DiplomacyEconomicBridge::GetWarImpact(types::EntityID aggressor, types::EntityID defender) {
        auto it = std::find_if(m_active_wars.begin(), m_active_wars.end(),
            [aggressor, defender](const WarEconomicImpact& war) {
                return (war.aggressor == aggressor && war.defender == defender) ||
                       (war.aggressor == defender && war.defender == aggressor);
            });

        if (it != m_active_wars.end()) {
            return &(*it);
        }
        return nullptr;
    }

    int DiplomacyEconomicBridge::GetMonthlyWarCost(types::EntityID realm_id) const {
        int total_cost = 0;

        for (const auto& war : m_active_wars) {
            if (war.aggressor == realm_id || war.defender == realm_id) {
                total_cost += war.monthly_war_cost;
            }
        }

        return total_cost;
    }

    std::vector<std::string> DiplomacyEconomicBridge::GetDisruptedTradeRoutes(types::EntityID realm_id) const {
        std::vector<std::string> disrupted;

        for (const auto& war : m_active_wars) {
            if (war.aggressor == realm_id || war.defender == realm_id) {
                disrupted.insert(disrupted.end(),
                               war.disrupted_trade_routes.begin(),
                               war.disrupted_trade_routes.end());
            }
        }

        return disrupted;
    }

    // ====================================================================
    // Diplomatic Event -> Economic Impact
    // ====================================================================

    void DiplomacyEconomicBridge::OnAllianceFormed(types::EntityID realm_a, types::EntityID realm_b) {
        // Create trade agreement with alliance bonus
        CreateTradeAgreement(realm_a, realm_b, 1.3, 20); // 30% trade bonus, 20 years

        LogBridgeEvent("Alliance economic benefits applied");
    }

    void DiplomacyEconomicBridge::OnAllianceBroken(types::EntityID realm_a, types::EntityID realm_b) {
        // Terminate trade agreement
        auto agreement = GetTradeAgreement(realm_a, realm_b);
        if (agreement.has_value()) {
            TerminateTradeAgreement(agreement->agreement_id);
        }

        LogBridgeEvent("Alliance economic benefits removed");
    }

    void DiplomacyEconomicBridge::OnTreatyViolation(types::EntityID violator, types::EntityID victim) {
        // Impose sanctions
        ImposeSanction(victim, violator,
                      SanctionType::DIPLOMATIC_ISOLATION,
                      SanctionSeverity::MODERATE,
                      "Treaty violation");

        LogBridgeEvent("Sanctions imposed for treaty violation");
    }

    void DiplomacyEconomicBridge::OnDiplomaticGift(types::EntityID sender, types::EntityID recipient, int value) {
        if (!m_economic_system) {
            LogBridgeEvent("Warning: EconomicSystem not set, cannot process diplomatic gift");
            return;
        }

        // Use EconomicSystem API for treasury operations
        if (m_economic_system->SpendMoney(sender, value)) {
            m_economic_system->AddMoney(recipient, value);
            LogBridgeEvent("Diplomatic gift processed: " + std::to_string(value));
        } else {
            LogBridgeEvent("Warning: Sender cannot afford diplomatic gift of " + std::to_string(value));
        }
    }

    void DiplomacyEconomicBridge::ApplyTreatyEconomicEffects(
        types::EntityID realm_id, diplomacy::TreatyType treaty_type) {

        auto* econ = GetEconomicComponent(realm_id);
        if (!econ) return;

        switch (treaty_type) {
            case diplomacy::TreatyType::TRADE_AGREEMENT:
                econ->trade_efficiency *= 1.2;
                break;
            case diplomacy::TreatyType::ALLIANCE:
                econ->trade_efficiency *= 1.1;
                break;
            default:
                break;
        }
    }

    void DiplomacyEconomicBridge::RemoveTreatyEconomicEffects(
        types::EntityID realm_id, diplomacy::TreatyType treaty_type) {

        auto* econ = GetEconomicComponent(realm_id);
        if (!econ) return;

        switch (treaty_type) {
            case diplomacy::TreatyType::TRADE_AGREEMENT:
                econ->trade_efficiency /= 1.2;
                break;
            case diplomacy::TreatyType::ALLIANCE:
                econ->trade_efficiency /= 1.1;
                break;
            default:
                break;
        }
    }

    // ====================================================================
    // Economic Event -> Diplomatic Impact
    // ====================================================================

    void DiplomacyEconomicBridge::OnTradeRouteDisrupted(
        types::EntityID source, types::EntityID destination, types::ResourceType resource) {

        auto* diplo = GetDiplomacyComponent(source);
        if (!diplo) return;

        // Reduce opinion if trading partner
        for (auto& [other_realm, state] : diplo->relationships) {
            if (other_realm == destination) {
                state.opinion -= 10;
                break;
            }
        }

        LogBridgeEvent("Trade disruption affected diplomatic relations");
    }

    void DiplomacyEconomicBridge::OnEconomicCrisis(
        types::EntityID realm_id, const std::string& crisis_type, double severity) {

        auto* diplo = GetDiplomacyComponent(realm_id);
        if (!diplo) return;

        // Economic crisis reduces prestige
        diplo->prestige -= severity * 10.0;

        // May strain relationships
        for (auto& [other_realm, state] : diplo->relationships) {
            if (state.trade_volume > 100.0) {
                state.opinion -= static_cast<int>(severity * 5);
            }
        }

        LogBridgeEvent("Economic crisis affected diplomatic standing");
    }

    void DiplomacyEconomicBridge::OnResourceShortage(
        types::EntityID realm_id, types::ResourceType resource, double severity) {

        // Severe shortage may lead to trade disputes or wars
        if (severity > 0.8) {
            LogBridgeEvent("Critical resource shortage may trigger diplomatic action");
        }
    }

    void DiplomacyEconomicBridge::AdjustOpinionBasedOnTrade(types::EntityID realm_a, types::EntityID realm_b) {
        auto* diplo_a = GetDiplomacyComponent(realm_a);
        if (!diplo_a) return;

        double trade_value = CalculateTradeValue(realm_a, realm_b);
        int opinion_bonus = static_cast<int>(trade_value * m_trade_opinion_modifier / 100.0);

        for (auto& [other_realm, state] : diplo_a->relationships) {
            if (other_realm == realm_b) {
                state.opinion += opinion_bonus;
                state.trade_volume = trade_value;
                break;
            }
        }
    }

    void DiplomacyEconomicBridge::ProcessEconomicInfluenceOnRelations() {
        // Update all realm diplomatic opinions based on trade
        // (Simplified - would iterate through actual realm entities)
    }

    // ====================================================================
    // Integration Utilities
    // ====================================================================

    double DiplomacyEconomicBridge::CalculateTradeValue(types::EntityID realm_a, types::EntityID realm_b) const {
        auto* econ_a = GetEconomicComponent(realm_a);
        auto* econ_b = GetEconomicComponent(realm_b);

        if (!econ_a || !econ_b) return 0.0;

        // Simplified trade value calculation
        double base_value = (econ_a->trade_income + econ_b->trade_income) / 20.0;

        // Apply trade agreement bonuses
        double agreement_bonus = GetTradeAgreementBonus(realm_a, realm_b, types::ResourceType::FOOD);

        // Apply sanction penalties
        double sanction_penalty_a = GetTotalSanctionImpact(realm_a);
        double sanction_penalty_b = GetTotalSanctionImpact(realm_b);
        double total_penalty = std::max(sanction_penalty_a, sanction_penalty_b);

        return base_value * agreement_bonus * (1.0 - total_penalty);
    }

    double DiplomacyEconomicBridge::CalculateEconomicLeverage(
        types::EntityID realm_id, types::EntityID target) const {

        // How much economic pressure can realm_id apply to target?
        double target_dependency = GetDependencyLevel(target, realm_id);
        double realm_dependency = GetDependencyLevel(realm_id, target);

        // Leverage is the difference in dependencies
        return target_dependency - realm_dependency;
    }

    bool DiplomacyEconomicBridge::WouldWarHurtEconomy(
        types::EntityID aggressor, types::EntityID target) const {

        double trade_value = CalculateTradeValue(aggressor, target);
        double dependency = GetDependencyLevel(aggressor, target);

        // War would hurt economy if significant trade or dependency
        return trade_value > 200.0 || dependency > 0.4;
    }

    int DiplomacyEconomicBridge::EstimateTributePotential(
        types::EntityID stronger, types::EntityID weaker) const {

        auto* weaker_econ = GetEconomicComponent(weaker);
        if (!weaker_econ) return 0;

        // Estimate 10% of monthly income as tribute potential
        return weaker_econ->monthly_income / 10;
    }

    bool DiplomacyEconomicBridge::ShouldAvoidWarForEconomicReasons(
        types::EntityID realm_id, types::EntityID target) const {

        return WouldWarHurtEconomy(realm_id, target);
    }

    bool DiplomacyEconomicBridge::ShouldFormAllianceForTrade(
        types::EntityID realm_id, types::EntityID potential_ally) const {

        double potential_trade = CalculateTradeValue(realm_id, potential_ally);
        return potential_trade > 100.0;
    }

    std::vector<types::EntityID> DiplomacyEconomicBridge::GetBestTradePartnerCandidates(
        types::EntityID realm_id, int count) const {

        // Simplified - would rank potential partners by trade value
        std::vector<types::EntityID> candidates;
        return candidates;
    }

    double DiplomacyEconomicBridge::GetTotalTradeRevenue(types::EntityID realm_id) const {
        auto* econ = GetEconomicComponent(realm_id);
        if (!econ) return 0.0;
        return static_cast<double>(econ->trade_income);
    }

    int DiplomacyEconomicBridge::GetTradePartnerCount(types::EntityID realm_id) const {
        auto* econ = GetEconomicComponent(realm_id);
        if (!econ) return 0;
        return static_cast<int>(econ->active_trade_routes.size());
    }

    double DiplomacyEconomicBridge::GetAverageDependencyLevel(types::EntityID realm_id) const {
        auto dependencies = GetDependencies(realm_id);
        if (dependencies.empty()) return 0.0;

        double total = 0.0;
        for (const auto& dep : dependencies) {
            total += dep.overall_dependency;
        }

        return total / dependencies.size();
    }

    // ====================================================================
    // Private Implementation
    // ====================================================================

    void DiplomacyEconomicBridge::SubscribeToEvents() {
        // Subscribe to monthly tick events from TimeManagementSystem
        m_message_bus.Subscribe<game::time::messages::TickOccurred>(
            [this](const game::time::messages::TickOccurred& event) {
                // Only process monthly ticks
                if (event.tick_type == game::time::TickType::MONTHLY) {
                    CORE_LOG_DEBUG("DiplomacyEconomicBridge", "Processing monthly updates at " +
                                  std::to_string(event.current_date.year) + "-" +
                                  std::to_string(event.current_date.month));
                    ProcessMonthlyUpdates();
                }
            }
        );

        // TODO: Subscribe to other diplomacy and economic system events via message bus
    }

    void DiplomacyEconomicBridge::LoadConfiguration() {
        // Load configuration from file or use defaults
    }

    void DiplomacyEconomicBridge::ProcessRegularUpdates(float delta_time) {
        // Regular per-second updates
    }

    void DiplomacyEconomicBridge::ProcessMonthlyUpdates() {
        UpdateSanctions();
        UpdateTradeAgreements();
        UpdateDependencies();
        ProcessWarEconomics();
        ProcessEconomicInfluenceOnRelations();
    }

    void DiplomacyEconomicBridge::UpdateSanctions() {
        if (!m_economic_system) {
            return;  // Cannot apply sanction damage without EconomicSystem
        }

        std::vector<std::string> expired_sanctions;

        for (auto& [id, sanction] : m_active_sanctions) {
            sanction.months_elapsed++;

            if (sanction.IsExpired()) {
                expired_sanctions.push_back(id);
            } else {
                // Apply monthly damage using EconomicSystem API
                auto* econ = GetEconomicComponent(sanction.target);
                if (econ && sanction.monthly_economic_damage > 0) {
                    m_economic_system->SpendMoney(sanction.target, sanction.monthly_economic_damage);
                }
            }
        }

        // Remove expired sanctions
        for (const auto& id : expired_sanctions) {
            LiftSanction(id);
        }
    }

    void DiplomacyEconomicBridge::UpdateTradeAgreements() {
        if (!m_economic_system) {
            return;  // Cannot apply trade agreement bonuses without EconomicSystem
        }

        std::vector<std::string> expired_agreements;

        for (auto& [id, agreement] : m_trade_agreements) {
            agreement.years_remaining--;

            if (agreement.years_remaining <= 0) {
                if (agreement.auto_renew) {
                    agreement.years_remaining = agreement.duration_years;
                } else {
                    expired_agreements.push_back(id);
                }
            } else {
                // Apply monthly revenue bonus using EconomicSystem API
                auto* econ_a = GetEconomicComponent(agreement.realm_a);
                auto* econ_b = GetEconomicComponent(agreement.realm_b);

                if (econ_a && agreement.monthly_revenue_bonus > 0) {
                    m_economic_system->AddMoney(agreement.realm_a, agreement.monthly_revenue_bonus);
                }
                if (econ_b && agreement.monthly_revenue_bonus > 0) {
                    m_economic_system->AddMoney(agreement.realm_b, agreement.monthly_revenue_bonus);
                }
            }
        }

        // Remove expired agreements
        for (const auto& id : expired_agreements) {
            TerminateTradeAgreement(id);
        }
    }

    void DiplomacyEconomicBridge::UpdateDependencies() {
        // Recalculate dependencies for all realms
        // (Would iterate through actual realm entities)
    }

    std::string DiplomacyEconomicBridge::GenerateSanctionId(types::EntityID imposer, types::EntityID target) const {
        return "SANCTION_" + std::to_string(imposer) + "_" + std::to_string(target) +
               "_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    }

    std::string DiplomacyEconomicBridge::GenerateAgreementId(types::EntityID realm_a, types::EntityID realm_b) const {
        // Ensure consistent ordering
        types::EntityID first = std::min(realm_a, realm_b);
        types::EntityID second = std::max(realm_a, realm_b);
        return "AGREEMENT_" + std::to_string(first) + "_" + std::to_string(second);
    }

    void DiplomacyEconomicBridge::ApplySanctionEffects(const Sanction& sanction) {
        auto* econ = GetEconomicComponent(sanction.target);
        if (!econ) return;

        // Store baseline if this is the first sanction
        auto it = m_sanction_baselines.find(sanction.target);
        if (it == m_sanction_baselines.end()) {
            m_sanction_baselines[sanction.target] = econ->trade_efficiency;
        }

        // Reduce trade efficiency
        econ->trade_efficiency *= (1.0 - sanction.GetEffectiveTradeReduction() * 0.5);

        // Adjust diplomatic opinion
        auto* diplo = GetDiplomacyComponent(sanction.target);
        if (diplo) {
            for (auto& [other_realm, state] : diplo->relationships) {
                if (other_realm == sanction.imposer) {
                    state.opinion += sanction.opinion_modifier;
                    break;
                }
            }
        }
    }

    void DiplomacyEconomicBridge::RemoveSanctionEffects(const Sanction& sanction) {
        auto* econ = GetEconomicComponent(sanction.target);
        if (!econ) return;

        // Calculate what the efficiency should be without this sanction
        // by removing its contribution multiplicatively
        double reduction_factor = (1.0 - sanction.GetEffectiveTradeReduction() * 0.5);
        if (reduction_factor > 0.0) {
            econ->trade_efficiency /= reduction_factor;
        }

        // If no more sanctions, restore baseline
        auto sanctions = GetActiveSanctionsAgainst(sanction.target);
        if (sanctions.empty()) {
            auto baseline_it = m_sanction_baselines.find(sanction.target);
            if (baseline_it != m_sanction_baselines.end()) {
                econ->trade_efficiency = baseline_it->second;
                m_sanction_baselines.erase(baseline_it);
            }
        }

        // Restore diplomatic opinion
        auto* diplo = GetDiplomacyComponent(sanction.target);
        if (diplo) {
            for (auto& [other_realm, state] : diplo->relationships) {
                if (other_realm == sanction.imposer) {
                    state.opinion -= sanction.opinion_modifier;
                    break;
                }
            }
        }
    }

    void DiplomacyEconomicBridge::ApplyTradeAgreementEffects(const TradeAgreement& agreement) {
        auto* econ_a = GetEconomicComponent(agreement.realm_a);
        auto* econ_b = GetEconomicComponent(agreement.realm_b);

        if (econ_a) econ_a->trade_efficiency *= agreement.trade_bonus_multiplier;
        if (econ_b) econ_b->trade_efficiency *= agreement.trade_bonus_multiplier;

        // Improve diplomatic relations
        auto* diplo_a = GetDiplomacyComponent(agreement.realm_a);
        if (diplo_a) {
            for (auto& [other_realm, state] : diplo_a->relationships) {
                if (other_realm == agreement.realm_b) {
                    state.opinion += agreement.opinion_bonus;
                    break;
                }
            }
        }
    }

    void DiplomacyEconomicBridge::RemoveTradeAgreementEffects(const TradeAgreement& agreement) {
        auto* econ_a = GetEconomicComponent(agreement.realm_a);
        auto* econ_b = GetEconomicComponent(agreement.realm_b);

        if (econ_a) econ_a->trade_efficiency /= agreement.trade_bonus_multiplier;
        if (econ_b) econ_b->trade_efficiency /= agreement.trade_bonus_multiplier;

        // Remove diplomatic bonus
        auto* diplo_a = GetDiplomacyComponent(agreement.realm_a);
        if (diplo_a) {
            for (auto& [other_realm, state] : diplo_a->relationships) {
                if (other_realm == agreement.realm_b) {
                    state.opinion -= agreement.opinion_bonus;
                    break;
                }
            }
        }
    }

    economy::EconomicComponent* DiplomacyEconomicBridge::GetEconomicComponent(types::EntityID realm_id) {
        // Note: GetComponent returns ComponentAccessResult which provides const access
        // Returning const pointer to maintain thread safety
        return const_cast<economy::EconomicComponent*>(
            m_access_manager.GetComponent<economy::EconomicComponent>(realm_id).Get()
        );
    }

    const economy::EconomicComponent* DiplomacyEconomicBridge::GetEconomicComponent(types::EntityID realm_id) const {
        return m_access_manager.GetComponent<economy::EconomicComponent>(realm_id).Get();
    }

    diplomacy::DiplomacyComponent* DiplomacyEconomicBridge::GetDiplomacyComponent(types::EntityID realm_id) {
        // Note: GetComponent returns ComponentAccessResult which provides const access
        // Returning const pointer to maintain thread safety
        return const_cast<diplomacy::DiplomacyComponent*>(
            m_access_manager.GetComponent<diplomacy::DiplomacyComponent>(realm_id).Get()
        );
    }

    const diplomacy::DiplomacyComponent* DiplomacyEconomicBridge::GetDiplomacyComponent(types::EntityID realm_id) const {
        return m_access_manager.GetComponent<diplomacy::DiplomacyComponent>(realm_id).Get();
    }

    double DiplomacyEconomicBridge::CalculateTradeVolume(types::EntityID realm_a, types::EntityID realm_b) const {
        // Simplified trade volume calculation
        return CalculateTradeValue(realm_a, realm_b);
    }

    double DiplomacyEconomicBridge::CalculateResourceDependency(
        types::EntityID realm_id, types::EntityID partner) const {

        // Simplified - would analyze resource imports
        return 0.3;
    }

    double DiplomacyEconomicBridge::CalculateFinancialDependency(
        types::EntityID realm_id, types::EntityID partner) const {

        // Simplified - would analyze loans and tribute
        return 0.1;
    }

    int DiplomacyEconomicBridge::CalculateMonthlyTradeRevenue(
        types::EntityID realm_id, types::EntityID partner) const {

        double trade_value = CalculateTradeValue(realm_id, partner);
        return static_cast<int>(trade_value * 0.1); // 10% as revenue
    }

    void DiplomacyEconomicBridge::CalculateWarCosts(WarEconomicImpact& war) {
        war.UpdateMonthlyCosts();
    }

    void DiplomacyEconomicBridge::DisruptTradeRoutesForWar(types::EntityID aggressor, types::EntityID defender) {
        // Find active war
        for (auto& war : m_active_wars) {
            if ((war.aggressor == aggressor && war.defender == defender) ||
                (war.aggressor == defender && war.defender == aggressor)) {

                // Mark trade routes as disrupted
                auto* econ_a = GetEconomicComponent(aggressor);
                auto* econ_b = GetEconomicComponent(defender);

                if (econ_a) {
                    for (auto& route : econ_a->active_trade_routes) {
                        if (route.to_province == defender || route.from_province == defender) {
                            route.is_active = false;
                            war.disrupted_trade_routes.push_back(
                                "route_" + std::to_string(route.from_province) + "_" + std::to_string(route.to_province)
                            );
                        }
                    }
                }

                if (econ_b) {
                    for (auto& route : econ_b->active_trade_routes) {
                        if (route.to_province == aggressor || route.from_province == aggressor) {
                            route.is_active = false;
                        }
                    }
                }

                break;
            }
        }
    }

    void DiplomacyEconomicBridge::RestoreTradeRoutesAfterPeace(types::EntityID realm_a, types::EntityID realm_b) {
        auto* econ_a = GetEconomicComponent(realm_a);
        auto* econ_b = GetEconomicComponent(realm_b);

        if (econ_a) {
            for (auto& route : econ_a->active_trade_routes) {
                if (route.to_province == realm_b || route.from_province == realm_b) {
                    route.is_active = true;
                }
            }
        }

        if (econ_b) {
            for (auto& route : econ_b->active_trade_routes) {
                if (route.to_province == realm_a || route.from_province == realm_a) {
                    route.is_active = true;
                }
            }
        }
    }

    std::vector<types::EntityID> DiplomacyEconomicBridge::GetAffectedNeutralParties(
        types::EntityID realm_a, types::EntityID realm_b) const {

        // Find neutral parties that trade with both warring parties
        std::vector<types::EntityID> affected;
        // (Simplified - would search through actual realm entities)
        return affected;
    }

    void DiplomacyEconomicBridge::HandleDiplomaticEvent(
        const std::string& event_type, types::EntityID realm_a, types::EntityID realm_b) {

        // Route to appropriate handler
        if (event_type == "alliance_formed") {
            OnAllianceFormed(realm_a, realm_b);
        } else if (event_type == "alliance_broken") {
            OnAllianceBroken(realm_a, realm_b);
        } else if (event_type == "war_declared") {
            OnWarDeclared(realm_a, realm_b);
        } else if (event_type == "peace_treaty") {
            OnPeaceTreaty(realm_a, realm_b);
        }
    }

    void DiplomacyEconomicBridge::HandleEconomicEvent(const std::string& event_type, types::EntityID realm_id) {
        // Route to appropriate handler
        if (event_type == "economic_crisis") {
            OnEconomicCrisis(realm_id, "recession", 0.7);
        }
    }

    void DiplomacyEconomicBridge::LogBridgeEvent(const std::string& message) const {
        // Log event (could use proper logging system)
        // CORE_STREAM_INFO("DiplomacyEconomicBridge") << "" << message;
    }

    void DiplomacyEconomicBridge::ValidateBridgeState() const {
        // Validate internal consistency
    }

} // namespace game::bridge
