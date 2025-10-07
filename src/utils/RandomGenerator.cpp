// ============================================================================
// RandomGenerator.cpp - Thread-safe Random Number Generation Implementation
// Created: September 24, 2025, 2:15 PM PST
// Location: src/utils/RandomGenerator.cpp
// ============================================================================

#include "utils/RandomGenerator.h"
#include <chrono>
#include <algorithm>
#include <stdexcept>

namespace utils {

    // Static member initialization
    std::unique_ptr<RandomGenerator> RandomGenerator::instance = nullptr;
    std::mutex RandomGenerator::instance_mutex;

    RandomGenerator::RandomGenerator() {
        // Use high-resolution clock for entropy
        auto now = std::chrono::high_resolution_clock::now();
        current_seed = static_cast<uint32_t>(now.time_since_epoch().count());
        generator.seed(current_seed);
    }

    RandomGenerator& RandomGenerator::getInstance() {
        std::lock_guard<std::mutex> lock(instance_mutex);
        if (!instance) {
            instance = std::unique_ptr<RandomGenerator>(new RandomGenerator());
        }
        return *instance;
    }

    void RandomGenerator::setSeed(uint32_t seed) {
        std::lock_guard<std::mutex> lock(generator_mutex);
        current_seed = seed;
        generator.seed(seed);
    }

    uint32_t RandomGenerator::getSeed() const {
        std::lock_guard<std::mutex> lock(generator_mutex);
        return current_seed;
    }

    int RandomGenerator::randomInt(int min, int max) {
        if (min > max) {
            std::swap(min, max);
        }
        
        std::lock_guard<std::mutex> lock(generator_mutex);
        std::uniform_int_distribution<int> dist(min, max);
        return dist(generator);
    }

    float RandomGenerator::randomFloat(float min, float max) {
        if (min > max) {
            std::swap(min, max);
        }
        
        std::lock_guard<std::mutex> lock(generator_mutex);
        std::uniform_real_distribution<float> dist(min, max);
        return dist(generator);
    }

    bool RandomGenerator::randomBool(float probability) {
        if (probability <= 0.0f) return false;
        if (probability >= 1.0f) return true;
        
        std::lock_guard<std::mutex> lock(generator_mutex);
        std::bernoulli_distribution dist(probability);
        return dist(generator);
    }

    bool RandomGenerator::percentageCheck(int percentage) {
        if (percentage <= 0) return false;
        if (percentage >= 100) return true;
        
        return randomBool(percentage / 100.0f);
    }

    int RandomGenerator::weightedChoice(const std::vector<int>& weights) {
        if (weights.empty()) {
            throw std::runtime_error("Cannot choose from empty weights");
        }
        
        std::lock_guard<std::mutex> lock(generator_mutex);
        std::discrete_distribution<int> dist(weights.begin(), weights.end());
        return dist(generator);
    }

    int RandomGenerator::normalDistribution(int mean, int stddev, int min_val, int max_val) {
        std::lock_guard<std::mutex> lock(generator_mutex);
        std::normal_distribution<float> dist(static_cast<float>(mean), static_cast<float>(stddev));
        
        int result = static_cast<int>(std::round(dist(generator)));
        return std::max(min_val, std::min(max_val, result));
    }

    int RandomGenerator::rollDice(int sides) {
        if (sides <= 0) {
            throw std::runtime_error("Dice must have at least 1 side");
        }
        
        return randomInt(1, sides);
    }

    int RandomGenerator::rollMultipleDice(int count, int sides) {
        if (count <= 0 || sides <= 0) {
            throw std::runtime_error("Invalid dice parameters");
        }
        
        int total = 0;
        for (int i = 0; i < count; ++i) {
            total += rollDice(sides);
        }
        return total;
    }

    // Convenience namespace implementation
    namespace random {
        int Int(int min, int max) {
            return RandomGenerator::getInstance().randomInt(min, max);
        }

        float Float(float min, float max) {
            return RandomGenerator::getInstance().randomFloat(min, max);
        }

        bool Bool(float probability) {
            return RandomGenerator::getInstance().randomBool(probability);
        }

        bool Percentage(int percentage) {
            return RandomGenerator::getInstance().percentageCheck(percentage);
        }

        int WeightedChoice(const std::vector<int>& weights) {
            return RandomGenerator::getInstance().weightedChoice(weights);
        }

        int Normal(int mean, int stddev, int min_val, int max_val) {
            return RandomGenerator::getInstance().normalDistribution(mean, stddev, min_val, max_val);
        }

        int Dice(int sides) {
            return RandomGenerator::getInstance().rollDice(sides);
        }

        int MultipleDice(int count, int sides) {
            return RandomGenerator::getInstance().rollMultipleDice(count, sides);
        }

        void SetSeed(uint32_t seed) {
            RandomGenerator::getInstance().setSeed(seed);
        }

        uint32_t GetSeed() {
            return RandomGenerator::getInstance().getSeed();
        }
    }

} // namespace utils
