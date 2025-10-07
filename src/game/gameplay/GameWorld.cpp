// ============================================================================
// Date/Time Created: Tuesday, September 16, 2025 - 11:45 AM PST
// Intended Folder Location: src/game/GameWorld.cpp
// GameWorld.cpp - Fixed Logic, Performance, and Separation of Concerns
// ============================================================================

#include "game/GameWorld.h"
#include <algorithm>
#include <cassert>

namespace game {

    void GameWorld::initializeTestProvinces() {
        clear();

        // FIXED: Reserve space to avoid reallocations during push_back
        reserveProvinces(8);

        // Create test provinces with better data organization
        const struct ProvinceTemplate {
            int id;
            const char* name;
            int base_population;
            int base_tax_capacity;
            int development_level;
            float admin_efficiency;
            float autonomy;
            float stability;
            float war_exhaustion;
        } province_templates[] = {
            {0, "London", 1150, 115, 2, 60.0f, 0.10f, 0.7f, 0.0f},
            {1, "Paris", 950, 100, 2, 50.0f, 0.15f, 0.6f, 0.05f},
            {2, "Milan", 1050, 105, 2, 45.0f, 0.20f, 0.5f, 0.10f},
            {3, "Vienna", 800, 90, 1, 55.0f, 0.25f, 0.6f, 0.15f},
            {4, "Prague", 1200, 110, 3, 40.0f, 0.15f, 0.4f, 0.20f},
            {5, "Rome", 650, 95, 1, 35.0f, 0.30f, 0.8f, 0.25f},
            {6, "Venice", 1050, 120, 3, 65.0f, 0.20f, 0.9f, 0.30f},
            {7, "Naples", 800, 85, 1, 45.0f, 0.35f, 0.5f, 0.35f}
        };

        // Create provinces using template data
        for (const auto& temp : province_templates) {
            Province province(temp.id, temp.name);
            province.owner_nation_id = 0; // All owned by player initially

            // Set population and development
            province.base_population = temp.base_population;
            province.current_population = province.base_population;
            province.base_tax_capacity = temp.base_tax_capacity;
            province.development_level = temp.development_level;

            // Set administrative data
            province.base_administrative_efficiency = temp.admin_efficiency;
            province.autonomy_level = temp.autonomy;
            province.stability = temp.stability;
            province.war_exhaustion = temp.war_exhaustion;

            // Set building levels based on development
            switch (temp.development_level) {
                case 3: // Highly developed
                    province.buildings.tax_office_level = 2;
                    province.buildings.market_level = 2;
                    province.buildings.fortification_level = 1;
                    province.buildings.temple_level = 1;
                    break;
                case 2: // Moderately developed
                    province.buildings.tax_office_level = 1;
                    province.buildings.market_level = 1;
                    if (temp.id % 2 == 0) province.buildings.fortification_level = 1;
                    break;
                case 1: // Basic development
                    if (temp.id % 3 == 0) province.buildings.tax_office_level = 1;
                    break;
                default:
                    break;
            }

            // FIXED: Move individual province properties to Province class if needed
            // garrison_strength and trade_value should be Province members, not GameWorld
            // For now, assuming they're part of Province structure
            // If Province has these fields, set them here:
            // province.garrison_strength = 100 + (temp.id * 25);
            // province.trade_value = 50 + (temp.id * 10);

            addProvince(std::move(province));
        }
    }

    // FIXED: Improved province lookup with validation
    Province* GameWorld::getProvinceById(int id) noexcept {
        if (!isValidProvinceId(id)) {
            return nullptr;
        }

        auto it = std::find_if(provinces.begin(), provinces.end(),
            [id](const Province& p) { return p.id == id; });
        return (it != provinces.end()) ? &(*it) : nullptr;
    }

    const Province* GameWorld::getProvinceById(int id) const noexcept {
        if (!isValidProvinceId(id)) {
            return nullptr;
        }

        auto it = std::find_if(provinces.begin(), provinces.end(),
            [id](const Province& p) { return p.id == id; });
        return (it != provinces.end()) ? &(*it) : nullptr;
    }

    void GameWorld::addProvince(const Province& province) {
        provinces.push_back(province);
    }

    // FIXED: Add move version for better performance
    void GameWorld::addProvince(Province&& province) {
        provinces.push_back(std::move(province));
    }

    // FIXED: Add memory pre-allocation
    void GameWorld::reserveProvinces(size_t count) {
        provinces.reserve(count);
    }

    bool GameWorld::removeProvinceById(int id) {
        auto it = std::find_if(provinces.begin(), provinces.end(),
            [id](const Province& p) { return p.id == id; });

        if (it != provinces.end()) {
            // Clear selection if removing selected province
            if (selected_province_id == id) {
                clearSelection();
            }
            provinces.erase(it);
            return true;
        }
        return false;
    }

    void GameWorld::selectProvince(int id) noexcept {
        if (getProvinceById(id) != nullptr) {
            selected_province_id = id;
        }
    }

    Province* GameWorld::getSelectedProvince() noexcept {
        if (hasSelection()) {
            return getProvinceById(selected_province_id);
        }
        return nullptr;
    }

    const Province* GameWorld::getSelectedProvince() const noexcept {
        if (hasSelection()) {
            return getProvinceById(selected_province_id);
        }
        return nullptr;
    }

    // FIXED: Add utility methods
    void GameWorld::clear() {
        provinces.clear();
        clearSelection();
    }

    // FIXED: Add validation helper
    bool GameWorld::isValidProvinceId(int id) const noexcept {
        // Basic validation - could be enhanced based on your ID scheme
        return id >= 0 && id < 10000; // Reasonable upper bound
    }

} // namespace game
