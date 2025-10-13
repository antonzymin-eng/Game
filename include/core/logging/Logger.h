// ============================================================================
// Logger.h - Minimal Logging Stub (for build integration)
// Created: October 11, 2025
// Location: include/core/logging/Logger.h
// ============================================================================
#pragma once
#include <string>
#include <iostream>

namespace core {
namespace logging {


inline void LogInfo(const std::string& system, const std::string& msg) {
    std::cout << "[INFO][" << system << "] " << msg << std::endl;
}

inline void LogWarning(const std::string& system, const std::string& msg) {
    std::cout << "[WARN][" << system << "] " << msg << std::endl;
}

inline void LogError(const std::string& system, const std::string& msg) {
    std::cerr << "[ERROR][" << system << "] " << msg << std::endl;
}

inline void LogDebug(const std::string& system, const std::string& msg) {
    std::cout << "[DEBUG][" << system << "] " << msg << std::endl;
}

} // namespace logging
} // namespace core
