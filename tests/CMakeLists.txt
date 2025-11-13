# ============================================================================
# tests/CMakeLists.txt - AI Director Testing Suite
# Created: November 10, 2025
# Purpose: Week 2 verification testing infrastructure
# ============================================================================

cmake_minimum_required(VERSION 3.15)

# Find Google Test (optional - only needed for AI Director tests)
find_package(GTest QUIET)
if(GTEST_FOUND)
    include_directories(${GTEST_INCLUDE_DIRS})
endif()

# Include project headers
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

# Get ImGui directories from parent scope (if available)
if(TARGET imgui)
    get_target_property(IMGUI_INCLUDE_DIRS imgui INTERFACE_INCLUDE_DIRECTORIES)
    if(IMGUI_INCLUDE_DIRS)
        include_directories(${IMGUI_INCLUDE_DIRS})
    endif()
endif()

# ============================================================================
# Threading Safety Tests (for ThreadSanitizer)
# ============================================================================

if(GTEST_FOUND)
add_executable(test_ai_director_threading
    threading/test_ai_director_threading.cpp
)

target_link_libraries(test_ai_director_threading
    ${GTEST_BOTH_LIBRARIES}
    pthread
    # Add your game libraries here
    # game_ai
    # core_ecs
    # core_threading
)

# Enable ThreadSanitizer for this target
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(test_ai_director_threading PRIVATE
        -fsanitize=thread
        -g
        -O1
    )
    target_link_options(test_ai_director_threading PRIVATE
        -fsanitize=thread
    )
endif()

# ============================================================================
# Performance Benchmarking Tests
# ============================================================================

add_executable(test_ai_director_performance
    performance/test_ai_director_performance.cpp
)

target_link_libraries(test_ai_director_performance
    ${GTEST_BOTH_LIBRARIES}
    pthread
    # Add your game libraries here
)

# Optimize for performance testing
target_compile_options(test_ai_director_performance PRIVATE
    -O2
)

add_executable(test_rendering_profile
    performance/test_rendering_profile.cpp
)

target_link_libraries(test_rendering_profile
    pthread
)

# ============================================================================
# Functional Integration Tests
# ============================================================================

add_executable(test_ai_director_integration
    integration/test_ai_director_integration.cpp
)

target_link_libraries(test_ai_director_integration
    ${GTEST_BOTH_LIBRARIES}
    pthread
    # Add your game libraries here
)

# ============================================================================
# Logging Macro Unit Tests
# ============================================================================

add_executable(test_logging_macros
    test_logging_macros.cpp
)

target_link_libraries(test_logging_macros
    ${GTEST_BOTH_LIBRARIES}
    pthread
)
endif() # GTEST_FOUND

# ============================================================================
# Testing Module Validation Tests
# ============================================================================

add_executable(test_testing_module
    test_testing_module.cpp
    ${CMAKE_SOURCE_DIR}/src/game/testing/TestingModule.cpp
)

target_link_libraries(test_testing_module
    pthread
)

# Ensure the executable can locate project headers
target_include_directories(test_testing_module PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# ============================================================================
# Diplomacy: Influence System Unit Tests (Phase 3)
# ============================================================================

add_executable(test_influence_system
    test_influence_system.cpp
    ${CMAKE_SOURCE_DIR}/src/game/diplomacy/InfluenceComponents.cpp
    ${CMAKE_SOURCE_DIR}/src/game/diplomacy/InfluenceCalculator.cpp
    ${CMAKE_SOURCE_DIR}/src/game/diplomacy/InfluenceSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/game/diplomacy/DiplomacyComponents.cpp
    ${CMAKE_SOURCE_DIR}/src/game/diplomacy/DiplomacySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/game/diplomacy/DiplomacySystemSerialization.cpp
    ${CMAKE_SOURCE_DIR}/src/game/diplomacy/DiplomaticMemory.cpp
    ${CMAKE_SOURCE_DIR}/src/game/diplomacy/MemorySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/game/diplomacy/TrustSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/core/ECS/ComponentAccessManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/ECS/MessageBus.cpp
    ${CMAKE_SOURCE_DIR}/src/core/types/TypeRegistry.cpp
    ${CMAKE_SOURCE_DIR}/src/game/config/GameConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/game/config/ConfigHelpers.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/RandomGenerator.cpp
)

target_link_libraries(test_influence_system
    ${JSONCPP_TARGET}
    pthread
)

# Platform-specific configuration
if(WIN32)
    target_compile_definitions(test_influence_system PRIVATE
        PLATFORM_WINDOWS
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
else()
    target_compile_definitions(test_influence_system PRIVATE
        PLATFORM_LINUX
    )
    target_link_libraries(test_influence_system PRIVATE dl)
endif()

# ============================================================================
# Diplomacy: Influence System Unit Tests (Phase 3)
# ============================================================================

add_executable(test_influence_system
    test_influence_system.cpp
)

target_link_libraries(test_influence_system
    ${JSONCPP_TARGET}
    pthread
)

# Platform-specific configuration
if(WIN32)
    target_compile_definitions(test_influence_system PRIVATE
        PLATFORM_WINDOWS
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
else()
    target_compile_definitions(test_influence_system PRIVATE
        PLATFORM_LINUX
    )
    target_link_libraries(test_influence_system PRIVATE dl)
endif()

# ============================================================================
# Test Registration
# ============================================================================

enable_testing()

if(GTEST_FOUND)
add_test(NAME ThreadingSafetyTests
    COMMAND test_ai_director_threading
)

add_test(NAME PerformanceBenchmarks
    COMMAND test_ai_director_performance
)

add_test(NAME FunctionalIntegrationTests
    COMMAND test_ai_director_integration
)

add_test(NAME LoggingMacroTests
    COMMAND test_logging_macros
)
endif()

add_test(NAME TestingModuleValidation
    COMMAND test_testing_module
)

add_test(NAME InfluenceSystemTests
    COMMAND test_influence_system
)

add_test(NAME InfluenceSystemTests
    COMMAND test_influence_system
)

# ============================================================================
# Custom Test Targets
# ============================================================================

# Run all tests
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS
        test_ai_director_threading
        test_ai_director_performance
        test_ai_director_integration
        test_influence_system
        test_testing_module
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Run threading tests with ThreadSanitizer
add_custom_target(run_tsan_tests
    COMMAND ./test_ai_director_threading
    DEPENDS test_ai_director_threading
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    COMMENT "Running ThreadSanitizer tests..."
)

# Run performance benchmarks
add_custom_target(run_performance_tests
    COMMAND ./test_ai_director_performance && ./test_rendering_profile
    DEPENDS test_ai_director_performance test_rendering_profile
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    COMMENT "Running performance benchmarks..."
)

# Run functional tests
add_custom_target(run_integration_tests
    COMMAND ./test_ai_director_integration
    DEPENDS test_ai_director_integration
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    COMMENT "Running functional integration tests..."
)
