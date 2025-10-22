// Created: January 16, 2025 - 15:30:00 (FIXED: January 17, 2025)
// Location: src/core/save/IncrementalSaveTracker.cpp
// Mechanica Imperii - Incremental Save Tracker Implementation (C++17 Compliant)
// FIXES: Bug #1 - Broken loop logic in GetSaveOptimizationHints()
// FIXES: Bug #3 - Missing CalculateHash() implementation

#include "core/save/IncrementalSaveTracker.h"
#include "core/save/SaveManager.h"
#include "utils/PlatformCompat.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace core::save {

// ============================================================================
// SystemStateSnapshot Implementation
// ============================================================================

std::chrono::seconds SystemStateSnapshot::TimeSinceLastSave() const {
    if (last_saved.time_since_epoch().count() == 0) {
        return std::chrono::seconds::max();
    }
    
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now - last_saved);
}

// ============================================================================
// SystemChangeEvent Implementation
// ============================================================================

SystemChangeEvent::SystemChangeEvent(const std::string& name, EventType t)
    : system_name(name), type(t), timestamp(std::chrono::system_clock::now()) {}

// ============================================================================
// IncrementalSaveTracker Implementation
// ============================================================================

IncrementalSaveTracker::IncrementalSaveTracker(const IncrementalSaveConfig& config)
    : m_config(config) {
    m_change_events.reserve(m_max_event_history);
}

IncrementalSaveTracker::~IncrementalSaveTracker() = default;

// ============================================================================
// System Registration
// ============================================================================

void IncrementalSaveTracker::RegisterSystem(const std::string& system_name, size_t estimated_size) {
    std::unique_lock lock(m_state_mutex);
    
    if (m_system_states.find(system_name) != m_system_states.end()) {
        return;  // Already registered
    }
    
    SystemStateSnapshot snapshot;
    snapshot.system_name = system_name;
    snapshot.dirty_flag = SystemDirtyFlag::CLEAN;
    snapshot.estimated_size = estimated_size;
    snapshot.last_modified = std::chrono::system_clock::now();
    snapshot.last_saved = snapshot.last_modified;
    
    m_system_states[system_name] = snapshot;
    
    std::lock_guard stats_lock(m_stats_mutex);
    m_stats.total_systems++;
    m_stats.clean_systems++;
}

void IncrementalSaveTracker::UnregisterSystem(const std::string& system_name) {
    std::unique_lock lock(m_state_mutex);
    
    auto it = m_system_states.find(system_name);
    if (it == m_system_states.end()) {
        return;
    }
    
    bool was_dirty = it->second.IsDirty();
    m_system_states.erase(it);
    
    std::lock_guard stats_lock(m_stats_mutex);
    if (m_stats.total_systems > 0) m_stats.total_systems--;
    if (was_dirty && m_stats.dirty_systems > 0) {
        m_stats.dirty_systems--;
    } else if (!was_dirty && m_stats.clean_systems > 0) {
        m_stats.clean_systems--;
    }
}

bool IncrementalSaveTracker::IsSystemRegistered(const std::string& system_name) const {
    std::shared_lock lock(m_state_mutex);
    return m_system_states.find(system_name) != m_system_states.end();
}

// ============================================================================
// Change Tracking
// ============================================================================

void IncrementalSaveTracker::MarkDirty(const std::string& system_name, SystemDirtyFlag flag) {
    std::unique_lock lock(m_state_mutex);
    
    auto it = m_system_states.find(system_name);
    if (it == m_system_states.end()) {
        return;  // System not registered
    }
    
    bool was_clean = !it->second.IsDirty();
    SystemDirtyFlag old_flag = it->second.dirty_flag;
    
    it->second.dirty_flag = flag;
    it->second.change_count++;
    it->second.last_modified = std::chrono::system_clock::now();
    
    // Update statistics
    {
        std::lock_guard stats_lock(m_stats_mutex);
        
        if (was_clean) {
            if (m_stats.clean_systems > 0) m_stats.clean_systems--;
            m_stats.dirty_systems++;
            m_stats.estimated_dirty_size += it->second.estimated_size;
        }
        
        if (flag == SystemDirtyFlag::CRITICAL && old_flag != SystemDirtyFlag::CRITICAL) {
            m_stats.critical_systems++;
            if (m_config.immediate_save_on_critical) {
                m_stats.immediate_save_triggers++;
            }
        }
        
        m_stats.total_changes_tracked++;
    }
    
    // Record change event
    SystemChangeEvent event(system_name, SystemChangeEvent::EventType::MODIFIED);
    event.estimated_size_delta = it->second.estimated_size;
    RecordChangeEvent(event);
}

void IncrementalSaveTracker::MarkClean(const std::string& system_name) {
    std::unique_lock lock(m_state_mutex);
    
    auto it = m_system_states.find(system_name);
    if (it == m_system_states.end()) {
        return;
    }
    
    bool was_dirty = it->second.IsDirty();
    bool was_critical = it->second.IsCritical();
    
    it->second.dirty_flag = SystemDirtyFlag::CLEAN;
    it->second.last_saved = std::chrono::system_clock::now();
    
    if (was_dirty) {
        std::lock_guard stats_lock(m_stats_mutex);
        if (m_stats.dirty_systems > 0) m_stats.dirty_systems--;
        m_stats.clean_systems++;
        
        if (m_stats.estimated_dirty_size >= it->second.estimated_size) {
            m_stats.estimated_dirty_size -= it->second.estimated_size;
        }
        
        if (was_critical && m_stats.critical_systems > 0) {
            m_stats.critical_systems--;
        }
        
        m_stats.last_save_time = std::chrono::system_clock::now();
    }
}

void IncrementalSaveTracker::MarkAllClean() {
    std::unique_lock lock(m_state_mutex);
    
    auto now = std::chrono::system_clock::now();
    
    for (auto& [name, snapshot] : m_system_states) {
        snapshot.dirty_flag = SystemDirtyFlag::CLEAN;
        snapshot.last_saved = now;
    }
    
    std::lock_guard stats_lock(m_stats_mutex);
    m_stats.clean_systems = m_stats.total_systems;
    m_stats.dirty_systems = 0;
    m_stats.critical_systems = 0;
    m_stats.estimated_dirty_size = 0;
    m_stats.last_save_time = now;
}

void IncrementalSaveTracker::MarkMultipleDirty(
    const std::vector<std::string>& system_names,
    SystemDirtyFlag flag) {
    
    for (const auto& name : system_names) {
        MarkDirty(name, flag);
    }
}

void IncrementalSaveTracker::MarkMultipleClean(const std::vector<std::string>& system_names) {
    for (const auto& name : system_names) {
        MarkClean(name);
    }
}

// ============================================================================
// Query Operations
// ============================================================================

bool IncrementalSaveTracker::IsDirty(const std::string& system_name) const {
    std::shared_lock lock(m_state_mutex);
    
    auto it = m_system_states.find(system_name);
    if (it == m_system_states.end()) {
        return false;
    }
    
    return it->second.IsDirty();
}

bool IncrementalSaveTracker::HasDirtySystems() const {
    std::shared_lock lock(m_state_mutex);
    
    for (const auto& [name, snapshot] : m_system_states) {
        if (snapshot.IsDirty()) {
            return true;
        }
    }
    
    return false;
}

bool IncrementalSaveTracker::HasCriticalChanges() const {
    std::shared_lock lock(m_state_mutex);
    
    for (const auto& [name, snapshot] : m_system_states) {
        if (snapshot.IsCritical()) {
            return true;
        }
    }
    
    return false;
}

size_t IncrementalSaveTracker::GetDirtySystemCount() const {
    std::shared_lock lock(m_state_mutex);
    
    size_t count = 0;
    for (const auto& [name, snapshot] : m_system_states) {
        if (snapshot.IsDirty()) {
            count++;
        }
    }
    
    return count;
}

std::vector<std::string> IncrementalSaveTracker::GetDirtySystems() const {
    std::shared_lock lock(m_state_mutex);
    
    std::vector<std::string> dirty_systems;
    for (const auto& [name, snapshot] : m_system_states) {
        if (snapshot.IsDirty()) {
            dirty_systems.push_back(name);
        }
    }
    
    return dirty_systems;
}

std::vector<std::string> IncrementalSaveTracker::GetCriticalSystems() const {
    std::shared_lock lock(m_state_mutex);
    
    std::vector<std::string> critical_systems;
    for (const auto& [name, snapshot] : m_system_states) {
        if (snapshot.IsCritical()) {
            critical_systems.push_back(name);
        }
    }
    
    return critical_systems;
}

std::vector<std::string> IncrementalSaveTracker::GetCleanSystems() const {
    std::shared_lock lock(m_state_mutex);
    
    std::vector<std::string> clean_systems;
    for (const auto& [name, snapshot] : m_system_states) {
        if (!snapshot.IsDirty()) {
            clean_systems.push_back(name);
        }
    }
    
    return clean_systems;
}

SystemStateSnapshot IncrementalSaveTracker::GetSystemSnapshot(const std::string& system_name) const {
    std::shared_lock lock(m_state_mutex);
    
    auto it = m_system_states.find(system_name);
    if (it != m_system_states.end()) {
        return it->second;
    }
    
    return SystemStateSnapshot{};
}

std::vector<SystemStateSnapshot> IncrementalSaveTracker::GetAllSnapshots() const {
    std::shared_lock lock(m_state_mutex);
    
    std::vector<SystemStateSnapshot> snapshots;
    snapshots.reserve(m_system_states.size());
    
    for (const auto& [name, snapshot] : m_system_states) {
        snapshots.push_back(snapshot);
    }
    
    return snapshots;
}

// ============================================================================
// Auto-Save Trigger Evaluation
// ============================================================================

bool IncrementalSaveTracker::ShouldTriggerAutoSave() const {
    if (!m_config.enable_auto_save) {
        return false;
    }
    
    std::shared_lock lock(m_state_mutex);
    
    // Check dirty system threshold
    size_t dirty_count = 0;
    size_t estimated_size = 0;
    auto now = std::chrono::system_clock::now();
    
    for (const auto& [name, snapshot] : m_system_states) {
        if (snapshot.IsDirty()) {
            dirty_count++;
            estimated_size += snapshot.estimated_size;
            
            // Check time threshold
            auto time_since_save = snapshot.TimeSinceLastSave();
            if (time_since_save >= m_config.dirty_time_threshold) {
                return true;
            }
        }
    }
    
    // Check dirty count threshold
    if (dirty_count >= m_config.dirty_system_threshold) {
        return true;
    }
    
    // Check estimated size threshold
    if (estimated_size >= m_config.estimated_size_threshold) {
        return true;
    }
    
    return false;
}

bool IncrementalSaveTracker::ShouldTriggerImmediateSave() const {
    if (!m_config.immediate_save_on_critical) {
        return false;
    }
    
    return HasCriticalChanges();
}

std::string IncrementalSaveTracker::GetAutoSaveTriggerReason() const {
    std::shared_lock lock(m_state_mutex);
    
    size_t dirty_count = 0;
    size_t estimated_size = 0;
    bool has_old_changes = false;
    
    for (const auto& [name, snapshot] : m_system_states) {
        if (snapshot.IsDirty()) {
            dirty_count++;
            estimated_size += snapshot.estimated_size;
            
            if (snapshot.TimeSinceLastSave() >= m_config.dirty_time_threshold) {
                has_old_changes = true;
            }
        }
    }
    
    std::ostringstream ss;
    
    if (HasCriticalChanges()) {
        ss << "Critical changes detected";
    } else if (has_old_changes) {
        ss << "Changes older than " << m_config.dirty_time_threshold.count() << " seconds";
    } else if (dirty_count >= m_config.dirty_system_threshold) {
        ss << dirty_count << " dirty systems (threshold: " << m_config.dirty_system_threshold << ")";
    } else if (estimated_size >= m_config.estimated_size_threshold) {
        ss << "Estimated size: " << estimated_size << " bytes (threshold: " 
           << m_config.estimated_size_threshold << ")";
    } else {
        ss << "No auto-save trigger met";
    }
    
    return ss.str();
}

// ============================================================================
// Change Event Tracking
// ============================================================================

void IncrementalSaveTracker::RecordChangeEvent(const SystemChangeEvent& event) {
    std::lock_guard lock(m_event_mutex);
    
    m_change_events.push_back(event);
    
    if (m_change_callback) {
        m_change_callback(event);
    }
    
    TrimEventHistory();
}

void IncrementalSaveTracker::SetChangeEventCallback(ChangeEventCallback callback) {
    std::lock_guard lock(m_event_mutex);
    m_change_callback = std::move(callback);
}

std::vector<SystemChangeEvent> IncrementalSaveTracker::GetRecentEvents(size_t count) const {
    std::lock_guard lock(m_event_mutex);
    
    if (count >= m_change_events.size()) {
        return m_change_events;
    }
    
    return std::vector<SystemChangeEvent>(
        m_change_events.end() - count,
        m_change_events.end());
}

void IncrementalSaveTracker::ClearEventHistory() {
    std::lock_guard lock(m_event_mutex);
    m_change_events.clear();
}

void IncrementalSaveTracker::TrimEventHistory() {
    if (m_change_events.size() > m_max_event_history) {
        size_t to_remove = m_change_events.size() - m_max_event_history;
        m_change_events.erase(
            m_change_events.begin(),
            m_change_events.begin() + to_remove);
    }
}

// ============================================================================
// Content Hashing
// ============================================================================

void IncrementalSaveTracker::UpdateContentHash(
    const std::string& system_name,
    const void* data,
    size_t size) {
    
    if (!m_config.enable_content_hashing || !data || size == 0) {
        return;
    }
    
    std::unique_lock lock(m_state_mutex);
    
    auto it = m_system_states.find(system_name);
    if (it == m_system_states.end()) {
        return;
    }
    
    uint64_t new_hash = m_config.fast_hash_mode 
        ? CalculateFastHash(data, size)
        : CalculateHash(data, size);
    
    if (it->second.content_hash != new_hash) {
        it->second.content_hash = new_hash;
    }
}

void IncrementalSaveTracker::UpdateContentHashFromJSON(
    const std::string& system_name,
    const Json::Value& data) {
    
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    std::string json_str = Json::writeString(builder, data);
    
    UpdateContentHash(system_name, json_str.data(), json_str.size());
}

bool IncrementalSaveTracker::HasContentChanged(
    const std::string& system_name,
    const void* data,
    size_t size) const {
    
    if (!m_config.enable_content_hashing || !data || size == 0) {
        return true;  // Assume changed if hashing disabled
    }
    
    std::shared_lock lock(m_state_mutex);
    
    auto it = m_system_states.find(system_name);
    if (it == m_system_states.end()) {
        return true;
    }
    
    uint64_t current_hash = m_config.fast_hash_mode
        ? CalculateFastHash(data, size)
        : CalculateHash(data, size);
    
    return it->second.content_hash != current_hash;
}

// ============================================================================
// Hashing Implementation (FIXED - Added missing CalculateHash)
// ============================================================================

uint64_t IncrementalSaveTracker::CalculateHash(const void* data, size_t size) const {
    // FIX Bug #3: Implement full FNV-1a hash (strong variant)
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    const uint64_t FNV_OFFSET = 14695981039346656037ULL;
    const uint64_t FNV_PRIME = 1099511628211ULL;
    
    uint64_t hash = FNV_OFFSET;
    for (size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= FNV_PRIME;
    }
    
    return hash;
}

uint64_t IncrementalSaveTracker::CalculateFastHash(const void* data, size_t size) const {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    const size_t SAMPLE_SIZE = 256;
    const size_t TAIL_SIZE = 1024;
    
    uint64_t hash = size;
    
    // Sample first 256 bytes
    size_t sample_end = std::min(SAMPLE_SIZE, size);
    for (size_t i = 0; i < sample_end; ++i) {
        hash = hash * 31 + bytes[i];
    }
    
    // Sample middle if large enough
    if (size > SAMPLE_SIZE * 3) {
        size_t mid_start = size / 2 - SAMPLE_SIZE / 2;
        size_t mid_end = mid_start + SAMPLE_SIZE;
        for (size_t i = mid_start; i < mid_end; ++i) {
            hash = hash * 31 + bytes[i];
        }
    }
    
    // Full FNV-1a on last 1KB
    if (size > TAIL_SIZE) {
        const uint64_t FNV_OFFSET = 14695981039346656037ULL;
        const uint64_t FNV_PRIME = 1099511628211ULL;
        uint64_t tail_hash = FNV_OFFSET;
        size_t tail_start = size - TAIL_SIZE;
        
        for (size_t i = tail_start; i < size; ++i) {
            tail_hash ^= bytes[i];
            tail_hash *= FNV_PRIME;
        }
        hash ^= tail_hash;
    }
    
    return hash;
}

// ============================================================================
// Statistics
// ============================================================================

IncrementalSaveTracker::Statistics IncrementalSaveTracker::GetStatistics() const {
    std::lock_guard lock(m_stats_mutex);
    
    auto stats = m_stats;
    
    if (stats.last_save_time.time_since_epoch().count() > 0) {
        auto now = std::chrono::system_clock::now();
        stats.time_since_last_save = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - stats.last_save_time);
    }
    
    return stats;
}

void IncrementalSaveTracker::ResetStatistics() {
    std::lock_guard lock(m_stats_mutex);
    m_stats = Statistics{};
    m_stats.total_systems = m_system_states.size();
    
    // Recalculate clean/dirty counts
    for (const auto& [name, snapshot] : m_system_states) {
        if (snapshot.IsDirty()) {
            m_stats.dirty_systems++;
        } else {
            m_stats.clean_systems++;
        }
    }
}

std::string IncrementalSaveTracker::Statistics::GenerateReport() const {
    std::ostringstream ss;
    ss << "Incremental Save Statistics:\n";
    ss << "  Total Systems: " << total_systems << "\n";
    ss << "  Clean Systems: " << clean_systems << "\n";
    ss << "  Dirty Systems: " << dirty_systems << "\n";
    ss << "  Critical Systems: " << critical_systems << "\n";
    ss << "  Total Changes Tracked: " << total_changes_tracked << "\n";
    ss << "  Auto-Save Triggers: " << auto_save_triggers << "\n";
    ss << "  Immediate Save Triggers: " << immediate_save_triggers << "\n";
    ss << "  Estimated Dirty Size: " << estimated_dirty_size << " bytes\n";
    
    if (time_since_last_save.count() > 0) {
        ss << "  Time Since Last Save: " << time_since_last_save.count() << "ms\n";
    }
    
    return ss.str();
}

Json::Value IncrementalSaveTracker::Statistics::ToJson() const {
    Json::Value root;
    root["total_systems"] = static_cast<Json::UInt64>(total_systems);
    root["clean_systems"] = static_cast<Json::UInt64>(clean_systems);
    root["dirty_systems"] = static_cast<Json::UInt64>(dirty_systems);
    root["critical_systems"] = static_cast<Json::UInt64>(critical_systems);
    root["total_changes_tracked"] = static_cast<Json::UInt64>(total_changes_tracked);
    root["auto_save_triggers"] = static_cast<Json::UInt64>(auto_save_triggers);
    root["immediate_save_triggers"] = static_cast<Json::UInt64>(immediate_save_triggers);
    root["estimated_dirty_size"] = static_cast<Json::UInt64>(estimated_dirty_size);
    root["time_since_last_save_ms"] = static_cast<Json::Int64>(time_since_last_save.count());
    return root;
}

// ============================================================================
// Configuration Management
// ============================================================================

void IncrementalSaveTracker::UpdateConfig(const IncrementalSaveConfig& config) {
    m_config = config;
}

IncrementalSaveConfig IncrementalSaveTracker::GetConfig() const {
    return m_config;
}

// ============================================================================
// Save Optimization Hints (FIXED - Bug #1)
// ============================================================================

IncrementalSaveTracker::SaveOptimizationHints 
IncrementalSaveTracker::GetSaveOptimizationHints() const {
    std::shared_lock lock(m_state_mutex);
    
    SaveOptimizationHints hints;
    
    // FIX Bug #1: Single pass calculation with proper loop closure
    size_t dirty_count = 0;
    size_t dirty_bytes = 0;
    size_t total_bytes = 0;
    
    for (const auto& [name, snapshot] : m_system_states) {
        total_bytes += snapshot.estimated_size;
        if (snapshot.IsDirty()) {
            dirty_count++;
            dirty_bytes += snapshot.estimated_size;
        }
    }
    
    // Early exit if nothing dirty
    if (dirty_count == 0) {
        hints.recommend_full_save = false;
        hints.recommend_incremental_save = false;
        hints.recommendation_reason = "No dirty systems";
        return hints;
    }
    
    // Use byte ratio, not count ratio
    double dirty_byte_ratio = (total_bytes > 0) 
        ? static_cast<double>(dirty_bytes) / total_bytes 
        : 0.0;
    
    const double FULL_SAVE_THRESHOLD = 0.60;
    hints.recommend_full_save = (dirty_byte_ratio > FULL_SAVE_THRESHOLD);
    hints.recommend_incremental_save = !hints.recommend_full_save;
    
    // Build priority lists
    for (const auto& [name, snapshot] : m_system_states) {
        if (snapshot.IsCritical()) {
            hints.priority_systems.push_back(name);
        } else if (snapshot.IsDirty()) {
            if (snapshot.TimeSinceLastSave() > m_config.dirty_time_threshold) {
                hints.priority_systems.push_back(name);
            } else {
                hints.deferrable_systems.push_back(name);
            }
        }
    }
    
    // Estimate save time (rough heuristic: 1ms per 100KB)
    size_t estimated_size = 0;
    for (const auto& name : hints.priority_systems) {
        auto it = m_system_states.find(name);
        if (it != m_system_states.end()) {
            estimated_size += it->second.estimated_size;
        }
    }
    
    hints.estimated_save_time_ms = estimated_size / (100 * 1024);
    
    if (hints.recommend_full_save) {
        hints.recommendation_reason = "Over 60% of data dirty by bytes - full save more efficient";
    } else {
        hints.recommendation_reason = std::to_string(dirty_count) + " dirty systems (" +
                                     std::to_string(static_cast<int>(dirty_byte_ratio * 100)) + 
                                     "% by bytes) - incremental save recommended";
    }
    
    return hints;
}

// ============================================================================
// IncrementalSaveManager Implementation
// ============================================================================

IncrementalSaveManager::IncrementalSaveManager(IncrementalSaveTracker& tracker)
    : m_tracker(tracker) {}

Expected<std::vector<std::string>> IncrementalSaveManager::PerformIncrementalSave(
    std::function<Expected<bool>(const std::vector<std::string>&)> save_function) {
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Get dirty systems
    std::vector<std::string> dirty_systems = m_tracker.GetDirtySystems();
    
    if (dirty_systems.empty()) {
        return dirty_systems;  // Nothing to save
    }
    
    // Perform save
    auto result = save_function(dirty_systems);
    if (!result.has_value()) {
        return result.error();
    }
    
    // Mark systems as clean
    m_tracker.MarkMultipleClean(dirty_systems);
    
    // Record performance metrics
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time);
    
    size_t estimated_bytes = 0;
    for (const auto& system_name : dirty_systems) {
        auto snapshot = m_tracker.GetSystemSnapshot(system_name);
        estimated_bytes += snapshot.estimated_size;
    }
    
    RecordIncrementalSave(dirty_systems.size(), duration, estimated_bytes);
    
    return dirty_systems;
}

Expected<std::vector<std::string>> IncrementalSaveManager::PerformSmartSave(
    std::function<Expected<bool>(const std::vector<std::string>&)> save_full,
    std::function<Expected<bool>(const std::vector<std::string>&)> save_incremental) {
    
    SaveStrategy strategy = DetermineSaveStrategy();
    
    switch (strategy) {
        case SaveStrategy::FULL_SAVE: {
            auto start_time = std::chrono::steady_clock::now();
            
            // Get all systems
            std::vector<std::string> all_systems;
            for (const auto& snapshot : m_tracker.GetAllSnapshots()) {
                all_systems.push_back(snapshot.system_name);
            }
            
            auto result = save_full(all_systems);
            if (!result.has_value()) {
                return result.error();
            }
            
            m_tracker.MarkAllClean();
            
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time);
            
            RecordFullSave(duration, 0);  // Size estimation for full save omitted
            
            return all_systems;
        }
        
        case SaveStrategy::INCREMENTAL_SAVE:
        case SaveStrategy::PRIORITY_SAVE: {
            return PerformIncrementalSave(save_incremental);
        }
        
        case SaveStrategy::NO_SAVE_NEEDED:
        default:
            return std::vector<std::string>{};
    }
}

IncrementalSaveManager::SaveStrategy IncrementalSaveManager::DetermineSaveStrategy() const {
    if (!m_tracker.HasDirtySystems()) {
        return SaveStrategy::NO_SAVE_NEEDED;
    }
    
    if (m_tracker.HasCriticalChanges()) {
        return SaveStrategy::PRIORITY_SAVE;
    }
    
    auto hints = m_tracker.GetSaveOptimizationHints();
    
    if (hints.recommend_full_save) {
        return SaveStrategy::FULL_SAVE;
    }
    
    return SaveStrategy::INCREMENTAL_SAVE;
}

std::string IncrementalSaveManager::GetStrategyDescription(SaveStrategy strategy) const {
    switch (strategy) {
        case SaveStrategy::FULL_SAVE:
            return "Full Save - Save all systems";
        case SaveStrategy::INCREMENTAL_SAVE:
            return "Incremental Save - Save only dirty systems";
        case SaveStrategy::PRIORITY_SAVE:
            return "Priority Save - Save critical systems first";
        case SaveStrategy::DEFERRED_SAVE:
            return "Deferred Save - Delay non-critical systems";
        case SaveStrategy::NO_SAVE_NEEDED:
            return "No Save Needed - All systems clean";
        default:
            return "Unknown Strategy";
    }
}

IncrementalSaveManager::AutoSaveSchedule 
IncrementalSaveManager::EvaluateAutoSaveSchedule() const {
    AutoSaveSchedule schedule;
    
    schedule.should_save_now = m_tracker.ShouldTriggerAutoSave() || 
                                m_tracker.ShouldTriggerImmediateSave();
    
    if (!schedule.should_save_now) {
        schedule.recommended_strategy = SaveStrategy::NO_SAVE_NEEDED;
        schedule.trigger_reason = "No auto-save conditions met";
        return schedule;
    }
    
    schedule.recommended_strategy = DetermineSaveStrategy();
    schedule.trigger_reason = m_tracker.GetAutoSaveTriggerReason();
    
    // Get systems to save based on strategy
    if (schedule.recommended_strategy == SaveStrategy::PRIORITY_SAVE ||
        schedule.recommended_strategy == SaveStrategy::INCREMENTAL_SAVE) {
        schedule.systems_to_save = m_tracker.GetDirtySystems();
    }
    
    // Estimate duration
    auto hints = m_tracker.GetSaveOptimizationHints();
    schedule.estimated_duration = std::chrono::milliseconds(hints.estimated_save_time_ms);
    
    return schedule;
}

IncrementalSaveManager::PerformanceMetrics 
IncrementalSaveManager::GetPerformanceMetrics() const {
    std::lock_guard lock(m_perf_mutex);
    return m_performance;
}

void IncrementalSaveManager::ResetPerformanceMetrics() {
    std::lock_guard lock(m_perf_mutex);
    m_performance = PerformanceMetrics{};
}

double IncrementalSaveManager::PerformanceMetrics::GetIncrementalSpeedupRatio() const {
    if (full_saves == 0 || total_full_time.count() == 0) {
        return 1.0;
    }
    
    double avg_full_time = static_cast<double>(total_full_time.count()) / full_saves;
    
    if (incremental_saves == 0 || total_incremental_time.count() == 0) {
        return 1.0;
    }
    
    double avg_incremental_time = static_cast<double>(total_incremental_time.count()) / incremental_saves;
    
    return avg_full_time / avg_incremental_time;
}

double IncrementalSaveManager::PerformanceMetrics::GetIncrementalSizeRatio() const {
    if (bytes_saved_full == 0) {
        return 1.0;
    }
    
    return static_cast<double>(bytes_saved_incremental) / bytes_saved_full;
}

std::string IncrementalSaveManager::PerformanceMetrics::GenerateReport() const {
    std::ostringstream ss;
    ss << "Incremental Save Performance:\n";
    ss << "  Incremental Saves: " << incremental_saves << "\n";
    ss << "  Full Saves: " << full_saves << "\n";
    
    if (incremental_saves > 0) {
        double avg_time = static_cast<double>(total_incremental_time.count()) / incremental_saves;
        ss << "  Average Incremental Time: " << std::fixed << std::setprecision(2) 
           << avg_time << "ms\n";
    }
    
    if (full_saves > 0) {
        double avg_time = static_cast<double>(total_full_time.count()) / full_saves;
        ss << "  Average Full Save Time: " << std::fixed << std::setprecision(2) 
           << avg_time << "ms\n";
    }
    
    double speedup = GetIncrementalSpeedupRatio();
    if (speedup > 1.0) {
        ss << "  Speedup: " << std::fixed << std::setprecision(2) << speedup << "x faster\n";
    }
    
    ss << "  Bytes Saved (Incremental): " << bytes_saved_incremental << "\n";
    ss << "  Bytes Saved (Full): " << bytes_saved_full << "\n";
    
    return ss.str();
}

void IncrementalSaveManager::RecordIncrementalSave(
    size_t system_count,
    std::chrono::milliseconds duration,
    size_t bytes_saved) {
    
    std::lock_guard lock(m_perf_mutex);
    m_performance.incremental_saves++;
    m_performance.total_incremental_time += duration;
    m_performance.bytes_saved_incremental += bytes_saved;
}

void IncrementalSaveManager::RecordFullSave(
    std::chrono::milliseconds duration,
    size_t bytes_saved) {
    
    std::lock_guard lock(m_perf_mutex);
    m_performance.full_saves++;
    m_performance.total_full_time += duration;
    m_performance.bytes_saved_full += bytes_saved;
}

// ============================================================================
// DirtySystemFilter Implementation
// ============================================================================

DirtySystemFilter::DirtySystemFilter(const IncrementalSaveTracker& tracker)
    : m_tracker(tracker) {}

std::vector<std::string> DirtySystemFilter::FilterDirtySystems(
    const std::vector<std::string>& all_systems) const {
    
    std::vector<std::string> dirty_systems;
    
    for (const auto& system_name : all_systems) {
        if (m_tracker.IsDirty(system_name)) {
            dirty_systems.push_back(system_name);
        }
    }
    
    return dirty_systems;
}

std::vector<std::string> DirtySystemFilter::FilterCleanSystems(
    const std::vector<std::string>& all_systems) const {
    
    std::vector<std::string> clean_systems;
    
    for (const auto& system_name : all_systems) {
        if (!m_tracker.IsDirty(system_name)) {
            clean_systems.push_back(system_name);
        }
    }
    
    return clean_systems;
}

std::vector<std::string> DirtySystemFilter::FilterCriticalSystems(
    const std::vector<std::string>& all_systems) const {
    
    std::vector<std::string> critical_systems;
    
    for (const auto& system_name : all_systems) {
        auto snapshot = m_tracker.GetSystemSnapshot(system_name);
        if (snapshot.IsCritical()) {
            critical_systems.push_back(system_name);
        }
    }
    
    return critical_systems;
}

std::vector<std::string> DirtySystemFilter::SortByPriority(
    const std::vector<std::string>& systems) const {
    
    std::vector<std::pair<std::string, int>> priority_pairs;
    
    for (const auto& system_name : systems) {
        auto snapshot = m_tracker.GetSystemSnapshot(system_name);
        
        int priority = 0;
        
        // Critical systems get highest priority
        if (snapshot.IsCritical()) {
            priority = 1000;
        }
        // Dirty systems by age
        else if (snapshot.IsDirty()) {
            auto time_since_save = snapshot.TimeSinceLastSave();
            priority = static_cast<int>(time_since_save.count());
        }
        
        priority_pairs.emplace_back(system_name, priority);
    }
    
    // Sort by priority (descending)
    std::sort(priority_pairs.begin(), priority_pairs.end(),
        [](const auto& a, const auto& b) {
            return a.second > b.second;
        });
    
    std::vector<std::string> sorted_systems;
    for (const auto& pair : priority_pairs) {
        sorted_systems.push_back(pair.first);
    }
    
    return sorted_systems;
}

std::vector<std::string> DirtySystemFilter::SortBySize(
    const std::vector<std::string>& systems,
    bool largest_first) const {
    
    std::vector<std::pair<std::string, size_t>> size_pairs;
    
    for (const auto& system_name : systems) {
        auto snapshot = m_tracker.GetSystemSnapshot(system_name);
        size_pairs.emplace_back(system_name, snapshot.estimated_size);
    }
    
    // Sort by size
    if (largest_first) {
        std::sort(size_pairs.begin(), size_pairs.end(),
            [](const auto& a, const auto& b) {
                return a.second > b.second;
            });
    } else {
        std::sort(size_pairs.begin(), size_pairs.end(),
            [](const auto& a, const auto& b) {
                return a.second < b.second;
            });
    }
    
    std::vector<std::string> sorted_systems;
    for (const auto& pair : size_pairs) {
        sorted_systems.push_back(pair.first);
    }
    
    return sorted_systems;
}

} // namespace core::save