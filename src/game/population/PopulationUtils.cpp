// ============================================================================
// PopulationTypes Utils - Missing Helper Functions (Complete Implementation)
// Created: October 11, 2025 at 2:50 PM
// Location: src/game/population/PopulationUtils.cpp (NEW FILE)
// ============================================================================

#include "game/population/PopulationTypes.h"
#include <unordered_map>

namespace game::population {
namespace utils {

// Social Class Navigation
// ============================================================================

SocialClass GetNextHigherClass(SocialClass current_class) {
    // Define social mobility paths upward
    switch (current_class) {
        case SocialClass::SERFS:
            return SocialClass::VILLEINS;
        case SocialClass::VILLEINS:
            return SocialClass::FREE_PEASANTS;
        case SocialClass::FREE_PEASANTS:
            return SocialClass::CRAFTSMEN;
        case SocialClass::URBAN_LABORERS:
            return SocialClass::CRAFTSMEN;
        case SocialClass::CRAFTSMEN:
            return SocialClass::GUILD_MASTERS;
        case SocialClass::GUILD_MASTERS:
            return SocialClass::WEALTHY_MERCHANTS;
        case SocialClass::BURGHERS:
            return SocialClass::WEALTHY_MERCHANTS;
        case SocialClass::WEALTHY_MERCHANTS:
            return SocialClass::LESSER_NOBILITY;
        case SocialClass::CLERGY:
            return SocialClass::HIGH_CLERGY;
        case SocialClass::SCHOLARS:
            return SocialClass::HIGH_CLERGY;
        case SocialClass::LESSER_NOBILITY:
            return SocialClass::HIGH_NOBILITY;
        case SocialClass::SLAVES:
            return SocialClass::SERFS;
        case SocialClass::FOREIGNERS:
            return SocialClass::BURGHERS;
        case SocialClass::OUTLAWS:
            return SocialClass::URBAN_LABORERS; // Redemption path
        case SocialClass::RELIGIOUS_ORDERS:
            return SocialClass::HIGH_CLERGY;
        default:
            return current_class;
    }
}

SocialClass GetNextLowerClass(SocialClass current_class) {
    // Define social mobility paths downward
    switch (current_class) {
        case SocialClass::HIGH_NOBILITY:
            return SocialClass::LESSER_NOBILITY;
        case SocialClass::LESSER_NOBILITY:
            return SocialClass::WEALTHY_MERCHANTS;
        case SocialClass::HIGH_CLERGY:
            return SocialClass::CLERGY;
        case SocialClass::WEALTHY_MERCHANTS:
            return SocialClass::BURGHERS;
        case SocialClass::BURGHERS:
            return SocialClass::URBAN_LABORERS;
        case SocialClass::GUILD_MASTERS:
            return SocialClass::CRAFTSMEN;
        case SocialClass::CRAFTSMEN:
            return SocialClass::URBAN_LABORERS;
        case SocialClass::SCHOLARS:
            return SocialClass::CRAFTSMEN;
        case SocialClass::FREE_PEASANTS:
            return SocialClass::VILLEINS;
        case SocialClass::VILLEINS:
            return SocialClass::SERFS;
        case SocialClass::URBAN_LABORERS:
            return SocialClass::OUTLAWS; // Desperation
        case SocialClass::CLERGY:
            return SocialClass::SCHOLARS;
        default:
            return current_class;
    }
}

bool CanPromoteToClass(SocialClass from_class, SocialClass to_class) {
    // Check if promotion is possible
    if (from_class == to_class) return false;
    
    // Certain classes cannot be reached through normal mobility
    if (to_class == SocialClass::HIGH_NOBILITY) {
        return from_class == SocialClass::LESSER_NOBILITY; // Only nobles can become high nobles
    }
    
    if (to_class == SocialClass::HIGH_CLERGY) {
        return (from_class == SocialClass::CLERGY || 
                from_class == SocialClass::SCHOLARS ||
                from_class == SocialClass::RELIGIOUS_ORDERS);
    }
    
    if (to_class == SocialClass::SLAVES) {
        return false; // Cannot voluntarily become slave
    }
    
    if (to_class == SocialClass::OUTLAWS) {
        return false; // Outlaws are punished, not promoted
    }
    
    // Check if it's the direct next step
    return GetNextHigherClass(from_class) == to_class;
}

double GetClassMobilityChance(SocialClass from_class, SocialClass to_class) {
    // Return probability of mobility between classes
    if (!CanPromoteToClass(from_class, to_class)) {
        return 0.0;
    }
    
    // Base mobility chances by transition type
    std::unordered_map<SocialClass, double> mobility_difficulty = {
        {SocialClass::HIGH_NOBILITY, 0.001},      // Very rare
        {SocialClass::LESSER_NOBILITY, 0.005},    // Rare
        {SocialClass::HIGH_CLERGY, 0.008},
        {SocialClass::WEALTHY_MERCHANTS, 0.015},  // Moderate (wealth helps)
        {SocialClass::GUILD_MASTERS, 0.020},
        {SocialClass::BURGHERS, 0.025},
        {SocialClass::CRAFTSMEN, 0.035},          // Easier trades
        {SocialClass::FREE_PEASANTS, 0.040},      // Common transitions
        {SocialClass::VILLEINS, 0.030},
        {SocialClass::CLERGY, 0.012}
    };
    
    auto it = mobility_difficulty.find(to_class);
    return (it != mobility_difficulty.end()) ? it->second : 0.01;
}

// Legal Status Helpers
// ============================================================================

LegalStatus GetCorrespondingLegalStatus(SocialClass social_class) {
    // Map social classes to their typical legal status
    switch (social_class) {
        case SocialClass::HIGH_NOBILITY:
        case SocialClass::LESSER_NOBILITY:
        case SocialClass::FREE_PEASANTS:
        case SocialClass::VILLEINS:
        case SocialClass::SERFS:
            return LegalStatus::FREE_PEASANT;
        case SocialClass::HIGH_CLERGY:
        case SocialClass::CLERGY:
        case SocialClass::RELIGIOUS_ORDERS:
            return LegalStatus::CLERIC;
        case SocialClass::WEALTHY_MERCHANTS:
        case SocialClass::BURGHERS:
            return LegalStatus::BURGHER_RIGHTS;
        case SocialClass::GUILD_MASTERS:
        case SocialClass::CRAFTSMEN:
            return LegalStatus::GUILD_MEMBER;
        case SocialClass::SLAVES:
            return LegalStatus::SLAVE;
        case SocialClass::URBAN_LABORERS:
            return LegalStatus::BURGHER_RIGHTS;
        case SocialClass::FOREIGNERS:
            return LegalStatus::FOREIGNER;
        case SocialClass::OUTLAWS:
            return LegalStatus::OUTLAW;
        default:
            return LegalStatus::FREE_PEASANT;
    }
}

// Employment Helpers
// ============================================================================

EmploymentType GetPrimaryEmployment(SocialClass social_class) {
    // Return the primary employment type for each social class
    switch (social_class) {
        case SocialClass::HIGH_NOBILITY:
        case SocialClass::LESSER_NOBILITY:
            return EmploymentType::LANDED_INCOME;
        case SocialClass::HIGH_CLERGY:
        case SocialClass::CLERGY:
        case SocialClass::RELIGIOUS_ORDERS:
            return EmploymentType::RELIGIOUS_BENEFICE;
        case SocialClass::WEALTHY_MERCHANTS:
            return EmploymentType::CAPITAL_INVESTMENT;
        case SocialClass::BURGHERS:
            return EmploymentType::TRADE;
        case SocialClass::GUILD_MASTERS:
            return EmploymentType::GUILD_ADMINISTRATION;
        case SocialClass::CRAFTSMEN:
            return EmploymentType::CRAFTING;
        case SocialClass::FREE_PEASANTS:
            return EmploymentType::AGRICULTURE;
        case SocialClass::VILLEINS:
        case SocialClass::SERFS:
            return EmploymentType::AGRICULTURE;
        case SocialClass::SLAVES:
            return EmploymentType::DOMESTIC_SERVICE;
        case SocialClass::URBAN_LABORERS:
            return EmploymentType::SEASONAL_LABOR;
        case SocialClass::FOREIGNERS:
            return EmploymentType::TRADE;
        case SocialClass::OUTLAWS:
            return EmploymentType::CRIMINAL_ACTIVITY;
        case SocialClass::SCHOLARS:
            return EmploymentType::HIGHER_LEARNING;
        default:
            return EmploymentType::UNEMPLOYED_SEEKING;
    }
}

bool IsProductiveEmployment(EmploymentType employment) {
    // Determine if employment generates economic value
    switch (employment) {
        case EmploymentType::AGRICULTURE:
        case EmploymentType::CRAFTING:
        case EmploymentType::TRADE:
        case EmploymentType::MILITARY:
        case EmploymentType::ADMINISTRATION:
        case EmploymentType::RELIGIOUS:
        case EmploymentType::CONSTRUCTION:
        case EmploymentType::EXTRACTION:
        case EmploymentType::MARITIME_TRADE:
        case EmploymentType::GUILD_ADMINISTRATION:
            return true;
        default:
            return false;
    }
}

bool IsIncomeGenerating(EmploymentType employment) {
    // Determine if employment provides personal income
    return employment != EmploymentType::UNEMPLOYED_SEEKING &&
           employment != EmploymentType::DEPENDENT &&
           employment != EmploymentType::UNEMPLOYABLE;
}

bool CanWorkInRole(SocialClass social_class, EmploymentType employment) {
    // Determine if a social class can perform specific employment
    
    // Anyone can be dependent or unemployable
    if (employment == EmploymentType::DEPENDENT || 
        employment == EmploymentType::UNEMPLOYABLE) {
        return true;
    }
    
    // Class-specific restrictions
    switch (employment) {
        case EmploymentType::LANDED_INCOME:
            return (social_class == SocialClass::HIGH_NOBILITY ||
                    social_class == SocialClass::LESSER_NOBILITY);
        case EmploymentType::RELIGIOUS_BENEFICE:
            return (social_class == SocialClass::HIGH_CLERGY ||
                    social_class == SocialClass::CLERGY ||
                    social_class == SocialClass::RELIGIOUS_ORDERS);
        case EmploymentType::CAPITAL_INVESTMENT:
            return (social_class == SocialClass::WEALTHY_MERCHANTS);
        case EmploymentType::TRADE:
            return (social_class == SocialClass::BURGHERS ||
                    social_class == SocialClass::FOREIGNERS);
        case EmploymentType::GUILD_ADMINISTRATION:
            return (social_class == SocialClass::GUILD_MASTERS);
        case EmploymentType::CRAFTING:
            return (social_class == SocialClass::CRAFTSMEN);
        case EmploymentType::AGRICULTURE:
            return (social_class == SocialClass::FREE_PEASANTS ||
                    social_class == SocialClass::VILLEINS ||
                    social_class == SocialClass::SERFS);
        case EmploymentType::DOMESTIC_SERVICE:
            return (social_class == SocialClass::SLAVES);
        case EmploymentType::SEASONAL_LABOR:
            return (social_class == SocialClass::URBAN_LABORERS);
        case EmploymentType::HIGHER_LEARNING:
            return (social_class == SocialClass::SCHOLARS);
        default:
            return false;
    }
}

double GetEmploymentProductivity(EmploymentType employment) {
    // Return relative productivity factor for different employment types
    std::unordered_map<EmploymentType, double> productivity = {
        {EmploymentType::AGRICULTURE, 1.0},
        {EmploymentType::CRAFTING, 1.2},
        {EmploymentType::TRADE, 1.3},
        {EmploymentType::MILITARY, 1.1},
        {EmploymentType::ADMINISTRATION, 1.0},
        {EmploymentType::RELIGIOUS, 1.0},
        {EmploymentType::CONSTRUCTION, 1.0},
        {EmploymentType::EXTRACTION, 1.0},
        {EmploymentType::MARITIME_TRADE, 1.2},
        {EmploymentType::GUILD_ADMINISTRATION, 1.1},
        {EmploymentType::UNEMPLOYED_SEEKING, 0.0},
        {EmploymentType::DEPENDENT, 0.0},
        {EmploymentType::UNEMPLOYABLE, 0.0}
    };
    
    auto it = productivity.find(employment);
    return (it != productivity.end()) ? it->second : 1.0;
}

// Settlement Type Helpers
// ============================================================================

bool IsUrbanSettlement(SettlementType type) {
    switch (type) {
        case SettlementType::CITY:
        case SettlementType::LARGE_CITY:
        case SettlementType::FREE_CITY:
        case SettlementType::PORT_TOWN:
        case SettlementType::MARKET_TOWN:
        case SettlementType::GUILD_TOWN:
        case SettlementType::UNIVERSITY_TOWN:
        case SettlementType::CATHEDRAL_TOWN:
            return true;
        default:
            return false;
    }
}

bool IsMilitarySettlement(SettlementType type) {
    switch (type) {
        case SettlementType::ROYAL_CASTLE:
        case SettlementType::DUCAL_CASTLE:
        case SettlementType::BORDER_FORTRESS:
        case SettlementType::WATCHTOWER:
        case SettlementType::FORTIFIED_MANOR:
        case SettlementType::MILITARY_CAMP:
            return true;
        default:
            return false;
    }
}

bool IsEconomicSettlement(SettlementType type) {
    switch (type) {
        case SettlementType::MARKET_TOWN:
        case SettlementType::GUILD_TOWN:
        case SettlementType::PORT_TOWN:
        case SettlementType::CITY:
        case SettlementType::LARGE_CITY:
        case SettlementType::FREE_CITY:
        case SettlementType::TRADE_POST:
        case SettlementType::BRIDGE_TOWN:
        case SettlementType::CUSTOMS_HOUSE:
            return true;
        default:
            return false;
    }
}

bool IsReligiousSettlement(SettlementType type) {
    switch (type) {
        case SettlementType::CATHEDRAL_TOWN:
        case SettlementType::MONASTERY:
        case SettlementType::CONVENT:
        case SettlementType::PILGRIMAGE_SITE:
        case SettlementType::HERMITAGE:
        case SettlementType::ABBEY_LANDS:
        case SettlementType::CATHEDRAL_SCHOOL:
        case SettlementType::SCRIPTORIUM:
            return true;
        default:
            return false;
    }
}

int GetSettlementSizeCategory(SettlementType type) {
    // Return 0-4 category (hamlet to large city)
    switch (type) {
        case SettlementType::RURAL_HAMLET: return 0;
        case SettlementType::VILLAGE: return 1;
        case SettlementType::MARKET_TOWN:
        case SettlementType::GUILD_TOWN:
        case SettlementType::PORT_TOWN: return 2;
        case SettlementType::CITY:
        case SettlementType::FREE_CITY: return 3;
        case SettlementType::LARGE_CITY: return 4;
        default: return 0;
    }
}

double GetSettlementDefensiveValue(SettlementType type) {
    // Return defensive strength (0.0 to 1.0)
    switch (type) {
        case SettlementType::RURAL_HAMLET:
        case SettlementType::VILLAGE:
            return 0.05;
        case SettlementType::MARKET_TOWN:
        case SettlementType::GUILD_TOWN:
        case SettlementType::PORT_TOWN:
            return 0.15;
        case SettlementType::CITY:
        case SettlementType::FREE_CITY:
            return 0.25;
        case SettlementType::LARGE_CITY:
            return 0.35;
        case SettlementType::ROYAL_CASTLE:
        case SettlementType::DUCAL_CASTLE:
        case SettlementType::BORDER_FORTRESS:
        case SettlementType::WATCHTOWER:
        case SettlementType::FORTIFIED_MANOR:
            return 0.7;
        case SettlementType::MILITARY_CAMP:
            return 0.3;
        case SettlementType::MONASTERY:
        case SettlementType::CATHEDRAL_TOWN:
        case SettlementType::CONVENT:
        case SettlementType::PILGRIMAGE_SITE:
        case SettlementType::HERMITAGE:
        case SettlementType::ABBEY_LANDS:
            return 0.1;
        default:
            return 0.0;
    }
}

// String Conversion Functions (Already Declared in Header)
// ============================================================================

std::string GetSettlementTypeName(SettlementType type) {
    switch (type) {
        case SettlementType::RURAL_HAMLET: return "Hamlet";
        case SettlementType::VILLAGE: return "Village";
        case SettlementType::MARKET_TOWN: return "Market Town";
        case SettlementType::GUILD_TOWN: return "Guild Town";
        case SettlementType::PORT_TOWN: return "Port Town";
        case SettlementType::CITY: return "City";
        case SettlementType::LARGE_CITY: return "Large City";
        case SettlementType::FREE_CITY: return "Free City";
        case SettlementType::ROYAL_CASTLE: return "Royal Castle";
        case SettlementType::DUCAL_CASTLE: return "Ducal Castle";
        case SettlementType::BORDER_FORTRESS: return "Border Fortress";
        case SettlementType::WATCHTOWER: return "Watchtower";
        case SettlementType::FORTIFIED_MANOR: return "Fortified Manor";
        case SettlementType::MILITARY_CAMP: return "Military Camp";
        case SettlementType::MONASTERY: return "Monastery";
        case SettlementType::CATHEDRAL_TOWN: return "Cathedral Town";
        case SettlementType::CONVENT: return "Convent";
        case SettlementType::PILGRIMAGE_SITE: return "Pilgrimage Site";
        case SettlementType::HERMITAGE: return "Hermitage";
        case SettlementType::ABBEY_LANDS: return "Abbey Lands";
        case SettlementType::UNIVERSITY_TOWN: return "University Town";
        case SettlementType::CATHEDRAL_SCHOOL: return "Cathedral School";
        case SettlementType::SCRIPTORIUM: return "Scriptorium";
        case SettlementType::ROYAL_MANOR: return "Royal Manor";
        case SettlementType::COUNTY_SEAT: return "County Seat";
        case SettlementType::TOLL_STATION: return "Toll Station";
        case SettlementType::TRADE_POST: return "Trade Post";
        case SettlementType::BRIDGE_TOWN: return "Bridge Town";
        case SettlementType::MOUNTAIN_PASS: return "Mountain Pass";
        case SettlementType::CUSTOMS_HOUSE: return "Customs House";
        case SettlementType::FARMING_VILLAGE: return "Farming Village";
        case SettlementType::FISHING_VILLAGE: return "Fishing Village";
        case SettlementType::HERDING_SETTLEMENT: return "Herding Settlement";
        case SettlementType::MINING_SETTLEMENT: return "Mining Settlement";
        case SettlementType::FORESTRY_SETTLEMENT: return "Forestry Settlement";
        case SettlementType::QUARRY_SETTLEMENT: return "Quarry Settlement";
        case SettlementType::SALT_WORKS: return "Salt Works";
        case SettlementType::VINEYARD_ESTATE: return "Vineyard Estate";
        case SettlementType::HANSEATIC_CITY: return "Hanseatic City";
        case SettlementType::REFUGEE_CAMP: return "Refugee Camp";
        case SettlementType::PLAGUE_QUARANTINE: return "Plague Quarantine";
        default: return "Unknown";
    }
}

// Missing String Conversion Functions
// ============================================================================

std::string GetSocialClassName(SocialClass social_class) {
    switch (social_class) {
        case SocialClass::HIGH_NOBILITY: return "High Nobility";
        case SocialClass::LESSER_NOBILITY: return "Lesser Nobility";
        case SocialClass::HIGH_CLERGY: return "High Clergy";
        case SocialClass::CLERGY: return "Clergy";
        case SocialClass::WEALTHY_MERCHANTS: return "Wealthy Merchants";
        case SocialClass::GUILD_MASTERS: return "Guild Masters";
        case SocialClass::BURGHERS: return "Burghers";
        case SocialClass::CRAFTSMEN: return "Craftsmen";
        case SocialClass::SCHOLARS: return "Scholars";
        case SocialClass::FREE_PEASANTS: return "Free Peasants";
        case SocialClass::VILLEINS: return "Villeins";
        case SocialClass::SERFS: return "Serfs";
        case SocialClass::URBAN_LABORERS: return "Urban Laborers";
        case SocialClass::SLAVES: return "Slaves";
        case SocialClass::FOREIGNERS: return "Foreigners";
        case SocialClass::OUTLAWS: return "Outlaws";
        case SocialClass::RELIGIOUS_ORDERS: return "Religious Orders";
        default: return "Unknown Class";
    }
}

std::string GetLegalStatusName(LegalStatus legal_status) {
    switch (legal_status) {
        case LegalStatus::FULL_CITIZEN: return "Full Citizen";
        case LegalStatus::BURGHER_RIGHTS: return "Burgher Rights";
        case LegalStatus::FREE_PEASANT: return "Free Peasant";
        case LegalStatus::VILLEIN: return "Villein";
        case LegalStatus::SERF: return "Serf";
        case LegalStatus::SLAVE: return "Slave";
        case LegalStatus::FOREIGNER: return "Foreigner";
        case LegalStatus::CLERIC: return "Cleric";
        case LegalStatus::OUTLAW: return "Outlaw";
        case LegalStatus::ROYAL_WARD: return "Royal Ward";
        case LegalStatus::GUILD_MEMBER: return "Guild Member";
        case LegalStatus::MILITARY_SERVICE: return "Military Service";
        default: return "Unknown Status";
    }
}

std::string GetEmploymentName(EmploymentType employment) {
    switch (employment) {
        case EmploymentType::LANDED_INCOME: return "Landed Income";
        case EmploymentType::CAPITAL_INVESTMENT: return "Capital Investment";
        case EmploymentType::RELIGIOUS_BENEFICE: return "Religious Benefice";
        case EmploymentType::ROYAL_PENSION: return "Royal Pension";
        case EmploymentType::HIGHER_LEARNING: return "Higher Learning";
        case EmploymentType::LEGAL_PROFESSION: return "Legal Profession";
        case EmploymentType::MEDICAL_PRACTICE: return "Medical Practice";
        case EmploymentType::SCRIBAL_WORK: return "Scribal Work";
        case EmploymentType::DIPLOMATIC_SERVICE: return "Diplomatic Service";
        case EmploymentType::AGRICULTURE: return "Agriculture";
        case EmploymentType::CRAFTING: return "Crafting";
        case EmploymentType::TRADE: return "Trade";
        case EmploymentType::MILITARY: return "Military";
        case EmploymentType::ADMINISTRATION: return "Administration";
        case EmploymentType::RELIGIOUS: return "Religious";
        case EmploymentType::CONSTRUCTION: return "Construction";
        case EmploymentType::EXTRACTION: return "Extraction";
        case EmploymentType::ENTERTAINMENT: return "Entertainment";
        case EmploymentType::DOMESTIC_SERVICE: return "Domestic Service";
        case EmploymentType::SEASONAL_LABOR: return "Seasonal Labor";
        case EmploymentType::MARITIME_TRADE: return "Maritime Trade";
        case EmploymentType::GUILD_ADMINISTRATION: return "Guild Administration";
        case EmploymentType::PILGRIMAGE_SERVICES: return "Pilgrimage Services";
        case EmploymentType::CRIMINAL_ACTIVITY: return "Criminal Activity";
        case EmploymentType::MONEY_LENDING: return "Money Lending";
        case EmploymentType::MERCENARY_SERVICE: return "Mercenary Service";
        case EmploymentType::UNEMPLOYED_SEEKING: return "Unemployed Seeking";
        case EmploymentType::UNEMPLOYABLE: return "Unemployable";
        case EmploymentType::RETIRED: return "Retired";
        case EmploymentType::DEPENDENT: return "Dependent";
        default: return "Unknown Employment";
    }
}

bool IsWealthyClass(SocialClass social_class) {
    switch (social_class) {
        case SocialClass::HIGH_NOBILITY:
        case SocialClass::LESSER_NOBILITY:
        case SocialClass::HIGH_CLERGY:
        case SocialClass::WEALTHY_MERCHANTS:
        case SocialClass::GUILD_MASTERS:
            return true;
        default:
            return false;
    }
}

bool IsNobleClass(SocialClass social_class) {
    return (social_class == SocialClass::HIGH_NOBILITY ||
            social_class == SocialClass::LESSER_NOBILITY);
}

bool IsReligiousClass(SocialClass social_class) {
    return (social_class == SocialClass::HIGH_CLERGY ||
            social_class == SocialClass::CLERGY ||
            social_class == SocialClass::RELIGIOUS_ORDERS);
}

bool IsUrbanClass(SocialClass social_class) {
    switch (social_class) {
        case SocialClass::BURGHERS:
        case SocialClass::WEALTHY_MERCHANTS:
        case SocialClass::GUILD_MASTERS:
        case SocialClass::CRAFTSMEN:
        case SocialClass::URBAN_LABORERS:
        case SocialClass::SCHOLARS:
            return true;
        default:
            return false;
    }
}

bool IsRuralClass(SocialClass social_class) {
    switch (social_class) {
        case SocialClass::FREE_PEASANTS:
        case SocialClass::VILLEINS:
        case SocialClass::SERFS:
        case SocialClass::SLAVES:
            return true;
        default:
            return false;
    }
}

bool IsEducatedClass(SocialClass social_class) {
    switch (social_class) {
        case SocialClass::HIGH_NOBILITY:
        case SocialClass::LESSER_NOBILITY:
        case SocialClass::HIGH_CLERGY:
        case SocialClass::CLERGY:
        case SocialClass::SCHOLARS:
        case SocialClass::WEALTHY_MERCHANTS:
            return true;
        default:
            return false;
    }
}

// Population Calculation Helpers
// ============================================================================

double CalculatePopulationPressure(int population, double carrying_capacity) {
    if (carrying_capacity <= 0.0) return 1.0; // Maximum pressure

    double ratio = static_cast<double>(population) / carrying_capacity;

    // Pressure increases exponentially above carrying capacity
    if (ratio > 1.0) {
        return 0.5 + (ratio - 1.0) * 2.0; // Caps at very high pressure
    } else {
        return ratio * 0.5; // Below capacity = low pressure
    }
}

double CalculateClassWealth(SocialClass social_class, double base_wealth) {
    // Wealth multipliers by social class
    switch (social_class) {
        case SocialClass::HIGH_NOBILITY:
            return base_wealth * 50.0;
        case SocialClass::LESSER_NOBILITY:
            return base_wealth * 20.0;
        case SocialClass::HIGH_CLERGY:
            return base_wealth * 15.0;
        case SocialClass::WEALTHY_MERCHANTS:
            return base_wealth * 12.0;
        case SocialClass::GUILD_MASTERS:
            return base_wealth * 6.0;
        case SocialClass::BURGHERS:
            return base_wealth * 3.0;
        case SocialClass::CLERGY:
            return base_wealth * 2.5;
        case SocialClass::CRAFTSMEN:
            return base_wealth * 2.0;
        case SocialClass::SCHOLARS:
            return base_wealth * 1.8;
        case SocialClass::FREE_PEASANTS:
            return base_wealth * 1.0;
        case SocialClass::VILLEINS:
            return base_wealth * 0.7;
        case SocialClass::URBAN_LABORERS:
            return base_wealth * 0.6;
        case SocialClass::SERFS:
            return base_wealth * 0.4;
        case SocialClass::SLAVES:
            return base_wealth * 0.1;
        case SocialClass::FOREIGNERS:
            return base_wealth * 0.8;
        case SocialClass::OUTLAWS:
            return base_wealth * 0.3;
        case SocialClass::RELIGIOUS_ORDERS:
            return base_wealth * 0.5; // Vow of poverty
        default:
            return base_wealth;
    }
}

double CalculateLiteracyExpectation(SocialClass social_class, int year) {
    // Base literacy rates by class (medieval period baseline)
    double base_literacy = 0.0;

    switch (social_class) {
        case SocialClass::HIGH_CLERGY:
            base_literacy = 0.95;
            break;
        case SocialClass::CLERGY:
            base_literacy = 0.85;
            break;
        case SocialClass::SCHOLARS:
            base_literacy = 0.90;
            break;
        case SocialClass::HIGH_NOBILITY:
            base_literacy = 0.60;
            break;
        case SocialClass::LESSER_NOBILITY:
            base_literacy = 0.40;
            break;
        case SocialClass::WEALTHY_MERCHANTS:
            base_literacy = 0.50;
            break;
        case SocialClass::GUILD_MASTERS:
            base_literacy = 0.35;
            break;
        case SocialClass::BURGHERS:
            base_literacy = 0.20;
            break;
        case SocialClass::CRAFTSMEN:
            base_literacy = 0.15;
            break;
        case SocialClass::FREE_PEASANTS:
            base_literacy = 0.05;
            break;
        case SocialClass::URBAN_LABORERS:
            base_literacy = 0.08;
            break;
        default:
            base_literacy = 0.02;
            break;
    }

    // Literacy improves over time (very gradual in medieval period)
    // Assume year 1000 as baseline, each century adds small improvement
    if (year >= 1000) {
        int centuries_since_1000 = (year - 1000) / 100;
        double improvement = centuries_since_1000 * 0.02; // 2% per century
        return std::min(1.0, base_literacy + improvement);
    }

    return base_literacy;
}

double CalculateMilitaryQuality(SocialClass social_class, double base_quality) {
    // Military quality multipliers by social class
    switch (social_class) {
        case SocialClass::HIGH_NOBILITY:
        case SocialClass::LESSER_NOBILITY:
            return base_quality * 2.0; // Knights, trained warriors
        case SocialClass::WEALTHY_MERCHANTS:
            return base_quality * 0.8; // Can afford equipment
        case SocialClass::GUILD_MASTERS:
            return base_quality * 0.9; // Urban militia
        case SocialClass::BURGHERS:
        case SocialClass::CRAFTSMEN:
            return base_quality * 0.7; // Militia service
        case SocialClass::FREE_PEASANTS:
            return base_quality * 0.6; // Levy troops
        case SocialClass::VILLEINS:
        case SocialClass::SERFS:
            return base_quality * 0.4; // Poorly equipped
        case SocialClass::URBAN_LABORERS:
            return base_quality * 0.5; // City watch
        case SocialClass::FOREIGNERS:
            return base_quality * 1.2; // Mercenaries
        case SocialClass::OUTLAWS:
            return base_quality * 0.7; // Guerrilla fighters
        default:
            return base_quality;
    }
}

// Historical Accuracy Helpers
// ============================================================================

bool IsClassAvailableInPeriod(SocialClass social_class, int year) {
    // Most classes are available throughout medieval period (500-1500)
    // Some classes emerge or decline at specific times

    switch (social_class) {
        case SocialClass::GUILD_MASTERS:
        case SocialClass::CRAFTSMEN:
            return year >= 1100; // Guilds emerge in High Middle Ages
        case SocialClass::WEALTHY_MERCHANTS:
            return year >= 1000; // Commercial revolution
        case SocialClass::BURGHERS:
            return year >= 1000; // Urban revival
        case SocialClass::SCHOLARS:
            return year >= 1100; // Universities emerge
        case SocialClass::SLAVES:
            return year <= 1300; // Slavery declining in Western Europe
        default:
            return true; // Most classes exist throughout period
    }
}

bool IsEmploymentAvailableInPeriod(EmploymentType employment, int year) {
    switch (employment) {
        case EmploymentType::GUILD_ADMINISTRATION:
            return year >= 1100; // Guilds established
        case EmploymentType::HIGHER_LEARNING:
            return year >= 1100; // Universities
        case EmploymentType::MARITIME_TRADE:
            return year >= 1000; // Naval expansion
        case EmploymentType::CAPITAL_INVESTMENT:
            return year >= 1200; // Banking emerges
        case EmploymentType::MONEY_LENDING:
            return year >= 1100; // Financial services
        default:
            return true; // Most employment types exist throughout
    }
}

bool IsSettlementTypeAvailableInPeriod(SettlementType type, int year) {
    switch (type) {
        case SettlementType::FREE_CITY:
            return year >= 1100; // Free cities emerge
        case SettlementType::UNIVERSITY_TOWN:
            return year >= 1150; // First universities
        case SettlementType::HANSEATIC_CITY:
            return year >= 1200; // Hanseatic League
        case SettlementType::GUILD_TOWN:
            return year >= 1100; // Guild dominance
        case SettlementType::CUSTOMS_HOUSE:
            return year >= 1200; // Formalized trade regulation
        default:
            return true; // Most settlement types exist throughout
    }
}

std::vector<std::string> GetAvailableCultures(int year) {
    // Simplified list of major medieval European cultures
    std::vector<std::string> cultures = {
        "frankish", "english", "german", "italian", "iberian",
        "french", "norman", "scandinavian", "slavic", "greek",
        "arabic", "celtic", "hungarian", "polish"
    };

    // Could expand with time-specific cultures
    if (year >= 1066) {
        cultures.push_back("anglo-norman");
    }

    if (year >= 1200) {
        cultures.push_back("mongol");
    }

    return cultures;
}

std::vector<std::string> GetAvailableReligions(int year) {
    std::vector<std::string> religions = {
        "catholic", "orthodox", "sunni", "shia", "jewish"
    };

    // Add time-specific religious movements
    if (year >= 1054) {
        // Great Schism formalized
        religions.push_back("eastern_orthodox");
        religions.push_back("roman_catholic");
    }

    if (year >= 1200) {
        religions.push_back("cathar"); // Albigensian heresy
        religions.push_back("waldensian");
    }

    if (year >= 1400) {
        religions.push_back("hussite"); // Bohemian reformation
    }

    return religions;
}

} // namespace utils
} // namespace game::population
