#ifndef WINDOWS_CLEANUP_H
#define WINDOWS_CLEANUP_H

// This file is force-included BEFORE every source file on Windows
// It ensures Windows.h macros are cleaned up before any game headers

#ifdef _WIN32
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    
    #include <Windows.h>
    
    // Clean up all Windows macros that conflict with game code
    #undef INVALID
    #undef ERROR
    #undef DELETE
    #undef IN
    #undef OUT
    #undef ABSOLUTE
    #undef RELATIVE
    #undef DIFFERENCE
    #undef TRANSPARENT
    
    #ifdef min
    #undef min
    #endif
    
    #ifdef max
    #undef max
    #endif
#endif

#endif // WINDOWS_CLEANUP_H
