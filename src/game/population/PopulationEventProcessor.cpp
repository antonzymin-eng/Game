// ============================================================================
// PopulationEventProcessor.cpp - Population Event Processing Implementation
// Created: December 19, 2024 at 12:05 PM
// Location: src/game/population/PopulationEventProcessor.cpp
// ============================================================================

#include "game/population/PopulationEvents.h"
#include "core/logging/Logger.h"
#include "core/types/game_types.h"
#include "utils/RandomGenerator.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace game::population {

    // ============================================================================
    // PopulationEventProcessor Implementation
    // ============================================================================

    PopulationEventProcessor::PopulationEventProcessor() 
        : m_processor_start_time(std::chrono::steady_clock::now()) {
        
        CORE_LOG_DEBUG("PopulationEventProcessor", "Population Event Processor initialized");
    }

    // ============================================================================
    // Core Event Processing Methods
    // ============================================================================

    void PopulationEventProcessor::ProcessEvent(const PopulationUpdateEvent& event) {
        LogEvent("PopulationUpdate", event.entity_id, 
            "Population: " + std::to_string(event.total_population) + 
            ", Growth: " + std::to_string(event.population_growth_rate * 100) + "%");

        UpdateGameState(event.entity_id, "population_updated");
        
        // Check for population thresholds that might trigger other events
        if (event.total_population > 10000) {
            TriggerConsequentialEvents(event.entity_id, "large_population");
        } else if (event.total_population < 500) {
            TriggerConsequentialEvents(event.entity_id, "population_decline");
        }

        // Track rapid growth or decline
        if (std::abs(event.population_growth_rate) > 0.05) { // >5% change
            TriggerConsequentialEvents(event.entity_id, "demographic_shift");
        }

        m_event_processing_counts["PopulationUpdate"]++;
        m_last_processed[event.entity_id] = std::chrono::steady_clock::now();
    }

    void PopulationEventProcessor::ProcessEvent(const SocialMobilityEvent& event) {
        LogEvent("SocialMobility", event.entity_id,
            utils::GetSocialClassName(event.from_class) + " -> " + 
            utils::GetSocialClassName(event.to_class) + 
            " (" + std::to_string(event.population_affected) + " people, " + event.reason + ")");

        UpdateGameState(event.entity_id, "social_structure_changed");

        // Significant upward mobility might indicate economic boom
        if (event.upward_mobility && event.population_affected > 100) {
            TriggerConsequentialEvents(event.entity_id, "economic_opportunity");
        }

        // Mass downward mobility might indicate crisis
        if (!event.upward_mobility && event.population_affected > 200) {
            TriggerConsequentialEvents(event.entity_id, "social_crisis");
        }

        m_event_processing_counts["SocialMobility"]++;
        m_last_processed[event.entity_id] = std::chrono::steady_clock::now();
    }

    void PopulationEventProcessor::ProcessEvent(const LegalStatusChangeEvent& event) {
        LogEvent("LegalStatusChange", event.entity_id,
            utils::GetLegalStatusName(event.from_status) + " -> " + 
            utils::GetLegalStatusName(event.to_status) + 
            " (" + std::to_string(event.population_affected) + " people, " + event.reason + ")");

        UpdateGameState(event.entity_id, "legal_system_changed");

        // Mass emancipations or enslavements are significant events
        if (event.population_affected > 50) {
            if (event.to_status == LegalStatus::FREE_PEASANT || 
                event.to_status == LegalStatus::FULL_CITIZEN) {
                TriggerConsequentialEvents(event.entity_id, "liberation_movement");
            } else if (event.to_status == LegalStatus::SERF || 
                      event.to_status == LegalStatus::SLAVE) {
                TriggerConsequentialEvents(event.entity_id, "oppression_increase");
            }
        }

        m_event_processing_counts["LegalStatusChange"]++;
        m_last_processed[event.entity_id] = std::chrono::steady_clock::now();
    }

    void PopulationEventProcessor::ProcessEvent(const EmploymentShiftEvent& event) {
        LogEvent("EmploymentShift", event.entity_id,
            utils::GetEmploymentName(event.from_employment) + " -> " + 
            utils::GetEmploymentName(event.to_employment) + 
            " (" + std::to_string(event.workers_affected) + " workers, " + event.reason + ")");

        UpdateGameState(event.entity_id, "economic_structure_changed");

        // Large employment shifts indicate economic transformation
        if (event.workers_affected > 100) {
            if (event.to_employment == EmploymentType::UNEMPLOYED_SEEKING) {
                TriggerConsequentialEvents(event.entity_id, "unemployment_crisis");
            } else if (event.from_employment == EmploymentType::AGRICULTURE && 
                      utils::IsProductiveEmployment(event.to_employment)) {
                TriggerConsequentialEvents(event.entity_id, "economic_modernization");
            }
        }

        m_event_processing_counts["EmploymentShift"]++;
        m_last_processed[event.entity_id] = std::chrono::steady_clock::now();
    }

    void PopulationEventProcessor::ProcessEvent(const SettlementUpdateEvent& event) {
        LogEvent("SettlementUpdate", event.entity_id,
            "Settlements: " + std::to_string(event.total_settlements) + 
            ", Urbanization: " + std::to_string(event.urbanization_rate * 100) + "%");

        UpdateGameState(event.entity_id, "settlement_structure_updated");

        // High urbanization indicates economic development
        if (event.urbanization_rate > 0.3) {
            TriggerConsequentialEvents(event.entity_id, "urban_development");
        }

        // Large numbers of military settlements indicate fortification
        if (event.military_settlements > 3) {
            TriggerConsequentialEvents(event.entity_id, "military_buildup");
        }

        m_event_processing_counts["SettlementUpdate"]++;
        m_last_processed[event.entity_id] = std::chrono::steady_clock::now();
    }

    void PopulationEventProcessor::ProcessEvent(const EconomicUpdateEvent& event) {
        LogEvent("EconomicUpdate", event.entity_id,
            "Tax Revenue: " + std::to_string(event.tax_revenue_potential) + 
            ", Workers: " + std::to_string(event.productive_workers) + 
            ", Unemployment: " + std::to_string(event.unemployment_rate * 100) + "%");

        UpdateGameState(event.entity_id, "economic_indicators_updated");

        // High unemployment triggers social problems
        if (event.unemployment_rate > 0.2) {
            TriggerConsequentialEvents(event.entity_id, "unemployment_crisis");
        }

        // High guild influence indicates strong craft economy
        if (event.guild_influence > 0.6) {
            TriggerConsequentialEvents(event.entity_id, "guild_dominance");
        }

        m_event_processing_counts["EconomicUpdate"]++;
        m_last_processed[event.entity_id] = std::chrono::steady_clock::now();
    }

    void PopulationEventProcessor::ProcessEvent(const MilitaryRecruitmentEvent& event) {
        LogEvent("MilitaryRecruitment", event.entity_id,
            "Needed: " + std::to_string(event.recruits_needed) + 
            ", Available: " + std::to_string(event.recruits_available) + 
            ", Quality: " + std::to_string(event.average_quality));

        UpdateGameState(event.entity_id, "military_recruitment_processed");

        // Recruitment shortfall indicates military weakness
        if (event.recruits_available < event.recruits_needed * 0.8) {
            TriggerConsequentialEvents(event.entity_id, "recruitment_shortage");
        }

        // High social disruption from recruitment
        if (event.social_disruption > 0.3) {
            TriggerConsequentialEvents(event.entity_id, "recruitment_unrest");
        }

        m_event_processing_counts["MilitaryRecruitment"]++;
        m_last_processed[event.entity_id] = std::chrono::steady_clock::now();
    }

    void PopulationEventProcessor::ProcessEvent(const CulturalAssimilationEvent& event) {
        LogEvent("CulturalAssimilation", event.entity_id,
            event.from_culture + " -> " + event.to_culture + 
            " (" + std::to_string(event.population_affected) + " people, " + 
            "Rate: " + std::to_string(event.assimilation_rate * 100) + "%)");

        UpdateGameState(event.entity_id, "cultural_composition_changed");

        // High resistance to assimilation might cause tension
        if (event.resistance_level > 0.7) {
            TriggerConsequentialEvents(event.entity_id, "cultural_resistance");
        }

        // Rapid assimilation indicates successful integration
        if (event.assimilation_rate > 0.1 && event.population_affected > 50) {
            TriggerConsequentialEvents(event.entity_id, "cultural_integration");
        }

        m_event_processing_counts["CulturalAssimilation"]++;
        m_last_processed[event.entity_id] = std::chrono::steady_clock::now();
    }

    void PopulationEventProcessor::ProcessEvent(const ReligiousConversionEvent& event) {
        LogEvent("ReligiousConversion", event.entity_id,
            event.from_religion + " -> " + event.to_religion + 
            " (" + std::to_string(event.population_affected) + " people, " + 
            "Mechanism: " + event.mechanism + ")");

        UpdateGameState(event.entity_id, "religious_composition_changed");

        // Forced conversions create social tension
        if (!event.voluntary && event.population_affected > 100) {
            TriggerConsequentialEvents(event.entity_id, "religious_persecution");
        }

        // Large voluntary conversions might indicate missionary success
        if (event.voluntary && event.conversion_rate > 0.05) {
            TriggerConsequentialEvents(event.entity_id, "religious_revival");
        }

        m_event_processing_counts["ReligiousConversion"]++;
        m_last_processed[event.entity_id] = std::chrono::steady_clock::now();
    }

    void PopulationEventProcessor::ProcessEvent(const GuildAdvancementEvent& event) {
        LogEvent("GuildAdvancement", event.entity_id,
            "Guild: " + event.guild_type + 
            " (" + std::to_string(event.population_affected) + " advanced, " +
            "Skill req: " + std::to_string(event.skill_requirement) + ")");

        UpdateGameState(event.entity_id, "guild_structure_advanced");

        // High skill requirements indicate professional development
        if (event.skill_requirement > 0.7) {
            TriggerConsequentialEvents(event.entity_id, "skill_development");
        }

        // Large guild advancement indicates economic growth
        if (event.population_affected > 50) {
            TriggerConsequentialEvents(event.entity_id, "craft_economy_expansion");
        }

        m_event_processing_counts["GuildAdvancement"]++;
        m_last_processed[event.entity_id] = std::chrono::steady_clock::now();
    }

    // ============================================================================
    // Crisis Event Processing Methods
    // ============================================================================

    void PopulationEventProcessor::ProcessEvent(const FamineEvent& event) {
        LogEvent("Famine", event.entity_id,
            "Severity: " + std::to_string(event.severity) + 
            ", Duration: " + std::to_string(event.duration_months) + " months" +
            ", Mortality increase: " + std::to_string(event.mortality_increase * 100) + "%");

        UpdateGameState(event.entity_id, "famine_active");
        
        // Mark as active crisis
        m_active_events[event.entity_id].push_back("famine");

        // Severe famines trigger additional consequences
        if (event.severity > 0.7) {
            TriggerConsequentialEvents(event.entity_id, "severe_famine");
        }

        // High migration pressure from famine
        if (event.migration_pressure > 500) {
            TriggerConsequentialEvents(event.entity_id, "famine_migration");
        }

        // Economic disruption triggers trade problems
        if (event.economic_disruption > 0.5) {
            TriggerConsequentialEvents(event.entity_id, "economic_collapse");
        }

        m_event_processing_counts["Famine"]++;
        m_last_processed[event.entity_id] = std::chrono::steady_clock::now();

        CORE_LOG_WARN("PopulationEventProcessor", 
            "Famine processed for province " + std::to_string(event.entity_id) + 
            " - Severity: " + std::to_string(event.severity));
    }

    void PopulationEventProcessor::ProcessEvent(const PlagueEvent& event) {
        LogEvent("Plague", event.entity_id,
            "Type: " + event.plague_type + 
            ", Infection rate: " + std::to_string(event.infection_rate * 100) + "%" +
            ", Mortality rate: " + std::to_string(event.mortality_rate * 100) + "%");

        UpdateGameState(event.entity_id, "plague_active");
        
        // Mark as active crisis
        m_active_events[event.entity_id].push_back("plague");

        // High infection rates trigger quarantine measures
        if (event.infection_rate > 0.3) {
            TriggerConsequentialEvents(event.entity_id, "quarantine_measures");
        }

        // Trade disruption from plague
        if (event.trade_impact > 0.4) {
            TriggerConsequentialEvents(event.entity_id, "trade_disruption");
        }

        // Social change potential from major plague
        if (event.social_change_potential > 0.6) {
            TriggerConsequentialEvents(event.entity_id, "social_upheaval");
        }

        m_event_processing_counts["Plague"]++;
        m_last_processed[event.entity_id] = std::chrono::steady_clock::now();

        CORE_LOG_ERROR("PopulationEventProcessor", 
            "Plague processed for province " + std::to_string(event.entity_id) + 
            " - Type: " + event.plague_type + 
            " - Mortality: " + std::to_string(event.mortality_rate));
    }

    void PopulationEventProcessor::ProcessEvent(const SocialUnrestEvent& event) {
        LogEvent("SocialUnrest", event.entity_id,
            "Grievance: " + event.primary_grievance + 
            ", Intensity: " + std::to_string(event.unrest_intensity) + 
            ", Participants: " + std::to_string(event.participants));

        UpdateGameState(event.entity_id, "social_unrest_active");
        
        // Mark as active crisis
        m_active_events[event.entity_id].push_back("social_unrest");

        // High violence triggers military response
        if (event.violence_level > 0.6) {
            TriggerConsequentialEvents(event.entity_id, "violent_uprising");
        }

        // Weak authority response might encourage more unrest
        if (event.authority_response < 0.3) {
            TriggerConsequentialEvents(event.entity_id, "authority_weakness");
        }

        // Strong authority response might cause oppression
        if (event.authority_response > 0.8 && event.violence_level < 0.3) {
            TriggerConsequentialEvents(event.entity_id, "harsh_crackdown");
        }

        // Significant property damage affects economy
        if (event.property_damage > 0.4) {
            TriggerConsequentialEvents(event.entity_id, "economic_damage");
        }

        m_event_processing_counts["SocialUnrest"]++;
        m_last_processed[event.entity_id] = std::chrono::steady_clock::now();

        CORE_LOG_WARN("PopulationEventProcessor", 
            "Social unrest processed for province " + std::to_string(event.entity_id) + 
            " - Grievance: " + event.primary_grievance + 
            " - Intensity: " + std::to_string(event.unrest_intensity));
    }

    void PopulationEventProcessor::ProcessEvent(const NaturalDisasterEvent& event) {
        LogEvent("NaturalDisaster", event.entity_id,
            "Type: " + event.disaster_type + 
            ", Severity: " + std::to_string(event.severity) + 
            ", Casualties: " + std::to_string(event.casualties));

        UpdateGameState(event.entity_id, "disaster_recovery");
        
        // Mark as active crisis
        m_active_events[event.entity_id].push_back(event.disaster_type);

        // Major disasters trigger reconstruction efforts
        if (event.severity > 0.6) {
            TriggerConsequentialEvents(event.entity_id, "major_disaster_response");
        }

        // Infrastructure damage triggers rebuilding
        if (event.infrastructure_damage > 0.4) {
            TriggerConsequentialEvents(event.entity_id, "infrastructure_rebuilding");
        }

        // Large displaced populations need resettlement
        if (event.displaced_population > 1000) {
            TriggerConsequentialEvents(event.entity_id, "refugee_crisis");
        }

        m_event_processing_counts["NaturalDisaster"]++;
        m_last_processed[event.entity_id] = std::chrono::steady_clock::now();

        CORE_LOG_ERROR("PopulationEventProcessor", 
            "Natural disaster processed for province " + std::to_string(event.entity_id) + 
            " - Type: " + event.disaster_type + 
            " - Casualties: " + std::to_string(event.casualties));
    }

    // ============================================================================
    // Settlement Event Processing Methods
    // ============================================================================

    void PopulationEventProcessor::ProcessEvent(const SettlementGrowthEvent& event) {
        LogEvent("SettlementGrowth", event.entity_id,
            "Settlement: " + event.settlement_name + 
            ", " + utils::GetSettlementTypeName(event.from_type) + " -> " + 
            utils::GetSettlementTypeName(event.to_type) + 
            ", Growth: " + std::to_string(event.population_growth));

        UpdateGameState(event.entity_id, "settlement_evolved");

        // Major settlement evolution indicates regional development
        if (event.to_type == SettlementType::CITY || event.to_type == SettlementType::LARGE_CITY) {
            TriggerConsequentialEvents(event.entity_id, "urban_center_emergence");
        }

        // Infrastructure demand from growth
        if (event.infrastructure_demand > 0.6) {
            TriggerConsequentialEvents(event.entity_id, "infrastructure_needs");
        }

        m_event_processing_counts["SettlementGrowth"]++;
        m_last_processed[event.entity_id] = std::chrono::steady_clock::now();
    }

    void PopulationEventProcessor::ProcessEvent(const UrbanizationEvent& event) {
        LogEvent("Urbanization", event.entity_id,
            "Urbanization: " + std::to_string(event.old_urbanization_rate * 100) + "% -> " + 
            std::to_string(event.new_urbanization_rate * 100) + "%" +
            ", Migrants: " + std::to_string(event.rural_to_urban_migrants));

        UpdateGameState(event.entity_id, "urbanization_increased");

        // High urban strain creates problems
        if (event.urban_strain > 0.7) {
            TriggerConsequentialEvents(event.entity_id, "urban_overcrowding");
        }

        // Rural labor shortage affects agriculture
        if (event.rural_labor_shortage > 0.5) {
            TriggerConsequentialEvents(event.entity_id, "agricultural_decline");
        }

        m_event_processing_counts["Urbanization"]++;
        m_last_processed[event.entity_id] = std::chrono::steady_clock::now();
    }

    void PopulationEventProcessor::ProcessEvent(const InfrastructureDevelopmentEvent& event) {
        LogEvent("InfrastructureDevelopment", event.entity_id,
            "Type: " + event.infrastructure_type + 
            ", Settlement: " + event.settlement_name + 
            ", Improvement: " + std::to_string(event.improvement_level));

        UpdateGameState(event.entity_id, "infrastructure_improved");

        // Major infrastructure projects indicate prosperity
        if (event.cost > 1000.0) {
            TriggerConsequentialEvents(event.entity_id, "major_construction");
        }

        // Population capacity increases allow growth
        if (event.population_capacity_increase > 0.3) {
            TriggerConsequentialEvents(event.entity_id, "growth_potential");
        }

        m_event_processing_counts["InfrastructureDevelopment"]++;
        m_last_processed[event.entity_id] = std::chrono::steady_clock::now();
    }

    // ============================================================================
    // Administrative Event Processing Methods
    // ============================================================================

    void PopulationEventProcessor::ProcessEvent(const TaxationChangeEvent& event) {
        LogEvent("TaxationChange", event.entity_id,
            "Rate: " + std::to_string(event.old_tax_rate * 100) + "% -> " + 
            std::to_string(event.new_tax_rate * 100) + "%" +
            ", Revenue change: " + std::to_string(event.revenue_change));

        UpdateGameState(event.entity_id, "taxation_changed");

        // High tax increases might cause unrest
        if (event.new_tax_rate > event.old_tax_rate + 0.05) {
            TriggerConsequentialEvents(event.entity_id, "tax_burden_increase");
        }

        // Low compliance indicates resistance
        if (event.compliance_rate < 0.6) {
            TriggerConsequentialEvents(event.entity_id, "tax_resistance");
        }

        m_event_processing_counts["TaxationChange"]++;
        m_last_processed[event.entity_id] = std::chrono::steady_clock::now();
    }

    void PopulationEventProcessor::ProcessEvent(const LegalCodeChangeEvent& event) {
        LogEvent("LegalCodeChange", event.entity_id,
            "Type: " + event.legal_change_type + 
            ", Stability impact: " + std::to_string(event.social_stability_impact));

        UpdateGameState(event.entity_id, "legal_system_reformed");

        // Major legal changes affect social order
        if (std::abs(event.social_stability_impact) > 0.3) {
            if (event.social_stability_impact > 0) {
                TriggerConsequentialEvents(event.entity_id, "legal_reform_success");
            } else {
                TriggerConsequentialEvents(event.entity_id, "legal_reform_backlash");
            }
        }

        m_event_processing_counts["LegalCodeChange"]++;
        m_last_processed[event.entity_id] = std::chrono::steady_clock::now();
    }

    void PopulationEventProcessor::ProcessEvent(const AdministrativeReformEvent& event) {
        LogEvent("AdministrativeReform", event.entity_id,
            "Type: " + event.reform_type + 
            ", Efficiency change: " + std::to_string(event.efficiency_change));

        UpdateGameState(event.entity_id, "administration_reformed");

        // Major efficiency improvements indicate modernization
        if (event.efficiency_change > 0.2) {
            TriggerConsequentialEvents(event.entity_id, "administrative_modernization");
        }

        // High implementation time might cause problems
        if (event.implementation_time_months > 12) {
            TriggerConsequentialEvents(event.entity_id, "reform_delays");
        }

        m_event_processing_counts["AdministrativeReform"]++;
        m_last_processed[event.entity_id] = std::chrono::steady_clock::now();
    }

    // ============================================================================
    // Migration and Growth Event Processing Methods
    // ============================================================================

    void PopulationEventProcessor::ProcessEvent(const MigrationEvent& event) {
        LogEvent("Migration", event.from_entity,
            "From: " + std::to_string(event.from_entity) + 
            " To: " + std::to_string(event.to_entity) + 
            ", Type: " + event.migration_type + 
            ", Population: " + std::to_string(event.migrant_population));

        UpdateGameState(event.from_entity, "population_emigrated");
        UpdateGameState(event.to_entity, "population_immigrated");

        // Large refugee movements indicate crisis
        if (event.migration_type == "refugee" && event.migrant_population > 500) {
            TriggerConsequentialEvents(event.to_entity, "refugee_influx");
            TriggerConsequentialEvents(event.from_entity, "population_exodus");
        }

        // High integration difficulty causes problems
        if (event.integration_difficulty > 0.7) {
            TriggerConsequentialEvents(event.to_entity, "integration_challenges");
        }

        m_event_processing_counts["Migration"]++;
        m_last_processed[event.from_entity] = std::chrono::steady_clock::now();
    }

    void PopulationEventProcessor::ProcessEvent(const PopulationGrowthEvent& event) {
        LogEvent("PopulationGrowth", event.entity_id,
            "Growth rate: " + std::to_string(event.old_growth_rate * 100) + "% -> " + 
            std::to_string(event.new_growth_rate * 100) + "%" +
            ", Driver: " + event.growth_driver);

        UpdateGameState(event.entity_id, "demographic_change");

        // Rapid growth might strain resources
        if (event.new_growth_rate > 0.03) {
            TriggerConsequentialEvents(event.entity_id, "rapid_population_growth");
        }

        // Population decline indicates problems
        if (event.new_growth_rate < -0.01) {
            TriggerConsequentialEvents(event.entity_id, "population_decline");
        }

        m_event_processing_counts["PopulationGrowth"]++;
        m_last_processed[event.entity_id] = std::chrono::steady_clock::now();
    }

    void PopulationEventProcessor::ProcessEvent(const EducationalAdvancementEvent& event) {
        LogEvent("EducationalAdvancement", event.entity_id,
            "Type: " + event.advancement_type + 
            ", Literacy improvement: " + std::to_string(event.literacy_improvement * 100) + "%" +
            ", Students: " + std::to_string(event.students_affected));

        UpdateGameState(event.entity_id, "education_improved");

        // Major educational advances indicate cultural development
        if (event.advancement_type == "university_establishment") {
            TriggerConsequentialEvents(event.entity_id, "intellectual_center");
        }

        // High cultural impact indicates renaissance potential
        if (event.cultural_impact > 0.6) {
            TriggerConsequentialEvents(event.entity_id, "cultural_flowering");
        }

        m_event_processing_counts["EducationalAdvancement"]++;
        m_last_processed[event.entity_id] = std::chrono::steady_clock::now();
    }

    // ============================================================================
    // Trend Analysis Implementation
    // ============================================================================

    PopulationTrendAnalysis PopulationEventProcessor::AnalyzeTrends(game::types::EntityID entity_id,
            const std::vector<PopulationUpdateEvent>& historical_events,
            std::chrono::hours analysis_period) {
        
        PopulationTrendAnalysis analysis;
        analysis.entity_id = entity_id;
        analysis.analysis_period_end = std::chrono::steady_clock::now();
        analysis.analysis_period_start = analysis.analysis_period_end - analysis_period;

        if (historical_events.size() < 2) {
            CORE_LOG_WARN("PopulationEventProcessor", 
                "Insufficient data for trend analysis - Province: " + std::to_string(entity_id));
            return analysis;
        }

        // Calculate population growth trend
        const auto& latest = historical_events.back();
        const auto& earlier = historical_events.front();
        
        if (earlier.total_population > 0) {
            analysis.population_growth_trend = 
                (static_cast<double>(latest.total_population) - earlier.total_population) / 
                earlier.total_population;
        }

        // Calculate other trends using linear regression over time series
        CalculateLiteracyTrend(analysis, historical_events);
        CalculateWealthTrend(analysis, historical_events);
        CalculateSocialMobilityTrend(analysis, entity_id);

        // Generate warnings based on trends
        GenerateTrendWarnings(analysis);
        GeneratePredictions(analysis);

        CORE_LOG_INFO("PopulationEventProcessor", 
            "Trend analysis completed for province " + std::to_string(entity_id) + 
            " - Growth trend: " + std::to_string(analysis.population_growth_trend * 100) + "%");

        return analysis;
    }

    // ============================================================================
    // Event Validation Methods
    // ============================================================================

    bool PopulationEventProcessor::ValidateEvent(const PopulationUpdateEvent& event) {
        if (event.total_population < 0) {
            CORE_LOG_WARN("PopulationEventProcessor", 
                "Invalid population count: " + std::to_string(event.total_population));
            return false;
        }

        if (event.military_eligible > event.total_population) {
            CORE_LOG_WARN("PopulationEventProcessor", 
                "Military eligible exceeds total population");
            return false;
        }

        if (std::abs(event.population_growth_rate) > 1.0) {
            CORE_LOG_WARN("PopulationEventProcessor", 
                "Unrealistic growth rate: " + std::to_string(event.population_growth_rate));
            return false;
        }

        return true;
    }

    bool PopulationEventProcessor::ValidateEvent(const SocialMobilityEvent& event) {
        if (event.population_affected <= 0) {
            CORE_LOG_WARN("PopulationEventProcessor", 
                "Invalid population affected in social mobility: " + std::to_string(event.population_affected));
            return false;
        }

        if (event.from_class == event.to_class) {
            CORE_LOG_WARN("PopulationEventProcessor", 
                "Social mobility event with identical from/to classes");
            return false;
        }

        return true;
    }

    bool PopulationEventProcessor::ValidateEvent(const CrisisEvent& event) {
        // Base crisis validation - would be called by specific crisis validators
        return true; // Placeholder implementation
    }

    // ============================================================================
    // Event Chain Processing Methods
    // ============================================================================

    void PopulationEventProcessor::ProcessEventChain(game::types::EntityID entity_id, const std::string& trigger_event) {
        CORE_LOG_DEBUG("PopulationEventProcessor", 
            "Processing event chain for province " + std::to_string(entity_id) + 
            " - Trigger: " + trigger_event);

        // Define event chains based on trigger events
        if (trigger_event == "severe_famine") {
            TriggerConsequentialEvents(entity_id, "social_unrest");
            TriggerConsequentialEvents(entity_id, "migration_pressure");
            TriggerConsequentialEvents(entity_id, "economic_collapse");
        }
        else if (trigger_event == "plague_outbreak") {
            TriggerConsequentialEvents(entity_id, "trade_disruption");
            TriggerConsequentialEvents(entity_id, "social_change");
            TriggerConsequentialEvents(entity_id, "religious_revival");
        }
        else if (trigger_event == "economic_boom") {
            TriggerConsequentialEvents(entity_id, "urban_growth");
            TriggerConsequentialEvents(entity_id, "social_mobility_increase");
            TriggerConsequentialEvents(entity_id, "cultural_development");
        }
        else if (trigger_event == "military_conquest") {
            TriggerConsequentialEvents(entity_id, "population_displacement");
            TriggerConsequentialEvents(entity_id, "cultural_suppression");
            TriggerConsequentialEvents(entity_id, "administrative_reorganization");
        }

        m_event_processing_counts["EventChain"]++;
    }

    std::vector<game::types::EntityID> PopulationEventProcessor::GetAffectedProvinces(const std::string& event_type) {
        std::vector<game::types::EntityID> affected_provinces;

        for (const auto& [entity_id, events] : m_active_events) {
            if (std::find(events.begin(), events.end(), event_type) != events.end()) {
                affected_provinces.push_back(entity_id);
            }
        }

        return affected_provinces;
    }

    // ============================================================================
    // Private Helper Methods
    // ============================================================================

    void PopulationEventProcessor::LogEvent(const std::string& event_type, game::types::EntityID entity_id, 
                                           const std::string& description) {
        CORE_LOG_INFO("PopulationEventProcessor", 
            "[" + event_type + "] Province " + std::to_string(entity_id) + ": " + description);
    }

    void PopulationEventProcessor::UpdateGameState(game::types::EntityID entity_id, const std::string& state_change) {
        // Update internal tracking of game state changes
        auto now = std::chrono::steady_clock::now();
        m_last_processed[entity_id] = now;

        CORE_LOG_DEBUG("PopulationEventProcessor", 
            "Game state updated for province " + std::to_string(entity_id) + ": " + state_change);
    }

    void PopulationEventProcessor::TriggerConsequentialEvents(game::types::EntityID entity_id, const std::string& trigger_event) {
        CORE_LOG_DEBUG("PopulationEventProcessor", 
            "Triggering consequential events for province " + std::to_string(entity_id) + 
            " - Trigger: " + trigger_event);

        // Add consequential event processing logic here
        // This would typically create new events and add them to a processing queue
        
        // Example consequences:
        if (trigger_event == "large_population") {
            // Large populations might trigger urbanization
            CalculateSecondaryEffects(entity_id, "urbanization_pressure");
        }
        else if (trigger_event == "unemployment_crisis") {
            // Unemployment might trigger social unrest
            CalculateSecondaryEffects(entity_id, "social_unrest_risk");
        }
        else if (trigger_event == "cultural_integration") {
            // Successful integration might reduce tensions
            CalculateSecondaryEffects(entity_id, "cultural_harmony");
        }

        m_event_processing_counts["ConsequentialEvent"]++;
    }

    void PopulationEventProcessor::CalculateSecondaryEffects(game::types::EntityID entity_id, const std::string& primary_event) {
        CORE_LOG_DEBUG("PopulationEventProcessor", 
            "Calculating secondary effects for province " + std::to_string(entity_id) + 
            " - Primary event: " + primary_event);

        // Calculate probability and magnitude of secondary effects
        double effect_probability = CalculateEffectProbability(primary_event);
        double effect_magnitude = CalculateEffectMagnitude(primary_event);

        // Apply secondary effects if probability threshold is met
        if (::utils::RandomGenerator::getInstance().randomFloat(0.0f, 1.0f) < effect_probability) {
            ApplySecondaryEffect(entity_id, primary_event, effect_magnitude);
        }
    }

    void PopulationEventProcessor::ValidateEventConsistency(game::types::EntityID entity_id) {
        CORE_LOG_DEBUG("PopulationEventProcessor", 
            "Validating event consistency for province " + std::to_string(entity_id));

        // Check for contradictory active events
        const auto& active = m_active_events[entity_id];
        
        // Example consistency checks:
        bool has_famine = std::find(active.begin(), active.end(), "famine") != active.end();
        bool has_prosperity = std::find(active.begin(), active.end(), "economic_boom") != active.end();
        
        if (has_famine && has_prosperity) {
            CORE_LOG_WARN("PopulationEventProcessor", 
                "Inconsistent events detected: famine and economic boom in province " + 
                std::to_string(entity_id));
        }
    }

    // ============================================================================
    // Trend Analysis Helper Methods
    // ============================================================================

    void PopulationEventProcessor::CalculateLiteracyTrend(PopulationTrendAnalysis& analysis, 
                                                         const std::vector<PopulationUpdateEvent>& events) {
        if (events.size() < 2) {
            analysis.literacy_trend = 0.0;
            return;
        }

        // Simple linear trend calculation
        double start_literacy = events.front().average_literacy;
        double end_literacy = events.back().average_literacy;
        
        analysis.literacy_trend = end_literacy - start_literacy;
    }

    void PopulationEventProcessor::CalculateWealthTrend(PopulationTrendAnalysis& analysis, 
                                                       const std::vector<PopulationUpdateEvent>& events) {
        if (events.size() < 2) {
            analysis.wealth_distribution_trend = 0.0;
            return;
        }

        // Calculate wealth inequality trend
        double start_wealth = events.front().average_wealth;
        double end_wealth = events.back().average_wealth;
        
        // Simple wealth change calculation
        if (start_wealth > 0) {
            analysis.wealth_distribution_trend = (end_wealth - start_wealth) / start_wealth;
        }
    }

    void PopulationEventProcessor::CalculateSocialMobilityTrend(PopulationTrendAnalysis& analysis, 
                                                              game::types::EntityID entity_id) {
        // Calculate social mobility based on processed events
        int mobility_events = m_event_processing_counts["SocialMobility"];
        
        // Normalize by time period and population
        analysis.overall_social_mobility = mobility_events * 0.001; // Simple calculation
    }

    void PopulationEventProcessor::GenerateTrendWarnings(PopulationTrendAnalysis& analysis) {
        // Generate warnings based on negative trends
        if (analysis.population_growth_trend < -0.05) {
            analysis.demographic_warnings.push_back("Rapid population decline detected");
        }

        if (analysis.literacy_trend < -0.02) {
            analysis.demographic_warnings.push_back("Literacy rates declining");
        }

        if (analysis.wealth_distribution_trend < -0.1) {
            analysis.economic_warnings.push_back("Wealth inequality increasing");
        }

        if (analysis.social_stability_trend < -0.1) {
            analysis.social_warnings.push_back("Social stability deteriorating");
        }
    }

    void PopulationEventProcessor::GeneratePredictions(PopulationTrendAnalysis& analysis) {
        // Generate predictions based on current trends
        if (analysis.population_growth_trend > 0.03) {
            analysis.predicted_developments.push_back("Continued population growth expected");
        }

        if (analysis.urbanization_trend > 0.02) {
            analysis.predicted_developments.push_back("Increased urbanization likely");
        }

        if (analysis.literacy_trend > 0.05) {
            analysis.predicted_developments.push_back("Educational advancement continuing");
        }
    }

    // ============================================================================
    // Effect Calculation Helper Methods
    // ============================================================================

    double PopulationEventProcessor::CalculateEffectProbability(const std::string& event_type) {
        // Base probabilities for different types of secondary effects
        std::unordered_map<std::string, double> base_probabilities = {
            {"urbanization_pressure", 0.3},
            {"social_unrest_risk", 0.4},
            {"cultural_harmony", 0.6},
            {"economic_disruption", 0.5},
            {"migration_pressure", 0.7}
        };

        auto it = base_probabilities.find(event_type);
        return (it != base_probabilities.end()) ? it->second : 0.2; // Default 20%
    }

    double PopulationEventProcessor::CalculateEffectMagnitude(const std::string& event_type) {
        // Base magnitudes for different types of secondary effects
        std::unordered_map<std::string, double> base_magnitudes = {
            {"urbanization_pressure", 0.1},
            {"social_unrest_risk", 0.2},
            {"cultural_harmony", 0.15},
            {"economic_disruption", 0.25},
            {"migration_pressure", 0.3}
        };

        auto it = base_magnitudes.find(event_type);
        return (it != base_magnitudes.end()) ? it->second : 0.1; // Default 10%
    }

    void PopulationEventProcessor::ApplySecondaryEffect(game::types::EntityID entity_id, const std::string& effect_type, 
                                                       double magnitude) {
        CORE_LOG_DEBUG("PopulationEventProcessor", 
            "Applying secondary effect to province " + std::to_string(entity_id) + 
            " - Effect: " + effect_type + 
            " - Magnitude: " + std::to_string(magnitude));

        // Apply the secondary effect based on type and magnitude
        // This would typically modify game state or create new events
        
        UpdateGameState(entity_id, "secondary_effect_applied");
        m_event_processing_counts["SecondaryEffect"]++;
    }

    // ============================================================================
    // PopulationEventFormatter Implementation
    // ============================================================================

    std::string PopulationEventFormatter::FormatEvent(const PopulationUpdateEvent& event) {
        return "Population Update - Province " + std::to_string(event.entity_id) + 
               ": " + FormatPopulationNumber(event.total_population) + " people" +
               " (Growth: " + FormatPercentage(event.population_growth_rate) + ")" +
               ", Wealth: " + FormatCurrency(event.average_wealth) +
               ", Happiness: " + FormatPercentage(event.average_happiness) +
               ", Military: " + std::to_string(event.military_eligible) + " eligible";
    }

    std::string PopulationEventFormatter::FormatEvent(const SocialMobilityEvent& event) {
        std::string direction = event.upward_mobility ? "↗" : "↘";
        return "Social Mobility " + direction + " - Province " + std::to_string(event.entity_id) + 
               ": " + std::to_string(event.population_affected) + " people moved from " +
               FormatSocialClassName(event.from_class) + " to " + 
               FormatSocialClassName(event.to_class) + " (" + event.reason + ")";
    }

    std::string PopulationEventFormatter::FormatEvent(const FamineEvent& event) {
        return "🌾 FAMINE - Province " + std::to_string(event.entity_id) + 
               ": Severity " + FormatPercentage(event.severity) +
               ", Duration: " + std::to_string(event.duration_months) + " months" +
               ", Mortality increase: " + FormatPercentage(event.mortality_increase);
    }

    std::string PopulationEventFormatter::FormatEvent(const PlagueEvent& event) {
        return "☠️ PLAGUE - Province " + std::to_string(event.entity_id) + 
               ": " + event.plague_type + 
               ", Infection rate: " + FormatPercentage(event.infection_rate) +
               ", Mortality rate: " + FormatPercentage(event.mortality_rate);
    }

    std::string PopulationEventFormatter::FormatSocialClassName(SocialClass social_class) {
        return utils::GetSocialClassName(social_class);
    }

    std::string PopulationEventFormatter::FormatPopulationNumber(int population) {
        if (population >= 1000000) {
            return std::to_string(population / 1000000) + "." + 
                   std::to_string((population % 1000000) / 100000) + "M";
        } else if (population >= 1000) {
            return std::to_string(population / 1000) + "." + 
                   std::to_string((population % 1000) / 100) + "K";
        }
        return std::to_string(population);
    }

    std::string PopulationEventFormatter::FormatPercentage(double percentage) {
        return std::to_string(static_cast<int>(percentage * 100)) + "%";
    }

    std::string PopulationEventFormatter::FormatCurrency(double amount) {
        return std::to_string(static_cast<int>(amount)) + " coins";
    }

} // namespace game::population
