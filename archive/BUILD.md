# Build Instructions - Mechanica Imperii

**Last Updated:** October 22, 2025

---

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Quick Start](#quick-start)
3. [Windows Build](#windows-build)
4. [Linux Build](#linux-build)
5. [Dependencies](#dependencies)
6. [CMake Configuration](#cmake-configuration)
7. [Platform-Specific Issues](#platform-specific-issues)
8. [Troubleshooting](#troubleshooting)

---

## Prerequisites

### All Platforms
- **CMake:** 3.15 or later (3.28+ recommended)
- **C++ Compiler:** C++17 support required
  - Windows: MSVC 2019+ (Visual Studio 2022 recommended)
  - Linux: GCC 9+ or Clang 10+
- **Git:** For repository management

### Windows
- **Visual Studio 2022** (Community Edition or higher)
  - C++ Desktop Development workload
  - CMake tools (included in VS installer)
- **vcpkg:** Dependency manager
  - Install location: `C:/vcpkg` (recommended)
  - Bootstrap vcpkg: `.\vcpkg\bootstrap-vcpkg.bat`

### Linux
- **Build tools:** `sudo apt install build-essential cmake pkg-config`
- **System libraries:** SDL2, OpenGL, jsoncpp, ImGui (via apt)

---

## Quick Start

### Windows (vcpkg)

```powershell
# 1. Install dependencies via vcpkg
cd C:\vcpkg
.\vcpkg install sdl2 glad jsoncpp openssl lz4 imgui --triplet x64-windows

# 2. Clone repository
git clone <repo-url>
cd Game

# 3. Configure CMake
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

# 4. Build
cmake --build . --config Release

# 5. Run
.\Release\mechanica_imperii.exe
```

### Linux (Codespaces / dev container)

```bash
# 1. Install system dependencies
sudo apt update
sudo apt install -y libsdl2-dev libgl1-mesa-dev libglu1-mesa-dev \
    libjsoncpp-dev libssl-dev liblz4-dev libimgui-dev

# 2. Clone repository (if not already in Codespaces)
git clone <repo-url>
cd Game

# 3. Configure and build
mkdir -p build
cd build
cmake ..
make mechanica_imperii -j$(nproc)

# 4. Run (if X11 display available)
./mechanica_imperii
```

---

## Windows Build

### 1. Install vcpkg

If not already installed:

```powershell
# Clone vcpkg repository
cd C:\
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg

# Bootstrap vcpkg
.\bootstrap-vcpkg.bat

# Add to PATH (optional but recommended)
[Environment]::SetEnvironmentVariable("Path", $env:Path + ";C:\vcpkg", "User")
```

### 2. Install Dependencies

```powershell
cd C:\vcpkg

# Install all dependencies at once
.\vcpkg install sdl2 glad jsoncpp openssl lz4 imgui --triplet x64-windows

# Verify installation
.\vcpkg list
```

**Expected output:**
```
glad:x64-windows
imgui:x64-windows
jsoncpp:x64-windows
lz4:x64-windows
openssl:x64-windows
sdl2:x64-windows
```

### 3. Configure CMake

```powershell
cd \path\to\Game
mkdir build
cd build

# Configure with vcpkg toolchain
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

**Optional CMake flags:**
- `-DBUILD_TESTS=ON` - Build test executables (default: OFF)
- `-DCMAKE_BUILD_TYPE=Debug` - Debug build (default: Release)

### 4. Build

```powershell
# Build Release configuration
cmake --build . --config Release

# Or build Debug configuration
cmake --build . --config Debug

# Build specific target
cmake --build . --config Release --target mechanica_imperii
```

### 5. Run

```powershell
# From build directory
.\Release\mechanica_imperii.exe

# Or with full path
cd Release
.\mechanica_imperii.exe
```

---

## Linux Build

### 1. Install System Dependencies

#### Ubuntu/Debian (Codespaces default)

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    pkg-config \
    libsdl2-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libjsoncpp-dev \
    libssl-dev \
    liblz4-dev \
    libimgui-dev
```

#### Fedora/RHEL

```bash
sudo dnf install -y \
    gcc-c++ \
    cmake \
    pkg-config \
    SDL2-devel \
    mesa-libGL-devel \
    mesa-libGLU-devel \
    jsoncpp-devel \
    openssl-devel \
    lz4-devel \
    imgui-devel
```

### 2. Configure CMake

```bash
cd /path/to/Game
mkdir -p build
cd build

# Configure (pkg-config finds system libraries automatically)
cmake ..

# Or specify build type explicitly
cmake .. -DCMAKE_BUILD_TYPE=Release
```

### 3. Build

```bash
# Parallel build (uses all CPU cores)
make mechanica_imperii -j$(nproc)

# Or single-threaded
make mechanica_imperii

# Verbose output for debugging
make mechanica_imperii VERBOSE=1
```

### 4. Run

```bash
# From build directory
./mechanica_imperii

# Or copy to bin/ (if configured)
make install
../bin/mechanica_imperii
```

---

## Dependencies

### Core Libraries

| Library | Version | Purpose | Windows (vcpkg) | Linux (apt) |
|---------|---------|---------|-----------------|-------------|
| SDL2 | 2.0+ | Window management, input | `sdl2` | `libsdl2-dev` |
| OpenGL | 3.2+ | Graphics rendering | System | `libgl1-mesa-dev` |
| GLAD | Latest | OpenGL loader | `glad` | N/A (bundled) |
| jsoncpp | 1.9+ | JSON parsing | `jsoncpp` | `libjsoncpp-dev` |
| OpenSSL | 1.1+ | Cryptography (save checksums) | `openssl` | `libssl-dev` |
| LZ4 | 1.9+ | Save compression | `lz4` | `liblz4-dev` |
| ImGui | 1.89+ | UI framework | `imgui` | `libimgui-dev` |

### Optional Libraries

| Library | Purpose | Installation |
|---------|---------|--------------|
| Doxygen | Documentation generation | `sudo apt install doxygen` |
| Valgrind | Memory debugging (Linux) | `sudo apt install valgrind` |
| gdb | Debugging (Linux) | `sudo apt install gdb` |

---

## CMake Configuration

### CMakeLists.txt Overview

```cmake
cmake_minimum_required(VERSION 3.15)
project(mechanica_imperii VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection
if(WIN32)
    # Windows: Use vcpkg packages
    find_package(SDL2 CONFIG REQUIRED)
    find_package(glad CONFIG REQUIRED)
    find_package(jsoncpp CONFIG REQUIRED)
    
    # Windows macro cleanup (force-include)
    if(MSVC)
        target_compile_options(mechanica_imperii PRIVATE 
            /FI"${CMAKE_SOURCE_DIR}/include/WindowsCleanup.h"
        )
    endif()
else()
    # Linux: Use pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(JSONCPP REQUIRED jsoncpp)
endif()

# LZ4 (optional, fetched if not found)
find_package(LZ4 QUIET)
if(NOT LZ4_FOUND)
    FetchContent_Declare(lz4 ...)
    FetchContent_MakeAvailable(lz4)
endif()
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_TESTS` | OFF | Build test executables |
| `BUILD_DOCS` | OFF | Generate Doxygen documentation |
| `USE_VENDOR_LZ4` | ON | Fetch LZ4 if not found system-wide |

**Example:**
```bash
cmake .. -DBUILD_TESTS=ON -DBUILD_DOCS=ON
```

### Output Directories

```
build/
├── mechanica_imperii(.exe)     # Main executable
├── bin/
│   └── data/
│       └── GameConfig.json     # Runtime configuration
├── CMakeFiles/                 # CMake internals
└── Testing/                    # Test results (if BUILD_TESTS=ON)
```

---

## Platform-Specific Issues

### Windows

#### Windows.h Macro Pollution

**Problem:** Windows.h defines macros (`INVALID`, `ERROR`, `min`, `max`) that conflict with C++ code.

**Solution:** Force-include `WindowsCleanup.h` before every source file.

```cmake
if(WIN32 AND MSVC)
    target_compile_options(mechanica_imperii PRIVATE 
        /FI"${CMAKE_SOURCE_DIR}/include/WindowsCleanup.h"
    )
endif()
```

**WindowsCleanup.h** includes Windows.h with `NOMINMAX` and `WIN32_LEAN_AND_MEAN`, then undefines all conflicting macros.

**After CMakeLists.txt changes, always reconfigure:**
```powershell
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

#### MSVC-Specific Warnings

Disabled warnings (can be customized):
```cmake
if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)  # Disable sprintf warnings
endif()
```

### Linux

#### Missing ImGui Headers

Some distributions don't package ImGui headers correctly.

**Workaround:** Use system headers or fetch via CMake:
```cmake
find_package(imgui QUIET)
if(NOT imgui_FOUND)
    FetchContent_Declare(imgui ...)
endif()
```

#### OpenGL Context Issues

If running in Codespaces without X11 forwarding:
```bash
# Enable X11 forwarding (if supported)
export DISPLAY=:0

# Or use headless rendering (future feature)
./mechanica_imperii --headless
```

---

## Troubleshooting

### vcpkg: Package Not Found

**Symptom:** CMake can't find a package installed via vcpkg.

**Solution:**
1. Verify installation: `.\vcpkg list`
2. Check triplet: `.\vcpkg list | findstr x64-windows`
3. Reconfigure CMake with toolchain file:
   ```powershell
   cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
   ```

### Linux: Missing pkg-config

**Symptom:** `pkg_check_modules failed` during CMake configuration.

**Solution:**
```bash
sudo apt install pkg-config
cmake .. # Re-run configure
```

### Compilation Errors: Missing Headers

**Symptom:** `fatal error: 'SDL2/SDL.h' file not found`

**Windows Solution:**
```powershell
# Reinstall SDL2 via vcpkg
cd C:\vcpkg
.\vcpkg remove sdl2:x64-windows
.\vcpkg install sdl2:x64-windows
```

**Linux Solution:**
```bash
sudo apt install libsdl2-dev
pkg-config --cflags sdl2  # Verify installation
```

### Windows Build: "syntax error: 'constant'"

**Symptom:** Errors in `game_types.h` around line 143.

**Cause:** Windows macros not cleaned up properly.

**Solution:**
1. Verify `WindowsCleanup.h` exists in `include/`
2. Reconfigure CMake (force-include may not have been applied):
   ```powershell
   cd build
   rm CMakeCache.txt  # Clear cache
   cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
   ```
3. Clean rebuild:
   ```powershell
   cmake --build . --config Release --clean-first
   ```

### Linux Build: Undefined References

**Symptom:** Linker errors like `undefined reference to 'SDL_Init'`

**Cause:** Libraries not linked properly.

**Solution:**
```bash
# Verify pkg-config can find libraries
pkg-config --libs sdl2 jsoncpp

# If missing, reinstall:
sudo apt install libsdl2-dev libjsoncpp-dev
```

### Build Hangs During Compilation

**Symptom:** Compiler stops responding.

**Cause:** Insufficient memory or too many parallel jobs.

**Solution:**
```bash
# Linux: Reduce parallel jobs
make mechanica_imperii -j2  # Instead of -j$(nproc)

# Windows: Use fewer threads
cmake --build . --config Release -- /maxcpucount:2
```

### Runtime: Missing DLL Files (Windows)

**Symptom:** `SDL2.dll not found` when running executable.

**Cause:** vcpkg DLLs not in PATH or copied to executable directory.

**Solution:**
```powershell
# Option 1: Copy DLLs to build directory
Copy-Item C:\vcpkg\installed\x64-windows\bin\*.dll .\Release\

# Option 2: Add vcpkg bin to PATH
$env:Path += ";C:\vcpkg\installed\x64-windows\bin"
```

---

## Build Performance

### Compilation Time

Typical build times (Release, parallel):
- **Windows (MSVC):** 3-5 minutes (8-core CPU)
- **Linux (GCC):** 2-3 minutes (8-core CPU)

**Tips for faster builds:**
- Use ccache (Linux): `sudo apt install ccache`
- Incremental builds (only rebuild changed files)
- Precompiled headers (future optimization)

### Build Artifacts

| File | Size | Description |
|------|------|-------------|
| `mechanica_imperii.exe` (Release) | ~15 MB | Main executable (stripped) |
| `mechanica_imperii.exe` (Debug) | ~50 MB | Debug executable (with symbols) |
| `libgame.a` | ~30 MB | Static library (if configured) |

---

## CI/CD Integration

### GitHub Actions (Linux)

```yaml
name: Build

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Install dependencies
        run: |
          sudo apt update
          sudo apt install -y libsdl2-dev libjsoncpp-dev \
              libssl-dev liblz4-dev libimgui-dev
      - name: Configure
        run: cmake -B build
      - name: Build
        run: cmake --build build -j$(nproc)
      - name: Test
        run: cd build && ctest --output-on-failure
```

### GitHub Actions (Windows)

```yaml
jobs:
  build-windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v3
      - name: Setup vcpkg
        uses: lukka/run-vcpkg@v10
        with:
          vcpkgGitCommitId: '<latest-commit>'
      - name: Configure
        run: cmake -B build -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
      - name: Build
        run: cmake --build build --config Release
```

---

## Additional Resources

- **vcpkg Documentation:** https://vcpkg.io/
- **CMake Documentation:** https://cmake.org/documentation/
- **SDL2 Documentation:** https://wiki.libsdl.org/
- **ImGui Documentation:** https://github.com/ocornut/imgui/wiki

---

**For More Details:**
- Architecture: See `ARCHITECTURE.md`
- AI Context: See `AI_CONTEXT.md`
- User Documentation: See `README.md`
