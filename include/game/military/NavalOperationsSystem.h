// ============================================================================
// NavalOperationsSystem.h - Naval Blockades and Coastal Bombardment
// Created: 2025-11-18 - Naval Operations System Implementation
// Location: include/game/military/NavalOperationsSystem.h
// ============================================================================

#pragma once

#include "game/military/MilitaryComponents.h"
#include "map/MapData.h"
#include "core/types/game_types.h"
#include <vector>
#include <string>

namespace game::military {

    /// Blockade effectiveness levels
    enum class BlockadeEffectiveness {
        NONE,           // No blockade
        PARTIAL,        // 25-50% trade disruption
        MODERATE,       // 50-75% trade disruption
        STRONG,         // 75-90% trade disruption
        TOTAL           // 90-100% trade disruption
    };

    /// Blockade status
    struct BlockadeStatus {
        bool is_active = false;
        BlockadeEffectiveness effectiveness = BlockadeEffectiveness::NONE;
        game::types::EntityID blockading_fleet = 0;
        game::types::EntityID target_port = 0;
        double trade_disruption_percent = 0.0;
        double enemy_attrition_rate = 0.0;
        uint32_t days_active = 0;
        uint32_t ships_intercepted = 0;
    };

    /// Coastal bombardment result
    struct CoastalBombardmentResult {
        bool was_successful = false;
        uint32_t fortification_damage = 0;
        uint32_t garrison_casualties = 0;
        uint32_t civilian_casualties = 0;
        double siege_progress_contribution = 0.0;
        uint32_t ammunition_expended = 0;
        std::string bombardment_summary;
    };

    /// Naval operations system
    class NavalOperationsSystem {
    public:
        // ========================================================================
        // Naval Blockade Operations
        // ========================================================================

        /// Establish blockade of a port/province
        static BlockadeStatus EstablishBlockade(
            const ArmyComponent& fleet,
            game::types::EntityID target_port,
            const game::map::ProvinceData& port_province
        );

        /// Calculate blockade effectiveness based on fleet strength
        static BlockadeEffectiveness CalculateBlockadeEffectiveness(
            const ArmyComponent& blockading_fleet,
            const game::map::ProvinceData& target_port
        );

        /// Update blockade status (called each day/turn)
        static void UpdateBlockade(
            BlockadeStatus& blockade,
            const ArmyComponent& fleet
        );

        /// Calculate trade disruption from blockade
        static double CalculateTradeDisruption(
            BlockadeEffectiveness effectiveness
        );

        /// Calculate attrition rate for blockaded province
        static double CalculateBlockadeAttrition(
            BlockadeEffectiveness effectiveness,
            uint32_t days_blockaded
        );

        /// Check if fleet can maintain blockade
        static bool CanMaintainBlockade(
            const ArmyComponent& fleet,
            const game::map::ProvinceData& target_port
        );

        /// Attempt to break blockade
        static bool AttemptBlockadeBreak(
            const ArmyComponent& blockaded_fleet,
            const ArmyComponent& blockading_fleet,
            const game::map::ProvinceData& port_province
        );

        // ========================================================================
        // Coastal Bombardment
        // ========================================================================

        /// Bombard coastal fortifications
        static CoastalBombardmentResult BombardCoastalFortifications(
            const ArmyComponent& fleet,
            const FortificationComponent& fortification,
            uint32_t bombardment_duration_hours
        );

        /// Calculate bombardment damage to fortifications
        static uint32_t CalculateFortificationDamage(
            const ArmyComponent& fleet,
            uint32_t bombardment_duration_hours
        );

        /// Calculate garrison casualties from bombardment
        static uint32_t CalculateGarrisonCasualties(
            const ArmyComponent& fleet,
            const MilitaryComponent& garrison,
            uint32_t bombardment_duration_hours
        );

        /// Check if fleet can bombard target province
        static bool CanBombardProvince(
            const ArmyComponent& fleet,
            const game::map::ProvinceData& target_province
        );

        /// Calculate siege support bonus from naval bombardment
        static double CalculateSiegeSupportBonus(
            const ArmyComponent& fleet,
            const FortificationComponent& fortification
        );

        // ========================================================================
        // Commerce Raiding
        // ========================================================================

        /// Conduct commerce raiding operations
        static uint32_t ConductCommerceRaiding(
            const ArmyComponent& raiding_fleet,
            const game::map::ProvinceData& target_sea_zone,
            uint32_t days_raiding
        );

        /// Calculate commerce raiding effectiveness
        static double CalculateRaidingEffectiveness(
            const ArmyComponent& raiding_fleet
        );

        /// Intercept enemy trade ships
        static uint32_t InterceptTradeShips(
            const ArmyComponent& raiding_fleet,
            double trade_volume,
            uint32_t days_raiding
        );

        /// Calculate gold captured from trade raiding
        static double CalculateCapturedGoods(
            uint32_t ships_intercepted,
            double average_trade_value
        );

        // ========================================================================
        // Naval Bombardment Effectiveness
        // ========================================================================

        /// Get fleet bombardment power
        static uint32_t GetFleetBombardmentPower(
            const ArmyComponent& fleet
        );

        /// Calculate effective range for bombardment
        static double GetBombardmentRange(
            const ArmyComponent& fleet
        );

        /// Check if ships have bombardment capability
        static bool HasBombardmentCapability(
            UnitType ship_type
        );

        /// Calculate ammunition requirements for bombardment
        static uint32_t CalculateAmmunitionRequired(
            const ArmyComponent& fleet,
            uint32_t bombardment_duration_hours
        );

        // ========================================================================
        // Amphibious Operations Support
        // ========================================================================

        /// Calculate landing support for amphibious assault
        static double CalculateLandingSupport(
            const ArmyComponent& fleet,
            const game::map::ProvinceData& landing_zone
        );

        /// Check if fleet can support landing
        static bool CanSupportLanding(
            const ArmyComponent& fleet,
            uint32_t troops_landing
        );

        /// Calculate transport capacity
        static uint32_t CalculateTransportCapacity(
            const ArmyComponent& fleet
        );

        // ========================================================================
        // Utility Functions
        // ========================================================================

        /// Get blockade effectiveness as string
        static std::string BlockadeEffectivenessToString(
            BlockadeEffectiveness effectiveness
        );

        /// Generate blockade report
        static std::string GenerateBlockadeReport(
            const BlockadeStatus& blockade,
            const ArmyComponent& fleet
        );

        /// Generate bombardment summary
        static std::string GenerateBombardmentSummary(
            const CoastalBombardmentResult& result
        );

        /// Calculate required fleet strength for effective blockade
        static uint32_t CalculateRequiredBlockadeStrength(
            const game::map::ProvinceData& target_port
        );

        /// Check if province is blockaded
        static bool IsProvinceBlockaded(
            game::types::EntityID province_id,
            const std::vector<BlockadeStatus>& active_blockades
        );
    };

} // namespace game::military
