# Cross-Platform Build Guide

**Last Updated:** October 21, 2025

## Overview

Mechanica Imperii supports **automatic platform detection** and configuration for both Windows and Linux. The build system automatically selects the correct dependencies and compiler settings based on your platform.

---

## Platform Support

| Platform | Compiler | Status | Package Manager |
|----------|----------|--------|----------------|
| **Linux** | GCC 13+ / Clang 15+ | ✅ **Tested** | apt, pkg-config |
| **Windows** | MSVC 2022 / MinGW-w64 | ⚙️ **In Progress** | vcpkg |
| **macOS** | Clang 15+ (Apple) | 🔄 Planned | Homebrew |

---

## Quick Start

### Linux (Ubuntu/Debian)

```bash
# Install dependencies
sudo apt update
sudo apt install -y build-essential cmake pkg-config \
    libsdl2-dev libjsoncpp-dev libimgui-dev \
    libglx-dev libopengl-dev libssl-dev liblz4-dev

# Build
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# Run
./mechanica_imperii
```

### Windows (using vcpkg)

```powershell
# Install vcpkg (if not already installed)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install

# Install dependencies
.\vcpkg install sdl2 glad jsoncpp openssl lz4 imgui

# Build with CMake
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release

# Run
.\Release\mechanica_imperii.exe
```

---

## Platform-Specific Details

### Dependency Differences

| Library | Linux Package | Windows (vcpkg) |
|---------|--------------|-----------------|
| **SDL2** | `libsdl2-dev` | `sdl2` |
| **JsonCpp** | `libjsoncpp-dev` | `jsoncpp` |
| **ImGui** | `libimgui-dev` | `imgui` |
| **OpenGL** | `libglx-dev`, `libopengl-dev` | Built-in (GLAD) |
| **OpenSSL** | `libssl-dev` | `openssl` |
| **LZ4** | `liblz4-dev` | `lz4` |

### Header Path Differences

The build system automatically handles platform-specific header paths:

**Linux (system packages):**
```cpp
#include <jsoncpp/json/json.h>  // JsonCpp
#include <GL/gl.h>               // OpenGL
```

**Windows (vcpkg):**
```cpp
#include <json/json.h>           // JsonCpp
#include <glad/glad.h>           // OpenGL (via GLAD)
```

**Unified approach (recommended):**
```cpp
#include "utils/PlatformCompat.h"  // Automatically includes correct headers
```

---

## Platform Compatibility Layer

### Using PlatformCompat.h

Instead of platform-specific conditional includes throughout the codebase, include the unified compatibility header:

```cpp
#include "utils/PlatformCompat.h"

// This automatically includes:
// - Correct JsonCpp headers (jsoncpp/json/json.h or json/json.h)
// - Correct OpenGL headers (GL/gl.h or glad/glad.h)
// - SDL2, OpenSSL headers
// - Platform-specific definitions (NOMINMAX, _CRT_SECURE_NO_WARNINGS)
```

### Platform Detection Macros

```cpp
#ifdef PLATFORM_WINDOWS
    // Windows-specific code
#endif

#ifdef PLATFORM_LINUX
    // Linux-specific code
#endif
```

### Cross-Platform Utilities

```cpp
// Path handling
std::string path = PlatformUtils::NormalizePath("data/config.json");
std::string fullPath = PlatformUtils::JoinPath("saves", "game1.sav");

// ImGui compatibility (works with both old and new ImGui versions)
if (ImGuiCompat::IsKeyDown(SDLK_w)) {
    // Handle W key
}

// Debug utilities
PLATFORM_ASSERT(entity != nullptr, "Entity is null");
DEBUG_OUTPUT("Debug message\n");
```

---

## CMake Platform Detection

The `CMakeLists.txt` automatically detects your platform:

```cmake
if(WIN32)
    # Windows configuration
    find_package(SDL2 CONFIG REQUIRED)
    find_package(jsoncpp CONFIG REQUIRED)
    # ... etc
else()
    # Linux configuration
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(JSONCPP REQUIRED jsoncpp)
    # ... etc
endif()
```

### Platform-Specific Compile Definitions

Automatically set based on platform:

**Windows:**
- `PLATFORM_WINDOWS`
- `NOMINMAX` (prevents `min`/`max` macro conflicts)
- `_CRT_SECURE_NO_WARNINGS` (disables MSVC security warnings)

**Linux:**
- `PLATFORM_LINUX`

---

## Known Platform Issues & Solutions

### Issue 1: JsonCpp Header Paths

**Problem:** Linux uses `jsoncpp/json/json.h`, Windows uses `json/json.h`

**Solution:** Use `PlatformCompat.h` which includes the correct header automatically.

### Issue 2: OpenGL APIENTRY Definition

**Problem:** Windows requires `Windows.h` before `gl.h` to define `APIENTRY`

**Solution:** `PlatformCompat.h` includes `Windows.h` first on Windows platforms.

### Issue 3: ImGui API Changes

**Problem:** ImGui 1.89+ deprecated `KeysDown[]` array in favor of `IsKeyDown()`

**Solution:** Use `ImGuiCompat::IsKeyDown()` wrapper that works with both versions.

### Issue 4: OpenSSL on Windows

**Problem:** SaveManager uses OpenSSL which may not be installed by default

**Solution:** vcpkg installs OpenSSL automatically. CMake links `OpenSSL::SSL` and `OpenSSL::Crypto`.

### Issue 5: Path Separators

**Problem:** Windows uses `\`, Linux uses `/`

**Solution:** Use `PlatformUtils::NormalizePath()` and `PlatformUtils::JoinPath()`.

---

## Building for Different Platforms

### Linux → Windows Cross-Compilation

Use MinGW-w64:

```bash
# Install cross-compiler
sudo apt install mingw-w64

# Configure CMake for cross-compilation
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/mingw-w64.cmake

# Build
make -j$(nproc)
```

### Windows → Linux (WSL)

Use Windows Subsystem for Linux:

```powershell
# Install WSL2
wsl --install Ubuntu-24.04

# Inside WSL, follow Linux build instructions
wsl
cd /path/to/game
mkdir build && cd build
cmake ..
make -j$(nproc)
```

---

## Continuous Integration

### GitHub Actions Matrix

```yaml
strategy:
  matrix:
    os: [ubuntu-latest, windows-latest]
    include:
      - os: ubuntu-latest
        install: sudo apt install -y libsdl2-dev libjsoncpp-dev ...
      - os: windows-latest
        install: vcpkg install sdl2 jsoncpp ...
```

---

## Testing Platform Compatibility

### Quick Compatibility Test

```bash
# Run platform detection test
./build/mechanica_imperii --platform-info

# Expected output:
# Platform: Linux
# Compiler: GCC 13.3.0
# C++ Standard: C++17
# Dependencies:
#   - SDL2: 2.30.0
#   - JsonCpp: 1.9.5
#   - ImGui: 1.90.1
#   - OpenGL: 4.6
```

### Build Verification

```bash
# Linux
./build/test_map_loading
./build/test_population_ui
./build/mechanica_imperii

# Windows
.\build\Release\test_map_loading.exe
.\build\Release\test_population_ui.exe
.\build\Release\mechanica_imperii.exe
```

---

## Troubleshooting

### CMake doesn't find dependencies (Linux)

```bash
# Update package lists
sudo apt update

# Install pkg-config
sudo apt install pkg-config

# Verify package installation
pkg-config --modversion sdl2
pkg-config --modversion jsoncpp
```

### CMake doesn't find dependencies (Windows)

```powershell
# Ensure vcpkg is integrated
.\vcpkg integrate install

# Use toolchain file
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

# List installed packages
.\vcpkg list
```

### Linker errors on Windows

Ensure you're linking the correct libraries:
- Use `SDL2::SDL2` and `SDL2::SDL2main` (not `SDL2_LIBRARIES`)
- Use `jsoncpp_lib` (not `jsoncpp`)
- Include `OpenSSL::SSL` and `OpenSSL::Crypto`

### Missing OpenGL on Linux

```bash
# Install full OpenGL development libraries
sudo apt install -y libglx-dev libopengl-dev mesa-common-dev
```

---

## Migration from Platform-Specific Code

### Old approach (platform-specific):

```cpp
#ifdef _WIN32
    #include <json/json.h>
    #include <glad/glad.h>
#else
    #include <jsoncpp/json/json.h>
    #include <GL/gl.h>
#endif

// ... scattered throughout codebase
```

### New approach (unified):

```cpp
#include "utils/PlatformCompat.h"

// No conditional includes needed!
// Just use Json::Value, OpenGL functions, etc. directly
```

---

## Future Platform Support

### macOS (Planned)

```bash
# Homebrew installation
brew install cmake sdl2 jsoncpp imgui openssl lz4

# Build
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
```

### Android/iOS (Research Phase)

Potential build system: CMake + Gradle (Android) / CMake + Xcode (iOS)

---

## Summary

✅ **Automatic platform detection** - No manual configuration needed  
✅ **Unified compatibility layer** - Include `PlatformCompat.h` once  
✅ **Cross-platform utilities** - Path handling, debug tools, assertions  
✅ **CMake handles everything** - Dependencies, linking, compile flags  
✅ **Easy to extend** - Add new platforms by updating CMakeLists.txt  

For questions or issues, see `docs/DEBUGGING-METHODOLOGY.md` or open an issue on GitHub.
