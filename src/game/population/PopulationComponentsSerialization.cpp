// Created: December 5, 2025
// Location: src/game/population/PopulationComponentsSerialization.cpp
// Purpose: Population system components serialization (Phase 7)

#include "game/population/PopulationComponents.h"
#include "game/population/PopulationTypes.h"
#include "game/population/PopulationEvents.h"
#include "core/save/SerializationConstants.h"
#include <json/json.h>
#include <sstream>

namespace game::population {

// =============================================================================
// Helper: Serialize PopulationGroup
// =============================================================================

static Json::Value SerializePopulationGroup(const PopulationGroup& group) {
    Json::Value data;

    // Identity
    data["social_class"] = static_cast<int>(group.social_class);
    data["legal_status"] = static_cast<int>(group.legal_status);
    data["culture"] = group.culture;
    data["religion"] = group.religion;

    // Basic demographics
    data["population_count"] = group.population_count;
    data["happiness"] = group.happiness;
    data["literacy_rate"] = group.literacy_rate;
    data["wealth_per_capita"] = group.wealth_per_capita;
    data["health_level"] = group.health_level;

    // Age and gender structure
    data["children_0_14"] = group.children_0_14;
    data["adults_15_64"] = group.adults_15_64;
    data["elderly_65_plus"] = group.elderly_65_plus;
    data["males"] = group.males;
    data["females"] = group.females;

    // Employment distribution (map)
    Json::Value employment_obj(Json::objectValue);
    for (const auto& [emp_type, count] : group.employment) {
        employment_obj[std::to_string(static_cast<int>(emp_type))] = count;
    }
    data["employment"] = employment_obj;
    data["employment_rate"] = group.employment_rate;

    // Demographic rates
    data["birth_rate"] = group.birth_rate;
    data["death_rate"] = group.death_rate;
    data["infant_mortality"] = group.infant_mortality;
    data["maternal_mortality"] = group.maternal_mortality;
    data["migration_tendency"] = group.migration_tendency;

    // Cultural and social factors
    data["assimilation_rate"] = group.assimilation_rate;
    data["conversion_rate"] = group.conversion_rate;
    data["education_access"] = group.education_access;
    data["social_mobility"] = group.social_mobility;

    // Economic factors
    data["taxation_burden"] = group.taxation_burden;
    data["feudal_obligations"] = group.feudal_obligations;
    data["guild_membership_rate"] = group.guild_membership_rate;

    // Military and service potential
    data["military_eligible"] = group.military_eligible;
    data["military_quality"] = group.military_quality;
    data["military_service_obligation"] = group.military_service_obligation;

    // Legal and social attributes (vectors)
    Json::Value privileges(Json::arrayValue);
    for (const auto& priv : group.legal_privileges) {
        privileges.append(priv);
    }
    data["legal_privileges"] = privileges;

    Json::Value rights(Json::arrayValue);
    for (const auto& right : group.economic_rights) {
        rights.append(right);
    }
    data["economic_rights"] = rights;

    Json::Value restrictions(Json::arrayValue);
    for (const auto& restriction : group.social_restrictions) {
        restrictions.append(restriction);
    }
    data["social_restrictions"] = restrictions;

    // Family structure
    data["average_household_size"] = group.average_household_size;
    data["extended_family_rate"] = group.extended_family_rate;
    data["servant_employment_rate"] = group.servant_employment_rate;

    return data;
}

// =============================================================================
// Helper: Deserialize PopulationGroup
// =============================================================================

static PopulationGroup DeserializePopulationGroup(const Json::Value& data) {
    PopulationGroup group;

    // Identity
    if (data.isMember("social_class")) {
        int val = data["social_class"].asInt();
        if (val >= 0 && val < static_cast<int>(SocialClass::COUNT)) {
            group.social_class = static_cast<SocialClass>(val);
        }
    }
    if (data.isMember("legal_status")) {
        int val = data["legal_status"].asInt();
        if (val >= 0 && val < static_cast<int>(LegalStatus::COUNT)) {
            group.legal_status = static_cast<LegalStatus>(val);
        }
    }
    if (data.isMember("culture")) group.culture = data["culture"].asString();
    if (data.isMember("religion")) group.religion = data["religion"].asString();

    // Use centralized constants
    using game::core::serialization::Clamp;
    using game::core::serialization::MIN_POPULATION;
    using game::core::serialization::MAX_POPULATION_GROUP_SIZE;
    using game::core::serialization::MIN_RATE;
    using game::core::serialization::MAX_RATE;
    using game::core::serialization::MIN_DEMOGRAPHIC_RATE;
    using game::core::serialization::MAX_DEMOGRAPHIC_RATE;
    using game::core::serialization::MIN_WEALTH;
    using game::core::serialization::MAX_WEALTH_PER_CAPITA;

    // Basic demographics with bounds checking
    if (data.isMember("population_count")) {
        group.population_count = Clamp(data["population_count"].asInt(),
            MIN_POPULATION, MAX_POPULATION_GROUP_SIZE);
    }
    if (data.isMember("happiness")) {
        group.happiness = Clamp(data["happiness"].asDouble(), MIN_RATE, MAX_RATE);
    }
    if (data.isMember("literacy_rate")) {
        group.literacy_rate = Clamp(data["literacy_rate"].asDouble(), MIN_RATE, MAX_RATE);
    }
    if (data.isMember("wealth_per_capita")) {
        group.wealth_per_capita = Clamp(data["wealth_per_capita"].asDouble(),
            MIN_WEALTH, MAX_WEALTH_PER_CAPITA);
    }
    if (data.isMember("health_level")) {
        group.health_level = Clamp(data["health_level"].asDouble(), MIN_RATE, MAX_RATE);
    }

    // Age and gender structure
    if (data.isMember("children_0_14")) group.children_0_14 = data["children_0_14"].asInt();
    if (data.isMember("adults_15_64")) group.adults_15_64 = data["adults_15_64"].asInt();
    if (data.isMember("elderly_65_plus")) group.elderly_65_plus = data["elderly_65_plus"].asInt();
    if (data.isMember("males")) group.males = data["males"].asInt();
    if (data.isMember("females")) group.females = data["females"].asInt();

    // Employment distribution with size limit
    if (data.isMember("employment") && data["employment"].isObject()) {
        const Json::Value& emp_obj = data["employment"];
        auto member_names = emp_obj.getMemberNames();

        // Limit to prevent DoS
        size_t max_types = std::min(member_names.size(),
            static_cast<size_t>(game::core::serialization::MAX_EMPLOYMENT_TYPES));

        for (size_t i = 0; i < max_types; ++i) {
            const auto& key = member_names[i];
            int emp_type_int = std::stoi(key);
            if (emp_type_int >= 0 && emp_type_int < static_cast<int>(EmploymentType::COUNT)) {
                EmploymentType emp_type = static_cast<EmploymentType>(emp_type_int);
                group.employment[emp_type] = emp_obj[key].asInt();
            }
        }
    }
    if (data.isMember("employment_rate")) {
        group.employment_rate = Clamp(data["employment_rate"].asDouble(), MIN_RATE, MAX_RATE);
    }

    // Demographic rates with bounds checking
    if (data.isMember("birth_rate")) {
        group.birth_rate = Clamp(data["birth_rate"].asDouble(), MIN_DEMOGRAPHIC_RATE, MAX_DEMOGRAPHIC_RATE);
    }
    if (data.isMember("death_rate")) {
        group.death_rate = Clamp(data["death_rate"].asDouble(), MIN_DEMOGRAPHIC_RATE, MAX_DEMOGRAPHIC_RATE);
    }
    if (data.isMember("infant_mortality")) {
        group.infant_mortality = Clamp(data["infant_mortality"].asDouble(), MIN_DEMOGRAPHIC_RATE, MAX_DEMOGRAPHIC_RATE);
    }
    if (data.isMember("maternal_mortality")) {
        group.maternal_mortality = Clamp(data["maternal_mortality"].asDouble(), MIN_DEMOGRAPHIC_RATE, MAX_DEMOGRAPHIC_RATE);
    }
    if (data.isMember("migration_tendency")) {
        group.migration_tendency = Clamp(data["migration_tendency"].asDouble(), MIN_RATE, MAX_RATE);
    }

    // Cultural and social factors with bounds checking
    if (data.isMember("assimilation_rate")) {
        group.assimilation_rate = Clamp(data["assimilation_rate"].asDouble(), MIN_RATE, MAX_RATE);
    }
    if (data.isMember("conversion_rate")) {
        group.conversion_rate = Clamp(data["conversion_rate"].asDouble(), MIN_RATE, MAX_RATE);
    }
    if (data.isMember("education_access")) {
        group.education_access = Clamp(data["education_access"].asDouble(), MIN_RATE, MAX_RATE);
    }
    if (data.isMember("social_mobility")) {
        group.social_mobility = Clamp(data["social_mobility"].asDouble(), MIN_RATE, MAX_RATE);
    }

    // Economic factors with bounds checking
    if (data.isMember("taxation_burden")) {
        group.taxation_burden = Clamp(data["taxation_burden"].asDouble(), MIN_RATE, MAX_RATE);
    }
    if (data.isMember("feudal_obligations")) {
        group.feudal_obligations = Clamp(data["feudal_obligations"].asDouble(), MIN_RATE, MAX_RATE);
    }
    if (data.isMember("guild_membership_rate")) {
        group.guild_membership_rate = Clamp(data["guild_membership_rate"].asDouble(), MIN_RATE, MAX_RATE);
    }

    // Military and service potential with bounds checking
    if (data.isMember("military_eligible")) {
        group.military_eligible = Clamp(data["military_eligible"].asInt(), 0, MAX_POPULATION_GROUP_SIZE);
    }
    if (data.isMember("military_quality")) {
        group.military_quality = Clamp(data["military_quality"].asDouble(), MIN_RATE, MAX_RATE);
    }
    if (data.isMember("military_service_obligation")) {
        group.military_service_obligation = Clamp(data["military_service_obligation"].asInt(), 0, MAX_POPULATION_GROUP_SIZE);
    }

    // Legal and social attributes
    if (data.isMember("legal_privileges") && data["legal_privileges"].isArray()) {
        for (const auto& priv : data["legal_privileges"]) {
            if (priv.isString()) group.legal_privileges.push_back(priv.asString());
        }
    }
    if (data.isMember("economic_rights") && data["economic_rights"].isArray()) {
        for (const auto& right : data["economic_rights"]) {
            if (right.isString()) group.economic_rights.push_back(right.asString());
        }
    }
    if (data.isMember("social_restrictions") && data["social_restrictions"].isArray()) {
        for (const auto& restriction : data["social_restrictions"]) {
            if (restriction.isString()) group.social_restrictions.push_back(restriction.asString());
        }
    }

    // Family structure with bounds checking
    if (data.isMember("average_household_size")) {
        group.average_household_size = Clamp(data["average_household_size"].asDouble(),
            game::core::serialization::MIN_HOUSEHOLD_SIZE,
            game::core::serialization::MAX_HOUSEHOLD_SIZE);
    }
    if (data.isMember("extended_family_rate")) {
        group.extended_family_rate = Clamp(data["extended_family_rate"].asDouble(), MIN_RATE, MAX_RATE);
    }
    if (data.isMember("servant_employment_rate")) {
        group.servant_employment_rate = Clamp(data["servant_employment_rate"].asDouble(), MIN_RATE, MAX_RATE);
    }

    return group;
}

// =============================================================================
// PopulationComponent Serialization
// =============================================================================

std::string PopulationComponent::Serialize() const {
    Json::Value data;

    // Schema version for future migration support
    data["schema_version"] = game::core::serialization::POPULATION_COMPONENT_VERSION;

    // Serialize population groups array
    Json::Value groups_array(Json::arrayValue);
    for (const auto& group : population_groups) {
        groups_array.append(SerializePopulationGroup(group));
    }
    data["population_groups"] = groups_array;

    // Aggregate statistics
    data["total_population"] = total_population;
    data["total_children"] = total_children;
    data["total_adults"] = total_adults;
    data["total_elderly"] = total_elderly;
    data["total_males"] = total_males;
    data["total_females"] = total_females;

    data["average_happiness"] = average_happiness;
    data["average_literacy"] = average_literacy;
    data["average_wealth"] = average_wealth;
    data["average_health"] = average_health;
    data["overall_employment_rate"] = overall_employment_rate;

    data["total_military_eligible"] = total_military_eligible;
    data["average_military_quality"] = average_military_quality;
    data["total_military_service_obligation"] = total_military_service_obligation;

    // Distribution maps (culture, religion, class, legal status)
    Json::Value culture_dist(Json::objectValue);
    for (const auto& [culture, count] : culture_distribution) {
        culture_dist[culture] = count;
    }
    data["culture_distribution"] = culture_dist;

    Json::Value religion_dist(Json::objectValue);
    for (const auto& [religion, count] : religion_distribution) {
        religion_dist[religion] = count;
    }
    data["religion_distribution"] = religion_dist;

    Json::Value class_dist(Json::objectValue);
    for (const auto& [social_class, count] : class_distribution) {
        class_dist[std::to_string(static_cast<int>(social_class))] = count;
    }
    data["class_distribution"] = class_dist;

    Json::Value legal_dist(Json::objectValue);
    for (const auto& [legal_status, count] : legal_status_distribution) {
        legal_dist[std::to_string(static_cast<int>(legal_status))] = count;
    }
    data["legal_status_distribution"] = legal_dist;

    Json::Value employment_dist(Json::objectValue);
    for (const auto& [emp_type, count] : total_employment) {
        employment_dist[std::to_string(static_cast<int>(emp_type))] = count;
    }
    data["total_employment"] = employment_dist;

    // Employment categories
    data["productive_workers"] = productive_workers;
    data["non_productive_income"] = non_productive_income;
    data["unemployed_seeking"] = unemployed_seeking;
    data["unemployable"] = unemployable;
    data["dependents"] = dependents;

    // Economic and social metrics
    data["total_tax_revenue_potential"] = total_tax_revenue_potential;
    data["total_feudal_service_days"] = total_feudal_service_days;
    data["guild_membership_percentage"] = guild_membership_percentage;
    data["social_mobility_average"] = social_mobility_average;
    data["cultural_assimilation_rate"] = cultural_assimilation_rate;
    data["religious_conversion_rate"] = religious_conversion_rate;
    data["inter_class_tension"] = inter_class_tension;

    // Demographic metrics
    data["population_density"] = population_density;
    data["growth_rate"] = growth_rate;
    data["birth_rate_average"] = birth_rate_average;
    data["death_rate_average"] = death_rate_average;
    data["migration_net_rate"] = migration_net_rate;

    // Note: last_update (time_point) and historical_events (vector) are skipped for now
    // as they are transient/runtime data

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, data);
}

bool PopulationComponent::Deserialize(const std::string& json_str) {
    Json::Value data;
    Json::CharReaderBuilder builder;
    std::stringstream ss(json_str);
    std::string errors;

    if (!Json::parseFromStream(builder, ss, &data, &errors)) {
        return false;
    }

    // Check schema version
    if (data.isMember("schema_version")) {
        int version = data["schema_version"].asInt();
        if (version > game::core::serialization::POPULATION_COMPONENT_VERSION) {
            // Future: handle migration from older versions
        }
    }

    // Deserialize population groups with count limit
    if (data.isMember("population_groups") && data["population_groups"].isArray()) {
        population_groups.clear();
        const Json::Value& groups_array = data["population_groups"];

        size_t max_groups = std::min(groups_array.size(),
            static_cast<size_t>(game::core::serialization::MAX_POPULATION_GROUPS_PER_PROVINCE));

        for (Json::ArrayIndex i = 0; i < max_groups; ++i) {
            population_groups.push_back(DeserializePopulationGroup(groups_array[i]));
        }
    }

    // Use centralized constants for aggregate validation
    using game::core::serialization::Clamp;
    using game::core::serialization::MIN_POPULATION;
    using game::core::serialization::MAX_PROVINCE_POPULATION;
    using game::core::serialization::MIN_RATE;
    using game::core::serialization::MAX_RATE;

    // Aggregate statistics with bounds checking
    if (data.isMember("total_population")) {
        total_population = Clamp(data["total_population"].asInt(), MIN_POPULATION, MAX_PROVINCE_POPULATION);
    }
    if (data.isMember("total_children")) {
        total_children = Clamp(data["total_children"].asInt(), MIN_POPULATION, MAX_PROVINCE_POPULATION);
    }
    if (data.isMember("total_adults")) {
        total_adults = Clamp(data["total_adults"].asInt(), MIN_POPULATION, MAX_PROVINCE_POPULATION);
    }
    if (data.isMember("total_elderly")) {
        total_elderly = Clamp(data["total_elderly"].asInt(), MIN_POPULATION, MAX_PROVINCE_POPULATION);
    }
    if (data.isMember("total_males")) {
        total_males = Clamp(data["total_males"].asInt(), MIN_POPULATION, MAX_PROVINCE_POPULATION);
    }
    if (data.isMember("total_females")) {
        total_females = Clamp(data["total_females"].asInt(), MIN_POPULATION, MAX_PROVINCE_POPULATION);
    }

    if (data.isMember("average_happiness")) {
        average_happiness = Clamp(data["average_happiness"].asDouble(), MIN_RATE, MAX_RATE);
    }
    if (data.isMember("average_literacy")) {
        average_literacy = Clamp(data["average_literacy"].asDouble(), MIN_RATE, MAX_RATE);
    }
    if (data.isMember("average_wealth")) {
        average_wealth = data["average_wealth"].asDouble();  // No upper limit on wealth
    }
    if (data.isMember("average_health")) {
        average_health = Clamp(data["average_health"].asDouble(), MIN_RATE, MAX_RATE);
    }
    if (data.isMember("overall_employment_rate")) {
        overall_employment_rate = Clamp(data["overall_employment_rate"].asDouble(), MIN_RATE, MAX_RATE);
    }

    if (data.isMember("total_military_eligible")) total_military_eligible = data["total_military_eligible"].asInt();
    if (data.isMember("average_military_quality")) average_military_quality = data["average_military_quality"].asDouble();
    if (data.isMember("total_military_service_obligation")) total_military_service_obligation = data["total_military_service_obligation"].asInt();

    // Distribution maps
    if (data.isMember("culture_distribution") && data["culture_distribution"].isObject()) {
        culture_distribution.clear();
        for (const auto& key : data["culture_distribution"].getMemberNames()) {
            culture_distribution[key] = data["culture_distribution"][key].asInt();
        }
    }

    if (data.isMember("religion_distribution") && data["religion_distribution"].isObject()) {
        religion_distribution.clear();
        for (const auto& key : data["religion_distribution"].getMemberNames()) {
            religion_distribution[key] = data["religion_distribution"][key].asInt();
        }
    }

    if (data.isMember("class_distribution") && data["class_distribution"].isObject()) {
        class_distribution.clear();
        for (const auto& key : data["class_distribution"].getMemberNames()) {
            int class_int = std::stoi(key);
            if (class_int >= 0 && class_int < static_cast<int>(SocialClass::COUNT)) {
                class_distribution[static_cast<SocialClass>(class_int)] = data["class_distribution"][key].asInt();
            }
        }
    }

    if (data.isMember("legal_status_distribution") && data["legal_status_distribution"].isObject()) {
        legal_status_distribution.clear();
        for (const auto& key : data["legal_status_distribution"].getMemberNames()) {
            int status_int = std::stoi(key);
            if (status_int >= 0 && status_int < static_cast<int>(LegalStatus::COUNT)) {
                legal_status_distribution[static_cast<LegalStatus>(status_int)] = data["legal_status_distribution"][key].asInt();
            }
        }
    }

    if (data.isMember("total_employment") && data["total_employment"].isObject()) {
        total_employment.clear();
        for (const auto& key : data["total_employment"].getMemberNames()) {
            int emp_int = std::stoi(key);
            if (emp_int >= 0 && emp_int < static_cast<int>(EmploymentType::COUNT)) {
                total_employment[static_cast<EmploymentType>(emp_int)] = data["total_employment"][key].asInt();
            }
        }
    }

    // Employment categories
    if (data.isMember("productive_workers")) productive_workers = data["productive_workers"].asInt();
    if (data.isMember("non_productive_income")) non_productive_income = data["non_productive_income"].asInt();
    if (data.isMember("unemployed_seeking")) unemployed_seeking = data["unemployed_seeking"].asInt();
    if (data.isMember("unemployable")) unemployable = data["unemployable"].asInt();
    if (data.isMember("dependents")) dependents = data["dependents"].asInt();

    // Economic and social metrics
    if (data.isMember("total_tax_revenue_potential")) total_tax_revenue_potential = data["total_tax_revenue_potential"].asDouble();
    if (data.isMember("total_feudal_service_days")) total_feudal_service_days = data["total_feudal_service_days"].asDouble();
    if (data.isMember("guild_membership_percentage")) guild_membership_percentage = data["guild_membership_percentage"].asDouble();
    if (data.isMember("social_mobility_average")) social_mobility_average = data["social_mobility_average"].asDouble();
    if (data.isMember("cultural_assimilation_rate")) cultural_assimilation_rate = data["cultural_assimilation_rate"].asDouble();
    if (data.isMember("religious_conversion_rate")) religious_conversion_rate = data["religious_conversion_rate"].asDouble();
    if (data.isMember("inter_class_tension")) inter_class_tension = data["inter_class_tension"].asDouble();

    // Demographic metrics
    if (data.isMember("population_density")) population_density = data["population_density"].asDouble();
    if (data.isMember("growth_rate")) growth_rate = data["growth_rate"].asDouble();
    if (data.isMember("birth_rate_average")) birth_rate_average = data["birth_rate_average"].asDouble();
    if (data.isMember("death_rate_average")) death_rate_average = data["death_rate_average"].asDouble();
    if (data.isMember("migration_net_rate")) migration_net_rate = data["migration_net_rate"].asDouble();

    // Mark caches as dirty after deserialization
    MarkGroupIndexDirty();
    MarkEmploymentCacheDirty();

    return true;
}

// =============================================================================
// SettlementComponent & PopulationEventsComponent - Placeholder for now
// =============================================================================

std::string SettlementComponent::Serialize() const {
    // TODO: Implement Settlement serialization (complex, contains nested population groups)
    return "{}";
}

bool SettlementComponent::Deserialize(const std::string& json_str) {
    // TODO: Implement Settlement deserialization
    return true;
}

std::string PopulationEventsComponent::Serialize() const {
    // TODO: Implement PopulationEvents serialization
    return "{}";
}

bool PopulationEventsComponent::Deserialize(const std::string& json_str) {
    // TODO: Implement PopulationEvents deserialization
    return true;
}

} // namespace game::population
