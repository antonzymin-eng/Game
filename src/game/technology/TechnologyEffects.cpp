// ============================================================================
// TechnologyEffects.cpp - Implementation of Technology Effects Database
// Created: October 31, 2025
// Location: src/game/technology/TechnologyEffects.cpp
// ============================================================================

#include "game/technology/TechnologyEffects.h"
#include <sstream>

namespace game::technology {

// Static member initialization
std::unordered_map<TechnologyType, std::vector<TechnologyEffect>> TechnologyEffectsDatabase::s_effects_database;
bool TechnologyEffectsDatabase::s_initialized = false;

// ============================================================================
// Database Initialization
// ============================================================================

void TechnologyEffectsDatabase::InitializeDatabase() {
    if (s_initialized) return;

    // Agricultural Technologies
    s_effects_database[TechnologyType::THREE_FIELD_SYSTEM] = GetThreeFieldSystemEffects();
    s_effects_database[TechnologyType::HEAVY_PLOW] = GetHeavyPlowEffects();
    s_effects_database[TechnologyType::HORSE_COLLAR] = GetHorseCollarEffects();
    s_effects_database[TechnologyType::WINDMILL] = GetWindmillEffects();
    s_effects_database[TechnologyType::WATERMILL] = GetWatermillEffects();
    s_effects_database[TechnologyType::CROP_ROTATION] = GetCropRotationEffects();
    s_effects_database[TechnologyType::SELECTIVE_BREEDING] = GetSelectiveBreedingEffects();
    s_effects_database[TechnologyType::AGRICULTURAL_MANUAL] = GetAgriculturalManualEffects();
    s_effects_database[TechnologyType::IRRIGATION_SYSTEMS] = GetIrrigationSystemsEffects();
    s_effects_database[TechnologyType::NEW_WORLD_CROPS] = GetNewWorldCropsEffects();

    // Military Technologies
    s_effects_database[TechnologyType::CHAINMAIL_ARMOR] = GetChainmailArmorEffects();
    s_effects_database[TechnologyType::PLATE_ARMOR] = GetPlateArmorEffects();
    s_effects_database[TechnologyType::CROSSBOW] = GetCrossbowEffects();
    s_effects_database[TechnologyType::LONGBOW] = GetLongbowEffects();
    s_effects_database[TechnologyType::GUNPOWDER] = GetGunpowderEffects();
    s_effects_database[TechnologyType::CANNONS] = GetCannonsEffects();
    s_effects_database[TechnologyType::ARQUEBUS] = GetArquebusEffects();
    s_effects_database[TechnologyType::MUSKET] = GetMusketEffects();
    s_effects_database[TechnologyType::STAR_FORTRESS] = GetStarFortressEffects();
    s_effects_database[TechnologyType::MILITARY_ENGINEERING] = GetMilitaryEngineeringEffects();

    // Craft Technologies
    s_effects_database[TechnologyType::BLAST_FURNACE] = GetBlastFurnaceEffects();
    s_effects_database[TechnologyType::WATER_POWERED_MACHINERY] = GetWaterPoweredMachineryEffects();
    s_effects_database[TechnologyType::MECHANICAL_CLOCK] = GetMechanicalClockEffects();
    s_effects_database[TechnologyType::PRINTING_PRESS] = GetPrintingPressEffects();
    s_effects_database[TechnologyType::DOUBLE_ENTRY_BOOKKEEPING] = GetDoubleEntryBookkeepingEffects();
    s_effects_database[TechnologyType::PAPER_MAKING] = GetPaperMakingEffects();
    s_effects_database[TechnologyType::GLASS_MAKING] = GetGlassMakingEffects();
    s_effects_database[TechnologyType::TEXTILE_MACHINERY] = GetTextileMachineryEffects();
    s_effects_database[TechnologyType::ADVANCED_METALLURGY] = GetAdvancedMetallurgyEffects();
    s_effects_database[TechnologyType::PRECISION_INSTRUMENTS] = GetPrecisionInstrumentsEffects();

    // Administrative Technologies
    s_effects_database[TechnologyType::WRITTEN_LAW_CODES] = GetWrittenLawCodesEffects();
    s_effects_database[TechnologyType::BUREAUCRATIC_ADMINISTRATION] = GetBureaucraticAdministrationEffects();
    s_effects_database[TechnologyType::CENSUS_TECHNIQUES] = GetCensusTechniquesEffects();
    s_effects_database[TechnologyType::TAX_COLLECTION_SYSTEMS] = GetTaxCollectionSystemsEffects();
    s_effects_database[TechnologyType::DIPLOMATIC_PROTOCOLS] = GetDiplomaticProtocolsEffects();
    s_effects_database[TechnologyType::RECORD_KEEPING] = GetRecordKeepingEffects();
    s_effects_database[TechnologyType::STANDARDIZED_WEIGHTS] = GetStandardizedWeightsEffects();
    s_effects_database[TechnologyType::POSTAL_SYSTEMS] = GetPostalSystemsEffects();
    s_effects_database[TechnologyType::PROFESSIONAL_ARMY] = GetProfessionalArmyEffects();
    s_effects_database[TechnologyType::STATE_MONOPOLIES] = GetStateMonopoliesEffects();

    // Academic Technologies
    s_effects_database[TechnologyType::SCHOLASTIC_METHOD] = GetScholasticMethodEffects();
    s_effects_database[TechnologyType::UNIVERSITY_SYSTEM] = GetUniversitySystemEffects();
    s_effects_database[TechnologyType::VERNACULAR_WRITING] = GetVernacularWritingEffects();
    s_effects_database[TechnologyType::NATURAL_PHILOSOPHY] = GetNaturalPhilosophyEffects();
    s_effects_database[TechnologyType::MATHEMATICAL_NOTATION] = GetMathematicalNotationEffects();
    s_effects_database[TechnologyType::EXPERIMENTAL_METHOD] = GetExperimentalMethodEffects();
    s_effects_database[TechnologyType::HUMANIST_EDUCATION] = GetHumanistEducationEffects();
    s_effects_database[TechnologyType::SCIENTIFIC_INSTRUMENTS] = GetScientificInstrumentsEffects();
    s_effects_database[TechnologyType::OPTICAL_DEVICES] = GetOpticalDevicesEffects();
    s_effects_database[TechnologyType::CARTOGRAPHY] = GetCartographyEffects();

    // Naval Technologies
    s_effects_database[TechnologyType::IMPROVED_SHIP_DESIGN] = GetImprovedShipDesignEffects();
    s_effects_database[TechnologyType::NAVIGATION_INSTRUMENTS] = GetNavigationInstrumentsEffects();
    s_effects_database[TechnologyType::COMPASS_NAVIGATION] = GetCompassNavigationEffects();
    s_effects_database[TechnologyType::NAVAL_ARTILLERY] = GetNavalArtilleryEffects();
    s_effects_database[TechnologyType::OCEAN_NAVIGATION] = GetOceanNavigationEffects();
    s_effects_database[TechnologyType::SHIPYARD_TECHNIQUES] = GetShipyardTechniquesEffects();
    s_effects_database[TechnologyType::MARITIME_LAW] = GetMaritimeLawEffects();
    s_effects_database[TechnologyType::NAVAL_TACTICS] = GetNavalTacticsEffects();
    s_effects_database[TechnologyType::LIGHTHOUSE_SYSTEMS] = GetLighthouseSystemsEffects();
    s_effects_database[TechnologyType::HARBOR_ENGINEERING] = GetHarborEngineeringEffects();

    s_initialized = true;
}

// ============================================================================
// Public Interface Methods
// ============================================================================

std::vector<TechnologyEffect> TechnologyEffectsDatabase::GetEffects(TechnologyType tech) {
    InitializeDatabase();

    auto it = s_effects_database.find(tech);
    if (it != s_effects_database.end()) {
        return it->second;
    }

    return std::vector<TechnologyEffect>();
}

std::vector<TechnologyEffect> TechnologyEffectsDatabase::GetCategoryEffects(TechnologyCategory category) {
    InitializeDatabase();

    std::vector<TechnologyEffect> category_effects;

    // Collect all effects from technologies in the specified category
    for (const auto& [tech_type, effects] : s_effects_database) {
        int tech_id = static_cast<int>(tech_type);
        int category_base = static_cast<int>(category) * 100 + 1000;

        if (tech_id >= category_base && tech_id < category_base + 100) {
            category_effects.insert(category_effects.end(), effects.begin(), effects.end());
        }
    }

    return category_effects;
}

bool TechnologyEffectsDatabase::HasEffect(TechnologyType tech, EffectType effect) {
    InitializeDatabase();

    auto effects = GetEffects(tech);
    for (const auto& tech_effect : effects) {
        if (tech_effect.type == effect) {
            return true;
        }
    }

    return false;
}

double TechnologyEffectsDatabase::GetEffectValue(TechnologyType tech, EffectType effect) {
    InitializeDatabase();

    auto effects = GetEffects(tech);
    double total_value = 0.0;

    for (const auto& tech_effect : effects) {
        if (tech_effect.type == effect) {
            total_value += tech_effect.value;
        }
    }

    return total_value;
}

std::string TechnologyEffectsDatabase::GetEffectsDescription(TechnologyType tech) {
    InitializeDatabase();

    auto effects = GetEffects(tech);
    if (effects.empty()) {
        return "No effects defined for this technology.";
    }

    std::ostringstream desc;
    desc << "Effects:\n";

    for (const auto& effect : effects) {
        desc << "  - " << effect.description << " (" << effect.affected_system << ")\n";
    }

    return desc.str();
}

} // namespace game::technology
