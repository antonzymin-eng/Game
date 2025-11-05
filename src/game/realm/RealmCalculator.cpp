// ============================================================================
// Mechanica Imperii - Realm Calculator Implementation
// ============================================================================

#include "game/realm/RealmCalculator.h"
#include <cmath>

namespace game::realm {

    float RealmCalculator::CalculateRealmPower(const RealmComponent& realm) {
        return CalculateMilitaryStrength(realm) * 0.5f +
               CalculateEconomicStrength(realm) * 0.3f +
               CalculatePoliticalStrength(realm) * 0.2f;
    }

    float RealmCalculator::CalculateMilitaryStrength(const RealmComponent& realm) {
        return static_cast<float>(realm.levySize + realm.standingArmy) * realm.stability;
    }

    float RealmCalculator::CalculateEconomicStrength(const RealmComponent& realm) {
        return static_cast<float>(realm.treasury + realm.monthlyIncome * 12.0) * 0.01f;
    }

    float RealmCalculator::CalculatePoliticalStrength(const RealmComponent& realm) {
        return realm.legitimacy * realm.centralAuthority * realm.stability * 100.0f;
    }

    RealmRank RealmCalculator::DetermineRealmRank(size_t provinceCount) {
        if (provinceCount >= 80) return RealmRank::EMPIRE;
        if (provinceCount >= 30) return RealmRank::KINGDOM;
        if (provinceCount >= 10) return RealmRank::DUCHY;
        if (provinceCount >= 3) return RealmRank::COUNTY;
        return RealmRank::BARONY;
    }

    float RealmCalculator::GetRankMultiplier(RealmRank rank) {
        switch (rank) {
        case RealmRank::BARONY: return 1.0f;
        case RealmRank::COUNTY: return 1.5f;
        case RealmRank::DUCHY: return 2.5f;
        case RealmRank::KINGDOM: return 4.0f;
        case RealmRank::EMPIRE: return 6.0f;
        default: return 1.0f;
        }
    }

    std::vector<types::EntityID> RealmCalculator::GetValidHeirs(
        const RealmComponent& realm,
        SuccessionLaw law) {
        // Return empty vector for now - heirs system not yet implemented
        // TODO: Implement heir selection based on succession law
        return std::vector<types::EntityID>();
    }

    float RealmCalculator::CalculateSuccessionStability(SuccessionLaw law) {
        switch (law) {
        case SuccessionLaw::PRIMOGENITURE:
        case SuccessionLaw::ULTIMOGENITURE:
            return 0.95f;
        case SuccessionLaw::ELECTIVE:
            return 0.95f;
        case SuccessionLaw::GAVELKIND:
            return 0.80f;
        case SuccessionLaw::TANISTRY:
            return 0.90f;
        default:
            return 0.90f;
        }
    }

    float RealmCalculator::CalculateLegitimacyChange(SuccessionLaw law) {
        switch (law) {
        case SuccessionLaw::ELECTIVE:
            return 1.0f; // Full legitimacy
        case SuccessionLaw::PRIMOGENITURE:
        case SuccessionLaw::ULTIMOGENITURE:
            return 0.95f;
        case SuccessionLaw::GAVELKIND:
            return 0.90f;
        default:
            return 0.90f;
        }
    }

    float RealmCalculator::CalculateWarConsequences(float warscore) {
        return std::abs(warscore) / 100.0f;
    }

    size_t RealmCalculator::CalculateProvinceTransfer(size_t totalProvinces, float warscore) {
        if (warscore > 75.0f) {
            return totalProvinces; // Total conquest
        } else if (warscore > 50.0f) {
            return totalProvinces / 3; // Major victory
        } else if (warscore > 25.0f) {
            return totalProvinces / 10; // Minor victory
        }
        return 0;
    }

    double RealmCalculator::CalculateWarReparations(double treasury, float warscore) {
        return treasury * (warscore / 100.0) * 0.5;
    }

    float RealmCalculator::CalculateStabilityLoss(bool isWinner, float warscore) {
        if (isWinner) {
            return 0.1f; // Winners gain stability
        } else {
            return -0.3f * (warscore / 100.0f); // Losers lose stability
        }
    }

    float RealmCalculator::CalculateOpinionChange(DiplomaticAction action) {
        // Simplified opinion calculations
        return 0.0f;
    }

    bool RealmCalculator::CanDeclareWar(const RealmComponent& aggressor, const RealmComponent& defender) {
        // Basic war declaration checks
        // Cannot declare war on self
        if (aggressor.realmId == defender.realmId) {
            return false;
        }

        // Require minimum stability to declare war
        if (aggressor.stability < 0.3f) {
            return false;
        }

        // Cannot declare war if already at war (simplified check)
        // TODO: Implement proper war state tracking

        return true;
    }

    float RealmCalculator::CalculateAllianceValue(const RealmComponent& realm1, const RealmComponent& realm2) {
        float power1 = CalculateRealmPower(realm1);
        float power2 = CalculateRealmPower(realm2);
        return (power1 + power2) / 2.0f;
    }

    float RealmCalculator::CalculateCentralAuthority(CrownAuthority authority) {
        switch (authority) {
        case CrownAuthority::MINIMAL: return 0.2f;
        case CrownAuthority::LOW: return 0.4f;
        case CrownAuthority::MEDIUM: return 0.6f;
        case CrownAuthority::HIGH: return 0.8f;
        case CrownAuthority::ABSOLUTE: return 1.0f;
        default: return 0.5f;
        }
    }

    float RealmCalculator::CalculateLegitimacyByGovernment(GovernmentType government) {
        switch (government) {
        case GovernmentType::THEOCRACY: return 1.0f;
        case GovernmentType::REPUBLIC: return 0.9f;
        case GovernmentType::FEUDAL_MONARCHY: return 0.8f;
        case GovernmentType::ABSOLUTE_MONARCHY: return 0.7f;
        case GovernmentType::TRIBAL: return 0.6f;
        default: return 0.8f;
        }
    }

    double RealmCalculator::CalculateMonthlyIncome(const RealmComponent& realm) {
        return realm.monthlyIncome;
    }

    double RealmCalculator::CalculateTaxRevenue(const RealmComponent& realm, const LawsComponent& laws) {
        double baseRevenue = static_cast<double>(realm.ownedProvinces.size()) * 100.0;
        double taxMultiplier = (laws.baseTaxRate + laws.nobleTaxRate + laws.merchantTaxRate + laws.peasantTaxRate) / 4.0;
        return baseRevenue * taxMultiplier;
    }

    float RealmCalculator::Clamp(float value, float min_val, float max_val) {
        return std::max(min_val, std::min(value, max_val));
    }

} // namespace game::realm
