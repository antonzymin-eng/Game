// ============================================================================
// Mechanica Imperii - Population Calculator Implementation
// ============================================================================

#include "game/population/PopulationCalculator.h"
#include "game/population/PopulationSystem.h"
#include <cmath>
#include <unordered_map>

namespace game::population {

    // ========================================================================
    // Historical Demographics
    // ========================================================================

    double PopulationCalculator::GetHistoricalPercentage(SocialClass social_class, int year, double prosperity_level) {
        // Base percentages for medieval society around 1200 AD
        std::unordered_map<SocialClass, double> base_percentages = {
            {SocialClass::HIGH_NOBILITY, 0.001},
            {SocialClass::LESSER_NOBILITY, 0.015},
            {SocialClass::HIGH_CLERGY, 0.005},
            {SocialClass::CLERGY, 0.025},
            {SocialClass::WEALTHY_MERCHANTS, 0.01},
            {SocialClass::BURGHERS, 0.03},
            {SocialClass::GUILD_MASTERS, 0.008},
            {SocialClass::CRAFTSMEN, 0.06},
            {SocialClass::SCHOLARS, 0.003},
            {SocialClass::FREE_PEASANTS, 0.25},
            {SocialClass::VILLEINS, 0.35},
            {SocialClass::SERFS, 0.25},
            {SocialClass::URBAN_LABORERS, 0.08}
        };

        double base_percentage = base_percentages[social_class];

        // Adjust for prosperity level
        if (IsWealthyClass(social_class)) {
            base_percentage *= (1.0 + prosperity_level * 0.5);
        }

        // Adjust for historical period
        if (year < 1100) {
            // Earlier medieval period - more serfs, fewer merchants
            if (social_class == SocialClass::SERFS || social_class == SocialClass::VILLEINS) {
                base_percentage *= 1.2;
            } else if (social_class == SocialClass::WEALTHY_MERCHANTS || social_class == SocialClass::BURGHERS) {
                base_percentage *= 0.7;
            }
        } else if (year > 1300) {
            // Later medieval period - more free peasants and merchants
            if (social_class == SocialClass::FREE_PEASANTS || social_class == SocialClass::WEALTHY_MERCHANTS) {
                base_percentage *= 1.3;
            } else if (social_class == SocialClass::SERFS) {
                base_percentage *= 0.8;
            }
        }

        return base_percentage;
    }

    double PopulationCalculator::CalculateUrbanizationRate(int total_population, double prosperity_level, int year) {
        double base_rate = MEDIEVAL_URBANIZATION_BASE;

        // Prosperity impact
        base_rate += prosperity_level * 0.12; // Up to 20% urbanization with high prosperity

        // Historical period impact
        if (year > 1200) {
            base_rate *= 1.2; // Commercial revolution
        } else if (year < 1000) {
            base_rate *= 0.6; // Early medieval low urbanization
        }

        // Population size impact (larger populations support more urbanization)
        if (total_population > 100000) {
            base_rate *= 1.3;
        } else if (total_population > 50000) {
            base_rate *= 1.15;
        }

        return Clamp(base_rate, 0.02, 0.25);
    }

    double PopulationCalculator::CalculateFreePeasantPercentage(int year, double prosperity_level) {
        double base_percentage = 0.30;

        // More free peasants in prosperous regions
        base_percentage += prosperity_level * 0.15;

        // Historical trend toward freedom
        if (year > 1300) {
            base_percentage += 0.15; // Black Death aftermath
        } else if (year < 1100) {
            base_percentage -= 0.10; // More feudal
        }

        return Clamp(base_percentage, 0.10, 0.50);
    }

    double PopulationCalculator::CalculateVilleinPercentage(int year, double prosperity_level) {
        double base_percentage = 0.40;

        // Villeins more common in mid-prosperity
        base_percentage += (1.0 - prosperity_level) * 0.10;

        // Historical variations
        if (year > 1300) {
            base_percentage -= 0.10; // Decline of villeinage
        }

        return Clamp(base_percentage, 0.25, 0.50);
    }

    // ========================================================================
    // Wealth Calculations
    // ========================================================================

    double PopulationCalculator::GetClassBaseWealth(SocialClass social_class, double prosperity_level) {
        std::unordered_map<SocialClass, double> base_wealth = {
            {SocialClass::HIGH_NOBILITY, 1000.0},
            {SocialClass::LESSER_NOBILITY, 500.0},
            {SocialClass::HIGH_CLERGY, 400.0},
            {SocialClass::CLERGY, 150.0},
            {SocialClass::WEALTHY_MERCHANTS, 800.0},
            {SocialClass::BURGHERS, 200.0},
            {SocialClass::GUILD_MASTERS, 300.0},
            {SocialClass::CRAFTSMEN, 120.0},
            {SocialClass::SCHOLARS, 100.0},
            {SocialClass::FREE_PEASANTS, 80.0},
            {SocialClass::VILLEINS, 60.0},
            {SocialClass::SERFS, 40.0},
            {SocialClass::URBAN_LABORERS, 70.0},
            {SocialClass::RELIGIOUS_ORDERS, 100.0},
            {SocialClass::FOREIGNERS, 150.0}
        };

        return base_wealth[social_class] * (0.5 + prosperity_level);
    }

    double PopulationCalculator::CalculateGroupWealth(int population_count, double wealth_per_capita) {
        return static_cast<double>(population_count) * wealth_per_capita;
    }

    // ========================================================================
    // Literacy and Education
    // ========================================================================

    double PopulationCalculator::GetClassLiteracyRate(SocialClass social_class, int year) {
        std::unordered_map<SocialClass, double> base_literacy = {
            {SocialClass::HIGH_NOBILITY, 0.6},
            {SocialClass::LESSER_NOBILITY, 0.4},
            {SocialClass::HIGH_CLERGY, 0.95},
            {SocialClass::CLERGY, 0.8},
            {SocialClass::WEALTHY_MERCHANTS, 0.5},
            {SocialClass::BURGHERS, 0.2},
            {SocialClass::GUILD_MASTERS, 0.3},
            {SocialClass::CRAFTSMEN, 0.1},
            {SocialClass::SCHOLARS, 0.98},
            {SocialClass::FREE_PEASANTS, 0.05},
            {SocialClass::VILLEINS, 0.02},
            {SocialClass::SERFS, 0.01},
            {SocialClass::URBAN_LABORERS, 0.08},
            {SocialClass::RELIGIOUS_ORDERS, 0.9},
            {SocialClass::FOREIGNERS, 0.15}
        };

        double literacy = base_literacy[social_class];

        // Adjust for historical period
        if (year > 1300) {
            literacy *= 1.5; // Renaissance approaching
        } else if (year < 1100) {
            literacy *= 0.7; // Early medieval period
        }

        return std::min(0.98, literacy);
    }

    double PopulationCalculator::CalculateEducationAccess(SocialClass social_class, double prosperity_level) {
        double base_access = IsWealthyClass(social_class) ? 0.6 : 0.1;
        return base_access * (0.5 + prosperity_level * 0.5);
    }

    // ========================================================================
    // Demographics
    // ========================================================================

    std::tuple<int, int, int> PopulationCalculator::CalculateAgeDistribution(int total_population) {
        int children = static_cast<int>(total_population * MEDIEVAL_CHILDREN_PERCENTAGE);
        int adults = static_cast<int>(total_population * MEDIEVAL_ADULT_PERCENTAGE);
        int elderly = total_population - children - adults;

        return std::make_tuple(children, adults, elderly);
    }

    std::pair<int, int> PopulationCalculator::CalculateGenderDistribution(int total_population) {
        int males = static_cast<int>(total_population * MEDIEVAL_MALE_PERCENTAGE);
        int females = total_population - males;

        return std::make_pair(males, females);
    }

    // ========================================================================
    // Health and Happiness
    // ========================================================================

    double PopulationCalculator::GetClassBaseHappiness(SocialClass social_class, double prosperity_level) {
        // Base happiness varies by class (0.0-1.0 scale)
        std::unordered_map<SocialClass, double> base_happiness = {
            {SocialClass::HIGH_NOBILITY, 0.7},
            {SocialClass::LESSER_NOBILITY, 0.6},
            {SocialClass::HIGH_CLERGY, 0.65},
            {SocialClass::CLERGY, 0.6},
            {SocialClass::WEALTHY_MERCHANTS, 0.7},
            {SocialClass::BURGHERS, 0.55},
            {SocialClass::GUILD_MASTERS, 0.6},
            {SocialClass::CRAFTSMEN, 0.5},
            {SocialClass::SCHOLARS, 0.6},
            {SocialClass::FREE_PEASANTS, 0.5},
            {SocialClass::VILLEINS, 0.4},
            {SocialClass::SERFS, 0.3},
            {SocialClass::URBAN_LABORERS, 0.45},
            {SocialClass::RELIGIOUS_ORDERS, 0.65},
            {SocialClass::FOREIGNERS, 0.4}
        };

        return base_happiness[social_class] * (0.7 + prosperity_level * 0.3);
    }

    double PopulationCalculator::GetClassHealthLevel(SocialClass social_class, double prosperity_level) {
        // Wealthier classes have better health
        double base_health = IsWealthyClass(social_class) ? 0.7 : 0.5;
        return base_health * (0.6 + prosperity_level * 0.4);
    }

    // ========================================================================
    // Military Calculations
    // ========================================================================

    int PopulationCalculator::CalculateMilitaryEligible(int adult_males, SocialClass social_class) {
        double eligibility_rate = 0.7; // Default 70% of adult males

        // Adjust by social class
        if (social_class == SocialClass::HIGH_NOBILITY || social_class == SocialClass::LESSER_NOBILITY) {
            eligibility_rate = 0.9; // Nobles expected to fight
        } else if (social_class == SocialClass::CLERGY || social_class == SocialClass::HIGH_CLERGY) {
            eligibility_rate = 0.1; // Clergy rarely fight
        } else if (social_class == SocialClass::SERFS) {
            eligibility_rate = 0.5; // Limited military obligation
        }

        return static_cast<int>(adult_males * eligibility_rate);
    }

    double PopulationCalculator::CalculateMilitaryQuality(SocialClass social_class, double prosperity_level) {
        std::unordered_map<SocialClass, double> base_quality = {
            {SocialClass::HIGH_NOBILITY, 0.9},
            {SocialClass::LESSER_NOBILITY, 0.8},
            {SocialClass::GUILD_MASTERS, 0.6},
            {SocialClass::CRAFTSMEN, 0.5},
            {SocialClass::FREE_PEASANTS, 0.4},
            {SocialClass::VILLEINS, 0.3},
            {SocialClass::SERFS, 0.2},
            {SocialClass::URBAN_LABORERS, 0.35}
        };

        double quality = base_quality.count(social_class) ? base_quality[social_class] : 0.3;
        return quality * (0.8 + prosperity_level * 0.2);
    }

    // ========================================================================
    // Settlement Calculations
    // ========================================================================

    int PopulationCalculator::CalculateSettlementCount(int population, int avg_settlement_size) {
        return std::max(1, population / avg_settlement_size);
    }

    double PopulationCalculator::GetSettlementInfrastructure(SettlementType type, double prosperity_level) {
        std::unordered_map<SettlementType, double> base_infrastructure = {
            {SettlementType::MAJOR_CITY, 0.8},
            {SettlementType::LARGE_TOWN, 0.6},
            {SettlementType::SMALL_TOWN, 0.5},
            {SettlementType::VILLAGE, 0.3},
            {SettlementType::RURAL_HAMLET, 0.2},
            {SettlementType::ROYAL_CASTLE, 0.9},
            {SettlementType::MONASTERY, 0.6}
        };

        double infrastructure = base_infrastructure.count(type) ? base_infrastructure[type] : 0.4;
        return infrastructure * (0.6 + prosperity_level * 0.4);
    }

    double PopulationCalculator::GetSettlementFortification(SettlementType type, double prosperity_level) {
        std::unordered_map<SettlementType, double> base_fortification = {
            {SettlementType::MAJOR_CITY, 0.7},
            {SettlementType::LARGE_TOWN, 0.5},
            {SettlementType::ROYAL_CASTLE, 1.0},
            {SettlementType::DUCAL_CASTLE, 0.9},
            {SettlementType::BORDER_FORTRESS, 0.95},
            {SettlementType::WATCHTOWER, 0.6}
        };

        return base_fortification.count(type) ? base_fortification[type] : 0.2;
    }

    double PopulationCalculator::GetSettlementAutonomy(SettlementType type) {
        if (type == SettlementType::MAJOR_CITY || type == SettlementType::LARGE_TOWN) {
            return 0.7; // Cities have high autonomy
        } else if (type == SettlementType::VILLAGE || type == SettlementType::RURAL_HAMLET) {
            return 0.2; // Villages have low autonomy
        }
        return 0.4; // Default moderate autonomy
    }

    double PopulationCalculator::GetSettlementDiseaseRisk(SettlementType type, double prosperity_level) {
        // Urban areas have higher disease risk
        double base_risk = 0.1;

        if (type == SettlementType::MAJOR_CITY) {
            base_risk = 0.25;
        } else if (type == SettlementType::LARGE_TOWN || type == SettlementType::SMALL_TOWN) {
            base_risk = 0.15;
        }

        // Prosperity reduces disease risk through better sanitation
        return base_risk * (1.2 - prosperity_level * 0.4);
    }

    // ========================================================================
    // Utility Functions
    // ========================================================================

    bool PopulationCalculator::IsWealthyClass(SocialClass social_class) {
        return social_class == SocialClass::HIGH_NOBILITY ||
               social_class == SocialClass::LESSER_NOBILITY ||
               social_class == SocialClass::WEALTHY_MERCHANTS ||
               social_class == SocialClass::HIGH_CLERGY ||
               social_class == SocialClass::GUILD_MASTERS;
    }

    double PopulationCalculator::Clamp(double value, double min_val, double max_val) {
        return std::max(min_val, std::min(value, max_val));
    }

} // namespace game::population
