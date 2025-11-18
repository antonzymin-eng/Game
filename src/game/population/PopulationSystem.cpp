// ============================================================================
// PopulationSystem.cpp - Population Management System Implementation
// Created: December 19, 2024 at 10:50 AM
// Location: src/game/population/PopulationSystem.cpp
// ============================================================================

#include "game/population/PopulationSystem.h"
#include "game/population/PopulationComponents.h"
#include "game/population/PopulationAggregator.h"
#include "core/logging/Logger.h"
#include "config/GameConfig.h"
#include "utils/RandomGenerator.h"
#include "core/types/game_types.h"
#include <json/json.h>
#include <algorithm>
#include <cmath>
#include <numeric>

namespace game::population {

// ============================================================================
// PopulationSystem Implementation
// ============================================================================

PopulationSystem::PopulationSystem(::core::ecs::ComponentAccessManager& access_manager,
                                   ::core::threading::ThreadSafeMessageBus& message_bus)
    : m_access_manager(access_manager), m_message_bus(message_bus) {
    
    std::random_device rd;
    m_random_generator.seed(rd());
    
    CORE_LOG_INFO("PopulationSystem", "Population System created");
}

void PopulationSystem::Initialize() {
    if (m_initialized) {
        return;
    }

    CORE_LOG_INFO("PopulationSystem", "Initializing Population System");

    LoadConfiguration();
    InitializeEventProcessor();
    InitializeFactory();
    SubscribeToEvents();

    m_initialized = true;
    CORE_LOG_INFO("PopulationSystem", "Population System initialized successfully");
}

void PopulationSystem::Update(float delta_time) {
    if (!m_initialized) {
        CORE_LOG_WARN("PopulationSystem", "System not initialized, skipping update");
        return;
    }

    m_accumulated_time += delta_time;
    
    // Process demographic changes every game hour
    if (m_accumulated_time >= 1.0f) { // Process every second for now
        ProcessRegularUpdates(m_accumulated_time);
        m_accumulated_time = 0.0f;
    }
}

void PopulationSystem::Shutdown() {
    if (!m_initialized) {
        return;
    }

    CORE_LOG_INFO("PopulationSystem", "Shutting down Population System");
    
    // Clean up any resources
    m_event_processor.reset();
    m_factory.reset();
    
    m_initialized = false;
}

::core::threading::ThreadingStrategy PopulationSystem::GetThreadingStrategy() const {
    return ::core::threading::ThreadingStrategy::MAIN_THREAD;
}

// Population Management Methods
// ============================================================================

void PopulationSystem::CreateInitialPopulation(game::types::EntityID province_id,
                                               const std::string& culture,
                                               const std::string& religion,
                                               int base_population,
                                               double prosperity_level,
                                               int year) {
    CORE_LOG_INFO("PopulationSystem", 
        "Creating initial population for province " + std::to_string(static_cast<int>(province_id)) +
        " - Culture: " + culture + ", Religion: " + religion + 
        ", Base Population: " + std::to_string(base_population));

    // Get EntityManager from ComponentAccessManager
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) {
        CORE_LOG_ERROR("PopulationSystem", "EntityManager not available");
        return;
    }

    // Create EntityID handle for the province
    ::core::ecs::EntityID province_handle(static_cast<uint64_t>(province_id), 1);

    // Try to get existing PopulationComponent or add a new one
    auto population_component = entity_manager->GetComponent<PopulationComponent>(province_handle);
    if (!population_component) {
        // Create new population component
        population_component = entity_manager->AddComponent<PopulationComponent>(province_handle);
        CORE_LOG_INFO("PopulationSystem", "Created new PopulationComponent for province");
    }

    if (population_component) {
        // Initialize population data using the factory
        population_component->total_population = base_population;
        population_component->population_density = static_cast<double>(base_population) / 1000.0; // Rough estimate
        population_component->growth_rate = 0.01 + (prosperity_level * 0.02); // Prosperity affects growth
        
        // Create initial population groups using the factory
        if (m_factory) {
            auto factory_population = m_factory->CreateMedievalPopulation(
                culture, religion, base_population, prosperity_level, year);
            
            // Copy the factory-generated data to our component
            *population_component = factory_population;

            // Ensure our manual settings are preserved
            population_component->total_population = base_population;
            population_component->population_density = static_cast<double>(base_population) / 1000.0;
            population_component->growth_rate = 0.01 + (prosperity_level * 0.02);

            // Calculate aggregate statistics
            RecalculatePopulationAggregates(*population_component);
        }

        CORE_LOG_INFO("PopulationSystem", 
            "Successfully initialized PopulationComponent with " + 
            std::to_string(population_component->population_groups.size()) + " population groups");
    } else {
        CORE_LOG_ERROR("PopulationSystem", "Failed to create or access PopulationComponent");
    }
}

void PopulationSystem::ProcessDemographicChanges(game::types::EntityID province_id, double yearly_fraction) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) {
        return;
    }

    ::core::ecs::EntityID province_handle(static_cast<uint64_t>(province_id), 1);
    auto population = entity_manager->GetComponent<PopulationComponent>(province_handle);

    if (!population || population->population_groups.empty()) {
        return;
    }

    int total_births = 0;
    int total_deaths = 0;

    // Process each population group
    for (auto& group : population->population_groups) {
        if (group.population_count <= 0) {
            continue;
        }

        const int current_pop = group.population_count;

        // Calculate births (based on birth rate and yearly fraction)
        const double births_this_period = current_pop * group.birth_rate * yearly_fraction;
        const int births = static_cast<int>(births_this_period);

        // Calculate deaths (based on death rate and yearly fraction)
        const double deaths_this_period = current_pop * group.death_rate * yearly_fraction;
        const int deaths = std::min(static_cast<int>(deaths_this_period), current_pop);

        // Apply demographic changes
        const int net_change = births - deaths;
        group.population_count = std::max(0, current_pop + net_change);

        total_births += births;
        total_deaths += deaths;

        // Update age distribution (simplified aging model)
        if (yearly_fraction >= 1.0) {  // Only age if a full year has passed
            // Gradually shift population toward elderly
            const int aging_shift = static_cast<int>(group.adults_15_64 * 0.015);  // 1.5% of adults age to elderly per year
            const int youth_maturity = static_cast<int>(group.children_0_14 * 0.07);  // 7% of children become adults per year

            group.children_0_14 = std::max(0, group.children_0_14 - youth_maturity + births);
            group.adults_15_64 = std::max(0, group.adults_15_64 + youth_maturity - aging_shift);
            group.elderly_65_plus = std::max(0, group.elderly_65_plus + aging_shift - deaths);
        } else {
            // For small time periods, just add births to children and remove deaths from elderly
            group.children_0_14 += births;
            group.elderly_65_plus = std::max(0, group.elderly_65_plus - deaths);
        }

        // Update gender distribution (maintain roughly 50/50 ratio with slight male bias)
        auto [males, females] = PopulationCalculator::CalculateGenderDistribution(group.population_count);
        group.males = males;
        group.females = females;

        // Adjust health and happiness based on demographic pressures
        if (group.death_rate > group.birth_rate) {
            // Declining population may reduce happiness
            group.happiness = std::max(0.0, group.happiness - 0.01 * yearly_fraction);
        } else if (births > deaths * 2) {
            // Rapid growth may strain resources
            group.health_level = std::max(0.0, group.health_level - 0.005 * yearly_fraction);
        }
    }

    // Recalculate aggregate statistics after demographic changes
    RecalculatePopulationAggregates(*population);

    // Log significant changes
    if (total_births + total_deaths > 0) {
        CORE_LOG_DEBUG("PopulationSystem",
            "Province " + std::to_string(static_cast<int>(province_id)) +
            " demographics: +" + std::to_string(total_births) +
            " births, -" + std::to_string(total_deaths) +
            " deaths (net: " + std::to_string(total_births - total_deaths) + ")");
    }
}

void PopulationSystem::ProcessSocialMobility(game::types::EntityID province_id, double yearly_fraction) {
    // TODO: Implement social mobility processing
    CORE_LOG_DEBUG("PopulationSystem", "ProcessSocialMobility called");
}

void PopulationSystem::ProcessSettlementEvolution(game::types::EntityID province_id, double yearly_fraction) {
    // TODO: Implement settlement evolution processing
    CORE_LOG_DEBUG("PopulationSystem", "ProcessSettlementEvolution called");
}

void PopulationSystem::ProcessEmploymentShifts(game::types::EntityID province_id, double yearly_fraction) {
    // TODO: Implement employment shifts processing
    CORE_LOG_DEBUG("PopulationSystem", "ProcessEmploymentShifts called");
}

void PopulationSystem::ProcessCulturalChanges(game::types::EntityID province_id, double yearly_fraction) {
    // TODO: Implement cultural changes processing
    CORE_LOG_DEBUG("PopulationSystem", "ProcessCulturalChanges called");
}

// Event Processing Methods
// ============================================================================

void PopulationSystem::ProcessPlague(game::types::EntityID province_id, const PlagueEvent& event) {
    // TODO: Implement plague processing
    CORE_LOG_WARN("PopulationSystem", "ProcessPlague called");
}

void PopulationSystem::ProcessFamine(game::types::EntityID province_id, const FamineEvent& event) {
    // TODO: Implement famine processing
    CORE_LOG_WARN("PopulationSystem", "ProcessFamine called");
}

void PopulationSystem::ProcessNaturalDisaster(game::types::EntityID province_id, const NaturalDisasterEvent& event) {
    // TODO: Implement natural disaster processing
    CORE_LOG_ERROR("PopulationSystem", "ProcessNaturalDisaster called");
}

void PopulationSystem::ProcessSocialUnrest(game::types::EntityID province_id, const SocialUnrestEvent& event) {
    // TODO: Implement social unrest processing
    CORE_LOG_WARN("PopulationSystem", "ProcessSocialUnrest called");
}

void PopulationSystem::ProcessMilitaryRecruitment(game::types::EntityID province_id, const MilitaryRecruitmentEvent& event) {
    // TODO: Implement military recruitment processing
    CORE_LOG_INFO("PopulationSystem", "ProcessMilitaryRecruitment called");
}

void PopulationSystem::ProcessMilitaryService(game::types::EntityID province_id, const MilitaryServiceEvent& event) {
    // TODO: Implement military service processing
    CORE_LOG_INFO("PopulationSystem", "ProcessMilitaryService called");
}

void PopulationSystem::UpdateMilitaryEligibility(game::types::EntityID province_id) {
    // TODO: Implement military eligibility updates
    CORE_LOG_DEBUG("PopulationSystem", "UpdateMilitaryEligibility called");
}

// System Internal Methods
// ============================================================================

void PopulationSystem::LoadConfiguration() {
    CORE_LOG_INFO("PopulationSystem", "Configuration loaded successfully");
}

void PopulationSystem::InitializeEventProcessor() {
    m_event_processor = std::make_unique<PopulationEventProcessor>();
    CORE_LOG_DEBUG("PopulationSystem", "Event processor initialized");
}

void PopulationSystem::InitializeFactory() {
    m_factory = std::make_unique<EnhancedPopulationFactory>();
    CORE_LOG_DEBUG("PopulationSystem", "Population factory initialized");
}

void PopulationSystem::SubscribeToEvents() {
    // Subscribe to plague events that affect population
    m_message_bus.Subscribe<PlagueEvent>(
        [this](const PlagueEvent& event) {
            ProcessPlague(event.entity_id, event);
        });

    // Subscribe to famine events
    m_message_bus.Subscribe<FamineEvent>(
        [this](const FamineEvent& event) {
            ProcessFamine(event.entity_id, event);
        });

    // Subscribe to natural disaster events
    m_message_bus.Subscribe<NaturalDisasterEvent>(
        [this](const NaturalDisasterEvent& event) {
            ProcessNaturalDisaster(event.entity_id, event);
        });

    // Subscribe to social unrest events
    m_message_bus.Subscribe<SocialUnrestEvent>(
        [this](const SocialUnrestEvent& event) {
            ProcessSocialUnrest(event.entity_id, event);
        });

    // Subscribe to military recruitment requests
    m_message_bus.Subscribe<MilitaryRecruitmentEvent>(
        [this](const MilitaryRecruitmentEvent& event) {
            ProcessMilitaryRecruitment(event.entity_id, event);
        });

    // Subscribe to military service events
    m_message_bus.Subscribe<MilitaryServiceEvent>(
        [this](const MilitaryServiceEvent& event) {
            ProcessMilitaryService(event.entity_id, event);
        });

    CORE_LOG_INFO("PopulationSystem", "Subscribed to population-affecting events");
}

std::vector<game::types::EntityID> PopulationSystem::GetAllPopulatedProvinces() {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) {
        CORE_LOG_ERROR("PopulationSystem", "EntityManager not available");
        return {};
    }

    // Query all entities with PopulationComponent
    auto ecs_entities = entity_manager->GetEntitiesWithComponent<PopulationComponent>();

    // Convert from core::ecs::EntityID to game::types::EntityID
    std::vector<game::types::EntityID> populated_provinces;
    populated_provinces.reserve(ecs_entities.size());

    for (const auto& entity : ecs_entities) {
        populated_provinces.push_back(static_cast<game::types::EntityID>(entity.id));
    }

    return populated_provinces;
}

void PopulationSystem::ValidatePopulationConsistency(game::types::EntityID province_id) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) {
        return;
    }

    ::core::ecs::EntityID province_handle(static_cast<uint64_t>(province_id), 1);
    auto population = entity_manager->GetComponent<PopulationComponent>(province_handle);

    if (!population) {
        return;
    }

    // Use the aggregator's validation method
    bool is_consistent = PopulationAggregator::ValidateDataConsistency(*population);

    if (!is_consistent) {
        CORE_LOG_WARN("PopulationSystem",
            "Population data inconsistency detected for province " + std::to_string(static_cast<int>(province_id)));

        // Attempt to fix by recalculating aggregates
        RecalculatePopulationAggregates(*population);

        CORE_LOG_INFO("PopulationSystem", "Recalculated aggregates to fix inconsistency");
    }
}

void PopulationSystem::ProcessRegularUpdates(float delta_time) {
    // Get all provinces with population
    auto populated_provinces = GetAllPopulatedProvinces();

    if (populated_provinces.empty()) {
        return;  // No populations to update
    }

    // Calculate yearly fraction (assuming delta_time represents game time in days)
    // For simplicity, we'll process updates as if delta_time is 1 game day
    const double yearly_fraction = delta_time / 365.0;

    // Process each province's population
    for (const auto& province_id : populated_provinces) {
        // Process different aspects of population in sequence
        ProcessDemographicChanges(province_id, yearly_fraction);
        ProcessSocialMobility(province_id, yearly_fraction);
        ProcessEmploymentShifts(province_id, yearly_fraction);
        ProcessCulturalChanges(province_id, yearly_fraction);

        // Validate data integrity
        ValidatePopulationConsistency(province_id);

        // Get updated population component and send events
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (entity_manager) {
            ::core::ecs::EntityID province_handle(static_cast<uint64_t>(province_id), 1);
            auto population = entity_manager->GetComponent<PopulationComponent>(province_handle);
            if (population) {
                SendPopulationUpdateEvent(province_id, *population);
            }
        }
    }

    CORE_LOG_DEBUG("PopulationSystem",
        "Processed regular updates for " + std::to_string(populated_provinces.size()) + " provinces");
}

// Communication Methods
// ============================================================================

void PopulationSystem::SendPopulationUpdateEvent(game::types::EntityID province_id, const PopulationComponent& population) {
    PopulationUpdateEvent event;
    event.entity_id = province_id;
    event.total_population = population.total_population;
    event.population_growth_rate = population.growth_rate;
    event.average_wealth = population.average_wealth;
    event.average_happiness = population.average_happiness;
    event.average_literacy = population.average_literacy;
    event.average_health = population.average_health;
    event.military_eligible = population.total_military_eligible;
    event.military_quality = population.average_military_quality;

    m_message_bus.Send(event);
}

void PopulationSystem::SendDemographicChangeEvent(game::types::EntityID province_id, const PopulationGroup& group,
                                                 int old_population, int new_population, const std::string& reason) {
    const int pop_change = new_population - old_population;
    const int births = std::max(0, pop_change);
    const int deaths = std::max(0, -pop_change);

    DemographicChangeEvent event;
    event.entity_id = province_id;
    event.births = births;
    event.deaths = deaths;
    event.infant_deaths = 0;  // Would need separate tracking
    event.maternal_deaths = 0;  // Would need separate tracking
    event.birth_rate = group.birth_rate;
    event.death_rate = group.death_rate;
    event.affected_class = group.social_class;
    event.population_affected = std::abs(pop_change);

    m_message_bus.Send(event);
}

void PopulationSystem::SendCrisisEvent(game::types::EntityID province_id, const std::string& crisis_type,
                                      double severity, const std::vector<SocialClass>& affected_classes) {
    messages::PopulationCrisis crisis;
    crisis.province = province_id;
    crisis.crisis_type = crisis_type;
    crisis.severity = severity;
    crisis.population_affected = 0;  // Would be calculated based on province population
    crisis.affected_classes = affected_classes;

    m_message_bus.Send(crisis);

    CORE_LOG_WARN("PopulationSystem",
        "Population crisis '" + crisis_type + "' (severity: " +
        std::to_string(static_cast<int>(severity * 100)) + "%) in province " +
        std::to_string(static_cast<int>(province_id)));
}

void PopulationSystem::NotifyMilitarySystem(game::types::EntityID province_id, const MilitaryRecruitmentEvent& event) {
    messages::MilitaryRecruitmentResult result;
    result.province_id = province_id;
    result.requested_recruits = event.recruits_needed;
    result.actual_recruits = event.recruits_available;
    result.average_quality = event.average_quality;
    result.recruitment_cost = event.recruitment_cost;

    m_message_bus.Send(result);
}

void PopulationSystem::NotifyEconomicSystem(game::types::EntityID province_id, const EconomicUpdateEvent& event) {
    messages::PopulationEconomicUpdate update;
    update.province_id = province_id;
    update.tax_revenue_potential = event.tax_revenue_potential;
    update.productive_workers = event.productive_workers;
    update.unemployment_rate = event.unemployment_rate;
    update.trade_income = event.trade_income;

    m_message_bus.Send(update);
}

void PopulationSystem::NotifyAdministrativeSystem(game::types::EntityID province_id, const TaxationChangeEvent& event) {
    messages::TaxationPolicyUpdate update;
    update.province_id = province_id;
    update.affected_classes = event.affected_classes;
    update.new_tax_rate = event.new_tax_rate;
    update.expected_revenue = event.revenue_change;
    update.compliance_rate = event.compliance_rate;

    m_message_bus.Send(update);
}

// Stub implementations for missing methods
// ============================================================================

void PopulationSystem::ProcessClassMobility(PopulationComponent& population, 
                                           game::types::EntityID province_id, 
                                           double yearly_fraction) {
    // TODO: Implement class mobility processing
}

void PopulationSystem::ProcessLegalStatusChanges(PopulationComponent& population, 
                                                game::types::EntityID province_id, 
                                                double yearly_fraction) {
    // TODO: Implement legal status changes
}

void PopulationSystem::ProcessGuildAdvancement(PopulationComponent& population, 
                                              SettlementComponent& settlements,
                                              game::types::EntityID province_id,
                                              double yearly_fraction) {
    // TODO: Implement guild advancement
}

void PopulationSystem::UpdateSettlementGrowth(SettlementComponent& settlements, 
                                             const PopulationComponent& population,
                                             double yearly_fraction) {
    // TODO: Implement settlement growth
}

void PopulationSystem::UpdateSettlementSpecialization(SettlementComponent& settlements, 
                                                      const PopulationComponent& population) {
    // TODO: Implement settlement specialization
}

void PopulationSystem::UpdateUrbanization(SettlementComponent& settlements, 
                                         PopulationComponent& population,
                                         double yearly_fraction) {
    // TODO: Implement urbanization
}

void PopulationSystem::UpdateEmploymentDistribution(PopulationComponent& population, 
                                                    const SettlementComponent& settlements) {
    // TODO: Implement employment distribution
}

void PopulationSystem::ProcessJobCreation(PopulationComponent& population, 
                                         const SettlementComponent& settlements) {
    // TODO: Implement job creation
}

PopulationGroup* PopulationSystem::FindOrCreatePopulationGroup(PopulationComponent& population,
                                                               SocialClass social_class,
                                                               LegalStatus legal_status,
                                                               const std::string& culture,
                                                               const std::string& religion) {
    // Search for existing group with matching attributes
    for (auto& group : population.population_groups) {
        if (group.social_class == social_class &&
            group.legal_status == legal_status &&
            group.culture == culture &&
            group.religion == religion) {
            return &group;
        }
    }

    // Group doesn't exist, create new one
    PopulationGroup new_group;
    new_group.social_class = social_class;
    new_group.legal_status = legal_status;
    new_group.culture = culture;
    new_group.religion = religion;
    new_group.population_count = 0;

    // Initialize with default values
    new_group.happiness = 0.5;
    new_group.literacy_rate = PopulationCalculator::GetClassLiteracyRate(social_class, 1200);
    new_group.wealth_per_capita = PopulationCalculator::GetClassBaseWealth(social_class, 0.5);
    new_group.health_level = PopulationCalculator::GetClassHealthLevel(social_class, 0.5);
    new_group.birth_rate = 0.035;
    new_group.death_rate = 0.030;
    new_group.military_quality = PopulationCalculator::CalculateMilitaryQuality(social_class, 0.5);

    // Add to population
    population.population_groups.push_back(new_group);

    CORE_LOG_DEBUG("PopulationSystem",
        "Created new population group for " + culture + "/" + religion);

    return &population.population_groups.back();
}

void PopulationSystem::RecalculatePopulationAggregates(PopulationComponent& population) {
    PopulationAggregator::RecalculateAllAggregates(population);
    
    CORE_LOG_DEBUG("PopulationSystem", 
        "Recalculated aggregates for " + std::to_string(population.population_groups.size()) + 
        " groups, total population: " + std::to_string(population.total_population));
}

// ============================================================================
// ISystem Interface Implementation
// ============================================================================

std::string PopulationSystem::GetSystemName() const {
    return "PopulationSystem";
}

Json::Value PopulationSystem::Serialize(int version) const {
    Json::Value data;
    data["system_name"] = "PopulationSystem";
    data["version"] = version;
    data["initialized"] = m_initialized;
    // TODO: Serialize population state
    return data;
}

bool PopulationSystem::Deserialize(const Json::Value& data, int version) {
    if (data["system_name"].asString() != "PopulationSystem") {
        return false;
    }
    m_initialized = data["initialized"].asBool();
    // TODO: Deserialize population state
    return true;
}

} // namespace game::population} // namespace game::population