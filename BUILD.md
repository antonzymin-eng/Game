# Build Instructions - Mechanica Imperii

**Last Updated:** October 26, 2025  
**Build System:** CMake 3.15+ with presets  
**Status:** ✅ Windows build compiling successfully after major API fixes

---

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Quick Start](#quick-start)
3. [CMake Presets](#cmake-presets)
4. [Windows Build](#windows-build)
5. [Linux Build](#linux-build)
6. [Dependencies](#dependencies)
7. [Build Options](#build-options)
8. [Platform-Specific Issues](#platform-specific-issues)
9. [Troubleshooting](#troubleshooting)

---

## Prerequisites

### All Platforms
- **CMake:** 3.15 or later (3.28+ recommended)
- **C++ Compiler:** C++17 support required with C language support
  - Windows: MSVC 2019+ (Visual Studio 2022 recommended)
  - Linux: GCC 9+ or Clang 10+
- **Git:** For repository management

### Windows (vcpkg Required)
- **Visual Studio 2022** (Community Edition or higher)
  - C++ Desktop Development workload
  - CMake tools (included in VS installer)
- **vcpkg:** Dependency manager (REQUIRED for Windows)
  - Install location: `C:/vcpkg` (recommended)
  - Bootstrap vcpkg: `.\vcpkg\bootstrap-vcpkg.bat`
  - Set environment variable: `VCPKG_ROOT=C:\vcpkg`

### Linux (System Packages)
- **Build tools:** `sudo apt install build-essential cmake pkg-config ninja-build`
- **System libraries:** SDL2, jsoncpp, OpenSSL, Mesa OpenGL, lz4
- **vcpkg:** ❌ NOT REQUIRED (uses system packages + FetchContent)

---

## Quick Start

### Windows (Ninja - Fastest)

```powershell
# 1. Set vcpkg environment variable (one-time)
$env:VCPKG_ROOT = "C:\vcpkg"
# Add to system environment for persistence

# 2. Clone repository
git clone <repo-url>
cd Game

# 3. Configure with preset
cmake --preset windows-release

# 4. Build
cmake --build --preset windows-release

# 5. Run
.\build\windows-release\bin\mechanica_imperii.exe
```

### Windows (Visual Studio - IDE Integration)

```powershell
# 1. Set vcpkg environment variable
$env:VCPKG_ROOT = "C:\vcpkg"

# 2. Configure with Visual Studio preset
cmake --preset windows-vs-release

# 3. Build
cmake --build --preset windows-vs-release

# 4. Run
.\build\windows-vs-release\bin\mechanica_imperii.exe

# Or open in Visual Studio:
# build\windows-vs-release\mechanica_imperii.sln
```

### Linux (Codespaces / dev container)

```bash
# 1. Install system dependencies
sudo apt install -y build-essential cmake ninja-build pkg-config \
    libsdl2-dev libgl1-mesa-dev libjsoncpp-dev libssl-dev liblz4-dev

# 2. Clone repository (if not in Codespaces)
git clone <repo-url>
cd Game

# 3. Configure (no vcpkg needed!)
cmake --preset linux-release

# 4. Build
cmake --build --preset linux-release

# 5. Run (if X11 display available)
./build/linux-release/bin/mechanica_imperii
```

**Note:** glad and ImGui will be auto-fetched if needed - no vcpkg required!

---

## ⚠️ Important: Reconfiguring After Updates

**If you've pulled recent changes to `CMakeLists.txt`, you MUST reconfigure:**

### Windows Reconfiguration

```powershell
# Delete old build configuration
Remove-Item -Recurse -Force build\windows-release  # or windows-vs-release

# Reconfigure with your preset
cmake --preset windows-release
# Or for Visual Studio:
cmake --preset windows-vs-release

# Build
cmake --build --preset windows-release
```

### Linux Reconfiguration

```bash
# Delete old build configuration
rm -rf build/linux-release

# Reconfigure
cmake --preset linux-release

# Build
cmake --build --preset linux-release
```

### Recent Changes (October 22, 2025)
- Added C language support (required for glad and lz4)
- Added FetchContent fallback for glad on Linux
- Improved ImGui linking with SDL2 and OpenGL
- Flexible GLAD_LIBRARIES variable for cross-platform builds

---

## CMake Presets

### Available Presets

| Preset | Platform | Generator | Build Type | Use Case |
|--------|----------|-----------|------------|----------|
| `windows-debug` | Windows | Ninja | Debug | Fast debug builds |
| `windows-release` | Windows | Ninja | Release | Fast release builds |
| `windows-vs-debug` | Windows | Visual Studio | Debug | IDE integration (debugging) |
| `windows-vs-release` | Windows | Visual Studio | Release | IDE integration (release) |
| `dev` | Windows | Ninja | Debug | Quick iteration (tests ON) |
| `linux-debug` | Linux | Ninja | Debug | Linux debug builds |
| `linux-release` | Linux | Ninja | Release | Linux release builds |

### Preset Selection Guide

**For daily development:**
- Windows: `windows-debug` (Ninja) - fastest iteration
- Linux: `linux-debug`

**For IDE users:**
- Windows: `windows-vs-debug` - open `.sln` in Visual Studio

**For testing:**
- Any platform: `dev` preset (enables `BUILD_TESTS=ON`)

**For final builds:**
- Windows: `windows-release` or `windows-vs-release`
- Linux: `linux-release`

### Preset Usage

```bash
# Configure
cmake --preset <preset-name>

# Build
cmake --build --preset <preset-name>

# Clean rebuild
cmake --build --preset <preset-name> --clean-first

# Parallel build (faster)
cmake --build --preset <preset-name> -j 8
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

# Set environment variable (PowerShell - current session)
$env:VCPKG_ROOT = "C:\vcpkg"

# Set environment variable (persistent - System Properties)
# Control Panel → System → Advanced → Environment Variables
# Add User Variable: VCPKG_ROOT = C:\vcpkg
```

### 2. Install Dependencies

vcpkg will automatically install dependencies via the `vcpkg.json` manifest when you configure CMake. No manual installation needed!

**To verify dependencies are available:**
```powershell
cd C:\vcpkg
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

### 3. Configure with Preset

```powershell
cd \path\to\Game

# Option A: Ninja (fastest builds)
cmake --preset windows-release

# Option B: Visual Studio (IDE integration)
cmake --preset windows-vs-release

# Option C: Development (with tests)
cmake --preset dev
```

### 4. Build

```powershell
# Build with preset
cmake --build --preset windows-release

# Or specify configuration explicitly
cmake --build --preset windows-release --config Release

# Parallel build (8 threads)
cmake --build --preset windows-release -j 8
```

### 5. Run

```powershell
# From project root
.\build\windows-release\bin\mechanica_imperii.exe

# Or navigate to bin directory
cd build\windows-release\bin
.\mechanica_imperii.exe
```

---

## Linux Build

### 1. Install System Dependencies

**✅ Linux builds NO LONGER require vcpkg!** The build system uses system packages with automatic FetchContent fallbacks for missing libraries.

#### Ubuntu/Debian (Codespaces default)

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    libsdl2-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libjsoncpp-dev \
    libssl-dev \
    liblz4-dev
```

**Automatic FetchContent Fallbacks:**
- **glad:** Auto-generated via Python if not found (OpenGL 3.3 core)
- **ImGui:** Auto-fetched from GitHub if not available via pkg-config
- **lz4:** Auto-fetched if not found system-wide

No manual dependency management needed!

#### Fedora/RHEL

```bash
sudo dnf install -y \
    gcc-c++ \
    cmake \
    ninja-build \
    pkg-config \
    SDL2-devel \
    mesa-libGL-devel \
    mesa-libGLU-devel \
    jsoncpp-devel \
    openssl-devel \
    lz4-devel
```

### 2. Set vcpkg Environment

```bash
# If using vcpkg (recommended for consistency)
export VCPKG_ROOT="/path/to/vcpkg"

# Add to ~/.bashrc for persistence
echo 'export VCPKG_ROOT="/path/to/vcpkg"' >> ~/.bashrc
```

### 3. Configure with Preset

```bash
cd /path/to/Game

# Release build (optimized)
cmake --preset linux-release

# Debug build (with symbols)
cmake --preset linux-debug
```

### 4. Build

```bash
# Parallel build (uses all CPU cores)
cmake --build --preset linux-release -j$(nproc)

# Or single-threaded
cmake --build --preset linux-release

# Verbose output for debugging
cmake --build --preset linux-release --verbose
```

### 5. Run

```bash
# From project root
./build/linux-release/bin/mechanica_imperii

# Or navigate to bin directory
cd build/linux-release/bin
./mechanica_imperii
```

---

## Dependencies

### Core Libraries

| Library | Version | Purpose | vcpkg Package |
|---------|---------|---------|---------------|
| SDL2 | 2.0+ | Window management, input | `sdl2` |
| OpenGL | 3.2+ | Graphics rendering | System |
| GLAD | Latest | OpenGL loader | `glad` |
| jsoncpp | 1.9+ | JSON parsing | `jsoncpp` |
| OpenSSL | 3.0+ | Cryptography (save checksums) | `openssl` |
| LZ4 | 1.9+ | Save compression | `lz4` |
| ImGui | 1.89+ | UI framework | `imgui` |

### Dependency Management

**Automatic via vcpkg.json:**
- Windows: vcpkg installs dependencies during CMake configure
- Linux: vcpkg or system packages (pkg-config fallback)

**Manual LZ4 (optional):**
- If LZ4 not found, CMake auto-fetches via FetchContent
- Disable with: `cmake --preset <name> -DUSE_VENDOR_LZ4=OFF`

---

## Build Options

### CMake Cache Variables

```bash
# Enable tests
cmake --preset windows-release -DBUILD_TESTS=ON

# Enable documentation generation
cmake --preset windows-release -DBUILD_DOCS=ON

# Disable vendored LZ4
cmake --preset windows-release -DUSE_VENDOR_LZ4=OFF
```

### Available Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_TESTS` | OFF | Build test executables |
| `BUILD_DOCS` | OFF | Generate Doxygen documentation |
| `USE_VENDOR_LZ4` | ON | Fetch LZ4 if not found system-wide |

---

## Platform-Specific Issues

### Windows

#### Windows.h Macro Pollution

**Problem:** Windows.h defines macros (`INVALID`, `ERROR`, `min`, `max`) that conflict with C++ code.

**Solution:** `WindowsCleanup.h` is automatically force-included via CMake `/FI` flag.

**After CMakeLists.txt changes, always reconfigure:**
```powershell
# Delete cache to ensure changes apply
Remove-Item -Recurse -Force build\windows-release\CMakeCache.txt

# Reconfigure
cmake --preset windows-release
```

#### MSVC-Specific Warnings

Disabled warnings (customizable in CMakeLists.txt):
```cmake
target_compile_definitions(mechanica_imperii PRIVATE
    _CRT_SECURE_NO_WARNINGS  # Disable sprintf warnings
)
```

### Linux

#### Missing ImGui Headers

**Problem:** Some distributions don't package ImGui.

**Solution:** CMake automatically fetches ImGui via FetchContent if pkg-config fails.

**Manual verification:**
```bash
pkg-config --cflags imgui  # Check if available
```

#### OpenGL Context Issues

**Problem:** Running in Codespaces without X11 forwarding.

**Workaround:**
```bash
# Enable X11 forwarding (if supported)
export DISPLAY=:0

# Or use headless mode (future feature)
./build/linux-release/bin/mechanica_imperii --headless
```

---

## Troubleshooting

### vcpkg Not Found

**Symptom:** CMake error `VCPKG_ROOT environment variable not set`

**Solution:**
```powershell
# Windows (PowerShell)
$env:VCPKG_ROOT = "C:\vcpkg"
[Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\vcpkg", "User")

# Linux (Bash)
export VCPKG_ROOT="/path/to/vcpkg"
echo 'export VCPKG_ROOT="/path/to/vcpkg"' >> ~/.bashrc
```

### Preset Not Found

**Symptom:** CMake error `Could not find preset "<name>"`

**Solution:**
1. Verify `CMakePresets.json` exists in project root
2. Check preset name spelling
3. Ensure you're in the project root directory

### Build Fails After CMakeLists.txt Changes

**Symptom:** Linker errors or missing targets after updating CMakeLists.txt

**Solution:**
```bash
# Clean cache and reconfigure
cmake --preset <name> --fresh

# Or delete build directory
rm -rf build/<preset-name>
cmake --preset <name>
```

### Missing DLL Files (Windows Runtime)

**Symptom:** `SDL2.dll not found` when running executable

**Solution:** vcpkg automatically handles DLLs when using presets. If issues persist:

```powershell
# Copy DLLs manually (rarely needed)
Copy-Item $env:VCPKG_ROOT\installed\x64-windows\bin\*.dll build\windows-release\bin\
```

### Compilation Hangs

**Symptom:** Build process stops responding

**Solution:** Reduce parallel jobs:
```bash
# Linux: Reduce to 2 jobs
cmake --build --preset linux-release -j2

# Windows: Use fewer threads
cmake --build --preset windows-release -- /maxcpucount:2
```

### Undefined References (Linux)

**Symptom:** Linker errors like `undefined reference to 'SDL_Init'`

**Solution:**
```bash
# Verify libraries are installed
pkg-config --libs sdl2 jsoncpp openssl lz4

# Reinstall if missing
sudo apt install libsdl2-dev libjsoncpp-dev libssl-dev liblz4-dev
```

---

## Output Structure

```
build/<preset-name>/
├── bin/
│   ├── mechanica_imperii(.exe)     # Main executable
│   ├── data/
│   │   └── GameConfig.json         # Runtime configuration
│   └── shaders/
│       └── *.glsl                  # GLSL shaders
├── CMakeFiles/                     # CMake internals
└── Testing/                        # Test results (if BUILD_TESTS=ON)
```

---

## Build Performance

### Compilation Time

Typical build times (Release, parallel):
- **Windows (MSVC, Ninja):** 2-4 minutes (8-core CPU)
- **Windows (Visual Studio):** 4-6 minutes (8-core CPU)
- **Linux (GCC, Ninja):** 2-3 minutes (8-core CPU)

**Tips for faster builds:**
- Use Ninja generator (faster than Visual Studio)
- Enable parallel builds: `-j$(nproc)` or `-j8`
- Use precompiled headers (future optimization)
- Incremental builds (only rebuild changed files)

### Build Artifacts

| File | Size | Description |
|------|------|-------------|
| `mechanica_imperii.exe` (Release) | ~15 MB | Main executable (optimized) |
| `mechanica_imperii.exe` (Debug) | ~50 MB | Debug executable (with symbols) |

---

## CI/CD Integration

### GitHub Actions (Windows)

```yaml
name: Build Windows

on: [push, pull_request]

jobs:
  build-windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Setup vcpkg
        run: |
          git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
          C:\vcpkg\bootstrap-vcpkg.bat
          echo "VCPKG_ROOT=C:\vcpkg" >> $GITHUB_ENV
      
      - name: Configure
        run: cmake --preset windows-release
      
      - name: Build
        run: cmake --build --preset windows-release
      
      - name: Test
        run: .\build\windows-release\bin\mechanica_imperii.exe --version
```

### GitHub Actions (Linux)

```yaml
name: Build Linux

on: [push, pull_request]

jobs:
  build-linux:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Install dependencies
        run: |
          sudo apt update
          sudo apt install -y libsdl2-dev libjsoncpp-dev \
              libssl-dev liblz4-dev ninja-build
      
      - name: Setup vcpkg
        run: |
          git clone https://github.com/microsoft/vcpkg.git
          ./vcpkg/bootstrap-vcpkg.sh
          echo "VCPKG_ROOT=$PWD/vcpkg" >> $GITHUB_ENV
      
      - name: Configure
        run: cmake --preset linux-release
      
      - name: Build
        run: cmake --build --preset linux-release
      
      - name: Test
        run: ./build/linux-release/bin/mechanica_imperii --version
```

---

## Recent Changes (October 26, 2025)

### ✅ Major Fixes Applied

**Problem Solved:** main.cpp had 30+ compilation errors due to API mismatches between system constructors and actual implementations.

**Changes Made:**

1. **Constructor Signature Updates (10 systems)**
   - All game systems now use `ComponentAccessManager&` instead of `EntityManager&`
   - `ThreadedSystemManager` uses pointer parameters instead of references
   - `TimeManagementSystem` now requires 3 parameters (was 0)

2. **Missing Method Implementations Added**
   - `EconomicSystem`: `GetSystemName()`, `Serialize()`, `Deserialize()`
   - `AdministrativeSystem`: `GetSystemName()`, `Serialize()`, `Deserialize()`
   - `MilitarySystem`: `GetThreadingStrategy()`
   - `MilitaryRecruitmentSystem`: All serialization + threading methods
   - `PopulationSystem`: All serialization methods

3. **CMakeLists.txt Updates**
   - Added C language support (required for LZ4)
   - Added `GAMEPLAY_SOURCES` section
   - TypeRegistry.cpp added to build (FULLY FUNCTIONAL - enum mismatches fixed)

4. **Include Fixes**
   - Added `<json/json.h>` to all system implementations
   - Fixed include paths in TypeRegistry.cpp and CoreGameplaySystem.cpp

**Current Build Status:**
- ✅ Windows: Compiles successfully (17 of 18 systems active)
- ✅ TypeRegistry: Fully functional with all enum mappings (ThreadingStrategy, SocialClass, etc.)
- ⚠️ 1 system temporarily disabled (GameplayCoordinator)
- ⚠️ Linker may still report exe locked if previous instance running

**Known Limitations:**
- GameplayCoordinator temporarily unavailable (method mismatches in header/impl)

**Next Steps:**
1. Kill any running instances: `taskkill /F /IM mechanica_imperii.exe`
2. Build: `cmake --build --preset windows-vs-release`
3. Run: `build\windows-vs-release\bin\mechanica_imperii.exe`

---

## Additional Resources

- **vcpkg Documentation:** https://vcpkg.io/
- **CMake Presets:** https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html
- **SDL2 Documentation:** https://wiki.libsdl.org/
- **ImGui Documentation:** https://github.com/ocornut/imgui/wiki
- **API Reference:** See `docs/API_REFERENCE.md` (NEW - Oct 26, 2025)

---

**For More Details:**
- Architecture: See `ARCHITECTURE.md`
- AI Context: See `AI_CONTEXT.md`
- User Documentation: See `README.md`
