// ============================================================================
// ShapefileLoader.cpp - Shapefile Loader Implementation
// Mechanica Imperii
// ============================================================================

#include "map/loaders/ShapefileLoader.h"
#include <iostream>

namespace game::map::loaders {

    ShapefileLoader::ShapefileLoader() {}
    ShapefileLoader::~ShapefileLoader() {}

    bool ShapefileLoader::LoadShapefile(const std::string& filepath,
                                       std::vector<ProvinceData>& provinces) {
        // Stub: Load ESRI shapefile format
        // This would require a shapefile library like GDAL or shapelib
        m_last_error = "Shapefile loading not yet implemented";
        std::cerr << "WARNING: " << m_last_error << std::endl;
        return false;
    }

    bool ShapefileLoader::LoadMultipleShapefiles(const std::vector<std::string>& filepaths,
                                                std::vector<ProvinceData>& provinces) {
        for (const auto& filepath : filepaths) {
            if (!LoadShapefile(filepath, provinces)) {
                return false;
            }
        }
        return !provinces.empty();
    }

    void ShapefileLoader::SetCoordinateTransform(double min_lat, double max_lat,
                                                 double min_lon, double max_lon) {
        m_min_lat = min_lat;
        m_max_lat = max_lat;
        m_min_lon = min_lon;
        m_max_lon = max_lon;
    }

} // namespace game::map::loaders
