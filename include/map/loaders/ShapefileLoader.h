// ============================================================================
// ShapefileLoader.h - ESRI Shapefile Loader
// Mechanica Imperii - Load map data from shapefiles
// ============================================================================

#pragma once

#include "map/MapData.h"
#include <string>
#include <vector>

namespace game::map::loaders {

    // ============================================================================
    // Shapefile Loader
    // ============================================================================

    class ShapefileLoader {
    public:
        ShapefileLoader();
        ~ShapefileLoader();

        // Load shapefiles
        bool LoadShapefile(const std::string& filepath, std::vector<ProvinceData>& provinces);
        bool LoadMultipleShapefiles(const std::vector<std::string>& filepaths,
                                    std::vector<ProvinceData>& provinces);

        // Configuration
        void SetCoordinateTransform(double min_lat, double max_lat,
                                    double min_lon, double max_lon);

        // Error handling
        std::string GetLastError() const { return m_last_error; }

    private:
        std::string m_last_error;
        double m_min_lat = 35.0;
        double m_max_lat = 72.0;
        double m_min_lon = -15.0;
        double m_max_lon = 45.0;
    };

} // namespace game::map::loaders
