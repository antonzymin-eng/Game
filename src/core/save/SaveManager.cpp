// Created: September 18, 2025 - 14:00:00 (Updated for C++17)
// Location: src/core/save/SaveManager.cpp
// Mechanica Imperii - SaveManager Core Operations Implementation (C++17 Compliant)

#include "core/save/SaveManager.h"
#include "utils/PlatformCompat.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <random>
#include <thread>
#include <chrono>
#include <regex>

// Platform includes for file operations
#ifdef PLATFORM_WINDOWS
  #include <io.h>
  #include <fcntl.h>
#else
  #include <unistd.h>
  #include <sys/stat.h>
  #include <fcntl.h>
  #include <cerrno>
#endif

namespace core::save {

// C++17 helper for string starts_with
static bool string_starts_with(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && 
           str.compare(0, prefix.size(), prefix) == 0;
}

// C++17 helper for string ends_with
static bool string_ends_with(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && 
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// ============================================================================
// Error Handling Implementation
// ============================================================================

std::string ToString(SaveError error) {
    switch (error) {
        case SaveError::NONE: return "No error";
        case SaveError::INVALID_FILENAME: return "Invalid filename - contains illegal characters or path components";
        case SaveError::PATH_TRAVERSAL: return "Path traversal attempt detected - filename escapes base directory";
        case SaveError::INSUFFICIENT_SPACE: return "Insufficient disk space - operation requires more storage";
        case SaveError::PERMISSION_DENIED: return "Permission denied - check file system permissions";
        case SaveError::FILE_NOT_FOUND: return "File not found - specified save file does not exist";
        case SaveError::CORRUPTION_DETECTED: return "File corruption detected - data integrity compromised";
        case SaveError::SERIALIZATION_FAILED: return "Serialization failed - unable to convert game data to save format";
        case SaveError::VALIDATION_FAILED: return "Validation failed - save data does not meet requirements";
        case SaveError::MIGRATION_FAILED: return "Migration failed - unable to upgrade save file to current version";
        case SaveError::CHECKSUM_MISMATCH: return "Checksum mismatch - file may be corrupted or tampered";
        case SaveError::CONCURRENT_LIMIT_EXCEEDED: return "Concurrent operation limit exceeded - too many active operations";
        case SaveError::OPERATION_CANCELLED: return "Operation was cancelled by user request";
        case SaveError::UNKNOWN_ERROR: return "Unknown error occurred during operation";
        default: return "Unrecognized error code: " + std::to_string(static_cast<int>(error));
    }
}

// ============================================================================
// DefaultLogger Implementation
// ============================================================================

DefaultLogger::DefaultLogger(LogLevel level) : m_level(level) {}

void DefaultLogger::Debug(const std::string& msg) {
    if (m_level.load() <= LogLevel::DEBUG) {
        std::cout << "[Save] DEBUG " << msg << std::endl;
    }
}

void DefaultLogger::Info(const std::string& msg) {
    if (m_level.load() <= LogLevel::INFO) {
        std::cout << "[Save] INFO  " << msg << std::endl;
    }
}

void DefaultLogger::Warn(const std::string& msg) {
    if (m_level.load() <= LogLevel::WARN) {
        std::cout << "[Save] WARN  " << msg << std::endl;
    }
}

void DefaultLogger::Error(const std::string& msg) {
    if (m_level.load() <= LogLevel::ERROR) {
        std::cerr << "[Save] ERROR " << msg << std::endl;
    }
}

void DefaultLogger::LogMetric(const std::string& name, double value, const std::unordered_map<std::string, std::string>& tags) {
    if (m_level.load() <= LogLevel::INFO) {
        std::ostringstream ss;
        ss << "[Save] METRIC " << name << "=" << value;
        if (!tags.empty()) {
            ss << " {";
            bool first = true;
            for (const auto& [key, val] : tags) {
                if (!first) ss << ", ";
                ss << key << "=" << val;
                first = false;
            }
            ss << "}";
        }
        std::cout << ss.str() << std::endl;
    }
}

// ============================================================================
// SaveVersion Implementation
// ============================================================================

SaveVersion::SaveVersion(int maj, int min, int pat, const std::string& hash)
    : major(maj), minor(min), patch(pat), build_hash(hash), created_time(std::chrono::system_clock::now()) {}

std::string SaveVersion::ToString() const {
    std::string s = std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    if (!build_hash.empty()) s += "-" + build_hash;
    return s;
}

Expected<SaveVersion> SaveVersion::FromString(const std::string& s) {
    if (!IsValidVersionString(s)) {
        return SaveError::VALIDATION_FAILED;
    }
    
    std::regex re(R"(^(\d+)\.(\d+)\.(\d+)(?:-([A-Za-z0-9\-_]+))?$)");
    std::smatch m;
    if (!std::regex_match(s, m, re)) {
        return SaveError::VALIDATION_FAILED;
    }
    
    SaveVersion v;
    try {
        v.major = std::stoi(m[1]);
        v.minor = std::stoi(m[2]);
        v.patch = std::stoi(m[3]);
        if (m[4].matched) v.build_hash = m[4].str();
        v.created_time = std::chrono::system_clock::now();
        return v;
    } catch (const std::exception&) {
        return SaveError::VALIDATION_FAILED;
    }
}

bool SaveVersion::IsValidVersionString(const std::string& s) {
    std::regex re(R"(^(\d+)\.(\d+)\.(\d+)(?:-([A-Za-z0-9\-_]+))?$)");
    return std::regex_match(s, re);
}

bool SaveVersion::operator==(const SaveVersion& o) const { 
    return major == o.major && minor == o.minor && patch == o.patch; 
}

bool SaveVersion::operator!=(const SaveVersion& o) const { 
    return !(*this == o); 
}

bool SaveVersion::operator<(const SaveVersion& o) const {
    if (major != o.major) return major < o.major;
    if (minor != o.minor) return minor < o.minor;
    return patch < o.patch;
}

bool SaveVersion::operator<=(const SaveVersion& o) const { 
    return *this < o || *this == o; 
}

bool SaveVersion::operator>(const SaveVersion& o) const { 
    return o < *this; 
}

bool SaveVersion::operator>=(const SaveVersion& o) const { 
    return o <= *this; 
}

int SaveVersion::ToInt() const { 
    return major * 10000 + minor * 100 + patch; 
}

SaveVersion SaveVersion::FromInt(int v) { 
    return SaveVersion(v / 10000, (v % 10000) / 100, v % 100); 
}

bool SaveVersion::IsCompatibleWith(const SaveVersion& other) const { 
    return major == other.major && *this >= other; 
}

// ============================================================================
// SaveProgress Implementation
// ============================================================================

void SaveProgress::UpdateProgress(double percent, const std::string& op) {
    percentage.store(std::clamp(percent, 0.0, 100.0));
    current_operation = op;
    
    if (percent >= 100.0) {
        is_complete.store(true);
    }
    
    // Update estimated completion time based on progress
    if (start_time.time_since_epoch().count() == 0) {
        start_time = std::chrono::steady_clock::now();
    }
    
    if (percent > 0.0 && percent < 100.0) {
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        auto estimated_total = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            elapsed * (100.0 / percent));
        estimated_completion = start_time + estimated_total;
    }
}

double SaveProgress::GetEstimatedSecondsRemaining() const {
    if (is_complete.load() || percentage.load() >= 100.0) {
        return 0.0;
    }
    
    auto now = std::chrono::steady_clock::now();
    if (estimated_completion > now) {
        auto remaining = estimated_completion - now;
        return std::chrono::duration<double>(remaining).count();
    }
    
    return 0.0;
}

std::string SaveProgress::GetFormattedTimeRemaining() const {
    double seconds = GetEstimatedSecondsRemaining();
    if (seconds <= 0.0) return "Complete";
    
    if (seconds < 60.0) {
        return std::to_string(static_cast<int>(seconds)) + "s";
    } else if (seconds < 3600.0) {
        int minutes = static_cast<int>(seconds / 60.0);
        int secs = static_cast<int>(seconds) % 60;
        return std::to_string(minutes) + "m " + std::to_string(secs) + "s";
    } else {
        int hours = static_cast<int>(seconds / 3600.0);
        int minutes = static_cast<int>(seconds / 60.0) % 60;
        return std::to_string(hours) + "h " + std::to_string(minutes) + "m";
    }
}

// ============================================================================
// ValidationReport Implementation
// ============================================================================

void ValidationReport::AddError(const std::string& validator, const std::string& path, const std::string& message, 
                               const std::optional<std::string>& fix) {
    passed = false;
    issues.emplace_back(ValidationIssue::ERROR, validator, path, message, fix);
}

void ValidationReport::AddWarning(const std::string& validator, const std::string& path, const std::string& message,
                                const std::optional<std::string>& fix) {
    issues.emplace_back(ValidationIssue::WARNING, validator, path, message, fix);
}

void ValidationReport::AddCritical(const std::string& validator, const std::string& path, const std::string& message,
                                 const std::optional<std::string>& fix) {
    passed = false;
    issues.emplace_back(ValidationIssue::CRITICAL, validator, path, message, fix);
}

size_t ValidationReport::GetErrorCount() const {
    return std::count_if(issues.begin(), issues.end(), 
        [](const ValidationIssue& issue) { return issue.severity == ValidationIssue::ERROR; });
}

size_t ValidationReport::GetWarningCount() const {
    return std::count_if(issues.begin(), issues.end(), 
        [](const ValidationIssue& issue) { return issue.severity == ValidationIssue::WARNING; });
}

size_t ValidationReport::GetCriticalCount() const {
    return std::count_if(issues.begin(), issues.end(), 
        [](const ValidationIssue& issue) { return issue.severity == ValidationIssue::CRITICAL; });
}

std::string ValidationReport::GenerateReport() const {
    std::ostringstream ss;
    ss << "Validation Report:\n";
    ss << "  Status: " << (passed ? "PASSED" : "FAILED") << "\n";
    ss << "  Duration: " << validation_time.count() << "ms\n";
    ss << "  Issues: " << issues.size() << " total (";
    ss << GetCriticalCount() << " critical, " << GetErrorCount() << " errors, " << GetWarningCount() << " warnings)\n";
    
    if (!issues.empty()) {
        ss << "\nDetailed Issues:\n";
        for (const auto& issue : issues) {
            std::string severity_str;
            switch (issue.severity) {
                case ValidationIssue::CRITICAL: severity_str = "CRITICAL"; break;
                case ValidationIssue::ERROR: severity_str = "ERROR"; break;
                case ValidationIssue::WARNING: severity_str = "WARNING"; break;
            }
            
            ss << "  [" << severity_str << "] " << issue.validator_name;
            if (!issue.field_path.empty()) {
                ss << " at " << issue.field_path;
            }
            ss << ": " << issue.message;
            if (issue.suggested_fix) {
                ss << " (Suggested fix: " << *issue.suggested_fix << ")";
            }
            ss << "\n";
        }
    }
    
    return ss.str();
}

// Note: ToJson(), AddError(), AddWarning(), AddCritical() already defined above

// ============================================================================
// SaveOperationResult Implementation (C++17)
// ============================================================================

bool SaveOperationResult::IsSuccess() const {
    return result == SaveResult::SUCCESS;
}

bool SaveOperationResult::IsInProgress() const {
    return result == SaveResult::IN_PROGRESS;
}

std::string SaveOperationResult::GetFullReport() const {
    std::ostringstream ss;
    ss << "Operation Result: " << ResultToString(result) << "\n";
    ss << "Operation ID: " << operation_id << "\n";
    
    if (!message.empty()) {
        ss << "Message: " << message << "\n";
    }
    
    if (migration_performed) {
        ss << "Migration: " << version_loaded.ToString() << " -> " << version_saved.ToString() << "\n";
        if (!migration_steps.empty()) {
            ss << "Migration Steps:\n";
            for (const auto& step : migration_steps) {
                ss << "  - " << step << "\n";
            }
        }
    }
    
    if (backup_created) {
        ss << "Backup: Created\n";
    }
    
    if (atomic_write_used) {
        ss << "Write Mode: Atomic\n";
    }
    
    if (bytes_written > 0) {
        ss << "Bytes: " << bytes_written << "\n";
    }
    
    if (operation_time.count() > 0) {
        ss << "Duration: " << operation_time.count() << "ms\n";
    }
    
    if (!sha256_checksum.empty()) {
        ss << "Checksum: " << sha256_checksum << "\n";
    }
    
    if (!warnings.empty()) {
        ss << "Warnings:\n";
        for (const auto& warning : warnings) {
            ss << "  - " << warning << "\n";
        }
    }
    
    return ss.str();
}

Json::Value SaveOperationResult::ToJson() const {
    Json::Value root;
    
    root["result"] = ResultToString(result);
    root["operation_id"] = operation_id;
    root["message"] = message;
    root["migration_performed"] = migration_performed;
    root["backup_created"] = backup_created;
    root["atomic_write_used"] = atomic_write_used;
    root["bytes_written"] = static_cast<Json::UInt64>(bytes_written);
    root["estimated_size"] = static_cast<Json::UInt64>(estimated_size);
    root["operation_time_ms"] = static_cast<Json::Int64>(operation_time.count());
    root["sha256_checksum"] = sha256_checksum;
    
    if (migration_performed) {
        root["version_loaded"] = version_loaded.ToString();
        root["version_saved"] = version_saved.ToString();
        
        Json::Value steps_array(Json::arrayValue);
        for (const auto& step : migration_steps) {
            steps_array.append(step);
        }
        root["migration_steps"] = steps_array;
    }
    
    Json::Value warnings_array(Json::arrayValue);
    for (const auto& warning : warnings) {
        warnings_array.append(warning);
    }
    root["warnings"] = warnings_array;
    
    if (!validation_report.issues.empty()) {
        root["validation_report"] = validation_report.ToJson();
    }
    
    Json::Value debug_object(Json::objectValue);
    for (const auto& [key, value] : debug_info) {
        debug_object[key] = value;
    }
    root["debug_info"] = debug_object;
    
    return root;
}

std::string SaveOperationResult::ResultToString(SaveResult r) const {
    switch (r) {
        case SaveResult::SUCCESS: return "Success";
        case SaveResult::IN_PROGRESS: return "In Progress";
        case SaveResult::CANCELLED: return "Cancelled";
        case SaveResult::FILE_ERROR: return "File Error";
        case SaveResult::SERIALIZATION_ERROR: return "Serialization Error";
        case SaveResult::VALIDATION_ERROR: return "Validation Error";
        case SaveResult::MIGRATION_ERROR: return "Migration Error";
        case SaveResult::BACKUP_ERROR: return "Backup Error";
        case SaveResult::VERSION_ERROR: return "Version Error";
        case SaveResult::ATOMIC_WRITE_FAILED: return "Atomic Write Failed";
        case SaveResult::CORRUPTION_DETECTED: return "Corruption Detected";
        case SaveResult::INSUFFICIENT_SPACE: return "Insufficient Space";
        case SaveResult::THREAD_SAFETY_ERROR: return "Thread Safety Error";
        case SaveResult::CHECKSUM_MISMATCH: return "Checksum Mismatch";
        case SaveResult::CONCURRENT_OPERATION_LIMIT: return "Concurrent Operation Limit";
        case SaveResult::INVALID_FILENAME: return "Invalid Filename";
        case SaveResult::PERMISSION_DENIED: return "Permission Denied";
        default: return "Unknown Result";
    }
}

// ============================================================================
// SaveManager Core Implementation
// ============================================================================

SaveManager::SaveManager(Config config) 
    : m_logger(config.logger ? std::move(config.logger) : std::make_unique<DefaultLogger>()),
      m_auto_backup(config.enable_auto_backup),
      m_max_backups(config.max_backups),
      m_atomic_writes_enabled(config.enable_atomic_writes),
      m_operation_timeout(config.operation_timeout) {
    
    // Validate configuration parameters
    if (config.max_concurrent_saves == 0 || config.max_concurrent_loads == 0) {
        throw std::invalid_argument("Concurrency limits must be greater than zero");
    }
    
    if (config.max_backups < 0) {
        throw std::invalid_argument("Max backups cannot be negative");
    }
    
    if (config.operation_timeout.count() <= 0) {
        throw std::invalid_argument("Operation timeout must be positive");
    }
    
    m_concurrency.max_saves = config.max_concurrent_saves;
    m_concurrency.max_loads = config.max_concurrent_loads;
    
    // Initialize components with comprehensive error handling
    try {
        // Ensure save directory exists with proper error handling
        std::error_code ec;
        std::filesystem::create_directories(m_save_dir, ec);
        if (ec && ec != std::errc::file_exists) {
            throw std::runtime_error("Failed to create save directory: " + ec.message());
        }
        
        // Verify directory is writable
        auto test_file = m_save_dir / ".write_test";
        std::ofstream test_stream(test_file);
        if (!test_stream.is_open()) {
            throw std::runtime_error("Save directory is not writable: " + m_save_dir.string());
        }
        test_stream.close();
        std::filesystem::remove(test_file, ec);
        
        // Initialize recovery manager
        m_recovery = std::make_unique<CrashRecoveryManager>(m_save_dir, m_logger.get());
        
        // Configure JSON cache with validation
        if (config.json_cache_size > 0) {
            CanonicalJSONBuilder::SetCacheSize(config.json_cache_size);
        } else {
            m_logger->Warn("JSON cache disabled (size = 0) - may impact performance");
        }
        
        // Initialize built-in validators and migrations
        InitializeBuiltinValidators();
        MigrationRegistry::Instance().InitializeDefaultMigrations();
        
        m_logger->Info("SaveManager initialized successfully");
        m_logger->Info("Configuration: " + 
                      std::to_string(config.max_concurrent_saves) + "/" + 
                      std::to_string(config.max_concurrent_loads) + " save/load slots, " +
                      std::to_string(config.max_backups) + " max backups, " +
                      std::to_string(config.operation_timeout.count()) + "s timeout");
        
    } catch (const std::exception& e) {
        m_logger->Error("SaveManager initialization failed: " + std::string(e.what()));
        throw;
    }
}

SaveManager::~SaveManager() {
    m_logger->Debug("SaveManager shutting down");
    
    auto cancel_result = CancelAllOperations();
    if (!cancel_result.has_value()) {
        m_logger->Warn("Failed to cancel all operations during shutdown");
    }
    
    if (!WaitForOperationsToComplete(m_operation_timeout)) {
        m_logger->Error("Some operations did not complete within shutdown timeout");
    }
    
    m_logger->Info("SaveManager shutdown complete");
}

void SaveManager::RegisterSystem(std::shared_ptr<core::ecs::ISerializable> system) {
    if (system) {
        m_systems.push_back(std::move(system));
        m_logger->Debug("Registered system: " + system->GetSystemName());
    }
}

Expected<bool> SaveManager::SetCurrentVersion(const SaveVersion& v) {
    m_current_version = v;
    m_logger->Info("Set current version to " + v.ToString());
    return true;
}

Expected<bool> SaveManager::SetSaveDirectory(const std::filesystem::path& dir) {
    try {
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        if (ec) {
            m_logger->Error("Failed to create save directory: " + ec.message());
            return SaveError::PERMISSION_DENIED;
        }
        
        m_save_dir = dir;
        m_recovery = std::make_unique<CrashRecoveryManager>(dir, m_logger.get());
        
        m_logger->Info("Set save directory to " + dir.string());
        return true;
        
    } catch (const std::exception& e) {
        m_logger->Error("Exception setting save directory: " + std::string(e.what()));
        return SaveError::UNKNOWN_ERROR;
    }
}

Expected<bool> SaveManager::SetAutoBackup(bool enabled, int max_backups) {
    if (max_backups < 0) {
        return SaveError::VALIDATION_FAILED;
    }
    
    m_auto_backup = enabled;
    m_max_backups = max_backups;
    
    m_logger->Info("Auto backup " + std::string(enabled ? "enabled" : "disabled") + 
                   " with max " + std::to_string(max_backups) + " backups");
    return true;
}

Expected<bool> SaveManager::SetMaxConcurrentOperations(size_t max_saves, size_t max_loads) {
    if (max_saves == 0 || max_loads == 0) {
        return SaveError::VALIDATION_FAILED;
    }
    
    std::unique_lock lock(m_concurrency.mtx);
    m_concurrency.max_saves = max_saves;
    m_concurrency.max_loads = max_loads;
    m_concurrency.cv.notify_all();
    
    m_logger->Info("Set concurrency limits: " + std::to_string(max_saves) + 
                   " saves, " + std::to_string(max_loads) + " loads");
    return true;
}

// ============================================================================
// Helper Methods Implementation
// ============================================================================

void SaveManager::LogDebug(const std::string& m) const { if (m_logger) m_logger->Debug("[SaveMgr] " + m); }
void SaveManager::LogInfo(const std::string& m) const { if (m_logger) m_logger->Info("[SaveMgr] " + m); }
void SaveManager::LogWarn(const std::string& m) const { if (m_logger) m_logger->Warn("[SaveMgr] " + m); }
void SaveManager::LogError(const std::string& m) const { if (m_logger) m_logger->Error("[SaveMgr] " + m); }

std::string SaveManager::MakeOperationId(bool is_save) {
    static std::atomic<uint64_t> counter{0};
    auto now = std::chrono::steady_clock::now();
    auto timestamp = now.time_since_epoch().count();
    
    std::ostringstream ss;
    ss << (is_save ? "save_" : "load_") << ++counter << "_" << timestamp;
    return ss.str();
}

SaveManager::SlotGuard::~SlotGuard() {
    if (!mgr) return;
    
    std::unique_lock lock(mgr->m_concurrency.mtx);
    if (save) {
        if (mgr->m_concurrency.active_saves > 0) {
            mgr->m_concurrency.active_saves--;
        }
    } else {
        if (mgr->m_concurrency.active_loads > 0) {
            mgr->m_concurrency.active_loads--;
        }
    }
    mgr->m_concurrency.cv.notify_all();
}

Expected<std::unique_ptr<SaveManager::SlotGuard>> SaveManager::AcquireSlot(bool save, std::chrono::seconds timeout) {
    if (timeout == std::chrono::seconds{30}) {
        timeout = m_operation_timeout;
    }
    
    std::unique_lock lock(m_concurrency.mtx);
    
    auto deadline = std::chrono::steady_clock::now() + timeout;
    
    auto predicate = [&]() {
        return save ? (m_concurrency.active_saves < m_concurrency.max_saves)
                    : (m_concurrency.active_loads < m_concurrency.max_loads);
    };
    
    if (!m_concurrency.cv.wait_until(lock, deadline, predicate)) {
        LogWarn("Timeout waiting for operation slot");
        return SaveError::CONCURRENT_LIMIT_EXCEEDED;
    }
    
    if (save) {
        m_concurrency.active_saves++;
    } else {
        m_concurrency.active_loads++;
    }
    
    size_t total_active = m_concurrency.active_saves + m_concurrency.active_loads;
    m_concurrency.peak_concurrent = std::max(m_concurrency.peak_concurrent, total_active);
    
    auto guard = std::make_unique<SlotGuard>();
    guard->mgr = this;
    guard->save = save;
    guard->acquired_at = std::chrono::steady_clock::now();
    
    return guard;
}

Expected<bool> SaveManager::CheckDiskSpace(const std::filesystem::path& dirpath, size_t estimated) const {
    try {
        auto space_result = platform::FileOperations::GetAvailableSpace(dirpath);
        if (!space_result.has_value()) {
            LogWarn("Failed to check disk space");
            return space_result.error();
        }
        
        uint64_t available = *space_result;
        
        // More accurate space calculation based on actual usage
        uint64_t required = static_cast<uint64_t>(estimated);
        if (m_auto_backup) {
            required *= 2; // Original + backup
        }
        if (m_atomic_writes_enabled) {
            required += static_cast<uint64_t>(estimated); // Temp file during atomic write
        }
        required += 50 * 1024 * 1024; // 50MB safety margin
        
        if (available < required) {
            LogError("Insufficient disk space: need " + std::to_string(required) + 
                    " bytes, have " + std::to_string(available) + " bytes");
            return false;
        }
        
        // Additional check for very low disk space (< 100MB)
        if (available < 100 * 1024 * 1024) {
            LogWarn("Low disk space warning: " + std::to_string(available / (1024 * 1024)) + " MB remaining");
            }
        
            return true;
        
         } catch (const std::exception& e) {
        LogError("Exception checking disk space: " + std::string(e.what()));
        return SaveError::UNKNOWN_ERROR;
    }
}

Json::Value SaveManager::CreateSaveHeader(const SaveVersion& version) const {
    Json::Value header;
    
    header["version"] = version.ToString();
    header["timestamp"] = static_cast<Json::Int64>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    header["game_name"] = "Mechanica Imperii";
    header["save_format"] = "json_canonical";
    header["created_by"] = "SaveManager";
    
    return header;
}

Expected<bool> SaveManager::CancelAllOperations() {
    LogInfo("Cancelling all active operations");
    
    std::shared_lock lock(m_ops_mtx);
    for (auto& [id, op] : m_active_ops) {
        if (op.progress) {
            op.progress->Cancel();
        }
    }
    
    return true;
}

bool SaveManager::WaitForOperationsToComplete(std::chrono::seconds timeout) {
    auto start = std::chrono::steady_clock::now();
    
    while (std::chrono::steady_clock::now() - start < timeout) {
        {
            std::lock_guard lock(m_concurrency.mtx);
            if (m_concurrency.active_saves == 0 && m_concurrency.active_loads == 0) {
                LogInfo("All operations completed successfully");
                return true;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::lock_guard lock(m_concurrency.mtx);
    size_t remaining = m_concurrency.active_saves + m_concurrency.active_loads;
    LogWarn("Timeout waiting for operations to complete. " + 
            std::to_string(remaining) + " operations still active");
    
    return false;
}

// Operation tracking implementation
std::string SaveManager::RegisterOperation(const std::string& filename, bool is_save) {
    std::string op_id = MakeOperationId(is_save);
    
    std::unique_lock lock(m_ops_mtx);
    ActiveOperation op;
    op.id = op_id;
    op.filename = filename;
    op.progress = std::make_shared<SaveProgress>();
    op.start_time = std::chrono::steady_clock::now();
    op.timeout_time = op.start_time + m_operation_timeout;
    op.is_save = is_save;
    op.thread_id = std::this_thread::get_id();
    
    m_active_ops[op_id] = std::move(op);
    return op_id;
}

void SaveManager::UnregisterOperation(const std::string& operation_id) {
    std::unique_lock lock(m_ops_mtx);
    m_active_ops.erase(operation_id);
}

std::vector<std::string> SaveManager::GetActiveOperations() const {
    std::shared_lock lock(m_ops_mtx);
    std::vector<std::string> active_ops;
    active_ops.reserve(m_active_ops.size());
    
    for (const auto& [id, op] : m_active_ops) {
        active_ops.push_back(id);
    }
    
    return active_ops;
}

bool SaveManager::IsOperationActive(const std::string& operation_id) const {
    std::shared_lock lock(m_ops_mtx);
    return m_active_ops.find(operation_id) != m_active_ops.end();
}

Expected<bool> SaveManager::CancelOperation(const std::string& operation_id) {
    std::shared_lock lock(m_ops_mtx);
    auto it = m_active_ops.find(operation_id);
    if (it == m_active_ops.end()) {
        return SaveError::OPERATION_CANCELLED;
    }
    
    if (it->second.progress) {
        it->second.progress->Cancel();
    }
    
    LogInfo("Cancelled operation: " + operation_id);
    return true;
}

// Performance tracking implementation
void SaveManager::RecordSaveMetrics(const SaveOperationResult& result) {
    std::unique_lock lock(m_stats_mtx);
    
    if (result.IsSuccess()) {
        m_successful_save_time += result.operation_time;
    }
    
    // Update cache stats
    m_stats.json_cache_stats = CanonicalJSONBuilder::GetCacheStats();
}

void SaveManager::RecordLoadMetrics(const SaveOperationResult& result) {
    std::unique_lock lock(m_stats_mtx);
    
    if (result.IsSuccess()) {
        m_successful_load_time += result.operation_time;
    }
}

// ============================================================================
// Core Save/Load Operations (C++17)
// ============================================================================

Expected<SaveOperationResult> SaveManager::SaveGame(const std::string& filename) {
    auto start_time = std::chrono::steady_clock::now();
    std::string operation_id = RegisterOperation(filename, true);
    
    LogInfo("Starting save operation: " + operation_id + " for file: " + filename);
    
    try {
        SaveProgress progress;
        progress.UpdateProgress(0.0, "Initializing save operation");
        
        // Acquire operation slot with proper timeout
        auto slot_guard = AcquireSlot(true, m_operation_timeout);
        if (!slot_guard.has_value()) {
            UnregisterOperation(operation_id);
            LogError("Failed to acquire save slot for operation: " + operation_id);
            return slot_guard.error();
        }
        
        // Secure path resolution
        auto resolved_path_result = SecurePathResolver::Resolve(m_save_dir, filename, m_logger.get());
        if (!resolved_path_result.has_value()) {
            UnregisterOperation(operation_id);
            LogError("Path resolution failed for: " + filename);
            return resolved_path_result.error();
        }
        auto resolved_path = *resolved_path_result;
        
        progress.UpdateProgress(5.0, "Validating systems");
        
        if (m_systems.empty()) {
            LogWarn("No systems registered for saving");
        }
        
        // Serialize game data
        progress.UpdateProgress(10.0, "Serializing game data");
        auto serialized_result = SerializeGameData(m_current_version, progress);
        if (!serialized_result.has_value()) {
            UnregisterOperation(operation_id);
            LogError("Serialization failed for operation: " + operation_id);
            return serialized_result.error();
        }
        
        auto& serialized = *serialized_result;
        
        // Check disk space before writing
        progress.UpdateProgress(75.0, "Checking disk space");
        auto space_check = CheckDiskSpace(m_save_dir, serialized.estimated_size);
        if (!space_check.has_value()) {
            UnregisterOperation(operation_id);
            return space_check.error();
        }
        
        if (!*space_check) {
            UnregisterOperation(operation_id);
            LogError("Insufficient disk space for save operation");
            return SaveError::INSUFFICIENT_SPACE;
        }
        
        // Auto-backup existing file if enabled
        progress.UpdateProgress(80.0, "Creating backup");
        bool backup_created = false;
        
        if (m_auto_backup && std::filesystem::exists(resolved_path)) {
            auto backup_result = CreateBackup(filename);
            if (backup_result.has_value()) {
                backup_created = true;
                LogInfo("Created backup for: " + filename);
            } else {
                LogWarn("Failed to create backup: " + ToString(backup_result.error()));
            }
        }
        
        // Write file using atomic operations or direct write (C++17 compatible)
        progress.UpdateProgress(85.0, "Writing save file");
        const uint8_t* data_ptr = reinterpret_cast<const uint8_t*>(serialized.canonical.data());
        size_t data_size = serialized.canonical.size();
        
        Expected<bool> write_result(SaveError::UNKNOWN_ERROR);  // Initialize with error
        if (m_atomic_writes_enabled) {
            write_result = platform::FileOperations::WriteAtomic(data_ptr, data_size, resolved_path);
        } else {
            write_result = platform::FileOperations::WriteDirect(data_ptr, data_size, resolved_path);
        }
        
        if (!write_result.has_value()) {
            UnregisterOperation(operation_id);
            LogError("Failed to write save file: " + ToString(write_result.error()));
            return write_result.error();
        }
        
        // Sync directory for durability
        auto sync_result = platform::FileOperations::SyncDirectory(m_save_dir);
        if (!sync_result.has_value()) {
            LogWarn("Directory sync failed - save may not be durable");
        }
        
        // Cleanup old backups
        if (m_auto_backup && m_max_backups > 0) {
            CleanupOldBackups(filename);
        }
        
        progress.UpdateProgress(100.0, "Save complete");
        
        // Prepare result
        SaveOperationResult result;
        result.result = SaveResult::SUCCESS;
        result.operation_id = operation_id;
        result.version_saved = m_current_version;
        result.bytes_written = serialized.canonical.size();
        result.estimated_size = serialized.estimated_size;
        result.sha256_checksum = serialized.sha256;
        result.atomic_write_used = m_atomic_writes_enabled;
        result.backup_created = backup_created;
        
        result.operation_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time);
        
        LogInfo("Save operation completed successfully: " + operation_id + 
                " (" + std::to_string(result.operation_time.count()) + "ms)");
        
        RecordSaveMetrics(result);
        
        // Update statistics
        std::unique_lock stats_lock(m_stats_mtx);
        m_stats.total_saves++;
        m_stats.successful_saves++;
        m_stats.total_bytes_saved += serialized.canonical.size();
        
        UnregisterOperation(operation_id);
        return result;
        
    } catch (const std::exception& e) {
        UnregisterOperation(operation_id);
        LogError("Exception in save operation " + operation_id + ": " + std::string(e.what()));
        
        std::unique_lock stats_lock(m_stats_mtx);
        m_stats.total_saves++;
        m_stats.failed_saves++;
        
        return SaveError::UNKNOWN_ERROR;
    }
}

Expected<SaveOperationResult> SaveManager::SaveGameAtomic(const std::string& filename) {
    bool original_atomic_setting = m_atomic_writes_enabled;
    m_atomic_writes_enabled = true;
    
    auto result = SaveGame(filename);
    
    m_atomic_writes_enabled = original_atomic_setting;
    return result;
}

// ============================================================================
// Initialization Methods (C++17)
// ============================================================================

void SaveManager::InitializeBuiltinValidators() {
    // Register default validators
    RegisterValidator("structure", [this](const Json::Value& data, const std::vector<std::string>& systems) -> ValidationReport {
        ValidationReport report;
        
        if (!data.isMember("header")) {
            report.AddCritical("structure", "", "Missing header section", "Ensure save file has valid header");
        } else {
            const Json::Value& header = data["header"];
            if (!header.isMember("version")) {
                report.AddError("structure", "header", "Missing version field", "Add version to header");
            }
            if (!header.isMember("game_name")) {
                report.AddError("structure", "header", "Missing game_name field", "Add game_name to header");
            }
        }
        
        if (!data.isMember("systems")) {
            report.AddCritical("structure", "", "Missing systems section", "Ensure save file has systems data");
        }
        
        return report;
    });
    
    RegisterValidator("systems", [this](const Json::Value& data, const std::vector<std::string>& expected_systems) -> ValidationReport {
        ValidationReport report;
        
        if (data.isMember("systems")) {
            const Json::Value& systems = data["systems"];
            
            // Check for expected systems
            for (const auto& system_name : expected_systems) {
                if (!systems.isMember(system_name)) {
                    report.AddWarning("systems", "systems", 
                        "Missing expected system: " + system_name, 
                        "System will use default state");
                }
            }
            
            // Check for unknown systems
            auto member_names = systems.getMemberNames();
            for (const auto& name : member_names) {
                if (std::find(expected_systems.begin(), expected_systems.end(), name) == expected_systems.end()) {
                    report.AddWarning("systems", "systems." + name, 
                        "Unknown system in save file: " + name, 
                        "System data will be ignored");
                }
            }
        }
        
        return report;
    });
}

void SaveManager::RegisterValidator(const std::string& name, ValidationCallback validator) {
    std::unique_lock lock(m_val_mtx);
    m_validators[name] = std::move(validator);
    LogDebug("Registered validator: " + name);
}

// Additional helper for C++17 string compatibility
Expected<std::filesystem::path> SaveManager::CanonicalSavePath(const std::string& filename) const {
    return SecurePathResolver::Resolve(m_save_dir, filename, m_logger.get());
}

} // namespace core::save
