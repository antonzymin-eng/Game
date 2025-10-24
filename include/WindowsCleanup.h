// ============================================================================
// Date/Time Created: October 23, 2025 - 2:15 PM PST
// Intended Folder Location: include/WindowsCleanup.h
// WindowsCleanup.h - Macro Cleanup (Force-included before all Windows builds)
// ============================================================================

#ifndef WINDOWS_CLEANUP_H
#define WINDOWS_CLEANUP_H

// This file is force-included BEFORE every source file on Windows via CMake /FI flag
// It undefines Windows.h macros that conflict with game code WITHOUT re-including Windows.h

#ifdef _WIN32
    // Undefine ALL Windows macros that conflict with C++ identifiers
    // These are defined by Windows.h which may have been included by system headers
    
    #ifdef INVALID
    #undef INVALID
    #endif
    
    #ifdef ERROR
    #undef ERROR
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
    
    #ifdef min
    #undef min
    #endif
    
    #ifdef max
    #undef max
    #endif
    
    #ifdef CONST
    #undef CONST
    #endif
    
    #ifdef VOID
    #undef VOID
    #endif
    
    #ifdef THIS
    #undef THIS
    #endif
    
    #ifdef STRICT
    #undef STRICT
    #endif
    
    #ifdef NEAR
    #undef NEAR
    #endif
    
    #ifdef FAR
    #undef FAR
    #endif
#endif // _WIN32

#endif // WINDOWS_CLEANUP_H
