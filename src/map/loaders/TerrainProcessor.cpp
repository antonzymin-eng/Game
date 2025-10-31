// ============================================================================
// TerrainProcessor.cpp - Terrain Processor Implementation
// Mechanica Imperii
// ============================================================================

#include "map/loaders/TerrainProcessor.h"
#include "map/GeographicUtils.h"

namespace game::map::loaders {

    TerrainProcessor::TerrainProcessor() {}
    TerrainProcessor::~TerrainProcessor() {}

    void TerrainProcessor::ProcessTerrain(std::vector<ProvinceData>& provinces) {
        DetectCoastalProvinces(provinces);
        DetectRiverProvinces(provinces);

        for (auto& province : provinces) {
            ClassifyTerrain(province);
            ClassifyClimate(province);
        }
    }

    void TerrainProcessor::ClassifyTerrain(ProvinceData& province) {
        if (province.terrain == TerrainType::UNKNOWN) {
            province.terrain = InferTerrainFromProperties(province);
        }
    }

    void TerrainProcessor::DetectCoastalProvinces(std::vector<ProvinceData>& provinces) {
        // Stub: Detect which provinces are coastal
        // This would check proximity to water bodies
    }

    void TerrainProcessor::DetectRiverProvinces(std::vector<ProvinceData>& provinces) {
        // Stub: Detect which provinces have rivers
        // This would check for river networks
    }

    void TerrainProcessor::ClassifyClimate(ProvinceData& province) {
        if (province.climate == ClimateZone::UNKNOWN) {
            province.climate = InferClimateFromLocation(province.center);
        }
    }

    void TerrainProcessor::ProcessClimateData(std::vector<ProvinceData>& provinces) {
        for (auto& province : provinces) {
            ClassifyClimate(province);
        }
    }

    TerrainType TerrainProcessor::InferTerrainFromProperties(const ProvinceData& province) {
        // Stub: Infer terrain from position and other properties
        if (province.is_coastal) {
            return TerrainType::COAST;
        }
        // Default to plains
        return TerrainType::PLAINS;
    }

    ClimateZone TerrainProcessor::InferClimateFromLocation(const Coordinate& location) {
        // Stub: Infer climate from latitude/position
        // For European context, most would be temperate
        return ClimateZone::TEMPERATE;
    }

    bool TerrainProcessor::IsNearWater(const ProvinceData& province,
                                      const std::vector<ProvinceData>& all_provinces) {
        // Stub: Check if province is near water
        return province.is_coastal;
    }

} // namespace game::map::loaders
