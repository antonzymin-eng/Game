# Mechanica Imperii - CMake Build Configuration
# Created: October 22, 2025
# Updated: January 13, 2025 - Fixed JsonCpp CMake policy compatibility
# Location: /Game/CMakeLists.txt

# Fix for JsonCpp CMake policy error (must be BEFORE cmake_minimum_required)
if(POLICY CMP0000)
    cmake_policy(SET CMP0000 NEW)
endif()

cmake_minimum_required(VERSION 3.15)
project(mechanica_imperii VERSION 1.0.0 LANGUAGES C CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# Windows Platform Configuration (MUST be early to prevent macro pollution)
# ============================================================================

if(WIN32)
    # CRITICAL: Define architecture BEFORE any compilation
    add_compile_definitions(
        _WIN64
        _AMD64_
        WIN64
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
    )
    
    # Force-include WindowsCleanup.h before any other includes
    add_compile_options(/FI"${CMAKE_SOURCE_DIR}/include/WindowsCleanup.h")
endif()

# ============================================================================
# Build Options (Single Definition)
# ============================================================================

option(BUILD_TESTS "Build test executables" OFF)
option(BUILD_DOCS "Build documentation" OFF)
option(USE_VENDOR_LZ4 "Fetch LZ4 if not found system-wide" ON)

# ============================================================================
# Find Dependencies
# ============================================================================

# Platform-specific message
if(WIN32)
    message(STATUS "Platform: Windows (vcpkg)")
else()
    message(STATUS "Platform: Linux (pkg-config)")
endif()

# Core dependencies
find_package(SDL2 CONFIG REQUIRED)

# GLAD with fallback for Linux
find_package(glad CONFIG QUIET)
if(NOT glad_FOUND)
    message(STATUS "glad not found - generating glad via Python")
    include(FetchContent)
    FetchContent_Declare(
        glad
        GIT_REPOSITORY https://github.com/Dav1dde/glad.git
        GIT_TAG v2.0.6
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(glad)

    # Include the glad CMake configuration
    add_subdirectory(${glad_SOURCE_DIR}/cmake glad_cmake)
    include(${glad_SOURCE_DIR}/cmake/GladConfig.cmake)

    # Generate glad library for OpenGL 3.3 core profile
    glad_add_library(glad REPRODUCIBLE STATIC API gl:core=3.3)

    set(GLAD_LIBRARIES glad)
else()
    set(GLAD_LIBRARIES glad::glad)
endif()

# Ensure compatibility with older package config files that call
# cmake_policy(VERSION ...) with very old minimums. Some vcpkg
# generated config files set a policy VERSION < 3.5 which CMake
# now rejects. Setting this variable prevents the configure-time
# failure reported by jsoncpp's config file.
if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

find_package(jsoncpp CONFIG REQUIRED)

# Set JsonCpp target based on what's available
if(TARGET JsonCpp::JsonCpp)
    set(JSONCPP_TARGET JsonCpp::JsonCpp)
    message(STATUS "Using JsonCpp target: JsonCpp::JsonCpp")
elseif(TARGET jsoncpp_static)
    set(JSONCPP_TARGET jsoncpp_static)
    message(STATUS "Using JsonCpp target: jsoncpp_static")
elseif(TARGET jsoncpp_lib)
    set(JSONCPP_TARGET jsoncpp_lib)
    message(STATUS "Using JsonCpp target: jsoncpp_lib")
else()
    message(FATAL_ERROR "No JsonCpp target found")
endif()

find_package(OpenSSL REQUIRED)
find_package(OpenGL REQUIRED)

# ImGui with backends
if(WIN32)
    # vcpkg provides imgui with backends already compiled when using features
    # Install with: vcpkg install imgui[sdl2-binding,opengl3-binding]
    find_package(imgui CONFIG REQUIRED)
    set(IMGUI_LIBRARIES imgui::imgui)
    message(STATUS "Using vcpkg imgui (backends included via features)")
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(IMGUI QUIET imgui)
    
    if(NOT IMGUI_FOUND)
        message(STATUS "ImGui not found via pkg-config - fetching via FetchContent")
        include(FetchContent)
        FetchContent_Declare(
            imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG v1.89.9
        )
        FetchContent_MakeAvailable(imgui)
        
        # Create ImGui library target
        add_library(imgui STATIC
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        )
        target_include_directories(imgui PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
        )
        target_link_libraries(imgui PUBLIC SDL2::SDL2 OpenGL::GL)
        set(IMGUI_LIBRARIES imgui)
        set(IMGUI_FOUND TRUE)
    endif()
endif()

# LZ4 (optional, auto-fetched if missing)
find_package(LZ4 QUIET)
if(NOT LZ4_FOUND AND USE_VENDOR_LZ4)
    message(STATUS "LZ4 not found - fetching via FetchContent")
    include(FetchContent)
    FetchContent_Declare(
        lz4
        GIT_REPOSITORY https://github.com/lz4/lz4.git
        GIT_TAG v1.9.4
        SOURCE_SUBDIR build/cmake
    )
    FetchContent_MakeAvailable(lz4)
    
    if(TARGET lz4_static)
        set(LZ4_LIBRARIES lz4_static)
    elseif(TARGET lz4_shared)
        set(LZ4_LIBRARIES lz4_shared)
    else()
        set(LZ4_LIBRARIES lz4)
    endif()
    set(LZ4_FOUND TRUE)
endif()

# ============================================================================
# Source Files - Core Systems
# ============================================================================

set(CONFIG_SOURCES
    src/game/config/GameConfig.cpp
    src/game/config/ConfigHelpers.cpp
)

set(SAVE_SOURCES
    src/core/save/SaveManager.cpp
    src/core/save/SaveManagerSerialization.cpp
    src/core/save/SaveManagerValidation.cpp
    src/core/save/SaveManagerRecovery.cpp
)

set(THREADING_SOURCES
    src/core/threading/ThreadedSystemManager.cpp
)

set(ECS_SOURCES
    src/core/ECS/ComponentAccessManager.cpp
    src/core/ECS/MessageBus.cpp
    src/core/types/TypeRegistry.cpp  # Fixed: enum mismatches resolved
)

set(UTILITY_SOURCES
    src/utils/RandomGenerator.cpp
)

# ============================================================================
# Source Files - Game Systems
# ============================================================================

set(ADMINISTRATIVE_SOURCES
    src/game/administration/AdministrativeSystem.cpp
    src/game/administration/AdministrativeOfficial.cpp
    src/game/administration/AdministrativeComponents.cpp
)

set(MILITARY_SOURCES
    src/game/military/MilitarySystem.cpp
    src/game/military/MilitaryRecruitmentSystem.cpp
    src/game/military/MilitaryComponents.cpp
    src/game/military/MilitaryDatabase_Utils.cpp
    src/game/military/BattleResolutionCalculator.cpp
)

set(MILITARY_ECONOMIC_BRIDGE_SOURCES
    src/game/military/MilitaryEconomicBridge.cpp
)

set(POPULATION_SOURCES
    src/game/population/PopulationSystem.cpp
    src/game/population/PopulationFactory.cpp
    src/game/population/PopulationEventProcessor.cpp
    src/game/population/PopulationUtils.cpp
    src/game/population/PopulationAggregator.cpp
    src/game/population/PopulationComponents.cpp
)

set(MANAGEMENT_SOURCES
    src/game/province/ProvinceManagementSystem.cpp
    src/game/province/ProvinceSystem.cpp
)

set(TIME_SOURCES
    src/game/time/TimeManagementSystem.cpp
    src/game/time/TimeComponents.cpp
)

set(TECHNOLOGY_SOURCES
    src/game/technology/TechnologySystem.cpp
    src/game/technology/TechnologyUtils.cpp
)

set(ECONOMIC_SOURCES
    src/game/economy/EconomicSystem.cpp
    src/game/economy/EconomicComponents.cpp
)

set(ECONOMIC_BRIDGE_SOURCES
    src/game/economy/EconomicPopulationBridge.cpp
    src/game/economy/EconomicPopulationBridgeSerialization.cpp
    src/game/economy/TradeEconomicBridge.cpp
)

set(DIPLOMACY_SOURCES
    src/game/diplomacy/DiplomacyComponents.cpp
    src/game/diplomacy/DiplomacySystem.cpp
    src/game/diplomacy/DiplomacySystemSerialization.cpp
)

set(SCENARIO_SOURCES
    src/game/scenario/ScenarioSystem.cpp
)

set(GAMEPLAY_SOURCES
    src/game/gameplay/CoreGameplaySystem.cpp
)

set(REALM_SOURCES
    src/game/realm/RealmComponents.cpp
    src/game/realm/RealmManager.cpp
)

set(TRADE_SOURCES
    src/game/trade/TradeSystem.cpp
    src/game/trade/TradeCalculator.cpp
    src/game/trade/MarketDynamicsEngine.cpp
    src/game/trade/HubManager.cpp
    src/game/trade/TradeRepository.cpp
    src/game/trade/handlers/EstablishRouteHandler.cpp
    src/game/trade/handlers/DisruptRouteHandler.cpp
)

# ============================================================================
# Source Files - AI Systems
# ============================================================================

set(AI_SOURCES
    src/game/ai/InformationPropagationSystem.cpp
    src/game/ai/AIAttentionManager.cpp
    src/game/ai/AIDirector.cpp
    src/game/ai/NationAI.cpp
    src/game/ai/CharacterAI.cpp
)

# ============================================================================
# Source Files - UI & Rendering
# ============================================================================

set(UI_SOURCES
    src/ui/Toast.cpp
    src/ui/AdministrativeUI.cpp
    src/ui/SimpleProvincePanel.cpp
    src/ui/MainMenuUI.cpp
    src/ui/PopulationInfoWindow.cpp
    src/ui/TechnologyInfoWindow.cpp
    src/ui/PerformanceWindow.cpp
    # New UI windows (Oct 29, 2025)
    src/ui/GameControlPanel.cpp
    src/ui/ProvinceInfoWindow.cpp
    src/ui/NationOverviewWindow.cpp
)

set(RENDER_SOURCES
    src/rendering/MapRenderer.cpp
    src/rendering/TerrainRenderer.cpp
    src/rendering/ViewportCuller.cpp
    src/game/map/MapDataLoader.cpp
)

# ============================================================================
# Main Executable
# ============================================================================

# Use full game main.cpp with all systems
set(MAIN_SOURCES
    apps/main.cpp
)

# Combine all sources
set(ALL_SOURCES
    ${MAIN_SOURCES}
    ${ECS_SOURCES}
    ${UTILITY_SOURCES}
    ${UI_SOURCES}
    ${CONFIG_SOURCES}
    ${SAVE_SOURCES}
    ${THREADING_SOURCES}
    ${ADMINISTRATIVE_SOURCES}
    ${MILITARY_SOURCES}
    ${MILITARY_ECONOMIC_BRIDGE_SOURCES}
    ${POPULATION_SOURCES}
    ${MANAGEMENT_SOURCES}
    ${TIME_SOURCES}
    ${TECHNOLOGY_SOURCES}
    ${ECONOMIC_SOURCES}
    ${ECONOMIC_BRIDGE_SOURCES}
    ${DIPLOMACY_SOURCES}
    ${SCENARIO_SOURCES}
    ${GAMEPLAY_SOURCES}
    ${REALM_SOURCES}
    ${TRADE_SOURCES}
    ${AI_SOURCES}
    ${RENDER_SOURCES}
)

# Create main executable
add_executable(mechanica_imperii ${ALL_SOURCES})

# Set output directory to bin/ for consistent runtime paths
set_target_properties(mechanica_imperii PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY         ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
)

# ============================================================================
# Target-Scoped Include Directories
# ============================================================================

target_include_directories(mechanica_imperii PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)


# ============================================================================
# Link Libraries
# ============================================================================

target_link_libraries(mechanica_imperii PRIVATE
    SDL2::SDL2
    SDL2::SDL2main
    ${GLAD_LIBRARIES}
    ${JSONCPP_TARGET}
    OpenSSL::SSL
    OpenSSL::Crypto
    OpenGL::GL
    ${IMGUI_LIBRARIES}
)

# Add ImGui includes if using pkg-config (target-scoped)
if(UNIX AND IMGUI_FOUND AND NOT TARGET imgui::imgui)
    target_include_directories(mechanica_imperii PRIVATE ${IMGUI_INCLUDE_DIRS})
endif()

# Windows vcpkg: Add backends directory to include path
if(WIN32 AND TARGET imgui::imgui)
    get_target_property(IMGUI_INCLUDE_DIRS imgui::imgui INTERFACE_INCLUDE_DIRECTORIES)
    foreach(IMGUI_DIR ${IMGUI_INCLUDE_DIRS})
        if(EXISTS "${IMGUI_DIR}/backends")
            target_include_directories(mechanica_imperii PRIVATE "${IMGUI_DIR}/backends")
            message(STATUS "Added ImGui backends include: ${IMGUI_DIR}/backends")
        endif()
    endforeach()
endif()

if(LZ4_FOUND)
    if(TARGET LZ4::LZ4)
        target_link_libraries(mechanica_imperii PRIVATE LZ4::LZ4)
    else()
        target_link_libraries(mechanica_imperii PRIVATE ${LZ4_LIBRARIES})
        target_include_directories(mechanica_imperii PRIVATE ${LZ4_INCLUDE_DIRS})
    endif()
endif()

if(WIN32)
    target_link_libraries(mechanica_imperii PRIVATE ws2_32 winmm)
else()
    target_link_libraries(mechanica_imperii PRIVATE pthread dl)
endif()

# ============================================================================
# Optional Test Executables
# ============================================================================

if(BUILD_TESTS)
    # Test: Enhanced Config
    add_executable(test_enhanced_config apps/test_enhanced_config.cpp ${CONFIG_SOURCES})
    
    target_include_directories(test_enhanced_config PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    
    target_link_libraries(test_enhanced_config PRIVATE ${JSONCPP_TARGET})
    
    set_target_properties(test_enhanced_config PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
    )
    
    # Test: Scenario Demo
    add_executable(test_scenario_demo 
        apps/test_scenario_demo.cpp 
        ${CONFIG_SOURCES} 
        ${ECS_SOURCES} 
        ${TIME_SOURCES} 
        ${SCENARIO_SOURCES}
    )
    
    target_include_directories(test_scenario_demo PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    
    target_link_libraries(test_scenario_demo PRIVATE
        SDL2::SDL2
        SDL2::SDL2main
        ${GLAD_LIBRARIES}
        ${JSONCPP_TARGET}
        OpenGL::GL
        ${IMGUI_LIBRARIES}
    )
    
    set_target_properties(test_scenario_demo PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
    )
    
    # Platform-specific test configuration
    if(WIN32)
        target_compile_options(test_scenario_demo PRIVATE 
            /FI"${CMAKE_SOURCE_DIR}/include/WindowsCleanup.h"
        )
        target_compile_definitions(test_scenario_demo PRIVATE
            PLATFORM_WINDOWS
            NOMINMAX
            _CRT_SECURE_NO_WARNINGS
        )
    else()
        target_compile_definitions(test_scenario_demo PRIVATE
            PLATFORM_LINUX
        )
        target_link_libraries(test_scenario_demo PRIVATE pthread dl)
    endif()
    
    # Add ImGui includes for test if needed
    if(UNIX AND IMGUI_FOUND AND NOT TARGET imgui::imgui)
        target_include_directories(test_scenario_demo PRIVATE ${IMGUI_INCLUDE_DIRS})
    endif()
endif()

# ============================================================================
# Resource Files
# ============================================================================

configure_file(${CMAKE_SOURCE_DIR}/config/GameConfig.json ${CMAKE_BINARY_DIR}/bin/data/GameConfig.json COPYONLY)

file(GLOB SHADER_FILES "${CMAKE_SOURCE_DIR}/shaders/*.glsl")
foreach(SHADER ${SHADER_FILES})
    configure_file(${SHADER} ${CMAKE_BINARY_DIR}/bin/shaders/ COPYONLY)
endforeach()

file(GLOB DATA_FILES "${CMAKE_SOURCE_DIR}/data/*.json")
foreach(DATA_FILE ${DATA_FILES})
    configure_file(${DATA_FILE} ${CMAKE_BINARY_DIR}/bin/data/ COPYONLY)
endforeach()

# ============================================================================
# Build Summary
# ============================================================================

message(STATUS "==============================================")
message(STATUS "Mechanica Imperii - Build Configuration")
message(STATUS "==============================================")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "")
message(STATUS "Production-Ready Systems: 18")
message(STATUS "  Core: ECS, Threading, Save, Config")
message(STATUS "  Game: Economic, Population, Military, Admin")
message(STATUS "  Game: Diplomacy, Technology, Time, Province")
message(STATUS "  AI: Director, Nation, Character, Attention, Info")
message(STATUS "  Render: Map, Terrain, Viewport Culling")
message(STATUS "")
message(STATUS "Output: ${CMAKE_BINARY_DIR}/bin/mechanica_imperii")
message(STATUS "Include Scope: Target-scoped (no global pollution)")
message(STATUS "==============================================")
