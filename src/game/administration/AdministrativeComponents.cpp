// ============================================================================
// AdministrativeComponents.cpp - ECS Component Method Implementations
// Created: October 13, 2025 - Following Architecture Database Patterns
// Updated: October 28, 2025 - Unified AdministrativeOfficial implementation
// Location: src/game/administration/AdministrativeComponents.cpp
// ============================================================================

#include "game/administration/AdministrativeComponents.h"
#include "core/logging/Logger.h"
#include "utils/RandomGenerator.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace game::administration {

    // ============================================================================
    // AdministrativeOfficial Implementation
    // ============================================================================

    AdministrativeOfficial::AdministrativeOfficial(uint32_t id, const std::string& official_name, 
                                                   OfficialType official_type, game::types::EntityID province)
        : official_id(id), name(official_name), type(official_type), assigned_province(province) {
        // Initialize with reasonable defaults
        competence = 0.5 + (utils::RandomGenerator::getInstance().randomFloat() * 0.3); // 0.5-0.8
        loyalty = 0.7 + (utils::RandomGenerator::getInstance().randomFloat() * 0.2); // 0.7-0.9
        efficiency = competence; // Efficiency starts equal to competence
        corruption_resistance = 0.6 + (utils::RandomGenerator::getInstance().randomFloat() * 0.3); // 0.6-0.9
        satisfaction = 0.7;
        age = 25 + (utils::RandomGenerator::getInstance().randomInt(0, 100) % 20); // 25-44
    }

    double AdministrativeOfficial::GetEffectiveCompetence() const {
        double effective = competence;
        
        // Loyalty modifier: loyal officials work harder
        effective *= (1.0 + (loyalty - 0.5) * 0.2); // ±10% based on loyalty
        
        // Trait modifiers
        if (HasTrait(OfficialTrait::EFFICIENT)) {
            effective *= 1.15; // +15%
        }
        if (HasTrait(OfficialTrait::EXPERIENCED)) {
            effective *= 1.10; // +10%
        }
        if (HasTrait(OfficialTrait::CORRUPT)) {
            effective *= 0.80; // -20%
        }
        if (HasTrait(OfficialTrait::STUBBORN)) {
            effective *= 0.95; // -5%
        }
        if (HasTrait(OfficialTrait::SCHOLARLY)) {
            effective *= 1.05; // +5%
        }
        
        // Age penalty for very old officials
        if (age > 60) {
            effective *= 0.95; // -5%
        }
        
        return std::clamp(effective, 0.0, 1.0);
    }

    double AdministrativeOfficial::GetLoyaltyModifier() const {
        double modifier = loyalty;
        
        // Satisfaction affects loyalty
        modifier += (satisfaction - 0.5) * 0.1;
        
        // Trait modifiers
        if (HasTrait(OfficialTrait::LOYAL)) {
            modifier += 0.15;
        }
        if (HasTrait(OfficialTrait::AMBITIOUS)) {
            modifier -= 0.10; // Ambitious officials are less loyal
        }
        if (HasTrait(OfficialTrait::WELL_CONNECTED)) {
            modifier += 0.05;
        }
        
        return std::clamp(modifier, 0.0, 1.0);
    }

    double AdministrativeOfficial::GetMonthlyUpkeepCost() const {
        double cost = salary_cost;
        
        // Experience and traits increase cost
        if (HasTrait(OfficialTrait::EXPERIENCED)) {
            cost *= 1.2;
        }
        if (HasTrait(OfficialTrait::WELL_CONNECTED)) {
            cost *= 1.15;
        }
        if (HasTrait(OfficialTrait::SCHOLARLY)) {
            cost *= 1.1;
        }
        
        // High-competence officials cost more
        cost *= (0.8 + competence * 0.4); // 80%-120% based on competence
        
        return cost;
    }

    bool AdministrativeOfficial::IsCorrupt() const {
        return corruption_suspicion > 70 || HasTrait(OfficialTrait::CORRUPT);
    }

    void AdministrativeOfficial::ProcessMonthlyUpdate(double competence_drift_rate, double satisfaction_decay_rate) {
        // Age
        if (utils::RandomGenerator::getInstance().randomFloat() < 0.083) { // ~1/12 chance
            age++;
        }
        
        // Months in position
        months_in_position++;
        
        // Competence drift (small random changes)
        double drift = (utils::RandomGenerator::getInstance().randomFloat() - 0.5) * competence_drift_rate;
        competence = std::clamp(competence + drift, 0.0, 1.0);
        
        // Satisfaction naturally decays
        satisfaction = std::clamp(satisfaction - satisfaction_decay_rate, 0.0, 1.0);
        
        // Loyalty increases with tenure (if satisfied)
        if (satisfaction > 0.6 && months_in_position % 12 == 0) {
            loyalty = std::clamp(loyalty + 0.05, 0.0, 1.0);
        }
        
        // Gain experience trait after a year
        if (months_in_position >= 12 && !HasTrait(OfficialTrait::EXPERIENCED)) {
            if (competence > 0.6) {
                AddTrait(OfficialTrait::EXPERIENCED);
            }
        }
        
        // Corruption suspicion changes based on traits and satisfaction
        if (HasTrait(OfficialTrait::CORRUPT)) {
            corruption_suspicion = std::min(100u, corruption_suspicion + 2);
        } else if (satisfaction < 0.3) {
            // Dissatisfied officials may become corrupt
            if (utils::RandomGenerator::getInstance().randomFloat() < 0.1) {
                corruption_suspicion = std::min(100u, corruption_suspicion + 1);
            }
        } else if (corruption_suspicion > 0 && corruption_resistance > 0.7) {
            // High resistance officials reduce suspicion over time
            corruption_suspicion = std::max(0u, corruption_suspicion - 1);
        }
        
        // Update efficiency based on current state
        efficiency = GetEffectiveCompetence();
    }

    void AdministrativeOfficial::AdjustSatisfaction(double change) {
        satisfaction = std::clamp(satisfaction + change, 0.0, 1.0);
        
        // Very low satisfaction may trigger corrupt trait
        if (satisfaction < 0.2 && !HasTrait(OfficialTrait::CORRUPT)) {
            if (utils::RandomGenerator::getInstance().randomFloat() < 0.05) { // 5% chance
                AddTrait(OfficialTrait::CORRUPT);
                ::core::logging::LogWarning("AdministrativeOfficial", 
                    "Official " + name + " has become corrupt due to low satisfaction");
            }
        }
    }

    bool AdministrativeOfficial::HasTrait(OfficialTrait trait) const {
        return std::find(traits.begin(), traits.end(), trait) != traits.end();
    }

    void AdministrativeOfficial::AddTrait(OfficialTrait trait) {
        if (!HasTrait(trait)) {
            traits.push_back(trait);
        }
    }

    std::string AdministrativeOfficial::GetTraitDescription(OfficialTrait trait) const {
        switch (trait) {
            case OfficialTrait::CORRUPT: return "Corrupt";
            case OfficialTrait::EFFICIENT: return "Efficient";
            case OfficialTrait::LOYAL: return "Loyal";
            case OfficialTrait::AMBITIOUS: return "Ambitious";
            case OfficialTrait::EXPERIENCED: return "Experienced";
            case OfficialTrait::YOUNG_TALENT: return "Young Talent";
            case OfficialTrait::WELL_CONNECTED: return "Well-Connected";
            case OfficialTrait::STUBBORN: return "Stubborn";
            case OfficialTrait::SCHOLARLY: return "Scholarly";
            default: return "Unknown";
        }
    }

    Json::Value AdministrativeOfficial::ToJson() const {
        Json::Value json;
        json["official_id"] = official_id;
        json["name"] = name;
        json["type"] = static_cast<int>(type);
        json["assigned_province"] = static_cast<int>(assigned_province);
        json["competence"] = competence;
        json["loyalty"] = loyalty;
        json["efficiency"] = efficiency;
        json["corruption_resistance"] = corruption_resistance;
        json["age"] = age;
        json["months_in_position"] = months_in_position;
        json["satisfaction"] = satisfaction;
        json["salary_cost"] = salary_cost;
        json["administrative_effectiveness"] = administrative_effectiveness;
        json["corruption_suspicion"] = corruption_suspicion;
        json["has_pending_event"] = has_pending_event;
        
        Json::Value traits_array(Json::arrayValue);
        for (auto trait : traits) {
            traits_array.append(static_cast<int>(trait));
        }
        json["traits"] = traits_array;
        
        return json;
    }

    AdministrativeOfficial AdministrativeOfficial::FromJson(const Json::Value& data) {
        AdministrativeOfficial official;
        official.official_id = data["official_id"].asUInt();
        official.name = data["name"].asString();
        official.type = static_cast<OfficialType>(data["type"].asInt());
        official.assigned_province = data["assigned_province"].asUInt();
        official.competence = data["competence"].asDouble();
        official.loyalty = data["loyalty"].asDouble();
        official.efficiency = data["efficiency"].asDouble();
        official.corruption_resistance = data["corruption_resistance"].asDouble();
        official.age = data["age"].asUInt();
        official.months_in_position = data["months_in_position"].asUInt();
        official.satisfaction = data["satisfaction"].asDouble();
        official.salary_cost = data["salary_cost"].asDouble();
        official.administrative_effectiveness = data["administrative_effectiveness"].asDouble();
        official.corruption_suspicion = data["corruption_suspicion"].asUInt();
        official.has_pending_event = data["has_pending_event"].asBool();
        
        const Json::Value& traits_array = data["traits"];
        for (const auto& trait_val : traits_array) {
            official.traits.push_back(static_cast<OfficialTrait>(trait_val.asInt()));
        }
        
        return official;
    }

    AdministrativeOfficial AdministrativeOfficial::GenerateRandom(uint32_t id, OfficialType type, 
                                                                  game::types::EntityID province) {
        std::string name = GenerateRandomName();
        AdministrativeOfficial official(id, name, type, province);
        
        // Assign random traits (0-2 traits)
        int trait_count = utils::RandomGenerator::getInstance().randomInt(0, 100) % 3;
        for (int i = 0; i < trait_count; ++i) {
            int trait_idx = utils::RandomGenerator::getInstance().randomInt(0, 100) % 
                           (static_cast<int>(OfficialTrait::COUNT) - 1);
            official.AddTrait(static_cast<OfficialTrait>(trait_idx + 1)); // Skip NONE
        }
        
        return official;
    }

    std::string AdministrativeOfficial::GenerateRandomName() {
        static const std::vector<std::string> first_names = {
            "Marcus", "Julius", "Aurelius", "Constantine", "Hadrian", "Trajan",
            "William", "Henry", "Edward", "Richard", "Thomas", "Robert",
            "Giovanni", "Lorenzo", "Francesco", "Antonio", "Pietro", "Carlo",
            "Jean", "Pierre", "Louis", "Henri", "Charles", "Philippe"
        };
        
        static const std::vector<std::string> last_names = {
            "Antonius", "Claudius", "Flavius", "Valerius", "Aurelius",
            "of York", "of Lancaster", "of Chester", "of Kent", "of Sussex",
            "de' Medici", "Visconti", "Sforza", "Gonzaga", "Este",
            "de Valois", "de Bourbon", "de Montfort", "de Lusignan", "de Capet"
        };
        
        int first_idx = utils::RandomGenerator::getInstance().randomInt(0, 100) % first_names.size();
        int last_idx = utils::RandomGenerator::getInstance().randomInt(0, 100) % last_names.size();
        
        return first_names[first_idx] + " " + last_names[last_idx];
    }

    // ============================================================================
    // GovernanceComponent Methods
    // ============================================================================

    std::string GovernanceComponent::GetComponentTypeName() const {
        return "GovernanceComponent";
    }

    // ============================================================================
    // BureaucracyComponent Methods
    // ============================================================================

    std::string BureaucracyComponent::GetComponentTypeName() const {
        return "BureaucracyComponent";
    }

    // ============================================================================
    // LawComponent Methods
    // ============================================================================

    std::string LawComponent::GetComponentTypeName() const {
        return "LawComponent";
    }

    // ============================================================================
    // AdministrativeEventsComponent Methods
    // ============================================================================

    std::string AdministrativeEventsComponent::GetComponentTypeName() const {
        return "AdministrativeEventsComponent";
    }

} // namespace game::administration