// ============================================================================
// Constants.h - Global constants for the Mechanica Imperii engine
// Location: include/core/Constants.h
//
// This file contains named constants to replace magic numbers throughout
// the codebase. Constants are organized by subsystem for clarity.
// ============================================================================

#pragma once

#include <cstddef>
#include <cstdint>

namespace core::constants {

    // ========================================================================
    // Save System Constants
    // ========================================================================

    /// Safety margin when checking available disk space (in bytes)
    /// Applied before save operations to ensure sufficient space
    constexpr size_t DISK_SPACE_SAFETY_MARGIN_BYTES = 50ULL * 1024ULL * 1024ULL; // 50 MB

    /// Maximum size for save file validation
    constexpr size_t MAX_SAVE_FILE_SIZE_BYTES = 2ULL * 1024ULL * 1024ULL * 1024ULL; // 2 GB

    // ========================================================================
    // Performance Monitoring Constants
    // ========================================================================

    /// Number of samples to use for exponential moving average calculation
    /// Higher values = smoother but less responsive to changes
    constexpr double PERFORMANCE_SAMPLE_WINDOW = 100.0;

    /// Number of frames to smooth over for frame time averaging
    constexpr double FRAME_TIME_SAMPLE_WINDOW = 60.0;

    /// Threshold for identifying slow systems (in milliseconds)
    /// Systems exceeding this should consider dedicated threading
    constexpr double SLOW_SYSTEM_THRESHOLD_MS = 8.0;

    /// Minimum number of executions before making threading decisions
    constexpr uint64_t MIN_EXECUTIONS_FOR_THREADING = 10;

    // ========================================================================
    // Threading Constants
    // ========================================================================

    /// Default target frames per second
    constexpr double DEFAULT_TARGET_FPS = 60.0;

    /// Frame budget in milliseconds (derived from target FPS)
    constexpr double DEFAULT_FRAME_BUDGET_MS = 1000.0 / DEFAULT_TARGET_FPS; // ~16.67ms

    /// Fallback thread count if hardware_concurrency() returns 0
    constexpr size_t FALLBACK_THREAD_COUNT = 4;

    /// Maximum thread pool size
    constexpr size_t MAX_THREAD_POOL_SIZE = 32;

    /// Thread sleep duration when system is disabled (in milliseconds)
    constexpr uint32_t DISABLED_SYSTEM_SLEEP_MS = 100;

    /// Peak execution time threshold for system promotion (in milliseconds)
    constexpr double PEAK_EXECUTION_PROMOTION_THRESHOLD_MS = 25.0;

    /// Average execution time threshold for system demotion (in milliseconds)
    constexpr double AVG_EXECUTION_DEMOTION_THRESHOLD_MS = 4.0;

    /// Frame count threshold for system promotion (3 seconds at 60fps)
    constexpr uint64_t PROMOTION_FRAME_THRESHOLD = 180;

    /// Frame count threshold for system demotion (10 seconds at 60fps)
    constexpr uint64_t DEMOTION_FRAME_THRESHOLD = 600;

    /// Frame count for load balancing checks (5 seconds at 60fps)
    constexpr uint64_t LOAD_BALANCE_CHECK_FRAMES = 300;

    // ========================================================================
    // Error Handling Constants
    // ========================================================================

    /// Maximum number of errors before disabling a system
    constexpr size_t MAX_SYSTEM_ERRORS = 5;

    /// Time window for counting system errors (in seconds)
    constexpr uint32_t ERROR_COUNT_WINDOW_SECONDS = 30;

    // ========================================================================
    // Save System Additional Constants
    // ========================================================================

    /// Low disk space warning threshold (in bytes)
    constexpr size_t LOW_DISK_SPACE_WARNING_BYTES = 100ULL * 1024ULL * 1024ULL; // 100 MB

    /// Version number multiplier for major version
    constexpr int VERSION_MAJOR_MULTIPLIER = 10000;

    /// Version number multiplier for minor version
    constexpr int VERSION_MINOR_MULTIPLIER = 100;

    // ========================================================================
    // Memory Constants
    // ========================================================================

    /// Size units for readability
    constexpr size_t BYTES_PER_KB = 1024;
    constexpr size_t BYTES_PER_MB = 1024 * BYTES_PER_KB;
    constexpr size_t BYTES_PER_GB = 1024 * BYTES_PER_MB;

    // ========================================================================
    // Time Constants
    // ========================================================================

    /// Milliseconds per second
    constexpr double MS_PER_SECOND = 1000.0;

    /// Seconds per minute
    constexpr uint32_t SECONDS_PER_MINUTE = 60;

    /// Milliseconds per minute
    constexpr double MS_PER_MINUTE = MS_PER_SECOND * SECONDS_PER_MINUTE;

} // namespace core::constants
