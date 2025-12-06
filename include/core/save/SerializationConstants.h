// Created: December 5, 2025
// Location: include/core/save/SerializationConstants.h
// Purpose: Constants and validation rules for serialization

#pragma once

#include <cstdint>
#include <limits>

namespace game::core::serialization {

// =============================================================================
// Schema Versioning
// =============================================================================

constexpr int SERIALIZATION_VERSION = 1;

// Component-specific versions
constexpr int TRAITS_COMPONENT_VERSION = 1;
constexpr int CHARACTER_EDUCATION_VERSION = 1;
constexpr int CHARACTER_LIFE_EVENTS_VERSION = 1;
constexpr int CHARACTER_RELATIONSHIPS_VERSION = 1;
constexpr int POPULATION_COMPONENT_VERSION = 1;

// =============================================================================
// Validation Limits - Traits Component
// =============================================================================

constexpr size_t MAX_TRAIT_COUNT = 50;
constexpr int64_t MIN_TIMESTAMP_MS = 0;  // Epoch
constexpr int64_t MAX_TIMESTAMP_MS = 4102444800000;  // Year 2100

// =============================================================================
// Validation Limits - Character Education
// =============================================================================

constexpr int MIN_SKILL_XP = 0;
constexpr int MAX_SKILL_XP = 100000;  // Reasonable max XP
constexpr float MIN_LEARNING_RATE = 0.1f;
constexpr float MAX_LEARNING_RATE = 5.0f;

// =============================================================================
// Validation Limits - Character Life Events
// =============================================================================

constexpr size_t MAX_LIFE_EVENTS = 1000;
constexpr float MIN_IMPACT_VALUE = -1000.0f;
constexpr float MAX_IMPACT_VALUE = 1000.0f;
constexpr int MIN_AGE = 0;
constexpr int MAX_AGE = 200;

// =============================================================================
// Validation Limits - Character Relationships
// =============================================================================

constexpr int MIN_OPINION = -100;
constexpr int MAX_OPINION = 100;
constexpr double MIN_BOND_STRENGTH = 0.0;
constexpr double MAX_BOND_STRENGTH = 1.0;
constexpr size_t MAX_RELATIONSHIPS = 500;
constexpr size_t MAX_MARRIAGES = 20;
constexpr size_t MAX_CHILDREN = 50;

// =============================================================================
// Validation Limits - Population Component
// =============================================================================

constexpr int MIN_POPULATION = 0;
constexpr int MAX_PROVINCE_POPULATION = 10000000;  // 10 million per province
constexpr int MAX_POPULATION_GROUP_SIZE = 5000000;  // 5 million per group

constexpr double MIN_RATE = 0.0;
constexpr double MAX_RATE = 1.0;

// Birth/death rates (annual percentages 0-15%)
constexpr double MIN_DEMOGRAPHIC_RATE = 0.0;
constexpr double MAX_DEMOGRAPHIC_RATE = 0.15;

// Happiness, health, etc. (0.0 to 1.0)
constexpr double MIN_QUALITY_METRIC = 0.0;
constexpr double MAX_QUALITY_METRIC = 1.0;

constexpr size_t MAX_POPULATION_GROUPS_PER_PROVINCE = 100;
constexpr size_t MAX_EMPLOYMENT_TYPES = 50;

// Economic values
constexpr double MIN_WEALTH = 0.0;
constexpr double MAX_WEALTH_PER_CAPITA = 1000000.0;  // 1M gold per capita max

constexpr double MIN_TAXATION = 0.0;
constexpr double MAX_TAXATION = 1.0;  // 100% taxation max

// Military values
constexpr double MIN_MILITARY_QUALITY = 0.0;
constexpr double MAX_MILITARY_QUALITY = 1.0;

// Family structure
constexpr double MIN_HOUSEHOLD_SIZE = 1.0;
constexpr double MAX_HOUSEHOLD_SIZE = 20.0;

// =============================================================================
// Helper Functions
// =============================================================================

template<typename T>
inline T Clamp(T value, T min_val, T max_val) {
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

inline bool IsValidTimestamp(int64_t timestamp_ms) {
    return timestamp_ms >= MIN_TIMESTAMP_MS && timestamp_ms <= MAX_TIMESTAMP_MS;
}

} // namespace game::core::serialization
