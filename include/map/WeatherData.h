// ============================================================================
// WeatherData.h - LOD 4 Weather and Environmental Effect Data Structures
// Created: November 1, 2025
// Description: Weather systems, atmospheric effects, and environmental data
//              for tactical zoom level rendering
// ============================================================================

#pragma once

#include "utils/PlatformMacros.h"
#include "ProvinceRenderComponent.h"
#include <vector>
#include <cstdint>
#include <random>

namespace game::map {

    // ========================================================================
    // WeatherType - Types of weather conditions
    // ========================================================================
    enum class WeatherType : uint8_t {
        CLEAR = 0,      // Clear skies, no precipitation
        CLOUDY,         // Overcast, no precipitation
        LIGHT_RAIN,     // Light rain showers
        HEAVY_RAIN,     // Heavy rain
        LIGHT_SNOW,     // Light snowfall
        HEAVY_SNOW,     // Heavy snowfall
        FOGGY,          // Dense fog
        STORMY,         // Thunderstorm with lightning
        SANDSTORM,      // Desert sandstorm
        BLIZZARD,       // Heavy snow with strong winds
        UNKNOWN
    };

    // ========================================================================
    // Season - Seasonal variations
    // ========================================================================
    enum class Season : uint8_t {
        SPRING = 0,
        SUMMER,
        AUTUMN,
        WINTER
    };

    // ========================================================================
    // TimeOfDay - Time-based lighting variations
    // ========================================================================
    enum class TimeOfDay : uint8_t {
        DAWN = 0,       // 05:00-07:00
        MORNING,        // 07:00-12:00
        AFTERNOON,      // 12:00-17:00
        DUSK,           // 17:00-19:00
        NIGHT           // 19:00-05:00
    };

    // ========================================================================
    // ParticleType - Types of weather particles
    // ========================================================================
    enum class ParticleType : uint8_t {
        RAIN = 0,
        SNOW,
        SAND,
        DUST,
        LEAF,           // Falling leaves (autumn)
        ASH             // Volcanic ash
    };

    // ========================================================================
    // WeatherParticle - Individual particle for rain/snow/etc
    // ========================================================================
    struct WeatherParticle {
        Vector2 position;           // Current world position
        Vector2 velocity;           // Velocity vector (dx/dt, dy/dt)
        float life_time = 0.0f;     // Current age in seconds
        float max_life = 5.0f;      // Maximum lifetime before recycling
        float size = 1.0f;          // Particle size
        uint8_t alpha = 255;        // Transparency (0-255)
        ParticleType type = ParticleType::RAIN;

        WeatherParticle() = default;
        WeatherParticle(Vector2 pos, Vector2 vel, ParticleType t)
            : position(pos), velocity(vel), type(t) {}

        // Update particle position
        void Update(float delta_time, const Vector2& wind_force) {
            life_time += delta_time;

            // Apply wind
            velocity.x += wind_force.x * delta_time;
            velocity.y += wind_force.y * delta_time;

            // Update position
            position.x += velocity.x * delta_time;
            position.y += velocity.y * delta_time;

            // Fade out near end of life
            float life_ratio = life_time / max_life;
            if (life_ratio > 0.8f) {
                alpha = static_cast<uint8_t>(255 * (1.0f - life_ratio) / 0.2f);
            }
        }

        bool IsDead() const {
            return life_time >= max_life;
        }

        void Reset(Vector2 new_pos, Vector2 new_vel) {
            position = new_pos;
            velocity = new_vel;
            life_time = 0.0f;
            alpha = 255;
        }
    };

    // ========================================================================
    // LightningStrike - Temporary lightning effect
    // ========================================================================
    struct LightningStrike {
        Vector2 start_position;
        Vector2 end_position;
        float duration = 0.2f;          // Total duration in seconds
        float elapsed_time = 0.0f;
        std::vector<Vector2> segments;  // Jagged lightning path
        uint8_t brightness = 255;

        LightningStrike() = default;
        LightningStrike(Vector2 start, Vector2 end)
            : start_position(start), end_position(end) {
            GenerateSegments();
        }

        void Update(float delta_time) {
            elapsed_time += delta_time;
            // Fade out quickly
            float fade = 1.0f - (elapsed_time / duration);
            brightness = static_cast<uint8_t>(255 * std::max(0.0f, fade));
        }

        bool IsFinished() const {
            return elapsed_time >= duration;
        }

        void GenerateSegments() {
            segments.clear();
            segments.push_back(start_position);

            // Create jagged path from start to end
            const int num_segments = 8;
            for (int i = 1; i < num_segments; ++i) {
                float t = static_cast<float>(i) / num_segments;
                Vector2 point;
                point.x = start_position.x + (end_position.x - start_position.x) * t;
                point.y = start_position.y + (end_position.y - start_position.y) * t;

                // Add random offset perpendicular to main direction
                float offset = ((rand() % 100) / 100.0f - 0.5f) * 20.0f;
                point.x += offset;

                segments.push_back(point);
            }

            segments.push_back(end_position);
        }
    };

    // ========================================================================
    // AtmosphericEffect - Fog, clouds, ambient lighting
    // ========================================================================
    struct AtmosphericEffect {
        float fog_density = 0.0f;       // 0.0 (none) to 1.0 (dense)
        float cloud_coverage = 0.0f;    // 0.0 (clear) to 1.0 (overcast)
        float ambient_brightness = 1.0f; // Lighting multiplier (0.0-1.5)
        Color ambient_tint = Color(255, 255, 255); // Color tint for ambient light

        AtmosphericEffect() = default;

        // Get overlay color for fog effect
        Color GetFogOverlay() const {
            if (fog_density <= 0.0f) return Color(0, 0, 0, 0);

            uint8_t alpha = static_cast<uint8_t>(fog_density * 180.0f);
            return Color(200, 200, 210, alpha);
        }

        // Get lighting modifier for terrain/objects
        float GetLightingModifier() const {
            return ambient_brightness * (1.0f - cloud_coverage * 0.3f);
        }
    };

    // ========================================================================
    // WindData - Wind force and direction
    // ========================================================================
    struct WindData {
        Vector2 direction = Vector2(0.0f, 0.0f); // Wind direction vector
        float strength = 0.0f;                    // Wind force (0-100)
        float turbulence = 0.0f;                  // Random variation (0-1)

        WindData() = default;
        WindData(float angle_degrees, float force)
            : strength(force) {
            float rad = angle_degrees * 3.14159f / 180.0f;
            direction.x = cos(rad) * force;
            direction.y = sin(rad) * force;
        }

        Vector2 GetWindForce(float random_factor = 0.0f) const {
            float turb = turbulence * random_factor;
            return Vector2(
                direction.x * (1.0f + turb),
                direction.y * (1.0f + turb)
            );
        }
    };

    // ========================================================================
    // WeatherState - Complete weather state for a region
    // ========================================================================
    struct WeatherState {
        WeatherType current_weather = WeatherType::CLEAR;
        Season current_season = Season::SUMMER;
        TimeOfDay time_of_day = TimeOfDay::AFTERNOON;

        AtmosphericEffect atmosphere;
        WindData wind;

        float precipitation_intensity = 0.0f; // 0.0 to 1.0
        float temperature = 20.0f;            // Celsius

        // Active effects
        std::vector<WeatherParticle> particles;
        std::vector<LightningStrike> lightning_strikes;

        WeatherState() = default;

        // Update weather state
        void Update(float delta_time) {
            // Update particles
            for (auto& particle : particles) {
                particle.Update(delta_time, wind.GetWindForce(0.1f));
            }

            // Remove dead particles
            particles.erase(
                std::remove_if(particles.begin(), particles.end(),
                    [](const WeatherParticle& p) { return p.IsDead(); }),
                particles.end()
            );

            // Update lightning
            for (auto& lightning : lightning_strikes) {
                lightning.Update(delta_time);
            }

            // Remove finished lightning
            lightning_strikes.erase(
                std::remove_if(lightning_strikes.begin(), lightning_strikes.end(),
                    [](const LightningStrike& l) { return l.IsFinished(); }),
                lightning_strikes.end()
            );
        }

        // Apply weather type to state
        void SetWeather(WeatherType weather) {
            current_weather = weather;

            switch (weather) {
                case WeatherType::CLEAR:
                    atmosphere.cloud_coverage = 0.0f;
                    atmosphere.fog_density = 0.0f;
                    atmosphere.ambient_brightness = 1.2f;
                    precipitation_intensity = 0.0f;
                    break;

                case WeatherType::CLOUDY:
                    atmosphere.cloud_coverage = 0.7f;
                    atmosphere.fog_density = 0.0f;
                    atmosphere.ambient_brightness = 0.9f;
                    precipitation_intensity = 0.0f;
                    break;

                case WeatherType::LIGHT_RAIN:
                    atmosphere.cloud_coverage = 0.8f;
                    atmosphere.fog_density = 0.1f;
                    atmosphere.ambient_brightness = 0.7f;
                    precipitation_intensity = 0.3f;
                    wind.strength = 10.0f;
                    break;

                case WeatherType::HEAVY_RAIN:
                    atmosphere.cloud_coverage = 1.0f;
                    atmosphere.fog_density = 0.3f;
                    atmosphere.ambient_brightness = 0.5f;
                    precipitation_intensity = 0.8f;
                    wind.strength = 20.0f;
                    break;

                case WeatherType::LIGHT_SNOW:
                    atmosphere.cloud_coverage = 0.6f;
                    atmosphere.fog_density = 0.1f;
                    atmosphere.ambient_brightness = 1.0f;
                    atmosphere.ambient_tint = Color(240, 245, 255);
                    precipitation_intensity = 0.3f;
                    temperature = -5.0f;
                    break;

                case WeatherType::HEAVY_SNOW:
                    atmosphere.cloud_coverage = 0.9f;
                    atmosphere.fog_density = 0.4f;
                    atmosphere.ambient_brightness = 0.8f;
                    atmosphere.ambient_tint = Color(230, 235, 245);
                    precipitation_intensity = 0.7f;
                    temperature = -10.0f;
                    wind.strength = 15.0f;
                    break;

                case WeatherType::FOGGY:
                    atmosphere.cloud_coverage = 0.5f;
                    atmosphere.fog_density = 0.8f;
                    atmosphere.ambient_brightness = 0.6f;
                    precipitation_intensity = 0.0f;
                    break;

                case WeatherType::STORMY:
                    atmosphere.cloud_coverage = 1.0f;
                    atmosphere.fog_density = 0.2f;
                    atmosphere.ambient_brightness = 0.4f;
                    atmosphere.ambient_tint = Color(200, 200, 220);
                    precipitation_intensity = 1.0f;
                    wind.strength = 35.0f;
                    wind.turbulence = 0.5f;
                    break;

                case WeatherType::SANDSTORM:
                    atmosphere.cloud_coverage = 0.3f;
                    atmosphere.fog_density = 0.7f;
                    atmosphere.ambient_brightness = 0.5f;
                    atmosphere.ambient_tint = Color(220, 200, 150);
                    precipitation_intensity = 0.6f;
                    wind.strength = 40.0f;
                    wind.turbulence = 0.7f;
                    break;

                case WeatherType::BLIZZARD:
                    atmosphere.cloud_coverage = 1.0f;
                    atmosphere.fog_density = 0.9f;
                    atmosphere.ambient_brightness = 0.3f;
                    atmosphere.ambient_tint = Color(220, 230, 240);
                    precipitation_intensity = 1.0f;
                    temperature = -20.0f;
                    wind.strength = 50.0f;
                    wind.turbulence = 0.8f;
                    break;

                default:
                    break;
            }
        }
    };

    // ========================================================================
    // ProvinceWeatherData - Weather data component for provinces
    // ========================================================================
    struct ProvinceWeatherData {
        uint32_t province_id = 0;
        WeatherState weather_state;
        bool has_weather = true;

        ProvinceWeatherData() = default;
        ProvinceWeatherData(uint32_t id) : province_id(id) {}
    };

    // ========================================================================
    // Utility Functions
    // ========================================================================

    // Convert string to WeatherType
    inline WeatherType StringToWeatherType(const std::string& str) {
        if (str == "clear") return WeatherType::CLEAR;
        if (str == "cloudy") return WeatherType::CLOUDY;
        if (str == "light_rain") return WeatherType::LIGHT_RAIN;
        if (str == "heavy_rain") return WeatherType::HEAVY_RAIN;
        if (str == "light_snow") return WeatherType::LIGHT_SNOW;
        if (str == "heavy_snow") return WeatherType::HEAVY_SNOW;
        if (str == "foggy") return WeatherType::FOGGY;
        if (str == "stormy") return WeatherType::STORMY;
        if (str == "sandstorm") return WeatherType::SANDSTORM;
        if (str == "blizzard") return WeatherType::BLIZZARD;
        return WeatherType::UNKNOWN;
    }

    // Convert WeatherType to string
    inline const char* WeatherTypeToString(WeatherType type) {
        switch (type) {
            case WeatherType::CLEAR: return "clear";
            case WeatherType::CLOUDY: return "cloudy";
            case WeatherType::LIGHT_RAIN: return "light_rain";
            case WeatherType::HEAVY_RAIN: return "heavy_rain";
            case WeatherType::LIGHT_SNOW: return "light_snow";
            case WeatherType::HEAVY_SNOW: return "heavy_snow";
            case WeatherType::FOGGY: return "foggy";
            case WeatherType::STORMY: return "stormy";
            case WeatherType::SANDSTORM: return "sandstorm";
            case WeatherType::BLIZZARD: return "blizzard";
            default: return "unknown";
        }
    }

    // Get particle type for weather
    inline ParticleType GetParticleTypeForWeather(WeatherType weather) {
        switch (weather) {
            case WeatherType::LIGHT_RAIN:
            case WeatherType::HEAVY_RAIN:
            case WeatherType::STORMY:
                return ParticleType::RAIN;
            case WeatherType::LIGHT_SNOW:
            case WeatherType::HEAVY_SNOW:
            case WeatherType::BLIZZARD:
                return ParticleType::SNOW;
            case WeatherType::SANDSTORM:
                return ParticleType::SAND;
            default:
                return ParticleType::RAIN;
        }
    }

} // namespace game::map
