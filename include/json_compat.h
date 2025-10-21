// ============================================================================
// json_compat.h - Cross-platform JSON include compatibility
// Created: October 21, 2025
// Description: Handles differences between Linux and Windows JSON paths
// ============================================================================

#pragma once

// Windows (vcpkg) uses json/json.h
// Linux uses jsoncpp/json/json.h
#ifdef _WIN32
    #include <json/json.h>
#else
    #include <jsoncpp/json/json.h>
#endif
