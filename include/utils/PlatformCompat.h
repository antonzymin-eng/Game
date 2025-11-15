#pragma once
/**
 * @file PlatformCompat.h
 * @brief Cross-platform compatibility layer for Windows and Linux builds
 * 
 * This header provides unified includes and definitions for platform-specific
 * libraries and APIs, eliminating the need for conditional includes throughout
 * the codebase.
 * 
 * Usage: Include this header instead of platform-specific headers.
 * 
 * Created: October 21, 2025
 */

// ============================================================================
// Platform Detection
// ============================================================================

#if defined(_WIN32) || defined(_WIN64) || defined(WIN32)
    #ifndef PLATFORM_WINDOWS
        #define PLATFORM_WINDOWS
    #endif
#elif defined(__linux__)
    #ifndef PLATFORM_LINUX
        #define PLATFORM_LINUX
    #endif
#elif defined(__APPLE__)
    #ifndef PLATFORM_MACOS
        #define PLATFORM_MACOS
    #endif
#else
    #error "Unsupported platform"
#endif

// ============================================================================
// Windows-specific headers and definitions
// ============================================================================

#ifdef PLATFORM_WINDOWS
    // Windows.h is force-included via WindowsCleanup.h - DO NOT include it again!
    // WindowsCleanup.h already handles NOMINMAX and macro cleanup

    // JsonCpp: Windows uses json/json.h (vcpkg convention)
    #include <json/json.h>
    
    // OpenGL: Use GLAD on Windows
    #include <glad/glad.h>
    
    // Platform-specific utilities
    #define PATH_SEPARATOR '\\'
    #define LINE_ENDING "\r\n"
    
#endif

// ============================================================================
// Linux-specific headers and definitions
// ============================================================================

#ifdef PLATFORM_LINUX
    // JsonCpp: Try fetched version first, then system package
    #if __has_include(<json/json.h>)
        #include <json/json.h>
    #else
        #include <jsoncpp/json/json.h>
    #endif
    
    // OpenGL: Use system OpenGL headers on Linux
    #include <GL/gl.h>
    #include <GL/glext.h>
    
    // Platform-specific utilities
    #define PATH_SEPARATOR '/'
    #define LINE_ENDING "\n"
    
#endif

// ============================================================================
// Common cross-platform includes
// ============================================================================

// SDL2 (same on both platforms)
#include <SDL2/SDL.h>

// OpenSSL (same on both platforms, but may need OpenSSL::SSL on Windows)
#include <openssl/sha.h>
#include <openssl/evp.h>

// ImGui (need to include before using in compatibility functions)
#include <imgui.h>

// ============================================================================
// Platform-specific function wrappers
// ============================================================================

#ifdef PLATFORM_WINDOWS
    // Windows doesn't have strcasecmp, use _stricmp instead
    #define strcasecmp _stricmp
    #define strncasecmp _strnicmp
#endif

// ============================================================================
// Compiler-specific attributes
// ============================================================================

#ifdef PLATFORM_WINDOWS
    // MSVC-specific attributes
    #define FORCE_INLINE __forceinline
    #define NO_INLINE __declspec(noinline)
    #define DEPRECATED __declspec(deprecated)
#else
    // GCC/Clang attributes
    #define FORCE_INLINE inline __attribute__((always_inline))
    #define NO_INLINE __attribute__((noinline))
    #define DEPRECATED __attribute__((deprecated))
#endif

// ============================================================================
// ImGui compatibility
// ============================================================================

// ImGui 1.89+ deprecated KeysDown[] in favor of IsKeyDown()
// This compatibility layer works with both old and new versions
namespace ImGuiCompat {
    inline bool IsKeyPressed(int key) {
        #if IMGUI_VERSION_NUM >= 18900
            return ImGui::IsKeyPressed((ImGuiKey)key);
        #else
            return ImGui::IsKeyPressed(key);
        #endif
    }
    
    inline bool IsKeyDown(int key) {
        #if IMGUI_VERSION_NUM >= 18900
            return ImGui::IsKeyDown((ImGuiKey)key);
        #else
            return ImGui::GetIO().KeysDown[key];
        #endif
    }
}

// ============================================================================
// File path utilities
// ============================================================================

#include <string>

namespace PlatformUtils {
    /**
     * @brief Convert path separators to platform-specific format
     * @param path Input path with forward or backslashes
     * @return Path with platform-appropriate separators
     */
    inline std::string NormalizePath(const std::string& path) {
        std::string result = path;
        #ifdef PLATFORM_WINDOWS
            // Convert / to \ on Windows
            for (char& c : result) {
                if (c == '/') c = '\\';
            }
        #else
            // Convert \ to / on Linux
            for (char& c : result) {
                if (c == '\\') c = '/';
            }
        #endif
        return result;
    }
    
    /**
     * @brief Combine path components with platform-appropriate separator
     */
    inline std::string JoinPath(const std::string& base, const std::string& relative) {
        if (base.empty()) return relative;
        if (relative.empty()) return base;
        
        char lastChar = base.back();
        if (lastChar != '/' && lastChar != '\\') {
            return base + PATH_SEPARATOR + relative;
        }
        return base + relative;
    }
}

// ============================================================================
// Debug output utilities
// ============================================================================

#ifdef PLATFORM_WINDOWS
    #include <debugapi.h>
    #define DEBUG_BREAK() __debugbreak()
    #define DEBUG_OUTPUT(msg) OutputDebugStringA(msg)
#else
    #include <signal.h>
    #include <iostream>
    #define DEBUG_BREAK() raise(SIGTRAP)
    #define DEBUG_OUTPUT(msg) std::cerr << msg
#endif

// ============================================================================
// Assertions
// ============================================================================

#ifndef NDEBUG
    #define PLATFORM_ASSERT(expr, msg) \
        do { \
            if (!(expr)) { \
                DEBUG_OUTPUT("Assertion failed: " msg "\n"); \
                DEBUG_BREAK(); \
            } \
        } while(0)
#else
    #define PLATFORM_ASSERT(expr, msg) ((void)0)
#endif
