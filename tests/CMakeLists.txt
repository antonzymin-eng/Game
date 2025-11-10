# ============================================================================
# tests/CMakeLists.txt - AI Director Testing Suite
# Created: November 10, 2025
# Purpose: Week 2 verification testing infrastructure
# ============================================================================

cmake_minimum_required(VERSION 3.15)

# Find Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Include project headers
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

# Get ImGui directories from parent scope
get_target_property(IMGUI_INCLUDE_DIRS imgui INTERFACE_INCLUDE_DIRECTORIES)
if(IMGUI_INCLUDE_DIRS)
    include_directories(${IMGUI_INCLUDE_DIRS})
endif()

# ============================================================================
# Threading Safety Tests (for ThreadSanitizer)
# ============================================================================

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
# Test Registration
# ============================================================================

enable_testing()

add_test(NAME ThreadingSafetyTests
    COMMAND test_ai_director_threading
)

add_test(NAME PerformanceBenchmarks
    COMMAND test_ai_director_performance
)

add_test(NAME FunctionalIntegrationTests
    COMMAND test_ai_director_integration
)

# ============================================================================
# Custom Test Targets
# ============================================================================

# Run all tests
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_ai_director_threading test_ai_director_performance test_ai_director_integration
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
    COMMAND ./test_ai_director_performance
    DEPENDS test_ai_director_performance
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
