// ============================================================================
// TechnologySerialization.cpp - Serialization Support for Technology Components
// Created: 2025-01-19
// Location: src/game/technology/TechnologySerialization.cpp
// Implements save/load functionality for technology system
// ============================================================================

#include "game/technology/TechnologyComponents.h"
#include <iostream>
#include <sstream>

namespace game::technology {

// ============================================================================
// ResearchComponent Serialization
// ============================================================================

void ResearchComponent::Serialize(std::ostream& out) const {
    // Write version for future compatibility
    uint32_t version = 1;
    out.write(reinterpret_cast<const char*>(&version), sizeof(version));

    // Serialize technology states
    uint32_t state_count = static_cast<uint32_t>(technology_states.size());
    out.write(reinterpret_cast<const char*>(&state_count), sizeof(state_count));
    for (const auto& [tech, state] : technology_states) {
        out.write(reinterpret_cast<const char*>(&tech), sizeof(tech));
        out.write(reinterpret_cast<const char*>(&state), sizeof(state));
    }

    // Serialize research progress
    uint32_t progress_count = static_cast<uint32_t>(research_progress.size());
    out.write(reinterpret_cast<const char*>(&progress_count), sizeof(progress_count));
    for (const auto& [tech, progress] : research_progress) {
        out.write(reinterpret_cast<const char*>(&tech), sizeof(tech));
        out.write(reinterpret_cast<const char*>(&progress), sizeof(progress));
    }

    // Serialize implementation levels
    uint32_t impl_count = static_cast<uint32_t>(implementation_level.size());
    out.write(reinterpret_cast<const char*>(&impl_count), sizeof(impl_count));
    for (const auto& [tech, level] : implementation_level) {
        out.write(reinterpret_cast<const char*>(&tech), sizeof(tech));
        out.write(reinterpret_cast<const char*>(&level), sizeof(level));
    }

    // Serialize simple fields
    out.write(reinterpret_cast<const char*>(&current_focus), sizeof(current_focus));
    out.write(reinterpret_cast<const char*>(&focus_bonus), sizeof(focus_bonus));
    out.write(reinterpret_cast<const char*>(&universities), sizeof(universities));
    out.write(reinterpret_cast<const char*>(&monasteries), sizeof(monasteries));
    out.write(reinterpret_cast<const char*>(&libraries), sizeof(libraries));
    out.write(reinterpret_cast<const char*>(&workshops), sizeof(workshops));
    out.write(reinterpret_cast<const char*>(&scholar_population), sizeof(scholar_population));
    out.write(reinterpret_cast<const char*>(&base_research_efficiency), sizeof(base_research_efficiency));
    out.write(reinterpret_cast<const char*>(&literacy_bonus), sizeof(literacy_bonus));
    out.write(reinterpret_cast<const char*>(&trade_network_bonus), sizeof(trade_network_bonus));
    out.write(reinterpret_cast<const char*>(&stability_bonus), sizeof(stability_bonus));
    out.write(reinterpret_cast<const char*>(&war_military_bonus), sizeof(war_military_bonus));
    out.write(reinterpret_cast<const char*>(&monthly_research_budget), sizeof(monthly_research_budget));
    out.write(reinterpret_cast<const char*>(&total_research_investment), sizeof(total_research_investment));
    out.write(reinterpret_cast<const char*>(&primary_specialization), sizeof(primary_specialization));

    // Serialize category investments
    uint32_t cat_inv_count = static_cast<uint32_t>(category_investment.size());
    out.write(reinterpret_cast<const char*>(&cat_inv_count), sizeof(cat_inv_count));
    for (const auto& [cat, investment] : category_investment) {
        out.write(reinterpret_cast<const char*>(&cat), sizeof(cat));
        out.write(reinterpret_cast<const char*>(&investment), sizeof(investment));
    }

    // Serialize secondary specializations
    uint32_t sec_spec_count = static_cast<uint32_t>(secondary_specializations.size());
    out.write(reinterpret_cast<const char*>(&sec_spec_count), sizeof(sec_spec_count));
    for (const auto& spec : secondary_specializations) {
        out.write(reinterpret_cast<const char*>(&spec), sizeof(spec));
    }
}

void ResearchComponent::Deserialize(const std::string& data) {
    std::istringstream in(data, std::ios::binary);
    // Read version
    uint32_t version;
    in.read(reinterpret_cast<char*>(&version), sizeof(version));

    // Deserialize technology states
    uint32_t state_count;
    in.read(reinterpret_cast<char*>(&state_count), sizeof(state_count));
    technology_states.clear();
    for (uint32_t i = 0; i < state_count; ++i) {
        TechnologyType tech;
        ResearchState state;
        in.read(reinterpret_cast<char*>(&tech), sizeof(tech));
        in.read(reinterpret_cast<char*>(&state), sizeof(state));
        technology_states[tech] = state;
    }

    // Deserialize research progress
    uint32_t progress_count;
    in.read(reinterpret_cast<char*>(&progress_count), sizeof(progress_count));
    research_progress.clear();
    for (uint32_t i = 0; i < progress_count; ++i) {
        TechnologyType tech;
        double progress;
        in.read(reinterpret_cast<char*>(&tech), sizeof(tech));
        in.read(reinterpret_cast<char*>(&progress), sizeof(progress));
        research_progress[tech] = progress;
    }

    // Deserialize implementation levels
    uint32_t impl_count;
    in.read(reinterpret_cast<char*>(&impl_count), sizeof(impl_count));
    implementation_level.clear();
    for (uint32_t i = 0; i < impl_count; ++i) {
        TechnologyType tech;
        double level;
        in.read(reinterpret_cast<char*>(&tech), sizeof(tech));
        in.read(reinterpret_cast<char*>(&level), sizeof(level));
        implementation_level[tech] = level;
    }

    // Deserialize simple fields
    in.read(reinterpret_cast<char*>(&current_focus), sizeof(current_focus));
    in.read(reinterpret_cast<char*>(&focus_bonus), sizeof(focus_bonus));
    in.read(reinterpret_cast<char*>(&universities), sizeof(universities));
    in.read(reinterpret_cast<char*>(&monasteries), sizeof(monasteries));
    in.read(reinterpret_cast<char*>(&libraries), sizeof(libraries));
    in.read(reinterpret_cast<char*>(&workshops), sizeof(workshops));
    in.read(reinterpret_cast<char*>(&scholar_population), sizeof(scholar_population));
    in.read(reinterpret_cast<char*>(&base_research_efficiency), sizeof(base_research_efficiency));
    in.read(reinterpret_cast<char*>(&literacy_bonus), sizeof(literacy_bonus));
    in.read(reinterpret_cast<char*>(&trade_network_bonus), sizeof(trade_network_bonus));
    in.read(reinterpret_cast<char*>(&stability_bonus), sizeof(stability_bonus));
    in.read(reinterpret_cast<char*>(&war_military_bonus), sizeof(war_military_bonus));
    in.read(reinterpret_cast<char*>(&monthly_research_budget), sizeof(monthly_research_budget));
    in.read(reinterpret_cast<char*>(&total_research_investment), sizeof(total_research_investment));
    in.read(reinterpret_cast<char*>(&primary_specialization), sizeof(primary_specialization));

    // Deserialize category investments
    uint32_t cat_inv_count;
    in.read(reinterpret_cast<char*>(&cat_inv_count), sizeof(cat_inv_count));
    category_investment.clear();
    for (uint32_t i = 0; i < cat_inv_count; ++i) {
        TechnologyCategory cat;
        double investment;
        in.read(reinterpret_cast<char*>(&cat), sizeof(cat));
        in.read(reinterpret_cast<char*>(&investment), sizeof(investment));
        category_investment[cat] = investment;
    }

    // Deserialize secondary specializations
    uint32_t sec_spec_count;
    in.read(reinterpret_cast<char*>(&sec_spec_count), sizeof(sec_spec_count));
    secondary_specializations.clear();
    for (uint32_t i = 0; i < sec_spec_count; ++i) {
        TechnologyCategory spec;
        in.read(reinterpret_cast<char*>(&spec), sizeof(spec));
        secondary_specializations.push_back(spec);
    }
}

// ============================================================================
// InnovationComponent Serialization
// ============================================================================

void InnovationComponent::Serialize(std::ostream& out) const {
    // Write version
    uint32_t version = 1;
    out.write(reinterpret_cast<const char*>(&version), sizeof(version));

    // Serialize simple fields
    out.write(reinterpret_cast<const char*>(&innovation_rate), sizeof(innovation_rate));
    out.write(reinterpret_cast<const char*>(&breakthrough_chance), sizeof(breakthrough_chance));
    out.write(reinterpret_cast<const char*>(&invention_quality), sizeof(invention_quality));
    out.write(reinterpret_cast<const char*>(&inventors), sizeof(inventors));
    out.write(reinterpret_cast<const char*>(&craftsmen_innovators), sizeof(craftsmen_innovators));
    out.write(reinterpret_cast<const char*>(&scholar_innovators), sizeof(scholar_innovators));
    out.write(reinterpret_cast<const char*>(&foreign_scholars), sizeof(foreign_scholars));
    out.write(reinterpret_cast<const char*>(&cultural_openness), sizeof(cultural_openness));
    out.write(reinterpret_cast<const char*>(&innovation_encouragement), sizeof(innovation_encouragement));
    out.write(reinterpret_cast<const char*>(&knowledge_preservation_rate), sizeof(knowledge_preservation_rate));
    out.write(reinterpret_cast<const char*>(&experimentation_freedom), sizeof(experimentation_freedom));
    out.write(reinterpret_cast<const char*>(&guild_resistance), sizeof(guild_resistance));
    out.write(reinterpret_cast<const char*>(&religious_restriction), sizeof(religious_restriction));
    out.write(reinterpret_cast<const char*>(&royal_patronage), sizeof(royal_patronage));
    out.write(reinterpret_cast<const char*>(&merchant_funding), sizeof(merchant_funding));

    // Serialize recent discoveries
    uint32_t disc_count = static_cast<uint32_t>(recent_discoveries.size());
    out.write(reinterpret_cast<const char*>(&disc_count), sizeof(disc_count));
    for (const auto& tech : recent_discoveries) {
        out.write(reinterpret_cast<const char*>(&tech), sizeof(tech));
    }

    // Serialize innovation expertise
    uint32_t exp_count = static_cast<uint32_t>(innovation_expertise.size());
    out.write(reinterpret_cast<const char*>(&exp_count), sizeof(exp_count));
    for (const auto& [cat, expertise] : innovation_expertise) {
        out.write(reinterpret_cast<const char*>(&cat), sizeof(cat));
        out.write(reinterpret_cast<const char*>(&expertise), sizeof(expertise));
    }

    // Note: innovation_attempts, failed_experiments, and local_innovations are not serialized
    // as they are temporary/ephemeral data
}

void InnovationComponent::Deserialize(const std::string& data) {
    std::istringstream in(data, std::ios::binary);
    // Read version
    uint32_t version;
    in.read(reinterpret_cast<char*>(&version), sizeof(version));

    // Deserialize simple fields
    in.read(reinterpret_cast<char*>(&innovation_rate), sizeof(innovation_rate));
    in.read(reinterpret_cast<char*>(&breakthrough_chance), sizeof(breakthrough_chance));
    in.read(reinterpret_cast<char*>(&invention_quality), sizeof(invention_quality));
    in.read(reinterpret_cast<char*>(&inventors), sizeof(inventors));
    in.read(reinterpret_cast<char*>(&craftsmen_innovators), sizeof(craftsmen_innovators));
    in.read(reinterpret_cast<char*>(&scholar_innovators), sizeof(scholar_innovators));
    in.read(reinterpret_cast<char*>(&foreign_scholars), sizeof(foreign_scholars));
    in.read(reinterpret_cast<char*>(&cultural_openness), sizeof(cultural_openness));
    in.read(reinterpret_cast<char*>(&innovation_encouragement), sizeof(innovation_encouragement));
    in.read(reinterpret_cast<char*>(&knowledge_preservation_rate), sizeof(knowledge_preservation_rate));
    in.read(reinterpret_cast<char*>(&experimentation_freedom), sizeof(experimentation_freedom));
    in.read(reinterpret_cast<char*>(&guild_resistance), sizeof(guild_resistance));
    in.read(reinterpret_cast<char*>(&religious_restriction), sizeof(religious_restriction));
    in.read(reinterpret_cast<char*>(&royal_patronage), sizeof(royal_patronage));
    in.read(reinterpret_cast<char*>(&merchant_funding), sizeof(merchant_funding));

    // Deserialize recent discoveries
    uint32_t disc_count;
    in.read(reinterpret_cast<char*>(&disc_count), sizeof(disc_count));
    recent_discoveries.clear();
    for (uint32_t i = 0; i < disc_count; ++i) {
        TechnologyType tech;
        in.read(reinterpret_cast<char*>(&tech), sizeof(tech));
        recent_discoveries.push_back(tech);
    }

    // Deserialize innovation expertise
    uint32_t exp_count;
    in.read(reinterpret_cast<char*>(&exp_count), sizeof(exp_count));
    innovation_expertise.clear();
    for (uint32_t i = 0; i < exp_count; ++i) {
        TechnologyCategory cat;
        double expertise;
        in.read(reinterpret_cast<char*>(&cat), sizeof(cat));
        in.read(reinterpret_cast<char*>(&expertise), sizeof(expertise));
        innovation_expertise[cat] = expertise;
    }

    // Temporary data is left empty
    innovation_attempts.clear();
    failed_experiments.clear();
    local_innovations.clear();
}

} // namespace game::technology
