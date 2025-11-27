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
# Platform and Compiler Configuration (MUST be early to prevent macro pollution)
# ============================================================================

# Windows-specific defines (for both MSVC and MinGW)
if(WIN32)
    add_compile_definitions(
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
    )

    # Force-include WindowsCleanup.h before any other includes
    # Use compiler-specific syntax
    if(MSVC)
        add_compile_options("/FI${CMAKE_SOURCE_DIR}/include/WindowsCleanup.h")
        add_compile_options(/Zi /FS)
        add_link_options(/DEBUG:FULL /INCREMENTAL:NO)

        # Architecture detection and validation for MSVC
        # Fix for "No Target Architecture" error in winnt.h
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            # 64-bit build - ensure _M_X64 or _M_AMD64 is defined
            message(STATUS "MSVC: Detected 64-bit build (x64)")
            # Explicitly set architecture if not already set by generator
            if(NOT CMAKE_GENERATOR_PLATFORM)
                set(CMAKE_GENERATOR_PLATFORM x64 CACHE STRING "Generator platform" FORCE)
            endif()
            # Add fallback definition if architecture macro is missing
            add_compile_definitions(_AMD64_)
        elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
            # 32-bit build
            message(STATUS "MSVC: Detected 32-bit build (x86)")
            if(NOT CMAKE_GENERATOR_PLATFORM)
                set(CMAKE_GENERATOR_PLATFORM Win32 CACHE STRING "Generator platform" FORCE)
            endif()
            add_compile_definitions(_X86_)
        else()
            message(FATAL_ERROR "MSVC: Unable to detect target architecture. CMAKE_SIZEOF_VOID_P=${CMAKE_SIZEOF_VOID_P}")
        endif()
    else()
        # MinGW/GCC on Windows
        add_compile_options(-include "${CMAKE_SOURCE_DIR}/include/WindowsCleanup.h")
        add_compile_options(-g -fno-omit-frame-pointer)
    endif()
else()
    # Linux/Unix
    add_compile_options(-g -fno-omit-frame-pointer)
    add_link_options(-Wl,--build-id=uuid)
endif()

# ============================================================================
# Build Options (Single Definition)
# ============================================================================

option(BUILD_TESTS "Build test executables" OFF)
option(BUILD_DOCS "Build documentation" OFF)
option(USE_VENDOR_LZ4 "Fetch LZ4 if not found system-wide" ON)
option(ENABLE_VERBOSE_DIAGNOSTICS "Enable verbose diagnostics across subsystems" OFF)
option(ENABLE_MESSAGE_BUS_TRACE "Trace message bus operations" OFF)
option(ENABLE_ECS_LIFECYCLE_TRACE "Trace ECS entity lifecycle events" OFF)

if(ENABLE_VERBOSE_DIAGNOSTICS)
    add_compile_definitions(ENABLE_VERBOSE_DIAGNOSTICS)
endif()

if(ENABLE_MESSAGE_BUS_TRACE)
    add_compile_definitions(ENABLE_MESSAGE_BUS_TRACE)
endif()

if(ENABLE_ECS_LIFECYCLE_TRACE)
    add_compile_definitions(ENABLE_ECS_LIFECYCLE_TRACE)
endif()

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
if(WIN32)
    find_package(SDL2 CONFIG REQUIRED)
    set(SDL2_LIBRARIES SDL2::SDL2 SDL2::SDL2main)
else()
    # Linux: Use pkg-config to find SDL2 (optional for test-only builds)
    find_package(PkgConfig REQUIRED)
    if(NOT BUILD_TESTS OR BUILD_MAIN_EXECUTABLE)
        pkg_check_modules(SDL2 REQUIRED sdl2)
    else()
        pkg_check_modules(SDL2 QUIET sdl2)
    endif()
    if(SDL2_FOUND)
        include_directories(${SDL2_INCLUDE_DIRS})
    else()
        message(WARNING "SDL2 not found - main executable will not be built")
        set(SKIP_MAIN_EXECUTABLE TRUE)
    endif()
endif()

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

if(WIN32)
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
else()
    # Linux: Use pkg-config to find jsoncpp
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(JSONCPP QUIET jsoncpp)

    if(NOT JSONCPP_FOUND)
        message(STATUS "jsoncpp not found - fetching via FetchContent")
        include(FetchContent)
        FetchContent_Declare(
            jsoncpp
            GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
            GIT_TAG 1.9.5
            GIT_SHALLOW TRUE
        )
        set(JSONCPP_WITH_TESTS OFF CACHE BOOL "" FORCE)
        set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "" FORCE)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        set(BUILD_OBJECT_LIBS OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(jsoncpp)
        set(JSONCPP_TARGET jsoncpp_static)
        message(STATUS "Using fetched jsoncpp")
    else()
        include_directories(${JSONCPP_INCLUDE_DIRS})
        set(JSONCPP_TARGET ${JSONCPP_LIBRARIES})
        message(STATUS "Using system jsoncpp via pkg-config")
    endif()
endif()

if(NOT SKIP_MAIN_EXECUTABLE)
    find_package(OpenSSL REQUIRED)
    find_package(OpenGL REQUIRED)
endif()

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
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        )
        target_include_directories(imgui PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
        )
        if(WIN32)
            target_link_libraries(imgui PUBLIC SDL2::SDL2 OpenGL::GL)
        else()
            target_link_libraries(imgui PUBLIC ${SDL2_LIBRARIES} OpenGL::GL)
            target_link_directories(imgui PUBLIC ${SDL2_LIBRARY_DIRS})
        endif()
        set(IMGUI_LIBRARIES imgui)
        set(IMGUI_FOUND TRUE)
    endif()
endif()

# LZ4 (optional, auto-fetched if missing)
# vcpkg provides lz4::lz4 target, manual find provides LZ4::LZ4 or lz4_static
find_package(lz4 CONFIG QUIET)
if(NOT lz4_FOUND)
    find_package(LZ4 QUIET)
endif()

if(NOT lz4_FOUND AND NOT LZ4_FOUND AND USE_VENDOR_LZ4)
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
    src/core/save/SaveCompression.cpp
    src/core/save/IncrementalSaveTracker.cpp
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
    src/core/diagnostics/CrashHandler.cpp
)

# ============================================================================
# Source Files - Game Systems
# ============================================================================

set(ADMINISTRATIVE_SOURCES
    src/game/administration/AdministrativeSystem.cpp
    src/game/administration/AdministrativeComponents.cpp
)

set(FACTION_SOURCES
    src/game/faction/FactionSystem.cpp
)

set(MILITARY_SOURCES
    src/game/military/MilitarySystem.cpp
    src/game/military/MilitaryRecruitmentSystem.cpp
    src/game/military/MilitaryComponents.cpp
    src/game/military/MilitaryDatabase_Utils.cpp
    src/game/military/BattleResolutionCalculator.cpp
    src/game/military/NavalCombatCalculator.cpp
    src/game/military/NavalMovementSystem.cpp
    src/game/military/FleetManagementSystem.cpp
    src/game/military/NavalOperationsSystem.cpp
    src/game/military/NavalCombatConfig.cpp
    src/game/military/NavalCombatLogger.cpp
    src/game/military/FleetCompositionCache.cpp
    src/game/military/CommandDelay.cpp
    src/game/military/MilitaryCampaignManager.cpp
    src/game/military/MilitaryOrders.cpp
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
    src/game/province/ProvinceSpatialIndex.cpp
)

set(TIME_SOURCES
    src/game/time/TimeManagementSystem.cpp
    src/game/time/TimeComponents.cpp
)

set(TECHNOLOGY_SOURCES
    src/game/technology/TechnologySystem.cpp
    src/game/technology/TechnologyUtils.cpp
    src/game/technology/TechnologyPrerequisites.cpp
    src/game/technology/TechnologyComponents.cpp
)

set(ECONOMIC_SOURCES
    src/game/economy/EconomicSystem.cpp
    src/game/economy/EconomicComponents.cpp
)

set(ECONOMIC_BRIDGE_SOURCES
    src/game/economy/EconomicPopulationBridge.cpp
    src/game/economy/EconomicPopulationBridgeSerialization.cpp
    src/game/economy/TradeEconomicBridge.cpp
    src/game/economy/TechnologyEconomicBridge.cpp
    src/game/bridge/DiplomacyEconomicBridge.cpp
)

set(DIPLOMACY_SOURCES
    src/game/diplomacy/DiplomacyComponents.cpp
    src/game/diplomacy/DiplomacySystem.cpp
    src/game/diplomacy/DiplomacySystemSerialization.cpp
    src/game/diplomacy/DiplomaticMemory.cpp
    src/game/diplomacy/MemorySystem.cpp
    src/game/diplomacy/TrustSystem.cpp
    src/game/diplomacy/InfluenceComponents.cpp
    src/game/diplomacy/InfluenceCalculator.cpp
    src/game/diplomacy/InfluenceSystem.cpp
    src/game/diplomacy/InfluenceSystemIntegration.cpp
)

set(SCENARIO_SOURCES
    src/game/scenario/ScenarioSystem.cpp
)

set(GAMEPLAY_SOURCES
    src/game/gameplay/CoreGameplaySystem.cpp
)

set(TESTING_SOURCES
    src/game/testing/TestingModule.cpp
)

set(REALM_SOURCES
    src/game/realm/RealmComponents.cpp
    src/game/realm/RealmManager.cpp
    src/game/realm/RealmRepository.cpp
    src/game/realm/RealmCalculator.cpp
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

set(DATA_SOURCES
    src/game/data/DefinitionLoader.cpp
)

set(NEWS_SOURCES
    src/game/news/NewsDelaySystem.cpp
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
    # Trade System UI (Oct 31, 2025)
    src/ui/TradeSystemWindow.cpp
    # Complete UI Windows (Oct 31, 2025)
    src/ui/BattleViewerWindow.cpp
    src/ui/MapModeSelector.cpp
    # UI Navigation System (Nov 17, 2025)
    src/ui/SplashScreen.cpp
    src/ui/NationSelector.cpp
    src/ui/InGameHUD.cpp
    # EU4-style UI System (Nov 18, 2025)
    src/ui/WindowManager.cpp
    src/ui/LeftSidebar.cpp
    src/ui/EconomyWindow.cpp
    src/ui/MilitaryWindow.cpp
    src/ui/DiplomacyWindow.cpp
    src/ui/RealmWindow.cpp
    # Portrait Generator (Nov 18, 2025)
    src/ui/PortraitGenerator.cpp
    # Save/Load and Settings (Nov 26, 2025)
    src/ui/SaveLoadDialog.cpp
    src/ui/SettingsWindow.cpp
)

set(RENDER_SOURCES
    src/rendering/MapRenderer.cpp
    src/rendering/TerrainRenderer.cpp
    src/rendering/TacticalTerrainRenderer.cpp
    src/rendering/BuildingRenderer.cpp
    src/rendering/UnitRenderer.cpp
    src/rendering/EnvironmentalEffectRenderer.cpp
    src/rendering/ViewportCuller.cpp
    src/rendering/FogOfWarRenderer.cpp
    src/game/map/MapDataLoader.cpp
)

set(MAP_SOURCES
    src/map/MapSystem.cpp
    src/map/GeographicUtils.cpp
    src/map/ProvinceGeometry.cpp
    src/map/SpatialIndex.cpp
    src/map/HistoricalMapLoader.cpp
    src/map/GeoJSONView.cpp
    src/map/FogOfWar.cpp
    src/map/LineOfSight.cpp
    src/map/loaders/GeoJSONLoader.cpp
    src/map/loaders/MapDataParser.cpp
    src/map/loaders/ProvinceBuilder.cpp
    src/map/loaders/TerrainProcessor.cpp
    src/map/loaders/ShapefileLoader.cpp
)

# ============================================================================
# Main Executable
# ============================================================================

# Use full game main.cpp with all systems
set(MAIN_SOURCES
    apps/main.cpp
    apps/StressTestRunner.cpp
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
    ${FACTION_SOURCES}
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
    ${TESTING_SOURCES}
    ${REALM_SOURCES}
    ${TRADE_SOURCES}
    ${DATA_SOURCES}
    ${NEWS_SOURCES}
    ${AI_SOURCES}
    ${RENDER_SOURCES}
    ${MAP_SOURCES}
)

# Create main executable (skip if SDL2 not available and tests only)
if(NOT SKIP_MAIN_EXECUTABLE)
add_executable(mechanica_imperii ${ALL_SOURCES})

set(SYMBOL_OUTPUT_DIR ${CMAKE_BINARY_DIR}/symbols)
add_custom_command(TARGET mechanica_imperii POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SYMBOL_OUTPUT_DIR}
)

if(MSVC)
    add_custom_command(TARGET mechanica_imperii POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_PDB_FILE:mechanica_imperii> ${SYMBOL_OUTPUT_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:mechanica_imperii> ${SYMBOL_OUTPUT_DIR}/$<TARGET_FILE_NAME:mechanica_imperii>
        COMMENT "Collecting PDB symbols"
    )
else()
    if(CMAKE_OBJCOPY)
        add_custom_command(TARGET mechanica_imperii POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:mechanica_imperii> ${SYMBOL_OUTPUT_DIR}/$<TARGET_FILE_NAME:mechanica_imperii>.debug
            COMMAND ${CMAKE_OBJCOPY} --strip-debug $<TARGET_FILE:mechanica_imperii>
            COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=${SYMBOL_OUTPUT_DIR}/$<TARGET_FILE_NAME:mechanica_imperii>.debug $<TARGET_FILE:mechanica_imperii>
            COMMENT "Generating split debug symbols"
        )
    else()
        add_custom_command(TARGET mechanica_imperii POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:mechanica_imperii> ${SYMBOL_OUTPUT_DIR}/$<TARGET_FILE_NAME:mechanica_imperii>
            COMMENT "Objcopy not available; copied binary with embedded symbols"
        )
    endif()
endif()

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

if(WIN32)
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
else()
    target_link_libraries(mechanica_imperii PRIVATE
        ${SDL2_LIBRARIES}
        ${GLAD_LIBRARIES}
        ${JSONCPP_TARGET}
        OpenSSL::SSL
        OpenSSL::Crypto
        OpenGL::GL
        ${IMGUI_LIBRARIES}
    )
    target_link_directories(mechanica_imperii PRIVATE ${SDL2_LIBRARY_DIRS})
endif()

if(WIN32)
    target_link_libraries(mechanica_imperii PRIVATE DbgHelp)
endif()

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

if(lz4_FOUND OR LZ4_FOUND)
    if(TARGET lz4::lz4)
        # vcpkg provides lz4::lz4 target
        target_link_libraries(mechanica_imperii PRIVATE lz4::lz4)
        message(STATUS "Linking LZ4: lz4::lz4 (vcpkg)")
    elseif(TARGET LZ4::LZ4)
        # Standard LZ4::LZ4 target
        target_link_libraries(mechanica_imperii PRIVATE LZ4::LZ4)
        message(STATUS "Linking LZ4: LZ4::LZ4")
    else()
        # Fallback to variables
        target_link_libraries(mechanica_imperii PRIVATE ${LZ4_LIBRARIES})
        target_include_directories(mechanica_imperii PRIVATE ${LZ4_INCLUDE_DIRS})
        message(STATUS "Linking LZ4: ${LZ4_LIBRARIES}")
    endif()
endif()

if(WIN32)
    target_link_libraries(mechanica_imperii PRIVATE ws2_32 winmm)
else()
    target_link_libraries(mechanica_imperii PRIVATE pthread dl)
endif()
endif() # NOT SKIP_MAIN_EXECUTABLE

# ============================================================================
# Optional Test Executables
# ============================================================================

if(BUILD_TESTS)
    # Test: Enhanced Config
    add_executable(test_enhanced_config tests/test_enhanced_config.cpp ${CONFIG_SOURCES})

    target_include_directories(test_enhanced_config PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(test_enhanced_config PRIVATE
        ${JSONCPP_TARGET}
        ${IMGUI_LIBRARIES}
    )

    # Add ImGui includes for test_enhanced_config
    if(UNIX AND IMGUI_FOUND AND NOT TARGET imgui::imgui)
        target_include_directories(test_enhanced_config PRIVATE ${IMGUI_INCLUDE_DIRS})
    endif()

    set_target_properties(test_enhanced_config PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
    )

    # Test: Scenario Demo
    add_executable(test_scenario_demo
        tests/test_scenario_demo.cpp 
        ${CONFIG_SOURCES} 
        ${ECS_SOURCES} 
        ${TIME_SOURCES} 
        ${SCENARIO_SOURCES}
    )
    
    target_include_directories(test_scenario_demo PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    
    if(WIN32)
        target_link_libraries(test_scenario_demo PRIVATE
            SDL2::SDL2
            SDL2::SDL2main
            ${GLAD_LIBRARIES}
            ${JSONCPP_TARGET}
            OpenGL::GL
            ${IMGUI_LIBRARIES}
        )
    else()
        target_link_libraries(test_scenario_demo PRIVATE
            ${SDL2_LIBRARIES}
            ${GLAD_LIBRARIES}
            ${JSONCPP_TARGET}
            OpenGL::GL
            ${IMGUI_LIBRARIES}
        )
        target_link_directories(test_scenario_demo PRIVATE ${SDL2_LIBRARY_DIRS})
    endif()
    
    set_target_properties(test_scenario_demo PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
    )
    
    # Platform-specific test configuration
    if(WIN32)
        if(MSVC)
            target_compile_options(test_scenario_demo PRIVATE
                "/FI${CMAKE_SOURCE_DIR}/include/WindowsCleanup.h"
            )
        else()
            target_compile_options(test_scenario_demo PRIVATE
                -include "${CMAKE_SOURCE_DIR}/include/WindowsCleanup.h"
            )
        endif()
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

    # Add AI Director integration tests
    add_subdirectory(tests)
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

# Copy definition files to build directory
file(COPY ${CMAKE_SOURCE_DIR}/data/definitions/
     DESTINATION ${CMAKE_BINARY_DIR}/bin/data/definitions
     FILES_MATCHING PATTERN "*.json")

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
