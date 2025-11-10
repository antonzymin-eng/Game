// ============================================================================
// DebugAssert.h - Debug Assertion Utilities
// Created: 2025-11-10 - Phase 2.1 Implementation
// Location: include/utils/DebugAssert.h
// ============================================================================

#pragma once

#include <cassert>
#include <string>
#include "core/logging/Logger.h"

namespace game::debug {

    // ============================================================================
    // Debug Assertion Macros
    // ============================================================================

    #ifdef NDEBUG
        // Release build - assertions compiled out
        #define DEBUG_ASSERT(condition, message) ((void)0)
        #define DEBUG_ASSERT_MSG(condition, category, message) ((void)0)
        #define VERIFY_COMPONENT(component, type, entity_id) ((void)0)
    #else
        // Debug build - assertions active

        /**
         * @brief Basic debug assertion
         * @param condition Expression that should be true
         * @param message Error message if assertion fails
         */
        #define DEBUG_ASSERT(condition, message) \
            do { \
                if (!(condition)) { \
                    ::core::logging::LogError("ASSERTION_FAILED", \
                        std::string("Assertion failed: ") + (message) + \
                        " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
                    assert(condition); \
                } \
            } while(0)

        /**
         * @brief Debug assertion with logging category
         * @param condition Expression that should be true
         * @param category Logging category for better filtering
         * @param message Error message if assertion fails
         */
        #define DEBUG_ASSERT_MSG(condition, category, message) \
            do { \
                if (!(condition)) { \
                    ::core::logging::LogError(category, \
                        std::string("Assertion failed: ") + (message) + \
                        " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
                    assert(condition); \
                } \
            } while(0)

        /**
         * @brief Verify component pointer is valid
         * @param component Raw pointer to component
         * @param type Component type name (for error message)
         * @param entity_id Entity ID (for error message)
         */
        #define VERIFY_COMPONENT(component, type, entity_id) \
            DEBUG_ASSERT_MSG((component) != nullptr, \
                "ComponentAccess", \
                std::string(type) + " missing for entity " + std::to_string(entity_id))

    #endif

    // ============================================================================
    // Component Lifetime Assertions
    // ============================================================================

    /**
     * @brief Verify component exists for entity (debug only)
     * @param component Component pointer from GetComponent
     * @param system_name Name of calling system
     * @param entity_id Entity ID being accessed
     * @return True if component valid, false otherwise
     */
    inline bool VerifyComponentLifetime(void* component,
                                       const char* system_name,
                                       uint64_t entity_id) {
#ifndef NDEBUG
        if (component == nullptr) {
            ::core::logging::LogWarning(system_name,
                "Component not found for entity " + std::to_string(entity_id) +
                " - may have been deleted");
            return false;
        }
#endif
        return component != nullptr;
    }

} // namespace game::debug
