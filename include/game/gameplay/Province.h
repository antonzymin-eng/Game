#pragma once

#include <string>
#include <vector>

// ============================================================================
// ⚠️ DEPRECATED - DO NOT USE ⚠️
// ============================================================================
// This entire file is DEPRECATED and will be REMOVED in a future version.
// Use the modern ECS-based province system instead!
//
// Migration:
//   1. Use game::province::ProvinceSystem for all province operations
//   2. Access province data via game::province::ProvinceDataComponent
//   3. For UI migration, use game::province::ProvinceAdapter (temporary bridge)
//
// Modern ECS components (in game/province/ProvinceSystem.h):
//   - ProvinceDataComponent      (administrative data)
//   - ProvinceBuildingsComponent (buildings & construction)
//   - ProvinceResourcesComponent (resources & production)
//   - ProvinceProsperityComponent (prosperity & economics)
//
// This struct is only kept for:
//   - ProvinceInfoWindow UI migration (use ProvinceAdapter)
//   - Legacy code removal is in progress
// ============================================================================

#ifdef _MSC_VER
#pragma message("warning: game/gameplay/Province.h is DEPRECATED - Use game::province::ProvinceSystem instead")
#else
#warning "game/gameplay/Province.h is DEPRECATED - Use game::province::ProvinceSystem instead"
#endif

namespace game {

    struct [[deprecated("Use game::province::ProvinceDataComponent instead")]] Province {
        int id;
        std::string name;
        int owner_nation_id = 0;
        
        // Population data
        int base_population = 1000;
        int current_population = 1000;
        
        // Economic data
        int base_tax_capacity = 100;
        int development_level = 1;
        
        // Administrative data
        float admin_efficiency = 0.5f;
        float autonomy = 0.0f;
        float stability = 0.5f;
        float war_exhaustion = 0.0f;
        
        // Geographic data
        double x_coordinate = 0.0;
        double y_coordinate = 0.0;
        
        // Default constructor
        Province() = default;
        
        // Constructor with basic data
        Province(int province_id, const std::string& province_name) 
            : id(province_id), name(province_name) {}
    };

} // namespace game