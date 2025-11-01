// ============================================================================
// EnvironmentalEffectRenderer.cpp - Weather and Environmental Effects Implementation
// Created: November 1, 2025
// ============================================================================

#include "map/render/EnvironmentalEffectRenderer.h"
#include "map/render/TacticalTerrainRenderer.h"  // For Camera2D
#include "utils/PlatformCompat.h"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace game::map {

    // ========================================================================
    // Constructor / Destructor
    // ========================================================================
    EnvironmentalEffectRenderer::EnvironmentalEffectRenderer(::core::ecs::EntityManager& entity_manager)
        : entity_manager_(entity_manager)
        , rng_(std::random_device{}())
        , random_dist_(0.0f, 1.0f)
    {
    }

    // ========================================================================
    // Initialization
    // ========================================================================
    bool EnvironmentalEffectRenderer::Initialize() {
        std::cout << "EnvironmentalEffectRenderer: Initializing..." << std::endl;
        weather_data_.clear();
        std::cout << "EnvironmentalEffectRenderer: Initialized successfully" << std::endl;
        return true;
    }

    // ========================================================================
    // Update
    // ========================================================================
    void EnvironmentalEffectRenderer::Update(float delta_time) {
        // Update all weather states
        for (auto& [province_id, weather_data] : weather_data_) {
            weather_data.weather_state.Update(delta_time);
        }

        last_update_time_ += delta_time;
    }

    // ========================================================================
    // Main Rendering
    // ========================================================================
    void EnvironmentalEffectRenderer::RenderProvinceEffects(
        const ProvinceRenderComponent& province,
        const Camera2D& camera,
        ImDrawList* draw_list)
    {
        // Only render effects at sufficient zoom levels
        if (camera.zoom < min_zoom_for_effects_) {
            return;
        }

        // Get or generate weather data for this province
        auto it = weather_data_.find(province.province_id);
        if (it == weather_data_.end()) {
            weather_data_[province.province_id] = GenerateDefaultWeather(province);
            it = weather_data_.find(province.province_id);
        }

        ProvinceWeatherData& weather_data = it->second;
        if (!weather_data.has_weather) {
            return;
        }

        WeatherState& weather = weather_data.weather_state;

        // Get viewport bounds for particle spawning
        Rect viewport_bounds = GetViewportBounds(camera);

        // Spawn new particles if needed
        if (show_weather_particles_ && weather.precipitation_intensity > 0.0f) {
            SpawnParticles(weather, viewport_bounds, 1.0f / 60.0f);  // Assume 60 FPS
        }

        // Trigger lightning for storms
        if (show_lightning_ && weather.current_weather == WeatherType::STORMY) {
            if (ShouldTriggerLightning(weather, 1.0f / 60.0f)) {
                TriggerLightning(weather, viewport_bounds);
            }
        }

        // Render atmospheric effects first (background)
        if (show_atmospheric_effects_) {
            RenderAtmosphericEffects(weather.atmosphere, camera, draw_list);
        }

        // Render weather particles
        if (show_weather_particles_) {
            RenderWeatherParticles(weather.particles, camera, draw_list);
        }

        // Render lightning
        if (show_lightning_) {
            RenderLightningStrikes(weather.lightning_strikes, camera, draw_list);
        }

        // Render fog overlay (foreground)
        if (show_fog_ && weather.atmosphere.fog_density > 0.0f) {
            RenderFogOverlay(weather.atmosphere, camera, draw_list);
        }

        // Recycle dead particles
        RecycleDeadParticles(weather, viewport_bounds);
    }

    void EnvironmentalEffectRenderer::RenderAllEffects(
        const Camera2D& camera,
        ImDrawList* draw_list)
    {
        rendered_particle_count_ = 0;
        active_lightning_count_ = 0;

        // Get all provinces with render components
        auto entities = entity_manager_.GetEntitiesWithComponent<ProvinceRenderComponent>();

        for (const auto& entity_id : entities) {
            auto render = entity_manager_.GetComponent<ProvinceRenderComponent>(entity_id);
            if (!render) continue;

            RenderProvinceEffects(*render, camera, draw_list);
        }
    }

    // ========================================================================
    // Weather Particle Rendering
    // ========================================================================
    void EnvironmentalEffectRenderer::RenderWeatherParticles(
        const std::vector<WeatherParticle>& particles,
        const Camera2D& camera,
        ImDrawList* draw_list)
    {
        for (const auto& particle : particles) {
            if (IsParticleVisible(particle.position, camera)) {
                RenderParticle(particle, camera, draw_list);
                rendered_particle_count_++;
            }
        }
    }

    void EnvironmentalEffectRenderer::RenderParticle(
        const WeatherParticle& particle,
        const Camera2D& camera,
        ImDrawList* draw_list)
    {
        Vector2 screen_pos = camera.WorldToScreen(particle.position.x, particle.position.y);
        Color color = GetParticleColor(particle.type, particle.alpha);

        switch (particle.type) {
            case ParticleType::RAIN:
                // Draw rain as vertical line
                {
                    float length = 5.0f * camera.zoom;
                    draw_list->AddLine(
                        ImVec2(screen_pos.x, screen_pos.y),
                        ImVec2(screen_pos.x, screen_pos.y + length),
                        IM_COL32(color.r, color.g, color.b, particle.alpha),
                        1.0f
                    );
                }
                break;

            case ParticleType::SNOW:
                // Draw snow as small circle
                {
                    float radius = particle.size * camera.zoom;
                    if (radius < 1.0f) radius = 1.0f;
                    draw_list->AddCircleFilled(
                        ImVec2(screen_pos.x, screen_pos.y),
                        radius,
                        IM_COL32(color.r, color.g, color.b, particle.alpha)
                    );
                }
                break;

            case ParticleType::SAND:
            case ParticleType::DUST:
                // Draw dust/sand as small rectangle
                {
                    float size = particle.size * camera.zoom;
                    if (size < 1.0f) size = 1.0f;
                    draw_list->AddRectFilled(
                        ImVec2(screen_pos.x - size / 2, screen_pos.y - size / 2),
                        ImVec2(screen_pos.x + size / 2, screen_pos.y + size / 2),
                        IM_COL32(color.r, color.g, color.b, particle.alpha)
                    );
                }
                break;

            case ParticleType::LEAF:
                // Draw leaf as rotated small rectangle
                {
                    float size = particle.size * camera.zoom;
                    draw_list->AddRectFilled(
                        ImVec2(screen_pos.x - size, screen_pos.y),
                        ImVec2(screen_pos.x + size, screen_pos.y + size / 2),
                        IM_COL32(color.r, color.g, color.b, particle.alpha)
                    );
                }
                break;

            case ParticleType::ASH:
                // Draw ash as very small fading circle
                {
                    float radius = particle.size * camera.zoom * 0.5f;
                    if (radius < 0.5f) radius = 0.5f;
                    draw_list->AddCircleFilled(
                        ImVec2(screen_pos.x, screen_pos.y),
                        radius,
                        IM_COL32(color.r, color.g, color.b, particle.alpha / 2)
                    );
                }
                break;
        }
    }

    // ========================================================================
    // Lightning Rendering
    // ========================================================================
    void EnvironmentalEffectRenderer::RenderLightningStrikes(
        const std::vector<LightningStrike>& strikes,
        const Camera2D& camera,
        ImDrawList* draw_list)
    {
        for (const auto& lightning : strikes) {
            RenderLightning(lightning, camera, draw_list);
            active_lightning_count_++;
        }
    }

    void EnvironmentalEffectRenderer::RenderLightning(
        const LightningStrike& lightning,
        const Camera2D& camera,
        ImDrawList* draw_list)
    {
        if (lightning.segments.size() < 2) return;

        // Draw main lightning bolt
        for (size_t i = 0; i < lightning.segments.size() - 1; ++i) {
            Vector2 start_screen = camera.WorldToScreen(
                lightning.segments[i].x,
                lightning.segments[i].y
            );
            Vector2 end_screen = camera.WorldToScreen(
                lightning.segments[i + 1].x,
                lightning.segments[i + 1].y
            );

            // Main bolt
            draw_list->AddLine(
                ImVec2(start_screen.x, start_screen.y),
                ImVec2(end_screen.x, end_screen.y),
                IM_COL32(255, 255, 255, lightning.brightness),
                3.0f
            );

            // Glow effect
            draw_list->AddLine(
                ImVec2(start_screen.x, start_screen.y),
                ImVec2(end_screen.x, end_screen.y),
                IM_COL32(180, 200, 255, lightning.brightness / 2),
                6.0f
            );
        }
    }

    // ========================================================================
    // Atmospheric Effects Rendering
    // ========================================================================
    void EnvironmentalEffectRenderer::RenderAtmosphericEffects(
        const AtmosphericEffect& atmosphere,
        const Camera2D& camera,
        ImDrawList* draw_list)
    {
        // Apply ambient lighting tint to entire viewport
        if (atmosphere.ambient_brightness < 1.0f ||
            atmosphere.ambient_tint.r != 255 ||
            atmosphere.ambient_tint.g != 255 ||
            atmosphere.ambient_tint.b != 255) {

            // Calculate darkening/tinting overlay
            uint8_t overlay_alpha = static_cast<uint8_t>(
                (1.0f - atmosphere.ambient_brightness) * 100.0f
            );

            if (overlay_alpha > 0) {
                draw_list->AddRectFilled(
                    ImVec2(0, 0),
                    ImVec2(camera.viewport_width, camera.viewport_height),
                    IM_COL32(
                        atmosphere.ambient_tint.r,
                        atmosphere.ambient_tint.g,
                        atmosphere.ambient_tint.b,
                        overlay_alpha
                    )
                );
            }
        }
    }

    void EnvironmentalEffectRenderer::RenderFogOverlay(
        const AtmosphericEffect& atmosphere,
        const Camera2D& camera,
        ImDrawList* draw_list)
    {
        Color fog_color = atmosphere.GetFogOverlay();
        if (fog_color.a == 0) return;

        // Draw fog as semi-transparent overlay
        draw_list->AddRectFilled(
            ImVec2(0, 0),
            ImVec2(camera.viewport_width, camera.viewport_height),
            IM_COL32(fog_color.r, fog_color.g, fog_color.b, fog_color.a)
        );
    }

    // ========================================================================
    // Particle Management
    // ========================================================================
    void EnvironmentalEffectRenderer::SpawnParticles(
        WeatherState& weather,
        const Rect& viewport_bounds,
        float delta_time)
    {
        // Calculate spawn rate based on precipitation intensity
        int particles_per_second = static_cast<int>(
            weather.precipitation_intensity * 500.0f * particle_density_
        );
        int particles_to_spawn = static_cast<int>(particles_per_second * delta_time);

        // Limit total particles
        int current_count = static_cast<int>(weather.particles.size());
        int max_particles = static_cast<int>(max_particles_per_province_ * particle_density_);

        if (current_count >= max_particles) {
            return;
        }

        particles_to_spawn = std::min(particles_to_spawn, max_particles - current_count);

        // Spawn particles
        ParticleType particle_type = GetParticleTypeForWeather(weather.current_weather);

        for (int i = 0; i < particles_to_spawn; ++i) {
            // Spawn at random position above viewport
            Vector2 spawn_pos(
                RandomFloat(viewport_bounds.min_x, viewport_bounds.max_x),
                viewport_bounds.min_y - 50.0f  // Spawn above viewport
            );

            WeatherParticle particle = CreateParticle(particle_type, spawn_pos, weather.wind);
            weather.particles.push_back(particle);
        }
    }

    void EnvironmentalEffectRenderer::RecycleDeadParticles(
        WeatherState& weather,
        const Rect& viewport_bounds)
    {
        // Remove or recycle particles that have left the viewport or died
        for (auto& particle : weather.particles) {
            if (particle.IsDead() ||
                particle.position.y > viewport_bounds.max_y + 100.0f ||
                particle.position.x < viewport_bounds.min_x - 100.0f ||
                particle.position.x > viewport_bounds.max_x + 100.0f) {

                // Recycle particle at top of viewport
                Vector2 new_pos(
                    RandomFloat(viewport_bounds.min_x, viewport_bounds.max_x),
                    viewport_bounds.min_y - 50.0f
                );
                particle.Reset(new_pos, RandomVelocity(particle.type, weather.wind));
            }
        }
    }

    WeatherParticle EnvironmentalEffectRenderer::CreateParticle(
        ParticleType type,
        const Vector2& position,
        const WindData& wind)
    {
        WeatherParticle particle;
        particle.position = position;
        particle.type = type;
        particle.velocity = RandomVelocity(type, wind);
        particle.size = GetParticleSize(type);
        particle.max_life = RandomFloat(3.0f, 8.0f);
        particle.life_time = 0.0f;
        particle.alpha = 255;

        return particle;
    }

    // ========================================================================
    // Lightning Management
    // ========================================================================
    void EnvironmentalEffectRenderer::TriggerLightning(
        WeatherState& weather,
        const Rect& viewport_bounds)
    {
        if (static_cast<int>(weather.lightning_strikes.size()) >= max_lightning_strikes_) {
            return;
        }

        // Random position in viewport
        Vector2 start_pos(
            RandomFloat(viewport_bounds.min_x, viewport_bounds.max_x),
            viewport_bounds.min_y
        );

        Vector2 end_pos(
            start_pos.x + RandomFloat(-50.0f, 50.0f),
            RandomFloat(viewport_bounds.min_y + 100.0f, viewport_bounds.max_y)
        );

        LightningStrike lightning(start_pos, end_pos);
        weather.lightning_strikes.push_back(lightning);
    }

    bool EnvironmentalEffectRenderer::ShouldTriggerLightning(
        const WeatherState& weather,
        float delta_time)
    {
        // Random chance based on weather intensity
        float chance = 0.01f;  // 1% chance per frame for stormy weather
        return RandomFloat(0.0f, 1.0f) < (chance * delta_time * 60.0f);
    }

    // ========================================================================
    // Weather Control
    // ========================================================================
    void EnvironmentalEffectRenderer::SetProvinceWeather(EntityID province_id, WeatherType weather) {
        auto it = weather_data_.find(static_cast<uint32_t>(province_id.id));
        if (it != weather_data_.end()) {
            it->second.weather_state.SetWeather(weather);
        }
    }

    void EnvironmentalEffectRenderer::SetGlobalWeather(WeatherType weather) {
        for (auto& [province_id, weather_data] : weather_data_) {
            weather_data.weather_state.SetWeather(weather);
        }
    }

    void EnvironmentalEffectRenderer::SetSeason(Season season) {
        current_season_ = season;
        for (auto& [province_id, weather_data] : weather_data_) {
            weather_data.weather_state.current_season = season;
        }
    }

    void EnvironmentalEffectRenderer::SetTimeOfDay(TimeOfDay time) {
        current_time_ = time;
        for (auto& [province_id, weather_data] : weather_data_) {
            weather_data.weather_state.time_of_day = time;
        }
    }

    // ========================================================================
    // Weather State Access
    // ========================================================================
    const WeatherState* EnvironmentalEffectRenderer::GetProvinceWeather(EntityID province_id) const {
        auto it = weather_data_.find(static_cast<uint32_t>(province_id.id));
        if (it != weather_data_.end()) {
            return &it->second.weather_state;
        }
        return nullptr;
    }

    WeatherState* EnvironmentalEffectRenderer::GetProvinceWeather(EntityID province_id) {
        auto it = weather_data_.find(static_cast<uint32_t>(province_id.id));
        if (it != weather_data_.end()) {
            return &it->second.weather_state;
        }
        return nullptr;
    }

    // ========================================================================
    // Statistics
    // ========================================================================
    int EnvironmentalEffectRenderer::GetTotalParticleCount() const {
        int total = 0;
        for (const auto& [province_id, weather_data] : weather_data_) {
            total += static_cast<int>(weather_data.weather_state.particles.size());
        }
        return total;
    }

    // ========================================================================
    // Viewport Culling
    // ========================================================================
    bool EnvironmentalEffectRenderer::IsParticleVisible(
        const Vector2& world_pos,
        const Camera2D& camera) const
    {
        Vector2 screen_pos = camera.WorldToScreen(world_pos.x, world_pos.y);

        return screen_pos.x >= -50.0f && screen_pos.x <= camera.viewport_width + 50.0f &&
               screen_pos.y >= -50.0f && screen_pos.y <= camera.viewport_height + 50.0f;
    }

    Rect EnvironmentalEffectRenderer::GetViewportBounds(const Camera2D& camera) const {
        Vector2 top_left = camera.ScreenToWorld(0, 0);
        Vector2 bottom_right = camera.ScreenToWorld(camera.viewport_width, camera.viewport_height);

        return Rect(top_left.x, top_left.y, bottom_right.x, bottom_right.y);
    }

    // ========================================================================
    // Weather Generation
    // ========================================================================
    void EnvironmentalEffectRenderer::GenerateWeatherForProvince(EntityID province_id) {
        auto render = entity_manager_.GetComponent<ProvinceRenderComponent>(province_id);
        if (!render) return;

        weather_data_[static_cast<uint32_t>(province_id.id)] = GenerateDefaultWeather(*render);
    }

    ProvinceWeatherData EnvironmentalEffectRenderer::GenerateDefaultWeather(
        const ProvinceRenderComponent& province)
    {
        ProvinceWeatherData data(province.province_id);
        data.has_weather = true;

        // Determine weather based on terrain type
        WeatherType weather = DetermineWeatherFromTerrain(province);
        data.weather_state.SetWeather(weather);
        data.weather_state.current_season = current_season_;
        data.weather_state.time_of_day = current_time_;

        return data;
    }

    WeatherType EnvironmentalEffectRenderer::DetermineWeatherFromTerrain(
        const ProvinceRenderComponent& province)
    {
        // For now, randomize weather based on terrain
        // In a real game, this would be more sophisticated

        float rand = RandomFloat(0.0f, 1.0f);

        if (province.terrain_type == TerrainType::MOUNTAINS) {
            if (rand < 0.3f) return WeatherType::LIGHT_SNOW;
            if (rand < 0.5f) return WeatherType::CLOUDY;
            return WeatherType::CLEAR;
        }
        else if (province.terrain_type == TerrainType::DESERT) {
            if (rand < 0.1f) return WeatherType::SANDSTORM;
            return WeatherType::CLEAR;
        }
        else if (province.terrain_type == TerrainType::PLAINS) {
            if (rand < 0.2f) return WeatherType::LIGHT_RAIN;
            if (rand < 0.4f) return WeatherType::CLOUDY;
            return WeatherType::CLEAR;
        }
        else if (province.terrain_type == TerrainType::FOREST) {
            if (rand < 0.3f) return WeatherType::LIGHT_RAIN;
            if (rand < 0.5f) return WeatherType::CLOUDY;
            return WeatherType::CLEAR;
        }

        // Default to clear
        return WeatherType::CLEAR;
    }

    // ========================================================================
    // Helper Methods
    // ========================================================================
    float EnvironmentalEffectRenderer::RandomFloat(float min, float max) {
        return min + random_dist_(rng_) * (max - min);
    }

    Vector2 EnvironmentalEffectRenderer::RandomVelocity(ParticleType type, const WindData& wind) {
        Vector2 velocity;

        switch (type) {
            case ParticleType::RAIN:
                velocity.x = wind.direction.x + RandomFloat(-5.0f, 5.0f);
                velocity.y = 200.0f + RandomFloat(-20.0f, 20.0f);  // Fast downward
                break;

            case ParticleType::SNOW:
                velocity.x = wind.direction.x + RandomFloat(-10.0f, 10.0f);
                velocity.y = 30.0f + RandomFloat(-10.0f, 10.0f);  // Slow downward
                break;

            case ParticleType::SAND:
            case ParticleType::DUST:
                velocity.x = wind.direction.x * 2.0f + RandomFloat(-20.0f, 20.0f);
                velocity.y = RandomFloat(-10.0f, 10.0f);  // Mostly horizontal
                break;

            case ParticleType::LEAF:
                velocity.x = wind.direction.x + RandomFloat(-15.0f, 15.0f);
                velocity.y = 20.0f + RandomFloat(-10.0f, 10.0f);  // Slow downward with drift
                break;

            case ParticleType::ASH:
                velocity.x = wind.direction.x * 0.5f + RandomFloat(-5.0f, 5.0f);
                velocity.y = 10.0f + RandomFloat(-5.0f, 5.0f);  // Very slow
                break;
        }

        return velocity;
    }

    Color EnvironmentalEffectRenderer::GetParticleColor(ParticleType type, uint8_t alpha) const {
        switch (type) {
            case ParticleType::RAIN:
                return Color(150, 180, 220, alpha);
            case ParticleType::SNOW:
                return Color(240, 245, 255, alpha);
            case ParticleType::SAND:
                return Color(220, 190, 140, alpha);
            case ParticleType::DUST:
                return Color(180, 170, 150, alpha);
            case ParticleType::LEAF:
                return Color(180, 140, 60, alpha);
            case ParticleType::ASH:
                return Color(80, 80, 80, alpha);
            default:
                return Color(255, 255, 255, alpha);
        }
    }

    float EnvironmentalEffectRenderer::GetParticleSize(ParticleType type) const {
        switch (type) {
            case ParticleType::RAIN:
                return 1.0f;
            case ParticleType::SNOW:
                return 2.0f;
            case ParticleType::SAND:
                return 1.5f;
            case ParticleType::DUST:
                return 1.0f;
            case ParticleType::LEAF:
                return 3.0f;
            case ParticleType::ASH:
                return 0.8f;
            default:
                return 1.0f;
        }
    }

    // ========================================================================
    // Time of Day Lighting
    // ========================================================================
    Color EnvironmentalEffectRenderer::GetAmbientLightColor(TimeOfDay time) const {
        switch (time) {
            case TimeOfDay::DAWN:
                return Color(255, 200, 150);  // Warm orange
            case TimeOfDay::MORNING:
                return Color(255, 250, 240);  // Bright white
            case TimeOfDay::AFTERNOON:
                return Color(255, 255, 255);  // Pure white
            case TimeOfDay::DUSK:
                return Color(255, 180, 120);  // Warm red-orange
            case TimeOfDay::NIGHT:
                return Color(150, 160, 200);  // Cool blue
            default:
                return Color(255, 255, 255);
        }
    }

    float EnvironmentalEffectRenderer::GetAmbientLightIntensity(TimeOfDay time) const {
        switch (time) {
            case TimeOfDay::DAWN:
                return 0.6f;
            case TimeOfDay::MORNING:
                return 1.0f;
            case TimeOfDay::AFTERNOON:
                return 1.2f;
            case TimeOfDay::DUSK:
                return 0.5f;
            case TimeOfDay::NIGHT:
                return 0.3f;
            default:
                return 1.0f;
        }
    }

} // namespace game::map
