#pragma once
#include <string>
#include <vector>

namespace core {

struct WeatherInfo {
    std::string name;      // region / province name
    std::string weather;   // "Clear", "Rain", etc.
    float visibility = 1.0f; // 0..1
    // optional extras (unused by UI but useful)
    float temperature_c    = 15.f;
    float precipitation_mm = 0.f;
    float wind_kph         = 5.f;
    float humidity01       = 0.5f;
};

} // namespace core
