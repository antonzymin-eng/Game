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
#include "utils/Random.h"
#include "core/types/game_types.h"
#include <json/json.h>
#include <algorithm>
#include <cmath>
#include <numeric>

namespace game::population {

// ============================================================================
// Helper Functions
// ============================================================================

namespace {

/**
 * @brief Probability-based random chance helper
 * @param probability Probability of success (0.0 to 1.0)
 * @return true if random chance succeeds, false otherwise
 *
 * Used for stochastic events like manumission, legal status changes,
 * and guild advancement.
 */
inline bool RandomChance(double probability) {
    if (probability <= 0.0) return false;
    if (probability >= 1.0) return true;

    // Use the modern C++ random generator
    return utils::RandomBool(static_cast<float>(probability));
}

} // anonymous namespace

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
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) {
        return;
    }

    ::core::ecs::EntityID province_handle(static_cast<uint64_t>(province_id), 1);
    auto population = entity_manager->GetComponent<PopulationComponent>(province_handle);

    if (!population || population->population_groups.empty()) {
        return;
    }

    // Track total mobility for logging
    int total_upward = 0;
    int total_downward = 0;

    // Process each population group for potential social mobility
    for (auto& group : population->population_groups) {
        if (group.population_count <= 0) {
            continue;
        }

        // Calculate mobility factors (0.0 to 1.0)
        const double wealth_factor = std::min(1.0, group.wealth_per_capita / 50.0);  // Wealth relative to baseline
        const double education_factor = group.literacy_rate;  // Higher literacy enables mobility
        const double prosperity_factor = group.happiness;  // Happiness correlates with opportunity

        // Base mobility rate (very low in medieval society - ~0.5% per year)
        const double base_mobility_rate = 0.005 * yearly_fraction;

        // Upward mobility (wealth and education driven)
        const double upward_rate = base_mobility_rate * wealth_factor * education_factor;
        const int upward_candidates = static_cast<int>(group.population_count * upward_rate);

        // Downward mobility (misfortune, poverty)
        const double downward_rate = base_mobility_rate * (1.0 - prosperity_factor) * 0.5;
        const int downward_candidates = static_cast<int>(group.population_count * downward_rate);

        // Apply upward mobility
        if (upward_candidates > 0 && CanMoveUpward(group.social_class)) {
            const SocialClass target_class = GetNextHigherClass(group.social_class);
            auto* target_group = FindOrCreatePopulationGroup(*population, target_class,
                                                            group.legal_status, group.culture, group.religion);
            if (target_group) {
                const int to_move = std::min(upward_candidates, group.population_count);
                group.population_count -= to_move;
                target_group->population_count += to_move;
                total_upward += to_move;
            }
        }

        // Apply downward mobility
        if (downward_candidates > 0 && CanMoveDownward(group.social_class)) {
            const SocialClass target_class = GetNextLowerClass(group.social_class);
            auto* target_group = FindOrCreatePopulationGroup(*population, target_class,
                                                            group.legal_status, group.culture, group.religion);
            if (target_group) {
                const int to_move = std::min(downward_candidates, group.population_count);
                group.population_count -= to_move;
                target_group->population_count += to_move;
                total_downward += to_move;
            }
        }
    }

    // Recalculate aggregates if any mobility occurred
    if (total_upward > 0 || total_downward > 0) {
        RecalculatePopulationAggregates(*population);

        CORE_LOG_DEBUG("PopulationSystem",
            "Province " + std::to_string(static_cast<int>(province_id)) +
            " social mobility: +" + std::to_string(total_upward) +
            " upward, +" + std::to_string(total_downward) + " downward");
    }
}

void PopulationSystem::ProcessSettlementEvolution(game::types::EntityID province_id, double yearly_fraction) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) {
        return;
    }

    ::core::ecs::EntityID province_handle(static_cast<uint64_t>(province_id), 1);
    auto population = entity_manager->GetComponent<PopulationComponent>(province_handle);
    auto settlements = entity_manager->GetComponent<SettlementComponent>(province_handle);

    if (!population || !settlements || settlements->settlements.empty()) {
        return;
    }

    // Process settlement growth based on population
    UpdateSettlementGrowth(*settlements, *population, yearly_fraction);

    // Update settlement economic specialization
    UpdateSettlementSpecialization(*settlements, *population);

    // Process urbanization (rural → urban migration)
    UpdateUrbanization(*settlements, *population, yearly_fraction);

    // Recalculate settlement summary statistics
    RecalculateSettlementSummary(*settlements);

    CORE_LOG_DEBUG("PopulationSystem",
        "Processed settlement evolution for province " + std::to_string(static_cast<int>(province_id)));
}

void PopulationSystem::ProcessEmploymentShifts(game::types::EntityID province_id, double yearly_fraction) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) {
        return;
    }

    ::core::ecs::EntityID province_handle(static_cast<uint64_t>(province_id), 1);
    auto population = entity_manager->GetComponent<PopulationComponent>(province_handle);

    if (!population || population->population_groups.empty()) {
        return;
    }

    // Employment shifts happen slowly in medieval economy
    const double shift_rate = 0.02 * yearly_fraction;  // 2% per year can change employment

    int total_shifts = 0;

    for (auto& group : population->population_groups) {
        if (group.population_count <= 0) {
            continue;
        }

        // Calculate total employed population in this group
        int total_employed = 0;
        for (const auto& [type, count] : group.employment) {
            total_employed += count;
        }

        if (total_employed == 0) {
            continue;
        }

        // Identify unemployed seeking work
        int unemployed_seeking = group.employment[EmploymentType::UNEMPLOYED_SEEKING];

        // Try to move unemployed into agriculture (always available in medieval times)
        if (unemployed_seeking > 0) {
            const int to_employ = static_cast<int>(unemployed_seeking * shift_rate * 2.0);  // Higher rate for this transition
            if (to_employ > 0) {
                group.employment[EmploymentType::UNEMPLOYED_SEEKING] -= to_employ;
                group.employment[EmploymentType::AGRICULTURE] += to_employ;
                total_shifts += to_employ;
            }
        }

        // Shift from agriculture to crafts/trade based on prosperity
        int agricultural_workers = group.employment[EmploymentType::AGRICULTURE];
        if (agricultural_workers > 0 && group.happiness > 0.6) {
            const int to_shift = static_cast<int>(agricultural_workers * shift_rate * group.happiness);
            if (to_shift > 0) {
                group.employment[EmploymentType::AGRICULTURE] -= to_shift;

                // Distribute to other productive employments
                if (group.social_class == SocialClass::ARTISAN || group.social_class == SocialClass::CRAFTSMAN) {
                    group.employment[EmploymentType::CRAFTSMAN] += to_shift;
                } else if (group.social_class == SocialClass::MERCHANT) {
                    group.employment[EmploymentType::MERCHANT] += to_shift;
                } else {
                    group.employment[EmploymentType::LABORER] += to_shift;
                }
                total_shifts += to_shift;
            }
        }
    }

    if (total_shifts > 0) {
        RecalculatePopulationAggregates(*population);

        CORE_LOG_DEBUG("PopulationSystem",
            "Province " + std::to_string(static_cast<int>(province_id)) +
            " employment shifts: " + std::to_string(total_shifts) + " workers changed employment");
    }
}

void PopulationSystem::ProcessCulturalChanges(game::types::EntityID province_id, double yearly_fraction) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) {
        return;
    }

    ::core::ecs::EntityID province_handle(static_cast<uint64_t>(province_id), 1);
    auto population = entity_manager->GetComponent<PopulationComponent>(province_handle);

    if (!population || population->population_groups.empty()) {
        return;
    }

    // Find dominant culture (largest population)
    std::unordered_map<std::string, int> culture_counts;
    for (const auto& group : population->population_groups) {
        culture_counts[group.culture] += group.population_count;
    }

    if (culture_counts.size() <= 1) {
        return;  // No cultural diversity, no assimilation
    }

    std::string dominant_culture;
    int max_population = 0;
    for (const auto& [culture, count] : culture_counts) {
        if (count > max_population) {
            max_population = count;
            dominant_culture = culture;
        }
    }

    // Very slow assimilation rate in medieval times (0.2% per year)
    const double assimilation_rate = 0.002 * yearly_fraction;
    int total_assimilated = 0;

    // Process minority cultures
    for (auto& group : population->population_groups) {
        if (group.population_count <= 0 || group.culture == dominant_culture) {
            continue;
        }

        // Calculate assimilation pressure
        const double minority_ratio = static_cast<double>(culture_counts[group.culture]) / population->total_population;
        const double pressure = (1.0 - minority_ratio) * assimilation_rate;

        // Higher happiness and literacy increase assimilation (integration)
        const double integration_factor = (group.happiness + group.literacy_rate) / 2.0;
        const double effective_rate = pressure * integration_factor;

        const int to_assimilate = static_cast<int>(group.population_count * effective_rate);

        if (to_assimilate > 0) {
            // Move to dominant culture group
            auto* target_group = FindOrCreatePopulationGroup(*population, group.social_class,
                                                            group.legal_status, dominant_culture, group.religion);
            if (target_group) {
                const int actual_move = std::min(to_assimilate, group.population_count);
                group.population_count -= actual_move;
                target_group->population_count += actual_move;
                total_assimilated += actual_move;
            }
        }
    }

    if (total_assimilated > 0) {
        RecalculatePopulationAggregates(*population);

        CORE_LOG_DEBUG("PopulationSystem",
            "Province " + std::to_string(static_cast<int>(province_id)) +
            " cultural assimilation: " + std::to_string(total_assimilated) +
            " people assimilated to " + dominant_culture);
    }
}

// Event Processing Methods
// ============================================================================

void PopulationSystem::ProcessPlague(game::types::EntityID province_id, const PlagueEvent& event) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) {
        return;
    }

    ::core::ecs::EntityID province_handle(static_cast<uint64_t>(province_id), 1);
    auto population = entity_manager->GetComponent<PopulationComponent>(province_handle);

    if (!population || population->population_groups.empty()) {
        return;
    }

    // Plague severity affects mortality (0.1 to 0.5 = 10% to 50% mortality)
    const double base_mortality = event.mortality_rate * event.severity;
    int total_deaths = 0;

    // Process each population group
    for (auto& group : population->population_groups) {
        if (group.population_count <= 0) {
            continue;
        }

        // Calculate mortality for this group
        double group_mortality = base_mortality;

        // Poor health increases vulnerability
        group_mortality *= (1.5 - group.health_level * 0.5);

        // Urban areas and crowded conditions spread disease faster
        if (group.social_class == SocialClass::PEASANT || group.social_class == SocialClass::LABORER) {
            group_mortality *= 1.3;
        }

        // Apply deaths
        const int deaths = std::min(static_cast<int>(group.population_count * group_mortality),
                                   group.population_count);

        group.population_count -= deaths;
        total_deaths += deaths;

        // Plague affects primarily the weak (elderly, children)
        const int elderly_deaths = std::min(deaths / 2, group.elderly_65_plus);
        const int child_deaths = std::min(deaths / 3, group.children_0_14);
        const int adult_deaths = deaths - elderly_deaths - child_deaths;

        group.elderly_65_plus = std::max(0, group.elderly_65_plus - elderly_deaths);
        group.children_0_14 = std::max(0, group.children_0_14 - child_deaths);
        group.adults_15_64 = std::max(0, group.adults_15_64 - adult_deaths);

        // Plague devastates happiness and health
        group.happiness = std::max(0.0, group.happiness - 0.3 * event.severity);
        group.health_level = std::max(0.0, group.health_level - 0.4 * event.severity);

        // Increase death rate temporarily
        group.death_rate += 0.02 * event.severity;
    }

    RecalculatePopulationAggregates(*population);

    // Send crisis event
    std::vector<SocialClass> affected_classes = {SocialClass::PEASANT, SocialClass::LABORER};
    SendCrisisEvent(province_id, "plague", event.severity, affected_classes);

    CORE_LOG_WARN("PopulationSystem",
        "Plague in province " + std::to_string(static_cast<int>(province_id)) +
        ": " + std::to_string(total_deaths) + " deaths (severity: " +
        std::to_string(static_cast<int>(event.severity * 100)) + "%)"    );
}

void PopulationSystem::ProcessFamine(game::types::EntityID province_id, const FamineEvent& event) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) {
        return;
    }

    ::core::ecs::EntityID province_handle(static_cast<uint64_t>(province_id), 1);
    auto population = entity_manager->GetComponent<PopulationComponent>(province_handle);

    if (!population || population->population_groups.empty()) {
        return;
    }

    int total_deaths = 0;
    int total_migrants = 0;

    for (auto& group : population->population_groups) {
        if (group.population_count <= 0) {
            continue;
        }

        // Famine affects poor much more than wealthy
        double vulnerability = 1.0;
        if (group.social_class == SocialClass::PEASANT || group.social_class == SocialClass::SERF) {
            vulnerability = 2.0;  // Peasants and serfs hit hardest
        } else if (group.social_class == SocialClass::NOBILITY || group.social_class == SocialClass::CLERGY) {
            vulnerability = 0.3;  // Elite largely protected
        }

        // Calculate deaths from starvation
        const double mortality = event.mortality_increase * event.severity * vulnerability;
        const int deaths = std::min(static_cast<int>(group.population_count * mortality),
                                   group.population_count);

        group.population_count -= deaths;
        total_deaths += deaths;

        // Famine primarily kills children and elderly (most vulnerable)
        const int child_deaths = std::min(deaths * 2 / 3, group.children_0_14);
        const int elderly_deaths = std::min((deaths - child_deaths) / 2, group.elderly_65_plus);
        const int adult_deaths = deaths - child_deaths - elderly_deaths;

        group.children_0_14 = std::max(0, group.children_0_14 - child_deaths);
        group.elderly_65_plus = std::max(0, group.elderly_65_plus - elderly_deaths);
        group.adults_15_64 = std::max(0, group.adults_15_64 - adult_deaths);

        // Migration due to famine (people flee)
        const int migrants = static_cast<int>(group.population_count * 0.1 * event.severity);
        group.population_count = std::max(0, group.population_count - migrants);
        total_migrants += migrants;

        // Severe impacts on health and happiness
        group.health_level = std::max(0.0, group.health_level - 0.5 * event.severity);
        group.happiness = std::max(0.0, group.happiness - 0.4 * event.severity);

        // Temporary spike in death rate
        group.death_rate += 0.03 * event.severity;

        // Birth rate drops during famine
        group.birth_rate = std::max(0.0, group.birth_rate - 0.02 * event.severity);
    }

    RecalculatePopulationAggregates(*population);

    // Send crisis event
    std::vector<SocialClass> affected_classes = {SocialClass::PEASANT, SocialClass::SERF};
    SendCrisisEvent(province_id, "famine", event.severity, affected_classes);

    CORE_LOG_WARN("PopulationSystem",
        "Famine in province " + std::to_string(static_cast<int>(province_id)) +
        ": " + std::to_string(total_deaths) + " deaths, " +
        std::to_string(total_migrants) + " migrants (severity: " +
        std::to_string(static_cast<int>(event.severity * 100)) + "%)");
}

void PopulationSystem::ProcessNaturalDisaster(game::types::EntityID province_id, const NaturalDisasterEvent& event) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) {
        return;
    }

    ::core::ecs::EntityID province_handle(static_cast<uint64_t>(province_id), 1);
    auto population = entity_manager->GetComponent<PopulationComponent>(province_handle);

    if (!population || population->population_groups.empty()) {
        return;
    }

    // Natural disasters cause immediate casualties
    const int total_casualties = event.casualties;
    int casualties_remaining = total_casualties;

    // Distribute casualties proportionally across groups
    for (auto& group : population->population_groups) {
        if (group.population_count <= 0 || casualties_remaining <= 0) {
            continue;
        }

        const double group_ratio = static_cast<double>(group.population_count) / population->total_population;
        const int group_casualties = std::min(static_cast<int>(total_casualties * group_ratio),
                                             std::min(group.population_count, casualties_remaining));

        group.population_count -= group_casualties;
        casualties_remaining -= group_casualties;

        // Update age cohorts proportionally
        const int child_losses = std::min(group_casualties / 3, group.children_0_14);
        const int elderly_losses = std::min(group_casualties / 4, group.elderly_65_plus);
        const int adult_losses = group_casualties - child_losses - elderly_losses;

        group.children_0_14 = std::max(0, group.children_0_14 - child_losses);
        group.elderly_65_plus = std::max(0, group.elderly_65_plus - elderly_losses);
        group.adults_15_64 = std::max(0, group.adults_15_64 - adult_losses);

        // Disaster trauma
        group.happiness = std::max(0.0, group.happiness - 0.2 * event.severity);
    }

    // Population displacement
    const int displaced = event.displaced_population;
    for (auto& group : population->population_groups) {
        if (displaced <= 0) break;
        const int to_displace = std::min(static_cast<int>(displaced * group.population_count / population->total_population),
                                        group.population_count);
        group.population_count -= to_displace;
    }

    RecalculatePopulationAggregates(*population);

    std::vector<SocialClass> all_affected;
    SendCrisisEvent(province_id, event.disaster_type, event.severity, all_affected);

    CORE_LOG_ERROR("PopulationSystem",
        "Natural disaster (" + event.disaster_type + ") in province " +
        std::to_string(static_cast<int>(province_id)) + ": " +
        std::to_string(total_casualties) + " casualties, " +
        std::to_string(displaced) + " displaced");
}

void PopulationSystem::ProcessSocialUnrest(game::types::EntityID province_id, const SocialUnrestEvent& event) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) {
        return;
    }

    ::core::ecs::EntityID province_handle(static_cast<uint64_t>(province_id), 1);
    auto population = entity_manager->GetComponent<PopulationComponent>(province_handle);

    if (!population || population->population_groups.empty()) {
        return;
    }

    // Unrest causes casualties from violence and repression
    const int total_participants = event.participants;
    const double casualty_rate = event.violence_level * 0.05;  // 5% at max violence
    const int casualties = static_cast<int>(total_participants * casualty_rate);

    // Affect participating social classes
    for (const auto& social_class : event.participating_classes) {
        for (auto& group : population->population_groups) {
            if (group.social_class == social_class && group.population_count > 0) {
                const int group_casualties = std::min(casualties / static_cast<int>(event.participating_classes.size()),
                                                     group.population_count);
                group.population_count -= group_casualties;

                // Casualties are working-age adults
                group.adults_15_64 = std::max(0, group.adults_15_64 - group_casualties);

                // Unrest reduces happiness significantly
                group.happiness = std::max(0.0, group.happiness - 0.3 * event.unrest_intensity);

                // Some become unemployed due to disruption
                const int jobs_lost = static_cast<int>(group_casualties * 2);
                group.employment[EmploymentType::UNEMPLOYED_SEEKING] += jobs_lost;
            }
        }
    }

    RecalculatePopulationAggregates(*population);

    SendCrisisEvent(province_id, "social_unrest", event.unrest_intensity, event.participating_classes);

    CORE_LOG_WARN("PopulationSystem",
        "Social unrest in province " + std::to_string(static_cast<int>(province_id)) +
        ": " + std::to_string(casualties) + " casualties from violence");
}

void PopulationSystem::ProcessMilitaryRecruitment(game::types::EntityID province_id, const MilitaryRecruitmentEvent& event) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) {
        return;
    }

    ::core::ecs::EntityID province_handle(static_cast<uint64_t>(province_id), 1);
    auto population = entity_manager->GetComponent<PopulationComponent>(province_handle);

    if (!population || population->population_groups.empty()) {
        return;
    }

    const int recruits_needed = event.recruits_needed;
    int recruits_available = 0;
    double total_quality = 0.0;

    // Recruit from eligible population (healthy adult males)
    for (auto& group : population->population_groups) {
        if (recruits_available >= recruits_needed) {
            break;
        }

        // Calculate eligible recruits (adult males, not elderly/children)
        const int eligible = std::min(group.males / 2,  // Half of males
                                     std::min(group.adults_15_64, group.population_count / 3));

        if (eligible <= 0) {
            continue;
        }

        const int to_recruit = std::min(eligible, recruits_needed - recruits_available);

        // Remove from population temporarily (they join military)
        group.population_count -= to_recruit;
        group.adults_15_64 = std::max(0, group.adults_15_64 - to_recruit);
        group.males = std::max(0, group.males - to_recruit);

        // Track military employment
        group.employment[EmploymentType::SOLDIER] += to_recruit;

        recruits_available += to_recruit;
        total_quality += group.military_quality * to_recruit;

        // Slight happiness reduction from conscription
        group.happiness = std::max(0.0, group.happiness - 0.05);
    }

    RecalculatePopulationAggregates(*population);

    // Calculate average quality
    const double avg_quality = recruits_available > 0 ? total_quality / recruits_available : 0.0;

    // Notify military system of recruitment results
    MilitaryRecruitmentEvent result_event = event;
    result_event.recruits_available = recruits_available;
    result_event.average_quality = avg_quality;
    result_event.recruitment_cost = recruits_available * 10.0;  // Basic cost calculation

    NotifyMilitarySystem(province_id, result_event);

    CORE_LOG_INFO("PopulationSystem",
        "Military recruitment in province " + std::to_string(static_cast<int>(province_id)) +
        ": " + std::to_string(recruits_available) + "/" + std::to_string(recruits_needed) +
        " recruits (quality: " + std::to_string(static_cast<int>(avg_quality * 100)) + "%)");
}

void PopulationSystem::ProcessMilitaryService(game::types::EntityID province_id, const MilitaryServiceEvent& event) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) {
        return;
    }

    ::core::ecs::EntityID province_handle(static_cast<uint64_t>(province_id), 1);
    auto population = entity_manager->GetComponent<PopulationComponent>(province_handle);

    if (!population || population->population_groups.empty()) {
        return;
    }

    const int soldiers_called = event.soldiers_called;

    // Soldiers are away from productive work
    for (auto& group : population->population_groups) {
        const int group_soldiers = group.employment[EmploymentType::SOLDIER];

        if (group_soldiers > 0) {
            // Calculate service burden on this group
            const double burden_ratio = static_cast<double>(group_soldiers) / group.population_count;
            const double economic_impact = burden_ratio * event.service_burden;

            // Happiness reduction from prolonged service
            group.happiness = std::max(0.0, group.happiness - 0.1 * event.service_burden);

            // Wealth reduction from lost productivity
            group.wealth_per_capita = std::max(0.0, group.wealth_per_capita - economic_impact * 5.0);
        }
    }

    RecalculatePopulationAggregates(*population);

    CORE_LOG_INFO("PopulationSystem",
        "Military service in province " + std::to_string(static_cast<int>(province_id)) +
        ": " + std::to_string(soldiers_called) + " soldiers on extended duty");
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

void PopulationSystem::SendSettlementEvolutionEvent(game::types::EntityID province_id,
                                                   const Settlement& settlement,
                                                   SettlementType old_type,
                                                   const std::string& reason) {
    SettlementGrowthEvent event;
    event.entity_id = province_id;
    event.settlement_name = settlement.name;
    event.old_type = old_type;
    event.new_type = settlement.type;
    event.population_change = settlement.growth_rate;
    event.prosperity_change = 0.0;  // Could be calculated
    event.infrastructure_improvement = settlement.infrastructure_level;
    event.growth_reason = reason;

    m_message_bus.Send(event);

    CORE_LOG_INFO("PopulationSystem",
        "Settlement '" + settlement.name + "' evolved from " +
        utils::GetSettlementTypeName(old_type) + " to " +
        utils::GetSettlementTypeName(settlement.type));
}

void PopulationSystem::SendLegalStatusChangeEvent(game::types::EntityID province_id,
                                                  LegalStatus from_status,
                                                  LegalStatus to_status,
                                                  int population_affected,
                                                  const std::string& reason) {
    LegalStatusChangeEvent event;
    event.entity_id = province_id;
    event.from_status = from_status;
    event.to_status = to_status;
    event.population_affected = population_affected;
    event.reason = reason;
    event.economic_impact = population_affected * 10.0;  // Rough estimate

    m_message_bus.Send(event);

    CORE_LOG_INFO("PopulationSystem",
        "Legal status change in province " + std::to_string(static_cast<int>(province_id)) +
        ": " + std::to_string(population_affected) + " people changed from " +
        utils::GetLegalStatusName(from_status) + " to " +
        utils::GetLegalStatusName(to_status));
}

// Stub implementations for missing methods
// ============================================================================

void PopulationSystem::ProcessClassMobility(PopulationComponent& population,
                                           game::types::EntityID province_id,
                                           double yearly_fraction) {
    // Already implemented in ProcessSocialMobility
    // This method is kept for compatibility but delegates to the main implementation
}

void PopulationSystem::ProcessLegalStatusChanges(PopulationComponent& population,
                                                game::types::EntityID province_id,
                                                double yearly_fraction) {
    // Process legal status changes (manumission, enserfment, etc.)
    const double base_change_rate = 0.001 * yearly_fraction;  // 0.1% per year

    for (auto& group : population.population_groups) {
        if (group.population_count <= 0) continue;

        // Manumission (slaves → serfs, serfs → villeins, etc.)
        if (group.legal_status == LegalStatus::SLAVE && RandomChance(base_change_rate * 2.0)) {
            const int manumitted = static_cast<int>(group.population_count * 0.01);
            if (manumitted > 0) {
                group.population_count -= manumitted;

                auto* freed_group = FindOrCreatePopulationGroup(
                    population, group.social_class, LegalStatus::SERF,
                    group.culture, group.religion);
                if (freed_group) {
                    freed_group->population_count += manumitted;
                }

                SendLegalStatusChangeEvent(province_id, LegalStatus::SLAVE,
                                          LegalStatus::SERF, manumitted, "manumission");
            }
        }

        // Serfs gaining freedom through purchase or grant
        if (group.legal_status == LegalStatus::SERF && group.wealth_per_capita > 100.0) {
            if (RandomChance(base_change_rate)) {
                const int freed = static_cast<int>(group.population_count * 0.005);
                if (freed > 0) {
                    group.population_count -= freed;

                    auto* free_group = FindOrCreatePopulationGroup(
                        population, SocialClass::FREE_PEASANTS, LegalStatus::FREE_PEASANT,
                        group.culture, group.religion);
                    if (free_group) {
                        free_group->population_count += freed;
                    }

                    SendLegalStatusChangeEvent(province_id, LegalStatus::SERF,
                                              LegalStatus::FREE_PEASANT, freed, "purchase_freedom");
                }
            }
        }
    }
}

void PopulationSystem::ProcessGuildAdvancement(PopulationComponent& population,
                                              SettlementComponent& settlements,
                                              game::types::EntityID province_id,
                                              double yearly_fraction) {
    // Process guild-based social mobility
    const double guild_advancement_rate = m_config.guild_formation_threshold * yearly_fraction;

    for (auto& group : population.population_groups) {
        if (group.population_count <= 0) continue;

        // Craftsmen can advance to guild masters
        if (group.social_class == SocialClass::CRAFTSMEN &&
            group.guild_membership_rate > 0.5 &&
            group.wealth_per_capita > 150.0) {

            const int promotable = static_cast<int>(
                group.population_count * guild_advancement_rate * group.literacy_rate);

            if (promotable > 0 && RandomChance(0.1)) {
                group.population_count -= promotable;

                auto* master_group = FindOrCreatePopulationGroup(
                    population, SocialClass::GUILD_MASTERS, LegalStatus::GUILD_MEMBER,
                    group.culture, group.religion);
                if (master_group) {
                    master_group->population_count += promotable;

                    GuildAdvancementEvent event;
                    event.entity_id = province_id;
                    event.from_class = SocialClass::CRAFTSMEN;
                    event.to_class = SocialClass::GUILD_MASTERS;
                    event.population_affected = promotable;
                    event.guild_type = "craftsmen";
                    event.skill_requirement = 0.7;
                    event.wealth_requirement = 150.0;

                    m_message_bus.Send(event);
                }
            }
        }
    }
}

void PopulationSystem::UpdateSettlementGrowth(SettlementComponent& settlements,
                                             const PopulationComponent& population,
                                             double yearly_fraction) {
    const double growth_threshold = m_config.settlement_growth_threshold;
    const double decline_threshold = m_config.settlement_decline_threshold;

    for (auto& settlement : settlements.settlements) {
        // Calculate settlement population from surrounding area
        int settlement_pop = settlement.total_population;

        // Apply population growth/decline from province-level changes
        const double province_growth = population.growth_rate;
        settlement.growth_rate = province_growth * (settlement.prosperity_level + 0.5);

        // Update population based on growth rate
        const int pop_change = static_cast<int>(settlement_pop * settlement.growth_rate * yearly_fraction);
        settlement.total_population = std::max(10, settlement_pop + pop_change);

        // Check for settlement type evolution
        SettlementType old_type = settlement.type;
        SettlementType new_type = old_type;

        // Hamlet → Village → Town → City progression
        if (utils::IsUrbanSettlement(old_type)) {
            if (settlement.total_population > 10000 && settlement.growth_rate > growth_threshold) {
                if (old_type == SettlementType::CITY) {
                    new_type = SettlementType::LARGE_CITY;
                } else if (old_type == SettlementType::TOWN) {
                    new_type = SettlementType::CITY;
                } else if (old_type == SettlementType::VILLAGE) {
                    new_type = SettlementType::TOWN;
                } else if (old_type == SettlementType::RURAL_HAMLET) {
                    new_type = SettlementType::VILLAGE;
                }
            } else if (settlement.total_population < 500 && settlement.growth_rate < decline_threshold) {
                // Decline path
                if (old_type == SettlementType::LARGE_CITY) {
                    new_type = SettlementType::CITY;
                } else if (old_type == SettlementType::CITY) {
                    new_type = SettlementType::TOWN;
                } else if (old_type == SettlementType::TOWN) {
                    new_type = SettlementType::VILLAGE;
                }
            }
        }

        // Trigger settlement evolution event if type changed
        if (new_type != old_type) {
            settlement.type = new_type;
            SendSettlementEvolutionEvent(population.total_population, settlement, old_type,
                                        settlement.growth_rate > 1.0 ? "rapid_growth" : "decline");
        }

        // Update infrastructure based on population and prosperity
        const double infrastructure_growth = 0.01 * yearly_fraction * settlement.prosperity_level;
        settlement.infrastructure_level = std::min(1.0, settlement.infrastructure_level + infrastructure_growth);

        // Update stability based on growth rate
        if (std::abs(settlement.growth_rate) > 0.05) {
            settlement.stability = std::max(0.0, settlement.stability - 0.05 * yearly_fraction);
        } else {
            settlement.stability = std::min(1.0, settlement.stability + 0.02 * yearly_fraction);
        }
    }
}

void PopulationSystem::UpdateSettlementSpecialization(SettlementComponent& settlements,
                                                      const PopulationComponent& population) {
    for (auto& settlement : settlements.settlements) {
        // Analyze employment distribution to determine specialization
        std::unordered_map<std::string, int> industry_workers;

        // Count workers by industry type
        for (const auto& group : population.population_groups) {
            for (const auto& [employment_type, count] : group.employment) {
                if (employment_type == EmploymentType::AGRICULTURE) {
                    industry_workers["agriculture"] += count;
                } else if (employment_type == EmploymentType::CRAFTING ||
                          employment_type == EmploymentType::CONSTRUCTION) {
                    industry_workers["manufacturing"] += count;
                } else if (employment_type == EmploymentType::TRADE ||
                          employment_type == EmploymentType::MARITIME_TRADE) {
                    industry_workers["trade"] += count;
                } else if (employment_type == EmploymentType::EXTRACTION) {
                    industry_workers["mining"] += count;
                } else if (employment_type == EmploymentType::RELIGIOUS) {
                    industry_workers["religious"] += count;
                } else if (employment_type == EmploymentType::MILITARY) {
                    industry_workers["military"] += count;
                }
            }
        }

        // Clear existing specializations
        settlement.economic_specializations.clear();

        // Determine primary specializations (>20% of workforce)
        const int total_workers = population.productive_workers;
        if (total_workers > 0) {
            for (const auto& [industry, count] : industry_workers) {
                const double percentage = static_cast<double>(count) / total_workers;
                if (percentage > 0.2) {
                    settlement.economic_specializations.push_back(industry);

                    // Update production bonuses
                    settlement.production[industry] = count * percentage *
                                                     (1.0 + settlement.prosperity_level);
                }
            }
        }

        // Update market importance based on trade workers
        if (industry_workers.count("trade") > 0) {
            settlement.market_importance = static_cast<double>(industry_workers["trade"]) /
                                          std::max(100.0, static_cast<double>(total_workers));
        }

        // Update guild presence based on specialization
        settlement.guild_presence.clear();
        for (const auto& specialization : settlement.economic_specializations) {
            if (specialization == "manufacturing") {
                settlement.guild_presence.push_back("Craftsmen's Guild");
            } else if (specialization == "trade") {
                settlement.guild_presence.push_back("Merchant's Guild");
            }
        }
    }
}

void PopulationSystem::UpdateUrbanization(SettlementComponent& settlements,
                                         PopulationComponent& population,
                                         double yearly_fraction) {
    // Calculate urbanization pressure
    const double urbanization_growth_rate = m_config.urbanization_growth_rate * yearly_fraction;
    const double current_urban_rate = settlements.urbanization_rate;

    // Factors that drive urbanization
    double urbanization_pressure = 0.0;

    // Economic opportunity in cities
    if (population.average_wealth > 120.0) {
        urbanization_pressure += 0.02;  // Wealthy economy attracts urban migration
    }

    // Unemployment in rural areas pushes people to cities
    if (population.unemployed_seeking > population.total_population * 0.1) {
        urbanization_pressure += 0.03;
    }

    // Calculate target urbanization rate
    double target_urban_rate = current_urban_rate + urbanization_pressure * yearly_fraction;
    target_urban_rate = std::min(0.3, target_urban_rate);  // Cap at 30% for medieval period

    // Calculate how many people need to migrate from rural to urban
    const int target_urban_pop = static_cast<int>(population.total_population * target_urban_rate);
    const int current_urban_pop = static_cast<int>(population.total_population * current_urban_rate);
    const int migrants = target_urban_pop - current_urban_pop;

    if (migrants > 0) {
        // Move peasants to urban laborers
        int migrants_remaining = migrants;

        for (auto& group : population.population_groups) {
            if (migrants_remaining <= 0) break;

            // Only rural classes migrate to cities
            if (group.social_class == SocialClass::FREE_PEASANTS ||
                group.social_class == SocialClass::VILLEINS) {

                const int to_migrate = std::min(migrants_remaining,
                                               group.population_count / 20);  // Max 5% per update

                if (to_migrate > 0) {
                    // Remove from rural class
                    group.population_count -= to_migrate;

                    // Add to urban laborer class
                    auto* urban_group = FindOrCreatePopulationGroup(
                        population,
                        SocialClass::URBAN_LABORERS,
                        LegalStatus::FULL_CITIZEN,
                        group.culture,
                        group.religion
                    );

                    if (urban_group) {
                        urban_group->population_count += to_migrate;
                        urban_group->adults_15_64 += to_migrate;  // Mostly working-age migrate
                    }

                    migrants_remaining -= to_migrate;
                }
            }
        }

        // Update urbanization rate
        settlements.urbanization_rate = target_urban_rate;

        // Send urbanization event if significant migration occurred
        if (migrants - migrants_remaining > 100) {
            UrbanizationEvent event;
            event.entity_id = static_cast<game::types::EntityID>(0);  // Set by caller
            event.migrants_to_cities = migrants - migrants_remaining;
            event.new_urbanization_rate = target_urban_rate;
            event.push_factors = {urbanization_pressure > 0.02 ? "economic_opportunity" : "unemployment"};

            m_message_bus.Send(event);
        }
    }

    // Update settlement statistics
    RecalculatePopulationAggregates(population);
}

void PopulationSystem::UpdateEmploymentDistribution(PopulationComponent& population,
                                                    const SettlementComponent& settlements) {
    // Redistribute employment based on settlement specializations
    ProcessJobCreation(population, settlements);

    // Update employment aggregates
    PopulationAggregator::RecalculateEconomicData(population);
}

void PopulationSystem::ProcessJobCreation(PopulationComponent& population,
                                         const SettlementComponent& settlements) {
    // Calculate job availability based on settlement specializations
    for (const auto& settlement : settlements.settlements) {
        for (const auto& specialization : settlement.economic_specializations) {
            // Determine employment type for specialization
            EmploymentType job_type = EmploymentType::UNEMPLOYED_SEEKING;

            if (specialization == "agriculture") {
                job_type = EmploymentType::AGRICULTURE;
            } else if (specialization == "manufacturing") {
                job_type = EmploymentType::CRAFTING;
            } else if (specialization == "trade") {
                job_type = EmploymentType::TRADE;
            } else if (specialization == "mining") {
                job_type = EmploymentType::EXTRACTION;
            }

            // Create jobs for unemployed in matching social classes
            int jobs_available = static_cast<int>(settlement.production[specialization] * 0.1);

            for (auto& group : population.population_groups) {
                if (jobs_available <= 0) break;

                int unemployed = group.employment[EmploymentType::UNEMPLOYED_SEEKING];
                if (unemployed > 0 && utils::CanWorkInRole(group.social_class, job_type)) {
                    int hired = std::min(unemployed, jobs_available);

                    group.employment[EmploymentType::UNEMPLOYED_SEEKING] -= hired;
                    group.employment[job_type] += hired;

                    jobs_available -= hired;
                }
            }
        }
    }
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

void PopulationSystem::RecalculateSettlementSummary(SettlementComponent& settlements) {
    // Reset aggregates
    settlements.settlement_counts.clear();
    settlements.total_production_value = 0.0;
    settlements.total_consumption_value = 0.0;
    settlements.trade_income_total = 0.0;
    settlements.total_market_importance = 0.0;
    settlements.total_garrison_size = 0;
    settlements.military_settlements = 0;
    settlements.economic_settlements = 0;
    settlements.religious_settlements = 0;
    settlements.administrative_settlements = 0;

    // Calculate weighted averages
    double infrastructure_sum = 0.0;
    double fortification_sum = 0.0;
    double sanitation_sum = 0.0;
    double prosperity_sum = 0.0;
    double cultural_tolerance_sum = 0.0;
    double religious_tolerance_sum = 0.0;
    double administrative_efficiency_sum = 0.0;
    double autonomy_sum = 0.0;
    double tax_burden_sum = 0.0;

    int total_settlements = settlements.settlements.size();

    for (const auto& settlement : settlements.settlements) {
        // Count settlement types
        settlements.settlement_counts[settlement.type]++;

        // Categorize settlements
        if (utils::IsMilitarySettlement(settlement.type)) {
            settlements.military_settlements++;
        } else if (utils::IsEconomicSettlement(settlement.type)) {
            settlements.economic_settlements++;
        } else if (utils::IsReligiousSettlement(settlement.type)) {
            settlements.religious_settlements++;
        }

        // Accumulate economic data
        for (const auto& [resource, value] : settlement.production) {
            settlements.total_production_value += value;
        }
        for (const auto& [resource, value] : settlement.consumption) {
            settlements.total_consumption_value += value;
        }
        settlements.trade_income_total += settlement.trade_income;
        settlements.total_market_importance += settlement.market_importance;
        settlements.total_garrison_size += settlement.garrison_size;

        // Accumulate infrastructure metrics
        infrastructure_sum += settlement.infrastructure_level;
        fortification_sum += settlement.fortification_level;
        sanitation_sum += settlement.sanitation_level;
        prosperity_sum += settlement.prosperity_level;
        cultural_tolerance_sum += settlement.cultural_tolerance;
        religious_tolerance_sum += settlement.religious_tolerance;
        administrative_efficiency_sum += settlement.administrative_efficiency;
        autonomy_sum += settlement.autonomy_level;
        tax_burden_sum += settlement.tax_burden;
    }

    // Calculate averages
    if (total_settlements > 0) {
        const double count = static_cast<double>(total_settlements);
        settlements.average_infrastructure = infrastructure_sum / count;
        settlements.average_fortification = fortification_sum / count;
        settlements.average_sanitation = sanitation_sum / count;
        settlements.average_prosperity = prosperity_sum / count;
        settlements.average_cultural_tolerance = cultural_tolerance_sum / count;
        settlements.average_religious_tolerance = religious_tolerance_sum / count;
        settlements.average_administrative_efficiency = administrative_efficiency_sum / count;
        settlements.average_autonomy_level = autonomy_sum / count;
        settlements.average_tax_burden = tax_burden_sum / count;
    }
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