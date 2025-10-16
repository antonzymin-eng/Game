// Created: January 16, 2025 - 15:15:00
// Location: include/core/save/IncrementalSaveTracker.h
// Mechanica Imperii - Incremental Save System (C++17 Compliant)

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <functional>
#include <atomic>

namespace Json { class Value; }

namespace core::save {

// Forward declarations
template<typename T> class Expected;
enum class SaveError;

// ============================================================================
// System State Tracking
// ============================================================================

enum class SystemDirtyFlag : uint8_t {
    CLEAN = 0,              // No changes since last save
    DIRTY = 1,              // Has unsaved changes
    CRITICAL = 2,           // Critical changes requiring immediate save
    PARTIALLY_DIRTY = 3     // Some components changed, not all
};

struct SystemStateSnapshot {
    std::string system_name;
    SystemDirtyFlag dirty_flag = SystemDirtyFlag::CLEAN;
    size_t change_count = 0;
    size_t estimated_size = 0;  // Estimated serialized size in bytes
    std::chrono::system_clock::time_point last_modified;
    std::chrono::system_clock::time_point last_saved;
    uint64_t content_hash = 0;  // Fast hash of system state
    
    bool IsDirty() const { return dirty_flag != SystemDirtyFlag::CLEAN; }
    bool IsCritical() const { return dirty_flag == SystemDirtyFlag::CRITICAL; }
    std::chrono::seconds TimeSinceLastSave() const;
};

// ============================================================================
// Change Tracking Configuration
// ============================================================================

struct IncrementalSaveConfig {
    // Auto-save triggers
    bool enable_auto_save = true;
    size_t dirty_system_threshold = 3;      // Save after N systems dirty
    std::chrono::seconds dirty_time_threshold{300};  // Save after 5 min
    size_t estimated_size_threshold = 1024 * 1024;   // 1MB changes
    
    // Critical change handling
    bool immediate_save_on_critical = true;
    std::chrono::milliseconds critical_save_delay{5000};  // 5 sec buffer
    
    // Performance settings
    bool enable_partial_saves = true;       // Save only dirty systems
    bool enable_delta_encoding = false;     // Store only changes (future)
    size_t max_concurrent_trackers = 100;
    
    // Hashing configuration
    bool enable_content_hashing = true;
    bool fast_hash_mode = true;             // Use faster, simpler hash
};

// ============================================================================
// Change Event System
// ============================================================================

struct SystemChangeEvent {
    enum class EventType {
        MODIFIED,       // General modification
        CREATED,        // New entity created
        DELETED,        // Entity deleted
        BULK_UPDATE,    // Multiple entities updated
        STATE_RESET     // System state reset
    };
    
    std::string system_name;
    EventType type = EventType::MODIFIED;
    size_t affected_entities = 1;
    size_t estimated_size_delta = 0;
    std::chrono::system_clock::time_point timestamp;
    std::string description;
    
    SystemChangeEvent(const std::string& name, EventType t = EventType::MODIFIED);
};

using ChangeEventCallback = std::function<void(const SystemChangeEvent&)>;

// ============================================================================
// Incremental Save Tracker
// ============================================================================

class IncrementalSaveTracker {
public:
    explicit IncrementalSaveTracker(const IncrementalSaveConfig& config = {});
    ~IncrementalSaveTracker();
    
    // System registration
    void RegisterSystem(const std::string& system_name, size_t estimated_size = 0);
    void UnregisterSystem(const std::string& system_name);
    bool IsSystemRegistered(const std::string& system_name) const;
    
    // Change tracking
    void MarkDirty(const std::string& system_name, 
                   SystemDirtyFlag flag = SystemDirtyFlag::DIRTY);
    void MarkClean(const std::string& system_name);
    void MarkAllClean();
    
    // Batch operations
    void MarkMultipleDirty(const std::vector<std::string>& system_names,
                          SystemDirtyFlag flag = SystemDirtyFlag::DIRTY);
    void MarkMultipleClean(const std::vector<std::string>& system_names);
    
    // Query operations
    bool IsDirty(const std::string& system_name) const;
    bool HasDirtySystems() const;
    bool HasCriticalChanges() const;
    size_t GetDirtySystemCount() const;
    
    std::vector<std::string> GetDirtySystems() const;
    std::vector<std::string> GetCriticalSystems() const;
    std::vector<std::string> GetCleanSystems() const;
    
    SystemStateSnapshot GetSystemSnapshot(const std::string& system_name) const;
    std::vector<SystemStateSnapshot> GetAllSnapshots() const;
    
    // Auto-save trigger evaluation
    bool ShouldTriggerAutoSave() const;
    bool ShouldTriggerImmediateSave() const;
    std::string GetAutoSaveTriggerReason() const;
    
    // Change event tracking
    void RecordChangeEvent(const SystemChangeEvent& event);
    void SetChangeEventCallback(ChangeEventCallback callback);
    std::vector<SystemChangeEvent> GetRecentEvents(size_t count = 100) const;
    void ClearEventHistory();
    
    // Content hashing
    void UpdateContentHash(const std::string& system_name, const void* data, size_t size);
    void UpdateContentHashFromJSON(const std::string& system_name, const Json::Value& data);
    bool HasContentChanged(const std::string& system_name, const void* data, size_t size) const;
    
    // Statistics and reporting
    struct Statistics {
        size_t total_systems = 0;
        size_t dirty_systems = 0;
        size_t clean_systems = 0;
        size_t critical_systems = 0;
        size_t total_changes_tracked = 0;
        size_t auto_save_triggers = 0;
        size_t immediate_save_triggers = 0;
        std::chrono::system_clock::time_point last_save_time;
        std::chrono::milliseconds time_since_last_save{0};
        size_t estimated_dirty_size = 0;
        
        std::string GenerateReport() const;
        Json::Value ToJson() const;
    };
    
    Statistics GetStatistics() const;
    void ResetStatistics();
    
    // Configuration management
    void UpdateConfig(const IncrementalSaveConfig& config);
    IncrementalSaveConfig GetConfig() const;
    
    // Optimization hints
    struct SaveOptimizationHints {
        bool recommend_full_save = false;
        bool recommend_incremental_save = true;
        std::vector<std::string> priority_systems;  // Save these first
        std::vector<std::string> deferrable_systems; // Can wait
        size_t estimated_save_time_ms = 0;
        std::string recommendation_reason;
    };
    
    SaveOptimizationHints GetSaveOptimizationHints() const;
    
private:
    IncrementalSaveConfig m_config;
    
    // System state tracking
    mutable std::shared_mutex m_state_mutex;
    std::unordered_map<std::string, SystemStateSnapshot> m_system_states;
    
    // Event tracking
    mutable std::mutex m_event_mutex;
    std::vector<SystemChangeEvent> m_change_events;
    size_t m_max_event_history = 1000;
    ChangeEventCallback m_change_callback;
    
    // Statistics
    mutable std::mutex m_stats_mutex;
    Statistics m_stats;
    
    // Helper methods
    uint64_t CalculateHash(const void* data, size_t size) const;
    uint64_t CalculateFastHash(const void* data, size_t size) const;
    void UpdateStatistics();
    void TrimEventHistory();
};

// ============================================================================
// Incremental Save Manager Integration
// ============================================================================

class IncrementalSaveManager {
public:
    explicit IncrementalSaveManager(IncrementalSaveTracker& tracker);
    ~IncrementalSaveManager() = default;
    
    // Incremental save operations
    Expected<std::vector<std::string>> PerformIncrementalSave(
        std::function<Expected<bool>(const std::vector<std::string>&)> save_function);
    
    Expected<std::vector<std::string>> PerformSmartSave(
        std::function<Expected<bool>(const std::vector<std::string>&)> save_full,
        std::function<Expected<bool>(const std::vector<std::string>&)> save_incremental);
    
    // Save strategy determination
    enum class SaveStrategy {
        FULL_SAVE,          // Save all systems
        INCREMENTAL_SAVE,   // Save only dirty systems
        PRIORITY_SAVE,      // Save critical systems first
        DEFERRED_SAVE,      // Delay non-critical systems
        NO_SAVE_NEEDED      // Nothing to save
    };
    
    SaveStrategy DetermineSaveStrategy() const;
    std::string GetStrategyDescription(SaveStrategy strategy) const;
    
    // Auto-save scheduling
    struct AutoSaveSchedule {
        bool should_save_now = false;
        SaveStrategy recommended_strategy = SaveStrategy::NO_SAVE_NEEDED;
        std::chrono::milliseconds estimated_duration{0};
        std::vector<std::string> systems_to_save;
        std::string trigger_reason;
    };
    
    AutoSaveSchedule EvaluateAutoSaveSchedule() const;
    
    // Performance tracking
    struct PerformanceMetrics {
        size_t incremental_saves = 0;
        size_t full_saves = 0;
        std::chrono::milliseconds total_incremental_time{0};
        std::chrono::milliseconds total_full_time{0};
        size_t bytes_saved_incremental = 0;
        size_t bytes_saved_full = 0;
        
        double GetIncrementalSpeedupRatio() const;
        double GetIncrementalSizeRatio() const;
        std::string GenerateReport() const;
    };
    
    PerformanceMetrics GetPerformanceMetrics() const;
    void ResetPerformanceMetrics();
 
private:
    // ========================================================================
    // LOCK ORDERING CONVENTION (CRITICAL FOR DEADLOCK PREVENTION)
    // ========================================================================
    // When acquiring multiple locks, ALWAYS acquire in this order:
    //   1. m_state_mutex (system state tracking)
    //   2. m_stats_mutex (statistics)
    //   3. m_event_mutex (event history)
    //
    // NEVER acquire locks in reverse order or skip levels.
    // Current code follows this ordering. Maintain when adding features.
    // ========================================================================
    
    // System state tracking (LOCK LEVEL 1)
    mutable std::shared_mutex m_state_mutex;
    // ... rest of members
   
private:
    IncrementalSaveTracker& m_tracker;
    mutable std::mutex m_perf_mutex;
    PerformanceMetrics m_performance;
    
    void RecordIncrementalSave(size_t system_count, 
                               std::chrono::milliseconds duration,
                               size_t bytes_saved);
    void RecordFullSave(std::chrono::milliseconds duration, size_t bytes_saved);
};

// ============================================================================
// Dirty System Filter
// ============================================================================

class DirtySystemFilter {
public:
    explicit DirtySystemFilter(const IncrementalSaveTracker& tracker);
    
    // Filter systems for serialization
    std::vector<std::string> FilterDirtySystems(
        const std::vector<std::string>& all_systems) const;
    
    std::vector<std::string> FilterCleanSystems(
        const std::vector<std::string>& all_systems) const;
    
    std::vector<std::string> FilterCriticalSystems(
        const std::vector<std::string>& all_systems) const;
    
    // Priority sorting
    std::vector<std::string> SortByPriority(
        const std::vector<std::string>& systems) const;
    
    std::vector<std::string> SortBySize(
        const std::vector<std::string>& systems,
        bool largest_first = false) const;
    
private:
    const IncrementalSaveTracker& m_tracker;
};

} // namespace core::save
