// ============================================================================
// TechnologyPrerequisites.h - Technology Prerequisites Database
// Created: 2025-01-19
// Location: include/game/technology/TechnologyPrerequisites.h
// Defines prerequisite relationships between technologies
// ============================================================================

#pragma once

#include "game/technology/TechnologyComponents.h"
#include <unordered_map>
#include <vector>

namespace game::technology {

// ============================================================================
// Prerequisites Database
// ============================================================================

class TechnologyPrerequisites {
public:
    // Get prerequisites for a specific technology
    static std::vector<TechnologyType> GetPrerequisites(TechnologyType tech);

    // Check if a technology has prerequisites
    static bool HasPrerequisites(TechnologyType tech);

    // Get all technologies that unlock when this technology is discovered
    static std::vector<TechnologyType> GetUnlockedTechnologies(TechnologyType tech);

private:
    static void InitializeDatabase();
    static std::unordered_map<TechnologyType, std::vector<TechnologyType>> s_prerequisites_database;
    static bool s_initialized;
};

// ============================================================================
// Prerequisites Data
// ============================================================================

// Agricultural Technologies
inline std::vector<TechnologyType> GetThreeFieldSystemPrerequisites() {
    return {}; // Starting technology
}

inline std::vector<TechnologyType> GetHeavyPlowPrerequisites() {
    return {}; // Starting technology
}

inline std::vector<TechnologyType> GetHorseCollarPrerequisites() {
    return { TechnologyType::HEAVY_PLOW };
}

inline std::vector<TechnologyType> GetWindmillPrerequisites() {
    return {}; // Independent discovery
}

inline std::vector<TechnologyType> GetWatermillPrerequisites() {
    return {}; // Independent discovery
}

inline std::vector<TechnologyType> GetCropRotationPrerequisites() {
    return { TechnologyType::THREE_FIELD_SYSTEM };
}

inline std::vector<TechnologyType> GetSelectiveBreedingPrerequisites() {
    return { TechnologyType::THREE_FIELD_SYSTEM };
}

inline std::vector<TechnologyType> GetAgriculturalManualPrerequisites() {
    return { TechnologyType::CROP_ROTATION };
}

inline std::vector<TechnologyType> GetIrrigationSystemsPrerequisites() {
    return { TechnologyType::WATERMILL };
}

inline std::vector<TechnologyType> GetNewWorldCropsPrerequisites() {
    return { TechnologyType::OCEAN_NAVIGATION }; // Requires exploration
}

// Military Technologies
inline std::vector<TechnologyType> GetChainmailArmorPrerequisites() {
    return {}; // Starting technology
}

inline std::vector<TechnologyType> GetPlateArmorPrerequisites() {
    return {
        TechnologyType::CHAINMAIL_ARMOR,
        TechnologyType::BLAST_FURNACE
    };
}

inline std::vector<TechnologyType> GetCrossbowPrerequisites() {
    return {}; // Starting technology
}

inline std::vector<TechnologyType> GetLongbowPrerequisites() {
    return {}; // Independent discovery
}

inline std::vector<TechnologyType> GetGunpowderPrerequisites() {
    return {}; // Independent discovery (historically from trade)
}

inline std::vector<TechnologyType> GetCannonsPrerequisites() {
    return {
        TechnologyType::GUNPOWDER,
        TechnologyType::BLAST_FURNACE
    };
}

inline std::vector<TechnologyType> GetArquebusPrerequisites() {
    return { TechnologyType::GUNPOWDER };
}

inline std::vector<TechnologyType> GetMusketPrerequisites() {
    return { TechnologyType::ARQUEBUS };
}

inline std::vector<TechnologyType> GetStarFortressPrerequisites() {
    return {
        TechnologyType::CANNONS,
        TechnologyType::MILITARY_ENGINEERING
    };
}

inline std::vector<TechnologyType> GetMilitaryEngineeringPrerequisites() {
    return { TechnologyType::WRITTEN_LAW_CODES };
}

// Craft Technologies
inline std::vector<TechnologyType> GetBlastFurnacePrerequisites() {
    return {}; // Independent discovery
}

inline std::vector<TechnologyType> GetWaterPoweredMachineryPrerequisites() {
    return { TechnologyType::WATERMILL };
}

inline std::vector<TechnologyType> GetMechanicalClockPrerequisites() {
    return { TechnologyType::WATER_POWERED_MACHINERY };
}

inline std::vector<TechnologyType> GetPrintingPressPrerequisites() {
    return {
        TechnologyType::PAPER_MAKING,
        TechnologyType::MECHANICAL_CLOCK
    };
}

inline std::vector<TechnologyType> GetDoubleEntryBookkeepingPrerequisites() {
    return { TechnologyType::WRITTEN_LAW_CODES };
}

inline std::vector<TechnologyType> GetPaperMakingPrerequisites() {
    return {}; // Independent discovery (historically from trade)
}

inline std::vector<TechnologyType> GetGlassMakingPrerequisites() {
    return {}; // Starting technology
}

inline std::vector<TechnologyType> GetTextileMachineryPrerequisites() {
    return { TechnologyType::WATER_POWERED_MACHINERY };
}

inline std::vector<TechnologyType> GetAdvancedMetallurgyPrerequisites() {
    return { TechnologyType::BLAST_FURNACE };
}

inline std::vector<TechnologyType> GetPrecisionInstrumentsPrerequisites() {
    return {
        TechnologyType::MECHANICAL_CLOCK,
        TechnologyType::ADVANCED_METALLURGY
    };
}

// Administrative Technologies
inline std::vector<TechnologyType> GetWrittenLawCodesPrerequisites() {
    return {}; // Starting technology
}

inline std::vector<TechnologyType> GetBureaucraticAdministrationPrerequisites() {
    return { TechnologyType::WRITTEN_LAW_CODES };
}

inline std::vector<TechnologyType> GetCensusTechniquesPrerequisites() {
    return {
        TechnologyType::BUREAUCRATIC_ADMINISTRATION,
        TechnologyType::RECORD_KEEPING
    };
}

inline std::vector<TechnologyType> GetTaxCollectionSystemsPrerequisites() {
    return { TechnologyType::WRITTEN_LAW_CODES };
}

inline std::vector<TechnologyType> GetDiplomaticProtocolsPrerequisites() {
    return { TechnologyType::WRITTEN_LAW_CODES };
}

inline std::vector<TechnologyType> GetRecordKeepingPrerequisites() {
    return { TechnologyType::PAPER_MAKING };
}

inline std::vector<TechnologyType> GetStandardizedWeightsPrerequisites() {
    return { TechnologyType::WRITTEN_LAW_CODES };
}

inline std::vector<TechnologyType> GetPostalSystemsPrerequisites() {
    return {
        TechnologyType::BUREAUCRATIC_ADMINISTRATION,
        TechnologyType::RECORD_KEEPING
    };
}

inline std::vector<TechnologyType> GetProfessionalArmyPrerequisites() {
    return {
        TechnologyType::BUREAUCRATIC_ADMINISTRATION,
        TechnologyType::TAX_COLLECTION_SYSTEMS
    };
}

inline std::vector<TechnologyType> GetStateMonopoliesPrerequisites() {
    return { TechnologyType::TAX_COLLECTION_SYSTEMS };
}

// Academic Technologies
inline std::vector<TechnologyType> GetScholasticMethodPrerequisites() {
    return {}; // Starting technology
}

inline std::vector<TechnologyType> GetUniversitySystemPrerequisites() {
    return { TechnologyType::SCHOLASTIC_METHOD };
}

inline std::vector<TechnologyType> GetVernacularWritingPrerequisites() {
    return { TechnologyType::PRINTING_PRESS };
}

inline std::vector<TechnologyType> GetNaturalPhilosophyPrerequisites() {
    return { TechnologyType::UNIVERSITY_SYSTEM };
}

inline std::vector<TechnologyType> GetMathematicalNotationPrerequisites() {
    return { TechnologyType::UNIVERSITY_SYSTEM };
}

inline std::vector<TechnologyType> GetExperimentalMethodPrerequisites() {
    return {
        TechnologyType::NATURAL_PHILOSOPHY,
        TechnologyType::MATHEMATICAL_NOTATION
    };
}

inline std::vector<TechnologyType> GetHumanistEducationPrerequisites() {
    return {
        TechnologyType::UNIVERSITY_SYSTEM,
        TechnologyType::VERNACULAR_WRITING
    };
}

inline std::vector<TechnologyType> GetScientificInstrumentsPrerequisites() {
    return {
        TechnologyType::EXPERIMENTAL_METHOD,
        TechnologyType::PRECISION_INSTRUMENTS
    };
}

inline std::vector<TechnologyType> GetOpticalDevicesPrerequisites() {
    return {
        TechnologyType::GLASS_MAKING,
        TechnologyType::PRECISION_INSTRUMENTS
    };
}

inline std::vector<TechnologyType> GetCartographyPrerequisites() {
    return {
        TechnologyType::MATHEMATICAL_NOTATION,
        TechnologyType::PRECISION_INSTRUMENTS
    };
}

// Naval Technologies
inline std::vector<TechnologyType> GetImprovedShipDesignPrerequisites() {
    return {}; // Starting technology
}

inline std::vector<TechnologyType> GetNavigationInstrumentsPrerequisites() {
    return { TechnologyType::IMPROVED_SHIP_DESIGN };
}

inline std::vector<TechnologyType> GetCompassNavigationPrerequisites() {
    return { TechnologyType::NAVIGATION_INSTRUMENTS };
}

inline std::vector<TechnologyType> GetNavalArtilleryPrerequisites() {
    return {
        TechnologyType::CANNONS,
        TechnologyType::IMPROVED_SHIP_DESIGN
    };
}

inline std::vector<TechnologyType> GetOceanNavigationPrerequisites() {
    return {
        TechnologyType::COMPASS_NAVIGATION,
        TechnologyType::CARTOGRAPHY
    };
}

inline std::vector<TechnologyType> GetShipyardTechniquesPrerequisites() {
    return { TechnologyType::IMPROVED_SHIP_DESIGN };
}

inline std::vector<TechnologyType> GetMaritimeLawPrerequisites() {
    return {
        TechnologyType::WRITTEN_LAW_CODES,
        TechnologyType::IMPROVED_SHIP_DESIGN
    };
}

inline std::vector<TechnologyType> GetNavalTacticsPrerequisites() {
    return { TechnologyType::NAVAL_ARTILLERY };
}

inline std::vector<TechnologyType> GetLighthouseSystemsPrerequisites() {
    return { TechnologyType::NAVIGATION_INSTRUMENTS };
}

inline std::vector<TechnologyType> GetHarborEngineeringPrerequisites() {
    return {
        TechnologyType::IMPROVED_SHIP_DESIGN,
        TechnologyType::MILITARY_ENGINEERING
    };
}

} // namespace game::technology
