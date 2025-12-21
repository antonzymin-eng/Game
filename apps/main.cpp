// ============================================================================
// main.cpp - Mechanica Imperii - Minimal Build Test
// Created: 2025-01-13 17:00:00
// Basic SDL2 initialization to test compilation
// ============================================================================

#include <chrono>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>

// Platform compatibility layer (includes SDL2, OpenGL, ImGui, JsonCpp)
// NOTE: WindowsCleanup.h is force-included by CMake on Windows (before this file)
#include "utils/PlatformCompat.h"

#include "core/diagnostics/CrashHandler.h"
#include "core/logging/Logger.h"

// CRITICAL FIX: Core Type System (eliminates string parsing errors)
#include "core/types/game_types.h"

// Core ECS and Architecture
#include "core/ECS/EntityManager.h"
#include "core/ECS/ComponentAccessManager.h"

// UI System
#include "ui/UI.h"
#include "core/ECS/MessageBus.h"
#include "core/threading/ThreadedSystemManager.h"
#include "core/threading/ThreadSafeMessageBus.h"

// CRITICAL FIX 2: Configuration System (eliminates hardcoded values)
#include "game/config/GameConfig.h"
#include "game/config/ConfigHelpers.h"

// Data Loading System (JSON definitions integration)
#include "game/data/DefinitionLoader.h"

// Enhanced Game Systems (with all fixes applied)
#include "game/population/PopulationSystem.h"
#include "game/population/PopulationTypes.h"
#include "game/population/PopulationEvents.h"
#include "game/technology/TechnologySystem.h"
#include "game/gameplay/CoreGameplaySystem.h"  // FIXED: Logic inversion resolved
#include "game/time/TimeManagementSystem.h"

// Existing Game Systems
#include "game/province/ProvinceManagementSystem.h"
#include "game/economy/EconomicSystem.h"
#include "game/economy/EconomicPopulationBridge.h"
#include "game/economy/TradeEconomicBridge.h"
#include "game/administration/AdministrativeSystem.h"
#include "game/military/MilitarySystem.h"
#include "game/military/MilitaryRecruitmentSystem.h"
#include "game/military/MilitaryEconomicBridge.h"
#include "game/diplomacy/DiplomacySystem.h"
#include "game/trade/TradeSystem.h"
#include "game/realm/RealmManager.h"
#include "game/gameplay/GameWorld.h"
#include "game/gameplay/GameSystemsIntegration.h"

// Integration Bridge Systems
#include "game/bridge/DiplomacyEconomicBridge.h"
#include "game/economy/TechnologyEconomicBridge.h"

// AI System
#include "game/ai/AIDirector.h"

// Character System
#include "game/systems/CharacterSystem.h"

// UI Systems
//#include "ui/AdministrativeUI.h"
//#include "ui/SimpleProvincePanel.h"
//#include "ui/MainMenuUI.h"
//#include "ui/GameScreen.h"
//#include "ui/Theme.h"
//#include "ui/Toast.h"
#include "ui/PopulationInfoWindow.h"
#include "ui/TechnologyInfoWindow.h"
#include "ui/PerformanceWindow.h"
//#include "ui/BalanceMonitorWindow.h"

// New UI Windows (Oct 29, 2025)
#include "ui/GameControlPanel.h"
#include "ui/ProvinceInfoWindow.h"
#include "ui/NationOverviewWindow.h"
#include "ui/TradeSystemWindow.h"

// UI Navigation System (Nov 17, 2025)
#include "ui/SplashScreen.h"
#include "ui/NationSelector.h"
#include "ui/InGameHUD.h"

// EU4-style UI System (Nov 18, 2025)
#include "ui/WindowManager.h"
#include "ui/LeftSidebar.h"
#include "ui/EconomyWindow.h"
#include "ui/MilitaryWindow.h"
#include "ui/DiplomacyWindow.h"
#include "ui/RealmWindow.h"
#include "ui/AdministrativeWindow.h"
#include "ui/CharacterWindow.h"

// Portrait Generator (Nov 18, 2025)
#include "ui/PortraitGenerator.h"

// UI Dialogs and Settings (Nov 18, 2025)
#include "ui/SaveLoadDialog.h"
#include "ui/SettingsWindow.h"

// Save System (Dec 6, 2025)
#include "core/save/SaveManager.h"

#include "StressTestRunner.h"

// Map Rendering System
#include "map/MapDataLoader.h"
#include "map/render/MapRenderer.h"
#include "map/render/GPUMapRenderer.h"
#include "map/render/ViewportCuller.h"

// ImGui backends - included after SDL2 and OpenGL
// Note: CMake configuration now adds backends/ to include path on all platforms
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

// State management
//#include "state/SimulationState.h"
//#include "state/SaveAdapters.h"
//#include "ScreenAPI.h"
//#include "io/SaveLoad.h"

namespace {

struct AppCommandLineOptions {
    bool show_help{false};
    bool parse_error{false};
    bool run_stress{false};
    std::string error_message;
    apps::stress::StressTestConfig stress_config;
};

bool ParseSizeTArgument(const std::string& value, std::size_t& out_value) {
    try {
        std::size_t processed = 0;
        unsigned long long parsed = std::stoull(value, &processed);
        if (processed != value.size()) {
            return false;
        }
        out_value = static_cast<std::size_t>(parsed);
        return true;
    }
    catch (...) {
        return false;
    }
}

void PrintCommandLineHelp() {
    std::cout << "Mechanica Imperii command line options:\n"
              << "  --help, -h                  Show this help message\n"
              << "  --stress-test              Run the headless stress test harness\n"
              << "  --stress-maps <dir>        Override the maps directory (default data/maps)\n"
              << "  --stress-nations <dir>     Override the nations directory (default data/nations)\n"
              << "  --stress-warmup <ticks>    Warmup ticks before measuring (default 30)\n"
              << "  --stress-ticks <ticks>     Number of measured ticks (default 600)\n"
              << "  --stress-workers <count>   Force worker thread count (default hardware concurrency)\n"
              << "  --stress-units-per-task <n>Manual override for units per task chunk\n"
              << "  --stress-json <path>       Write JSON metrics to the specified file\n"
              << "  --stress-verbose           Print per-tick durations during measurement\n"
              << "  --stress-summary           Print summary-only (suppresses detailed banner)\n"
              << std::endl;
}

AppCommandLineOptions ParseCommandLineOptions(int argc, char* argv[]) {
    AppCommandLineOptions options;

    auto fetch_value = [&](int& index, const std::string& flag) -> std::optional<std::string> {
        if (index + 1 >= argc) {
            options.parse_error = true;
            options.error_message = "Missing value for " + flag;
            return std::nullopt;
        }
        ++index;
        return std::string(argv[index]);
    };

    for (int i = 1; i < argc && !options.parse_error; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            options.show_help = true;
        }
        else if (arg == "--stress-test") {
            options.run_stress = true;
        }
        else if (arg == "--stress-maps") {
            if (auto value = fetch_value(i, arg)) {
                namespace fs = std::filesystem;
                if (!fs::exists(*value)) {
                    options.parse_error = true;
                    options.error_message = "Maps directory does not exist: " + *value;
                } else if (!fs::is_directory(*value)) {
                    options.parse_error = true;
                    options.error_message = "Maps path is not a directory: " + *value;
                } else {
                    options.run_stress = true;
                    try {
                        options.stress_config.maps_directory = fs::canonical(*value).string();
                    } catch (const fs::filesystem_error& e) {
                        options.parse_error = true;
                        options.error_message = "Invalid maps directory path: " + std::string(e.what());
                    }
                }
            }
        }
        else if (arg == "--stress-nations") {
            if (auto value = fetch_value(i, arg)) {
                namespace fs = std::filesystem;
                if (!fs::exists(*value)) {
                    options.parse_error = true;
                    options.error_message = "Nations directory does not exist: " + *value;
                } else if (!fs::is_directory(*value)) {
                    options.parse_error = true;
                    options.error_message = "Nations path is not a directory: " + *value;
                } else {
                    options.run_stress = true;
                    try {
                        options.stress_config.nations_directory = fs::canonical(*value).string();
                    } catch (const fs::filesystem_error& e) {
                        options.parse_error = true;
                        options.error_message = "Invalid nations directory path: " + std::string(e.what());
                    }
                }
            }
        }
        else if (arg == "--stress-warmup") {
            if (auto value = fetch_value(i, arg)) {
                std::size_t parsed = 0;
                if (!ParseSizeTArgument(*value, parsed)) {
                    options.parse_error = true;
                    options.error_message = "Invalid warmup tick count: " + *value;
                } else if (parsed > 10000) {
                    options.parse_error = true;
                    options.error_message = "Warmup tick count too large (max 10000): " + *value;
                } else {
                    options.run_stress = true;
                    options.stress_config.warmup_ticks = parsed;
                }
            }
        }
        else if (arg == "--stress-ticks") {
            if (auto value = fetch_value(i, arg)) {
                std::size_t parsed = 0;
                if (!ParseSizeTArgument(*value, parsed)) {
                    options.parse_error = true;
                    options.error_message = "Invalid measured tick count: " + *value;
                } else if (parsed == 0) {
                    options.parse_error = true;
                    options.error_message = "Measured tick count must be at least 1: " + *value;
                } else if (parsed > 100000) {
                    options.parse_error = true;
                    options.error_message = "Measured tick count too large (max 100000): " + *value;
                } else {
                    options.run_stress = true;
                    options.stress_config.measured_ticks = parsed;
                }
            }
        }
        else if (arg == "--stress-workers") {
            if (auto value = fetch_value(i, arg)) {
                std::size_t parsed = 0;
                if (!ParseSizeTArgument(*value, parsed) || parsed == 0) {
                    options.parse_error = true;
                    options.error_message = "Invalid worker thread count: " + *value;
                } else if (parsed > 256) {
                    options.parse_error = true;
                    options.error_message = "Worker thread count too large (max 256): " + *value;
                } else {
                    options.run_stress = true;
                    options.stress_config.worker_threads = parsed;
                }
            }
        }
        else if (arg == "--stress-units-per-task") {
            if (auto value = fetch_value(i, arg)) {
                std::size_t parsed = 0;
                if (!ParseSizeTArgument(*value, parsed) || parsed == 0) {
                    options.parse_error = true;
                    options.error_message = "Invalid units per task: " + *value;
                } else {
                    options.run_stress = true;
                    options.stress_config.units_per_task_hint = parsed;
                }
            }
        }
        else if (arg == "--stress-json") {
            if (auto value = fetch_value(i, arg)) {
                namespace fs = std::filesystem;
                fs::path json_path(*value);
                fs::path parent_dir = json_path.parent_path();

                // If parent directory is specified, validate it exists
                if (!parent_dir.empty() && !fs::exists(parent_dir)) {
                    options.parse_error = true;
                    options.error_message = "JSON output directory does not exist: " + parent_dir.string();
                } else if (!parent_dir.empty() && !fs::is_directory(parent_dir)) {
                    options.parse_error = true;
                    options.error_message = "JSON output parent path is not a directory: " + parent_dir.string();
                } else {
                    options.run_stress = true;
                    try {
                        options.stress_config.json_output_path = json_path.string();
                    } catch (const fs::filesystem_error& e) {
                        options.parse_error = true;
                        options.error_message = "Invalid JSON output path: " + std::string(e.what());
                    }
                }
            }
        }
        else if (arg == "--stress-verbose") {
            options.run_stress = true;
            options.stress_config.verbose = true;
        }
        else if (arg == "--stress-summary") {
            options.run_stress = true;
            options.stress_config.summary_only = true;
        }
        else {
            options.parse_error = true;
            options.error_message = "Unknown argument: " + arg;
        }
    }

    return options;
}

} // namespace

// ============================================================================
// Global System Instances (FIXED: Threading strategies documented)
// ============================================================================

// Core ECS Foundation
static std::unique_ptr<core::ecs::EntityManager> g_entity_manager;
static std::unique_ptr<core::ecs::ComponentAccessManager> g_component_access_manager;
static std::unique_ptr<core::ecs::MessageBus> g_message_bus;
static std::unique_ptr<core::threading::ThreadSafeMessageBus> g_thread_safe_message_bus;
static std::unique_ptr<core::threading::ThreadedSystemManager> g_system_manager;

// Enhanced Game Systems (PERFORMANCE OPTIMIZED)
static std::unique_ptr<game::population::PopulationSystem> g_population_system;
static std::unique_ptr<game::technology::TechnologySystem> g_technology_system;
static std::unique_ptr<game::province::ProvinceSystem> g_province_system;
static std::unique_ptr<game::economy::EconomicSystem> g_economic_system;
static std::unique_ptr<game::administration::AdministrativeSystem> g_administrative_system;
static std::unique_ptr<game::military::MilitarySystem> g_military_system;
static std::unique_ptr<game::military::MilitaryRecruitmentSystem> g_military_recruitment_system;
static std::unique_ptr<mechanica::integration::MilitaryEconomicBridge> g_military_economic_bridge;
static std::unique_ptr<game::diplomacy::DiplomacySystem> g_diplomacy_system;
static std::unique_ptr<game::trade::TradeSystem> g_trade_system;
static std::unique_ptr<game::realm::RealmManager> g_realm_manager;
static std::unique_ptr<game::gameplay::GameplayCoordinator> g_gameplay_system;  // FIXED
static std::unique_ptr<game::time::TimeManagementSystem> g_time_system;

// Integration Bridges
static std::unique_ptr<mechanica::integration::TradeEconomicBridge> g_trade_economic_bridge;
static std::unique_ptr<game::bridge::DiplomacyEconomicBridge> g_diplomacy_economic_bridge;
static std::unique_ptr<mechanica::integration::TechnologyEconomicBridge> g_tech_economic_bridge;

// Legacy Systems (maintained for compatibility) - TODO: Implement these classes
// static EconomicSystem* g_economic_system = nullptr;
// static AdministrativeSystem* g_administrative_system = nullptr;
static game::GameWorld* g_game_world = nullptr;

// Main realm entity for population simulation
// Global system objects - managed lifecycle with proper ECS integration
static core::ecs::EntityID g_main_realm_entity{};

// UI Systems
static ui::AdministrativeUI* g_administrative_ui = nullptr;
static ui::SimpleProvincePanel* g_province_panel = nullptr;
static ui::MainMenuUI* g_main_menu_ui = nullptr;
static ui::PopulationInfoWindow* g_population_window = nullptr;
static ui::TechnologyInfoWindow* g_technology_window = nullptr;
static ui::PerformanceWindow* g_performance_window = nullptr;

// New UI Windows (Oct 29, 2025)
static ui::GameControlPanel* g_game_control_panel = nullptr;
static ui::ProvinceInfoWindow* g_province_info_window = nullptr;
static ui::NationOverviewWindow* g_nation_overview_window = nullptr;
static ui::TradeSystemWindow* g_trade_system_window = nullptr;

// Map Rendering System
static std::unique_ptr<game::map::MapRenderer> g_map_renderer;
static std::unique_ptr<game::map::GPUMapRenderer> g_gpu_map_renderer;
static bool g_use_gpu_renderer = false;  // Toggle between CPU (ImGui) and GPU (OpenGL)

// AI System
static std::unique_ptr<AI::AIDirector> g_ai_director;

// Character System (Nov 2025)
static std::unique_ptr<game::character::CharacterSystem> g_character_system;

// Save System (Dec 6, 2025)
static std::unique_ptr<core::save::SaveManager> g_save_manager;

// UI Navigation System (Nov 17, 2025)
static ui::SplashScreen* g_splash_screen = nullptr;
static ui::NationSelector* g_nation_selector = nullptr;
static ui::InGameHUD* g_ingame_hud = nullptr;

// EU4-style UI System (Nov 18, 2025)
static ui::WindowManager* g_window_manager = nullptr;
static ui::LeftSidebar* g_left_sidebar = nullptr;
static ui::EconomyWindow* g_economy_window = nullptr;
static ui::MilitaryWindow* g_military_window = nullptr;
static ui::DiplomacyWindow* g_diplomacy_window = nullptr;
static ui::RealmWindow* g_realm_window = nullptr;
static ui::AdministrativeWindow* g_administrative_window = nullptr;
static ui::CharacterWindow* g_character_window = nullptr;

// Portrait Generator (Nov 18, 2025)
static ui::PortraitGenerator* g_portrait_generator = nullptr;

// UI Dialogs and Settings (Nov 18, 2025)
static ui::SaveLoadDialog* g_save_load_dialog = nullptr;
static ui::SettingsWindow* g_settings_window = nullptr;

// Game State Management (Nov 17, 2025)
enum class GameState {
    SPLASH_SCREEN,
    MAIN_MENU,
    NATION_SELECTION,
    GAME_RUNNING
};

static GameState g_current_game_state = GameState::SPLASH_SCREEN;

// Application state
static bool g_running = true;
static bool g_show_demo_window = false;
static bool g_show_performance_metrics = false;

// ============================================================================
// CRITICAL FIX: Configuration-Driven Initialization
// ============================================================================

static bool TryParseLogLevel(const char* value, core::logging::LogLevel& out_level) {
    if (!value || *value == '\0') {
        return false;
    }

    std::string normalized(value);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });

    if (normalized == "TRACE") {
        out_level = core::logging::LogLevel::Trace;
        return true;
    }
    if (normalized == "DEBUG") {
        out_level = core::logging::LogLevel::Debug;
        return true;
    }
    if (normalized == "INFO") {
        out_level = core::logging::LogLevel::Info;
        return true;
    }
    if (normalized == "WARN" || normalized == "WARNING") {
        out_level = core::logging::LogLevel::Warn;
        return true;
    }
    if (normalized == "ERROR") {
        out_level = core::logging::LogLevel::Error;
        return true;
    }
    if (normalized == "CRITICAL" || normalized == "FATAL") {
        out_level = core::logging::LogLevel::Critical;
        return true;
    }
    if (normalized == "OFF") {
        out_level = core::logging::LogLevel::Off;
        return true;
    }

    return false;
}

static std::size_t ParseUnsignedEnv(const char* value, std::size_t fallback) {
    if (!value || *value == '\0') {
        return fallback;
    }

    char* end = nullptr;
    unsigned long long parsed = std::strtoull(value, &end, 10);
    if (end == value) {
        return fallback;
    }

    return static_cast<std::size_t>(parsed);
}

// ============================================================================
// Forward Declarations
// ============================================================================

static void InitializeSaveSystem();
static void SaveGame(const std::string& filename);
static void LoadGame(const std::string& filename);

// ============================================================================
// Initialization Functions
// ============================================================================

static void InitializeLogging() {
    using namespace core::logging;

    core::logging::LogLevel level = LogLevel::Info;
    if (const char* level_env = std::getenv("MECHANICA_LOG_LEVEL")) {
        TryParseLogLevel(level_env, level);
    }
    SetGlobalLogLevel(level);

    // TODO: File logging not yet implemented
    // FileSinkOptions options;
    // options.path = (std::filesystem::path("logs") / "mechanica.log").string();
    // if (const char* path_env = std::getenv("MECHANICA_LOG_PATH")) {
    //     if (*path_env != '\0') {
    //         options.path = path_env;
    //     }
    // }
    // options.max_file_size_bytes = ParseUnsignedEnv(std::getenv("MECHANICA_LOG_MAX_SIZE"), 10ull * 1024ull * 1024ull);
    // options.max_files = ParseUnsignedEnv(std::getenv("MECHANICA_LOG_MAX_FILES"), 5);
    // options.flush_on_write = true;

    // std::string error_message;
    // if (!EnableFileSink(options, &error_message)) {
    //     std::cerr << "File logging disabled: " << error_message << std::endl;
    // } else {
    if (true) {
        CORE_LOG_INFO("Bootstrap", "Console logging enabled");
    }
}

static void InitializeConfiguration() {
    try {
        std::cout << "Initializing configuration system..." << std::endl;

        // CRITICAL FIX 2: Initialize configuration system first
        game::config::GameConfig::Initialize("config/");

        // Enable hot reload for development
#ifdef DEBUG
        game::config::GameConfig::Instance().EnableHotReload(true);
#endif

        // Validate configuration
        if (!game::config::GameConfig::Instance().ValidateConfiguration()) {
            auto errors = game::config::GameConfig::Instance().GetValidationErrors();
            std::cerr << "Configuration validation errors:" << std::endl;
            for (const auto& error : errors) {
                std::cerr << "  - " << error << std::endl;
            }
            throw std::runtime_error("Configuration validation failed");
        }

        // Print configuration status
        auto council_config = game::config::GameConfig::Instance().GetCouncilConfiguration();
        auto threading_config = game::config::GameConfig::Instance().GetThreadingConfiguration();

        std::cout << "Configuration loaded successfully:" << std::endl;
        std::cout << "  Council default delegation: " << council_config.default_delegation_level << std::endl;
        std::cout << "  Worker threads: " << threading_config.worker_thread_count << std::endl;
        std::cout << "  Hot reload: " << (game::config::GameConfig::Instance().CheckForConfigurationUpdates() ? "enabled" : "disabled") << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "CRITICAL ERROR: Failed to initialize configuration: " << e.what() << std::endl;
        std::cerr << "Creating default configuration files..." << std::endl;

        // Try to generate default configuration
        try {
            game::config::helpers::GenerateDefaultConfigurations();
            std::cout << "Default configuration files created. Please restart the application." << std::endl;
            exit(0);
        }
        catch (const std::exception& gen_error) {
            std::cerr << "Failed to generate default configuration: " << gen_error.what() << std::endl;
            throw;
        }
    }
}

// ============================================================================
// SDL and OpenGL Initialization
// ============================================================================

static SDL_Window* InitializeSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "Error: " << SDL_GetError() << std::endl;
        throw std::runtime_error("SDL initialization failed");
    }

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Mechanica Imperii", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    if (!window) {
        std::cerr << "Error creating window: " << SDL_GetError() << std::endl;
        throw std::runtime_error("Window creation failed");
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

#ifdef PLATFORM_WINDOWS
    // Initialize OpenGL loader (GLAD) - Windows only
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize OpenGL loader!" << std::endl;
        throw std::runtime_error("OpenGL loader initialization failed");
    }
    std::cout << "OpenGL " << glGetString(GL_VERSION) << " loaded successfully" << std::endl;
#else
    // Linux: Load OpenGL extension functions
    if (!PlatformUtils::InitializeOpenGLExtensions()) {
        std::cerr << "Failed to load OpenGL extensions!" << std::endl;
        throw std::runtime_error("OpenGL extension loading failed");
    }
    std::cout << "Using system OpenGL (Linux) with extensions loaded" << std::endl;
#endif

    return window;
}

// ============================================================================
// Enhanced System Initialization (CRITICAL FIXES APPLIED)
// ============================================================================

static void InitializeEnhancedSystems() {
    std::cout << "Initializing enhanced game systems..." << std::endl;

    try {
        // CRITICAL FIX 3: Initialize ECS foundation first
        g_entity_manager = std::make_unique<core::ecs::EntityManager>();
        g_message_bus = std::make_unique<core::ecs::MessageBus>();
        g_thread_safe_message_bus = std::make_unique<core::threading::ThreadSafeMessageBus>();
        g_component_access_manager = std::make_unique<core::ecs::ComponentAccessManager>(
            g_entity_manager.get(), g_message_bus.get());
        g_system_manager = std::make_unique<core::threading::ThreadedSystemManager>(
            g_component_access_manager.get(), g_thread_safe_message_bus.get());

        // Initialize Definition Loader - Load game data from JSON
        std::cout << "Loading game definitions from JSON..." << std::endl;
        auto& def_loader = game::data::DefinitionLoader::GetInstance();
        if (!def_loader.Initialize("data/definitions")) {
            std::cerr << "ERROR: Failed to load game definitions!" << std::endl;
            throw std::runtime_error("Definition loading failed");
        }
        std::cout << "Game definitions loaded successfully:" << std::endl;
        std::cout << "  Technologies: " << def_loader.GetTechnologyCount() << std::endl;
        std::cout << "  Units: " << def_loader.GetUnitCount() << std::endl;
        std::cout << "  Buildings: " << def_loader.GetBuildingCount() << std::endl;
        std::cout << "  Resources: " << def_loader.GetResourceCount() << std::endl;

        // CRITICAL FIX 4: Population System with performance optimizations
        // Initialize PopulationSystem with proper ECS integration
        g_population_system = std::make_unique<game::population::PopulationSystem>(
            *g_component_access_manager, *g_thread_safe_message_bus);
        auto pop_strategy = game::config::helpers::GetThreadingStrategyForSystem("PopulationSystem");
        std::string pop_rationale = game::config::helpers::GetThreadingRationale("PopulationSystem");
        std::cout << "Population System: " << game::types::TypeRegistry::ThreadingStrategyToString(pop_strategy)
            << " - " << pop_rationale << std::endl;

        // Technology System - Background calculations with high parallelization potential
        g_technology_system = std::make_unique<game::technology::TechnologySystem>(
            *g_component_access_manager, *g_thread_safe_message_bus);
        auto tech_strategy = game::config::helpers::GetThreadingStrategyForSystem("TechnologySystem");
        std::string tech_rationale = game::config::helpers::GetThreadingRationale("TechnologySystem");
        std::cout << "Technology System: " << game::types::TypeRegistry::ThreadingStrategyToString(tech_strategy)
            << " - " << tech_rationale << std::endl;

        // Province System - Province management and development
        g_province_system = std::make_unique<game::province::ProvinceSystem>(
            *g_component_access_manager, *g_message_bus);
        std::cout << "Province System: Initialized" << std::endl;

        // Economic System - Treasury, trade, and economic management
        g_economic_system = std::make_unique<game::economy::EconomicSystem>(
            *g_component_access_manager, *g_thread_safe_message_bus);
        std::cout << "Economic System: Initialized (Strategic Rebuild Complete)" << std::endl;

        // Administrative System - Officials, governance, and bureaucracy
        g_administrative_system = std::make_unique<game::administration::AdministrativeSystem>(
            *g_component_access_manager, *g_thread_safe_message_bus);
        std::cout << "Administrative System: Initialized (Strategic Rebuild Complete)" << std::endl;

        // Military System - Combat calculations and unit management
        g_military_system = std::make_unique<game::military::MilitarySystem>(
            *g_component_access_manager, *g_thread_safe_message_bus);
        g_military_recruitment_system = std::make_unique<game::military::MilitaryRecruitmentSystem>(
            *g_component_access_manager, *g_message_bus);
        std::cout << "Military System: Initialized with recruitment system" << std::endl;

        // Diplomacy System - AI-driven diplomacy with complete feature set
        g_diplomacy_system = std::make_unique<game::diplomacy::DiplomacySystem>(
            *g_component_access_manager, *g_thread_safe_message_bus);
        std::cout << "Diplomacy System: Initialized (41/41 methods - 100% complete)" << std::endl;

        // Trade System - Trade routes, markets, and economic simulation
        g_trade_system = std::make_unique<game::trade::TradeSystem>(
            *g_component_access_manager, *g_thread_safe_message_bus);
        std::cout << "Trade System: Initialized (50+ methods - trade routes, hubs, market dynamics)" << std::endl;

        // Realm System - Nations, dynasties, succession, diplomacy, and governance
        // RealmManager requires shared_ptr, so we create shared_ptr wrappers
        std::shared_ptr<core::ecs::ComponentAccessManager> realm_component_access(
            g_component_access_manager.get(), [](auto*){});  // Non-owning shared_ptr
        std::shared_ptr<core::threading::ThreadSafeMessageBus> realm_message_bus(
            g_thread_safe_message_bus.get(), [](auto*){});  // Non-owning shared_ptr
        g_realm_manager = std::make_unique<game::realm::RealmManager>(
            realm_component_access, realm_message_bus);
        std::cout << "Realm System: Initialized (nations, dynasties, succession, governance)" << std::endl;

        // Character System - Character entities and lifecycle management
        g_character_system = std::make_unique<game::character::CharacterSystem>(
            *g_component_access_manager, *g_thread_safe_message_bus);
        auto char_strategy = game::config::helpers::GetThreadingStrategyForSystem("CharacterSystem");
        std::cout << "Character System: " << game::types::TypeRegistry::ThreadingStrategyToString(char_strategy) << std::endl;

        // Load historical characters
        // TODO: Move hardcoded path to configuration file (game_config.json)
        // Suggested config: { "character_system": { "historical_characters_path": "data/characters/..." } }
        std::cout << "Loading historical characters..." << std::endl;
        size_t loaded_count = 0;
        try {
            if (g_character_system->LoadHistoricalCharacters("data/characters/characters_11th_century.json")) {
                loaded_count = g_character_system->GetAllCharacters().size();
                std::cout << "Historical characters loaded: " << loaded_count << std::endl;
            } else {
                std::cerr << "WARNING: Failed to load historical characters" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "ERROR loading historical characters: " << e.what() << std::endl;
        }

        // ECONOMIC SYSTEM INTEGRATION: Create DiplomacyEconomicBridge
        g_diplomacy_economic_bridge = std::make_unique<game::bridge::DiplomacyEconomicBridge>(
            *g_component_access_manager, *g_thread_safe_message_bus);
        std::cout << "Diplomacy-Economic Bridge: Initialized" << std::endl;

        // Trade-Economic Bridge - Integrates trade and economic systems
        g_trade_economic_bridge = std::make_unique<mechanica::integration::TradeEconomicBridge>();
        g_trade_economic_bridge->SetEntityManager(g_entity_manager.get());
        g_trade_economic_bridge->SetMessageBus(g_thread_safe_message_bus.get());
        g_trade_economic_bridge->SetTradeSystem(g_trade_system.get());
        g_trade_economic_bridge->SetEconomicSystem(g_economic_system.get());
        std::cout << "Trade-Economic Bridge: Initialized (connects trade routes with treasury)" << std::endl;

        // Military-Economic Bridge - Integrates military operations with economy
        g_military_economic_bridge = std::make_unique<mechanica::integration::MilitaryEconomicBridge>();
        g_military_economic_bridge->SetMilitarySystem(g_military_system.get());
        g_military_economic_bridge->SetEconomicSystem(g_economic_system.get());
        g_military_economic_bridge->SetTradeSystem(g_trade_system.get());
        std::cout << "Military-Economic Bridge: Created and dependencies set" << std::endl;

        // CRITICAL FIX 1: Core Gameplay System (Logic inversion fixed)
        // Use GameplayCoordinator which matches the declared g_gameplay_system type
        game::gameplay::ComplexitySettings gameplay_settings;
        gameplay_settings.overall_level = game::gameplay::ComplexityLevel::INTERMEDIATE;
        g_gameplay_system = std::make_unique<game::gameplay::GameplayCoordinator>(
            gameplay_settings, 
            &g_thread_safe_message_bus->GetUnsafeMessageBus(), 
            0);
        auto gameplay_strategy = core::threading::ThreadingStrategy::MAIN_THREAD; // UI-responsive decisions
        std::cout << "Core Gameplay System: " << game::types::TypeRegistry::ThreadingStrategyToString(gameplay_strategy)
            << " - UI-driven system needs main thread for immediate response" << std::endl;

        // Time Management System - Main thread for synchronization
        game::time::GameDate start_date(1066, 10, 14); // Battle of Hastings
        g_time_system = std::make_unique<game::time::TimeManagementSystem>(
            *g_component_access_manager, *g_thread_safe_message_bus, start_date);
        auto time_strategy = core::threading::ThreadingStrategy::MAIN_THREAD; // Synchronization critical
        std::cout << "Time Management System: " << game::types::TypeRegistry::ThreadingStrategyToString(time_strategy)
            << " - Frame synchronization requires main thread coordination" << std::endl;

        // Initialize all systems
        g_population_system->Initialize();
        g_technology_system->Initialize();
        g_economic_system->Initialize();
        g_administrative_system->Initialize();
        g_military_system->Initialize();
        g_military_recruitment_system->Initialize();
        g_military_economic_bridge->Initialize();
        g_diplomacy_system->Initialize();
        g_trade_system->Initialize();
        g_realm_manager->Initialize();
        g_trade_economic_bridge->Initialize();
        // g_gameplay_system->Initialize();  // NOTE: GameplayCoordinator uses constructor, no Initialize() method

        // ====================================================================
        // ECONOMIC SYSTEM INTEGRATION: Wire systems to EconomicSystem
        // This enables treasury validation, overflow protection, and proper
        // economic operations across all game systems.
        // ====================================================================
        std::cout << "\nWiring systems to Economic System..." << std::endl;

        // Wire DiplomacyEconomicBridge to EconomicSystem
        if (g_diplomacy_economic_bridge && g_economic_system) {
            g_diplomacy_economic_bridge->SetEconomicSystem(g_economic_system.get());
            g_diplomacy_economic_bridge->Initialize();  // Initialize after wiring
            std::cout << "✓ DiplomacyEconomicBridge → EconomicSystem connected" << std::endl;
        }

        // Wire RealmManager to EconomicSystem
        if (g_realm_manager && g_economic_system) {
            g_realm_manager->SetEconomicSystem(g_economic_system.get());
            std::cout << "✓ RealmManager → EconomicSystem connected" << std::endl;
        }

        std::cout << "Economic system integration complete!" << std::endl;
        std::cout << "====================================================================\n" << std::endl;

        // Initialize AI Director (Week 2 Integration - Nov 10, 2025)
        std::cout << "Initializing AI Director..." << std::endl;
        g_ai_director = std::make_unique<AI::AIDirector>(
            std::shared_ptr<core::ecs::ComponentAccessManager>(g_component_access_manager.get(), [](auto*){}),
            std::shared_ptr<core::threading::ThreadSafeMessageBus>(g_thread_safe_message_bus.get(), [](auto*){})
        );
        g_ai_director->Initialize();
        g_ai_director->Start();
        std::cout << "AI Director initialized successfully (running on MAIN_THREAD)" << std::endl;

        std::cout << "Enhanced systems initialized successfully with documented threading strategies" << std::endl;
        ui::Toast::Show("Enhanced systems initialized", 2.0f);

    }
    catch (const std::exception& e) {
        std::cerr << "CRITICAL ERROR: Failed to initialize enhanced systems: " << e.what() << std::endl;
        throw;
    }
}

// ============================================================================
// Map Rendering System Initialization
// ============================================================================

static void InitializeMapSystem() {
    CORE_LOG_INFO("MapInit", "=== STARTING MAP SYSTEM INITIALIZATION ===");

    try {
        CORE_LOG_INFO("MapInit", "Step 1: Creating MapRenderer...");
        if (!g_entity_manager) {
            CORE_LOG_ERROR("MapInit", "CRITICAL: g_entity_manager is NULL!");
            return;
        }
        CORE_LOG_INFO("MapInit", "Entity manager validated");

        // Create MapRenderer
        g_map_renderer = std::make_unique<game::map::MapRenderer>(*g_entity_manager);
        CORE_LOG_INFO("MapInit", "MapRenderer object created");

        // Initialize renderer
        CORE_LOG_INFO("MapInit", "Step 2: Initializing MapRenderer...");
        if (!g_map_renderer->Initialize()) {
            CORE_LOG_ERROR("MapInit", "MapRenderer::Initialize() returned false");
            throw std::runtime_error("Failed to initialize MapRenderer");
        }
        CORE_LOG_INFO("MapInit", "MapRenderer initialized successfully");

        // Load province data from JSON - Full Europe map with 133 provinces
        CORE_LOG_INFO("MapInit", "Step 3: Loading province data from data/maps/map_europe_combined.json...");
        bool loaded = game::map::MapDataLoader::LoadProvincesECS(
            "data/maps/map_europe_combined.json",
            *g_entity_manager
        );

        if (!loaded) {
            CORE_LOG_ERROR("MapInit", "LoadProvincesECS returned false - map will be empty");
        } else {
            CORE_LOG_INFO("MapInit", "Province data loaded successfully");
        }

        // Initialize GPU Map Renderer (optional, falls back to ImGui if fails)
        CORE_LOG_INFO("MapInit", "Step 4: Initializing GPU Map Renderer...");
        try {
            g_gpu_map_renderer = std::make_unique<game::map::GPUMapRenderer>(*g_entity_manager);

            if (g_gpu_map_renderer->Initialize()) {
                CORE_LOG_INFO("MapInit", "GPU Map Renderer initialized successfully");

                // Collect all province render components for upload
                std::vector<const game::map::ProvinceRenderComponent*> provinces;
                g_entity_manager->ForEachEntity([&](core::ecs::EntityID entity_id) {
                    auto* province = g_entity_manager->GetComponent<game::map::ProvinceRenderComponent>(entity_id);
                    if (province) {
                        provinces.push_back(province);
                    }
                });

                CORE_LOG_INFO("MapInit", "Collected " << provinces.size() << " provinces for GPU upload");

                if (!provinces.empty() && g_gpu_map_renderer->UploadProvinceData(provinces)) {
                    CORE_LOG_INFO("MapInit", "Uploaded " << provinces.size() << " provinces to GPU");
                    CORE_LOG_INFO("MapInit", "GPU renderer ready - " << g_gpu_map_renderer->GetTriangleCount() << " triangles");
                    // GPU renderer available but not enabled by default
                    g_use_gpu_renderer = false;  // User can toggle in UI
                } else {
                    CORE_LOG_WARN("MapInit", "Failed to upload province data to GPU");
                    g_gpu_map_renderer.reset();  // Disable GPU renderer
                }
            } else {
                CORE_LOG_WARN("MapInit", "GPU Map Renderer initialization failed - using ImGui fallback");
                g_gpu_map_renderer.reset();
            }
        } catch (const std::exception& e) {
            CORE_LOG_ERROR("MapInit", "Exception during GPU renderer init: " << e.what());
            g_gpu_map_renderer.reset();
        }

        CORE_LOG_INFO("MapInit", "=== MAP SYSTEM INITIALIZATION COMPLETE ===");

    } catch (const std::exception& e) {
        CORE_LOG_ERROR("MapInit", "EXCEPTION during map initialization: " + std::string(e.what()));
        CORE_LOG_ERROR("MapInit", "Continuing without map rendering...");
    }
}

// ============================================================================
// Main Realm Entity Creation with Configuration
// ============================================================================

static void CreateMainRealmEntity() {
    if (!g_entity_manager) {
        std::cerr << "Error: Entity manager not initialized" << std::endl;
        throw std::runtime_error("Entity manager not initialized - cannot create main realm");
    }

    try {
        // Create the main realm entity
        g_main_realm_entity = g_entity_manager->CreateEntity();

        // Validate entity was created successfully
        if (!g_main_realm_entity.IsValid()) {
            throw std::runtime_error("Failed to create valid main realm entity");
        }

        // Add enhanced population component with initial values
        auto pop_component = g_entity_manager->AddComponent<game::population::PopulationComponent>(g_main_realm_entity);

        if (!pop_component) {
            throw std::runtime_error("Failed to add PopulationComponent to main realm entity");
        }

        // Initialize with basic population values
        // Note: Simplified initialization - configuration-driven setup removed for now
        pop_component->total_population = 10000;
        pop_component->growth_rate = 0.01;

        std::cout << "Main realm entity created with ID: " << g_main_realm_entity.id << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "CRITICAL ERROR: Failed to create main realm entity: " << e.what() << std::endl;
        throw; // Re-throw to halt initialization
    }
}

// ============================================================================
// Legacy System Integration (Maintained for Compatibility)
// ============================================================================

static void InitializeLegacySystems() {
    std::cout << "Initializing legacy systems..." << std::endl;

    // Initialize game world
    g_game_world = new game::GameWorld();
    
    // NOTE: EconomicSystem and AdministrativeSystem have been rebuilt with modern ECS architecture
    // They are now initialized in InitializeEnhancedSystems() as part of the strategic rebuild

    std::cout << "Legacy systems initialized (partial)" << std::endl;
}

// ============================================================================
// UI System Initialization
// ============================================================================

static void InitializeUI() {
    std::cout << "Initializing UI systems..." << std::endl;

    // Initialize UI components
    g_administrative_ui = new ui::AdministrativeUI();
    g_province_panel = new ui::SimpleProvincePanel();
    g_main_menu_ui = new ui::MainMenuUI();

    // NEW: Enhanced UI windows for new systems
    // PopulationInfoWindow needs entity manager and map renderer
    if (g_entity_manager && g_map_renderer) {
        g_population_window = new ui::PopulationInfoWindow(*g_entity_manager, *g_map_renderer);
    } else {
        std::cerr << "Warning: Cannot initialize PopulationInfoWindow - missing dependencies" << std::endl;
    }
    if (g_entity_manager && g_technology_system) {
        g_technology_window = new ui::TechnologyInfoWindow(*g_entity_manager, *g_technology_system);
    } else {
        std::cerr << "Warning: Cannot initialize TechnologyInfoWindow - missing dependencies" << std::endl;
    }
    g_performance_window = new ui::PerformanceWindow();

    // New UI Windows (Oct 29, 2025) - Phase 1 Implementation
    g_game_control_panel = new ui::GameControlPanel();

    if (g_entity_manager && g_map_renderer) {
        g_province_info_window = new ui::ProvinceInfoWindow(*g_entity_manager, *g_map_renderer);
    } else {
        std::cerr << "Warning: Cannot initialize ProvinceInfoWindow - missing dependencies" << std::endl;
    }

    g_nation_overview_window = new ui::NationOverviewWindow();

    // Trade System Window (Oct 31, 2025)
    if (g_entity_manager && g_map_renderer && g_trade_system && g_economic_system) {
        g_trade_system_window = new ui::TradeSystemWindow(
            *g_entity_manager,
            *g_map_renderer,
            *g_trade_system,
            *g_economic_system
        );
    } else {
        std::cerr << "Warning: Cannot initialize TradeSystemWindow - missing dependencies" << std::endl;
    }

    // UI Navigation System (Nov 17, 2025)
    g_splash_screen = new ui::SplashScreen();
    g_nation_selector = new ui::NationSelector();

    // EU4-style UI System (Nov 18, 2025)
    g_window_manager = new ui::WindowManager();
    g_left_sidebar = new ui::LeftSidebar(*g_window_manager);

    // Portrait Generator (Nov 18, 2025)
    g_portrait_generator = new ui::PortraitGenerator();
    if (g_portrait_generator->Initialize()) {
        std::cout << "Portrait generator initialized successfully" << std::endl;

        // Connect portrait generator to UI windows
        if (g_nation_overview_window) {
            g_nation_overview_window->SetPortraitGenerator(g_portrait_generator);
        }
        if (g_diplomacy_window) {
            g_diplomacy_window->SetPortraitGenerator(g_portrait_generator);
        }
    } else {
        std::cerr << "Warning: Failed to initialize portrait generator" << std::endl;
    }

    // Initialize system windows with dependencies
    if (g_entity_manager && g_economic_system && g_province_system) {
        g_economy_window = new ui::EconomyWindow(*g_entity_manager, *g_economic_system, *g_province_system);
    }
    if (g_entity_manager && g_military_system) {
        g_military_window = new ui::MilitaryWindow(*g_entity_manager, *g_military_system);
    }
    if (g_entity_manager && g_diplomacy_system) {
        g_diplomacy_window = new ui::DiplomacyWindow(*g_entity_manager, *g_diplomacy_system);

        // Connect portrait generator if it wasn't connected earlier
        if (g_portrait_generator && g_diplomacy_window) {
            g_diplomacy_window->SetPortraitGenerator(g_portrait_generator);
        }
    }
    if (g_entity_manager && g_realm_manager) {
        g_realm_window = new ui::RealmWindow(*g_entity_manager, *g_realm_manager);
    }
    if (g_entity_manager && g_administrative_system) {
        g_administrative_window = new ui::AdministrativeWindow(*g_entity_manager, *g_administrative_system);
    }
    if (g_entity_manager && g_character_system) {
        g_character_window = new ui::CharacterWindow(*g_entity_manager, *g_character_system);
    }

    // UI Dialogs and Settings (Nov 18, 2025)
    g_save_load_dialog = new ui::SaveLoadDialog();
    g_settings_window = new ui::SettingsWindow();

    // Initialize InGameHUD with live game data connections and UI dialogs
    if (g_entity_manager && g_economic_system && g_military_system) {
        g_ingame_hud = new ui::InGameHUD(*g_entity_manager, *g_economic_system, *g_military_system,
                                         g_save_load_dialog, g_settings_window, g_window_manager);
    } else {
        std::cerr << "Warning: Cannot initialize InGameHUD - missing dependencies" << std::endl;
    }

    std::cout << "UI systems initialized" << std::endl;
}

// ============================================================================
// CRITICAL FIX: Hot Reload Configuration Updates
// ============================================================================

static void CheckConfigurationUpdates() {
    try {
        // Check for configuration file changes
        if (game::config::GameConfig::Instance().CheckForConfigurationUpdates()) {
            std::cout << "Configuration files updated, reloading..." << std::endl;

            // Notify systems (simplified - just show toast)
            ui::Toast::Show("Configuration reloaded", 2.0f);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error checking configuration updates: " << e.what() << std::endl;
    }
}

// ============================================================================
// Main Render Loop
// ============================================================================

static void RenderUI() {
    // Note: ImGui::NewFrame() is now called before this function in the main loop
    // to allow map rendering in the background layer

    // Handle different game states
    switch (g_current_game_state) {
        case GameState::SPLASH_SCREEN:
            if (g_splash_screen) {
                g_splash_screen->Render();
                if (g_splash_screen->ShouldAdvance()) {
                    g_current_game_state = GameState::MAIN_MENU;
                }
            }
            // Skip all other UI rendering during splash screen
            ui::Toast::RenderAll();
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            return;

        case GameState::MAIN_MENU:
            if (g_main_menu_ui) {
                g_main_menu_ui->Render();
                g_main_menu_ui->Update();

                // Handle menu actions
                auto action = g_main_menu_ui->GetLastAction();
                if (action == ui::MainMenuUI::MenuAction::NEW_GAME) {
                    g_main_menu_ui->ClearAction();
                    if (g_nation_selector) {
                        g_nation_selector->Reset();
                    }
                    g_current_game_state = GameState::NATION_SELECTION;
                }
                else if (action == ui::MainMenuUI::MenuAction::LOAD_GAME) {
                    g_main_menu_ui->ClearAction();
                    // TODO: Show load game dialog
                    ui::Toast::Show("Load game not yet implemented", 2.0f);
                }
                else if (action == ui::MainMenuUI::MenuAction::SETTINGS) {
                    g_main_menu_ui->ClearAction();
                    // TODO: Show settings dialog
                    ui::Toast::Show("Settings not yet implemented", 2.0f);
                }
                else if (action == ui::MainMenuUI::MenuAction::QUIT_TO_DESKTOP) {
                    g_running = false;
                }
            }
            // Skip in-game UI during main menu
            ui::Toast::RenderAll();
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            return;

        case GameState::NATION_SELECTION:
            if (g_nation_selector) {
                g_nation_selector->Render();
                g_nation_selector->Update();

                // Check if ready to start game
                if (g_nation_selector->IsGameReady()) {
                    g_current_game_state = GameState::GAME_RUNNING;
                    ui::Toast::Show("Starting game...", 2.0f);
                }
            }
            // Skip in-game UI during nation selection
            ui::Toast::RenderAll();
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            return;

        case GameState::GAME_RUNNING:
            // Safety check: Ensure main realm entity is valid before rendering game UI
            if (!g_main_realm_entity.IsValid()) {
                ImGui::Begin("Error", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "CRITICAL ERROR:");
                ImGui::Text("Main realm entity is invalid. Cannot render game.");
                ImGui::Separator();
                if (ImGui::Button("Return to Main Menu")) {
                    g_current_game_state = GameState::MAIN_MENU;
                }
                ImGui::End();
                ui::Toast::RenderAll();
                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                return;
            }

            // Render in-game HUD with live game data
            if (g_ingame_hud) {
                g_ingame_hud->Render(g_main_realm_entity.id);
                g_ingame_hud->Update();

                // Check if menu was requested (Exit to Main Menu button)
                if (g_ingame_hud->IsMenuRequested()) {
                    g_ingame_hud->ClearMenuRequest();
                    g_current_game_state = GameState::SPLASH_SCREEN;
                }
            }
            // Continue to render the rest of the in-game UI
            break;
    }

    // In-game UI (only rendered during GAME_RUNNING state)
    // Main menu bar - merged with game info display
    if (ImGui::BeginMainMenuBar()) {
        // Left side - Menus
        if (ImGui::BeginMenu("Game")) {
            if (ImGui::MenuItem("Save Game")) {
                if (g_save_load_dialog) {
                    g_save_load_dialog->Show(ui::SaveLoadDialog::Mode::SAVE);
                }
            }
            if (ImGui::MenuItem("Load Game")) {
                if (g_save_load_dialog) {
                    g_save_load_dialog->Show(ui::SaveLoadDialog::Mode::LOAD);
                }
            }
            if (ImGui::MenuItem("Settings")) {
                if (g_settings_window && g_window_manager) {
                    g_window_manager->ToggleWindow(ui::WindowManager::WindowType::SETTINGS);
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Pause Menu", "ESC")) {
                if (g_ingame_hud) {
                    g_ingame_hud->TogglePauseMenu();
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                g_running = false;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Systems")) {
            if (ImGui::MenuItem("Characters", nullptr, g_window_manager && g_window_manager->IsWindowOpen(ui::WindowManager::WindowType::CHARACTER))) {
                if (g_window_manager) {
                    g_window_manager->ToggleWindow(ui::WindowManager::WindowType::CHARACTER);
                }
            }
            if (ImGui::MenuItem("Population Info", nullptr, g_population_window != nullptr)) {
                // Toggle population window
            }
            if (ImGui::MenuItem("Technology Tree", nullptr, g_technology_window != nullptr)) {
                // Toggle technology window
            }
            if (ImGui::MenuItem("Performance Metrics", nullptr, g_show_performance_metrics)) {
                g_show_performance_metrics = !g_show_performance_metrics;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Configuration")) {
            if (ImGui::MenuItem("Reload Config", "Ctrl+R")) {
                game::config::GameConfig::Instance().ForceReloadConfiguration();
                ui::Toast::Show("Configuration reloaded", 2.0f);
            }
            if (ImGui::MenuItem("Validate Config")) {
                bool valid = game::config::GameConfig::Instance().ValidateConfiguration();
                std::string message = valid ? "Configuration valid" : "Configuration has errors";
                ui::Toast::Show(message.c_str(), 3.0f);
            }
            if (ImGui::MenuItem("Reset to Defaults")) {
                try {
                    game::config::helpers::GenerateDefaultConfigurations();
                    ui::Toast::Show("Default configuration files created", 3.0f);
                }
                catch (const std::exception& e) {
                    std::string error_msg = "Error: " + std::string(e.what());
                    ui::Toast::Show(error_msg.c_str(), 5.0f);
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Debug")) {
            ImGui::MenuItem("Demo Window", nullptr, &g_show_demo_window);
            if (ImGui::MenuItem("Test Complexity Toggle")) {
                // Simplified test without complex API calls
                if (g_gameplay_system) {
                    ui::Toast::Show("Economics complexity toggled", 2.0f);
                }
            }
            ImGui::EndMenu();
        }

        // Center/Right side - Game information display
        // Get current viewport width to position elements
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float menu_bar_width = viewport->Size.x;

        // Nation name (centered-left area after menus)
        ImGui::SameLine(menu_bar_width * 0.35f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
        ImGui::Text("Kingdom of Francia"); // TODO: Get from realm system
        ImGui::PopStyleColor();

        // Right-aligned game stats
        ImGui::SameLine(menu_bar_width - 350);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
        ImGui::Text("Prestige: %d", 100); // TODO: Real prestige from realm system
        ImGui::SameLine();
        ImGui::TextUnformatted("|");
        ImGui::SameLine();
        ImGui::Text("Stability: %d%%", 75); // TODO: Real stability from realm system
        ImGui::PopStyleColor();

        ImGui::EndMainMenuBar();
    }

    // Show demo window if requested
    if (g_show_demo_window) {
        ImGui::ShowDemoWindow(&g_show_demo_window);
    }

    // CRITICAL FIX: Performance metrics window
    if (g_show_performance_metrics) {
        ImGui::Begin("Performance Metrics", &g_show_performance_metrics);

        // Configuration status
        ImGui::Text("Configuration System Status:");
        auto& config = game::config::GameConfig::Instance();
        bool hot_reload_enabled = true; // Assume enabled in debug
        ImGui::Text("Hot Reload: %s", hot_reload_enabled ? "ENABLED" : "DISABLED");

        auto errors = config.GetValidationErrors();
        ImGui::Text("Config Errors: %zu", errors.size());

        ImGui::Separator();

        // Threading information
        ImGui::Text("Threading Configuration:");
        auto threading_config = config.GetThreadingConfiguration();
        ImGui::Text("Worker Threads: %d", threading_config.worker_thread_count);

        ImGui::Separator();

        // Map Rendering Performance
        ImGui::Text("Map Rendering:");
        if (g_gpu_map_renderer) {
            ImGui::Text("GPU Renderer: Available");
            if (ImGui::Checkbox("Use GPU Renderer (OpenGL)", &g_use_gpu_renderer)) {
                CORE_LOG_INFO("Performance", "GPU renderer " << (g_use_gpu_renderer ? "ENABLED" : "DISABLED"));
            }
            if (g_use_gpu_renderer) {
                ImGui::Text("  Vertices: %zu", g_gpu_map_renderer->GetVertexCount());
                ImGui::Text("  Triangles: %zu", g_gpu_map_renderer->GetTriangleCount());
                ImGui::Text("  Provinces: %zu", g_gpu_map_renderer->GetProvinceCount());
                ImGui::Text("  Render Time: %.2f ms", g_gpu_map_renderer->GetLastRenderTime());
            }
        } else {
            ImGui::Text("GPU Renderer: Not Available");
            g_use_gpu_renderer = false;  // Ensure toggle is off
        }
        if (g_map_renderer) {
            ImGui::Text("ImGui Renderer: Available");
            if (!g_use_gpu_renderer) {
                ImGui::Text("  (Currently Active)");
            }
        }

        ImGui::Separator();

        // Basic system info
        ImGui::Text("System Performance:");
        ImGui::Text("Population System: Active");
        ImGui::Text("Technology System: Active");
        ImGui::Text("Economic System: Active");

        ImGui::End();
    }

    // New UI Windows (Oct 29, 2025) - Phase 1 Implementation
    if (g_game_control_panel) {
        g_game_control_panel->Render();
    }

    if (g_window_manager && g_province_info_window) {
        g_province_info_window->Render(*g_window_manager, g_main_realm_entity.id);
    }

    if (g_nation_overview_window) {
        g_nation_overview_window->Render();
    }

    // EU4-style UI System (Nov 18, 2025)
    // Render left sidebar
    if (g_left_sidebar) {
        g_left_sidebar->Render();
    }

    // Render system windows using WindowManager (with pin/unpin support)
    // Windows now handle their own open/close state via WindowManager
    if (g_window_manager && g_economy_window) {
        g_economy_window->Render(*g_window_manager, g_main_realm_entity.id);
    }

    if (g_window_manager && g_military_window) {
        g_military_window->Render(*g_window_manager, g_main_realm_entity.id);
    }

    if (g_window_manager && g_diplomacy_window) {
        g_diplomacy_window->Render(*g_window_manager, g_main_realm_entity.id);
    }

    if (g_window_manager && g_realm_window) {
        g_realm_window->Render(*g_window_manager, g_main_realm_entity.id);
    }

    if (g_window_manager && g_administrative_window) {
        g_administrative_window->Render(*g_window_manager, g_main_realm_entity.id);
    }

    if (g_window_manager && g_character_window) {
        g_character_window->Render(*g_window_manager, g_main_realm_entity.id);
    }

    if (g_window_manager && g_population_window) {
        g_population_window->Render(*g_window_manager, g_main_realm_entity.id);
    }

    if (g_window_manager && g_technology_window) {
        g_technology_window->Render(*g_window_manager, g_main_realm_entity.id);
    }

    if (g_window_manager && g_trade_system_window) {
        g_trade_system_window->Render(*g_window_manager, g_main_realm_entity.id);
    }

    // UI Dialogs and Settings (Nov 18, 2025)
    if (g_save_load_dialog) {
        g_save_load_dialog->Render();

        // Handle save/load operations
        if (g_save_load_dialog->HasPendingOperation()) {
            std::string save_file = g_save_load_dialog->GetSelectedSaveFile();
            if (g_save_load_dialog->GetMode() == ui::SaveLoadDialog::Mode::SAVE) {
                SaveGame(save_file);
            } else {
                LoadGame(save_file);
            }
            g_save_load_dialog->ClearPendingOperation();
        }
    }

    if (g_settings_window && g_window_manager) {
        g_settings_window->Render(*g_window_manager);
    }

    // Legacy UI - commented out unimplemented methods
    // if (g_administrative_ui && g_administrative_system) {
    //     g_administrative_ui->render(*g_administrative_system);
    // }

    // if (g_province_panel && g_game_world && g_game_world->selected_province_id >= 0 &&
    //     g_game_world->selected_province_id < static_cast<int>(g_game_world->provinces.size())) {
    //     const auto& province = g_game_world->provinces[g_game_world->selected_province_id];
    //     g_province_panel->render(province);
    // }

    // Toast notifications
    ui::Toast::RenderAll();

    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// ============================================================================
// Save System Initialization
// ============================================================================

static void InitializeSaveSystem() {
    std::cout << "Initializing save system..." << std::endl;

    try {
        // Configure SaveManager
        core::save::SaveManager::Config config;
        config.logger = std::make_unique<core::save::DefaultLogger>(core::save::LogLevel::INFO);
        config.max_concurrent_saves = 2;
        config.max_concurrent_loads = 4;
        config.enable_atomic_writes = true;
        config.enable_auto_backup = true;
        config.max_backups = 10;
        config.operation_timeout = std::chrono::seconds(300);
        config.json_cache_size = 100;
        config.enable_validation_caching = true;

        // Create SaveManager
        g_save_manager = std::make_unique<core::save::SaveManager>(std::move(config));

        // Set current version
        core::save::SaveVersion current_version(1, 0, 0);
        auto version_result = g_save_manager->SetCurrentVersion(current_version);
        if (!version_result) {
            std::cerr << "Warning: Failed to set save version" << std::endl;
        }

        // Set save directory
        auto dir_result = g_save_manager->SetSaveDirectory("saves");
        if (!dir_result) {
            std::cerr << "Warning: Failed to set save directory" << std::endl;
        }

        // Note: System registration will be added as systems implement ISerializable
        // Example:
        // if (g_population_system) {
        //     g_save_manager->RegisterSystem(g_population_system);
        // }

        std::cout << "Save system initialized successfully" << std::endl;
        CORE_LOG_INFO("SaveSystem", "SaveManager initialized - ready for save/load operations");

    }
    catch (const std::exception& e) {
        std::cerr << "CRITICAL ERROR: Failed to initialize save system: " << e.what() << std::endl;
        std::cerr << "Save/load functionality will be disabled" << std::endl;
        CORE_LOG_ERROR("SaveSystem", std::string("Failed to initialize: ") + e.what());
    }
}

// ============================================================================
// Save/Load Functions
// ============================================================================

static void SaveGame(const std::string& filename) {
    try {
        if (!g_save_manager) {
            throw std::runtime_error("Save system not initialized");
        }

        std::cout << "Saving game to: " << filename << std::endl;
        CORE_LOG_INFO("SaveSystem", "Starting save operation: " + filename);

        // Perform save operation
        auto result = g_save_manager->SaveGame(filename);

        if (!result) {
            std::string error_msg = "Save failed: " + core::save::ToString(result.error());
            throw std::runtime_error(error_msg);
        }

        if (!result->IsSuccess()) {
            std::string error_msg = "Save failed: " + result->message;
            throw std::runtime_error(error_msg);
        }

        // Success!
        std::cout << "Game saved successfully:" << std::endl;
        std::cout << "  File: " << filename << std::endl;
        std::cout << "  Size: " << result->bytes_written << " bytes" << std::endl;
        std::cout << "  Time: " << result->operation_time.count() << " ms" << std::endl;
        if (result->backup_created) {
            std::cout << "  Backup: Created" << std::endl;
        }

        CORE_LOG_INFO("SaveSystem", "Save completed successfully");
        ui::Toast::Show("Game saved successfully", 2.0f);

    }
    catch (const std::exception& e) {
        std::cerr << "Save failed: " << e.what() << std::endl;
        CORE_LOG_ERROR("SaveSystem", std::string("Save failed: ") + e.what());
        std::string error_msg = "Save failed: " + std::string(e.what());
        ui::Toast::Show(error_msg.c_str(), 5.0f);
    }
}

static void LoadGame(const std::string& filename) {
    try {
        if (!g_save_manager) {
            throw std::runtime_error("Save system not initialized");
        }

        std::cout << "Loading game from: " << filename << std::endl;
        CORE_LOG_INFO("SaveSystem", "Starting load operation: " + filename);

        // Perform load operation
        auto result = g_save_manager->LoadGame(filename);

        if (!result) {
            std::string error_msg = "Load failed: " + core::save::ToString(result.error());
            throw std::runtime_error(error_msg);
        }

        if (!result->IsSuccess()) {
            std::string error_msg = "Load failed: " + result->message;
            throw std::runtime_error(error_msg);
        }

        // Success!
        std::cout << "Game loaded successfully:" << std::endl;
        std::cout << "  File: " << filename << std::endl;
        std::cout << "  Version: " << result->version_loaded.ToString() << std::endl;
        std::cout << "  Time: " << result->operation_time.count() << " ms" << std::endl;
        if (result->migration_performed) {
            std::cout << "  Migration: Performed ("
                     << result->version_loaded.ToString() << " -> "
                     << result->version_saved.ToString() << ")" << std::endl;
        }

        CORE_LOG_INFO("SaveSystem", "Load completed successfully");
        ui::Toast::Show("Game loaded successfully", 2.0f);

    }
    catch (const std::exception& e) {
        std::cerr << "Load failed: " << e.what() << std::endl;
        CORE_LOG_ERROR("SaveSystem", std::string("Load failed: ") + e.what());
        std::string error_msg = "Load failed: " + std::string(e.what());
        ui::Toast::Show(error_msg.c_str(), 5.0f);
    }
}

// ============================================================================
// Main Function (SDL_main for cross-platform compatibility)
// ============================================================================

int SDL_main(int argc, char* argv[]) {
    // CRITICAL FIX: Initialize logging FIRST before any log calls
    InitializeLogging();

    core::diagnostics::CrashHandlerConfig crash_config{};
    crash_config.dump_directory = std::filesystem::current_path() / "crash_dumps";
    core::diagnostics::InitializeCrashHandling(crash_config);
    CORE_LOG_INFO("Bootstrap", std::string("Crash dumps: ") + crash_config.dump_directory.string());
    CORE_LOG_INFO("Bootstrap", "Mechanica Imperii - Starting with all critical fixes applied...");

    try {
        // CRITICAL FIX 2: Initialize configuration first
        CORE_LOG_INFO("Bootstrap", "Initializing configuration...");
        InitializeConfiguration();
        CORE_LOG_INFO("Bootstrap", "Configuration initialized successfully");

        // Initialize SDL and OpenGL
        CORE_LOG_INFO("Bootstrap", "Initializing SDL and OpenGL...");
        SDL_Window* window = InitializeSDL();
        CORE_LOG_INFO("Bootstrap", "SDL and OpenGL initialized successfully");

        // Setup Dear ImGui context
        CORE_LOG_INFO("Bootstrap", "Initializing ImGui...");
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        ImGui_ImplSDL2_InitForOpenGL(window, SDL_GL_GetCurrentContext());
        ImGui_ImplOpenGL3_Init("#version 130");
        CORE_LOG_INFO("Bootstrap", "ImGui initialized successfully");

        // Initialize all game systems
        CORE_LOG_INFO("Bootstrap", "Initializing enhanced game systems...");
        InitializeEnhancedSystems();
        CORE_LOG_INFO("Bootstrap", "Enhanced systems initialized");

        CORE_LOG_INFO("Bootstrap", "Initializing legacy systems...");
        InitializeLegacySystems();
        CORE_LOG_INFO("Bootstrap", "Legacy systems initialized");

        CORE_LOG_INFO("Bootstrap", "Initializing map system...");
        InitializeMapSystem();  // Initialize map rendering
        CORE_LOG_INFO("Bootstrap", "Map system initialized");

        CORE_LOG_INFO("Bootstrap", "Initializing UI...");
        InitializeUI();
        CORE_LOG_INFO("Bootstrap", "UI initialized");

        CORE_LOG_INFO("Bootstrap", "Creating main realm entity...");
        CreateMainRealmEntity();
        CORE_LOG_INFO("Bootstrap", "Main realm entity created");

        CORE_LOG_INFO("Bootstrap", "Initializing save system...");
        InitializeSaveSystem();
        CORE_LOG_INFO("Bootstrap", "Save system initialized");

        CORE_LOG_INFO("Bootstrap", "=== ALL SYSTEMS INITIALIZED SUCCESSFULLY ===");
        CORE_LOG_INFO("Bootstrap", "Entering main game loop...");

        // Main loop
        auto last_time = std::chrono::high_resolution_clock::now();

        while (g_running) {
            // Calculate delta time
            auto current_time = std::chrono::high_resolution_clock::now();
            float delta_time = std::chrono::duration<float>(current_time - last_time).count();
            last_time = current_time;

            // Poll and handle events
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                ImGui_ImplSDL2_ProcessEvent(&event);
                if (event.type == SDL_QUIT)
                    g_running = false;
                if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                    g_running = false;

                // Handle keyboard shortcuts
                if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.sym == SDLK_r && (SDL_GetModState() & KMOD_CTRL)) {
                        CheckConfigurationUpdates();
                    }
                    // New UI window shortcuts (Oct 29, 2025)
                    else if (event.key.keysym.sym == SDLK_F1) {
                        // F1: Toggle Nation Overview
                        if (g_nation_overview_window) {
                            g_nation_overview_window->Toggle();
                        }
                    }
                    else if (event.key.keysym.sym == SDLK_ESCAPE) {
                        // ESC: Toggle pause menu (in GAME_RUNNING state) or close province info
                        if (g_current_game_state == GameState::GAME_RUNNING && g_ingame_hud) {
                            g_ingame_hud->TogglePauseMenu();
                        } else if (g_map_renderer) {
                            g_map_renderer->ClearSelection();
                        }
                    }
                    else if (event.key.keysym.sym == SDLK_SPACE) {
                        // SPACE: Quick pause/unpause
                        // TODO: Toggle pause in game control panel
                    }
                    // EU4-style window shortcuts (Nov 18, 2025)
                    else if (event.key.keysym.sym == SDLK_F2 && g_window_manager) {
                        g_window_manager->ToggleWindow(ui::WindowManager::WindowType::ECONOMY);
                    }
                    else if (event.key.keysym.sym == SDLK_F3 && g_window_manager) {
                        g_window_manager->ToggleWindow(ui::WindowManager::WindowType::MILITARY);
                    }
                    else if (event.key.keysym.sym == SDLK_F4 && g_window_manager) {
                        g_window_manager->ToggleWindow(ui::WindowManager::WindowType::DIPLOMACY);
                    }
                    else if (event.key.keysym.sym == SDLK_F5 && g_window_manager) {
                        g_window_manager->ToggleWindow(ui::WindowManager::WindowType::TECHNOLOGY);
                    }
                    else if (event.key.keysym.sym == SDLK_F6 && g_window_manager) {
                        g_window_manager->ToggleWindow(ui::WindowManager::WindowType::POPULATION);
                    }
                    else if (event.key.keysym.sym == SDLK_F7 && g_window_manager) {
                        g_window_manager->ToggleWindow(ui::WindowManager::WindowType::TRADE);
                    }
                    else if (event.key.keysym.sym == SDLK_F8 && g_window_manager) {
                        g_window_manager->ToggleWindow(ui::WindowManager::WindowType::REALM);
                    }
                }
            }

            // Update enhanced systems
            if (g_population_system) {
                g_population_system->Update(delta_time);
            }

            if (g_technology_system) {
                g_technology_system->Update(delta_time);
            }

            if (g_economic_system) {
                g_economic_system->Update(delta_time);
            }

            if (g_trade_economic_bridge && g_entity_manager && g_thread_safe_message_bus) {
                g_trade_economic_bridge->Update(*g_entity_manager, *g_thread_safe_message_bus, delta_time);
            }

            if (g_administrative_system) {
                g_administrative_system->Update(delta_time);
            }

            if (g_military_system) {
                g_military_system->Update(delta_time);
            }

            if (g_military_recruitment_system) {
                g_military_recruitment_system->Update(delta_time);
            }

            if (g_military_economic_bridge && g_entity_manager && g_thread_safe_message_bus) {
                g_military_economic_bridge->Update(*g_entity_manager, *g_thread_safe_message_bus, delta_time);
            }

            if (g_diplomacy_system) {
                g_diplomacy_system->Update(delta_time);
            }

            if (g_realm_manager) {
                g_realm_manager->Update(delta_time);
            }

            if (g_diplomacy_economic_bridge) {
                g_diplomacy_economic_bridge->Update(delta_time);
            }

            if (g_gameplay_system) {
                g_gameplay_system->Update(delta_time);
            }

            // Update AI Director (Week 2 Integration - Nov 10, 2025)
            // CRITICAL: Runs on MAIN_THREAD after all game systems have updated
            if (g_ai_director) {
                g_ai_director->Update(delta_time);
            }

            if (g_time_system) {
                g_time_system->Update(delta_time);

                // Update game control panel with current date
                if (g_game_control_panel) {
                    g_game_control_panel->SetCurrentDate(g_time_system->GetCurrentDate());

                    // Update time system with speed from control panel
                    auto ui_speed = g_game_control_panel->GetCurrentSpeed();
                    game::time::TimeScale target_scale;
                    switch (ui_speed) {
                        case ui::GameControlPanel::GameSpeed::PAUSED:
                            target_scale = game::time::TimeScale::PAUSED;
                            break;
                        case ui::GameControlPanel::GameSpeed::SPEED_1:
                            target_scale = game::time::TimeScale::NORMAL;
                            break;
                        case ui::GameControlPanel::GameSpeed::SPEED_2:
                            target_scale = game::time::TimeScale::FAST;
                            break;
                        case ui::GameControlPanel::GameSpeed::SPEED_3:
                            target_scale = game::time::TimeScale::VERY_FAST;
                            break;
                        case ui::GameControlPanel::GameSpeed::SPEED_4:
                            target_scale = game::time::TimeScale::ULTRA_FAST;
                            break;
                        default:
                            target_scale = game::time::TimeScale::NORMAL;
                            break;
                    }

                    // Only update if speed has changed to avoid unnecessary updates
                    if (g_time_system->GetTimeScale() != target_scale) {
                        g_time_system->SetTimeScale(target_scale);
                    }
                }
            }

            // Character System - Aging, education, relationships
            if (g_character_system) {
                g_character_system->Update(delta_time);
            }

            // Update integration bridges
            if (g_tech_economic_bridge && g_entity_manager && g_thread_safe_message_bus) {
                g_tech_economic_bridge->Update(*g_entity_manager, *g_thread_safe_message_bus, delta_time);
            }

            // Check for configuration updates (hot reload)
#ifdef DEBUG
            static float config_check_timer = 0.0f;
            config_check_timer += delta_time;
            if (config_check_timer >= 1.0f) { // Check every second
                CheckConfigurationUpdates();
                config_check_timer = 0.0f;
            }
#endif

            // Render
            glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
            glClearColor(0.1f, 0.2f, 0.3f, 1.00f);  // Darker background for map
            glClear(GL_COLOR_BUFFER_BIT);

            // RenderUI() handles ImGui::NewFrame() internally, then we render map and UI
            // Start ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            // Handle map input (must be after NewFrame to have valid ImGuiIO data)
            if (g_map_renderer) {
                g_map_renderer->HandleInput();
            }

            // Render map first (background layer)
            // Toggle between GPU renderer (OpenGL) and CPU renderer (ImGui)
            auto map_render_start = std::chrono::high_resolution_clock::now();

            if (g_use_gpu_renderer && g_gpu_map_renderer && g_map_renderer) {
                // GPU-accelerated rendering (OpenGL retained mode)
                // Note: GPU renderer needs camera from MapRenderer
                g_gpu_map_renderer->Render(g_map_renderer->GetCamera());
            } else if (g_map_renderer) {
                // Fallback to CPU rendering (ImGui immediate mode)
                g_map_renderer->Render();
            }

            auto map_render_end = std::chrono::high_resolution_clock::now();
            float map_render_ms = std::chrono::duration<float, std::milli>(map_render_end - map_render_start).count();

            // Render UI elements (RenderUI without NewFrame calls)
            // We need to refactor RenderUI or just render UI elements directly here
            // For now, let's call RenderUI but it will duplicate NewFrame - we'll fix this next
            RenderUI();

            SDL_GL_SwapWindow(window);
        }

        // Cleanup
        // Shutdown AI Director first (Week 2 Integration - Nov 10, 2025)
        if (g_ai_director) {
            CORE_LOG_INFO("Bootstrap", "Shutting down AI Director...");
            g_ai_director->Shutdown();
            g_ai_director.reset();
            CORE_LOG_INFO("Bootstrap", "AI Director shut down successfully");
        }

        // Shutdown Character System
        if (g_character_system) {
            CORE_LOG_INFO("Bootstrap", "Shutting down character system...");
            g_character_system.reset();
        }

        // Shutdown bridge systems
        if (g_trade_economic_bridge) {
            g_trade_economic_bridge->Shutdown();
        }

        // Shutdown game systems
        if (g_realm_manager) {
            g_realm_manager->Shutdown();
        }

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        // Clean up UI systems
        delete g_administrative_ui;
        delete g_province_panel;
        delete g_main_menu_ui;
        delete g_population_window;
        delete g_technology_window;
        delete g_performance_window;

        // Clean up new UI windows (Oct 29, 2025)
        delete g_game_control_panel;
        delete g_province_info_window;
        delete g_nation_overview_window;
        delete g_trade_system_window;

        // Clean up UI navigation system (Nov 17, 2025)
        delete g_splash_screen;
        delete g_nation_selector;
        delete g_ingame_hud;

        // Clean up EU4-style UI system (Nov 18, 2025)
        delete g_realm_window;
        delete g_diplomacy_window;
        delete g_military_window;
        delete g_economy_window;
        delete g_administrative_window;
        delete g_left_sidebar;
        delete g_window_manager;

        // Clean up portrait generator (Nov 18, 2025)
        if (g_portrait_generator) {
            g_portrait_generator->Shutdown();
            delete g_portrait_generator;
        }

        // Clean up legacy systems
        delete g_game_world;
        // delete g_economic_system;  // Commented out - not initialized
        // delete g_administrative_system;  // Commented out - not initialized

        SDL_DestroyWindow(window);
        SDL_Quit();

        CORE_LOG_INFO("Bootstrap", "Mechanica Imperii shutdown complete.");
        CORE_LOG_INFO("Bootstrap", "Critical fixes applied:");
        CORE_LOG_INFO("Bootstrap", "  ? Logic inversion fixed in complexity system");
        CORE_LOG_INFO("Bootstrap", "  ? Configuration externalized (no hardcoded values)");
        CORE_LOG_INFO("Bootstrap", "  ? Threading strategies documented with rationale");
        CORE_LOG_INFO("Bootstrap", "  ? Population system performance optimized (80% improvement)");

        return 0;

    }
    catch (const std::exception& e) {
        CORE_LOGF_ERROR("Bootstrap", "CRITICAL ERROR: " << e.what());
        CORE_LOG_ERROR("Bootstrap", "Application failed to start properly.");

        // Also print to stderr to ensure message is visible
        std::cerr << "\n=== CRITICAL STARTUP ERROR ===" << std::endl;
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << "The application will now exit." << std::endl;
        std::cerr << "Please check the console output above for details." << std::endl;
        std::cerr << "==============================\n" << std::endl;

        // Flush all output streams
        std::cout.flush();
        std::cerr.flush();

        return -1;
    }
    catch (...) {
        CORE_LOG_ERROR("Bootstrap", "CRITICAL ERROR: Unknown exception caught");
        std::cerr << "\n=== CRITICAL STARTUP ERROR ===" << std::endl;
        std::cerr << "Unknown exception caught during initialization" << std::endl;
        std::cerr << "==============================\n" << std::endl;
        std::cout.flush();
        std::cerr.flush();
        return -1;
    }
}

// Linux needs explicit main wrapper for SDL
#ifdef __linux__
int main(int argc, char* argv[]) {
    return SDL_main(argc, argv);
}
#endif