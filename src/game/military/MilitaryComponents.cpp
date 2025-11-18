// ============================================================================
// MilitaryComponents.cpp - ECS Component Method Implementations
// Created: October 13, 2025 - Following Architecture Database Patterns
// Location: src/game/military/MilitaryComponents.cpp
// ============================================================================

#include "game/military/MilitaryComponents.h"
#include "core/logging/Logger.h"

namespace game::military {

    // ============================================================================
    // MilitaryUnit Methods
    // ============================================================================

    MilitaryUnit::MilitaryUnit(UnitType unit_type) : type(unit_type) {
        // Set defaults based on unit type
        switch (unit_type) {
            case UnitType::LEVIES:
                max_strength = 1000;
                recruitment_cost = 50.0;
                monthly_maintenance = 5.0;
                attack_strength = 8.0;
                defense_strength = 6.0;
                break;
            case UnitType::SPEARMEN:
                max_strength = 800;
                recruitment_cost = 100.0;
                monthly_maintenance = 10.0;
                attack_strength = 12.0;
                defense_strength = 10.0;
                break;
            case UnitType::HEAVY_CAVALRY:
                max_strength = 200;
                recruitment_cost = 500.0;
                monthly_maintenance = 50.0;
                attack_strength = 25.0;
                defense_strength = 18.0;
                movement_speed = 2.0;
                break;
            default:
                max_strength = 500;
                recruitment_cost = 150.0;
                monthly_maintenance = 15.0;
                attack_strength = 10.0;
                defense_strength = 8.0;
                break;
        }
        
        current_strength = max_strength;
        training = 0.5;
        equipment_quality = 0.5;
        morale = MoraleState::STEADY;
        cohesion = 0.8;
        loyalty = 0.8;
    }

    void MilitaryUnit::ApplyLosses(uint32_t casualties) {
        if (casualties >= current_strength) {
            current_strength = 0;
            morale = MoraleState::BROKEN;
        } else {
            current_strength -= casualties;
            // Morale impact based on losses
            double loss_ratio = static_cast<double>(casualties) / max_strength;
            if (loss_ratio > 0.3) {
                morale = static_cast<MoraleState>(static_cast<int>(morale) - 1);
                if (morale < MoraleState::ROUTING) morale = MoraleState::ROUTING;
            }
        }
    }

    void MilitaryUnit::RestoreStrength(uint32_t reinforcements) {
        current_strength = std::min(current_strength + reinforcements, max_strength);
        if (current_strength > max_strength * 0.8) {
            // Restore morale if well-reinforced
            if (morale < MoraleState::STEADY) {
                morale = MoraleState::STEADY;
            }
        }
    }

    void MilitaryUnit::UpdateMorale(double morale_change) {
        // Convert morale_change to discrete state change
        int current_state = static_cast<int>(morale);
        if (morale_change > 0.2) {
            current_state = std::min(current_state + 1, static_cast<int>(MoraleState::FANATICAL));
        } else if (morale_change < -0.2) {
            current_state = std::max(current_state - 1, static_cast<int>(MoraleState::ROUTING));
        }
        morale = static_cast<MoraleState>(current_state);
    }

    void MilitaryUnit::ConsumeSupplies(double consumption_rate) {
        supply_level = std::max(0.0, supply_level - consumption_rate);
        ammunition = std::max(0.0, ammunition - consumption_rate * 0.5);
        
        // Low supplies affect morale and effectiveness
        if (supply_level < 0.3) {
            UpdateMorale(-0.1);
        }
    }

    bool MilitaryUnit::IsEffective() const {
        return current_strength > max_strength * 0.1 && 
               morale > MoraleState::BROKEN && 
               supply_level > 0.1;
    }

    double MilitaryUnit::GetCombatEffectiveness() const {
        if (!IsEffective()) return 0.0;
        
        double strength_factor = static_cast<double>(current_strength) / max_strength;
        double morale_factor = static_cast<double>(morale) / static_cast<double>(MoraleState::FANATICAL);
        double supply_factor = supply_level;
        double equipment_factor = equipment_quality;
        
        return attack_strength * strength_factor * morale_factor * supply_factor * equipment_factor;
    }

    // ============================================================================
    // Commander Methods
    // ============================================================================

    Commander::Commander(const std::string& commander_name) : name(commander_name) {
        // Default values are already set in the struct definition
    }

    double Commander::GetCommandEffectiveness(uint32_t army_size) const {
        double base_effectiveness = (martial_skill + tactical_skill) / 200.0; // 0.0 to 1.0
        
        // Penalty for commanding larger armies
        double size_penalty = std::max(0.0, (static_cast<double>(army_size) - command_limit) / 5000.0);
        
        return std::max(0.1, base_effectiveness - size_penalty);
    }

    double Commander::GetMoraleBonus() const {
        return charisma / 100.0; // 0.0 to 1.0 bonus
    }

    bool Commander::CanCommand(UnitType unit_type) const {
        // Basic commanders can command all unit types
        // Could be extended with specialized commander types
        return true;
    }

    // ============================================================================
    // MilitaryComponent Methods
    // ============================================================================

    bool MilitaryComponent::CanRecruit(UnitType unit_type, uint32_t quantity) const {
        // Check recruitment capacity
        std::lock_guard<std::mutex> lock(garrison_mutex);
        uint32_t current_capacity = 0;
        for (const auto& unit : garrison_units) {
            current_capacity += unit.current_strength;
        }

        if (current_capacity + quantity > recruitment_capacity) {
            return false;
        }

        // Check if unit type is available
        auto it = unit_type_available.find(unit_type);
        return (it != unit_type_available.end()) ? it->second : true;
    }

    MilitaryUnit* MilitaryComponent::CreateUnit(UnitType unit_type, uint32_t initial_strength) {
        std::lock_guard<std::mutex> lock(garrison_mutex);

        // Check recruitment capacity (inline to avoid nested lock)
        uint32_t current_capacity = 0;
        for (const auto& unit : garrison_units) {
            current_capacity += unit.current_strength;
        }

        if (current_capacity + initial_strength > recruitment_capacity) {
            return nullptr;
        }

        // Check if unit type is available
        auto it = unit_type_available.find(unit_type);
        if (it != unit_type_available.end() && !it->second) {
            return nullptr;
        }

        MilitaryUnit new_unit(unit_type);
        new_unit.current_strength = std::min(initial_strength, new_unit.max_strength);

        garrison_units.push_back(new_unit);
        return &garrison_units.back();
    }

    void MilitaryComponent::DisbandUnit(size_t unit_index) {
        std::lock_guard<std::mutex> lock(garrison_mutex);
        if (unit_index < garrison_units.size()) {
            garrison_units.erase(garrison_units.begin() + unit_index);
        }
    }

    double MilitaryComponent::CalculateTotalMaintenance() const {
        std::lock_guard<std::mutex> lock(garrison_mutex);
        double total = 0.0;
        for (const auto& unit : garrison_units) {
            total += unit.monthly_maintenance;
        }
        return total;
    }

    uint32_t MilitaryComponent::GetTotalGarrisonStrength() const {
        std::lock_guard<std::mutex> lock(garrison_mutex);
        uint32_t total = 0;
        for (const auto& unit : garrison_units) {
            total += unit.current_strength;
        }
        return total;
    }

    // ============================================================================
    // ArmyComponent Methods
    // ============================================================================

    ArmyComponent::ArmyComponent(const std::string& name) : army_name(name) {
        movement_points = 100.0;
        max_movement_points = 100.0;
        total_strength = 0;
        supply_level = 1.0;
        is_besieging = false;
        is_active = true;
    }

    void ArmyComponent::AddUnit(const MilitaryUnit& unit) {
        std::lock_guard<std::mutex> lock(units_mutex);
        units.push_back(unit);
        RecalculateStrength();
    }

    void ArmyComponent::RemoveUnit(size_t unit_index) {
        std::lock_guard<std::mutex> lock(units_mutex);
        if (unit_index < units.size()) {
            units.erase(units.begin() + unit_index);
            RecalculateStrength();
        }
    }

    void ArmyComponent::RecalculateStrength() {
        // Note: This method should be called with units_mutex already locked
        total_strength = 0;
        for (const auto& unit : units) {
            total_strength += unit.current_strength;
        }
    }

    double ArmyComponent::GetCombatStrength() const {
        std::lock_guard<std::mutex> lock(units_mutex);
        double strength = 0.0;
        for (const auto& unit : units) {
            strength += unit.GetCombatEffectiveness();
        }
        return strength;
    }

    bool ArmyComponent::CanMove() const {
        std::lock_guard<std::mutex> lock(units_mutex);
        return movement_points > 0 && !units.empty() && !is_besieging;
    }

} // namespace game::military