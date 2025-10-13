// ============================================================================
// PopulationAggregator.cpp - High-Performance Population Statistics Implementation
// Created: October 12, 2025 - Extensive Refactoring
// Location: src/game/population/PopulationAggregator.cpp
// ============================================================================

#include "game/population/PopulationAggregator.h"
#include "game/population/PopulationComponents.h"
#include "game/population/PopulationTypes.h"
#include "core/logging/Logger.h"
#include <algorithm>

namespace game::population {

void PopulationAggregator::RecalculateAllAggregates(PopulationComponent& population, bool update_timestamp) {
    ResetAggregates(population);
    auto context = ProcessPopulationGroups(population);
    ApplyAggregationResults(population, context);
    
    if (update_timestamp) {
        population.last_update = std::chrono::steady_clock::now();
    }
}

void PopulationAggregator::RecalculateBasicTotals(PopulationComponent& population) {
    // Reset only basic counters
    population.total_population = 0;
    population.total_children = 0;
    population.total_adults = 0;
    population.total_elderly = 0;
    population.total_males = 0;
    population.total_females = 0;
    
    // Fast aggregation of basic demographics
    for (const auto& group : population.population_groups) {
        population.total_population += group.population_count;
        population.total_children += group.children_0_14;
        population.total_adults += group.adults_15_64;
        population.total_elderly += group.elderly_65_plus;
        population.total_males += group.males;
        population.total_females += group.females;
    }
}

void PopulationAggregator::RecalculateWeightedAverages(PopulationComponent& population) {
    if (population.total_population == 0) {
        RecalculateBasicTotals(population);
        if (population.total_population == 0) return;
    }
    
    double happiness_sum = 0.0;
    double literacy_sum = 0.0;
    double wealth_sum = 0.0;
    double health_sum = 0.0;
    double birth_rate_sum = 0.0;
    double death_rate_sum = 0.0;
    double military_quality_sum = 0.0;
    
    // Weight by population size for accurate averages
    for (const auto& group : population.population_groups) {
        const double weight = static_cast<double>(group.population_count);
        happiness_sum += group.happiness * weight;
        literacy_sum += group.literacy_rate * weight;
        wealth_sum += group.wealth_per_capita * weight;
        health_sum += group.health_level * weight;
        birth_rate_sum += group.birth_rate * weight;
        death_rate_sum += group.death_rate * weight;
        military_quality_sum += group.military_quality * weight;
    }
    
    const double total_weight = static_cast<double>(population.total_population);
    population.average_happiness = happiness_sum / total_weight;
    population.average_literacy = literacy_sum / total_weight;
    population.average_wealth = wealth_sum / total_weight;
    population.average_health = health_sum / total_weight;
    population.birth_rate_average = birth_rate_sum / total_weight;
    population.death_rate_average = death_rate_sum / total_weight;
    population.average_military_quality = military_quality_sum / total_weight;
    
    // Calculate derived metrics
    population.growth_rate = population.birth_rate_average - population.death_rate_average;
    population.population_density = total_weight / 1000.0; // Assume 1000 km² area
}

void PopulationAggregator::RecalculateEconomicData(PopulationComponent& population) {
    population.total_employment.clear();
    population.productive_workers = 0;
    population.unemployed_seeking = 0;
    
    for (const auto& group : population.population_groups) {
        for (const auto& [employment_type, count] : group.employment) {
            population.total_employment[employment_type] += count;
            
            if (employment_type == EmploymentType::UNEMPLOYED_SEEKING) {
                population.unemployed_seeking += count;
            } else if (utils::IsProductiveEmployment(employment_type)) {
                population.productive_workers += count;
            }
        }
    }
    
    // Calculate employment rate
    population.overall_employment_rate = population.total_population > 0 ?
        static_cast<double>(population.productive_workers) / population.total_population : 0.0;
}

void PopulationAggregator::RecalculateMilitaryData(PopulationComponent& population) {
    population.total_military_eligible = 0;
    population.total_military_service_obligation = 0;
    
    for (const auto& group : population.population_groups) {
        population.total_military_eligible += group.military_eligible;
        population.total_military_service_obligation += group.military_service_obligation;
    }
}

bool PopulationAggregator::ValidateDataConsistency(const PopulationComponent& population) {
    bool is_consistent = true;
    
    // Check basic demographic consistency
    const int calculated_total = population.total_children + population.total_adults + population.total_elderly;
    if (std::abs(calculated_total - population.total_population) > 1) {
        ::core::logging::LogWarning("PopulationAggregator", 
            "Age group totals don't match total population: " + 
            std::to_string(calculated_total) + " vs " + std::to_string(population.total_population));
        is_consistent = false;
    }
    
    // Check gender consistency
    const int calculated_gender_total = population.total_males + population.total_females;
    if (std::abs(calculated_gender_total - population.total_population) > 1) {
        ::core::logging::LogWarning("PopulationAggregator", 
            "Gender totals don't match total population: " + 
            std::to_string(calculated_gender_total) + " vs " + std::to_string(population.total_population));
        is_consistent = false;
    }
    
    // Check for impossible values
    if (population.average_happiness < 0.0 || population.average_happiness > 1.0) {
        ::core::logging::LogWarning("PopulationAggregator", 
            "Invalid happiness value: " + std::to_string(population.average_happiness));
        is_consistent = false;
    }
    
    if (population.average_literacy < 0.0 || population.average_literacy > 1.0) {
        ::core::logging::LogWarning("PopulationAggregator", 
            "Invalid literacy value: " + std::to_string(population.average_literacy));
        is_consistent = false;
    }
    
    return is_consistent;
}

void PopulationAggregator::ResetAggregates(PopulationComponent& population) {
    // Reset all counters
    population.total_population = 0;
    population.total_children = 0;
    population.total_adults = 0;
    population.total_elderly = 0;
    population.total_males = 0;
    population.total_females = 0;
    population.total_military_eligible = 0;
    population.productive_workers = 0;
    population.unemployed_seeking = 0;
    
    // Clear distributions
    population.culture_distribution.clear();
    population.religion_distribution.clear();
    population.class_distribution.clear();
    population.legal_status_distribution.clear();
    population.total_employment.clear();
}

PopulationAggregator::AggregationContext PopulationAggregator::ProcessPopulationGroups(const PopulationComponent& population) {
    AggregationContext context;
    
    for (const auto& group : population.population_groups) {
        const int group_pop = group.population_count;
        const double weight = static_cast<double>(group_pop);
        
        // Basic demographics
        context.total_population += group_pop;
        
        // Weighted sums for averages
        context.happiness_sum += group.happiness * weight;
        context.literacy_sum += group.literacy_rate * weight;
        context.wealth_sum += group.wealth_per_capita * weight;
        context.health_sum += group.health_level * weight;
        context.birth_rate_sum += group.birth_rate * weight;
        context.death_rate_sum += group.death_rate * weight;
        context.military_quality_sum += group.military_quality * weight;
        
        // Military and employment
        context.military_eligible += group.military_eligible;
        
        // Count productive employment
        for (const auto& [employment_type, count] : group.employment) {
            if (employment_type == EmploymentType::UNEMPLOYED_SEEKING) {
                context.unemployed_seeking += count;
            } else if (utils::IsProductiveEmployment(employment_type)) {
                context.productive_workers += count;
            }
        }
    }
    
    return context;
}

void PopulationAggregator::ApplyAggregationResults(PopulationComponent& population, const AggregationContext& context) {
    // Apply basic totals
    population.total_population = context.total_population;
    population.productive_workers = context.productive_workers;
    population.unemployed_seeking = context.unemployed_seeking;
    population.total_military_eligible = context.military_eligible;
    
    // Calculate averages if population exists
    if (context.total_population > 0) {
        const double total_weight = static_cast<double>(context.total_population);
        population.average_happiness = context.happiness_sum / total_weight;
        population.average_literacy = context.literacy_sum / total_weight;
        population.average_wealth = context.wealth_sum / total_weight;
        population.average_health = context.health_sum / total_weight;
        population.birth_rate_average = context.birth_rate_sum / total_weight;
        population.death_rate_average = context.death_rate_sum / total_weight;
        population.average_military_quality = context.military_quality_sum / total_weight;
        
        // Derived calculations
        population.growth_rate = population.birth_rate_average - population.death_rate_average;
        population.population_density = total_weight / 1000.0;
        population.overall_employment_rate = static_cast<double>(context.productive_workers) / total_weight;
    } else {
        // Reset to defaults if no population
        population.average_happiness = 0.5;
        population.average_literacy = 0.1;
        population.average_wealth = 100.0;
        population.average_health = 0.7;
        population.birth_rate_average = 0.035;
        population.death_rate_average = 0.030;
        population.average_military_quality = 0.5;
        population.growth_rate = 0.005;
        population.population_density = 0.0;
        population.overall_employment_rate = 0.0;
    }
    
    // Recalculate detailed distributions
    for (const auto& group : population.population_groups) {
        const int group_pop = group.population_count;
        
        // Update demographics
        population.total_children += group.children_0_14;
        population.total_adults += group.adults_15_64;
        population.total_elderly += group.elderly_65_plus;
        population.total_males += group.males;
        population.total_females += group.females;
        
        // Update distributions
        population.culture_distribution[group.culture] += group_pop;
        population.religion_distribution[group.religion] += group_pop;
        population.class_distribution[group.social_class] += group_pop;
        population.legal_status_distribution[group.legal_status] += group_pop;
        
        // Update employment
        for (const auto& [employment_type, count] : group.employment) {
            population.total_employment[employment_type] += count;
        }
    }
}

} // namespace game::population