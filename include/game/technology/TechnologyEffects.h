// ============================================================================
// TechnologyEffects.h - Specific Effects for Individual Technologies
// Created: October 31, 2025
// Location: include/game/technology/TechnologyEffects.h
// ============================================================================

#pragma once

#include "game/technology/TechnologyComponents.h"
#include "core/types/game_types.h"
#include <unordered_map>
#include <string>
#include <vector>

namespace game::technology {

// ============================================================================
// Technology Effect Types
// ============================================================================

enum class EffectType : uint8_t {
    PRODUCTION_BONUS,           // Increases resource production
    TRADE_EFFICIENCY,           // Improves trade income
    TAX_EFFICIENCY,             // Improves tax collection
    MILITARY_STRENGTH,          // Increases unit effectiveness
    MILITARY_DEFENSE,           // Improves defensive capabilities
    RESEARCH_SPEED,             // Increases research progress
    POPULATION_GROWTH,          // Increases population growth rate
    BUILDING_COST_REDUCTION,    // Reduces building costs
    UNIT_COST_REDUCTION,        // Reduces unit recruitment costs
    INFRASTRUCTURE_QUALITY,     // Improves infrastructure
    DIPLOMATIC_REPUTATION,      // Improves diplomatic standing
    KNOWLEDGE_TRANSMISSION,     // Improves knowledge spread
    INNOVATION_RATE,            // Increases innovation chance
    FOOD_PRODUCTION,            // Specific food production bonus
    MILITARY_MAINTENANCE,       // Reduces military upkeep costs
    FORTIFICATION_STRENGTH,     // Increases fort effectiveness
    NAVAL_STRENGTH,             // Increases naval power
    MARKET_ACCESS,              // Improves trade access
    ADMINISTRATIVE_CAPACITY,    // Increases province management capacity
    COUNT
};

// ============================================================================
// Technology Effect Structure
// ============================================================================

struct TechnologyEffect {
    EffectType type;
    double value;                   // The magnitude of the effect
    std::string description;        // Human-readable description
    std::string affected_system;    // Which system is affected (economy, military, etc.)

    TechnologyEffect() = default;
    TechnologyEffect(EffectType t, double v, const std::string& desc, const std::string& sys)
        : type(t), value(v), description(desc), affected_system(sys) {}
};

// ============================================================================
// Complete Technology Effects Database
// ============================================================================

class TechnologyEffectsDatabase {
public:
    // Get all effects for a specific technology
    static std::vector<TechnologyEffect> GetEffects(TechnologyType tech);

    // Get effects by category
    static std::vector<TechnologyEffect> GetCategoryEffects(TechnologyCategory category);

    // Check if a technology has a specific effect type
    static bool HasEffect(TechnologyType tech, EffectType effect);

    // Get the total value of a specific effect type from a technology
    static double GetEffectValue(TechnologyType tech, EffectType effect);

    // Get human-readable description of all effects
    static std::string GetEffectsDescription(TechnologyType tech);

private:
    // Initialize the effects database
    static void InitializeDatabase();

    // Database storage
    static std::unordered_map<TechnologyType, std::vector<TechnologyEffect>> s_effects_database;
    static bool s_initialized;
};

// ============================================================================
// Technology Effects Data - Agricultural Technologies (1000-1099)
// ============================================================================

inline std::vector<TechnologyEffect> GetThreeFieldSystemEffects() {
    return {
        TechnologyEffect(EffectType::FOOD_PRODUCTION, 0.25, "+25% food production", "economy"),
        TechnologyEffect(EffectType::PRODUCTION_BONUS, 0.15, "+15% agricultural productivity", "economy"),
        TechnologyEffect(EffectType::POPULATION_GROWTH, 0.10, "+10% population growth from better nutrition", "population")
    };
}

inline std::vector<TechnologyEffect> GetHeavyPlowEffects() {
    return {
        TechnologyEffect(EffectType::FOOD_PRODUCTION, 0.30, "+30% food production from heavy soils", "economy"),
        TechnologyEffect(EffectType::PRODUCTION_BONUS, 0.20, "+20% agricultural efficiency", "economy"),
        TechnologyEffect(EffectType::INFRASTRUCTURE_QUALITY, 0.05, "+5% rural infrastructure", "economy")
    };
}

inline std::vector<TechnologyEffect> GetHorseCollarEffects() {
    return {
        TechnologyEffect(EffectType::FOOD_PRODUCTION, 0.15, "+15% plowing efficiency", "economy"),
        TechnologyEffect(EffectType::PRODUCTION_BONUS, 0.10, "+10% agricultural labor efficiency", "economy"),
        TechnologyEffect(EffectType::TRADE_EFFICIENCY, 0.05, "+5% transport capacity", "economy")
    };
}

inline std::vector<TechnologyEffect> GetWindmillEffects() {
    return {
        TechnologyEffect(EffectType::FOOD_PRODUCTION, 0.20, "+20% grain processing efficiency", "economy"),
        TechnologyEffect(EffectType::PRODUCTION_BONUS, 0.15, "+15% craft production", "economy"),
        TechnologyEffect(EffectType::INFRASTRUCTURE_QUALITY, 0.10, "+10% rural development", "economy")
    };
}

inline std::vector<TechnologyEffect> GetWatermillEffects() {
    return {
        TechnologyEffect(EffectType::PRODUCTION_BONUS, 0.25, "+25% production from water power", "economy"),
        TechnologyEffect(EffectType::FOOD_PRODUCTION, 0.15, "+15% milling efficiency", "economy"),
        TechnologyEffect(EffectType::INFRASTRUCTURE_QUALITY, 0.08, "+8% infrastructure", "economy")
    };
}

inline std::vector<TechnologyEffect> GetCropRotationEffects() {
    return {
        TechnologyEffect(EffectType::FOOD_PRODUCTION, 0.35, "+35% sustainable food production", "economy"),
        TechnologyEffect(EffectType::POPULATION_GROWTH, 0.15, "+15% population growth", "population"),
        TechnologyEffect(EffectType::PRODUCTION_BONUS, 0.10, "+10% agricultural yield", "economy")
    };
}

inline std::vector<TechnologyEffect> GetSelectiveBreedingEffects() {
    return {
        TechnologyEffect(EffectType::FOOD_PRODUCTION, 0.20, "+20% livestock quality", "economy"),
        TechnologyEffect(EffectType::PRODUCTION_BONUS, 0.15, "+15% animal products", "economy"),
        TechnologyEffect(EffectType::RESEARCH_SPEED, 0.05, "+5% agricultural research", "technology")
    };
}

inline std::vector<TechnologyEffect> GetAgriculturalManualEffects() {
    return {
        TechnologyEffect(EffectType::FOOD_PRODUCTION, 0.15, "+15% farming efficiency from knowledge", "economy"),
        TechnologyEffect(EffectType::KNOWLEDGE_TRANSMISSION, 0.20, "+20% agricultural knowledge spread", "technology"),
        TechnologyEffect(EffectType::RESEARCH_SPEED, 0.10, "+10% agricultural research speed", "technology")
    };
}

inline std::vector<TechnologyEffect> GetIrrigationSystemsEffects() {
    return {
        TechnologyEffect(EffectType::FOOD_PRODUCTION, 0.40, "+40% food production in irrigated areas", "economy"),
        TechnologyEffect(EffectType::PRODUCTION_BONUS, 0.20, "+20% agricultural reliability", "economy"),
        TechnologyEffect(EffectType::INFRASTRUCTURE_QUALITY, 0.15, "+15% infrastructure development", "economy")
    };
}

inline std::vector<TechnologyEffect> GetNewWorldCropsEffects() {
    return {
        TechnologyEffect(EffectType::FOOD_PRODUCTION, 0.50, "+50% food production from new crops", "economy"),
        TechnologyEffect(EffectType::POPULATION_GROWTH, 0.25, "+25% population capacity", "population"),
        TechnologyEffect(EffectType::TRADE_EFFICIENCY, 0.10, "+10% trade in new goods", "economy")
    };
}

// ============================================================================
// Military Technologies (1100-1199)
// ============================================================================

inline std::vector<TechnologyEffect> GetChainmailArmorEffects() {
    return {
        TechnologyEffect(EffectType::MILITARY_DEFENSE, 0.20, "+20% troop defense", "military"),
        TechnologyEffect(EffectType::MILITARY_STRENGTH, 0.10, "+10% melee effectiveness", "military"),
        TechnologyEffect(EffectType::UNIT_COST_REDUCTION, -0.05, "-5% armor production cost", "economy")
    };
}

inline std::vector<TechnologyEffect> GetPlateArmorEffects() {
    return {
        TechnologyEffect(EffectType::MILITARY_DEFENSE, 0.40, "+40% heavy cavalry defense", "military"),
        TechnologyEffect(EffectType::MILITARY_STRENGTH, 0.25, "+25% knight effectiveness", "military"),
        TechnologyEffect(EffectType::DIPLOMATIC_REPUTATION, 0.10, "+10% military prestige", "diplomacy")
    };
}

inline std::vector<TechnologyEffect> GetCrossbowEffects() {
    return {
        TechnologyEffect(EffectType::MILITARY_STRENGTH, 0.30, "+30% ranged attack power", "military"),
        TechnologyEffect(EffectType::MILITARY_DEFENSE, 0.15, "+15% fortification defense", "military"),
        TechnologyEffect(EffectType::UNIT_COST_REDUCTION, 0.00, "Lower training requirements", "military")
    };
}

inline std::vector<TechnologyEffect> GetLongbowEffects() {
    return {
        TechnologyEffect(EffectType::MILITARY_STRENGTH, 0.35, "+35% archer effectiveness", "military"),
        TechnologyEffect(EffectType::MILITARY_DEFENSE, 0.10, "+10% defensive battles", "military"),
        TechnologyEffect(EffectType::UNIT_COST_REDUCTION, -0.10, "-10% archer recruitment cost", "economy")
    };
}

inline std::vector<TechnologyEffect> GetGunpowderEffects() {
    return {
        TechnologyEffect(EffectType::MILITARY_STRENGTH, 0.50, "+50% siege and battle effectiveness", "military"),
        TechnologyEffect(EffectType::FORTIFICATION_STRENGTH, -0.30, "-30% enemy fortification effectiveness", "military"),
        TechnologyEffect(EffectType::DIPLOMATIC_REPUTATION, 0.20, "+20% military intimidation", "diplomacy")
    };
}

inline std::vector<TechnologyEffect> GetCannonsEffects() {
    return {
        TechnologyEffect(EffectType::MILITARY_STRENGTH, 0.60, "+60% siege power", "military"),
        TechnologyEffect(EffectType::MILITARY_DEFENSE, 0.20, "+20% defensive artillery", "military"),
        TechnologyEffect(EffectType::FORTIFICATION_STRENGTH, -0.40, "-40% enemy fort strength", "military")
    };
}

inline std::vector<TechnologyEffect> GetArquebusEffects() {
    return {
        TechnologyEffect(EffectType::MILITARY_STRENGTH, 0.40, "+40% infantry firepower", "military"),
        TechnologyEffect(EffectType::MILITARY_DEFENSE, 0.15, "+15% defensive fire", "military"),
        TechnologyEffect(EffectType::UNIT_COST_REDUCTION, -0.05, "-5% gunpowder unit cost", "economy")
    };
}

inline std::vector<TechnologyEffect> GetMusketEffects() {
    return {
        TechnologyEffect(EffectType::MILITARY_STRENGTH, 0.50, "+50% musketeer effectiveness", "military"),
        TechnologyEffect(EffectType::MILITARY_DEFENSE, 0.25, "+25% line infantry defense", "military"),
        TechnologyEffect(EffectType::DIPLOMATIC_REPUTATION, 0.15, "+15% military reputation", "diplomacy")
    };
}

inline std::vector<TechnologyEffect> GetStarFortressEffects() {
    return {
        TechnologyEffect(EffectType::FORTIFICATION_STRENGTH, 0.80, "+80% fortification defense", "military"),
        TechnologyEffect(EffectType::MILITARY_DEFENSE, 0.50, "+50% siege defense", "military"),
        TechnologyEffect(EffectType::BUILDING_COST_REDUCTION, -0.10, "-10% fort maintenance", "economy")
    };
}

inline std::vector<TechnologyEffect> GetMilitaryEngineeringEffects() {
    return {
        TechnologyEffect(EffectType::MILITARY_STRENGTH, 0.30, "+30% siege effectiveness", "military"),
        TechnologyEffect(EffectType::FORTIFICATION_STRENGTH, 0.40, "+40% fortification quality", "military"),
        TechnologyEffect(EffectType::INFRASTRUCTURE_QUALITY, 0.15, "+15% military infrastructure", "economy")
    };
}

// ============================================================================
// Craft Technologies (1200-1299)
// ============================================================================

inline std::vector<TechnologyEffect> GetBlastFurnaceEffects() {
    return {
        TechnologyEffect(EffectType::PRODUCTION_BONUS, 0.40, "+40% metal production", "economy"),
        TechnologyEffect(EffectType::MILITARY_STRENGTH, 0.15, "+15% weapon quality", "military"),
        TechnologyEffect(EffectType::BUILDING_COST_REDUCTION, -0.10, "-10% metal-based construction costs", "economy")
    };
}

inline std::vector<TechnologyEffect> GetWaterPoweredMachineryEffects() {
    return {
        TechnologyEffect(EffectType::PRODUCTION_BONUS, 0.50, "+50% industrial output", "economy"),
        TechnologyEffect(EffectType::INFRASTRUCTURE_QUALITY, 0.20, "+20% infrastructure development", "economy"),
        TechnologyEffect(EffectType::INNOVATION_RATE, 0.15, "+15% innovation rate", "technology")
    };
}

inline std::vector<TechnologyEffect> GetMechanicalClockEffects() {
    return {
        TechnologyEffect(EffectType::PRODUCTION_BONUS, 0.15, "+15% precision crafts", "economy"),
        TechnologyEffect(EffectType::RESEARCH_SPEED, 0.20, "+20% research coordination", "technology"),
        TechnologyEffect(EffectType::ADMINISTRATIVE_CAPACITY, 0.10, "+10% administrative efficiency", "administration")
    };
}

inline std::vector<TechnologyEffect> GetPrintingPressEffects() {
    return {
        TechnologyEffect(EffectType::RESEARCH_SPEED, 0.50, "+50% knowledge dissemination", "technology"),
        TechnologyEffect(EffectType::KNOWLEDGE_TRANSMISSION, 0.60, "+60% knowledge spread rate", "technology"),
        TechnologyEffect(EffectType::INNOVATION_RATE, 0.30, "+30% innovation from information", "technology"),
        TechnologyEffect(EffectType::DIPLOMATIC_REPUTATION, 0.15, "+15% cultural influence", "diplomacy")
    };
}

inline std::vector<TechnologyEffect> GetDoubleEntryBookkeepingEffects() {
    return {
        TechnologyEffect(EffectType::TAX_EFFICIENCY, 0.30, "+30% financial management", "economy"),
        TechnologyEffect(EffectType::TRADE_EFFICIENCY, 0.25, "+25% merchant efficiency", "economy"),
        TechnologyEffect(EffectType::ADMINISTRATIVE_CAPACITY, 0.20, "+20% accounting accuracy", "administration")
    };
}

inline std::vector<TechnologyEffect> GetPaperMakingEffects() {
    return {
        TechnologyEffect(EffectType::RESEARCH_SPEED, 0.25, "+25% documentation speed", "technology"),
        TechnologyEffect(EffectType::KNOWLEDGE_TRANSMISSION, 0.30, "+30% knowledge preservation", "technology"),
        TechnologyEffect(EffectType::ADMINISTRATIVE_CAPACITY, 0.15, "+15% record keeping", "administration")
    };
}

inline std::vector<TechnologyEffect> GetGlassMakingEffects() {
    return {
        TechnologyEffect(EffectType::PRODUCTION_BONUS, 0.20, "+20% luxury goods production", "economy"),
        TechnologyEffect(EffectType::TRADE_EFFICIENCY, 0.15, "+15% trade in glass goods", "economy"),
        TechnologyEffect(EffectType::RESEARCH_SPEED, 0.10, "+10% optical research", "technology")
    };
}

inline std::vector<TechnologyEffect> GetTextileMachineryEffects() {
    return {
        TechnologyEffect(EffectType::PRODUCTION_BONUS, 0.45, "+45% textile production", "economy"),
        TechnologyEffect(EffectType::TRADE_EFFICIENCY, 0.20, "+20% cloth trade", "economy"),
        TechnologyEffect(EffectType::POPULATION_GROWTH, 0.10, "+10% employment opportunities", "population")
    };
}

inline std::vector<TechnologyEffect> GetAdvancedMetallurgyEffects() {
    return {
        TechnologyEffect(EffectType::PRODUCTION_BONUS, 0.35, "+35% metal quality", "economy"),
        TechnologyEffect(EffectType::MILITARY_STRENGTH, 0.20, "+20% weapon and armor quality", "military"),
        TechnologyEffect(EffectType::BUILDING_COST_REDUCTION, -0.15, "-15% construction costs", "economy")
    };
}

inline std::vector<TechnologyEffect> GetPrecisionInstrumentsEffects() {
    return {
        TechnologyEffect(EffectType::RESEARCH_SPEED, 0.30, "+30% scientific research", "technology"),
        TechnologyEffect(EffectType::PRODUCTION_BONUS, 0.20, "+20% precision manufacturing", "economy"),
        TechnologyEffect(EffectType::INNOVATION_RATE, 0.25, "+25% technological innovation", "technology")
    };
}

// ============================================================================
// Administrative Technologies (1300-1399)
// ============================================================================

inline std::vector<TechnologyEffect> GetWrittenLawCodesEffects() {
    return {
        TechnologyEffect(EffectType::ADMINISTRATIVE_CAPACITY, 0.25, "+25% legal efficiency", "administration"),
        TechnologyEffect(EffectType::TAX_EFFICIENCY, 0.15, "+15% tax compliance", "economy"),
        TechnologyEffect(EffectType::DIPLOMATIC_REPUTATION, 0.10, "+10% diplomatic standing", "diplomacy")
    };
}

inline std::vector<TechnologyEffect> GetBureaucraticAdministrationEffects() {
    return {
        TechnologyEffect(EffectType::ADMINISTRATIVE_CAPACITY, 0.40, "+40% administrative efficiency", "administration"),
        TechnologyEffect(EffectType::TAX_EFFICIENCY, 0.30, "+30% tax collection", "economy"),
        TechnologyEffect(EffectType::INFRASTRUCTURE_QUALITY, 0.15, "+15% state organization", "economy")
    };
}

inline std::vector<TechnologyEffect> GetCensusTechniquesEffects() {
    return {
        TechnologyEffect(EffectType::ADMINISTRATIVE_CAPACITY, 0.30, "+30% population management", "administration"),
        TechnologyEffect(EffectType::TAX_EFFICIENCY, 0.25, "+25% accurate taxation", "economy"),
        TechnologyEffect(EffectType::MILITARY_STRENGTH, 0.10, "+10% recruitment efficiency", "military")
    };
}

inline std::vector<TechnologyEffect> GetTaxCollectionSystemsEffects() {
    return {
        TechnologyEffect(EffectType::TAX_EFFICIENCY, 0.50, "+50% tax collection efficiency", "economy"),
        TechnologyEffect(EffectType::ADMINISTRATIVE_CAPACITY, 0.20, "+20% fiscal administration", "administration"),
        TechnologyEffect(EffectType::DIPLOMATIC_REPUTATION, -0.05, "-5% popular satisfaction", "diplomacy")
    };
}

inline std::vector<TechnologyEffect> GetDiplomaticProtocolsEffects() {
    return {
        TechnologyEffect(EffectType::DIPLOMATIC_REPUTATION, 0.40, "+40% diplomatic effectiveness", "diplomacy"),
        TechnologyEffect(EffectType::TRADE_EFFICIENCY, 0.15, "+15% diplomatic trade", "economy"),
        TechnologyEffect(EffectType::KNOWLEDGE_TRANSMISSION, 0.20, "+20% diplomatic knowledge exchange", "technology")
    };
}

inline std::vector<TechnologyEffect> GetRecordKeepingEffects() {
    return {
        TechnologyEffect(EffectType::ADMINISTRATIVE_CAPACITY, 0.35, "+35% administrative memory", "administration"),
        TechnologyEffect(EffectType::TAX_EFFICIENCY, 0.20, "+20% financial records", "economy"),
        TechnologyEffect(EffectType::KNOWLEDGE_TRANSMISSION, 0.15, "+15% information preservation", "technology")
    };
}

inline std::vector<TechnologyEffect> GetStandardizedWeightsEffects() {
    return {
        TechnologyEffect(EffectType::TRADE_EFFICIENCY, 0.30, "+30% trade standardization", "economy"),
        TechnologyEffect(EffectType::TAX_EFFICIENCY, 0.15, "+15% fair taxation", "economy"),
        TechnologyEffect(EffectType::DIPLOMATIC_REPUTATION, 0.10, "+10% commercial reputation", "diplomacy")
    };
}

inline std::vector<TechnologyEffect> GetPostalSystemsEffects() {
    return {
        TechnologyEffect(EffectType::ADMINISTRATIVE_CAPACITY, 0.30, "+30% communication speed", "administration"),
        TechnologyEffect(EffectType::TRADE_EFFICIENCY, 0.20, "+20% trade coordination", "economy"),
        TechnologyEffect(EffectType::KNOWLEDGE_TRANSMISSION, 0.25, "+25% information spread", "technology"),
        TechnologyEffect(EffectType::DIPLOMATIC_REPUTATION, 0.15, "+15% diplomatic communication", "diplomacy")
    };
}

inline std::vector<TechnologyEffect> GetProfessionalArmyEffects() {
    return {
        TechnologyEffect(EffectType::MILITARY_STRENGTH, 0.45, "+45% army effectiveness", "military"),
        TechnologyEffect(EffectType::MILITARY_DEFENSE, 0.30, "+30% discipline and training", "military"),
        TechnologyEffect(EffectType::MILITARY_MAINTENANCE, -0.15, "-15% long-term military costs", "economy"),
        TechnologyEffect(EffectType::ADMINISTRATIVE_CAPACITY, 0.15, "+15% military administration", "administration")
    };
}

inline std::vector<TechnologyEffect> GetStateMonopoliesEffects() {
    return {
        TechnologyEffect(EffectType::TAX_EFFICIENCY, 0.35, "+35% state revenue", "economy"),
        TechnologyEffect(EffectType::PRODUCTION_BONUS, 0.20, "+20% controlled production", "economy"),
        TechnologyEffect(EffectType::ADMINISTRATIVE_CAPACITY, 0.15, "+15% economic control", "administration")
    };
}

// ============================================================================
// Academic Technologies (1400-1499)
// ============================================================================

inline std::vector<TechnologyEffect> GetScholasticMethodEffects() {
    return {
        TechnologyEffect(EffectType::RESEARCH_SPEED, 0.35, "+35% systematic research", "technology"),
        TechnologyEffect(EffectType::KNOWLEDGE_TRANSMISSION, 0.25, "+25% teaching efficiency", "technology"),
        TechnologyEffect(EffectType::INNOVATION_RATE, 0.20, "+20% academic innovation", "technology")
    };
}

inline std::vector<TechnologyEffect> GetUniversitySystemEffects() {
    return {
        TechnologyEffect(EffectType::RESEARCH_SPEED, 0.50, "+50% research capacity", "technology"),
        TechnologyEffect(EffectType::KNOWLEDGE_TRANSMISSION, 0.40, "+40% knowledge institutionalization", "technology"),
        TechnologyEffect(EffectType::INNOVATION_RATE, 0.30, "+30% breakthrough chance", "technology"),
        TechnologyEffect(EffectType::DIPLOMATIC_REPUTATION, 0.20, "+20% cultural prestige", "diplomacy")
    };
}

inline std::vector<TechnologyEffect> GetVernacularWritingEffects() {
    return {
        TechnologyEffect(EffectType::KNOWLEDGE_TRANSMISSION, 0.45, "+45% popular education", "technology"),
        TechnologyEffect(EffectType::ADMINISTRATIVE_CAPACITY, 0.20, "+20% administrative literacy", "administration"),
        TechnologyEffect(EffectType::POPULATION_GROWTH, 0.10, "+10% educated population", "population")
    };
}

inline std::vector<TechnologyEffect> GetNaturalPhilosophyEffects() {
    return {
        TechnologyEffect(EffectType::RESEARCH_SPEED, 0.40, "+40% scientific inquiry", "technology"),
        TechnologyEffect(EffectType::INNOVATION_RATE, 0.35, "+35% scientific innovation", "technology"),
        TechnologyEffect(EffectType::PRODUCTION_BONUS, 0.15, "+15% applied science", "economy")
    };
}

inline std::vector<TechnologyEffect> GetMathematicalNotationEffects() {
    return {
        TechnologyEffect(EffectType::RESEARCH_SPEED, 0.30, "+30% mathematical progress", "technology"),
        TechnologyEffect(EffectType::ADMINISTRATIVE_CAPACITY, 0.15, "+15% financial calculations", "administration"),
        TechnologyEffect(EffectType::INNOVATION_RATE, 0.25, "+25% technical innovation", "technology")
    };
}

inline std::vector<TechnologyEffect> GetExperimentalMethodEffects() {
    return {
        TechnologyEffect(EffectType::RESEARCH_SPEED, 0.45, "+45% empirical research", "technology"),
        TechnologyEffect(EffectType::INNOVATION_RATE, 0.50, "+50% experimental breakthroughs", "technology"),
        TechnologyEffect(EffectType::PRODUCTION_BONUS, 0.20, "+20% practical applications", "economy")
    };
}

inline std::vector<TechnologyEffect> GetHumanistEducationEffects() {
    return {
        TechnologyEffect(EffectType::KNOWLEDGE_TRANSMISSION, 0.35, "+35% educational quality", "technology"),
        TechnologyEffect(EffectType::DIPLOMATIC_REPUTATION, 0.25, "+25% cultural sophistication", "diplomacy"),
        TechnologyEffect(EffectType::ADMINISTRATIVE_CAPACITY, 0.20, "+20% educated officials", "administration")
    };
}

inline std::vector<TechnologyEffect> GetScientificInstrumentsEffects() {
    return {
        TechnologyEffect(EffectType::RESEARCH_SPEED, 0.40, "+40% precision research", "technology"),
        TechnologyEffect(EffectType::INNOVATION_RATE, 0.35, "+35% instrumental discovery", "technology"),
        TechnologyEffect(EffectType::PRODUCTION_BONUS, 0.15, "+15% precision manufacturing", "economy")
    };
}

inline std::vector<TechnologyEffect> GetOpticalDevicesEffects() {
    return {
        TechnologyEffect(EffectType::RESEARCH_SPEED, 0.35, "+35% observational science", "technology"),
        TechnologyEffect(EffectType::MILITARY_STRENGTH, 0.10, "+10% reconnaissance", "military"),
        TechnologyEffect(EffectType::NAVAL_STRENGTH, 0.15, "+15% navigation", "military")
    };
}

inline std::vector<TechnologyEffect> GetCartographyEffects() {
    return {
        TechnologyEffect(EffectType::TRADE_EFFICIENCY, 0.30, "+30% trade route efficiency", "economy"),
        TechnologyEffect(EffectType::MILITARY_STRENGTH, 0.20, "+20% strategic planning", "military"),
        TechnologyEffect(EffectType::DIPLOMATIC_REPUTATION, 0.15, "+15% exploration prestige", "diplomacy"),
        TechnologyEffect(EffectType::ADMINISTRATIVE_CAPACITY, 0.15, "+15% territorial management", "administration")
    };
}

// ============================================================================
// Naval Technologies (1500-1599)
// ============================================================================

inline std::vector<TechnologyEffect> GetImprovedShipDesignEffects() {
    return {
        TechnologyEffect(EffectType::NAVAL_STRENGTH, 0.35, "+35% ship performance", "military"),
        TechnologyEffect(EffectType::TRADE_EFFICIENCY, 0.25, "+25% maritime trade", "economy"),
        TechnologyEffect(EffectType::MILITARY_STRENGTH, 0.15, "+15% naval combat", "military")
    };
}

inline std::vector<TechnologyEffect> GetNavigationInstrumentsEffects() {
    return {
        TechnologyEffect(EffectType::NAVAL_STRENGTH, 0.30, "+30% navigation accuracy", "military"),
        TechnologyEffect(EffectType::TRADE_EFFICIENCY, 0.30, "+30% sea trade safety", "economy"),
        TechnologyEffect(EffectType::DIPLOMATIC_REPUTATION, 0.15, "+15% maritime reputation", "diplomacy")
    };
}

inline std::vector<TechnologyEffect> GetCompassNavigationEffects() {
    return {
        TechnologyEffect(EffectType::NAVAL_STRENGTH, 0.40, "+40% all-weather sailing", "military"),
        TechnologyEffect(EffectType::TRADE_EFFICIENCY, 0.35, "+35% reliable trade routes", "economy"),
        TechnologyEffect(EffectType::MARKET_ACCESS, 0.25, "+25% new market access", "economy")
    };
}

inline std::vector<TechnologyEffect> GetNavalArtilleryEffects() {
    return {
        TechnologyEffect(EffectType::NAVAL_STRENGTH, 0.60, "+60% naval firepower", "military"),
        TechnologyEffect(EffectType::MILITARY_STRENGTH, 0.30, "+30% sea battle effectiveness", "military"),
        TechnologyEffect(EffectType::DIPLOMATIC_REPUTATION, 0.20, "+20% naval intimidation", "diplomacy")
    };
}

inline std::vector<TechnologyEffect> GetOceanNavigationEffects() {
    return {
        TechnologyEffect(EffectType::NAVAL_STRENGTH, 0.50, "+50% ocean-going capability", "military"),
        TechnologyEffect(EffectType::TRADE_EFFICIENCY, 0.45, "+45% long-distance trade", "economy"),
        TechnologyEffect(EffectType::MARKET_ACCESS, 0.40, "+40% global trade access", "economy"),
        TechnologyEffect(EffectType::DIPLOMATIC_REPUTATION, 0.25, "+25% maritime empire", "diplomacy")
    };
}

inline std::vector<TechnologyEffect> GetShipyardTechniquesEffects() {
    return {
        TechnologyEffect(EffectType::NAVAL_STRENGTH, 0.25, "+25% ship quality", "military"),
        TechnologyEffect(EffectType::PRODUCTION_BONUS, 0.20, "+20% shipbuilding efficiency", "economy"),
        TechnologyEffect(EffectType::BUILDING_COST_REDUCTION, -0.15, "-15% ship construction costs", "economy")
    };
}

inline std::vector<TechnologyEffect> GetMaritimeLawEffects() {
    return {
        TechnologyEffect(EffectType::TRADE_EFFICIENCY, 0.30, "+30% maritime commerce", "economy"),
        TechnologyEffect(EffectType::DIPLOMATIC_REPUTATION, 0.25, "+25% maritime diplomacy", "diplomacy"),
        TechnologyEffect(EffectType::ADMINISTRATIVE_CAPACITY, 0.15, "+15% port administration", "administration")
    };
}

inline std::vector<TechnologyEffect> GetNavalTacticsEffects() {
    return {
        TechnologyEffect(EffectType::NAVAL_STRENGTH, 0.45, "+45% naval combat effectiveness", "military"),
        TechnologyEffect(EffectType::MILITARY_STRENGTH, 0.25, "+25% fleet coordination", "military"),
        TechnologyEffect(EffectType::DIPLOMATIC_REPUTATION, 0.15, "+15% naval prestige", "diplomacy")
    };
}

inline std::vector<TechnologyEffect> GetLighthouseSystemsEffects() {
    return {
        TechnologyEffect(EffectType::TRADE_EFFICIENCY, 0.25, "+25% safe harbor access", "economy"),
        TechnologyEffect(EffectType::NAVAL_STRENGTH, 0.20, "+20% coastal navigation", "military"),
        TechnologyEffect(EffectType::INFRASTRUCTURE_QUALITY, 0.20, "+20% coastal infrastructure", "economy")
    };
}

inline std::vector<TechnologyEffect> GetHarborEngineeringEffects() {
    return {
        TechnologyEffect(EffectType::TRADE_EFFICIENCY, 0.40, "+40% port capacity", "economy"),
        TechnologyEffect(EffectType::NAVAL_STRENGTH, 0.30, "+30% naval base efficiency", "military"),
        TechnologyEffect(EffectType::INFRASTRUCTURE_QUALITY, 0.25, "+25% maritime infrastructure", "economy"),
        TechnologyEffect(EffectType::PRODUCTION_BONUS, 0.15, "+15% port-based production", "economy")
    };
}

} // namespace game::technology
