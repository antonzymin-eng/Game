// ============================================================================
// CrashHandler.h - Cross-platform crash dump collection utilities
// Created: January 18, 2026
// ============================================================================

#pragma once

#include <filesystem>
#include <string>

namespace core::diagnostics {

struct CrashHandlerConfig {
    std::filesystem::path dump_directory;
    bool capture_backtraces = true;
    bool enable_symbol_linkage = true;
};

// Initializes crash handling for the current process. Safe to call multiple times.
void InitializeCrashHandling(const CrashHandlerConfig& config);

// Returns the directory where crash dumps are written.
std::filesystem::path GetCrashDumpDirectory();

// Writes a textual diagnostic breadcrumb that is appended to the next crash dump.
void AppendCrashBreadcrumb(const std::string& message);

} // namespace core::diagnostics

