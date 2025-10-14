#pragma once

#include <string>
#include <vector>

namespace game {
    
    // Simple Province structure for GameWorld compatibility
    // Note: This is a legacy structure - the main game uses ECS approach
    struct Province {
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