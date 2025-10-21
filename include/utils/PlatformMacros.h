#pragma once
/**
 * @file PlatformMacros.h
 * @brief Minimal platform macro handling for core headers
 * 
 * This header ONLY handles Windows macro conflicts without pulling in
 * heavy dependencies like SDL2, ImGui, or JsonCpp. Use this in core
 * type headers to avoid circular dependencies.
 * 
 * For full platform compatibility (with library includes), use PlatformCompat.h
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
#endif

// ============================================================================
// Windows macro cleanup
// ============================================================================

#ifdef PLATFORM_WINDOWS
    // Prevent Windows.h from defining min/max macros
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    
    // Include Windows.h to get APIENTRY and other definitions
    #include <Windows.h>
    
    // Undefine problematic Windows macros that conflict with our code
    #ifdef ERROR
        #undef ERROR
    #endif
    #ifdef min
        #undef min
    #endif
    #ifdef max
        #undef max
    #endif
    #ifdef INVALID
        #undef INVALID
    #endif
    #ifdef DELETE
        #undef DELETE
    #endif
    #ifdef IN
        #undef IN
    #endif
    #ifdef OUT
        #undef OUT
    #endif
    #ifdef ABSOLUTE
        #undef ABSOLUTE
    #endif
    #ifdef RELATIVE
        #undef RELATIVE
    #endif
    #ifdef DIFFERENCE
        #undef DIFFERENCE
    #endif
    #ifdef TRANSPARENT
        #undef TRANSPARENT
    #endif
#endif
