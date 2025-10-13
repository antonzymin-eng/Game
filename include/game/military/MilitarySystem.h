// ============================================================================
// Date/Time Created: Wednesday, September 24, 2025 - 2:45 PM PST
// Intended Folder Location: include/game/military/MilitarySystem.h
// MilitarySystem.h - Core Military System (Declarations Only)
// Dependencies: PopulationSystem, ProvinceSystem, TechnologySystem, ECS Foundation
// ============================================================================

#pragma once

#include "core/ECS/ISystem.h"
#include "core/ECS/ComponentAccessManager.h"
#include "core/ECS/MessageBus.h"
#include "core/ECS/ISerializable.h"
#include "core/types/game_types.h"
#include "game/military/MilitaryComponents.h"
#include "game/population/PopulationTypes.h"
#include "game/population/PopulationComponents.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>

// Forward declarations
namespace game { namespace technology { enum class TechnologyType : uint16_t; } }

namespace game::military {

    // Note: Military types and components are now defined in MilitaryComponents.h

    // ============================================================================
    // Military System - Main Class Declaration  
    // ============================================================================

    class MilitarySystem : public game::core::ISystem {
    private:
        ::core::ecs::ComponentAccessManager& m_access_manager;
        ::core::ecs::MessageBus& m_message_bus;
        bool m_initialized = false;

    public:
        explicit MilitarySystem(::core::ecs::ComponentAccessManager& access_manager,
                               ::core::ecs::MessageBus& message_bus);
        virtual ~MilitarySystem() = default;

        // ISystem interface
        void Initialize() override;
        void Update(float delta_time) override;
        void Shutdown() override;
        
        // Optional: ISerializable interface (if needed)
        Json::Value Serialize(int version) const;
        bool Deserialize(const Json::Value& data, int version);
        std::string GetSystemName() const { return "MilitarySystem"; }

        // ECS component creation
        void CreateMilitaryComponents(game::types::EntityID entity_id);
        void CreateArmyComponents(game::types::EntityID army_id, const std::string& army_name);

        // ECS-based unit management
        bool RecruitUnit(game::types::EntityID province_id, UnitType unit_type, uint32_t quantity);
        void DisbandUnit(game::types::EntityID province_id, size_t unit_index);
        void MergeUnits(game::types::EntityID province_id, size_t unit_a, size_t unit_b);
        void SplitUnit(game::types::EntityID province_id, size_t unit_index, uint32_t split_size);
        
        // ECS-based army management
        game::types::EntityID CreateArmy(game::types::EntityID home_province, const std::string& army_name);
        void DisbandArmy(game::types::EntityID army_id);
        void MoveArmy(game::types::EntityID army_id, game::types::EntityID destination);
        void AssignCommander(game::types::EntityID army_id, game::types::EntityID commander_id);
        
        // ECS-based combat operations
        void InitiateBattle(game::types::EntityID attacker_army, game::types::EntityID defender_army);
        void ProcessBattle(game::types::EntityID battle_id, float battle_duration);
        void ResolveBattle(game::types::EntityID battle_id);
        
        // ECS-based siege operations
        void BeginSiege(game::types::EntityID besieging_army, game::types::EntityID target_province);
        void ProcessSiege(game::types::EntityID siege_id, float time_delta);
        void ResolveSiege(game::types::EntityID siege_id, bool attacker_success);
        
        // ECS-based military development
        void UpgradeTrainingFacilities(game::types::EntityID province_id, double investment);
        void ImproveEquipment(game::types::EntityID province_id, UnitType unit_type, double investment);
        void ConstructFortifications(game::types::EntityID province_id, uint32_t fortification_type, double cost);
        
        // ECS-based query methods
        uint32_t GetTotalMilitaryStrength(game::types::EntityID province_id) const;
        double GetMilitaryMaintenance(game::types::EntityID province_id) const;
        std::vector<game::types::EntityID> GetAllArmies() const;

    private:
        // ECS-based internal methods
        void ProcessMilitaryUpdate(game::types::EntityID entity_id);
        void ProcessRecruitment(game::types::EntityID entity_id);
        void ProcessArmyMaintenance(game::types::EntityID army_id);
        void UpdateMilitaryBudget(game::types::EntityID entity_id);
        
        // Helper methods
        MilitaryUnit CreateUnitTemplate(UnitType unit_type) const;
        
        // Data storage
        std::unordered_map<UnitType, MilitaryUnit> m_unit_templates;
        std::unordered_map<game::technology::TechnologyType, std::vector<UnitType>> m_tech_unlocks;

        // Timing and update control
        float m_accumulated_time = 0.0f;
        float m_monthly_timer = 0.0f;
        float m_update_interval = 1.0f;

        // Combat resolution
        double m_combat_speed_multiplier = 1.0;
        double m_base_morale_impact = 1.0;
        double m_supply_consumption_rate = 1.0;

        // Active military operations
        std::vector<types::EntityID> m_active_battles;
        std::vector<types::EntityID> m_active_sieges;

        // Internal helper methods (declarations only)
        void InitializeUnitTemplates();
        void InitializeTechnologyUnlocks();
        void SubscribeToEvents();

        // Recruitment calculations
        bool CanRecruitFromClass(types::EntityID province_id, game::population::SocialClass social_class,
            UnitType unit_type, uint32_t quantity);
        double CalculateRecruitmentCost(UnitType unit_type, uint32_t quantity,
            game::population::SocialClass social_class);

        // Combat calculations
        double CalculateUnitEffectiveness(const MilitaryUnit& unit, const Commander* commander);
        double CalculateTerrainModifier(types::EntityID battle_location, UnitType unit_type);
        double CalculateMoraleImpact(MoraleState attacker_morale, MoraleState defender_morale);

        // Siege calculations
        double CalculateSiegeProgress(const ArmyComponent& besieging_army,
            const FortificationComponent& fortifications);
        double CalculateSiegeAttrition(const ArmyComponent& army, double siege_duration);

        // Supply calculations
        double CalculateSupplyRequirement(const ArmyComponent& army);
        double CalculateSupplyDistance(types::EntityID army_location, types::EntityID supply_source);
        bool IsSupplyRouteSecure(types::EntityID source, types::EntityID destination);

        // Technology effects
        void ApplyTechnologyToUnit(MilitaryUnit& unit, game::technology::TechnologyType tech);
        std::vector<game::technology::TechnologyType> GetRelevantTechnologies(UnitType unit_type);

        // Event handlers
        void OnPopulationChanged(const std::string& message);
        void OnTechnologyDiscovered(const std::string& message);
        void OnEconomicUpdate(const std::string& message);
        void OnBuildingConstructed(const std::string& message);

        // Utility methods
        void LogMilitaryEvent(types::EntityID province_id, const std::string& event_description);
        void ValidateMilitaryState(types::EntityID province_id);

        // Component access methods (missing declarations)
        MilitaryComponent* GetMilitaryComponent(types::EntityID province_id);
        const MilitaryComponent* GetMilitaryComponent(types::EntityID province_id) const;
        ArmyComponent* GetArmyComponent(types::EntityID army_id);
        const ArmyComponent* GetArmyComponent(types::EntityID army_id) const;
    };

    // ============================================================================
    // Military Database and Utility Functions (Declarations Only)
    // ============================================================================

    namespace database {
        // Unit template creation
        MilitaryUnit CreateInfantryUnit(UnitType type, population::SocialClass recruitment_class);
        MilitaryUnit CreateCavalryUnit(UnitType type, population::SocialClass recruitment_class);
        MilitaryUnit CreateSiegeUnit(UnitType type, population::SocialClass recruitment_class);
        MilitaryUnit CreateNavalUnit(UnitType type, population::SocialClass recruitment_class);

        // Commander generation
        Commander GenerateCommander(const std::string& name, population::SocialClass social_class,
            MilitaryRank rank);

        // Technology effects on military units
        namespace effects {
            constexpr const char* UNIT_ATTACK = "unit_attack";
            constexpr const char* UNIT_DEFENSE = "unit_defense";
            constexpr const char* UNIT_MORALE = "unit_morale";
            constexpr const char* RECRUITMENT_COST = "recruitment_cost";
            constexpr const char* MAINTENANCE_COST = "maintenance_cost";
            constexpr const char* SIEGE_EFFICIENCY = "siege_efficiency";
            constexpr const char* FORTIFICATION_STRENGTH = "fortification_strength";
        }
    }

    namespace utils {
        // String conversion utilities
        std::string UnitTypeToString(UnitType type);
        std::string UnitClassToString(UnitClass unit_class);
        std::string MilitaryRankToString(MilitaryRank rank);
        std::string CombatRoleToString(CombatRole role);
        std::string MoraleStateToString(MoraleState morale);

        // Military analysis utilities
        bool IsUnitTypeAvailable(UnitType type, int current_year);
        bool RequiresTechnology(UnitType type, game::technology::TechnologyType tech);
        UnitClass GetUnitClass(UnitType type);
        CombatRole GetPrimaryCombatRole(UnitType type);

        // Combat effectiveness calculations
        double CalculateUnitMatchup(UnitType attacker, UnitType defender);
        double GetTerrainAdvantage(UnitType unit_type, const std::string& terrain_type);
        double GetSeasonalModifier(UnitType unit_type, int current_month);

        // Resource requirements
        std::vector<game::types::ResourceType> GetUnitResourceRequirements(UnitType type);
        double GetUnitResourceConsumption(UnitType type, game::types::ResourceType resource);

        // Historical availability
        int GetHistoricalIntroductionYear(UnitType type);
        int GetHistoricalObsolescenceYear(UnitType type);
        bool IsHistoricallyAccurate(UnitType type, int year);
    }

} // namespace game::military