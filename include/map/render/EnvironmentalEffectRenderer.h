// ============================================================================
// EnvironmentalEffectRenderer.h - LOD 4 Weather and Environmental Effects Renderer
// Created: November 1, 2025
// Description: Renders weather particles, atmospheric effects, and environmental
//              conditions at tactical zoom level
// ============================================================================

#pragma once

#include "map/WeatherData.h"
#include "map/ProvinceRenderComponent.h"
#include "core/ECS/EntityManager.h"
#include <memory>
#include <unordered_map>
#include <random>

// Forward declarations
struct ImDrawList;

namespace game::map {

    // Use core::ecs::EntityID
    using ::core::ecs::EntityID;

    // ========================================================================
    // Camera2D - Camera structure (matching MapRenderer)
    // ========================================================================
    struct Camera2D;  // Forward declaration (defined in TacticalTerrainRenderer.h)

    // ========================================================================
    // EnvironmentalEffectRenderer - Renders weather and environmental effects
    // ========================================================================
    class EnvironmentalEffectRenderer {
    public:
        EnvironmentalEffectRenderer(::core::ecs::EntityManager& entity_manager);
        ~EnvironmentalEffectRenderer() = default;

        // Initialize renderer
        bool Initialize();

        // Update effects (called every frame)
        void Update(float delta_time);

        // Render effects for a province
        void RenderProvinceEffects(
            const ProvinceRenderComponent& province,
            const Camera2D& camera,
            ImDrawList* draw_list
        );

        // Render all visible effects
        void RenderAllEffects(
            const Camera2D& camera,
            ImDrawList* draw_list
        );

        // Generate weather data for a province if it doesn't exist
        void GenerateWeatherForProvince(EntityID province_id);

        // Weather control
        void SetProvinceWeather(EntityID province_id, WeatherType weather);
        void SetGlobalWeather(WeatherType weather);
        void SetSeason(Season season);
        void SetTimeOfDay(TimeOfDay time);

        // Settings
        void SetShowWeatherParticles(bool show) { show_weather_particles_ = show; }
        void SetShowAtmosphericEffects(bool show) { show_atmospheric_effects_ = show; }
        void SetShowLightning(bool show) { show_lightning_ = show; }
        void SetShowFog(bool show) { show_fog_ = show; }
        void SetParticleDensity(float density) { particle_density_ = density; }
        void SetMinZoomForEffects(float zoom) { min_zoom_for_effects_ = zoom; }

        // Getters
        bool IsShowingWeatherParticles() const { return show_weather_particles_; }
        bool IsShowingAtmosphericEffects() const { return show_atmospheric_effects_; }
        bool IsShowingLightning() const { return show_lightning_; }
        bool IsShowingFog() const { return show_fog_; }
        float GetParticleDensity() const { return particle_density_; }

        // Get weather state
        const WeatherState* GetProvinceWeather(EntityID province_id) const;
        WeatherState* GetProvinceWeather(EntityID province_id);

        // Statistics
        int GetRenderedParticleCount() const { return rendered_particle_count_; }
        int GetActiveLightningCount() const { return active_lightning_count_; }
        int GetTotalParticleCount() const;

    private:
        // Core systems
        ::core::ecs::EntityManager& entity_manager_;

        // Weather data storage (province_id -> weather data)
        std::unordered_map<uint32_t, ProvinceWeatherData> weather_data_;

        // Global weather settings
        Season current_season_ = Season::SUMMER;
        TimeOfDay current_time_ = TimeOfDay::AFTERNOON;

        // Rendering settings
        bool show_weather_particles_ = true;
        bool show_atmospheric_effects_ = true;
        bool show_lightning_ = true;
        bool show_fog_ = true;
        float particle_density_ = 1.0f;      // Multiplier for particle count
        float min_zoom_for_effects_ = 2.0f;  // Minimum zoom to show effects

        // Particle pool limits
        int max_particles_per_province_ = 1000;
        int max_lightning_strikes_ = 5;

        // Statistics
        mutable int rendered_particle_count_ = 0;
        mutable int active_lightning_count_ = 0;

        // Random number generator
        std::mt19937 rng_;
        std::uniform_real_distribution<float> random_dist_;

        // Last update time for delta tracking
        float last_update_time_ = 0.0f;

        // Rendering methods
        void RenderWeatherParticles(
            const std::vector<WeatherParticle>& particles,
            const Camera2D& camera,
            ImDrawList* draw_list
        );

        void RenderParticle(
            const WeatherParticle& particle,
            const Camera2D& camera,
            ImDrawList* draw_list
        );

        void RenderLightningStrikes(
            const std::vector<LightningStrike>& strikes,
            const Camera2D& camera,
            ImDrawList* draw_list
        );

        void RenderLightning(
            const LightningStrike& lightning,
            const Camera2D& camera,
            ImDrawList* draw_list
        );

        void RenderAtmosphericEffects(
            const AtmosphericEffect& atmosphere,
            const Camera2D& camera,
            ImDrawList* draw_list
        );

        void RenderFogOverlay(
            const AtmosphericEffect& atmosphere,
            const Camera2D& camera,
            ImDrawList* draw_list
        );

        // Particle management
        void SpawnParticles(
            WeatherState& weather,
            const Rect& viewport_bounds,
            float delta_time
        );

        void RecycleDeadParticles(WeatherState& weather, const Rect& viewport_bounds);

        WeatherParticle CreateParticle(
            ParticleType type,
            const Vector2& position,
            const WindData& wind
        );

        // Lightning management
        void TriggerLightning(
            WeatherState& weather,
            const Rect& viewport_bounds
        );

        bool ShouldTriggerLightning(const WeatherState& weather, float delta_time);

        // Viewport culling
        bool IsParticleVisible(
            const Vector2& world_pos,
            const Camera2D& camera
        ) const;

        Rect GetViewportBounds(const Camera2D& camera) const;

        // Weather generation
        ProvinceWeatherData GenerateDefaultWeather(
            const ProvinceRenderComponent& province
        );

        WeatherType DetermineWeatherFromTerrain(
            const ProvinceRenderComponent& province
        );

        // Helper methods
        float RandomFloat(float min, float max);
        Vector2 RandomVelocity(ParticleType type, const WindData& wind);
        Color GetParticleColor(ParticleType type, uint8_t alpha) const;
        float GetParticleSize(ParticleType type) const;

        // Time of day lighting
        Color GetAmbientLightColor(TimeOfDay time) const;
        float GetAmbientLightIntensity(TimeOfDay time) const;
    };

} // namespace game::map
