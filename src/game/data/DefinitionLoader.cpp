// ============================================================================
// DefinitionLoader.cpp - Implementation of JSON definition loading
// Created: October 31, 2025 - JSON Integration
// Location: src/game/data/DefinitionLoader.cpp
// ============================================================================

#include "game/data/DefinitionLoader.h"
#include <fstream>
#include <iostream>
#include <algorithm>

namespace game::data {

    // Static members
    std::unique_ptr<DefinitionLoader> DefinitionLoader::s_instance;
    std::once_flag DefinitionLoader::s_once_flag;

    // ============================================================================
    // Constructor / Destructor
    // ============================================================================

    DefinitionLoader::DefinitionLoader() {
    }

    DefinitionLoader::~DefinitionLoader() {
    }

    // ============================================================================
    // Singleton Access
    // ============================================================================

    DefinitionLoader& DefinitionLoader::GetInstance() {
        std::call_once(s_once_flag, []() {
            s_instance.reset(new DefinitionLoader());
        });
        return *s_instance;
    }

    // ============================================================================
    // Initialization
    // ============================================================================

    bool DefinitionLoader::Initialize(const std::string& definitions_path) {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_definitions_path = definitions_path;
        m_initialized = false;

        // Clear existing definitions
        m_technologies.clear();
        m_units.clear();
        m_buildings.clear();
        m_resources.clear();

        // Load all definitions
        if (!LoadAllDefinitions()) {
            std::cerr << "Failed to load definitions from: " << definitions_path << std::endl;
            return false;
        }

        m_initialized = true;
        std::cout << "DefinitionLoader initialized successfully" << std::endl;
        std::cout << "  Technologies: " << m_technologies.size() << std::endl;
        std::cout << "  Units: " << m_units.size() << std::endl;
        std::cout << "  Buildings: " << m_buildings.size() << std::endl;
        std::cout << "  Resources: " << m_resources.size() << std::endl;

        return true;
    }

    bool DefinitionLoader::LoadAllDefinitions() {
        bool success = true;

        success &= LoadTechnologies(m_definitions_path + "/technologies.json");
        success &= LoadUnits(m_definitions_path + "/units.json");
        success &= LoadBuildings(m_definitions_path + "/buildings.json");
        success &= LoadResources(m_definitions_path + "/resources.json");

        return success;
    }

    // ============================================================================
    // Technology Loading
    // ============================================================================

    bool DefinitionLoader::LoadTechnologies(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open technologies file: " << filepath << std::endl;
            return false;
        }

        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errors;

        if (!Json::parseFromStream(builder, file, &root, &errors)) {
            std::cerr << "Failed to parse technologies JSON: " << errors << std::endl;
            return false;
        }

        if (!root.isMember("technologies") || !root["technologies"].isArray()) {
            std::cerr << "Invalid technologies JSON format" << std::endl;
            return false;
        }

        const Json::Value& techs = root["technologies"];
        for (const auto& tech_json : techs) {
            auto tech = ParseTechnology(tech_json);
            if (ValidateTechnology(tech)) {
                m_technologies[tech.type] = tech;
            }
        }

        return true;
    }

    game::technology::TechnologyDefinition DefinitionLoader::ParseTechnology(const Json::Value& json) {
        game::technology::TechnologyDefinition tech;

        tech.type = ParseTechnologyType(json.get("type", "INVALID").asString());
        tech.category = ParseTechnologyCategory(json.get("category", "AGRICULTURAL").asString());
        tech.name = json.get("name", "Unknown").asString();
        tech.description = json.get("description", "").asString();

        tech.base_research_cost = json.get("base_research_cost", 1000.0).asDouble();
        tech.literacy_requirement = json.get("literacy_requirement", 0.1).asDouble();
        tech.historical_emergence_year = json.get("historical_emergence_year", 1066).asUInt();
        tech.historical_spread_duration = json.get("historical_spread_duration", 50).asUInt();
        tech.historical_discovery_chance = json.get("historical_discovery_chance", 0.01).asDouble();

        // Parse prerequisites
        if (json.isMember("prerequisites") && json["prerequisites"].isArray()) {
            for (const auto& prereq : json["prerequisites"]) {
                tech.prerequisites.push_back(
                    ParseTechnologyType(std::to_string(prereq.asInt()))
                );
            }
        }

        // Parse effects
        if (json.isMember("effects") && json["effects"].isObject()) {
            for (const auto& key : json["effects"].getMemberNames()) {
                tech.effects[key] = json["effects"][key].asDouble();
            }
        }

        return tech;
    }

    // ============================================================================
    // Unit Loading
    // ============================================================================

    bool DefinitionLoader::LoadUnits(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open units file: " << filepath << std::endl;
            return false;
        }

        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errors;

        if (!Json::parseFromStream(builder, file, &root, &errors)) {
            std::cerr << "Failed to parse units JSON: " << errors << std::endl;
            return false;
        }

        if (!root.isMember("units") || !root["units"].isArray()) {
            std::cerr << "Invalid units JSON format" << std::endl;
            return false;
        }

        const Json::Value& units = root["units"];
        for (const auto& unit_json : units) {
            auto unit = ParseUnit(unit_json);
            if (ValidateUnit(unit)) {
                m_units[unit.type] = unit;
            }
        }

        return true;
    }

    UnitDefinition DefinitionLoader::ParseUnit(const Json::Value& json) {
        UnitDefinition unit;

        unit.id = json.get("id", 0).asUInt();
        unit.type = ParseUnitType(json.get("type", "LEVIES").asString());
        unit.unit_class = ParseUnitClass(json.get("class", "INFANTRY").asString());
        unit.role = ParseCombatRole(json.get("role", "MELEE").asString());
        unit.name = json.get("name", "Unknown").asString();
        unit.description = json.get("description", "").asString();

        // Combat stats
        unit.max_strength = json.get("max_strength", 1000).asUInt();
        unit.attack_strength = json.get("attack_strength", 10.0).asDouble();
        unit.defense_strength = json.get("defense_strength", 8.0).asDouble();
        unit.movement_speed = json.get("movement_speed", 1.0).asDouble();
        unit.range = json.get("range", 0.0).asDouble();

        // Quality
        unit.equipment_quality = json.get("equipment_quality", 0.5).asDouble();
        unit.training = json.get("training", 0.5).asDouble();

        // Costs
        unit.recruitment_cost = json.get("recruitment_cost", 100.0).asDouble();
        unit.monthly_maintenance = json.get("monthly_maintenance", 10.0).asDouble();

        // Equipment requirements
        if (json.isMember("equipment_requirements") && json["equipment_requirements"].isObject()) {
            for (const auto& key : json["equipment_requirements"].getMemberNames()) {
                unit.equipment_requirements[key] = json["equipment_requirements"][key].asInt();
            }
        }

        // Supply needs
        if (json.isMember("monthly_supply_needs") && json["monthly_supply_needs"].isObject()) {
            for (const auto& key : json["monthly_supply_needs"].getMemberNames()) {
                unit.monthly_supply_needs[key] = json["monthly_supply_needs"][key].asDouble();
            }
        }

        // Viable classes
        if (json.isMember("viable_classes") && json["viable_classes"].isArray()) {
            for (const auto& cls : json["viable_classes"]) {
                unit.viable_classes.push_back(cls.asString());
            }
        }

        unit.min_quality = json.get("min_quality", "STEADY").asString();
        unit.max_quality = json.get("max_quality", "CONFIDENT").asString();

        return unit;
    }

    // ============================================================================
    // Building Loading
    // ============================================================================

    bool DefinitionLoader::LoadBuildings(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open buildings file: " << filepath << std::endl;
            return false;
        }

        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errors;

        if (!Json::parseFromStream(builder, file, &root, &errors)) {
            std::cerr << "Failed to parse buildings JSON: " << errors << std::endl;
            return false;
        }

        if (!root.isMember("buildings") || !root["buildings"].isArray()) {
            std::cerr << "Invalid buildings JSON format" << std::endl;
            return false;
        }

        const Json::Value& buildings = root["buildings"];
        for (const auto& building_json : buildings) {
            auto building = ParseBuilding(building_json);
            if (ValidateBuilding(building)) {
                m_buildings[building.type] = building;
            }
        }

        return true;
    }

    BuildingDefinition DefinitionLoader::ParseBuilding(const Json::Value& json) {
        BuildingDefinition building;

        building.id = json.get("id", 0).asUInt();
        building.type = json.get("type", "UNKNOWN").asString();
        building.category = json.get("category", "PRODUCTION").asString();
        building.name = json.get("name", "Unknown").asString();
        building.description = json.get("description", "").asString();

        building.construction_cost = json.get("construction_cost", 0.0).asDouble();
        building.construction_time_days = json.get("construction_time_days", 0).asUInt();
        building.maintenance_cost = json.get("maintenance_cost", 0.0).asDouble();

        // Construction requirements
        if (json.isMember("construction_requirements") && json["construction_requirements"].isObject()) {
            for (const auto& key : json["construction_requirements"].getMemberNames()) {
                building.construction_requirements[key] = json["construction_requirements"][key].asDouble();
            }
        }

        // Production
        if (json.isMember("production") && json["production"].isObject()) {
            for (const auto& key : json["production"].getMemberNames()) {
                building.production[key] = json["production"][key].asDouble();
            }
        }

        // Effects
        if (json.isMember("effects") && json["effects"].isObject()) {
            for (const auto& key : json["effects"].getMemberNames()) {
                building.effects[key] = json["effects"][key].asDouble();
            }
        }

        // Prerequisites
        if (json.isMember("prerequisites") && json["prerequisites"].isArray()) {
            for (const auto& prereq : json["prerequisites"]) {
                building.prerequisites.push_back(prereq.asString());
            }
        }

        return building;
    }

    // ============================================================================
    // Resource Loading
    // ============================================================================

    bool DefinitionLoader::LoadResources(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open resources file: " << filepath << std::endl;
            return false;
        }

        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errors;

        if (!Json::parseFromStream(builder, file, &root, &errors)) {
            std::cerr << "Failed to parse resources JSON: " << errors << std::endl;
            return false;
        }

        if (!root.isMember("resources") || !root["resources"].isArray()) {
            std::cerr << "Invalid resources JSON format" << std::endl;
            return false;
        }

        const Json::Value& resources = root["resources"];
        for (const auto& resource_json : resources) {
            auto resource = ParseResource(resource_json);
            if (ValidateResource(resource)) {
                m_resources[resource.type] = resource;
            }
        }

        return true;
    }

    ResourceDefinition DefinitionLoader::ParseResource(const Json::Value& json) {
        ResourceDefinition resource;

        resource.id = json.get("id", 0).asUInt();
        resource.type = ParseResourceType(json.get("type", "INVALID").asString());
        resource.category = json.get("category", "BASIC").asString();
        resource.name = json.get("name", "Unknown").asString();
        resource.description = json.get("description", "").asString();

        resource.base_value = json.get("base_value", 1.0).asDouble();
        resource.storage_space = json.get("storage_space", 1.0).asDouble();
        resource.spoilage_rate = json.get("spoilage_rate", 0.0).asDouble();
        resource.tradeable = json.get("tradeable", true).asBool();
        resource.strategic = json.get("strategic", false).asBool();

        return resource;
    }

    // ============================================================================
    // Type Conversion Helpers
    // ============================================================================

    game::technology::TechnologyType DefinitionLoader::ParseTechnologyType(const std::string& type_str) {
        static const std::unordered_map<std::string, game::technology::TechnologyType> type_map = {
            {"THREE_FIELD_SYSTEM", game::technology::TechnologyType::THREE_FIELD_SYSTEM},
            {"HEAVY_PLOW", game::technology::TechnologyType::HEAVY_PLOW},
            {"HORSE_COLLAR", game::technology::TechnologyType::HORSE_COLLAR},
            {"WINDMILL", game::technology::TechnologyType::WINDMILL},
            {"WATERMILL", game::technology::TechnologyType::WATERMILL},
            {"CROP_ROTATION", game::technology::TechnologyType::CROP_ROTATION},
            {"SELECTIVE_BREEDING", game::technology::TechnologyType::SELECTIVE_BREEDING},
            {"AGRICULTURAL_MANUAL", game::technology::TechnologyType::AGRICULTURAL_MANUAL},
            {"IRRIGATION_SYSTEMS", game::technology::TechnologyType::IRRIGATION_SYSTEMS},
            {"NEW_WORLD_CROPS", game::technology::TechnologyType::NEW_WORLD_CROPS},
            {"CHAINMAIL_ARMOR", game::technology::TechnologyType::CHAINMAIL_ARMOR},
            {"PLATE_ARMOR", game::technology::TechnologyType::PLATE_ARMOR},
            {"CROSSBOW", game::technology::TechnologyType::CROSSBOW},
            {"LONGBOW", game::technology::TechnologyType::LONGBOW},
            {"GUNPOWDER", game::technology::TechnologyType::GUNPOWDER},
            {"CANNONS", game::technology::TechnologyType::CANNONS},
            {"ARQUEBUS", game::technology::TechnologyType::ARQUEBUS},
            {"MUSKET", game::technology::TechnologyType::MUSKET},
            {"STAR_FORTRESS", game::technology::TechnologyType::STAR_FORTRESS},
            {"MILITARY_ENGINEERING", game::technology::TechnologyType::MILITARY_ENGINEERING},
            {"BLAST_FURNACE", game::technology::TechnologyType::BLAST_FURNACE},
            {"WATER_POWERED_MACHINERY", game::technology::TechnologyType::WATER_POWERED_MACHINERY},
            {"MECHANICAL_CLOCK", game::technology::TechnologyType::MECHANICAL_CLOCK},
            {"PRINTING_PRESS", game::technology::TechnologyType::PRINTING_PRESS},
            {"DOUBLE_ENTRY_BOOKKEEPING", game::technology::TechnologyType::DOUBLE_ENTRY_BOOKKEEPING},
            {"PAPER_MAKING", game::technology::TechnologyType::PAPER_MAKING},
            {"GLASS_MAKING", game::technology::TechnologyType::GLASS_MAKING},
            {"TEXTILE_MACHINERY", game::technology::TechnologyType::TEXTILE_MACHINERY},
            {"ADVANCED_METALLURGY", game::technology::TechnologyType::ADVANCED_METALLURGY},
            {"PRECISION_INSTRUMENTS", game::technology::TechnologyType::PRECISION_INSTRUMENTS},
            {"WRITTEN_LAW_CODES", game::technology::TechnologyType::WRITTEN_LAW_CODES},
            {"BUREAUCRATIC_ADMINISTRATION", game::technology::TechnologyType::BUREAUCRATIC_ADMINISTRATION},
            {"CENSUS_TECHNIQUES", game::technology::TechnologyType::CENSUS_TECHNIQUES},
            {"TAX_COLLECTION_SYSTEMS", game::technology::TechnologyType::TAX_COLLECTION_SYSTEMS},
            {"DIPLOMATIC_PROTOCOLS", game::technology::TechnologyType::DIPLOMATIC_PROTOCOLS},
            {"RECORD_KEEPING", game::technology::TechnologyType::RECORD_KEEPING},
            {"STANDARDIZED_WEIGHTS", game::technology::TechnologyType::STANDARDIZED_WEIGHTS},
            {"POSTAL_SYSTEMS", game::technology::TechnologyType::POSTAL_SYSTEMS},
            {"PROFESSIONAL_ARMY", game::technology::TechnologyType::PROFESSIONAL_ARMY},
            {"STATE_MONOPOLIES", game::technology::TechnologyType::STATE_MONOPOLIES},
            {"SCHOLASTIC_METHOD", game::technology::TechnologyType::SCHOLASTIC_METHOD},
            {"UNIVERSITY_SYSTEM", game::technology::TechnologyType::UNIVERSITY_SYSTEM},
            {"VERNACULAR_WRITING", game::technology::TechnologyType::VERNACULAR_WRITING},
            {"NATURAL_PHILOSOPHY", game::technology::TechnologyType::NATURAL_PHILOSOPHY},
            {"MATHEMATICAL_NOTATION", game::technology::TechnologyType::MATHEMATICAL_NOTATION},
            {"EXPERIMENTAL_METHOD", game::technology::TechnologyType::EXPERIMENTAL_METHOD},
            {"HUMANIST_EDUCATION", game::technology::TechnologyType::HUMANIST_EDUCATION},
            {"SCIENTIFIC_INSTRUMENTS", game::technology::TechnologyType::SCIENTIFIC_INSTRUMENTS},
            {"OPTICAL_DEVICES", game::technology::TechnologyType::OPTICAL_DEVICES},
            {"CARTOGRAPHY", game::technology::TechnologyType::CARTOGRAPHY},
            {"IMPROVED_SHIP_DESIGN", game::technology::TechnologyType::IMPROVED_SHIP_DESIGN},
            {"NAVIGATION_INSTRUMENTS", game::technology::TechnologyType::NAVIGATION_INSTRUMENTS},
            {"COMPASS_NAVIGATION", game::technology::TechnologyType::COMPASS_NAVIGATION},
            {"NAVAL_ARTILLERY", game::technology::TechnologyType::NAVAL_ARTILLERY},
            {"OCEAN_NAVIGATION", game::technology::TechnologyType::OCEAN_NAVIGATION},
            {"SHIPYARD_TECHNIQUES", game::technology::TechnologyType::SHIPYARD_TECHNIQUES},
            {"MARITIME_LAW", game::technology::TechnologyType::MARITIME_LAW},
            {"NAVAL_TACTICS", game::technology::TechnologyType::NAVAL_TACTICS},
            {"LIGHTHOUSE_SYSTEMS", game::technology::TechnologyType::LIGHTHOUSE_SYSTEMS},
            {"HARBOR_ENGINEERING", game::technology::TechnologyType::HARBOR_ENGINEERING}
        };

        auto it = type_map.find(type_str);
        return (it != type_map.end()) ? it->second : game::technology::TechnologyType::INVALID;
    }

    game::technology::TechnologyCategory DefinitionLoader::ParseTechnologyCategory(const std::string& cat_str) {
        static const std::unordered_map<std::string, game::technology::TechnologyCategory> cat_map = {
            {"AGRICULTURAL", game::technology::TechnologyCategory::AGRICULTURAL},
            {"MILITARY", game::technology::TechnologyCategory::MILITARY},
            {"CRAFT", game::technology::TechnologyCategory::CRAFT},
            {"ADMINISTRATIVE", game::technology::TechnologyCategory::ADMINISTRATIVE},
            {"ACADEMIC", game::technology::TechnologyCategory::ACADEMIC},
            {"NAVAL", game::technology::TechnologyCategory::NAVAL}
        };

        auto it = cat_map.find(cat_str);
        return (it != cat_map.end()) ? it->second : game::technology::TechnologyCategory::AGRICULTURAL;
    }

    game::military::UnitType DefinitionLoader::ParseUnitType(const std::string& type_str) {
        static const std::unordered_map<std::string, game::military::UnitType> type_map = {
            {"LEVIES", game::military::UnitType::LEVIES},
            {"SPEARMEN", game::military::UnitType::SPEARMEN},
            {"CROSSBOWMEN", game::military::UnitType::CROSSBOWMEN},
            {"LONGBOWMEN", game::military::UnitType::LONGBOWMEN},
            {"MEN_AT_ARMS", game::military::UnitType::MEN_AT_ARMS},
            {"PIKEMEN", game::military::UnitType::PIKEMEN},
            {"ARQUEBUSIERS", game::military::UnitType::ARQUEBUSIERS},
            {"MUSKETEERS", game::military::UnitType::MUSKETEERS},
            {"LIGHT_CAVALRY", game::military::UnitType::LIGHT_CAVALRY},
            {"HEAVY_CAVALRY", game::military::UnitType::HEAVY_CAVALRY},
            {"MOUNTED_ARCHERS", game::military::UnitType::MOUNTED_ARCHERS},
            {"DRAGOONS", game::military::UnitType::DRAGOONS},
            {"CATAPULTS", game::military::UnitType::CATAPULTS},
            {"TREBUCHETS", game::military::UnitType::TREBUCHETS},
            {"CANNONS", game::military::UnitType::CANNONS},
            {"SIEGE_TOWERS", game::military::UnitType::SIEGE_TOWERS},
            {"GALLEYS", game::military::UnitType::GALLEYS},
            {"COGS", game::military::UnitType::COGS},
            {"CARRACKS", game::military::UnitType::CARRACKS},
            {"GALLEONS", game::military::UnitType::GALLEONS},
            {"SHIPS_OF_THE_LINE", game::military::UnitType::SHIPS_OF_THE_LINE}
        };

        auto it = type_map.find(type_str);
        return (it != type_map.end()) ? it->second : game::military::UnitType::LEVIES;
    }

    game::military::UnitClass DefinitionLoader::ParseUnitClass(const std::string& class_str) {
        static const std::unordered_map<std::string, game::military::UnitClass> class_map = {
            {"INFANTRY", game::military::UnitClass::INFANTRY},
            {"CAVALRY", game::military::UnitClass::CAVALRY},
            {"SIEGE", game::military::UnitClass::SIEGE},
            {"NAVAL", game::military::UnitClass::NAVAL}
        };

        auto it = class_map.find(class_str);
        return (it != class_map.end()) ? it->second : game::military::UnitClass::INFANTRY;
    }

    game::military::CombatRole DefinitionLoader::ParseCombatRole(const std::string& role_str) {
        static const std::unordered_map<std::string, game::military::CombatRole> role_map = {
            {"MELEE", game::military::CombatRole::MELEE},
            {"RANGED", game::military::CombatRole::RANGED},
            {"SIEGE", game::military::CombatRole::SIEGE},
            {"SUPPORT", game::military::CombatRole::SUPPORT},
            {"CAVALRY_CHARGE", game::military::CombatRole::CAVALRY_CHARGE},
            {"SKIRMISH", game::military::CombatRole::SKIRMISH}
        };

        auto it = role_map.find(role_str);
        return (it != role_map.end()) ? it->second : game::military::CombatRole::MELEE;
    }

    game::military::MoraleState DefinitionLoader::ParseMoraleState(const std::string& state_str) {
        static const std::unordered_map<std::string, game::military::MoraleState> state_map = {
            {"ROUTING", game::military::MoraleState::ROUTING},
            {"BROKEN", game::military::MoraleState::BROKEN},
            {"WAVERING", game::military::MoraleState::WAVERING},
            {"STEADY", game::military::MoraleState::STEADY},
            {"CONFIDENT", game::military::MoraleState::CONFIDENT},
            {"FANATICAL", game::military::MoraleState::FANATICAL}
        };

        auto it = state_map.find(state_str);
        return (it != state_map.end()) ? it->second : game::military::MoraleState::STEADY;
    }

    game::types::ResourceType DefinitionLoader::ParseResourceType(const std::string& type_str) {
        static const std::unordered_map<std::string, game::types::ResourceType> type_map = {
            {"FOOD", game::types::ResourceType::FOOD},
            {"WOOD", game::types::ResourceType::WOOD},
            {"STONE", game::types::ResourceType::STONE},
            {"IRON", game::types::ResourceType::IRON},
            {"LEATHER", game::types::ResourceType::LEATHER},
            {"CLOTH", game::types::ResourceType::CLOTH},
            {"HORSES", game::types::ResourceType::HORSES},
            {"SALTPETER", game::types::ResourceType::SALTPETER},
            {"GOLD", game::types::ResourceType::GOLD},
            {"SILVER", game::types::ResourceType::SILVER},
            {"SALT", game::types::ResourceType::SALT},
            {"SPICES", game::types::ResourceType::SPICES},
            {"SILK", game::types::ResourceType::SILK},
            {"WINE", game::types::ResourceType::WINE},
            {"FURS", game::types::ResourceType::FURS},
            {"IVORY", game::types::ResourceType::IVORY},
            {"JEWELS", game::types::ResourceType::JEWELS}
        };

        auto it = type_map.find(type_str);
        return (it != type_map.end()) ? it->second : game::types::ResourceType::INVALID;
    }

    // ============================================================================
    // Validation
    // ============================================================================

    bool DefinitionLoader::ValidateTechnology(const game::technology::TechnologyDefinition& tech) {
        return tech.type != game::technology::TechnologyType::INVALID && !tech.name.empty();
    }

    bool DefinitionLoader::ValidateUnit(const UnitDefinition& unit) {
        return unit.type != game::military::UnitType::LEVIES || !unit.name.empty();
    }

    bool DefinitionLoader::ValidateBuilding(const BuildingDefinition& building) {
        return !building.type.empty() && !building.name.empty();
    }

    bool DefinitionLoader::ValidateResource(const ResourceDefinition& resource) {
        return resource.type != game::types::ResourceType::INVALID && !resource.name.empty();
    }

    // ============================================================================
    // Query Methods - Technologies
    // ============================================================================

    const game::technology::TechnologyDefinition* DefinitionLoader::GetTechnologyDefinition(
        game::technology::TechnologyType type) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_technologies.find(type);
        return (it != m_technologies.end()) ? &it->second : nullptr;
    }

    std::vector<game::technology::TechnologyDefinition> DefinitionLoader::GetTechnologiesByCategory(
        game::technology::TechnologyCategory category) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<game::technology::TechnologyDefinition> result;

        for (const auto& [type, tech] : m_technologies) {
            if (tech.category == category) {
                result.push_back(tech);
            }
        }

        return result;
    }

    std::vector<game::technology::TechnologyType> DefinitionLoader::GetAllTechnologyTypes() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<game::technology::TechnologyType> result;

        for (const auto& [type, tech] : m_technologies) {
            result.push_back(type);
        }

        return result;
    }

    // ============================================================================
    // Query Methods - Units
    // ============================================================================

    const UnitDefinition* DefinitionLoader::GetUnitDefinition(game::military::UnitType type) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_units.find(type);
        return (it != m_units.end()) ? &it->second : nullptr;
    }

    std::vector<UnitDefinition> DefinitionLoader::GetUnitsByClass(game::military::UnitClass unit_class) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<UnitDefinition> result;

        for (const auto& [type, unit] : m_units) {
            if (unit.unit_class == unit_class) {
                result.push_back(unit);
            }
        }

        return result;
    }

    std::vector<game::military::UnitType> DefinitionLoader::GetAllUnitTypes() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<game::military::UnitType> result;

        for (const auto& [type, unit] : m_units) {
            result.push_back(type);
        }

        return result;
    }

    // ============================================================================
    // Query Methods - Buildings
    // ============================================================================

    const BuildingDefinition* DefinitionLoader::GetBuildingDefinition(const std::string& type) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_buildings.find(type);
        return (it != m_buildings.end()) ? &it->second : nullptr;
    }

    std::vector<BuildingDefinition> DefinitionLoader::GetBuildingsByCategory(const std::string& category) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<BuildingDefinition> result;

        for (const auto& [type, building] : m_buildings) {
            if (building.category == category) {
                result.push_back(building);
            }
        }

        return result;
    }

    std::vector<std::string> DefinitionLoader::GetAllBuildingTypes() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<std::string> result;

        for (const auto& [type, building] : m_buildings) {
            result.push_back(type);
        }

        return result;
    }

    // ============================================================================
    // Query Methods - Resources
    // ============================================================================

    const ResourceDefinition* DefinitionLoader::GetResourceDefinition(game::types::ResourceType type) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_resources.find(type);
        return (it != m_resources.end()) ? &it->second : nullptr;
    }

    std::vector<ResourceDefinition> DefinitionLoader::GetResourcesByCategory(const std::string& category) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<ResourceDefinition> result;

        for (const auto& [type, resource] : m_resources) {
            if (resource.category == category) {
                result.push_back(resource);
            }
        }

        return result;
    }

    std::vector<game::types::ResourceType> DefinitionLoader::GetAllResourceTypes() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<game::types::ResourceType> result;

        for (const auto& [type, resource] : m_resources) {
            result.push_back(type);
        }

        return result;
    }

    // ============================================================================
    // Unit Creation Helper
    // ============================================================================

    game::military::MilitaryUnit UnitDefinition::CreateMilitaryUnit() const {
        game::military::MilitaryUnit unit(type);

        unit.unit_class = unit_class;
        unit.primary_role = role;
        unit.max_strength = max_strength;
        unit.current_strength = max_strength;
        unit.attack_strength = attack_strength;
        unit.defense_strength = defense_strength;
        unit.movement_speed = movement_speed;
        unit.range = range;
        unit.equipment_quality = equipment_quality;
        unit.training = training;
        unit.recruitment_cost = recruitment_cost;
        unit.monthly_maintenance = monthly_maintenance;

        return unit;
    }

} // namespace game::data
