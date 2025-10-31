// ============================================================================
// DefinitionLoader.h - Loads JSON definitions for game data
// Created: October 31, 2025 - JSON Integration
// Location: include/game/data/DefinitionLoader.h
// ============================================================================

#pragma once

#include "game/technology/TechnologyComponents.h"
#include "game/military/MilitaryComponents.h"
#include "core/types/game_types.h"
#include <json/json.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>

namespace game::data {

    // ============================================================================
    // Resource Definition Structure
    // ============================================================================

    struct ResourceDefinition {
        game::types::ResourceType type = game::types::ResourceType::INVALID;
        uint16_t id = 0;
        std::string category;
        std::string name;
        std::string description;

        double base_value = 1.0;
        double storage_space = 1.0;
        double spoilage_rate = 0.0;
        bool tradeable = true;
        bool strategic = false;
    };

    // ============================================================================
    // Building Definition Structure
    // ============================================================================

    struct BuildingDefinition {
        uint16_t id = 0;
        std::string type;
        std::string category;
        std::string name;
        std::string description;

        double construction_cost = 0.0;
        uint32_t construction_time_days = 0;
        double maintenance_cost = 0.0;

        std::unordered_map<std::string, double> construction_requirements;
        std::unordered_map<std::string, double> production;
        std::unordered_map<std::string, double> effects;
        std::vector<std::string> prerequisites;
    };

    // ============================================================================
    // Unit Definition Structure (Enhanced from MilitaryComponents)
    // ============================================================================

    struct UnitDefinition {
        game::military::UnitType type = game::military::UnitType::LEVIES;
        uint16_t id = 0;
        game::military::UnitClass unit_class = game::military::UnitClass::INFANTRY;
        game::military::CombatRole role = game::military::CombatRole::MELEE;

        std::string name;
        std::string description;

        // Combat stats
        uint32_t max_strength = 1000;
        double attack_strength = 10.0;
        double defense_strength = 8.0;
        double movement_speed = 1.0;
        double range = 0.0;

        // Quality and training
        double equipment_quality = 0.5;
        double training = 0.5;

        // Costs
        double recruitment_cost = 100.0;
        double monthly_maintenance = 10.0;

        // Requirements
        std::unordered_map<std::string, int> equipment_requirements;
        std::unordered_map<std::string, double> monthly_supply_needs;
        std::vector<std::string> viable_classes;
        std::string min_quality;
        std::string max_quality;

        // Create MilitaryUnit from this definition
        game::military::MilitaryUnit CreateMilitaryUnit() const;
    };

    // ============================================================================
    // Definition Loader Class
    // ============================================================================

    class DefinitionLoader {
    public:
        DefinitionLoader();
        ~DefinitionLoader();

        // Initialization
        bool Initialize(const std::string& definitions_path);
        bool LoadAllDefinitions();

        // Individual file loading
        bool LoadTechnologies(const std::string& filepath);
        bool LoadUnits(const std::string& filepath);
        bool LoadBuildings(const std::string& filepath);
        bool LoadResources(const std::string& filepath);

        // Technology queries
        const game::technology::TechnologyDefinition* GetTechnologyDefinition(
            game::technology::TechnologyType type) const;
        std::vector<game::technology::TechnologyDefinition> GetTechnologiesByCategory(
            game::technology::TechnologyCategory category) const;
        std::vector<game::technology::TechnologyType> GetAllTechnologyTypes() const;

        // Unit queries
        const UnitDefinition* GetUnitDefinition(game::military::UnitType type) const;
        std::vector<UnitDefinition> GetUnitsByClass(game::military::UnitClass unit_class) const;
        std::vector<game::military::UnitType> GetAllUnitTypes() const;

        // Building queries
        const BuildingDefinition* GetBuildingDefinition(const std::string& type) const;
        std::vector<BuildingDefinition> GetBuildingsByCategory(const std::string& category) const;
        std::vector<std::string> GetAllBuildingTypes() const;

        // Resource queries
        const ResourceDefinition* GetResourceDefinition(game::types::ResourceType type) const;
        std::vector<ResourceDefinition> GetResourcesByCategory(const std::string& category) const;
        std::vector<game::types::ResourceType> GetAllResourceTypes() const;

        // Status queries
        bool IsInitialized() const { return m_initialized; }
        size_t GetTechnologyCount() const { return m_technologies.size(); }
        size_t GetUnitCount() const { return m_units.size(); }
        size_t GetBuildingCount() const { return m_buildings.size(); }
        size_t GetResourceCount() const { return m_resources.size(); }

        // Singleton access
        static DefinitionLoader& GetInstance();

    private:
        // Parsing helpers
        game::technology::TechnologyDefinition ParseTechnology(const Json::Value& json);
        UnitDefinition ParseUnit(const Json::Value& json);
        BuildingDefinition ParseBuilding(const Json::Value& json);
        ResourceDefinition ParseResource(const Json::Value& json);

        // Type conversion helpers
        game::technology::TechnologyType ParseTechnologyType(const std::string& type_str);
        game::technology::TechnologyCategory ParseTechnologyCategory(const std::string& cat_str);
        game::military::UnitType ParseUnitType(const std::string& type_str);
        game::military::UnitClass ParseUnitClass(const std::string& class_str);
        game::military::CombatRole ParseCombatRole(const std::string& role_str);
        game::military::MoraleState ParseMoraleState(const std::string& state_str);
        game::types::ResourceType ParseResourceType(const std::string& type_str);

        // Validation
        bool ValidateTechnology(const game::technology::TechnologyDefinition& tech);
        bool ValidateUnit(const UnitDefinition& unit);
        bool ValidateBuilding(const BuildingDefinition& building);
        bool ValidateResource(const ResourceDefinition& resource);

    private:
        // Definitions storage
        std::unordered_map<game::technology::TechnologyType, game::technology::TechnologyDefinition> m_technologies;
        std::unordered_map<game::military::UnitType, UnitDefinition> m_units;
        std::unordered_map<std::string, BuildingDefinition> m_buildings;
        std::unordered_map<game::types::ResourceType, ResourceDefinition> m_resources;

        // Configuration
        std::string m_definitions_path;
        bool m_initialized = false;

        // Thread safety
        mutable std::mutex m_mutex;

        // Singleton instance
        static std::unique_ptr<DefinitionLoader> s_instance;
        static std::once_flag s_once_flag;
    };

} // namespace game::data
