// ============================================================================
// PopulationTypes.h - Population System Type Definitions
// Created: December 19, 2024 at 11:15 AM
// Location: include/game/population/PopulationTypes.h
// ============================================================================

#pragma once

#include "utils/PlatformMacros.h"
#include "core/ECS/EntityManager.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <chrono>

namespace game::population {

    // ============================================================================
    // Core Population Enums
    // ============================================================================

    enum class SocialClass {
        HIGH_NOBILITY,        // Dukes, counts, major lords
        LESSER_NOBILITY,      // Knights, minor lords, landed gentry
        HIGH_CLERGY,          // Bishops, abbots, church leadership
        CLERGY,               // Priests, monks, regular clergy
        WEALTHY_MERCHANTS,    // International bankers, major trading houses
        BURGHERS,             // Urban middle class, shopkeepers, small traders
        GUILD_MASTERS,        // Master craftsmen, guild leaders
        CRAFTSMEN,            // Skilled workers, journeymen
        SCHOLARS,             // University teachers, scribes, lawyers, physicians
        FREE_PEASANTS,        // Independent farmers with land rights
        VILLEINS,             // Semi-free peasants with feudal obligations
        SERFS,                // Bound to land, minimal rights
        URBAN_LABORERS,       // City workers, servants, unskilled labor
        SLAVES,               // Bondsmen (rare but present in early medieval)
        FOREIGNERS,           // Foreign traders, diplomats, refugees
        OUTLAWS,              // Bandits, exiles, vagabonds
        RELIGIOUS_ORDERS      // Monks, nuns, military orders
    };

    enum class LegalStatus {
        FULL_CITIZEN,         // Complete legal rights and protections
        BURGHER_RIGHTS,       // Urban privileges, trade freedoms, city law
        FREE_PEASANT,         // Rural independence, land ownership rights
        VILLEIN,              // Semi-free, bound by feudal obligations
        SERF,                 // Bound to land, minimal legal standing
        SLAVE,                // No legal rights, property of owner
        FOREIGNER,            // Different legal standing, limited rights
        CLERIC,               // Church law jurisdiction, ecclesiastical immunity
        OUTLAW,               // Outside legal protection, hunted
        ROYAL_WARD,           // Under crown protection and obligation
        GUILD_MEMBER,         // Protected by guild law and privileges
        MILITARY_SERVICE      // Special military legal status
    };

    enum class EmploymentType {
        // Non-working income sources
        LANDED_INCOME,        // Nobles living off land rents and feudal dues
        CAPITAL_INVESTMENT,   // Merchant business income, banking profits
        RELIGIOUS_BENEFICE,   // Church positions with automatic income
        ROYAL_PENSION,        // Crown grants and payments

        // Intellectual and professional occupations
        HIGHER_LEARNING,      // University professors, teachers, tutors
        LEGAL_PROFESSION,     // Lawyers, judges, notaries, legal scribes
        MEDICAL_PRACTICE,     // Physicians, surgeons, apothecaries, healers
        SCRIBAL_WORK,         // Clerks, copyists, accountants, record keepers
        DIPLOMATIC_SERVICE,   // Ambassadors, envoys, translators

        // Traditional productive employment
        AGRICULTURE,          // Farming, animal husbandry, rural production
        CRAFTING,             // Manufacturing, artisan work, skilled production
        TRADE,                // Commerce, transportation, market activity
        MILITARY,             // Professional soldiers, guards, castellans
        ADMINISTRATION,       // Government clerks, tax collectors, officials
        RELIGIOUS,            // Church services, pastoral care, religious education
        CONSTRUCTION,         // Building, engineering, infrastructure work
        EXTRACTION,           // Mining, forestry, quarrying, fishing

        // Specialized medieval occupations
        ENTERTAINMENT,        // Minstrels, jesters, troubadours, performers
        DOMESTIC_SERVICE,     // Household servants, cooks, grooms, maids
        SEASONAL_LABOR,       // Harvest workers, temporary construction, traveling work
        MARITIME_TRADE,       // Sailors, ship captains, dock workers, navigators
        GUILD_ADMINISTRATION, // Guild officials, quality inspectors, trade regulators
        PILGRIMAGE_SERVICES,  // Guides, innkeepers for pilgrims, shrine keepers

        // Economic fringe activities
        CRIMINAL_ACTIVITY,    // Thieves, smugglers, bandits, black market
        MONEY_LENDING,        // Usury, pawnbrokers, financial services
        MERCENARY_SERVICE,    // Professional soldiers for hire

        // Non-productive categories
        UNEMPLOYED_SEEKING,   // Actively looking for work
        UNEMPLOYABLE,         // Too old, sick, disabled, or young to work
        RETIRED,              // Former workers living off savings/family
        DEPENDENT             // Children, wives without independent income
    };

    enum class SettlementType {
        // Military and defensive settlements
        ROYAL_CASTLE,         // Major fortification with royal court
        DUCAL_CASTLE,         // Noble family stronghold
        BORDER_FORTRESS,      // Strategic military outpost
        WATCHTOWER,           // Small garrison and observation point
        FORTIFIED_MANOR,      // Defended noble residence
        MILITARY_CAMP,        // Temporary or seasonal army base

        // Economic production settlements
        FARMING_VILLAGE,      // Agricultural production focus
        FISHING_VILLAGE,      // Maritime food production
        HERDING_SETTLEMENT,   // Pastoral livestock focus
        MINING_SETTLEMENT,    // Resource extraction (iron, silver, coal)
        FORESTRY_SETTLEMENT,  // Timber production and charcoal
        QUARRY_SETTLEMENT,    // Stone extraction
        SALT_WORKS,           // Salt production (extremely valuable)
        VINEYARD_ESTATE,      // Wine production

        // Commercial and trade settlements
        TRADE_POST,           // Commercial hub and warehouse center
        PORT_TOWN,            // Maritime trade and shipbuilding
        MARKET_TOWN,          // Regional commerce and fair center
        BRIDGE_TOWN,          // Strategic river crossing with tolls
        MOUNTAIN_PASS,        // Trade route control point
        CUSTOMS_HOUSE,        // Border trade regulation

        // Religious settlements
        CATHEDRAL_TOWN,       // Major religious administrative center
        MONASTERY,            // Religious community with economic activity
        CONVENT,              // Female religious community
        PILGRIMAGE_SITE,      // Religious tourism and services
        HERMITAGE,            // Single or small group religious retreat
        ABBEY_LANDS,          // Monastic agricultural estates

        // Educational and cultural settlements
        UNIVERSITY_TOWN,      // Higher learning and intellectual center
        CATHEDRAL_SCHOOL,     // Religious education center
        SCRIPTORIUM,          // Manuscript copying and preservation

        // Administrative settlements
        ROYAL_MANOR,          // Administrative center and royal residence
        COUNTY_SEAT,          // Regional government center
        TOLL_STATION,         // Revenue collection point

        // Standard population centers
        RURAL_HAMLET,         // < 100 people, basic agriculture
        VILLAGE,              // 100-500 people, local market
        TOWN,                 // 500-2000 people, regional importance
        CITY,                 // 2000-10000 people, major center
        LARGE_CITY,           // 10000+ people, international importance

        // Specialized settlements
        GUILD_TOWN,           // Dominated by specific craft guilds
        FREE_CITY,            // Independent urban republic
        HANSEATIC_CITY,       // Member of trading league
        REFUGEE_CAMP,         // Temporary displacement settlement
        PLAGUE_QUARANTINE     // Isolated disease containment
    };

    // ============================================================================
    // Population Data Structures
    // ============================================================================

    struct PopulationGroup {
        // Identity
        SocialClass social_class = SocialClass::FREE_PEASANTS;
        LegalStatus legal_status = LegalStatus::FREE_PEASANT;
        std::string culture = "english";
        std::string religion = "catholic";

        // Basic demographics
        int population_count = 0;
        double happiness = 0.5;
        double literacy_rate = 0.1;
        double wealth_per_capita = 100.0;
        double health_level = 0.7;

        // Age and gender structure
        int children_0_14 = 0;
        int adults_15_64 = 0;
        int elderly_65_plus = 0;
        int males = 0;
        int females = 0;

        // Employment distribution
        std::unordered_map<EmploymentType, int> employment;
        double employment_rate = 0.0;

        // Demographic rates
        double birth_rate = 0.035;
        double death_rate = 0.030;
        double infant_mortality = 0.25;
        double maternal_mortality = 0.02;
        double migration_tendency = 0.1;

        // Cultural and social factors
        double assimilation_rate = 0.02;
        double conversion_rate = 0.01;
        double education_access = 0.2;
        double social_mobility = 0.005;

        // Economic factors
        double taxation_burden = 0.1;
        double feudal_obligations = 0.0;
        double guild_membership_rate = 0.0;

        // Military and service potential
        int military_eligible = 0;
        double military_quality = 0.5;
        int military_service_obligation = 0;

        // Legal and social attributes
        std::vector<std::string> legal_privileges;
        std::vector<std::string> economic_rights;
        std::vector<std::string> social_restrictions;

        // Family structure
        double average_household_size = 5.0;
        double extended_family_rate = 0.3;
        double servant_employment_rate = 0.0;
    };

    struct Settlement {
        // Basic information
        std::string name;
        SettlementType type = SettlementType::VILLAGE;
        std::string parent_province;

        // Geographic and strategic
        double x_coordinate = 0.0;
        double y_coordinate = 0.0;
        std::string controlling_lord;
        std::vector<std::string> strategic_resources;

        // Population and demographics
        std::vector<PopulationGroup> population_groups;
        int total_population = 0;
        double population_density = 0.0;

        // Infrastructure and development
        std::unordered_map<std::string, int> buildings;
        double infrastructure_level = 0.5;
        double fortification_level = 0.0;
        double sanitation_level = 0.3;
        double water_access_quality = 0.7;

        // Economic specialization
        std::unordered_map<std::string, double> production;
        std::unordered_map<std::string, double> consumption;
        std::unordered_map<std::string, double> storage;
        std::vector<std::string> economic_specializations;

        // Growth and prosperity factors
        double growth_rate = 0.0;
        double prosperity_level = 0.5;
        double stability = 0.7;
        double administrative_efficiency = 0.5;

        // Trade and connections
        std::vector<std::string> trade_routes;
        double trade_income = 0.0;
        std::vector<std::string> guild_presence;
        double market_importance = 0.0;

        // Cultural and religious factors
        std::string dominant_culture;
        std::string dominant_religion;
        std::vector<std::string> cultural_minorities;
        std::vector<std::string> religious_minorities;
        double cultural_tolerance = 0.5;
        double religious_tolerance = 0.5;

        // Political and legal
        std::string government_type;
        std::vector<std::string> legal_privileges;
        double autonomy_level = 0.3;
        double tax_burden = 0.15;

        // Military and defense
        int garrison_size = 0;
        std::vector<std::string> defensive_features;
        double military_importance = 0.0;
        int militia_potential = 0;

        // Environmental and health factors
        double disease_risk = 0.1;
        double natural_disaster_risk = 0.05;
        std::vector<std::string> environmental_challenges;

        // Historical and cultural significance
        std::vector<std::string> historical_events;
        double cultural_prestige = 0.0;
        std::vector<std::string> famous_residents;
    };

    // ============================================================================
    // Historical Population Data
    // ============================================================================

    struct HistoricalPopulationData {
        std::string region_name;
        std::string culture;
        std::string religion;
        int year;
        int base_population;
        double prosperity_level;
        double urbanization_rate;

        // Social class percentages
        double nobility_percentage = 0.02;
        double clergy_percentage = 0.03;
        double merchant_percentage = 0.05;
        double craftsmen_percentage = 0.08;
        double peasant_percentage = 0.4;
        double serf_percentage = 0.42;

        // Regional data generators
        static std::vector<HistoricalPopulationData> GetMedievalEnglandData();
        static std::vector<HistoricalPopulationData> GetMedievalFranceData();
        static std::vector<HistoricalPopulationData> GetByzantineData();
        static std::vector<HistoricalPopulationData> GetHolyRomanEmpireData();
        static std::vector<HistoricalPopulationData> GetIberianData();
        static std::vector<HistoricalPopulationData> GetScandinavianData();
    };

    // ============================================================================
    // Population Analysis and Reporting
    // ============================================================================

    namespace analysis {

        struct PopulationMetrics {
            // Basic demographics
            int total_population = 0;
            double average_age = 25.0;
            double dependency_ratio = 0.6; // non-working / working population
            double sex_ratio = 1.0; // males per female

            // Social structure
            std::unordered_map<SocialClass, double> class_distribution;
            std::unordered_map<LegalStatus, double> legal_distribution;
            double social_mobility_index = 0.005;
            double class_inequality_index = 0.7;

            // Economic metrics
            double employment_rate = 0.7;
            double productivity_index = 1.0;
            double economic_diversity_index = 0.5;
            std::unordered_map<EmploymentType, double> employment_distribution;

            // Cultural metrics
            std::unordered_map<std::string, double> cultural_composition;
            std::unordered_map<std::string, double> religious_composition;
            double cultural_diversity_index = 0.3;
            double religious_diversity_index = 0.2;

            // Health and education
            double average_literacy = 0.15;
            double average_health = 0.6;
            double life_expectancy = 35.0;
            double child_mortality = 0.3;

            // Settlement patterns
            double urbanization_rate = 0.1;
            int number_of_settlements = 10;
            double average_settlement_size = 500.0;
            std::unordered_map<SettlementType, int> settlement_distribution;

            // Military potential
            int military_eligible_population = 0;
            double average_military_quality = 0.5;
            int current_military_service = 0;
            double military_participation_rate = 0.05;

            // Stability indicators
            double happiness_index = 0.5;
            double social_tension_level = 0.3;
            double crime_rate = 0.05;
            double migration_pressure = 0.1;
        };

        struct PopulationTrends {
            // Growth trends
            double population_growth_rate = 0.005;
            double birth_rate_trend = 0.0;
            double death_rate_trend = 0.0;
            double migration_rate_trend = 0.0;

            // Social change trends
            double social_mobility_trend = 0.0;
            double urbanization_trend = 0.001;
            double literacy_trend = 0.002;
            double wealth_inequality_trend = 0.001;

            // Cultural change trends
            double cultural_assimilation_trend = 0.02;
            double religious_conversion_trend = 0.01;
            double cultural_tension_trend = 0.0;

            // Economic trends
            double employment_trend = 0.0;
            double productivity_trend = 0.001;
            double specialization_trend = 0.005;

            // Predictive indicators
            std::vector<std::string> emerging_trends;
            std::vector<std::string> potential_crises;
            std::vector<std::string> growth_opportunities;
            double stability_forecast = 0.5;
        };

        struct CrisisIndicators {
            // Demographic crises
            double overpopulation_risk = 0.0;
            double depopulation_risk = 0.0;
            double age_structure_imbalance = 0.0;

            // Economic crises
            double unemployment_risk = 0.0;
            double inequality_crisis_risk = 0.0;
            double economic_collapse_risk = 0.0;

            // Social crises
            double social_unrest_risk = 0.0;
            double class_conflict_risk = 0.0;
            double cultural_conflict_risk = 0.0;

            // Health crises
            double epidemic_risk = 0.1;
            double famine_risk = 0.05;
            double water_crisis_risk = 0.0;

            // Migration crises
            double refugee_influx_risk = 0.0;
            double mass_emigration_risk = 0.0;
            double displacement_risk = 0.0;

            // Overall stability assessment
            double overall_crisis_risk = 0.0;
            std::vector<std::string> immediate_threats;
            std::vector<std::string> long_term_vulnerabilities;
        };

    } // namespace analysis

    // ============================================================================
    // Integration Data Structures
    // ============================================================================

    namespace integration {

        // Military system integration data
        struct MilitaryRecruitmentData {
            int total_eligible = 0;
            double average_quality = 0.5;
            std::unordered_map<SocialClass, int> recruitment_by_class;
            int feudal_service_days = 0;
            double recruitment_cost = 0.0;
            double social_disruption = 0.0;
        };

        // Economic system integration data
        struct EconomicData {
            double total_tax_revenue = 0.0;
            double total_production_value = 0.0;
            double total_consumption_value = 0.0;
            std::unordered_map<EmploymentType, int> workers_by_type;
            double guild_influence = 0.0;
            double trade_capacity = 0.0;
            double economic_complexity = 0.0;
        };

        // Technology system integration data
        struct TechnologyAdoptionData {
            double literacy_rate = 0.15;
            double education_access = 0.2;
            int scholar_population = 0;
            int university_count = 0;
            double innovation_potential = 0.0;
            double technology_adoption_rate = 0.01;
        };

        // Diplomatic system integration data
        struct CulturalInfluenceData {
            std::unordered_map<std::string, double> culture_influence;
            std::unordered_map<std::string, double> religion_influence;
            double cultural_tolerance = 0.5;
            double foreign_population_percentage = 0.05;
            double diplomatic_reputation = 0.5;
        };

        // Administrative system integration data
        struct AdministrativeData {
            double administrative_capacity = 0.5;
            double law_enforcement_effectiveness = 0.5;
            double tax_collection_efficiency = 0.7;
            double bureaucratic_corruption = 0.3;
            double local_autonomy = 0.3;
        };

    } // namespace integration

    // ============================================================================
    // Utility Functions
    // ============================================================================

    namespace utils {

        // String conversion functions
        std::string GetSocialClassName(SocialClass social_class);
        std::string GetLegalStatusName(LegalStatus legal_status);
        std::string GetEmploymentName(EmploymentType employment);
        std::string GetSettlementTypeName(SettlementType type);

        // Social class utility functions
        bool IsNobleClass(SocialClass social_class);
        bool IsReligiousClass(SocialClass social_class);
        bool IsUrbanClass(SocialClass social_class);
        bool IsRuralClass(SocialClass social_class);
        bool IsEducatedClass(SocialClass social_class);
        bool IsWealthyClass(SocialClass social_class);

        // Social mobility functions
        SocialClass GetNextHigherClass(SocialClass current_class);
        SocialClass GetNextLowerClass(SocialClass current_class);
        bool CanPromoteToClass(SocialClass from_class, SocialClass to_class);
        double GetClassMobilityChance(SocialClass from_class, SocialClass to_class);

        // Employment utility functions
        bool IsProductiveEmployment(EmploymentType employment);
        bool IsIncomeGenerating(EmploymentType employment);
        bool CanWorkInRole(SocialClass social_class, EmploymentType employment);
        EmploymentType GetPrimaryEmployment(SocialClass social_class);
        double GetEmploymentProductivity(EmploymentType employment);

        // Settlement utility functions
        bool IsUrbanSettlement(SettlementType type);
        bool IsMilitarySettlement(SettlementType type);
        bool IsEconomicSettlement(SettlementType type);
        bool IsReligiousSettlement(SettlementType type);
        int GetSettlementSizeCategory(SettlementType type); // 0=hamlet, 4=large city
        double GetSettlementDefensiveValue(SettlementType type);

        // Population calculation helpers
        double CalculatePopulationPressure(int population, double carrying_capacity);
        double CalculateClassWealth(SocialClass social_class, double base_wealth);
        double CalculateLiteracyExpectation(SocialClass social_class, int year);
        double CalculateMilitaryQuality(SocialClass social_class, double base_quality);
        
        // Historical accuracy helpers
        bool IsClassAvailableInPeriod(SocialClass social_class, int year);
        bool IsEmploymentAvailableInPeriod(EmploymentType employment, int year);
        bool IsSettlementTypeAvailableInPeriod(SettlementType type, int year);
        std::vector<std::string> GetAvailableCultures(int year);
        std::vector<std::string> GetAvailableReligions(int year);

    } // namespace utils

} // namespace game::population