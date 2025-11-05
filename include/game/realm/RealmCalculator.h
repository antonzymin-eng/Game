// ============================================================================
// Mechanica Imperii - Realm Calculator Header
// Pure Calculation Functions for Realm System
// ============================================================================

#pragma once

#include "game/realm/RealmComponents.h"
#include "game/diplomacy/DiplomacyComponents.h"
#include <vector>
#include <algorithm>

namespace game::realm {

    // Forward declaration
    using game::diplomacy::DiplomaticAction;

    /**
     * @brief Pure calculation functions for realm system
     * All functions are static with no side effects
     */
    class RealmCalculator {
    public:
        // Realm strength and power calculations
        static float CalculateRealmPower(const RealmComponent& realm);
        static float CalculateMilitaryStrength(const RealmComponent& realm);
        static float CalculateEconomicStrength(const RealmComponent& realm);
        static float CalculatePoliticalStrength(const RealmComponent& realm);

        // Rank calculations
        static RealmRank DetermineRealmRank(size_t provinceCount);
        static float GetRankMultiplier(RealmRank rank);

        // Succession calculations
        static std::vector<types::EntityID> GetValidHeirs(
            const RealmComponent& realm,
            SuccessionLaw law);
        static float CalculateSuccessionStability(SuccessionLaw law);
        static float CalculateLegitimacyChange(SuccessionLaw law);

        // War calculations
        static float CalculateWarConsequences(float warscore);
        static size_t CalculateProvinceTransfer(size_t totalProvinces, float warscore);
        static double CalculateWarReparations(double treasury, float warscore);
        static float CalculateStabilityLoss(bool isWinner, float warscore);

        // Diplomatic calculations
        static float CalculateOpinionChange(DiplomaticAction action);
        static bool CanDeclareWar(const RealmComponent& aggressor, const RealmComponent& defender);
        static float CalculateAllianceValue(const RealmComponent& realm1, const RealmComponent& realm2);

        // Authority and legitimacy
        static float CalculateCentralAuthority(CrownAuthority authority);
        static float CalculateLegitimacyByGovernment(GovernmentType government);

        // Economic calculations
        static double CalculateMonthlyIncome(const RealmComponent& realm);
        static double CalculateTaxRevenue(const RealmComponent& realm, const LawsComponent& laws);

        // Utility functions
        static float Clamp(float value, float min_val, float max_val);
    };

} // namespace game::realm
