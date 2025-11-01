// ============================================================================
// HistoricalMapLoader.h - Real European Historical Map Data System
// Mechanica Imperii - Loading actual historical geography and political boundaries
// ============================================================================

#pragma once

#include "MapSystem.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <fstream>

namespace game {
    namespace map {

        // ============================================================================
        // Historical Data Structures
        // ============================================================================

        struct HistoricalBoundary {
            std::string region_name;
            std::string historical_name;
            std::vector<Coordinate> boundary_points;
            int start_year = 1066;
            int end_year = 1900;

            // Political information
            std::string culture_group;
            std::string religion;
            std::string government_type;

            // Geographic data
            TerrainType dominant_terrain = TerrainType::PLAINS;
            ClimateZone climate_zone = ClimateZone::TEMPERATE;
            double elevation_avg = 100.0; // meters above sea level
            bool has_major_river = false;
            bool is_coastal = false;

            // Economic factors
            double base_fertility = 0.5;
            double mineral_deposits = 0.3;
            double trade_importance = 0.4;

            HistoricalBoundary() = default;
        };

        struct HistoricalSettlement {
            std::string modern_name;
            std::string historical_name_1066;
            std::string historical_name_1300;
            std::string historical_name_1500;
            std::string historical_name_1800;

            Coordinate position;
            game::population::SettlementType settlement_type_1066 = game::population::SettlementType::VILLAGE;
            game::population::SettlementType settlement_type_1800 = game::population::SettlementType::TOWN;

            // Historical population estimates
            int population_1066 = 1000;
            int population_1300 = 3000;
            int population_1500 = 5000;
            int population_1800 = 15000;

            bool is_capital = false;
            bool is_major_port = false;
            bool is_trade_center = false;
            bool has_university = false;
            int university_founded_year = 0;

            HistoricalSettlement() = default;
        };

        struct HistoricalRealm {
            std::string realm_name;
            std::string dynasty;
            std::string culture;
            std::string religion;

            std::vector<std::string> controlled_provinces;
            std::string capital_province;

            int formation_year = 1066;
            int dissolution_year = 1900;

            // Government characteristics
            std::string government_type; // "feudal_monarchy", "merchant_republic", etc.
            double centralization = 0.3; // 0.0 = very decentralized, 1.0 = highly centralized
            double administrative_efficiency = 0.4;

            HistoricalRealm() = default;
        };

        // ============================================================================
        // Data Source Types
        // ============================================================================

        enum class MapDataSource {
            NATURAL_EARTH,     // Natural Earth public domain map data
            GEOJSON_CUSTOM,    // Custom GeoJSON files
            SHAPEFILE,         // ESRI Shapefiles
            OSM_EXTRACT,       // OpenStreetMap historical extracts
            MANUAL_DEFINITION  // Hand-defined coordinate sets
        };

        // ============================================================================
        // Coordinate System Support
        // ============================================================================

        struct CoordinateSystem {
            enum Type {
                WGS84,           // Standard GPS coordinates (lat/lon)
                MERCATOR,        // Web Mercator projection
                LAMBERT_CONFORMAL, // Good for Europe
                GAME_WORLD       // Internal game coordinate system
            } type = WGS84;

            // Conversion parameters
            double central_longitude = 10.0; // Central meridian for Europe
            double central_latitude = 54.0;  // Central parallel for Europe
            double scale_factor = 1.0;

            CoordinateSystem(Type t = WGS84) : type(t) {}
        };

        // ============================================================================
        // Map Data Loader Classes
        // ============================================================================

        class CoordinateConverter {
        public:
            CoordinateConverter();

            // Convert between coordinate systems
            Coordinate LatLonToGame(double latitude, double longitude) const;
            void GameToLatLon(const Coordinate& game_pos, double& latitude, double& longitude) const;

            // Projection utilities
            Coordinate ProjectToMercator(double latitude, double longitude) const;
            Coordinate MercatorToGame(const Coordinate& mercator_pos) const;

            // Set game world bounds (in game coordinates)
            void SetGameWorldBounds(const BoundingBox& bounds);
            void SetRealWorldBounds(double min_lat, double max_lat, double min_lon, double max_lon);

            // Utility functions
            double CalculateDistanceKm(double lat1, double lon1, double lat2, double lon2) const;
            double CalculateAreaKm2(const std::vector<std::pair<double, double>>& lat_lon_points) const;

        private:
            BoundingBox m_game_bounds{ -500.0, -500.0, 500.0, 500.0 };
            BoundingBox m_real_bounds{ 35.0, 71.0, -10.0, 50.0 }; // Europe bounds: lat_min, lat_max, lon_min, lon_max

            // Projection constants
            static constexpr double EARTH_RADIUS = 6378137.0; // WGS84 equatorial radius in meters
            static constexpr double WGS84_A = 6378137.0;
            static constexpr double WGS84_F = 1.0 / 298.257223563;
        };

        class GeoJSONLoader {
        public:
            GeoJSONLoader();

            // Load geographic data from GeoJSON files
            bool LoadCountryBoundaries(const std::string& file_path, std::vector<HistoricalBoundary>& boundaries);
            bool LoadCityData(const std::string& file_path, std::vector<HistoricalSettlement>& settlements);
            bool LoadRiverNetwork(const std::string& file_path, std::vector<std::vector<Coordinate>>& rivers);

            // Process specific GeoJSON feature types
            bool ProcessPolygonFeature(const Json::Value& feature, HistoricalBoundary& boundary);
            bool ProcessPointFeature(const Json::Value& feature, HistoricalSettlement& settlement);
            bool ProcessLineFeature(const Json::Value& feature, std::vector<Coordinate>& line);

        private:
            CoordinateConverter m_converter;

            // JSON parsing helpers
            std::vector<Coordinate> ExtractCoordinates(const Json::Value& coordinates_array);
            std::string ExtractProperty(const Json::Value& properties, const std::string& key, const std::string& default_val = "");
            double ExtractNumericProperty(const Json::Value& properties, const std::string& key, double default_val = 0.0);
        };

        class HistoricalDataLoader {
        public:
            HistoricalDataLoader();

            // Load historical political data
            bool LoadHistoricalRealms(const std::string& file_path, int year, std::vector<HistoricalRealm>& realms);
            bool LoadProvinceHistory(const std::string& file_path, std::unordered_map<std::string, HistoricalBoundary>& provinces);
            bool LoadSettlementHistory(const std::string& file_path, std::vector<HistoricalSettlement>& settlements);

            // Apply historical data to map
            void ApplyHistoricalBoundaries(MapSystem& map_system, const std::vector<HistoricalBoundary>& boundaries, int year);
            void ApplyHistoricalSettlements(MapSystem& map_system, const std::vector<HistoricalSettlement>& settlements, int year);

            // Interpolate data for specific years
            HistoricalSettlement InterpolateSettlement(const HistoricalSettlement& settlement, int year) const;
            double InterpolatePopulation(const HistoricalSettlement& settlement, int year) const;
            game::population::SettlementType InterpolateSettlementType(const HistoricalSettlement& settlement, int year) const;

        private:
            CoordinateConverter m_converter;

            // Historical interpolation helpers
            double LinearInterpolate(double val1, int year1, double val2, int year2, int target_year) const;
            std::string GetHistoricalName(const HistoricalSettlement& settlement, int year) const;
        };

        // ============================================================================
        // Main Historical Map Loader
        // ============================================================================

        class HistoricalMapLoader {
        public:
            HistoricalMapLoader(MapSystem& map_system);
            ~HistoricalMapLoader();

            // Main loading interface
            bool LoadEuropeanMap(int start_year = 1066);
            bool LoadFromDataDirectory(const std::string& data_directory, int year = 1066);
            bool LoadCustomMap(const std::string& config_file, int year = 1066);

            // Specific region loaders
            bool LoadWesternEurope(int year = 1066);
            bool LoadEasternEurope(int year = 1066);
            bool LoadMediterranean(int year = 1066);
            bool LoadBritishIsles(int year = 1066);
            bool LoadScandinaviaBaldtics(int year = 1066);
            bool LoadMiddleEastNorthAfrica(int year = 1066);

            // Data source configuration
            void SetDataSource(MapDataSource source) { m_data_source = source; }
            void SetDataDirectory(const std::string& directory) { m_data_directory = directory; }
            void SetCoordinateSystem(const CoordinateSystem& coord_system);

            // Quality and detail settings
            void SetProvinceDetailLevel(int level); // 1=countries, 2=regions, 3=counties, 4=parishes
            void SetCoastlineDetail(int level);     // 1=simple, 2=medium, 3=high detail
            void SetRiverDetail(int level);         // 1=major rivers only, 2=all navigable, 3=all rivers

            // Historical accuracy settings
            void EnableHistoricalAccuracy(bool enable) { m_historical_accuracy = enable; }
            void SetYearRange(int start_year, int end_year);
            void LoadDynamicBoundaries(bool enable) { m_dynamic_boundaries = enable; }

            // Progress and status
            void SetProgressCallback(std::function<void(float, const std::string&)> callback);
            std::string GetLastError() const { return m_last_error; }

            // Validation and verification
            bool ValidateMapData() const;
            void GenerateMapReport(const std::string& output_file) const;

        private:
            MapSystem& m_map_system;
            MapDataSource m_data_source = MapDataSource::GEOJSON_CUSTOM;
            std::string m_data_directory = "data/maps/";

            // Loaders
            std::unique_ptr<GeoJSONLoader> m_geojson_loader;
            std::unique_ptr<HistoricalDataLoader> m_historical_loader;
            std::unique_ptr<CoordinateConverter> m_converter;

            // Configuration
            int m_province_detail_level = 3; // County level
            int m_coastline_detail = 2;      // Medium detail
            int m_river_detail = 2;          // Navigable rivers
            bool m_historical_accuracy = true;
            bool m_dynamic_boundaries = false;
            int m_start_year = 1066;
            int m_end_year = 1900;

            // Status
            std::string m_last_error;
            std::function<void(float, const std::string&)> m_progress_callback;

            // Internal loading methods
            bool LoadBaseGeography();
            bool LoadPoliticalBoundaries(int year);
            bool LoadSettlementData(int year);
            bool LoadEconomicData();
            bool LoadCulturalData();

            // Data processing
            void ProcessCountryData(const std::vector<HistoricalBoundary>& countries);
            void ProcessRegionalData(const std::vector<HistoricalBoundary>& regions);
            void ProcessProvinceData(const std::vector<HistoricalBoundary>& provinces);
            void ConnectAdjacentProvinces(const std::vector<HistoricalBoundary>& boundaries);

            // Geographic feature processing
            void ProcessCoastlines();
            void ProcessRiverSystems();
            void ProcessMountainRanges();
            void ProcessForestRegions();

            // Historical data application
            void ApplyHistoricalCultures(int year);
            void ApplyHistoricalReligions(int year);
            void ApplyHistoricalGovernments(int year);

            // Validation helpers
            bool ValidateBoundaries() const;
            bool ValidateSettlements() const;
            bool ValidateConnectivity() const;

            // Progress reporting
            void ReportProgress(float percentage, const std::string& message);
            void LogError(const std::string& error);
        };

        // ============================================================================
        // Map Data Sources and File Formats
        // ============================================================================

        namespace MapDataSources {
            // Recommended free data sources for European historical maps
            struct DataSource {
                std::string name;
                std::string url;
                std::string description;
                MapDataSource type;
                bool requires_processing;
            };

            // Available data sources
            extern const DataSource NATURAL_EARTH_COUNTRIES;
            extern const DataSource NATURAL_EARTH_PROVINCES;
            extern const DataSource EUROSTAT_NUTS;
            extern const DataSource GADM_ADMIN_BOUNDARIES;
            extern const DataSource OSM_HISTORICAL_BOUNDARIES;
            extern const DataSource WHGIS_HISTORICAL_MAPS;

            // Get all available sources
            std::vector<DataSource> GetAvailableSources();

            // Download and prepare data
            bool DownloadDataSource(const DataSource& source, const std::string& output_directory);
            bool PrepareDataForGame(const std::string& input_directory, const std::string& output_directory);
        }

        // ============================================================================
        // Map Generation Utilities
        // ============================================================================

        namespace MapGenerationUtils {
            // Province boundary generation
            std::vector<HistoricalBoundary> GenerateCountyBoundaries(const HistoricalBoundary& region);
            std::vector<Coordinate> SimplifyBoundary(const std::vector<Coordinate>& boundary, double tolerance);

            // Settlement placement
            std::vector<HistoricalSettlement> GenerateHistoricalSettlements(const HistoricalBoundary& province, int year);
            Coordinate FindOptimalSettlementLocation(const HistoricalBoundary& province, const std::vector<Coordinate>& rivers);

            // Geographic feature generation
            std::vector<std::vector<Coordinate>> GenerateRiverNetwork(const std::vector<HistoricalBoundary>& provinces);
            std::vector<Coordinate> GenerateCoastline(const std::vector<HistoricalBoundary>& coastal_provinces);

            // Historical data interpolation
            std::vector<HistoricalBoundary> InterpolateBoundaryChanges(const std::vector<HistoricalBoundary>& boundaries, int from_year, int to_year);
            void ApplyHistoricalEvents(std::vector<HistoricalBoundary>& boundaries, int year);
        }

    } // namespace map
} // namespace game