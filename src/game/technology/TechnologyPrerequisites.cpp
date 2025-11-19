// ============================================================================
// TechnologyPrerequisites.cpp - Implementation of Prerequisites Database
// Created: 2025-01-19
// Location: src/game/technology/TechnologyPrerequisites.cpp
// ============================================================================

#include "game/technology/TechnologyPrerequisites.h"

namespace game::technology {

// Static member initialization
std::unordered_map<TechnologyType, std::vector<TechnologyType>> TechnologyPrerequisites::s_prerequisites_database;
bool TechnologyPrerequisites::s_initialized = false;

// ============================================================================
// Database Initialization
// ============================================================================

void TechnologyPrerequisites::InitializeDatabase() {
    if (s_initialized) return;

    // Agricultural Technologies
    s_prerequisites_database[TechnologyType::THREE_FIELD_SYSTEM] = GetThreeFieldSystemPrerequisites();
    s_prerequisites_database[TechnologyType::HEAVY_PLOW] = GetHeavyPlowPrerequisites();
    s_prerequisites_database[TechnologyType::HORSE_COLLAR] = GetHorseCollarPrerequisites();
    s_prerequisites_database[TechnologyType::WINDMILL] = GetWindmillPrerequisites();
    s_prerequisites_database[TechnologyType::WATERMILL] = GetWatermillPrerequisites();
    s_prerequisites_database[TechnologyType::CROP_ROTATION] = GetCropRotationPrerequisites();
    s_prerequisites_database[TechnologyType::SELECTIVE_BREEDING] = GetSelectiveBreedingPrerequisites();
    s_prerequisites_database[TechnologyType::AGRICULTURAL_MANUAL] = GetAgriculturalManualPrerequisites();
    s_prerequisites_database[TechnologyType::IRRIGATION_SYSTEMS] = GetIrrigationSystemsPrerequisites();
    s_prerequisites_database[TechnologyType::NEW_WORLD_CROPS] = GetNewWorldCropsPrerequisites();

    // Military Technologies
    s_prerequisites_database[TechnologyType::CHAINMAIL_ARMOR] = GetChainmailArmorPrerequisites();
    s_prerequisites_database[TechnologyType::PLATE_ARMOR] = GetPlateArmorPrerequisites();
    s_prerequisites_database[TechnologyType::CROSSBOW] = GetCrossbowPrerequisites();
    s_prerequisites_database[TechnologyType::LONGBOW] = GetLongbowPrerequisites();
    s_prerequisites_database[TechnologyType::GUNPOWDER] = GetGunpowderPrerequisites();
    s_prerequisites_database[TechnologyType::CANNONS] = GetCannonsPrerequisites();
    s_prerequisites_database[TechnologyType::ARQUEBUS] = GetArquebusPrerequisites();
    s_prerequisites_database[TechnologyType::MUSKET] = GetMusketPrerequisites();
    s_prerequisites_database[TechnologyType::STAR_FORTRESS] = GetStarFortressPrerequisites();
    s_prerequisites_database[TechnologyType::MILITARY_ENGINEERING] = GetMilitaryEngineeringPrerequisites();

    // Craft Technologies
    s_prerequisites_database[TechnologyType::BLAST_FURNACE] = GetBlastFurnacePrerequisites();
    s_prerequisites_database[TechnologyType::WATER_POWERED_MACHINERY] = GetWaterPoweredMachineryPrerequisites();
    s_prerequisites_database[TechnologyType::MECHANICAL_CLOCK] = GetMechanicalClockPrerequisites();
    s_prerequisites_database[TechnologyType::PRINTING_PRESS] = GetPrintingPressPrerequisites();
    s_prerequisites_database[TechnologyType::DOUBLE_ENTRY_BOOKKEEPING] = GetDoubleEntryBookkeepingPrerequisites();
    s_prerequisites_database[TechnologyType::PAPER_MAKING] = GetPaperMakingPrerequisites();
    s_prerequisites_database[TechnologyType::GLASS_MAKING] = GetGlassMakingPrerequisites();
    s_prerequisites_database[TechnologyType::TEXTILE_MACHINERY] = GetTextileMachineryPrerequisites();
    s_prerequisites_database[TechnologyType::ADVANCED_METALLURGY] = GetAdvancedMetallurgyPrerequisites();
    s_prerequisites_database[TechnologyType::PRECISION_INSTRUMENTS] = GetPrecisionInstrumentsPrerequisites();

    // Administrative Technologies
    s_prerequisites_database[TechnologyType::WRITTEN_LAW_CODES] = GetWrittenLawCodesPrerequisites();
    s_prerequisites_database[TechnologyType::BUREAUCRATIC_ADMINISTRATION] = GetBureaucraticAdministrationPrerequisites();
    s_prerequisites_database[TechnologyType::CENSUS_TECHNIQUES] = GetCensusTechniquesPrerequisites();
    s_prerequisites_database[TechnologyType::TAX_COLLECTION_SYSTEMS] = GetTaxCollectionSystemsPrerequisites();
    s_prerequisites_database[TechnologyType::DIPLOMATIC_PROTOCOLS] = GetDiplomaticProtocolsPrerequisites();
    s_prerequisites_database[TechnologyType::RECORD_KEEPING] = GetRecordKeepingPrerequisites();
    s_prerequisites_database[TechnologyType::STANDARDIZED_WEIGHTS] = GetStandardizedWeightsPrerequisites();
    s_prerequisites_database[TechnologyType::POSTAL_SYSTEMS] = GetPostalSystemsPrerequisites();
    s_prerequisites_database[TechnologyType::PROFESSIONAL_ARMY] = GetProfessionalArmyPrerequisites();
    s_prerequisites_database[TechnologyType::STATE_MONOPOLIES] = GetStateMonopoliesPrerequisites();

    // Academic Technologies
    s_prerequisites_database[TechnologyType::SCHOLASTIC_METHOD] = GetScholasticMethodPrerequisites();
    s_prerequisites_database[TechnologyType::UNIVERSITY_SYSTEM] = GetUniversitySystemPrerequisites();
    s_prerequisites_database[TechnologyType::VERNACULAR_WRITING] = GetVernacularWritingPrerequisites();
    s_prerequisites_database[TechnologyType::NATURAL_PHILOSOPHY] = GetNaturalPhilosophyPrerequisites();
    s_prerequisites_database[TechnologyType::MATHEMATICAL_NOTATION] = GetMathematicalNotationPrerequisites();
    s_prerequisites_database[TechnologyType::EXPERIMENTAL_METHOD] = GetExperimentalMethodPrerequisites();
    s_prerequisites_database[TechnologyType::HUMANIST_EDUCATION] = GetHumanistEducationPrerequisites();
    s_prerequisites_database[TechnologyType::SCIENTIFIC_INSTRUMENTS] = GetScientificInstrumentsPrerequisites();
    s_prerequisites_database[TechnologyType::OPTICAL_DEVICES] = GetOpticalDevicesPrerequisites();
    s_prerequisites_database[TechnologyType::CARTOGRAPHY] = GetCartographyPrerequisites();

    // Naval Technologies
    s_prerequisites_database[TechnologyType::IMPROVED_SHIP_DESIGN] = GetImprovedShipDesignPrerequisites();
    s_prerequisites_database[TechnologyType::NAVIGATION_INSTRUMENTS] = GetNavigationInstrumentsPrerequisites();
    s_prerequisites_database[TechnologyType::COMPASS_NAVIGATION] = GetCompassNavigationPrerequisites();
    s_prerequisites_database[TechnologyType::NAVAL_ARTILLERY] = GetNavalArtilleryPrerequisites();
    s_prerequisites_database[TechnologyType::OCEAN_NAVIGATION] = GetOceanNavigationPrerequisites();
    s_prerequisites_database[TechnologyType::SHIPYARD_TECHNIQUES] = GetShipyardTechniquesPrerequisites();
    s_prerequisites_database[TechnologyType::MARITIME_LAW] = GetMaritimeLawPrerequisites();
    s_prerequisites_database[TechnologyType::NAVAL_TACTICS] = GetNavalTacticsPrerequisites();
    s_prerequisites_database[TechnologyType::LIGHTHOUSE_SYSTEMS] = GetLighthouseSystemsPrerequisites();
    s_prerequisites_database[TechnologyType::HARBOR_ENGINEERING] = GetHarborEngineeringPrerequisites();

    s_initialized = true;
}

// ============================================================================
// Public Interface Methods
// ============================================================================

std::vector<TechnologyType> TechnologyPrerequisites::GetPrerequisites(TechnologyType tech) {
    InitializeDatabase();

    auto it = s_prerequisites_database.find(tech);
    if (it != s_prerequisites_database.end()) {
        return it->second;
    }

    return std::vector<TechnologyType>();
}

bool TechnologyPrerequisites::HasPrerequisites(TechnologyType tech) {
    InitializeDatabase();

    auto it = s_prerequisites_database.find(tech);
    if (it != s_prerequisites_database.end()) {
        return !it->second.empty();
    }

    return false;
}

std::vector<TechnologyType> TechnologyPrerequisites::GetUnlockedTechnologies(TechnologyType tech) {
    InitializeDatabase();

    std::vector<TechnologyType> unlocked;

    // Find all technologies that have this tech as a prerequisite
    for (const auto& [tech_type, prerequisites] : s_prerequisites_database) {
        for (const auto& prereq : prerequisites) {
            if (prereq == tech) {
                unlocked.push_back(tech_type);
                break;
            }
        }
    }

    return unlocked;
}

} // namespace game::technology
