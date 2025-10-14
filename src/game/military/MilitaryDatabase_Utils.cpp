// ============================================================================
// Date/Time Created: Wednesday, September 24, 2025 - 4:00 PM PST
// Intended Folder Location: src/game/military/MilitaryDatabase_Utils.cpp
// MilitaryDatabase_Utils.cpp - Database and Utility Function Implementations
// ============================================================================

#include "game/military/MilitarySystem.h"
#include "game/technology/TechnologyComponents.h"
#include "config/GameConfig.h"
#include <unordered_map>
#include <string>
#include <vector>

namespace game::military {

    // ============================================================================
    // Database Namespace Implementation
    // ============================================================================

    namespace database {

        MilitaryUnit CreateInfantryUnit(UnitType type, types::SocialClass recruitment_class) {
            MilitaryUnit unit(type);
            
            // Adjust stats based on recruitment class
            switch (recruitment_class) {
                case types::SocialClass::NOBILITY:
                    unit.training += 0.3;
                    unit.equipment_quality += 0.2;
                    unit.loyalty += 0.2;
                    unit.recruitment_cost *= 1.8;
                    unit.monthly_maintenance *= 1.5;
                    break;
                    
                case types::SocialClass::MERCHANTS:
                    unit.equipment_quality += 0.15;
                    unit.recruitment_cost *= 1.3;
                    unit.monthly_maintenance *= 1.2;
                    break;
                    
                case types::SocialClass::CRAFTSMEN:
                    unit.equipment_quality += 0.1;
                    unit.recruitment_cost *= 1.1;
                    break;
                    
                case types::SocialClass::FREE_PEASANT:
                    // Standard stats, no modifications
                    break;
                    
                case types::SocialClass::PEASANTS:
                    unit.training -= 0.1;
                    unit.equipment_quality -= 0.1;
                    unit.recruitment_cost *= 0.8;
                    unit.monthly_maintenance *= 0.9;
                    break;
                    
                default:
                    break;
            }
            
            // Infantry-specific adjustments
            switch (type) {
                case UnitType::LEVIES:
                    unit.training = std::max(0.1, unit.training - 0.2);
                    unit.cohesion = 0.6;
                    break;
                    
                case UnitType::SPEARMEN:
                    unit.defense_strength *= 1.2; // Bonus against cavalry
                    break;
                    
                case UnitType::CROSSBOWMEN:
                    unit.ammunition = 0.8; // Starts with limited ammo
                    break;
                    
                case UnitType::PIKEMEN:
                    unit.defense_strength *= 1.4; // Strong against cavalry
                    unit.movement_speed *= 0.8; // Slower due to pikes
                    break;
                    
                case UnitType::ARQUEBUSIERS:
                case UnitType::MUSKETEERS:
                    unit.ammunition = 0.6; // Limited gunpowder
                    unit.equipment_quality += 0.1; // Better weapons
                    break;
                    
                default:
                    break;
            }
            
            return unit;
        }

        MilitaryUnit CreateCavalryUnit(UnitType type, types::SocialClass recruitment_class) {
            MilitaryUnit unit(type);
            
            // Cavalry requires higher social class - adjust if inappropriate
            if (recruitment_class == types::SocialClass::PEASANTS) {
                unit.training -= 0.3;
                unit.equipment_quality -= 0.2;
                unit.recruitment_cost *= 0.7;
            } else if (recruitment_class == types::SocialClass::NOBILITY) {
                unit.training += 0.4;
                unit.equipment_quality += 0.3;
                unit.loyalty += 0.3;
                unit.recruitment_cost *= 2.5;
                unit.monthly_maintenance *= 2.0;
            }
            
            // Cavalry-specific adjustments
            switch (type) {
                case UnitType::LIGHT_CAVALRY:
                    unit.movement_speed *= 1.5;
                    unit.cohesion = 0.7; // Less disciplined
                    break;
                    
                case UnitType::HEAVY_CAVALRY:
                    unit.attack_strength *= 1.3; // Charge bonus
                    unit.equipment_quality += 0.2; // Heavy armor
                    unit.movement_speed *= 0.8; // Slower due to armor
                    break;
                    
                case UnitType::MOUNTED_ARCHERS:
                    unit.range = 150.0;
                    unit.ammunition = 0.7;
                    unit.movement_speed *= 1.3;
                    break;
                    
                case UnitType::DRAGOONS:
                    unit.range = 80.0; // Can fight dismounted
                    unit.ammunition = 0.5;
                    break;
                    
                default:
                    break;
            }
            
            return unit;
        }

        MilitaryUnit CreateSiegeUnit(UnitType type, types::SocialClass recruitment_class) {
            MilitaryUnit unit(type);
            
            // Siege units require skilled operators
            if (recruitment_class != types::SocialClass::CRAFTSMEN && 
                recruitment_class != types::SocialClass::MERCHANTS) {
                unit.training -= 0.2;
                unit.equipment_quality -= 0.15;
            }
            
            // Siege-specific adjustments
            switch (type) {
                case UnitType::CATAPULTS:
                    unit.range = 300.0;
                    unit.ammunition = 1.0; // Uses stones
                    unit.movement_speed = 0.2;
                    break;
                    
                case UnitType::TREBUCHETS:
                    unit.range = 400.0;
                    unit.attack_strength *= 1.5;
                    unit.movement_speed = 0.1;
                    break;
                    
                case UnitType::CANNONS:
                    unit.range = 500.0;
                    unit.attack_strength *= 2.0;
                    unit.ammunition = 0.3; // Limited gunpowder
                    unit.movement_speed = 0.15;
                    break;
                    
                case UnitType::SIEGE_TOWERS:
                    unit.defense_strength *= 1.5;
                    unit.movement_speed = 0.1;
                    break;
                    
                default:
                    break;
            }
            
            return unit;
        }

        MilitaryUnit CreateNavalUnit(UnitType type, types::SocialClass recruitment_class) {
            MilitaryUnit unit(type);
            
            // Naval units need experienced sailors
            if (recruitment_class == types::SocialClass::MERCHANTS ||
                recruitment_class == types::SocialClass::CRAFTSMEN) {
                unit.training += 0.2;
                unit.equipment_quality += 0.1;
            }
            
            // Naval-specific adjustments
            switch (type) {
                case UnitType::GALLEYS:
                    unit.movement_speed = 1.5; // Fast in good weather
                    unit.max_strength = 200; // Smaller crews
                    break;
                    
                case UnitType::COGS:
                    unit.defense_strength *= 1.2;
                    unit.movement_speed = 1.0;
                    break;
                    
                case UnitType::CARRACKS:
                    unit.attack_strength *= 1.3;
                    unit.range = 200.0;
                    unit.max_strength = 300;
                    break;
                    
                case UnitType::GALLEONS:
                    unit.attack_strength *= 1.5;
                    unit.range = 300.0;
                    unit.max_strength = 400;
                    break;
                    
                case UnitType::SHIPS_OF_THE_LINE:
                    unit.attack_strength *= 2.0;
                    unit.range = 400.0;
                    unit.max_strength = 500;
                    unit.ammunition = 0.4; // Limited cannonballs
                    break;
                    
                default:
                    break;
            }
            
            return unit;
        }

        Commander GenerateCommander(const std::string& name, types::SocialClass social_class,
                                   MilitaryRank rank) {
            Commander commander(name);
            commander.rank = rank;
            
            // Social class affects base skills
            switch (social_class) {
                case types::SocialClass::NOBILITY:
                    commander.martial_skill += 0.2;
                    commander.tactical_skill += 0.2;
                    commander.charisma += 0.3;
                    commander.loyalty += 0.2;
                    commander.traits.push_back("Noble Born");
                    break;
                    
                case types::SocialClass::MERCHANTS:
                    commander.logistics_skill += 0.3;
                    commander.strategic_skill += 0.1;
                    commander.traits.push_back("Merchant Background");
                    break;
                    
                case types::SocialClass::CRAFTSMEN:
                    commander.logistics_skill += 0.2;
                    commander.martial_skill += 0.1;
                    commander.traits.push_back("Practical Experience");
                    break;
                    
                case types::SocialClass::FREE_PEASANT:
                    commander.martial_skill += 0.1;
                    commander.loyalty += 0.1;
                    commander.traits.push_back("Common Origin");
                    break;
                    
                default:
                    break;
            }
            
            // Rank affects skills and command capacity
            switch (rank) {
                case MilitaryRank::CAPTAIN:
                    commander.command_limit = 1000;
                    break;
                case MilitaryRank::MAJOR:
                    commander.tactical_skill += 0.1;
                    commander.command_limit = 2500;
                    break;
                case MilitaryRank::COLONEL:
                    commander.tactical_skill += 0.2;
                    commander.strategic_skill += 0.1;
                    commander.command_limit = 5000;
                    break;
                case MilitaryRank::GENERAL:
                    commander.tactical_skill += 0.3;
                    commander.strategic_skill += 0.2;
                    commander.command_limit = 10000;
                    break;
                case MilitaryRank::MARSHAL:
                    commander.tactical_skill += 0.4;
                    commander.strategic_skill += 0.3;
                    commander.charisma += 0.2;
                    commander.command_limit = 25000;
                    break;
                default:
                    commander.command_limit = 500;
                    break;
            }
            
            // Add random traits based on experience
            if (commander.experience > 0.7) {
                commander.traits.push_back("Veteran");
            }
            if (commander.martial_skill > 0.8) {
                commander.traits.push_back("Fierce Warrior");
            }
            if (commander.tactical_skill > 0.8) {
                commander.traits.push_back("Brilliant Tactician");
            }
            if (commander.logistics_skill > 0.8) {
                commander.traits.push_back("Master Organizer");
            }
            
            return commander;
        }

    } // namespace database

    // ============================================================================
    // Utils Namespace Implementation
    // ============================================================================

    namespace utils {

        std::string UnitTypeToString(UnitType type) {
            static const std::unordered_map<UnitType, std::string> unit_names = {
                {UnitType::LEVIES, "Levies"},
                {UnitType::SPEARMEN, "Spearmen"},
                {UnitType::CROSSBOWMEN, "Crossbowmen"},
                {UnitType::LONGBOWMEN, "Longbowmen"},
                {UnitType::MEN_AT_ARMS, "Men-at-Arms"},
                {UnitType::PIKEMEN, "Pikemen"},
                {UnitType::ARQUEBUSIERS, "Arquebusiers"},
                {UnitType::MUSKETEERS, "Musketeers"},
                {UnitType::LIGHT_CAVALRY, "Light Cavalry"},
                {UnitType::HEAVY_CAVALRY, "Heavy Cavalry"},
                {UnitType::MOUNTED_ARCHERS, "Mounted Archers"},
                {UnitType::DRAGOONS, "Dragoons"},
                {UnitType::CATAPULTS, "Catapults"},
                {UnitType::TREBUCHETS, "Trebuchets"},
                {UnitType::CANNONS, "Cannons"},
                {UnitType::SIEGE_TOWERS, "Siege Towers"},
                {UnitType::GALLEYS, "Galleys"},
                {UnitType::COGS, "Cogs"},
                {UnitType::CARRACKS, "Carracks"},
                {UnitType::GALLEONS, "Galleons"},
                {UnitType::SHIPS_OF_THE_LINE, "Ships of the Line"}
            };
            
            auto it = unit_names.find(type);
            return (it != unit_names.end()) ? it->second : "Unknown Unit";
        }

        std::string UnitClassToString(UnitClass unit_class) {
            switch (unit_class) {
                case UnitClass::INFANTRY: return "Infantry";
                case UnitClass::CAVALRY: return "Cavalry";
                case UnitClass::SIEGE: return "Siege";
                case UnitClass::NAVAL: return "Naval";
                default: return "Unknown";
            }
        }

        std::string MilitaryRankToString(MilitaryRank rank) {
            switch (rank) {
                case MilitaryRank::SOLDIER: return "Soldier";
                case MilitaryRank::SERGEANT: return "Sergeant";
                case MilitaryRank::LIEUTENANT: return "Lieutenant";
                case MilitaryRank::CAPTAIN: return "Captain";
                case MilitaryRank::MAJOR: return "Major";
                case MilitaryRank::COLONEL: return "Colonel";
                case MilitaryRank::GENERAL: return "General";
                case MilitaryRank::MARSHAL: return "Marshal";
                default: return "Unknown Rank";
            }
        }

        std::string CombatRoleToString(CombatRole role) {
            switch (role) {
                case CombatRole::MELEE: return "Melee";
                case CombatRole::RANGED: return "Ranged";
                case CombatRole::SIEGE: return "Siege";
                case CombatRole::SUPPORT: return "Support";
                case CombatRole::CAVALRY_CHARGE: return "Cavalry Charge";
                case CombatRole::SKIRMISH: return "Skirmish";
                default: return "Unknown Role";
            }
        }

        std::string MoraleStateToString(MoraleState morale) {
            switch (morale) {
                case MoraleState::ROUTING: return "Routing";
                case MoraleState::BROKEN: return "Broken";
                case MoraleState::WAVERING: return "Wavering";
                case MoraleState::STEADY: return "Steady";
                case MoraleState::CONFIDENT: return "Confident";
                case MoraleState::FANATICAL: return "Fanatical";
                default: return "Unknown Morale";
            }
        }

        bool IsUnitTypeAvailable(UnitType type, int current_year) {
            int introduction_year = GetHistoricalIntroductionYear(type);
            int obsolescence_year = GetHistoricalObsolescenceYear(type);
            
            return current_year >= introduction_year && 
                   (obsolescence_year == 0 || current_year <= obsolescence_year);
        }

        bool RequiresTechnology(UnitType type, technology::TechnologyType tech) {
            static const std::unordered_map<UnitType, std::vector<technology::TechnologyType>> tech_requirements = {
                {UnitType::CROSSBOWMEN, {technology::TechnologyType::CROSSBOW}},
                {UnitType::LONGBOWMEN, {technology::TechnologyType::LONGBOW}},
                {UnitType::ARQUEBUSIERS, {technology::TechnologyType::GUNPOWDER}},
                {UnitType::MUSKETEERS, {technology::TechnologyType::GUNPOWDER, technology::TechnologyType::IMPROVED_FIREARMS}},
                {UnitType::CANNONS, {technology::TechnologyType::GUNPOWDER, technology::TechnologyType::ARTILLERY}},
                {UnitType::CARRACKS, {technology::TechnologyType::IMPROVED_SHIPBUILDING}},
                {UnitType::GALLEONS, {technology::TechnologyType::ADVANCED_NAVIGATION}},
                {UnitType::SHIPS_OF_THE_LINE, {technology::TechnologyType::NAVAL_ARTILLERY}}
            };
            
            auto it = tech_requirements.find(type);
            if (it != tech_requirements.end()) {
                return std::find(it->second.begin(), it->second.end(), tech) != it->second.end();
            }
            
            return false;
        }

        UnitClass GetUnitClass(UnitType type) {
            if (type >= UnitType::LEVIES && type <= UnitType::MUSKETEERS) {
                return UnitClass::INFANTRY;
            } else if (type >= UnitType::LIGHT_CAVALRY && type <= UnitType::DRAGOONS) {
                return UnitClass::CAVALRY;
            } else if (type >= UnitType::CATAPULTS && type <= UnitType::SIEGE_TOWERS) {
                return UnitClass::SIEGE;
            } else if (type >= UnitType::GALLEYS && type <= UnitType::SHIPS_OF_THE_LINE) {
                return UnitClass::NAVAL;
            }
            
            return UnitClass::INFANTRY; // Default
        }

        CombatRole GetPrimaryCombatRole(UnitType type) {
            switch (type) {
                case UnitType::CROSSBOWMEN:
                case UnitType::LONGBOWMEN:
                case UnitType::ARQUEBUSIERS:
                case UnitType::MUSKETEERS:
                case UnitType::MOUNTED_ARCHERS:
                    return CombatRole::RANGED;
                    
                case UnitType::HEAVY_CAVALRY:
                    return CombatRole::CAVALRY_CHARGE;
                    
                case UnitType::LIGHT_CAVALRY:
                case UnitType::DRAGOONS:
                    return CombatRole::SKIRMISH;
                    
                case UnitType::CATAPULTS:
                case UnitType::TREBUCHETS:
                case UnitType::CANNONS:
                case UnitType::SIEGE_TOWERS:
                    return CombatRole::SIEGE;
                    
                default:
                    return CombatRole::MELEE;
            }
        }

        double CalculateUnitMatchup(UnitType attacker, UnitType defender) {
            double effectiveness = 1.0;
            
            UnitClass attacker_class = GetUnitClass(attacker);
            UnitClass defender_class = GetUnitClass(defender);
            CombatRole attacker_role = GetPrimaryCombatRole(attacker);
            CombatRole defender_role = GetPrimaryCombatRole(defender);
            
            // Class-based matchups
            if (attacker_class == UnitClass::CAVALRY && defender_class == UnitClass::INFANTRY) {
                // Cavalry vs Infantry - depends on infantry type
                if (defender == UnitType::PIKEMEN || defender == UnitType::SPEARMEN) {
                    effectiveness *= 0.6; // Pikes counter cavalry
                } else {
                    effectiveness *= 1.4; // Cavalry advantage vs other infantry
                }
            }
            
            if (attacker_class == UnitClass::INFANTRY && defender_class == UnitClass::CAVALRY) {
                if (attacker == UnitType::PIKEMEN || attacker == UnitType::SPEARMEN) {
                    effectiveness *= 1.5; // Pikes strong vs cavalry
                } else {
                    effectiveness *= 0.7; // Infantry disadvantage vs cavalry
                }
            }
            
            // Role-based matchups
            if (attacker_role == CombatRole::RANGED && defender_role == CombatRole::MELEE) {
                effectiveness *= 1.2; // Ranged advantage
            }
            
            if (attacker_role == CombatRole::CAVALRY_CHARGE && defender_role == CombatRole::RANGED) {
                effectiveness *= 1.3; // Cavalry can close distance
            }
            
            // Specific unit matchups
            if (attacker == UnitType::CANNONS && defender_class == UnitClass::SIEGE) {
                effectiveness *= 1.8; // Artillery vs siege engines
            }
            
            return std::max(0.2, std::min(3.0, effectiveness));
        }

        double GetTerrainAdvantage(UnitType unit_type, const std::string& terrain_type) {
            UnitClass unit_class = GetUnitClass(unit_type);
            
            if (terrain_type == "hills" || terrain_type == "mountains") {
                if (unit_class == UnitClass::INFANTRY) {
                    return 1.2; // Infantry advantage in rough terrain
                } else if (unit_class == UnitClass::CAVALRY) {
                    return 0.7; // Cavalry disadvantage
                }
            } else if (terrain_type == "plains" || terrain_type == "grassland") {
                if (unit_class == UnitClass::CAVALRY) {
                    return 1.3; // Cavalry advantage in open terrain
                }
            } else if (terrain_type == "forest") {
                if (unit_type == UnitType::LONGBOWMEN || unit_type == UnitType::CROSSBOWMEN) {
                    return 1.2; // Archers advantage in forests
                } else if (unit_class == UnitClass::CAVALRY) {
                    return 0.6; // Cavalry very limited in forests
                }
            } else if (terrain_type == "swamp" || terrain_type == "marsh") {
                if (unit_class == UnitClass::CAVALRY || unit_class == UnitClass::SIEGE) {
                    return 0.5; // Heavy units struggle in swamps
                }
            }
            
            return 1.0; // No advantage
        }

        double GetSeasonalModifier(UnitType unit_type, int current_month) {
            UnitClass unit_class = GetUnitClass(unit_type);
            
            // Winter months (December, January, February)
            if (current_month == 12 || current_month == 1 || current_month == 2) {
                if (unit_class == UnitClass::CAVALRY) {
                    return 0.8; // Horses struggle in winter
                } else if (unit_class == UnitClass::SIEGE) {
                    return 0.7; // Siege engines affected by weather
                }
            }
            
            // Summer months (June, July, August)
            if (current_month >= 6 && current_month <= 8) {
                if (unit_class == UnitClass::INFANTRY && 
                    (unit_type == UnitType::MEN_AT_ARMS || unit_type == UnitType::HEAVY_CAVALRY)) {
                    return 0.9; // Heavy armor disadvantage in summer heat
                }
            }
            
            return 1.0; // No seasonal modifier
        }

        std::vector<types::ResourceType> GetUnitResourceRequirements(UnitType type) {
            std::vector<types::ResourceType> requirements;
            
            // All units need food
            requirements.push_back(types::ResourceType::FOOD);
            
            switch (type) {
                case UnitType::CROSSBOWMEN:
                case UnitType::LONGBOWMEN:
                    requirements.push_back(types::ResourceType::WOOD);
                    requirements.push_back(types::ResourceType::IRON);
                    break;
                    
                case UnitType::ARQUEBUSIERS:
                case UnitType::MUSKETEERS:
                case UnitType::CANNONS:
                    requirements.push_back(types::ResourceType::IRON);
                    requirements.push_back(types::ResourceType::SALTPETER);
                    break;
                    
                case UnitType::HEAVY_CAVALRY:
                case UnitType::MEN_AT_ARMS:
                    requirements.push_back(types::ResourceType::IRON);
                    requirements.push_back(types::ResourceType::HORSES);
                    break;
                    
                case UnitType::LIGHT_CAVALRY:
                case UnitType::MOUNTED_ARCHERS:
                case UnitType::DRAGOONS:
                    requirements.push_back(types::ResourceType::HORSES);
                    break;
                    
                case UnitType::GALLEYS:
                case UnitType::COGS:
                case UnitType::CARRACKS:
                case UnitType::GALLEONS:
                case UnitType::SHIPS_OF_THE_LINE:
                    requirements.push_back(types::ResourceType::WOOD);
                    requirements.push_back(types::ResourceType::IRON);
                    break;
                    
                default:
                    requirements.push_back(types::ResourceType::IRON); // Basic weapons
                    break;
            }
            
            return requirements;
        }

        double GetUnitResourceConsumption(UnitType type, types::ResourceType resource) {
            if (resource == types::ResourceType::FOOD) {
                // All units consume food based on size
                UnitClass unit_class = GetUnitClass(type);
                switch (unit_class) {
                    case UnitClass::CAVALRY: return 1.5; // Horses need feeding too
                    case UnitClass::SIEGE: return 0.8;   // Smaller crews
                    case UnitClass::NAVAL: return 1.2;   // Ship crews
                    default: return 1.0;
                }
            }
            
            if (resource == types::ResourceType::SALTPETER) {
                switch (type) {
                    case UnitType::ARQUEBUSIERS: return 0.3;
                    case UnitType::MUSKETEERS: return 0.5;
                    case UnitType::CANNONS: return 1.0;
                    default: return 0.0;
                }
            }
            
            if (resource == types::ResourceType::HORSES) {
                UnitClass unit_class = GetUnitClass(type);
                if (unit_class == UnitClass::CAVALRY) {
                    return 0.1; // Maintenance for horse care
                }
            }
            
            return 0.0; // No consumption for other resources
        }

        int GetHistoricalIntroductionYear(UnitType type) {
            static const std::unordered_map<UnitType, int> introduction_years = {
                {UnitType::LEVIES, 1000},
                {UnitType::SPEARMEN, 1000},
                {UnitType::LIGHT_CAVALRY, 1000},
                {UnitType::HEAVY_CAVALRY, 1050},
                {UnitType::CROSSBOWMEN, 1100},
                {UnitType::LONGBOWMEN, 1200},
                {UnitType::MEN_AT_ARMS, 1150},
                {UnitType::PIKEMEN, 1300},
                {UnitType::CATAPULTS, 1000},
                {UnitType::TREBUCHETS, 1150},
                {UnitType::MOUNTED_ARCHERS, 1000},
                {UnitType::GALLEYS, 1000},
                {UnitType::COGS, 1200},
                {UnitType::ARQUEBUSIERS, 1400},
                {UnitType::CANNONS, 1350},
                {UnitType::CARRACKS, 1400},
                {UnitType::MUSKETEERS, 1550},
                {UnitType::DRAGOONS, 1600},
                {UnitType::GALLEONS, 1500},
                {UnitType::SHIPS_OF_THE_LINE, 1650},
                {UnitType::SIEGE_TOWERS, 1000}
            };
            
            auto it = introduction_years.find(type);
            return (it != introduction_years.end()) ? it->second : 1000;
        }

        int GetHistoricalObsolescenceYear(UnitType type) {
            static const std::unordered_map<UnitType, int> obsolescence_years = {
                {UnitType::CATAPULTS, 1400},
                {UnitType::TREBUCHETS, 1500},
                {UnitType::CROSSBOWMEN, 1600},
                {UnitType::LONGBOWMEN, 1650},
                {UnitType::ARQUEBUSIERS, 1700},
                {UnitType::GALLEYS, 1700},
                {UnitType::COGS, 1600}
            };
            
            auto it = obsolescence_years.find(type);
            return (it != obsolescence_years.end()) ? it->second : 0; // 0 means never obsolete
        }

        bool IsHistoricallyAccurate(UnitType type, int year) {
            return IsUnitTypeAvailable(type, year);
        }

    } // namespace utils

} // namespace game::military
                    
