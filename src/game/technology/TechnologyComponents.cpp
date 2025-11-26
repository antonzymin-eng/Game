// ============================================================================
// TechnologyComponents.cpp - Serialization Implementation
// Created: November 26, 2025
// Location: src/game/technology/TechnologyComponents.cpp
// ============================================================================

#include "game/technology/TechnologyComponents.h"
#include <sstream>

namespace game::technology {

// ============================================================================
// ResearchComponent Serialization
// ============================================================================

std::string ResearchComponent::Serialize() const {
    std::ostringstream oss;

    // Basic research state
    oss << "RESEARCH_V1\n";
    oss << "current_focus=" << static_cast<uint32_t>(current_focus) << "\n";
    oss << "focus_bonus=" << focus_bonus << "\n";

    // Infrastructure
    oss << "universities=" << universities << "\n";
    oss << "monasteries=" << monasteries << "\n";
    oss << "libraries=" << libraries << "\n";
    oss << "workshops=" << workshops << "\n";
    oss << "scholar_population=" << scholar_population << "\n";

    // Efficiency modifiers
    oss << "base_research_efficiency=" << base_research_efficiency << "\n";
    oss << "literacy_bonus=" << literacy_bonus << "\n";
    oss << "trade_network_bonus=" << trade_network_bonus << "\n";
    oss << "stability_bonus=" << stability_bonus << "\n";
    oss << "war_military_bonus=" << war_military_bonus << "\n";

    // Investment
    oss << "monthly_research_budget=" << monthly_research_budget << "\n";
    oss << "total_research_investment=" << total_research_investment << "\n";

    // Specialization
    oss << "primary_specialization=" << static_cast<uint32_t>(primary_specialization) << "\n";
    oss << "secondary_specializations_count=" << secondary_specializations.size() << "\n";
    for (const auto& spec : secondary_specializations) {
        oss << static_cast<uint32_t>(spec) << "\n";
    }

    // Technology states
    oss << "technology_states_count=" << technology_states.size() << "\n";
    for (const auto& [tech, state] : technology_states) {
        oss << static_cast<uint32_t>(tech) << "=" << static_cast<uint32_t>(state) << "\n";
    }

    // Research progress
    oss << "research_progress_count=" << research_progress.size() << "\n";
    for (const auto& [tech, progress] : research_progress) {
        oss << static_cast<uint32_t>(tech) << "=" << progress << "\n";
    }

    // Implementation level
    oss << "implementation_level_count=" << implementation_level.size() << "\n";
    for (const auto& [tech, level] : implementation_level) {
        oss << static_cast<uint32_t>(tech) << "=" << level << "\n";
    }

    // Category investment
    oss << "category_investment_count=" << category_investment.size() << "\n";
    for (const auto& [cat, invest] : category_investment) {
        oss << static_cast<uint32_t>(cat) << "=" << invest << "\n";
    }

    return oss.str();
}

bool ResearchComponent::Deserialize(const std::string& data) {
    std::istringstream iss(data);
    std::string line;

    // Check version
    if (!std::getline(iss, line) || line != "RESEARCH_V1") {
        return false;
    }

    auto parse_key_value = [](const std::string& line, std::string& key, std::string& value) {
        size_t pos = line.find('=');
        if (pos == std::string::npos) return false;
        key = line.substr(0, pos);
        value = line.substr(pos + 1);
        return true;
    };

    try {
        while (std::getline(iss, line)) {
            std::string key, value;
            if (!parse_key_value(line, key, value)) continue;

            if (key == "current_focus") {
                current_focus = static_cast<TechnologyType>(std::stoul(value));
            } else if (key == "focus_bonus") {
                focus_bonus = std::stod(value);
            } else if (key == "universities") {
                universities = std::stoul(value);
            } else if (key == "monasteries") {
                monasteries = std::stoul(value);
            } else if (key == "libraries") {
                libraries = std::stoul(value);
            } else if (key == "workshops") {
                workshops = std::stoul(value);
            } else if (key == "scholar_population") {
                scholar_population = std::stoul(value);
            } else if (key == "base_research_efficiency") {
                base_research_efficiency = std::stod(value);
            } else if (key == "literacy_bonus") {
                literacy_bonus = std::stod(value);
            } else if (key == "trade_network_bonus") {
                trade_network_bonus = std::stod(value);
            } else if (key == "stability_bonus") {
                stability_bonus = std::stod(value);
            } else if (key == "war_military_bonus") {
                war_military_bonus = std::stod(value);
            } else if (key == "monthly_research_budget") {
                monthly_research_budget = std::stod(value);
            } else if (key == "total_research_investment") {
                total_research_investment = std::stod(value);
            } else if (key == "primary_specialization") {
                primary_specialization = static_cast<TechnologyCategory>(std::stoul(value));
            } else if (key == "secondary_specializations_count") {
                size_t count = std::stoul(value);
                secondary_specializations.clear();
                for (size_t i = 0; i < count; ++i) {
                    if (std::getline(iss, line)) {
                        secondary_specializations.push_back(static_cast<TechnologyCategory>(std::stoul(line)));
                    }
                }
            } else if (key == "technology_states_count") {
                size_t count = std::stoul(value);
                technology_states.clear();
                for (size_t i = 0; i < count; ++i) {
                    if (std::getline(iss, line)) {
                        std::string k, v;
                        if (parse_key_value(line, k, v)) {
                            technology_states[static_cast<TechnologyType>(std::stoul(k))] =
                                static_cast<ResearchState>(std::stoul(v));
                        }
                    }
                }
            } else if (key == "research_progress_count") {
                size_t count = std::stoul(value);
                research_progress.clear();
                for (size_t i = 0; i < count; ++i) {
                    if (std::getline(iss, line)) {
                        std::string k, v;
                        if (parse_key_value(line, k, v)) {
                            research_progress[static_cast<TechnologyType>(std::stoul(k))] = std::stod(v);
                        }
                    }
                }
            } else if (key == "implementation_level_count") {
                size_t count = std::stoul(value);
                implementation_level.clear();
                for (size_t i = 0; i < count; ++i) {
                    if (std::getline(iss, line)) {
                        std::string k, v;
                        if (parse_key_value(line, k, v)) {
                            implementation_level[static_cast<TechnologyType>(std::stoul(k))] = std::stod(v);
                        }
                    }
                }
            } else if (key == "category_investment_count") {
                size_t count = std::stoul(value);
                category_investment.clear();
                for (size_t i = 0; i < count; ++i) {
                    if (std::getline(iss, line)) {
                        std::string k, v;
                        if (parse_key_value(line, k, v)) {
                            category_investment[static_cast<TechnologyCategory>(std::stoul(k))] = std::stod(v);
                        }
                    }
                }
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

// ============================================================================
// InnovationComponent Serialization
// ============================================================================

std::string InnovationComponent::Serialize() const {
    std::ostringstream oss;

    oss << "INNOVATION_V1\n";
    oss << "innovation_rate=" << innovation_rate << "\n";
    oss << "breakthrough_chance=" << breakthrough_chance << "\n";
    oss << "invention_quality=" << invention_quality << "\n";

    // Innovation sources
    oss << "inventors=" << inventors << "\n";
    oss << "craftsmen_innovators=" << craftsmen_innovators << "\n";
    oss << "scholar_innovators=" << scholar_innovators << "\n";
    oss << "foreign_scholars=" << foreign_scholars << "\n";

    // Environment
    oss << "cultural_openness=" << cultural_openness << "\n";
    oss << "innovation_encouragement=" << innovation_encouragement << "\n";
    oss << "knowledge_preservation_rate=" << knowledge_preservation_rate << "\n";
    oss << "experimentation_freedom=" << experimentation_freedom << "\n";

    // Modifiers
    oss << "guild_resistance=" << guild_resistance << "\n";
    oss << "religious_restriction=" << religious_restriction << "\n";
    oss << "royal_patronage=" << royal_patronage << "\n";
    oss << "merchant_funding=" << merchant_funding << "\n";

    // Recent discoveries
    oss << "recent_discoveries_count=" << recent_discoveries.size() << "\n";
    for (const auto& tech : recent_discoveries) {
        oss << static_cast<uint32_t>(tech) << "\n";
    }

    // Innovation attempts
    oss << "innovation_attempts_count=" << innovation_attempts.size() << "\n";
    for (const auto& attempt : innovation_attempts) {
        oss << attempt << "\n";
    }

    // Failed experiments
    oss << "failed_experiments_count=" << failed_experiments.size() << "\n";
    for (const auto& experiment : failed_experiments) {
        oss << experiment << "\n";
    }

    // Innovation expertise
    oss << "innovation_expertise_count=" << innovation_expertise.size() << "\n";
    for (const auto& [cat, expertise] : innovation_expertise) {
        oss << static_cast<uint32_t>(cat) << "=" << expertise << "\n";
    }

    // Local innovations
    oss << "local_innovations_count=" << local_innovations.size() << "\n";
    for (const auto& innovation : local_innovations) {
        oss << innovation << "\n";
    }

    return oss.str();
}

bool InnovationComponent::Deserialize(const std::string& data) {
    std::istringstream iss(data);
    std::string line;

    // Check version
    if (!std::getline(iss, line) || line != "INNOVATION_V1") {
        return false;
    }

    auto parse_key_value = [](const std::string& line, std::string& key, std::string& value) {
        size_t pos = line.find('=');
        if (pos == std::string::npos) return false;
        key = line.substr(0, pos);
        value = line.substr(pos + 1);
        return true;
    };

    try {
        while (std::getline(iss, line)) {
            std::string key, value;
            if (!parse_key_value(line, key, value)) continue;

            if (key == "innovation_rate") {
                innovation_rate = std::stod(value);
            } else if (key == "breakthrough_chance") {
                breakthrough_chance = std::stod(value);
            } else if (key == "invention_quality") {
                invention_quality = std::stod(value);
            } else if (key == "inventors") {
                inventors = std::stoul(value);
            } else if (key == "craftsmen_innovators") {
                craftsmen_innovators = std::stoul(value);
            } else if (key == "scholar_innovators") {
                scholar_innovators = std::stoul(value);
            } else if (key == "foreign_scholars") {
                foreign_scholars = std::stoul(value);
            } else if (key == "cultural_openness") {
                cultural_openness = std::stod(value);
            } else if (key == "innovation_encouragement") {
                innovation_encouragement = std::stod(value);
            } else if (key == "knowledge_preservation_rate") {
                knowledge_preservation_rate = std::stod(value);
            } else if (key == "experimentation_freedom") {
                experimentation_freedom = std::stod(value);
            } else if (key == "guild_resistance") {
                guild_resistance = std::stod(value);
            } else if (key == "religious_restriction") {
                religious_restriction = std::stod(value);
            } else if (key == "royal_patronage") {
                royal_patronage = std::stod(value);
            } else if (key == "merchant_funding") {
                merchant_funding = std::stod(value);
            } else if (key == "recent_discoveries_count") {
                size_t count = std::stoul(value);
                recent_discoveries.clear();
                for (size_t i = 0; i < count; ++i) {
                    if (std::getline(iss, line)) {
                        recent_discoveries.push_back(static_cast<TechnologyType>(std::stoul(line)));
                    }
                }
            } else if (key == "innovation_attempts_count") {
                size_t count = std::stoul(value);
                innovation_attempts.clear();
                for (size_t i = 0; i < count; ++i) {
                    if (std::getline(iss, line)) {
                        innovation_attempts.push_back(line);
                    }
                }
            } else if (key == "failed_experiments_count") {
                size_t count = std::stoul(value);
                failed_experiments.clear();
                for (size_t i = 0; i < count; ++i) {
                    if (std::getline(iss, line)) {
                        failed_experiments.push_back(line);
                    }
                }
            } else if (key == "innovation_expertise_count") {
                size_t count = std::stoul(value);
                innovation_expertise.clear();
                for (size_t i = 0; i < count; ++i) {
                    if (std::getline(iss, line)) {
                        std::string k, v;
                        if (parse_key_value(line, k, v)) {
                            innovation_expertise[static_cast<TechnologyCategory>(std::stoul(k))] = std::stod(v);
                        }
                    }
                }
            } else if (key == "local_innovations_count") {
                size_t count = std::stoul(value);
                local_innovations.clear();
                for (size_t i = 0; i < count; ++i) {
                    if (std::getline(iss, line)) {
                        local_innovations.push_back(line);
                    }
                }
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace game::technology
