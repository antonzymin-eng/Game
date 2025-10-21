// ============================================================================
// test_map_loading.cpp - Test Map Data Loading
// Created: October 21, 2025
// ============================================================================

#include "map/MapDataLoader.h"
#include "map/ProvinceRenderComponent.h"
#include "core/ECS/EntityManager.h"
#include <iostream>

int main() {
    std::cout << "=== Map Loading Test ===" << std::endl;
    
    try {
        // Create entity manager
        core::ecs::EntityManager entity_manager;
        
        // Load provinces
        std::cout << "Loading provinces from data/test_provinces.json..." << std::endl;
        bool success = game::map::MapDataLoader::LoadProvincesECS(
            "data/test_provinces.json",
            entity_manager
        );
        
        if (!success) {
            std::cerr << "ERROR: Failed to load provinces!" << std::endl;
            return 1;
        }
        
        // Verify loaded data
        auto entities = entity_manager.GetEntitiesWithComponent<game::map::ProvinceRenderComponent>();
        std::cout << "\n=== Loaded Provinces ===" << std::endl;
        std::cout << "Total provinces: " << entities.size() << std::endl;
        
        for (const auto& entity_id : entities) {
            auto render = entity_manager.GetComponent<game::map::ProvinceRenderComponent>(entity_id);
            if (render) {
                std::cout << "\nProvince: " << render->name << " (ID: " << render->province_id << ")" << std::endl;
                std::cout << "  Owner Realm: " << render->owner_realm_id << std::endl;
                std::cout << "  Terrain: " << static_cast<int>(render->terrain_type) << std::endl;
                std::cout << "  Center: (" << render->center_position.x << ", " << render->center_position.y << ")" << std::endl;
                std::cout << "  Boundary points: " << render->boundary_points.size() << std::endl;
                std::cout << "  LOD0 points: " << render->boundary_lod0.size() << std::endl;
                std::cout << "  LOD1 points: " << render->boundary_lod1.size() << std::endl;
                std::cout << "  LOD2 points: " << render->boundary_lod2.size() << std::endl;
                std::cout << "  Features: " << render->features.size() << std::endl;
                
                for (const auto& feature : render->features) {
                    std::cout << "    - " << feature.name << " (" 
                              << static_cast<int>(feature.type) << ")" << std::endl;
                }
            }
        }
        
        std::cout << "\n=== Test PASSED ===" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
}
