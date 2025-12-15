// ============================================================================
// ProvinceBuilder.h - Province Entity Builder
// Mechanica Imperii - Build province entities in ECS
// ============================================================================

#pragma once

#include "map/MapData.h"
#include "core/ECS/EntityManager.h"
#include <variant>
#include <string>

namespace game::map::loaders {

    // ============================================================================
    // Result<T> - Simple Result type for error handling (C++23 std::expected alternative)
    // ============================================================================
    template<typename T>
    class Result {
    public:
        // Construct success result
        static Result Success(T value) {
            return Result(std::move(value), "");
        }

        // Construct error result
        static Result Error(std::string error_message) {
            return Result(T{}, std::move(error_message));
        }

        // Check if result is successful
        bool IsSuccess() const { return m_error.empty(); }
        bool IsError() const { return !m_error.empty(); }

        // Get value (only valid if IsSuccess())
        const T& Value() const { return m_value; }
        T& Value() { return m_value; }

        // Get error message (only valid if IsError())
        const std::string& Error() const { return m_error; }

        // Explicit bool conversion
        explicit operator bool() const { return IsSuccess(); }

    private:
        Result(T value, std::string error)
            : m_value(std::move(value)), m_error(std::move(error)) {}

        T m_value;
        std::string m_error;
    };

    // ============================================================================
    // BatchBuildResult - Result type for batch operations
    // ============================================================================
    struct BatchBuildResult {
        std::vector<::core::ecs::EntityID> entities;
        size_t success_count = 0;
        size_t failure_count = 0;
        std::string error_summary;

        bool IsFullSuccess() const { return failure_count == 0; }
        bool IsPartialSuccess() const { return success_count > 0 && failure_count > 0; }
        bool IsFullFailure() const { return success_count == 0 && failure_count > 0; }
    };

    // ============================================================================
    // Province Builder - Creates Province Entities and Computes Adjacency
    // ============================================================================
    //
    // ProvinceBuilder is responsible for:
    // 1. Creating province entities from ProvinceData structures (BuildProvince)
    // 2. Batch creating multiple provinces (BuildProvinces)
    // 3. Computing province adjacency/neighbors using geometry (LinkProvinces)
    //
    // THREAD SAFETY:
    //   - ProvinceBuilder is NOT thread-safe
    //   - Do NOT call methods on the same instance from multiple threads
    //   - EntityManager must be externally synchronized if accessed from multiple threads
    //   - For parallel province building, create one ProvinceBuilder per thread
    //
    // Usage Example:
    //   ProvinceBuilder builder;
    //
    //   // Compute adjacency first
    //   auto link_result = builder.LinkProvinces(province_data_list, 1.0);
    //
    //   // Then build entities
    //   auto build_result = builder.BuildProvinces(province_data_list, entity_manager);
    //   if (build_result.IsFullSuccess()) {
    //       // Success!
    //   }
    //
    // Note: BuildProvince creates entities with default grey colors.
    //       MapDataLoader can override colors and add LOD boundaries afterward.
    //
    class ProvinceBuilder {
    public:
        // Configuration constants
        static constexpr size_t MIN_DEBUG_ADJACENCIES = 5;        // Show first N adjacencies in debug output
        static constexpr size_t PROGRESS_REPORT_THRESHOLD = 100;  // Show progress if comparisons > this
        static constexpr size_t PROGRESS_REPORT_INTERVAL = 10;    // Report every N%

        ProvinceBuilder() = default;
        ~ProvinceBuilder();

        // Build province entity from data - returns Result with EntityID or error
        // No exceptions thrown - all errors returned via Result
        Result<::core::ecs::EntityID> BuildProvince(
            const ProvinceData& data,
            ::core::ecs::EntityManager& entity_manager);

        // Batch build multiple provinces - returns detailed result with success/failure counts
        // Continues on individual failures, returns all successful entities
        BatchBuildResult BuildProvinces(
            const std::vector<ProvinceData>& provinces,
            ::core::ecs::EntityManager& entity_manager);

        // Link provinces (neighbors, etc.) - modifies province neighbor data
        // tolerance: distance threshold for considering provinces as neighbors (map coordinates)
        //            If tolerance <= 0.0, adaptive tolerance is calculated based on median province size
        // Returns Result<void> indicating success or error
        Result<bool> LinkProvinces(std::vector<ProvinceData>& provinces, double tolerance = 1.0);

    private:
        // No internal state - all state is in return values
    };

} // namespace game::map::loaders
