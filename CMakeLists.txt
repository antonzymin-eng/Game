cmake_minimum_required(VERSION 3.24)
project(mechanica_imperii LANGUAGES CXX)

# -----------------------
# Global compiler setup
# -----------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (MSVC)
  add_compile_options(/W4 /permissive- /Zc:__cplusplus /Zc:preprocessor /EHsc /MP)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Put exe runtime in project root (so assets resolve easily)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

# -----------------------
# Dependencies (vcpkg)
# Configure CMake with:
#   -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake"
# -----------------------
find_package(SDL2 CONFIG REQUIRED)     # SDL2::SDL2 (+ SDL2::SDL2main if available)
find_package(OpenGL REQUIRED)          # OpenGL::GL

# Optional GL loader (prefer glad)
find_package(glad CONFIG QUIET)        # glad::glad
if (NOT glad_FOUND)
  find_package(GLEW CONFIG QUIET)      # GLEW::GLEW
endif()

# -----------------------
# Dear ImGui (vendored)
# -----------------------
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/external/imgui")
if (NOT EXISTS "${IMGUI_DIR}/imgui.cpp")
  message(FATAL_ERROR
    "Dear ImGui sources not found at ${IMGUI_DIR}.\n"
    "Copy ImGui (with backends) to external/imgui")
endif()

option(BUILD_IMGUI_DEMO "Build ImGui demo window" OFF)

set(IMGUI_SOURCES
  "${IMGUI_DIR}/imgui.cpp"
  "${IMGUI_DIR}/imgui_draw.cpp"
  "${IMGUI_DIR}/imgui_tables.cpp"
  "${IMGUI_DIR}/imgui_widgets.cpp"
  "${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp"
  "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
  "${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp"   # <- enables std::string overloads
)
if (BUILD_IMGUI_DEMO AND EXISTS "${IMGUI_DIR}/imgui_demo.cpp")
  list(APPEND IMGUI_SOURCES "${IMGUI_DIR}/imgui_demo.cpp")
endif()

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
  "${IMGUI_DIR}"
  "${IMGUI_DIR}/backends"
  "${IMGUI_DIR}/misc/cpp"
)
target_link_libraries(imgui PUBLIC OpenGL::GL SDL2::SDL2)
if (TARGET SDL2::SDL2main)
  target_link_libraries(imgui PUBLIC SDL2::SDL2main)
endif()

# Pick GL loader for OpenGL3 backend
if (glad_FOUND)
  target_link_libraries(imgui PUBLIC glad::glad)
  target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)
elseif (GLEW_FOUND)
  target_link_libraries(imgui PUBLIC GLEW::GLEW)
  target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLEW)
else()
  message(WARNING "No GL loader found (glad/GLEW). Consider adding 'glad' to vcpkg.json.")
endif()

# -----------------------
# Engine/library targets
# -----------------------
# Gather sources (adjust/extend as needed)
file(GLOB_RECURSE CORE_SRC      CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/core/*.cpp")
file(GLOB_RECURSE UI_SRC        CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/ui/*.cpp")
file(GLOB_RECURSE PLAT_SRC      CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/platform/*.cpp")
file(GLOB_RECURSE IO_SRC        CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/io/*.cpp")
file(GLOB_RECURSE STATE_SRC     CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/state/*.cpp")
# Optional persistence sources (uncomment if you add them)
file(GLOB_RECURSE PERSIST_SRC   CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/persistence/*.cpp")

add_library(mech_core STATIC
  ${CORE_SRC} ${UI_SRC} ${PLAT_SRC} ${IO_SRC} ${STATE_SRC} ${PERSIST_SRC}
)

# Some headers live under include/ and src/
target_include_directories(mech_core PUBLIC
  "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/src"
)

target_link_libraries(mech_core PUBLIC imgui SDL2::SDL2 OpenGL::GL)
if (TARGET SDL2::SDL2main)
  target_link_libraries(mech_core PUBLIC SDL2::SDL2main)
endif()

# -----------------------
# Applications
# -----------------------
function(_add_app exe_name src_path)
  if (EXISTS "${CMAKE_SOURCE_DIR}/${src_path}")
    add_executable(${exe_name} "${src_path}")
    target_link_libraries(${exe_name} PRIVATE mech_core)
    set_property(TARGET ${exe_name} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
  endif()
endfunction()

_add_app(mechanica_imperii     "apps/main.cpp")
_add_app(mechanica_imperii_min "apps/main_min.cpp")

# -----------------------
# IDE / source groups
# -----------------------
file(GLOB_RECURSE PUBLIC_HDR CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/include/*")
source_group(TREE "${CMAKE_SOURCE_DIR}/src"     PREFIX "Source"
             FILES ${CORE_SRC} ${UI_SRC} ${PLAT_SRC} ${IO_SRC} ${STATE_SRC} ${PERSIST_SRC})
source_group(TREE "${CMAKE_SOURCE_DIR}/include" PREFIX "Include" FILES ${PUBLIC_HDR})
source_group(TREE "${IMGUI_DIR}"                PREFIX "external/imgui" FILES ${IMGUI_SOURCES})
target_sources(mech_core PRIVATE ${PUBLIC_HDR})

# -----------------------
# Summary
# -----------------------
message(STATUS "SDL2:        FOUND")
if (glad_FOUND)
  message(STATUS "OpenGL loader: GLAD")
elseif (GLEW_FOUND)
  message(STATUS "OpenGL loader: GLEW")
else()
  message(STATUS "OpenGL loader: NONE")
endif()
message(STATUS "ImGui dir:   ${IMGUI_DIR}")
