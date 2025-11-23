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
                CORE_LOG_WARN("AdministrativeOfficial", 
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

    Json::Value GovernanceComponent::ToJson() const {
        Json::Value json;

        // Governance structure
        json["governance_type"] = static_cast<int>(governance_type);

        // Serialize appointed officials (thread-safe)
        Json::Value officials_array(Json::arrayValue);
        {
            std::lock_guard<std::mutex> lock(officials_mutex);
            for (const auto& official : appointed_officials) {
                officials_array.append(official.ToJson());
            }
        }
        json["appointed_officials"] = officials_array;

        // Administrative efficiency
        json["administrative_efficiency"] = administrative_efficiency;
        json["bureaucratic_capacity"] = bureaucratic_capacity;
        json["governance_stability"] = governance_stability;

        // Tax and revenue system
        json["tax_collection_efficiency"] = tax_collection_efficiency;
        json["tax_rate"] = tax_rate;
        json["total_tax_revenue"] = total_tax_revenue;

        Json::Value tax_sources_obj(Json::objectValue);
        for (const auto& [source, amount] : tax_sources) {
            tax_sources_obj[source] = amount;
        }
        json["tax_sources"] = tax_sources_obj;

        // Trade and economic administration
        json["trade_administration_efficiency"] = trade_administration_efficiency;
        json["customs_efficiency"] = customs_efficiency;
        json["market_regulation_level"] = market_regulation_level;

        // Military administration
        json["military_administration_efficiency"] = military_administration_efficiency;
        json["recruitment_administration"] = recruitment_administration;
        json["logistics_efficiency"] = logistics_efficiency;

        // Population administration
        json["population_administration_efficiency"] = population_administration_efficiency;
        json["census_accuracy"] = census_accuracy;
        json["public_order_maintenance"] = public_order_maintenance;

        // Administrative costs
        json["monthly_administrative_costs"] = monthly_administrative_costs;
        json["official_salaries"] = official_salaries;
        json["infrastructure_costs"] = infrastructure_costs;

        return json;
    }

    void GovernanceComponent::FromJson(const Json::Value& data) {
        // Governance structure
        if (data.isMember("governance_type")) {
            governance_type = static_cast<GovernanceType>(data["governance_type"].asInt());
        }

        // Deserialize appointed officials (thread-safe)
        if (data.isMember("appointed_officials")) {
            std::lock_guard<std::mutex> lock(officials_mutex);
            appointed_officials.clear();
            const Json::Value& officials_array = data["appointed_officials"];
            for (const auto& official_data : officials_array) {
                appointed_officials.push_back(AdministrativeOfficial::FromJson(official_data));
            }
        }

        // Administrative efficiency
        if (data.isMember("administrative_efficiency")) {
            administrative_efficiency = data["administrative_efficiency"].asDouble();
        }
        if (data.isMember("bureaucratic_capacity")) {
            bureaucratic_capacity = data["bureaucratic_capacity"].asDouble();
        }
        if (data.isMember("governance_stability")) {
            governance_stability = data["governance_stability"].asDouble();
        }

        // Tax and revenue system
        if (data.isMember("tax_collection_efficiency")) {
            tax_collection_efficiency = data["tax_collection_efficiency"].asDouble();
        }
        if (data.isMember("tax_rate")) {
            tax_rate = data["tax_rate"].asDouble();
        }
        if (data.isMember("total_tax_revenue")) {
            total_tax_revenue = data["total_tax_revenue"].asDouble();
        }

        if (data.isMember("tax_sources")) {
            tax_sources.clear();
            const Json::Value& tax_sources_obj = data["tax_sources"];
            for (const auto& key : tax_sources_obj.getMemberNames()) {
                tax_sources[key] = tax_sources_obj[key].asDouble();
            }
        }

        // Trade and economic administration
        if (data.isMember("trade_administration_efficiency")) {
            trade_administration_efficiency = data["trade_administration_efficiency"].asDouble();
        }
        if (data.isMember("customs_efficiency")) {
            customs_efficiency = data["customs_efficiency"].asDouble();
        }
        if (data.isMember("market_regulation_level")) {
            market_regulation_level = data["market_regulation_level"].asDouble();
        }

        // Military administration
        if (data.isMember("military_administration_efficiency")) {
            military_administration_efficiency = data["military_administration_efficiency"].asDouble();
        }
        if (data.isMember("recruitment_administration")) {
            recruitment_administration = data["recruitment_administration"].asDouble();
        }
        if (data.isMember("logistics_efficiency")) {
            logistics_efficiency = data["logistics_efficiency"].asDouble();
        }

        // Population administration
        if (data.isMember("population_administration_efficiency")) {
            population_administration_efficiency = data["population_administration_efficiency"].asDouble();
        }
        if (data.isMember("census_accuracy")) {
            census_accuracy = data["census_accuracy"].asDouble();
        }
        if (data.isMember("public_order_maintenance")) {
            public_order_maintenance = data["public_order_maintenance"].asDouble();
        }

        // Administrative costs
        if (data.isMember("monthly_administrative_costs")) {
            monthly_administrative_costs = data["monthly_administrative_costs"].asDouble();
        }
        if (data.isMember("official_salaries")) {
            official_salaries = data["official_salaries"].asDouble();
        }
        if (data.isMember("infrastructure_costs")) {
            infrastructure_costs = data["infrastructure_costs"].asDouble();
        }
    }

    // ============================================================================
    // BureaucracyComponent Methods
    // ============================================================================

    std::string BureaucracyComponent::GetComponentTypeName() const {
        return "BureaucracyComponent";
    }

    Json::Value BureaucracyComponent::ToJson() const {
        Json::Value json;

        // Bureaucratic structure
        json["bureaucracy_level"] = bureaucracy_level;
        json["scribes_employed"] = scribes_employed;
        json["clerks_employed"] = clerks_employed;
        json["administrators_employed"] = administrators_employed;

        // Record keeping and documentation
        json["record_keeping_quality"] = record_keeping_quality;
        json["document_accuracy"] = document_accuracy;
        json["administrative_speed"] = administrative_speed;

        // Bureaucratic processes
        Json::Value process_eff_obj(Json::objectValue);
        for (const auto& [process, efficiency] : process_efficiency) {
            process_eff_obj[process] = efficiency;
        }
        json["process_efficiency"] = process_eff_obj;

        Json::Value tasks_array(Json::arrayValue);
        for (const auto& task : active_administrative_tasks) {
            tasks_array.append(task);
        }
        json["active_administrative_tasks"] = tasks_array;

        Json::Value decisions_array(Json::arrayValue);
        for (const auto& decision : pending_decisions) {
            decisions_array.append(decision);
        }
        json["pending_decisions"] = decisions_array;

        // Information flow
        json["information_gathering_efficiency"] = information_gathering_efficiency;
        json["communication_speed"] = communication_speed;
        json["inter_provincial_coordination"] = inter_provincial_coordination;

        // Corruption and oversight
        json["corruption_level"] = corruption_level;
        json["oversight_effectiveness"] = oversight_effectiveness;
        json["audit_frequency"] = audit_frequency;

        // Innovation and reform
        json["administrative_innovation"] = administrative_innovation;

        Json::Value reforms_array(Json::arrayValue);
        for (const auto& reform : recent_reforms) {
            reforms_array.append(reform);
        }
        json["recent_reforms"] = reforms_array;

        Json::Value improvements_array(Json::arrayValue);
        for (const auto& improvement : planned_improvements) {
            improvements_array.append(improvement);
        }
        json["planned_improvements"] = improvements_array;

        // Performance metrics
        json["citizen_satisfaction_with_services"] = citizen_satisfaction_with_services;
        json["administrative_response_time"] = administrative_response_time;
        json["documents_processed_monthly"] = documents_processed_monthly;

        return json;
    }

    void BureaucracyComponent::FromJson(const Json::Value& data) {
        // Bureaucratic structure
        if (data.isMember("bureaucracy_level")) {
            bureaucracy_level = data["bureaucracy_level"].asUInt();
        }
        if (data.isMember("scribes_employed")) {
            scribes_employed = data["scribes_employed"].asUInt();
        }
        if (data.isMember("clerks_employed")) {
            clerks_employed = data["clerks_employed"].asUInt();
        }
        if (data.isMember("administrators_employed")) {
            administrators_employed = data["administrators_employed"].asUInt();
        }

        // Record keeping and documentation
        if (data.isMember("record_keeping_quality")) {
            record_keeping_quality = data["record_keeping_quality"].asDouble();
        }
        if (data.isMember("document_accuracy")) {
            document_accuracy = data["document_accuracy"].asDouble();
        }
        if (data.isMember("administrative_speed")) {
            administrative_speed = data["administrative_speed"].asDouble();
        }

        // Bureaucratic processes
        if (data.isMember("process_efficiency")) {
            process_efficiency.clear();
            const Json::Value& process_eff_obj = data["process_efficiency"];
            for (const auto& key : process_eff_obj.getMemberNames()) {
                process_efficiency[key] = process_eff_obj[key].asDouble();
            }
        }

        if (data.isMember("active_administrative_tasks")) {
            active_administrative_tasks.clear();
            const Json::Value& tasks_array = data["active_administrative_tasks"];
            for (const auto& task : tasks_array) {
                active_administrative_tasks.push_back(task.asString());
            }
        }

        if (data.isMember("pending_decisions")) {
            pending_decisions.clear();
            const Json::Value& decisions_array = data["pending_decisions"];
            for (const auto& decision : decisions_array) {
                pending_decisions.push_back(decision.asString());
            }
        }

        // Information flow
        if (data.isMember("information_gathering_efficiency")) {
            information_gathering_efficiency = data["information_gathering_efficiency"].asDouble();
        }
        if (data.isMember("communication_speed")) {
            communication_speed = data["communication_speed"].asDouble();
        }
        if (data.isMember("inter_provincial_coordination")) {
            inter_provincial_coordination = data["inter_provincial_coordination"].asDouble();
        }

        // Corruption and oversight
        if (data.isMember("corruption_level")) {
            corruption_level = data["corruption_level"].asDouble();
        }
        if (data.isMember("oversight_effectiveness")) {
            oversight_effectiveness = data["oversight_effectiveness"].asDouble();
        }
        if (data.isMember("audit_frequency")) {
            audit_frequency = data["audit_frequency"].asDouble();
        }

        // Innovation and reform
        if (data.isMember("administrative_innovation")) {
            administrative_innovation = data["administrative_innovation"].asDouble();
        }

        if (data.isMember("recent_reforms")) {
            recent_reforms.clear();
            const Json::Value& reforms_array = data["recent_reforms"];
            for (const auto& reform : reforms_array) {
                recent_reforms.push_back(reform.asString());
            }
        }

        if (data.isMember("planned_improvements")) {
            planned_improvements.clear();
            const Json::Value& improvements_array = data["planned_improvements"];
            for (const auto& improvement : improvements_array) {
                planned_improvements.push_back(improvement.asString());
            }
        }

        // Performance metrics
        if (data.isMember("citizen_satisfaction_with_services")) {
            citizen_satisfaction_with_services = data["citizen_satisfaction_with_services"].asDouble();
        }
        if (data.isMember("administrative_response_time")) {
            administrative_response_time = data["administrative_response_time"].asDouble();
        }
        if (data.isMember("documents_processed_monthly")) {
            documents_processed_monthly = data["documents_processed_monthly"].asUInt();
        }
    }

    // ============================================================================
    // LawComponent Methods
    // ============================================================================

    std::string LawComponent::GetComponentTypeName() const {
        return "LawComponent";
    }

    Json::Value LawComponent::ToJson() const {
        Json::Value json;

        // Legal system structure
        json["primary_law_system"] = static_cast<int>(primary_law_system);

        Json::Value secondary_laws_array(Json::arrayValue);
        for (const auto& law_type : secondary_law_systems) {
            secondary_laws_array.append(static_cast<int>(law_type));
        }
        json["secondary_law_systems"] = secondary_laws_array;

        // Law enforcement
        json["law_enforcement_effectiveness"] = law_enforcement_effectiveness;
        json["judges_appointed"] = judges_appointed;
        json["bailiffs_employed"] = bailiffs_employed;
        json["courts_established"] = courts_established;

        // Legal processes
        json["legal_process_speed"] = legal_process_speed;
        json["justice_fairness"] = justice_fairness;
        json["legal_accessibility"] = legal_accessibility;

        // Crime and punishment
        json["crime_rate"] = crime_rate;

        Json::Value crime_types_obj(Json::objectValue);
        for (const auto& [crime, rate] : crime_types) {
            crime_types_obj[crime] = rate;
        }
        json["crime_types"] = crime_types_obj;

        Json::Value punishment_types_obj(Json::objectValue);
        for (const auto& [crime, punishment] : punishment_types) {
            punishment_types_obj[crime] = punishment;
        }
        json["punishment_types"] = punishment_types_obj;

        // Legal codes and regulations
        Json::Value active_laws_array(Json::arrayValue);
        for (const auto& law : active_laws) {
            active_laws_array.append(law);
        }
        json["active_laws"] = active_laws_array;

        Json::Value precedents_array(Json::arrayValue);
        for (const auto& precedent : legal_precedents) {
            precedents_array.append(precedent);
        }
        json["legal_precedents"] = precedents_array;

        Json::Value pending_legislation_array(Json::arrayValue);
        for (const auto& legislation : pending_legislation) {
            pending_legislation_array.append(legislation);
        }
        json["pending_legislation"] = pending_legislation_array;

        // Court system
        json["cases_pending"] = cases_pending;
        json["cases_resolved_monthly"] = cases_resolved_monthly;
        json["court_backlog_pressure"] = court_backlog_pressure;

        // Legal expertise
        json["legal_scholarship_level"] = legal_scholarship_level;
        json["legal_scholars"] = legal_scholars;

        Json::Value specializations_array(Json::arrayValue);
        for (const auto& specialization : legal_specializations) {
            specializations_array.append(specialization);
        }
        json["legal_specializations"] = specializations_array;

        // Social order
        json["public_order"] = public_order;
        json["legal_compliance"] = legal_compliance;
        json["respect_for_authority"] = respect_for_authority;

        return json;
    }

    void LawComponent::FromJson(const Json::Value& data) {
        // Legal system structure
        if (data.isMember("primary_law_system")) {
            primary_law_system = static_cast<LawType>(data["primary_law_system"].asInt());
        }

        if (data.isMember("secondary_law_systems")) {
            secondary_law_systems.clear();
            const Json::Value& secondary_laws_array = data["secondary_law_systems"];
            for (const auto& law_type : secondary_laws_array) {
                secondary_law_systems.push_back(static_cast<LawType>(law_type.asInt()));
            }
        }

        // Law enforcement
        if (data.isMember("law_enforcement_effectiveness")) {
            law_enforcement_effectiveness = data["law_enforcement_effectiveness"].asDouble();
        }
        if (data.isMember("judges_appointed")) {
            judges_appointed = data["judges_appointed"].asUInt();
        }
        if (data.isMember("bailiffs_employed")) {
            bailiffs_employed = data["bailiffs_employed"].asUInt();
        }
        if (data.isMember("courts_established")) {
            courts_established = data["courts_established"].asUInt();
        }

        // Legal processes
        if (data.isMember("legal_process_speed")) {
            legal_process_speed = data["legal_process_speed"].asDouble();
        }
        if (data.isMember("justice_fairness")) {
            justice_fairness = data["justice_fairness"].asDouble();
        }
        if (data.isMember("legal_accessibility")) {
            legal_accessibility = data["legal_accessibility"].asDouble();
        }

        // Crime and punishment
        if (data.isMember("crime_rate")) {
            crime_rate = data["crime_rate"].asDouble();
        }

        if (data.isMember("crime_types")) {
            crime_types.clear();
            const Json::Value& crime_types_obj = data["crime_types"];
            for (const auto& key : crime_types_obj.getMemberNames()) {
                crime_types[key] = crime_types_obj[key].asDouble();
            }
        }

        if (data.isMember("punishment_types")) {
            punishment_types.clear();
            const Json::Value& punishment_types_obj = data["punishment_types"];
            for (const auto& key : punishment_types_obj.getMemberNames()) {
                punishment_types[key] = punishment_types_obj[key].asString();
            }
        }

        // Legal codes and regulations
        if (data.isMember("active_laws")) {
            active_laws.clear();
            const Json::Value& active_laws_array = data["active_laws"];
            for (const auto& law : active_laws_array) {
                active_laws.push_back(law.asString());
            }
        }

        if (data.isMember("legal_precedents")) {
            legal_precedents.clear();
            const Json::Value& precedents_array = data["legal_precedents"];
            for (const auto& precedent : precedents_array) {
                legal_precedents.push_back(precedent.asString());
            }
        }

        if (data.isMember("pending_legislation")) {
            pending_legislation.clear();
            const Json::Value& pending_legislation_array = data["pending_legislation"];
            for (const auto& legislation : pending_legislation_array) {
                pending_legislation.push_back(legislation.asString());
            }
        }

        // Court system
        if (data.isMember("cases_pending")) {
            cases_pending = data["cases_pending"].asUInt();
        }
        if (data.isMember("cases_resolved_monthly")) {
            cases_resolved_monthly = data["cases_resolved_monthly"].asUInt();
        }
        if (data.isMember("court_backlog_pressure")) {
            court_backlog_pressure = data["court_backlog_pressure"].asDouble();
        }

        // Legal expertise
        if (data.isMember("legal_scholarship_level")) {
            legal_scholarship_level = data["legal_scholarship_level"].asDouble();
        }
        if (data.isMember("legal_scholars")) {
            legal_scholars = data["legal_scholars"].asUInt();
        }

        if (data.isMember("legal_specializations")) {
            legal_specializations.clear();
            const Json::Value& specializations_array = data["legal_specializations"];
            for (const auto& specialization : specializations_array) {
                legal_specializations.push_back(specialization.asString());
            }
        }

        // Social order
        if (data.isMember("public_order")) {
            public_order = data["public_order"].asDouble();
        }
        if (data.isMember("legal_compliance")) {
            legal_compliance = data["legal_compliance"].asDouble();
        }
        if (data.isMember("respect_for_authority")) {
            respect_for_authority = data["respect_for_authority"].asDouble();
        }
    }

    // ============================================================================
    // AdministrativeEventsComponent Methods
    // ============================================================================

    std::string AdministrativeEventsComponent::GetComponentTypeName() const {
        return "AdministrativeEventsComponent";
    }

    Json::Value AdministrativeEventsComponent::ToJson() const {
        Json::Value json;

        // Active administrative events
        Json::Value appointments_array(Json::arrayValue);
        for (const auto& appointment : active_appointments) {
            appointments_array.append(appointment);
        }
        json["active_appointments"] = appointments_array;

        Json::Value dismissals_array(Json::arrayValue);
        for (const auto& dismissal : pending_dismissals) {
            dismissals_array.append(dismissal);
        }
        json["pending_dismissals"] = dismissals_array;

        Json::Value investigations_array(Json::arrayValue);
        for (const auto& investigation : corruption_investigations) {
            investigations_array.append(investigation);
        }
        json["corruption_investigations"] = investigations_array;

        // Official events
        Json::Value promotions_array(Json::arrayValue);
        for (const auto& promotion : official_promotions) {
            promotions_array.append(promotion);
        }
        json["official_promotions"] = promotions_array;

        Json::Value scandals_array(Json::arrayValue);
        for (const auto& scandal : official_scandals) {
            scandals_array.append(scandal);
        }
        json["official_scandals"] = scandals_array;

        Json::Value reviews_array(Json::arrayValue);
        for (const auto& review : performance_reviews) {
            reviews_array.append(review);
        }
        json["performance_reviews"] = reviews_array;

        // Policy events
        Json::Value policy_changes_array(Json::arrayValue);
        for (const auto& change : policy_changes) {
            policy_changes_array.append(change);
        }
        json["policy_changes"] = policy_changes_array;

        Json::Value reforms_array(Json::arrayValue);
        for (const auto& reform : reform_initiatives) {
            reforms_array.append(reform);
        }
        json["reform_initiatives"] = reforms_array;

        Json::Value legislation_array(Json::arrayValue);
        for (const auto& proposal : legislative_proposals) {
            legislation_array.append(proposal);
        }
        json["legislative_proposals"] = legislation_array;

        // Administrative crises
        Json::Value failures_array(Json::arrayValue);
        for (const auto& failure : bureaucratic_failures) {
            failures_array.append(failure);
        }
        json["bureaucratic_failures"] = failures_array;

        Json::Value delays_array(Json::arrayValue);
        for (const auto& delay : administrative_delays) {
            delays_array.append(delay);
        }
        json["administrative_delays"] = delays_array;

        Json::Value conflicts_array(Json::arrayValue);
        for (const auto& conflict : inter_departmental_conflicts) {
            conflicts_array.append(conflict);
        }
        json["inter_departmental_conflicts"] = conflicts_array;

        // Public relations
        Json::Value announcements_array(Json::arrayValue);
        for (const auto& announcement : public_announcements) {
            announcements_array.append(announcement);
        }
        json["public_announcements"] = announcements_array;

        Json::Value complaints_array(Json::arrayValue);
        for (const auto& complaint : citizen_complaints) {
            complaints_array.append(complaint);
        }
        json["citizen_complaints"] = complaints_array;

        Json::Value communications_array(Json::arrayValue);
        for (const auto& communication : diplomatic_communications) {
            communications_array.append(communication);
        }
        json["diplomatic_communications"] = communications_array;

        // Event frequency and timing
        json["event_frequency_modifier"] = event_frequency_modifier;
        json["months_since_last_appointment"] = months_since_last_appointment;
        json["months_since_last_reform"] = months_since_last_reform;

        // Administrative reputation
        json["administrative_reputation"] = administrative_reputation;
        json["government_legitimacy"] = government_legitimacy;
        json["public_trust"] = public_trust;

        // Decision tracking
        Json::Value pending_decisions_array(Json::arrayValue);
        for (const auto& decision : pending_decisions) {
            pending_decisions_array.append(static_cast<int>(decision));
        }
        json["pending_decisions"] = pending_decisions_array;

        Json::Value policy_decisions_array(Json::arrayValue);
        for (const auto& decision : recent_policy_decisions) {
            policy_decisions_array.append(decision);
        }
        json["recent_policy_decisions"] = policy_decisions_array;

        // Maximum history tracking
        json["max_history_size"] = max_history_size;

        return json;
    }

    void AdministrativeEventsComponent::FromJson(const Json::Value& data) {
        // Active administrative events
        if (data.isMember("active_appointments")) {
            active_appointments.clear();
            const Json::Value& appointments_array = data["active_appointments"];
            for (const auto& appointment : appointments_array) {
                active_appointments.push_back(appointment.asString());
            }
        }

        if (data.isMember("pending_dismissals")) {
            pending_dismissals.clear();
            const Json::Value& dismissals_array = data["pending_dismissals"];
            for (const auto& dismissal : dismissals_array) {
                pending_dismissals.push_back(dismissal.asString());
            }
        }

        if (data.isMember("corruption_investigations")) {
            corruption_investigations.clear();
            const Json::Value& investigations_array = data["corruption_investigations"];
            for (const auto& investigation : investigations_array) {
                corruption_investigations.push_back(investigation.asString());
            }
        }

        // Official events
        if (data.isMember("official_promotions")) {
            official_promotions.clear();
            const Json::Value& promotions_array = data["official_promotions"];
            for (const auto& promotion : promotions_array) {
                official_promotions.push_back(promotion.asString());
            }
        }

        if (data.isMember("official_scandals")) {
            official_scandals.clear();
            const Json::Value& scandals_array = data["official_scandals"];
            for (const auto& scandal : scandals_array) {
                official_scandals.push_back(scandal.asString());
            }
        }

        if (data.isMember("performance_reviews")) {
            performance_reviews.clear();
            const Json::Value& reviews_array = data["performance_reviews"];
            for (const auto& review : reviews_array) {
                performance_reviews.push_back(review.asString());
            }
        }

        // Policy events
        if (data.isMember("policy_changes")) {
            policy_changes.clear();
            const Json::Value& policy_changes_array = data["policy_changes"];
            for (const auto& change : policy_changes_array) {
                policy_changes.push_back(change.asString());
            }
        }

        if (data.isMember("reform_initiatives")) {
            reform_initiatives.clear();
            const Json::Value& reforms_array = data["reform_initiatives"];
            for (const auto& reform : reforms_array) {
                reform_initiatives.push_back(reform.asString());
            }
        }

        if (data.isMember("legislative_proposals")) {
            legislative_proposals.clear();
            const Json::Value& legislation_array = data["legislative_proposals"];
            for (const auto& proposal : legislation_array) {
                legislative_proposals.push_back(proposal.asString());
            }
        }

        // Administrative crises
        if (data.isMember("bureaucratic_failures")) {
            bureaucratic_failures.clear();
            const Json::Value& failures_array = data["bureaucratic_failures"];
            for (const auto& failure : failures_array) {
                bureaucratic_failures.push_back(failure.asString());
            }
        }

        if (data.isMember("administrative_delays")) {
            administrative_delays.clear();
            const Json::Value& delays_array = data["administrative_delays"];
            for (const auto& delay : delays_array) {
                administrative_delays.push_back(delay.asString());
            }
        }

        if (data.isMember("inter_departmental_conflicts")) {
            inter_departmental_conflicts.clear();
            const Json::Value& conflicts_array = data["inter_departmental_conflicts"];
            for (const auto& conflict : conflicts_array) {
                inter_departmental_conflicts.push_back(conflict.asString());
            }
        }

        // Public relations
        if (data.isMember("public_announcements")) {
            public_announcements.clear();
            const Json::Value& announcements_array = data["public_announcements"];
            for (const auto& announcement : announcements_array) {
                public_announcements.push_back(announcement.asString());
            }
        }

        if (data.isMember("citizen_complaints")) {
            citizen_complaints.clear();
            const Json::Value& complaints_array = data["citizen_complaints"];
            for (const auto& complaint : complaints_array) {
                citizen_complaints.push_back(complaint.asString());
            }
        }

        if (data.isMember("diplomatic_communications")) {
            diplomatic_communications.clear();
            const Json::Value& communications_array = data["diplomatic_communications"];
            for (const auto& communication : communications_array) {
                diplomatic_communications.push_back(communication.asString());
            }
        }

        // Event frequency and timing
        if (data.isMember("event_frequency_modifier")) {
            event_frequency_modifier = data["event_frequency_modifier"].asDouble();
        }
        if (data.isMember("months_since_last_appointment")) {
            months_since_last_appointment = data["months_since_last_appointment"].asUInt();
        }
        if (data.isMember("months_since_last_reform")) {
            months_since_last_reform = data["months_since_last_reform"].asUInt();
        }

        // Administrative reputation
        if (data.isMember("administrative_reputation")) {
            administrative_reputation = data["administrative_reputation"].asDouble();
        }
        if (data.isMember("government_legitimacy")) {
            government_legitimacy = data["government_legitimacy"].asDouble();
        }
        if (data.isMember("public_trust")) {
            public_trust = data["public_trust"].asDouble();
        }

        // Decision tracking
        if (data.isMember("pending_decisions")) {
            pending_decisions.clear();
            const Json::Value& pending_decisions_array = data["pending_decisions"];
            for (const auto& decision : pending_decisions_array) {
                pending_decisions.push_back(static_cast<game::types::EntityID>(decision.asInt()));
            }
        }

        if (data.isMember("recent_policy_decisions")) {
            recent_policy_decisions.clear();
            const Json::Value& policy_decisions_array = data["recent_policy_decisions"];
            for (const auto& decision : policy_decisions_array) {
                recent_policy_decisions.push_back(decision.asString());
            }
        }

        // Maximum history tracking
        if (data.isMember("max_history_size")) {
            max_history_size = data["max_history_size"].asUInt();
        }
    }

} // namespace game::administration