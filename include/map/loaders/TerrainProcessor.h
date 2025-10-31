// ============================================================================
// TerrainProcessor.h - Terrain Data Processing
// Mechanica Imperii - Process and enhance terrain data
// ============================================================================

#pragma once

#include "map/MapData.h"
#include <vector>

namespace game::map::loaders {

    // ============================================================================
    // Terrain Processor
    // ============================================================================

    class TerrainProcessor {
    public:
        TerrainProcessor();
        ~TerrainProcessor();

        // Terrain processing
        void ProcessTerrain(std::vector<ProvinceData>& provinces);
        void ClassifyTerrain(ProvinceData& province);
        void DetectCoastalProvinces(std::vector<ProvinceData>& provinces);
        void DetectRiverProvinces(std::vector<ProvinceData>& provinces);

        // Climate processing
        void ClassifyClimate(ProvinceData& province);
        void ProcessClimateData(std::vector<ProvinceData>& provinces);

    private:
        TerrainType InferTerrainFromProperties(const ProvinceData& province);
        ClimateZone InferClimateFromLocation(const Coordinate& location);
        bool IsNearWater(const ProvinceData& province, const std::vector<ProvinceData>& all_provinces);
    };

} // namespace game::map::loaders
