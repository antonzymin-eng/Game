// ============================================================================
// RandomGenerator.h - Thread-safe Random Number Generation for Mechanica Imperii
// Created: September 24, 2025, 2:15 PM PST
// Location: include/utils/RandomGenerator.h
// ============================================================================

#pragma once

#include <random>
#include <mutex>
#include <vector>
#include <memory>

namespace utils {

    /**
     * Thread-safe random number generator using modern C++17 <random>
     * Provides deterministic seeding for testing and reproducible gameplay
     * Threading Strategy: Thread-safe singleton with mutex protection
     */
    class RandomGenerator {
    private:
        mutable std::mt19937 generator;
        mutable std::mutex generator_mutex;
        uint32_t current_seed;

        // Singleton implementation
        RandomGenerator();
        static std::unique_ptr<RandomGenerator> instance;
        static std::mutex instance_mutex;

    public:
        // Singleton access
        static RandomGenerator& getInstance();
        
        // No copy/move for singleton
        RandomGenerator(const RandomGenerator&) = delete;
        RandomGenerator& operator=(const RandomGenerator&) = delete;
        RandomGenerator(RandomGenerator&&) = delete;
        RandomGenerator& operator=(RandomGenerator&&) = delete;

        // Seed management
        void setSeed(uint32_t seed);
        uint32_t getSeed() const;
        
        // Basic random number generation
        int randomInt(int min, int max);
        float randomFloat(float min = 0.0f, float max = 1.0f);
        bool randomBool(float probability = 0.5f);
        
        // Percentage-based checks (0-100)
        bool percentageCheck(int percentage);
        
        // Array/vector selection
        template<typename T>
        const T& randomElement(const std::vector<T>& container);
        
        // Weighted random selection
        int weightedChoice(const std::vector<int>& weights);
        
        // Normal distribution (for stats like competence, loyalty)
        int normalDistribution(int mean, int stddev, int min_val, int max_val);
        
        // Dice rolling (for events)
        int rollDice(int sides);
        int rollMultipleDice(int count, int sides);
    };

    // Template implementations
    template<typename T>
    const T& RandomGenerator::randomElement(const std::vector<T>& container) {
        if (container.empty()) {
            throw std::runtime_error("Cannot select from empty container");
        }
        
        std::lock_guard<std::mutex> lock(generator_mutex);
        std::uniform_int_distribution<size_t> dist(0, container.size() - 1);
        return container[dist(generator)];
    }

    // Convenience namespace for global access
    namespace random {
        int Int(int min, int max);
        float Float(float min = 0.0f, float max = 1.0f);
        bool Bool(float probability = 0.5f);
        bool Percentage(int percentage);
        int WeightedChoice(const std::vector<int>& weights);
        int Normal(int mean, int stddev, int min_val, int max_val);
        int Dice(int sides);
        int MultipleDice(int count, int sides);
        void SetSeed(uint32_t seed);
        uint32_t GetSeed();
        
        template<typename T>
        const T& Element(const std::vector<T>& container) {
            return RandomGenerator::getInstance().randomElement(container);
        }
    }

} // namespace utils
