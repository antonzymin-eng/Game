// ============================================================================
// EnhancedPopulationFactory.cpp - Population Creation Factory Implementation
// Created: December 19, 2024 at 11:40 AM
// Location: src/game/population/EnhancedPopulationFactory.cpp
// ============================================================================

#include "game/population/PopulationSystem.h"
#include "game/population/PopulationAggregator.h"
#include "core/logging/Logger.h"
#include "utils/RandomGenerator.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace game::population {

    // ============================================================================
    // EnhancedPopulationFactory Implementation
    // ============================================================================

    EnhancedPopulationFactory::EnhancedPopulationFactory() {
        std::random_device rd;
        m_random_generator.seed(rd());
        
        CORE_LOG_DEBUG("PopulationFactory", "Enhanced Population Factory initialized");
    }

    PopulationComponent EnhancedPopulationFactory::CreateMedievalPopulation(const std::string& culture, 
                                                                           const std::string& religion,
                                                                           int base_population, 
                                                                           double prosperity_level, 
                                                                           int year) {
        PopulationComponent population;
        
        CORE_LOG_INFO("PopulationFactory", 
            "Creating medieval population - Culture: " + culture + 
            ", Religion: " + religion + 
            ", Population: " + std::to_string(base_population) + 
            ", Prosperity: " + std::to_string(prosperity_level) + 
            ", Year: " + std::to_string(year));

        // Historical population distribution based on medieval demographics
        double noble_percentage = GetHistoricalPercentage(SocialClass::HIGH_NOBILITY, year, prosperity_level);
        double lesser_noble_percentage = GetHistoricalPercentage(SocialClass::LESSER_NOBILITY, year, prosperity_level);
        double high_clergy_percentage = GetHistoricalPercentage(SocialClass::HIGH_CLERGY, year, prosperity_level);
        double clergy_percentage = GetHistoricalPercentage(SocialClass::CLERGY, year, prosperity_level);
        double wealthy_merchant_percentage = GetHistoricalPercentage(SocialClass::WEALTHY_MERCHANTS, year, prosperity_level);
        double burgher_percentage = GetHistoricalPercentage(SocialClass::BURGHERS, year, prosperity_level);
        double craftsmen_percentage = GetHistoricalPercentage(SocialClass::CRAFTSMEN, year, prosperity_level);
        double scholar_percentage = GetHistoricalPercentage(SocialClass::SCHOLARS, year, prosperity_level);
        double free_peasant_percentage = GetHistoricalPercentage(SocialClass::FREE_PEASANTS, year, prosperity_level);
        double villein_percentage = GetHistoricalPercentage(SocialClass::VILLEINS, year, prosperity_level);
        double serf_percentage = GetHistoricalPercentage(SocialClass::SERFS, year, prosperity_level);
        double urban_laborer_percentage = GetHistoricalPercentage(SocialClass::URBAN_LABORERS, year, prosperity_level);

        // Create population groups
        CreateNoblePopulation(population, culture, religion, 
            static_cast<int>(base_population * noble_percentage), prosperity_level, year);
        CreateLesserNoblePopulation(population, culture, religion, 
            static_cast<int>(base_population * lesser_noble_percentage), prosperity_level, year);
        CreateClergyPopulation(population, culture, religion, 
            static_cast<int>(base_population * (high_clergy_percentage + clergy_percentage)), prosperity_level, year);
        CreateMerchantPopulation(population, culture, religion, 
            static_cast<int>(base_population * (wealthy_merchant_percentage + burgher_percentage)), prosperity_level, year);
        CreateCraftsmanPopulation(population, culture, religion, 
            static_cast<int>(base_population * craftsmen_percentage), prosperity_level, year);
        CreateScholarPopulation(population, culture, religion, 
            static_cast<int>(base_population * scholar_percentage), prosperity_level, year);
        CreatePeasantPopulation(population, culture, religion, 
            static_cast<int>(base_population * (free_peasant_percentage + villein_percentage + serf_percentage)), 
            prosperity_level, year);
        CreateUrbanLaborerPopulation(population, culture, religion, 
            static_cast<int>(base_population * urban_laborer_percentage), prosperity_level, year);

        // Add specialized groups for certain periods
        if (year >= 1000) {
            CreateReligiousOrdersPopulation(population, culture, religion, 
                static_cast<int>(base_population * 0.005), prosperity_level, year);
        }

        if (prosperity_level > 0.6 && year >= 1100) {
            CreateForeignerPopulation(population, culture, religion, 
                static_cast<int>(base_population * 0.02), prosperity_level, year);
        }

        // Calculate aggregate statistics
        PopulationAggregator::RecalculateAllAggregates(population);

        CORE_LOG_INFO("PopulationFactory", 
            "Medieval population created with " + std::to_string(population.total_population) + " people in " +
            std::to_string(population.population_groups.size()) + " social groups");

        return population;
    }

    SettlementComponent EnhancedPopulationFactory::CreateMedievalSettlements(const std::string& province_name, 
                                                                            int total_population,
                                                                            double prosperity_level, 
                                                                            const std::string& culture,
                                                                            const std::string& religion, 
                                                                            int year,
                                                                            const std::vector<std::string>& strategic_resources) {
        SettlementComponent settlements;

        CORE_LOG_INFO("PopulationFactory", 
            "Creating medieval settlements for " + province_name + 
            " - Population: " + std::to_string(total_population) + 
            ", Resources: " + std::to_string(strategic_resources.size()));

        // Determine settlement distribution based on population size and prosperity
        double urbanization_rate = CalculateUrbanizationRate(total_population, prosperity_level, year);
        int urban_population = static_cast<int>(total_population * urbanization_rate);
        int rural_population = total_population - urban_population;

        // Create urban settlements
        if (urban_population > 0) {
            CreateUrbanSettlements(settlements, province_name, urban_population, prosperity_level, 
                                 culture, religion, year, strategic_resources);
        }

        // Create rural settlements
        if (rural_population > 0) {
            CreateRuralSettlements(settlements, province_name, rural_population, prosperity_level, 
                                 culture, religion, year, strategic_resources);
        }

        // Create specialized settlements based on strategic resources and prosperity
        CreateMilitarySettlements(settlements, province_name, prosperity_level, culture, religion, year, strategic_resources);
        CreateReligiousSettlements(settlements, province_name, prosperity_level, culture, religion, year);

        if (prosperity_level > 0.7) {
            CreateAdministrativeSettlements(settlements, province_name, prosperity_level, culture, religion, year);
        }

        // Calculate aggregate settlement statistics
        RecalculateSettlementSummary(settlements);

        CORE_LOG_INFO("PopulationFactory", 
            "Medieval settlements created: " + std::to_string(settlements.settlements.size()) + 
            " settlements with " + std::to_string(settlements.urbanization_rate * 100) + "% urbanization");

        return settlements;
    }

    // ============================================================================
    // Population Creation Methods
    // ============================================================================

    void EnhancedPopulationFactory::CreateNoblePopulation(PopulationComponent& population, const std::string& culture,
                                                         const std::string& religion, int base_population, 
                                                         double prosperity_level, int year) {
        if (base_population <= 0) return;

        PopulationGroup high_nobles;
        PopulationGroup lesser_nobles;

        // High Nobility (about 20% of noble population)
        int high_noble_count = std::max(1, static_cast<int>(base_population * 0.2));
        high_nobles.social_class = SocialClass::HIGH_NOBILITY;
        high_nobles.legal_status = LegalStatus::FULL_CITIZEN;
        high_nobles.culture = culture;
        high_nobles.religion = religion;
        high_nobles.population_count = high_noble_count;

        SetGroupCharacteristics(high_nobles, SocialClass::HIGH_NOBILITY, LegalStatus::FULL_CITIZEN, 
                              prosperity_level, year);

        // Lesser Nobility (about 80% of noble population)
        int lesser_noble_count = base_population - high_noble_count;
        lesser_nobles.social_class = SocialClass::LESSER_NOBILITY;
        lesser_nobles.legal_status = LegalStatus::FULL_CITIZEN;
        lesser_nobles.culture = culture;
        lesser_nobles.religion = religion;
        lesser_nobles.population_count = lesser_noble_count;

        SetGroupCharacteristics(lesser_nobles, SocialClass::LESSER_NOBILITY, LegalStatus::FULL_CITIZEN, 
                              prosperity_level, year);

        population.population_groups.push_back(high_nobles);
        population.population_groups.push_back(lesser_nobles);

        CORE_LOG_DEBUG("PopulationFactory", 
            "Created noble population: " + std::to_string(high_noble_count) + " high nobles, " +
            std::to_string(lesser_noble_count) + " lesser nobles");
    }

    void EnhancedPopulationFactory::CreateLesserNoblePopulation(PopulationComponent& population, const std::string& culture,
                                                              const std::string& religion, int base_population, 
                                                              double prosperity_level, int year) {
        if (base_population <= 0) return;

        PopulationGroup lesser_nobles;
        lesser_nobles.social_class = SocialClass::LESSER_NOBILITY;
        lesser_nobles.legal_status = LegalStatus::FULL_CITIZEN;
        lesser_nobles.culture = culture;
        lesser_nobles.religion = religion;
        lesser_nobles.population_count = base_population;

        SetGroupCharacteristics(lesser_nobles, SocialClass::LESSER_NOBILITY, LegalStatus::FULL_CITIZEN, 
                              prosperity_level, year);

        population.population_groups.push_back(lesser_nobles);
    }

    void EnhancedPopulationFactory::CreateClergyPopulation(PopulationComponent& population, const std::string& culture,
                                                         const std::string& religion, int base_population, 
                                                         double prosperity_level, int year) {
        if (base_population <= 0) return;

        PopulationGroup high_clergy;
        PopulationGroup regular_clergy;

        // High Clergy (about 15% of clergy population)
        int high_clergy_count = std::max(1, static_cast<int>(base_population * 0.15));
        high_clergy.social_class = SocialClass::HIGH_CLERGY;
        high_clergy.legal_status = LegalStatus::CLERIC;
        high_clergy.culture = culture;
        high_clergy.religion = religion;
        high_clergy.population_count = high_clergy_count;

        SetGroupCharacteristics(high_clergy, SocialClass::HIGH_CLERGY, LegalStatus::CLERIC, 
                              prosperity_level, year);

        // Regular Clergy (about 85% of clergy population)
        int regular_clergy_count = base_population - high_clergy_count;
        regular_clergy.social_class = SocialClass::CLERGY;
        regular_clergy.legal_status = LegalStatus::CLERIC;
        regular_clergy.culture = culture;
        regular_clergy.religion = religion;
        regular_clergy.population_count = regular_clergy_count;

        SetGroupCharacteristics(regular_clergy, SocialClass::CLERGY, LegalStatus::CLERIC, 
                              prosperity_level, year);

        population.population_groups.push_back(high_clergy);
        population.population_groups.push_back(regular_clergy);

        CORE_LOG_DEBUG("PopulationFactory", 
            "Created clergy population: " + std::to_string(high_clergy_count) + " high clergy, " +
            std::to_string(regular_clergy_count) + " regular clergy");
    }

    void EnhancedPopulationFactory::CreateMerchantPopulation(PopulationComponent& population, const std::string& culture,
                                                           const std::string& religion, int base_population, 
                                                           double prosperity_level, int year) {
        if (base_population <= 0) return;

        PopulationGroup wealthy_merchants;
        PopulationGroup burghers;

        // Wealthy Merchants (about 25% of merchant population)
        int wealthy_merchant_count = std::max(1, static_cast<int>(base_population * 0.25));
        wealthy_merchants.social_class = SocialClass::WEALTHY_MERCHANTS;
        wealthy_merchants.legal_status = LegalStatus::BURGHER_RIGHTS;
        wealthy_merchants.culture = culture;
        wealthy_merchants.religion = religion;
        wealthy_merchants.population_count = wealthy_merchant_count;

        SetGroupCharacteristics(wealthy_merchants, SocialClass::WEALTHY_MERCHANTS, LegalStatus::BURGHER_RIGHTS, 
                              prosperity_level, year);

        // Burghers (about 75% of merchant population)
        int burgher_count = base_population - wealthy_merchant_count;
        burghers.social_class = SocialClass::BURGHERS;
        burghers.legal_status = LegalStatus::BURGHER_RIGHTS;
        burghers.culture = culture;
        burghers.religion = religion;
        burghers.population_count = burgher_count;

        SetGroupCharacteristics(burghers, SocialClass::BURGHERS, LegalStatus::BURGHER_RIGHTS, 
                              prosperity_level, year);

        population.population_groups.push_back(wealthy_merchants);
        population.population_groups.push_back(burghers);

        CORE_LOG_DEBUG("PopulationFactory", 
            "Created merchant population: " + std::to_string(wealthy_merchant_count) + " wealthy merchants, " +
            std::to_string(burgher_count) + " burghers");
    }

    void EnhancedPopulationFactory::CreateCraftsmanPopulation(PopulationComponent& population, const std::string& culture,
                                                            const std::string& religion, int base_population, 
                                                            double prosperity_level, int year) {
        if (base_population <= 0) return;

        PopulationGroup guild_masters;
        PopulationGroup craftsmen;

        // Guild Masters (about 20% of craftsman population)
        int guild_master_count = std::max(1, static_cast<int>(base_population * 0.2));
        guild_masters.social_class = SocialClass::GUILD_MASTERS;
        guild_masters.legal_status = LegalStatus::GUILD_MEMBER;
        guild_masters.culture = culture;
        guild_masters.religion = religion;
        guild_masters.population_count = guild_master_count;

        SetGroupCharacteristics(guild_masters, SocialClass::GUILD_MASTERS, LegalStatus::GUILD_MEMBER, 
                              prosperity_level, year);

        // Regular Craftsmen (about 80% of craftsman population)
        int craftsmen_count = base_population - guild_master_count;
        craftsmen.social_class = SocialClass::CRAFTSMEN;
        craftsmen.legal_status = LegalStatus::GUILD_MEMBER;
        craftsmen.culture = culture;
        craftsmen.religion = religion;
        craftsmen.population_count = craftsmen_count;

        SetGroupCharacteristics(craftsmen, SocialClass::CRAFTSMEN, LegalStatus::GUILD_MEMBER, 
                              prosperity_level, year);

        population.population_groups.push_back(guild_masters);
        population.population_groups.push_back(craftsmen);

        CORE_LOG_DEBUG("PopulationFactory", 
            "Created craftsman population: " + std::to_string(guild_master_count) + " guild masters, " +
            std::to_string(craftsmen_count) + " craftsmen");
    }

    void EnhancedPopulationFactory::CreateScholarPopulation(PopulationComponent& population, const std::string& culture,
                                                          const std::string& religion, int base_population, 
                                                          double prosperity_level, int year) {
        if (base_population <= 0) return;

        PopulationGroup scholars;
        scholars.social_class = SocialClass::SCHOLARS;
        scholars.legal_status = LegalStatus::FULL_CITIZEN;
        scholars.culture = culture;
        scholars.religion = religion;
        scholars.population_count = base_population;

        SetGroupCharacteristics(scholars, SocialClass::SCHOLARS, LegalStatus::FULL_CITIZEN, 
                              prosperity_level, year);

        population.population_groups.push_back(scholars);

        CORE_LOG_DEBUG("PopulationFactory", 
            "Created scholar population: " + std::to_string(base_population) + " scholars");
    }

    void EnhancedPopulationFactory::CreatePeasantPopulation(PopulationComponent& population, const std::string& culture,
                                                          const std::string& religion, int base_population, 
                                                          double prosperity_level, int year) {
        if (base_population <= 0) return;

        PopulationGroup free_peasants;
        PopulationGroup villeins;
        PopulationGroup serfs;

        // Determine peasant distribution based on year and prosperity
        double free_percentage = CalculateFreePeasantPercentage(year, prosperity_level);
        double villein_percentage = CalculateVilleinPercentage(year, prosperity_level);
        double serf_percentage = 1.0 - free_percentage - villein_percentage;

        // Free Peasants
        int free_peasant_count = static_cast<int>(base_population * free_percentage);
        if (free_peasant_count > 0) {
            free_peasants.social_class = SocialClass::FREE_PEASANTS;
            free_peasants.legal_status = LegalStatus::FREE_PEASANT;
            free_peasants.culture = culture;
            free_peasants.religion = religion;
            free_peasants.population_count = free_peasant_count;

            SetGroupCharacteristics(free_peasants, SocialClass::FREE_PEASANTS, LegalStatus::FREE_PEASANT, 
                                  prosperity_level, year);
            population.population_groups.push_back(free_peasants);
        }

        // Villeins
        int villein_count = static_cast<int>(base_population * villein_percentage);
        if (villein_count > 0) {
            villeins.social_class = SocialClass::VILLEINS;
            villeins.legal_status = LegalStatus::VILLEIN;
            villeins.culture = culture;
            villeins.religion = religion;
            villeins.population_count = villein_count;

            SetGroupCharacteristics(villeins, SocialClass::VILLEINS, LegalStatus::VILLEIN, 
                                  prosperity_level, year);
            population.population_groups.push_back(villeins);
        }

        // Serfs
        int serf_count = base_population - free_peasant_count - villein_count;
        if (serf_count > 0) {
            serfs.social_class = SocialClass::SERFS;
            serfs.legal_status = LegalStatus::SERF;
            serfs.culture = culture;
            serfs.religion = religion;
            serfs.population_count = serf_count;

            SetGroupCharacteristics(serfs, SocialClass::SERFS, LegalStatus::SERF, 
                                  prosperity_level, year);
            population.population_groups.push_back(serfs);
        }

        CORE_LOG_DEBUG("PopulationFactory", 
            "Created peasant population: " + std::to_string(free_peasant_count) + " free peasants, " +
            std::to_string(villein_count) + " villeins, " + 
            std::to_string(serf_count) + " serfs");
    }

    void EnhancedPopulationFactory::CreateUrbanLaborerPopulation(PopulationComponent& population, const std::string& culture,
                                                               const std::string& religion, int base_population, 
                                                               double prosperity_level, int year) {
        if (base_population <= 0) return;

        PopulationGroup urban_laborers;
        urban_laborers.social_class = SocialClass::URBAN_LABORERS;
        urban_laborers.legal_status = LegalStatus::BURGHER_RIGHTS;
        urban_laborers.culture = culture;
        urban_laborers.religion = religion;
        urban_laborers.population_count = base_population;

        SetGroupCharacteristics(urban_laborers, SocialClass::URBAN_LABORERS, LegalStatus::BURGHER_RIGHTS, 
                              prosperity_level, year);

        population.population_groups.push_back(urban_laborers);

        CORE_LOG_DEBUG("PopulationFactory", 
            "Created urban laborer population: " + std::to_string(base_population) + " urban laborers");
    }

    void EnhancedPopulationFactory::CreateReligiousOrdersPopulation(PopulationComponent& population, const std::string& culture,
                                                                  const std::string& religion, int base_population, 
                                                                  double prosperity_level, int year) {
        if (base_population <= 0) return;

        PopulationGroup religious_orders;
        religious_orders.social_class = SocialClass::RELIGIOUS_ORDERS;
        religious_orders.legal_status = LegalStatus::CLERIC;
        religious_orders.culture = culture;
        religious_orders.religion = religion;
        religious_orders.population_count = base_population;

        SetGroupCharacteristics(religious_orders, SocialClass::RELIGIOUS_ORDERS, LegalStatus::CLERIC, 
                              prosperity_level, year);

        population.population_groups.push_back(religious_orders);

        CORE_LOG_DEBUG("PopulationFactory", 
            "Created religious orders population: " + std::to_string(base_population) + " religious orders");
    }

    void EnhancedPopulationFactory::CreateForeignerPopulation(PopulationComponent& population, const std::string& culture,
                                                            const std::string& religion, int base_population, 
                                                            double prosperity_level, int year) {
        if (base_population <= 0) return;

        PopulationGroup foreigners;
        foreigners.social_class = SocialClass::FOREIGNERS;
        foreigners.legal_status = LegalStatus::FOREIGNER;
        foreigners.culture = DetermineForeignCulture(culture, year);
        foreigners.religion = DetermineForeignReligion(religion, year);
        foreigners.population_count = base_population;

        SetGroupCharacteristics(foreigners, SocialClass::FOREIGNERS, LegalStatus::FOREIGNER, 
                              prosperity_level, year);

        population.population_groups.push_back(foreigners);

        CORE_LOG_DEBUG("PopulationFactory", 
            "Created foreigner population: " + std::to_string(base_population) + " foreigners");
    }

    // ============================================================================
    // Settlement Creation Methods
    // ============================================================================

    void EnhancedPopulationFactory::CreateUrbanSettlements(SettlementComponent& settlements, const std::string& province_name,
                                                          int urban_population, double prosperity_level, 
                                                          const std::string& culture, const std::string& religion, 
                                                          int year, const std::vector<std::string>& strategic_resources) {
        // Determine main city size and type
        SettlementType main_city_type = DetermineMainCityType(urban_population, prosperity_level);
        
        Settlement main_city = CreateSettlement(province_name + "_City", main_city_type, province_name, prosperity_level);
        main_city.dominant_culture = culture;
        main_city.dominant_religion = religion;
        
        // Allocate population to main city (60-80% of urban population)
        double main_city_percentage = 0.6 + (prosperity_level * 0.2);
        int main_city_population = static_cast<int>(urban_population * main_city_percentage);
        main_city.total_population = main_city_population;
        
        // Set economic specializations based on strategic resources
        SetEconomicSpecializations(main_city, strategic_resources, prosperity_level);
        
        settlements.settlements.push_back(main_city);

        // Create smaller urban settlements with remaining population
        int remaining_population = urban_population - main_city_population;
        if (remaining_population > 0) {
            CreateSecondaryUrbanSettlements(settlements, province_name, remaining_population, 
                                          prosperity_level, culture, religion, year, strategic_resources);
        }

        CORE_LOG_DEBUG("PopulationFactory", 
            "Created urban settlements: Main city with " + std::to_string(main_city_population) + 
            " population, " + std::to_string(remaining_population) + " in secondary settlements");
    }

    void EnhancedPopulationFactory::CreateRuralSettlements(SettlementComponent& settlements, const std::string& province_name,
                                                         int rural_population, double prosperity_level, 
                                                         const std::string& culture, const std::string& religion, 
                                                         int year, const std::vector<std::string>& strategic_resources) {
        // Calculate number of rural settlements based on population density
        int num_villages = std::max(1, rural_population / 300); // Average village size ~300
        int num_hamlets = std::max(2, rural_population / 150);  // Average hamlet size ~75

        int population_per_village = rural_population / (num_villages + num_hamlets);

        // Create villages
        for (int i = 0; i < num_villages; ++i) {
            Settlement village = CreateSettlement(province_name + "_Village_" + std::to_string(i + 1), 
                                                SettlementType::VILLAGE, province_name, prosperity_level);
            village.dominant_culture = culture;
            village.dominant_religion = religion;
            village.total_population = population_per_village;
            
            // Add agricultural specializations
            village.economic_specializations.push_back("agriculture");
            if (std::find(strategic_resources.begin(), strategic_resources.end(), "fertile_land") != strategic_resources.end()) {
                village.economic_specializations.push_back("grain_production");
            }
            
            settlements.settlements.push_back(village);
        }

        // Create hamlets
        for (int i = 0; i < num_hamlets; ++i) {
            Settlement hamlet = CreateSettlement(province_name + "_Hamlet_" + std::to_string(i + 1), 
                                               SettlementType::RURAL_HAMLET, province_name, prosperity_level);
            hamlet.dominant_culture = culture;
            hamlet.dominant_religion = religion;
            hamlet.total_population = population_per_village / 2;
            
            // Basic agricultural focus
            hamlet.economic_specializations.push_back("subsistence_farming");
            
            settlements.settlements.push_back(hamlet);
        }

        CORE_LOG_DEBUG("PopulationFactory", 
            "Created rural settlements: " + std::to_string(num_villages) + " villages, " + 
            std::to_string(num_hamlets) + " hamlets");
    }

    void EnhancedPopulationFactory::CreateMilitarySettlements(SettlementComponent& settlements, const std::string& province_name,
                                                            double prosperity_level, const std::string& culture, 
                                                            const std::string& religion, int year,
                                                            const std::vector<std::string>& strategic_resources) {
        // Create main defensive settlement based on prosperity and strategic importance
        SettlementType fortress_type = SettlementType::BORDER_FORTRESS;
        
        if (prosperity_level > 0.8) {
            fortress_type = SettlementType::ROYAL_CASTLE;
        } else if (prosperity_level > 0.6) {
            fortress_type = SettlementType::DUCAL_CASTLE;
        }

        Settlement main_fortress = CreateSettlement(province_name + "_Castle", fortress_type, province_name, prosperity_level);
        main_fortress.dominant_culture = culture;
        main_fortress.dominant_religion = religion;
        main_fortress.total_population = static_cast<int>(50 + prosperity_level * 200); // 50-250 people
        main_fortress.garrison_size = static_cast<int>(main_fortress.total_population * 0.6);
        main_fortress.military_importance = prosperity_level;
        
        settlements.settlements.push_back(main_fortress);

        // Add watchtowers for strategic locations
        if (std::find(strategic_resources.begin(), strategic_resources.end(), "mountain_pass") != strategic_resources.end() ||
            std::find(strategic_resources.begin(), strategic_resources.end(), "river_crossing") != strategic_resources.end()) {
            
            Settlement watchtower = CreateSettlement(province_name + "_Watchtower", SettlementType::WATCHTOWER, 
                                                   province_name, prosperity_level);
            watchtower.dominant_culture = culture;
            watchtower.dominant_religion = religion;
            watchtower.total_population = 20 + static_cast<int>(prosperity_level * 30);
            watchtower.garrison_size = static_cast<int>(watchtower.total_population * 0.8);
            
            settlements.settlements.push_back(watchtower);
        }

        CORE_LOG_DEBUG("PopulationFactory", 
            "Created military settlements: Main fortress with " + std::to_string(main_fortress.total_population) + " population");
    }

    void EnhancedPopulationFactory::CreateReligiousSettlements(SettlementComponent& settlements, const std::string& province_name,
                                                             double prosperity_level, const std::string& culture, 
                                                             const std::string& religion, int year) {
        // Create main religious center
        SettlementType religious_type = SettlementType::MONASTERY;
        
        if (prosperity_level > 0.7) {
            religious_type = SettlementType::CATHEDRAL_TOWN;
        }

        Settlement religious_center = CreateSettlement(province_name + "_Abbey", religious_type, province_name, prosperity_level);
        religious_center.dominant_culture = culture;
        religious_center.dominant_religion = religion;
        religious_center.total_population = static_cast<int>(30 + prosperity_level * 150); // 30-180 people
        religious_center.economic_specializations.push_back("religious_services");
        religious_center.economic_specializations.push_back("manuscript_copying");
        
        settlements.settlements.push_back(religious_center);

        // Create pilgrimage sites for certain religions and prosperity levels
        if (prosperity_level > 0.5 && (religion == "catholic" || religion == "orthodox")) {
            Settlement pilgrimage_site = CreateSettlement(province_name + "_Shrine", SettlementType::PILGRIMAGE_SITE, 
                                                        province_name, prosperity_level);
            pilgrimage_site.dominant_culture = culture;
            pilgrimage_site.dominant_religion = religion;
            pilgrimage_site.total_population = static_cast<int>(15 + prosperity_level * 50);
            pilgrimage_site.economic_specializations.push_back("pilgrimage_services");
            
            settlements.settlements.push_back(pilgrimage_site);
        }

        CORE_LOG_DEBUG("PopulationFactory", 
            "Created religious settlements: " + utils::GetSettlementTypeName(religious_type) + 
            " with " + std::to_string(religious_center.total_population) + " population");
    }

    void EnhancedPopulationFactory::CreateAdministrativeSettlements(SettlementComponent& settlements, const std::string& province_name,
                                                                  double prosperity_level, const std::string& culture, 
                                                                  const std::string& religion, int year) {
        Settlement administrative_center = CreateSettlement(province_name + "_Manor", SettlementType::ROYAL_MANOR, 
                                                          province_name, prosperity_level);
        administrative_center.dominant_culture = culture;
        administrative_center.dominant_religion = religion;
        administrative_center.total_population = static_cast<int>(40 + prosperity_level * 100); // 40-140 people
        administrative_center.economic_specializations.push_back("administration");
        administrative_center.economic_specializations.push_back("tax_collection");
        administrative_center.administrative_efficiency = prosperity_level;
        
        settlements.settlements.push_back(administrative_center);

        CORE_LOG_DEBUG("PopulationFactory", 
            "Created administrative settlement with " + std::to_string(administrative_center.total_population) + " population");
    }

    // ============================================================================
    // Helper Methods
    // ============================================================================

    void EnhancedPopulationFactory::SetGroupCharacteristics(PopulationGroup& group, SocialClass social_class, 
                                                           LegalStatus legal_status, double prosperity_level, int year) {
        // Set basic demographics based on social class and historical period
        group.wealth_per_capita = GetClassBaseWealth(social_class, prosperity_level);
        group.literacy_rate = GetClassLiteracyRate(social_class, year);
        group.happiness = GetClassBaseHappiness(social_class, prosperity_level);
        group.health_level = GetClassHealthLevel(social_class, prosperity_level);

        // Set age distribution (medieval demographics)
        group.children_0_14 = static_cast<int>(group.population_count * 0.35);
        group.adults_15_64 = static_cast<int>(group.population_count * 0.55);
        group.elderly_65_plus = group.population_count - group.children_0_14 - group.adults_15_64;

        // Set gender distribution (slightly more females survive)
        group.males = static_cast<int>(group.population_count * 0.48);
        group.females = group.population_count - group.males;

        // Set employment distribution
        SetEmploymentDistribution(group, social_class);

        // Set military characteristics
        group.military_eligible = CalculateMilitaryEligible(group);
        group.military_quality = CalculateMilitaryQuality(social_class, prosperity_level);

        // Set legal privileges and restrictions
        group.legal_privileges = GetLegalPrivileges(legal_status);
        group.economic_rights = GetEconomicRights(legal_status);
        group.social_restrictions = GetSocialRestrictions(legal_status);

        // Set demographic rates
        SetDemographicRates(group, social_class, prosperity_level);

        // Set cultural factors
        SetCulturalFactors(group, social_class, year);
    }

    Settlement EnhancedPopulationFactory::CreateSettlement(const std::string& name, SettlementType type, 
                                                          const std::string& province_name, double prosperity_level) {
        Settlement settlement;
        settlement.name = name;
        settlement.type = type;
        settlement.parent_province = province_name;
        settlement.prosperity_level = prosperity_level;

        // Set infrastructure based on settlement type and prosperity
        settlement.infrastructure_level = GetSettlementInfrastructure(type, prosperity_level);
        settlement.fortification_level = GetSettlementFortification(type, prosperity_level);
        settlement.sanitation_level = GetSettlementSanitation(type, prosperity_level);
        settlement.water_access_quality = GetSettlementWaterAccess(type, prosperity_level);

        // Set administrative characteristics
        settlement.administrative_efficiency = prosperity_level * 0.8;
        settlement.autonomy_level = GetSettlementAutonomy(type);
        settlement.tax_burden = 0.1 + (prosperity_level * 0.1);

        // Set environmental factors
        settlement.disease_risk = GetSettlementDiseaseRisk(type, prosperity_level);
        settlement.natural_disaster_risk = 0.05; // Base risk

        return settlement;
    }

    double EnhancedPopulationFactory::GetHistoricalPercentage(SocialClass social_class, int year, double prosperity_level) {
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
        if (utils::IsWealthyClass(social_class)) {
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

    double EnhancedPopulationFactory::GetClassBaseWealth(SocialClass social_class, double prosperity_level) {
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

    double EnhancedPopulationFactory::GetClassLiteracyRate(SocialClass social_class, int year) {
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

        // Adjust for historical period - literacy generally increases over time
        if (year > 1300) {
            literacy *= 1.5; // Renaissance approaching
        } else if (year < 1100) {
            literacy *= 0.7; // Early medieval period
        }

        return std::min(0.98, literacy);
    }

    void EnhancedPopulationFactory::SetEmploymentDistribution(PopulationGroup& group, SocialClass social_class) {
        group.employment.clear();

        switch (social_class) {
            case SocialClass::HIGH_NOBILITY:
            case SocialClass::LESSER_NOBILITY:
                group.employment[EmploymentType::LANDED_INCOME] = static_cast<int>(group.population_count * 0.6);
                group.employment[EmploymentType::ADMINISTRATION] = static_cast<int>(group.population_count * 0.3);
                group.employment[EmploymentType::MILITARY] = static_cast<int>(group.population_count * 0.1);
                break;

            case SocialClass::HIGH_CLERGY:
            case SocialClass::CLERGY:
            case SocialClass::RELIGIOUS_ORDERS:
                group.employment[EmploymentType::RELIGIOUS_BENEFICE] = static_cast<int>(group.population_count * 0.4);
                group.employment[EmploymentType::RELIGIOUS] = static_cast<int>(group.population_count * 0.5);
                group.employment[EmploymentType::HIGHER_LEARNING] = static_cast<int>(group.population_count * 0.1);
                break;

            case SocialClass::WEALTHY_MERCHANTS:
                group.employment[EmploymentType::CAPITAL_INVESTMENT] = static_cast<int>(group.population_count * 0.5);
                group.employment[EmploymentType::TRADE] = static_cast<int>(group.population_count * 0.4);
                group.employment[EmploymentType::MONEY_LENDING] = static_cast<int>(group.population_count * 0.1);
                break;

            case SocialClass::BURGHERS:
                group.employment[EmploymentType::TRADE] = static_cast<int>(group.population_count * 0.6);
                group.employment[EmploymentType::CRAFTING] = static_cast<int>(group.population_count * 0.2);
                group.employment[EmploymentType::DOMESTIC_SERVICE] = static_cast<int>(group.population_count * 0.2);
                break;

            case SocialClass::GUILD_MASTERS:
            case SocialClass::CRAFTSMEN:
                group.employment[EmploymentType::CRAFTING] = static_cast<int>(group.population_count * 0.7);
                group.employment[EmploymentType::TRADE] = static_cast<int>(group.population_count * 0.2);
                group.employment[EmploymentType::GUILD_ADMINISTRATION] = static_cast<int>(group.population_count * 0.1);
                break;

            case SocialClass::SCHOLARS:
                group.employment[EmploymentType::HIGHER_LEARNING] = static_cast<int>(group.population_count * 0.4);
                group.employment[EmploymentType::LEGAL_PROFESSION] = static_cast<int>(group.population_count * 0.3);
                group.employment[EmploymentType::MEDICAL_PRACTICE] = static_cast<int>(group.population_count * 0.2);
                group.employment[EmploymentType::SCRIBAL_WORK] = static_cast<int>(group.population_count * 0.1);
                break;

            case SocialClass::FREE_PEASANTS:
            case SocialClass::VILLEINS:
            case SocialClass::SERFS:
                group.employment[EmploymentType::AGRICULTURE] = static_cast<int>(group.population_count * 0.8);
                group.employment[EmploymentType::SEASONAL_LABOR] = static_cast<int>(group.population_count * 0.1);
                group.employment[EmploymentType::UNEMPLOYED_SEEKING] = static_cast<int>(group.population_count * 0.1);
                break;

            case SocialClass::URBAN_LABORERS:
                group.employment[EmploymentType::CONSTRUCTION] = static_cast<int>(group.population_count * 0.3);
                group.employment[EmploymentType::DOMESTIC_SERVICE] = static_cast<int>(group.population_count * 0.3);
                group.employment[EmploymentType::SEASONAL_LABOR] = static_cast<int>(group.population_count * 0.2);
                group.employment[EmploymentType::UNEMPLOYED_SEEKING] = static_cast<int>(group.population_count * 0.2);
                break;

            case SocialClass::FOREIGNERS:
                group.employment[EmploymentType::TRADE] = static_cast<int>(group.population_count * 0.4);
                group.employment[EmploymentType::DIPLOMATIC_SERVICE] = static_cast<int>(group.population_count * 0.2);
                group.employment[EmploymentType::CRAFTING] = static_cast<int>(group.population_count * 0.2);
                group.employment[EmploymentType::MERCENARY_SERVICE] = static_cast<int>(group.population_count * 0.2);
                break;

            default:
                group.employment[EmploymentType::UNEMPLOYED_SEEKING] = group.population_count;
                break;
        }

        // Calculate employment rate
        int employed = 0;
        for (const auto& [employment_type, count] : group.employment) {
            if (employment_type != EmploymentType::UNEMPLOYED_SEEKING && 
                employment_type != EmploymentType::UNEMPLOYABLE) {
                employed += count;
            }
        }
        group.employment_rate = group.population_count > 0 ? 
            static_cast<double>(employed) / group.population_count : 0.0;
    }

    // Function removed - replaced by PopulationAggregator::RecalculateAllAggregates()

    void EnhancedPopulationFactory::RecalculateSettlementSummary(SettlementComponent& settlements) {
        // Reset totals
        settlements.settlement_counts.clear();
        settlements.total_production_value = 0.0;
        settlements.urbanization_rate = 0.0;
        settlements.military_settlements = 0;
        settlements.economic_settlements = 0;
        settlements.religious_settlements = 0;
        settlements.administrative_settlements = 0;

        int total_population = 0;
        int urban_population = 0;
        double total_prosperity = 0.0;
        double total_infrastructure = 0.0;

        // Aggregate from all settlements
        for (const auto& settlement : settlements.settlements) {
            settlements.settlement_counts[settlement.type]++;
            total_population += settlement.total_population;
            total_prosperity += settlement.prosperity_level;
            total_infrastructure += settlement.infrastructure_level;

            // Categorize settlements
            if (utils::IsUrbanSettlement(settlement.type)) {
                urban_population += settlement.total_population;
            }

            if (utils::IsMilitarySettlement(settlement.type)) {
                settlements.military_settlements++;
            } else if (utils::IsEconomicSettlement(settlement.type)) {
                settlements.economic_settlements++;
            } else if (utils::IsReligiousSettlement(settlement.type)) {
                settlements.religious_settlements++;
            }

            // Add to production value
            for (const auto& [resource, production] : settlement.production) {
                settlements.total_production_value += production;
            }
        }

        // Calculate averages
        size_t settlement_count = settlements.settlements.size();
        if (settlement_count > 0) {
            settlements.average_prosperity = total_prosperity / settlement_count;
            settlements.average_infrastructure = total_infrastructure / settlement_count;
        }

        // Calculate urbanization rate
        if (total_population > 0) {
            settlements.urbanization_rate = static_cast<double>(urban_population) / total_population;
        }
    }

    // Additional helper methods would continue here...
    // These implement the remaining private methods declared in the header

    std::string EnhancedPopulationFactory::DetermineForeignCulture(const std::string& local_culture, int year) {
        // Simple implementation - return different culture based on local culture
        if (local_culture == "english") return "french";
        if (local_culture == "french") return "flemish";
        if (local_culture == "german") return "italian";
        return "byzantine";
    }

    std::string EnhancedPopulationFactory::DetermineForeignReligion(const std::string& local_religion, int year) {
        // Simple implementation - return different religion based on local religion
        if (local_religion == "catholic") return "orthodox";
        if (local_religion == "orthodox") return "catholic";
        return "islamic";
    }

    SettlementType EnhancedPopulationFactory::DetermineMainCityType(int urban_population, double prosperity_level) {
        // Large cities require substantial population AND prosperity
        if (urban_population >= 10000 && prosperity_level > 0.7) {
            return SettlementType::LARGE_CITY;
        }
        // Major cities need good population with decent prosperity
        if (urban_population >= 5000 && prosperity_level > 0.6) {
            return SettlementType::CITY;
        }
        // Free cities are independent and prosperous, but may be smaller
        if (urban_population >= 3000 && prosperity_level > 0.75) {
            return SettlementType::FREE_CITY;
        }
        // Regular cities with moderate population
        if (urban_population >= 2000) {
            return SettlementType::CITY;
        }
        // Towns for smaller urban centers
        if (urban_population >= 500) {
            return SettlementType::MARKET_TOWN;
        }
        // Default to market town for very small urban areas
        return SettlementType::MARKET_TOWN;
    }

    double EnhancedPopulationFactory::CalculateUrbanizationRate(int total_population, double prosperity_level, int year) {
        double base_rate = 0.05;
        if (year < 1100) {
            base_rate = 0.03;
        } else if (year >= 1100 && year < 1300) {
            base_rate = 0.08;
        } else if (year >= 1300) {
            base_rate = 0.12;
        }
        
        // Adjust for prosperity
        base_rate *= (0.5 + prosperity_level);
        
        // Limit to reasonable bounds
        return std::min(0.25, std::max(0.02, base_rate));
    }

    void EnhancedPopulationFactory::SetEconomicSpecializations(Settlement& settlement, const std::vector<std::string>& strategic_resources, double prosperity_level) {
        settlement.economic_specializations.clear();
        settlement.economic_specializations.push_back("trade");
        for (const auto& resource : strategic_resources) {
            if (resource == "iron") settlement.economic_specializations.push_back("blacksmithing");
            else if (resource == "grain") settlement.economic_specializations.push_back("milling");
            else if (resource == "timber") settlement.economic_specializations.push_back("carpentry");
            else if (resource == "salt") settlement.economic_specializations.push_back("saltworks");
            else if (resource == "wool") settlement.economic_specializations.push_back("textiles");
            else if (resource == "wine") settlement.economic_specializations.push_back("winemaking");
            else if (resource == "fish") settlement.economic_specializations.push_back("fishing");
            else if (resource == "horses") settlement.economic_specializations.push_back("stables");
            else if (resource == "silver") settlement.economic_specializations.push_back("minting");
        }
        if (prosperity_level > 0.7) {
            settlement.economic_specializations.push_back("luxury_goods");
        }
        if (settlement.type == SettlementType::PORT_TOWN) {
            settlement.economic_specializations.push_back("shipping");
            settlement.economic_specializations.push_back("fishing");
        } else if (settlement.type == SettlementType::MARKET_TOWN) {
            settlement.economic_specializations.push_back("livestock_trade");
        } else if (settlement.type == SettlementType::GUILD_TOWN) {
            settlement.economic_specializations.push_back("guild_crafts");
        }
        if (settlement.economic_specializations.size() <= 1) {
            settlement.economic_specializations.push_back("manufacturing");
        }
    }

    double EnhancedPopulationFactory::CalculateFreePeasantPercentage(int year, double prosperity_level) {
        double base_percentage = 0.25;
        if (year < 1100) {
            base_percentage = 0.15;
        } else if (year >= 1100 && year < 1300) {
            base_percentage = 0.25;
        } else if (year >= 1300 && year < 1500) {
            base_percentage = 0.35;
        } else if (year >= 1500) {
            base_percentage = 0.45;
        }
        base_percentage += prosperity_level * 0.2;
        return std::min(0.6, base_percentage);
    }

    double EnhancedPopulationFactory::CalculateVilleinPercentage(int year, double prosperity_level) {
        double base_percentage = 0.35;
        if (year < 1100) {
            base_percentage = 0.25;
        } else if (year >= 1100 && year < 1300) {
            base_percentage = 0.40;
        } else if (year >= 1300 && year < 1500) {
            base_percentage = 0.35;
        } else if (year >= 1500) {
            base_percentage = 0.25;
        }
        if (prosperity_level > 0.4 && prosperity_level < 0.7) {
            base_percentage += 0.05;
        }
        return base_percentage;
    }

    double EnhancedPopulationFactory::GetSettlementInfrastructure(SettlementType type, double prosperity_level) {
        double base_infrastructure = 0.3;
        double prosperity_modifier = 0.8 + (prosperity_level * 0.4);
        return std::min(1.0, base_infrastructure * prosperity_modifier);
    }

    double EnhancedPopulationFactory::GetSettlementFortification(SettlementType type, double prosperity_level) {
        double base_fortification = 0.1;
        if (type == SettlementType::ROYAL_CASTLE || type == SettlementType::DUCAL_CASTLE || type == SettlementType::BORDER_FORTRESS) {
            base_fortification = 0.7;
        } else if (type == SettlementType::LARGE_CITY) {
            base_fortification = 0.35;
        } else if (type == SettlementType::CITY || type == SettlementType::FREE_CITY) {
            base_fortification = 0.25;
        } else if (type == SettlementType::MARKET_TOWN || type == SettlementType::GUILD_TOWN || type == SettlementType::PORT_TOWN) {
            base_fortification = 0.15;
        }
        double prosperity_modifier = 0.7 + (prosperity_level * 0.5);
        return std::min(1.0, base_fortification * prosperity_modifier);
    }

    double EnhancedPopulationFactory::GetSettlementSanitation(SettlementType type, double prosperity_level) {
        double base_sanitation = 0.2;
        if (type == SettlementType::LARGE_CITY || type == SettlementType::CITY || type == SettlementType::FREE_CITY) {
            base_sanitation = 0.35;
        } else if (type == SettlementType::MARKET_TOWN || type == SettlementType::GUILD_TOWN || type == SettlementType::PORT_TOWN) {
            base_sanitation = 0.25;
        }
        double prosperity_modifier = 0.8 + (prosperity_level * 0.4);
        return std::min(1.0, base_sanitation * prosperity_modifier);
    }

    double EnhancedPopulationFactory::GetSettlementWaterAccess(SettlementType type, double prosperity_level) {
        double base_water = 0.3;
        if (type == SettlementType::LARGE_CITY || type == SettlementType::CITY || type == SettlementType::FREE_CITY) {
            base_water = 0.5;
        } else if (type == SettlementType::MARKET_TOWN || type == SettlementType::GUILD_TOWN || type == SettlementType::PORT_TOWN) {
            base_water = 0.4;
        }
        double prosperity_modifier = 0.7 + (prosperity_level * 0.5);
        return std::min(1.0, base_water * prosperity_modifier);
    }

    double EnhancedPopulationFactory::GetSettlementAutonomy(SettlementType type) {
        switch (type) {
            case SettlementType::FREE_CITY:
                return 0.8;
            case SettlementType::GUILD_TOWN:
                return 0.6;
            case SettlementType::MARKET_TOWN:
                return 0.4;
            case SettlementType::CITY:
            case SettlementType::LARGE_CITY:
                return 0.5;
            default:
                return 0.2;
        }
    }

    double EnhancedPopulationFactory::GetSettlementDiseaseRisk(SettlementType type, double prosperity_level) {
        double base_risk = 0.1;
        if (type == SettlementType::LARGE_CITY || type == SettlementType::CITY || type == SettlementType::FREE_CITY) {
            base_risk = 0.25;
        } else if (type == SettlementType::MARKET_TOWN || type == SettlementType::GUILD_TOWN || type == SettlementType::PORT_TOWN) {
            base_risk = 0.18;
        }
        double prosperity_modifier = 1.2 - (prosperity_level * 0.5);
        return std::max(0.01, base_risk * prosperity_modifier);
    }

    void EnhancedPopulationFactory::CreateSecondaryUrbanSettlements(SettlementComponent& settlements, const std::string& province_name, int remaining_population, double prosperity_level, const std::string& culture, const std::string& religion, int year, const std::vector<std::string>& strategic_resources) {
        // Example: create a secondary market town if enough population remains
        if (remaining_population > 1000) {
            Settlement secondary = CreateSettlement(province_name + " Secondary Town", SettlementType::MARKET_TOWN, province_name, prosperity_level);
            SetEconomicSpecializations(secondary, strategic_resources, prosperity_level);
            settlements.settlements.push_back(secondary);
        }
    }

    void EnhancedPopulationFactory::SetDemographicRates(PopulationGroup& group, SocialClass social_class, double prosperity_level) {
        group.birth_rate = 0.035;
        group.death_rate = 0.030;
        group.infant_mortality = 0.25;
        group.maternal_mortality = 0.02;
        double prosperity_modifier = 0.7 + (prosperity_level * 0.6);
        group.death_rate *= (2.0 - prosperity_modifier);
        group.infant_mortality *= (2.0 - prosperity_modifier);
        group.maternal_mortality *= (2.0 - prosperity_modifier);
        if (prosperity_level > 0.6) {
            group.birth_rate *= 1.1;
        } else if (prosperity_level < 0.3) {
            group.birth_rate *= 0.9;
        }
        group.migration_tendency = 0.1;
        if (social_class == SocialClass::SERFS || social_class == SocialClass::SLAVES) {
            group.migration_tendency = 0.02;
        } else if (social_class == SocialClass::FOREIGNERS || social_class == SocialClass::OUTLAWS) {
            group.migration_tendency = 0.4;
        } else if (social_class == SocialClass::URBAN_LABORERS) {
            group.migration_tendency = 0.2;
        }
        if (prosperity_level < 0.4) {
            group.migration_tendency *= 1.5;
        }
    }

    void EnhancedPopulationFactory::SetCulturalFactors(PopulationGroup& group, SocialClass social_class, int year) {
        group.assimilation_rate = 0.02;
        if (social_class == SocialClass::HIGH_NOBILITY || social_class == SocialClass::LESSER_NOBILITY) {
            group.assimilation_rate *= 0.5;
        } else if (social_class == SocialClass::SERFS || social_class == SocialClass::URBAN_LABORERS) {
            group.assimilation_rate *= 1.5;
        }
        group.conversion_rate = 0.01;
        if (social_class == SocialClass::HIGH_CLERGY || social_class == SocialClass::CLERGY) {
            group.conversion_rate *= 0.5;
        } else if (social_class == SocialClass::WEALTHY_MERCHANTS || social_class == SocialClass::SCHOLARS) {
            group.conversion_rate *= 1.3;
        }
        if (year >= 1300 && year < 1500) {
            group.assimilation_rate *= 1.2;
            group.conversion_rate *= 1.2;
        } else if (year >= 1500) {
            group.assimilation_rate *= 1.5;
            group.conversion_rate *= 1.5;
        }
        group.education_access = 0.1;
        switch (social_class) {
            case SocialClass::HIGH_NOBILITY:
            case SocialClass::HIGH_CLERGY:
                group.education_access = 0.8;
                break;
            case SocialClass::LESSER_NOBILITY:
            case SocialClass::WEALTHY_MERCHANTS:
                group.education_access = 0.5;
                break;
            case SocialClass::SCHOLARS:
                group.education_access = 0.9;
                break;
            default:
                group.education_access = 0.1;
                break;
        }
        if (year >= 1300) {
            group.education_access *= 1.2;
        }
        if (year >= 1500) {
            group.education_access *= 1.5;
        }
    }

    double EnhancedPopulationFactory::GetClassBaseHappiness(SocialClass social_class, double prosperity_level) {
        double base_happiness = 0.5;
        switch (social_class) {
            case SocialClass::HIGH_NOBILITY:
            case SocialClass::HIGH_CLERGY:
                base_happiness = 0.8;
                break;
            case SocialClass::LESSER_NOBILITY:
            case SocialClass::WEALTHY_MERCHANTS:
                base_happiness = 0.7;
                break;
            case SocialClass::CLERGY:
            case SocialClass::BURGHERS:
                base_happiness = 0.6;
                break;
            case SocialClass::CRAFTSMEN:
            case SocialClass::SCHOLARS:
                base_happiness = 0.55;
                break;
            case SocialClass::FREE_PEASANTS:
                base_happiness = 0.4;
                break;
            case SocialClass::URBAN_LABORERS:
                base_happiness = 0.35;
                break;
            case SocialClass::SERFS:
                base_happiness = 0.3;
                break;
            case SocialClass::SLAVES:
            case SocialClass::OUTLAWS:
                base_happiness = 0.2;
                break;
            default:
                base_happiness = 0.4;
                break;
        }
        return base_happiness * (0.7 + prosperity_level * 0.6);
    }

    double EnhancedPopulationFactory::GetClassHealthLevel(SocialClass social_class, double prosperity_level) {
        double base_health = 0.6;
        switch (social_class) {
            case SocialClass::HIGH_NOBILITY:
            case SocialClass::HIGH_CLERGY:
                base_health = 0.85;
                break;
            case SocialClass::LESSER_NOBILITY:
            case SocialClass::WEALTHY_MERCHANTS:
                base_health = 0.75;
                break;
            case SocialClass::CLERGY:
            case SocialClass::BURGHERS:
                base_health = 0.7;
                break;
            case SocialClass::CRAFTSMEN:
            case SocialClass::SCHOLARS:
                base_health = 0.65;
                break;
            case SocialClass::FREE_PEASANTS:
                base_health = 0.5;
                break;
            case SocialClass::URBAN_LABORERS:
                base_health = 0.45;
                break;
            case SocialClass::SERFS:
                base_health = 0.4;
                break;
            case SocialClass::SLAVES:
                base_health = 0.3;
                break;
            default:
                base_health = 0.5;
                break;
        }
        return base_health * (0.8 + prosperity_level * 0.4);
    }

    int EnhancedPopulationFactory::CalculateMilitaryEligible(const PopulationGroup& group) {
        // Assume roughly 25% of adult males are military eligible
        return static_cast<int>(group.population_count * 0.25);
    }

    double EnhancedPopulationFactory::CalculateMilitaryQuality(SocialClass social_class, double prosperity_level) {
        double base_quality = 0.3;
        switch (social_class) {
            case SocialClass::HIGH_NOBILITY:
            case SocialClass::LESSER_NOBILITY:
                base_quality = 0.8;
                break;
            case SocialClass::CRAFTSMEN:
                base_quality = 0.5;
                break;
            case SocialClass::FREE_PEASANTS:
                base_quality = 0.4;
                break;
            case SocialClass::URBAN_LABORERS:
                base_quality = 0.35;
                break;
            case SocialClass::SERFS:
                base_quality = 0.25;
                break;
            case SocialClass::SLAVES:
                base_quality = 0.2;
                break;
            default:
                base_quality = 0.3;
                break;
        }
        return base_quality * (0.7 + prosperity_level * 0.6);
    }

    std::vector<std::string> EnhancedPopulationFactory::GetLegalPrivileges(LegalStatus status) {
        std::vector<std::string> privileges;
        switch (status) {
            case LegalStatus::FREE_PEASANT:
                privileges = {"trade", "movement", "justice_access"};
                break;
            case LegalStatus::FULL_CITIZEN:
            case LegalStatus::BURGHER_RIGHTS:
                privileges = {"trade", "movement", "justice_access", "political_participation"};
                break;
            case LegalStatus::ROYAL_WARD:
                privileges = {"trade", "movement", "justice_access", "political_participation", "military_command", "land_ownership"};
                break;
            case LegalStatus::CLERIC:
                privileges = {"religious_authority", "tax_exemption", "sanctuary_rights"};
                break;
            default:
                privileges = {};
                break;
        }
        return privileges;
    }

    std::vector<std::string> EnhancedPopulationFactory::GetEconomicRights(LegalStatus status) {
        std::vector<std::string> rights;
        switch (status) {
            case LegalStatus::FREE_PEASANT:
            case LegalStatus::FULL_CITIZEN:
            case LegalStatus::BURGHER_RIGHTS:
                rights = {"property_ownership", "contract_making", "guild_membership"};
                break;
            case LegalStatus::ROYAL_WARD:
                rights = {"property_ownership", "contract_making", "guild_membership", "large_scale_trade", "land_grants"};
                break;
            case LegalStatus::CLERIC:
                rights = {"church_property", "tithe_collection"};
                break;
            case LegalStatus::SERF:
                rights = {"limited_property"};
                break;
            default:
                rights = {};
                break;
        }
        return rights;
    }

    std::vector<std::string> EnhancedPopulationFactory::GetSocialRestrictions(LegalStatus status) {
        std::vector<std::string> restrictions;
        switch (status) {
            case LegalStatus::SERF:
                restrictions = {"movement_restricted", "marriage_approval_required", "service_obligations"};
                break;
            case LegalStatus::SLAVE:
                restrictions = {"no_legal_rights", "property_status", "complete_subjugation"};
                break;
            case LegalStatus::OUTLAW:
                restrictions = {"social_exile", "no_legal_protection", "persecution_risk"};
                break;
            case LegalStatus::FOREIGNER:
                restrictions = {"limited_rights", "cultural_barriers", "legal_uncertainty"};
                break;
            default:
                restrictions = {};
                break;
        }
        return restrictions;
    }

} // namespace game::population
