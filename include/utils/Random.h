// Created: November 19, 2025
// Location: include/utils/Random.h
// Purpose: Modern C++ random number generation utility

#pragma once

#include <random>
#include <chrono>

namespace utils {

/**
 * Thread-safe random number generator using modern C++ <random>
 */
class Random {
public:
    /**
     * Get singleton instance (thread-safe since C++11)
     */
    static Random& Instance() {
        static Random instance;
        return instance;
    }

    // Prevent copying
    Random(const Random&) = delete;
    Random& operator=(const Random&) = delete;

    /**
     * Generate random float in range [0.0, 1.0]
     */
    float RandomFloat() {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        return dist(m_generator);
    }

    /**
     * Generate random float in range [min, max]
     */
    float RandomFloat(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(m_generator);
    }

    /**
     * Generate random int in range [min, max]
     */
    int RandomInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(m_generator);
    }

    /**
     * Generate random bool with given probability of true
     */
    bool RandomBool(float probability = 0.5f) {
        return RandomFloat() < probability;
    }

    /**
     * Roll percentile (0-100)
     */
    int RollPercentile() {
        return RandomInt(0, 100);
    }

    /**
     * Roll dice (e.g., d6, d20)
     */
    int RollDice(int sides) {
        return RandomInt(1, sides);
    }

    /**
     * Reseed the generator (useful for testing)
     */
    void Reseed(uint64_t seed) {
        m_generator.seed(seed);
    }

private:
    Random() {
        // Seed with high-resolution clock
        auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        m_generator.seed(static_cast<uint64_t>(seed));
    }

    std::mt19937_64 m_generator;  // Mersenne Twister 64-bit
};

// ============================================================================
// Convenience Functions
// ============================================================================

/**
 * Generate random float in range [0.0, 1.0]
 */
inline float RandomFloat() {
    return Random::Instance().RandomFloat();
}

/**
 * Generate random float in range [min, max]
 */
inline float RandomFloat(float min, float max) {
    return Random::Instance().RandomFloat(min, max);
}

/**
 * Generate random int in range [min, max]
 */
inline int RandomInt(int min, int max) {
    return Random::Instance().RandomInt(min, max);
}

/**
 * Generate random bool with given probability
 */
inline bool RandomBool(float probability = 0.5f) {
    return Random::Instance().RandomBool(probability);
}

/**
 * Roll percentile (0-100)
 */
inline int RollPercentile() {
    return Random::Instance().RollPercentile();
}

/**
 * Roll dice (e.g., d6, d20)
 */
inline int RollDice(int sides) {
    return Random::Instance().RollDice(sides);
}

} // namespace utils
