# Dependency Installation Guide - Mechanica Imperii

## Critical Issue: SDL2 Must Be Pre-Installed

### Problem

The game **requires SDL2 to be manually installed** before building. The build system does **NOT** automatically install SDL2 for you.

### Why Your Build Failed

When you tried to build on your laptop and got a window that "pops up and closes right away," one of two things happened:

1. **The game never built at all** - CMake configuration failed because SDL2 wasn't found
2. **The game built but crashes immediately on startup** - This would happen if dependencies were missing at runtime

Most likely, it's #1: The build never completed successfully.

---

## Solution: Install Dependencies Before Building

### Windows (Requires vcpkg)

#### Step 1: Install vcpkg

```powershell
# Clone vcpkg to C:\vcpkg
cd C:\
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg

# Bootstrap vcpkg
.\bootstrap-vcpkg.bat

# Set environment variable (PowerShell - current session)
$env:VCPKG_ROOT = "C:\vcpkg"

# Set environment variable (Persistent - Add to System Environment Variables)
# 1. Open: Control Panel → System → Advanced System Settings → Environment Variables
# 2. Add User Variable: VCPKG_ROOT = C:\vcpkg
# 3. Restart your terminal/IDE
```

#### Step 2: Verify vcpkg is Ready

```powershell
# Check that vcpkg is accessible
cd C:\vcpkg
.\vcpkg --version

# The game's vcpkg.json will automatically install:
# - sdl2
# - glad
# - jsoncpp
# - openssl
# - lz4
# - imgui (with sdl2-binding and opengl3-binding)
```

#### Step 3: Build the Game

```powershell
# Navigate to game directory
cd \path\to\Game

# Configure (vcpkg.json will automatically install dependencies)
cmake --preset windows-vs-release

# Build
cmake --build --preset windows-vs-release

# Run
.\build\windows-vs-release\bin\mechanica_imperii.exe
```

**Note:** The first build will take longer because vcpkg needs to compile all dependencies from source.

---

### Linux (Ubuntu/Debian)

#### Step 1: Install System Dependencies

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

**Critical:** You must install `libsdl2-dev`, not just `libsdl2-2.0-0`:
- `libsdl2-2.0-0` - Runtime library only (won't work for building)
- `libsdl2-dev` - Development headers and libraries (required for building)

#### Step 2: Verify SDL2 is Installed

```bash
# Check that SDL2 development files are available
pkg-config --modversion sdl2
pkg-config --cflags sdl2
pkg-config --libs sdl2

# Expected output:
# 2.30.0 (or similar version)
# -I/usr/include/SDL2 -D_REENTRANT
# -lSDL2
```

#### Step 3: Build the Game

```bash
# Navigate to game directory
cd /path/to/Game

# Configure
cmake --preset linux-release

# Build (using all CPU cores)
cmake --build --preset linux-release -j$(nproc)

# Run
./build/linux-release/bin/mechanica_imperii
```

---

### Linux (Fedora/RHEL)

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

# Build same as Ubuntu
cmake --preset linux-release
cmake --build --preset linux-release -j$(nproc)
```

---

## Dependency Overview

| Dependency | Windows (vcpkg) | Linux (apt) | Purpose |
|-----------|-----------------|-------------|---------|
| **SDL2** | `sdl2` | `libsdl2-dev` | ✅ Window management, input |
| **OpenGL** | System | `libgl1-mesa-dev`, `libglu1-mesa-dev` | ✅ Graphics rendering |
| **GLAD** | `glad` | Auto-fetched | OpenGL loader |
| **ImGui** | `imgui` | Auto-fetched | UI framework |
| **jsoncpp** | `jsoncpp` | `libjsoncpp-dev` | JSON parsing |
| **OpenSSL** | `openssl` | `libssl-dev` | Save checksums |
| **LZ4** | `lz4` | `liblz4-dev` | Save compression |

### Automatic vs Manual Installation

**Windows:**
- ✅ vcpkg.json automatically installs ALL dependencies when you run `cmake --preset`
- ❌ But you MUST install vcpkg itself first and set VCPKG_ROOT

**Linux:**
- ❌ SDL2, OpenGL, jsoncpp, OpenSSL, LZ4 must be installed manually via apt
- ✅ GLAD and ImGui are auto-fetched via CMake FetchContent if not found

---

## Troubleshooting

### "Could not find SDL2" on Windows

**Cause:** vcpkg not installed or VCPKG_ROOT not set

**Fix:**
```powershell
# Verify VCPKG_ROOT is set
echo $env:VCPKG_ROOT
# Should output: C:\vcpkg

# If not set:
$env:VCPKG_ROOT = "C:\vcpkg"
[Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\vcpkg", "User")

# Reconfigure
Remove-Item -Recurse -Force build\windows-vs-release
cmake --preset windows-vs-release
```

### "Package 'sdl2' not found" on Linux

**Cause:** SDL2 development package not installed

**Fix:**
```bash
# Install SDL2 development package
sudo apt install libsdl2-dev

# Verify installation
pkg-config --modversion sdl2

# Clean and reconfigure
rm -rf build/linux-release
cmake --preset linux-release
```

### "Window pops up and closes immediately"

This is NOT a missing start screen issue. Possible causes:

1. **Build never completed** - Check CMake configuration output for errors
2. **Missing config/data files** - Game expects `config/GameConfig.json` and `data/` directory
3. **Runtime crash** - Check crash dumps in `crash_dumps/` directory

**Debug steps:**
```bash
# Windows - Run from command line to see error output
cd build\windows-vs-release\bin
.\mechanica_imperii.exe

# Linux - Same approach
cd build/linux-release/bin
./mechanica_imperii

# Look for error messages before the window closes
```

### Verifying Successful Build

A successful build will:
1. CMake configures without errors
2. Compiles 137/137 files (may vary with updates)
3. Creates executable: `build/<preset>/bin/mechanica_imperii(.exe)`
4. Executable size: ~7-15 MB

```bash
# Check executable exists
ls -lh build/linux-release/bin/mechanica_imperii

# Windows
dir build\windows-vs-release\bin\mechanica_imperii.exe
```

---

## First-Time Setup Checklist

### Windows
- [ ] Install Visual Studio 2022 with C++ Desktop Development
- [ ] Install vcpkg to `C:\vcpkg`
- [ ] Run `C:\vcpkg\bootstrap-vcpkg.bat`
- [ ] Set `VCPKG_ROOT=C:\vcpkg` in System Environment Variables
- [ ] Restart terminal/IDE
- [ ] Clone game repository
- [ ] Run `cmake --preset windows-vs-release` (vcpkg auto-installs dependencies)
- [ ] Run `cmake --build --preset windows-vs-release`
- [ ] Verify executable exists

### Linux
- [ ] Run `sudo apt install build-essential cmake ninja-build pkg-config libsdl2-dev libgl1-mesa-dev libglu1-mesa-dev libjsoncpp-dev libssl-dev liblz4-dev`
- [ ] Clone game repository
- [ ] Run `cmake --preset linux-release`
- [ ] Run `cmake --build --preset linux-release -j$(nproc)`
- [ ] Verify executable exists

---

## Quick Reference

### Windows - Full Build from Scratch
```powershell
# One-time setup
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
$env:VCPKG_ROOT = "C:\vcpkg"

# Build game (every time)
cd \path\to\Game
cmake --preset windows-vs-release
cmake --build --preset windows-vs-release
.\build\windows-vs-release\bin\mechanica_imperii.exe
```

### Linux - Full Build from Scratch
```bash
# One-time setup
sudo apt update
sudo apt install -y build-essential cmake ninja-build pkg-config \
    libsdl2-dev libgl1-mesa-dev libglu1-mesa-dev \
    libjsoncpp-dev libssl-dev liblz4-dev

# Build game (every time)
cd /path/to/Game
cmake --preset linux-release
cmake --build --preset linux-release -j$(nproc)
./build/linux-release/bin/mechanica_imperii
```

---

## See Also

- **BUILD.md** - Comprehensive build documentation
- **README.md** - Project overview and game description
- **CMakeLists.txt** - Build system configuration
- **vcpkg.json** - Dependency manifest for Windows builds

---

*Last Updated: 2025-11-17*
