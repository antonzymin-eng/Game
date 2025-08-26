cmake_minimum_required(VERSION 3.20)
project(MechanicaImperii LANGUAGES CXX)

# ---- C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Options
option(MI_BUILD_EXE "Build main app" ON)
option(MI_USE_GLOB "Use file globs (not recommended)" OFF)
option(MI_COPY_ASSETS "Copy assets next to the exe after build (only if assets/ exists at configure time)" ON)

# ---- Dependencies (via vcpkg toolchain)
find_package(SDL2 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW CONFIG REQUIRED)            # Using GLEW loader
find_package(nlohmann_json CONFIG REQUIRED)

# ---- ImGui (core + backends)
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)
add_library(imgui STATIC
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
)
# Backends need SDL2 headers and a GL loader during compilation
target_link_libraries(imgui PUBLIC SDL2::SDL2 OpenGL::GL GLEW::GLEW)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLEW)

# ---- Helper to append files only if present
function(append_if_exists outvar)
  set(tmp "")
  foreach(path IN LISTS ARGN)
    if(EXISTS "${CMAKE_SOURCE_DIR}/${path}")
      list(APPEND tmp "${path}")
    endif()
  endforeach()
  set(${outvar} ${${outvar}} ${tmp} PARENT_SCOPE)
endfunction()

# ---- Core sources (explicit list; keep only files that actually exist)
set(CORE_SRC
  # IO
  src/io/SaveLoad.cpp
  src/io/Persist.cpp

  # State
  src/state/SaveAdapters.cpp
  src/state/SimulationState.cpp
  src/state/EngineBindings.cpp
  src/state/EngineCommands.cpp
  src/state/EngineFeeds.cpp
  src/state/EventBus.cpp
  src/state/Feeds.cpp
  src/state/Loaders.cpp
  src/state/Time.cpp

  # UI
  src/ui/HUDPanels.cpp
  src/ui/HUDState.cpp
  src/ui/SaveLoadUI.cpp
  src/ui/GameScreen.cpp
  src/ui/MainMenuUI.cpp
  src/ui/PauseMenu.cpp
  src/ui/AgentsPanel.cpp
  src/ui/AssetPath.cpp
  src/ui/BuildingsUI.cpp
  src/ui/CreditsUI.cpp
  src/ui/DiplomacyPanel.cpp
  src/ui/EconomyPanel.cpp
  src/ui/EventsPanel.cpp
  src/ui/FactionsPanel.cpp
  src/ui/LeftSidebar.cpp
  src/ui/MapViewport.cpp
  src/ui/MilitaryPanel.cpp
  src/ui/NationOverview.cpp
  src/ui/NavalPanel.cpp
  src/ui/NewGameUI.cpp
  src/ui/Notify.cpp
  src/ui/PanelSignals.cpp
  src/ui/PopulationPanel.cpp
  src/ui/ProvincePanelUI.cpp
  src/ui/RoutesPanel.cpp
  src/ui/SelectionContext.cpp
  src/ui/Style.cpp
  src/ui/SystemPanels_Left.cpp
  src/ui/SystemsDock.cpp
  src/ui/TechPanel.cpp
  src/ui/Theme.cpp
  src/ui/Toast.cpp
  src/ui/TradePanel.cpp
  src/ui/UIPrimitives.cpp
  src/ui/WeatherPanel.cpp
  src/ui/persist_stubs.cpp
  src/ui/stb_stubs.cpp
   src/ui/ScreenAPI.cpp
  src/ui/SystemsPanel.cpp

  # persistence
  src/persistence/SaveSystem.cpp
)

# Optional sources (only add if they exist locally)
append_if_exists(CORE_SRC
  src/ui/SettingsUI.cpp
  src/ui/Minimap.cpp
  src/ui/FloatingButtons.cpp
  src/ui/ImguiCompat.cpp
  # If you still have an alternative App singleton, uncomment:
  # src/ui/AppSingleton.cpp
)

list(REMOVE_DUPLICATES CORE_SRC)

# ---- Core library
add_library(mech_core STATIC ${CORE_SRC})
target_include_directories(mech_core PUBLIC
  "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/src"
)
# STB headers (stb_image.h lives here)
target_include_directories(mech_core PRIVATE
  ${CMAKE_SOURCE_DIR}/external/stb
)

# Link explicitly; don't rely on imgui to re-export everything in all setups
target_link_libraries(mech_core PUBLIC
  imgui
  SDL2::SDL2
  OpenGL::GL
  GLEW::GLEW
  nlohmann_json::nlohmann_json
)

# ---- App (single entry point)
if (MI_BUILD_EXE)
  add_executable(mechanica_imperii apps/main.cpp)
  target_link_libraries(mechanica_imperii PRIVATE mech_core)

  # SDL2main belongs on the executable
  if (TARGET SDL2::SDL2main)
    target_link_libraries(mechanica_imperii PRIVATE SDL2::SDL2main)
  endif()

  # Windows convenience define to avoid min/max macro collisions
  if (WIN32)
    target_compile_definitions(mechanica_imperii PRIVATE NOMINMAX)
  endif()

  # Output folders (keep binaries in build tree)
  foreach(cfg Debug Release RelWithDebInfo MinSizeRel)
    set_target_properties(mechanica_imperii PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY_${cfg} ${CMAKE_BINARY_DIR}/bin/${cfg})
  endforeach()

  # When launching from VS, run from the target dir
  set_property(TARGET mechanica_imperii PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$(TargetDir)")

  # Post-build asset copy (ONLY if assets/ exists at configure time)
  if (MI_COPY_ASSETS)
    if (EXISTS "${CMAKE_SOURCE_DIR}/assets")
      add_custom_command(TARGET mechanica_imperii POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
                "${CMAKE_SOURCE_DIR}/assets"
                "$<TARGET_FILE_DIR:mechanica_imperii>/assets"
        VERBATIM)
      message(STATUS "Post-build: will copy assets/ to the output folder.")
    else()
      message(STATUS "assets/ not found at configure time; skipping post-build copy.")
    endif()
  endif()
endif()

# ---- Coverage check: ensure no applicable .cpp is left out
file(GLOB_RECURSE ALL_CORE_CANDIDATES
  "${CMAKE_SOURCE_DIR}/src/ui/*.cpp"
  "${CMAKE_SOURCE_DIR}/src/io/*.cpp"
  "${CMAKE_SOURCE_DIR}/src/state/*.cpp"
  "${CMAKE_SOURCE_DIR}/src/persistence/*.cpp"
)

# Ignore list: sources that exist but should NOT be in mech_core
set(IGNORE_SOURCES
  "${CMAKE_SOURCE_DIR}/src/main_min.cpp"
  "${CMAKE_SOURCE_DIR}/apps/demo_main.cpp"
  "${CMAKE_SOURCE_DIR}/src/io/AssetPath.cpp" 
  "${CMAKE_SOURCE_DIR}/src/ui/AppSingleton.cpp" 
  "${CMAKE_SOURCE_DIR}/src/ui/DataBridge.cpp" 
)

# Normalize to repo-relative paths & filter ignored
set(ALL_CORE_REL "")
foreach(f ${ALL_CORE_CANDIDATES})
  list(FIND IGNORE_SOURCES "${f}" _ix)
  if(_ix GREATER -1)
    continue()
  endif()
  file(RELATIVE_PATH rel "${CMAKE_SOURCE_DIR}" "${f}")
  list(APPEND ALL_CORE_REL "${rel}")
endforeach()

list(REMOVE_DUPLICATES CORE_SRC)
set(MISSING "")
foreach(f ${ALL_CORE_REL})
  list(FIND CORE_SRC "${f}" _idx)
  if(_idx EQUAL -1)
    list(APPEND MISSING "${f}")
  endif()
endforeach()

if(MISSING)
  message(FATAL_ERROR "CMakeLists is missing these sources:\n  - ${MISSING}\nAdd them to CORE_SRC or to IGNORE_SOURCES.")
endif()
