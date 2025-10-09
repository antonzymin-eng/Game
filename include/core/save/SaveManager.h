// Created: September 18, 2025 - 12:00:00
// Location: include/core/save/SaveManager.h
// Mechanica Imperii - Production-Ready Save System (Final)
//
// Addresses all identified issues:
// - Structured ValidationReport with detailed failure reasons
// - std::expected for error handling consistency
// - Logger interface injection with configurable levels
// - Safe resource cleanup with timeout guarantees
// - Cached JSON canonicalization for performance
// - Proper shared_ptr ownership semantics
// - Platform abstraction for file operations
// - CLI verification tool hooks
// - Fuzz testing and chaos engineering support

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <future>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <chrono>
#include <unordered_map>
#include <filesystem>
#include <cstdint>
#include <optional>
#include <variant>

#include <jsoncpp/json/json.h>

namespace core { namespace ecs {
    struct ISerializable {
        virtual ~ISerializable() = default;
        virtual std::string GetSystemName() const = 0;
        virtual bool Serialize(Json::Value& out, int version_int) = 0;
        virtual bool Deserialize(const Json::Value& in, int version_int) = 0;
    };
}} // namespace core::ecs

namespace core::save {

// ============================================================================
// Enhanced Logger Interface with Configurable Levels
// ============================================================================

enum class LogLevel { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3, NONE = 4 };

struct ILogger {
    virtual ~ILogger() = default;
    virtual void SetLevel(LogLevel level) = 0;
    virtual LogLevel GetLevel() const = 0;
    virtual void Debug(const std::string& msg) = 0;
    virtual void Info (const std::string& msg) = 0;
    virtual void Warn (const std::string& msg) = 0;
    virtual void Error(const std::string& msg) = 0;
    
    // Structured logging for metrics/telemetry
    virtual void LogMetric(const std::string& name, double value, const std::unordered_map<std::string, std::string>& tags = {}) {}
};

struct DefaultLogger final : ILogger {
    explicit DefaultLogger(LogLevel level = LogLevel::INFO);
    void SetLevel(LogLevel level) override { m_level = level; }
    LogLevel GetLevel() const override { return m_level; }
    void Debug(const std::string& msg) override;
    void Info (const std::string& msg) override;
    void Warn (const std::string& msg) override;
    void Error(const std::string& msg) override;
    void LogMetric(const std::string& name, double value, const std::unordered_map<std::string, std::string>& tags = {}) override;
private:
    std::atomic<LogLevel> m_level{LogLevel::INFO};
};

// ============================================================================
// Error Handling with std::expected
// ============================================================================

enum class SaveError {
    NONE = 0,
    INVALID_FILENAME,
    PATH_TRAVERSAL,
    INSUFFICIENT_SPACE,
    PERMISSION_DENIED,
    FILE_NOT_FOUND,
    CORRUPTION_DETECTED,
    SERIALIZATION_FAILED,
    VALIDATION_FAILED,
    MIGRATION_FAILED,
    CHECKSUM_MISMATCH,
    CONCURRENT_LIMIT_EXCEEDED,
    OPERATION_CANCELLED,
    UNKNOWN_ERROR
};

std::string ToString(SaveError error);

// C++17 compatible Expected implementation
template<typename T>
class Expected {
private:
    std::variant<T, SaveError> data;
    
public:
    Expected(const T& value) : data(value) {}
    Expected(const SaveError& error) : data(error) {}
    
    bool has_value() const { return std::holds_alternative<T>(data); }
    const T& value() const { return std::get<T>(data); }
    T& value() { return std::get<T>(data); }
    const SaveError& error() const { return std::get<SaveError>(data); }
    
    operator bool() const { return has_value(); }
    const T& operator*() const { return value(); }
    T& operator*() { return value(); }
};

// Specialization for Expected<void>
template<>
class Expected<void> {
private:
    std::variant<bool, SaveError> data;
    
public:
    Expected() : data(true) {}
    Expected(const SaveError& error) : data(error) {}
    
    bool has_value() const { return std::holds_alternative<bool>(data); }
    const SaveError& error() const { return std::get<SaveError>(data); }
    
    operator bool() const { return has_value(); }
};

template<typename E>
Expected<void> unexpected(const E& error) {
    return Expected<void>(static_cast<SaveError>(error));
}

// ============================================================================
// Versioning System
// ============================================================================

struct SaveVersion {
    int major = 1, minor = 0, patch = 0;
    std::string build_hash;
    std::chrono::system_clock::time_point created_time;

    SaveVersion() = default;
    SaveVersion(int maj, int min, int pat, const std::string& hash = "");

    std::string ToString() const;
    static Expected<SaveVersion> FromString(const std::string& s);
    static bool IsValidVersionString(const std::string& s);

    bool operator==(const SaveVersion& o) const;
    bool operator!=(const SaveVersion& o) const;
    bool operator< (const SaveVersion& o) const;
    bool operator<=(const SaveVersion& o) const;
    bool operator> (const SaveVersion& o) const;
    bool operator>=(const SaveVersion& o) const;

    int  ToInt() const;
    static SaveVersion FromInt(int v);
    bool IsCompatibleWith(const SaveVersion& other) const;
};

// ============================================================================
// Structured Validation Reporting
// ============================================================================

struct ValidationIssue {
    enum Severity { WARNING, ERROR, CRITICAL };
    
    Severity severity;
    std::string validator_name;
    std::string field_path;  // JSON path like "systems.provinces[3].population"
    std::string message;
    std::optional<std::string> suggested_fix;
    
    ValidationIssue(Severity sev, const std::string& validator, const std::string& path, 
                   const std::string& msg, const std::optional<std::string>& fix = std::nullopt)
        : severity(sev), validator_name(validator), field_path(path), message(msg), suggested_fix(fix) {}
};

struct ValidationReport {
    bool passed = true;
    std::vector<ValidationIssue> issues;
    std::chrono::milliseconds validation_time{0};
    
    void AddError(const std::string& validator, const std::string& path, const std::string& message, 
                  const std::optional<std::string>& fix = std::nullopt);
    void AddWarning(const std::string& validator, const std::string& path, const std::string& message,
                    const std::optional<std::string>& fix = std::nullopt);
    void AddCritical(const std::string& validator, const std::string& path, const std::string& message,
                     const std::optional<std::string>& fix = std::nullopt);
    
    bool IsValid() const { return passed && GetErrorCount() == 0; }
    size_t GetErrorCount() const;
    size_t GetWarningCount() const;
    size_t GetCriticalCount() const;
    
    std::string GenerateReport() const;
    Json::Value ToJson() const;  // For CLI tools
};

// ============================================================================
// Enhanced Result Types with Rich Error Information
// ============================================================================

enum class SaveResult {
    SUCCESS,
    IN_PROGRESS,
    CANCELLED,
    FILE_ERROR,
    SERIALIZATION_ERROR,
    VALIDATION_ERROR,
    MIGRATION_ERROR,
    BACKUP_ERROR,
    VERSION_ERROR,
    ATOMIC_WRITE_FAILED,
    CORRUPTION_DETECTED,
    INSUFFICIENT_SPACE,
    THREAD_SAFETY_ERROR,
    CHECKSUM_MISMATCH,
    CONCURRENT_OPERATION_LIMIT,
    INVALID_FILENAME,
    PERMISSION_DENIED
};

struct SaveProgress {
    std::atomic<double> percentage{0.0};
    std::atomic<bool> is_complete{false};
    std::atomic<bool> is_cancelled{false};
    std::string current_operation;
    std::chrono::steady_clock::time_point start_time{};
    std::chrono::steady_clock::time_point estimated_completion{};

    void UpdateProgress(double percent, const std::string& op);
    double GetEstimatedSecondsRemaining() const;
    std::string GetFormattedTimeRemaining() const;
    void Cancel() { is_cancelled.store(true); }
    bool IsCancelled() const { return is_cancelled.load(); }
};

struct SaveOperationResult {
    SaveResult result = SaveResult::SUCCESS;
    std::string message;
    std::vector<std::string> warnings;
    SaveVersion version_loaded;
    SaveVersion version_saved;
    bool migration_performed = false;
    bool atomic_write_used = false;
    bool backup_created = false;
    std::string operation_id;

    std::shared_ptr<SaveProgress> progress;
    std::chrono::milliseconds operation_time{0};
    size_t bytes_written = 0;
    size_t estimated_size = 0;
    std::string sha256_checksum;
    
    // Enhanced error reporting
    ValidationReport validation_report;
    std::vector<std::string> migration_steps;
    std::unordered_map<std::string, std::string> debug_info;

    bool IsSuccess() const;
    bool IsInProgress() const;
    std::string GetFullReport() const;
    Json::Value ToJson() const;  // For CLI tools and telemetry

private:
    std::string ResultToString(SaveResult r) const;
};

// ============================================================================
// Platform-Abstracted File Operations
// ============================================================================

namespace platform {
    struct FileOperations {
        static Expected<bool> WriteAtomic(const uint8_t* data, size_t size, const std::filesystem::path& filepath);
        static Expected<bool> WriteDirect(const uint8_t* data, size_t size, const std::filesystem::path& filepath);
        static Expected<std::vector<uint8_t>> ReadFile(const std::filesystem::path& filepath);
        static Expected<bool> SyncDirectory(const std::filesystem::path& dir_path);
        static Expected<uint64_t> GetAvailableSpace(const std::filesystem::path& path);
    };
}

// ============================================================================
// Optimized Canonical JSON with Caching
// ============================================================================

class CanonicalJSONBuilder {
public:
    struct CacheStats {
        size_t hits = 0;
        size_t misses = 0;
        size_t evictions = 0;
        double hit_ratio() const { return (hits + misses) > 0 ? static_cast<double>(hits) / (hits + misses) : 0.0; }
    };

    // Thread-safe canonical JSON generation with LRU caching
    static std::string Build(const Json::Value& root, bool exclude_checksum = false);
    static void ClearCache();
    static CacheStats GetCacheStats();
    static void SetCacheSize(size_t max_entries);

private:
    static Json::Value SortKeysRecursive(const Json::Value& v);
    
    // LRU cache implementation details (private)
    struct CacheEntry;
    static std::unordered_map<std::string, std::unique_ptr<CacheEntry>> s_cache;
    static std::mutex s_cache_mutex;
    static size_t s_max_cache_size;
    static CacheStats s_cache_stats;
};

// ============================================================================
// Secure Path Resolution with std::expected
// ============================================================================

class SecurePathResolver {
public:
    enum class PathError {
        EMPTY_FILENAME,
        TOO_LONG,
        INVALID_CHARACTERS,
        RESERVED_NAME,
        ABSOLUTE_PATH,
        PATH_TRAVERSAL,
        ESCAPES_BASE_DIR,
        CANONICALIZATION_FAILED
    };

    // Returns canonical save file path or detailed error information
    static Expected<std::filesystem::path> Resolve(const std::filesystem::path& base_dir, 
                                                  const std::string& filename,
                                                  ILogger* logger = nullptr);
    
    static std::string ToString(PathError error);
    static bool IsWindowsReserved(const std::string& name);
    
    // For CLI verification tools
    static ValidationReport ValidateFilename(const std::string& filename);
};

// ============================================================================
// Enhanced Migration System
// ============================================================================

class SaveMigration {
public:
    SaveVersion from_version;
    SaveVersion to_version;
    std::string description;
    std::function<Expected<bool>(Json::Value&, ILogger*)> migrate_func;  // Enhanced with logging

    SaveMigration(const SaveVersion& from, const SaveVersion& to, const std::string& desc,
                  std::function<Expected<bool>(Json::Value&, ILogger*)> func);
};

class MigrationRegistry {
public:
    static MigrationRegistry& Instance();
    
    void RegisterMigration(const SaveVersion& from, const SaveVersion& to,
                           const std::string& description,
                           std::function<Expected<bool>(Json::Value&, ILogger*)> migration_func);
    
    Expected<std::vector<SaveMigration>> FindMigrationPath(const SaveVersion& from, const SaveVersion& to) const;
    bool IsMigrationSupported(const SaveVersion& from, const SaveVersion& to) const;
    std::vector<std::string> GetMigrationPreview(const SaveVersion& from, const SaveVersion& to) const;
    void InitializeDefaultMigrations();

private:
    std::vector<SaveMigration> m_migrations;
    mutable std::shared_mutex m_mutex;
    Expected<std::vector<SaveMigration>> BFS(const SaveVersion& from, const SaveVersion& to) const;
    MigrationRegistry() = default;
};

// ============================================================================
// Enhanced Crash Recovery
// ============================================================================

class CrashRecoveryManager {
public:
    explicit CrashRecoveryManager(const std::filesystem::path& save_dir, ILogger* logger = nullptr);

    Expected<std::vector<std::filesystem::path>> FindIncompleteOperations() const;
    Expected<std::vector<std::filesystem::path>> FindCorruptedSaves() const;
    Expected<std::vector<std::filesystem::path>> FindRecoverableBackups() const;
    Expected<bool> AttemptRecovery(const std::filesystem::path& save_file);
    Expected<bool> ValidateSaveIntegrity(const std::filesystem::path& save_file) const;

    Expected<bool> CleanupTempFiles();
    Expected<bool> CleanupIncompleteOperations();
    Expected<bool> CleanupOldBackups(const std::filesystem::path& save_file, int max_backups);

    struct RecoveryStats {
        size_t corrupted_files_found = 0;
        size_t successful_recoveries = 0;
        size_t failed_recoveries = 0;
        size_t temp_files_cleaned = 0;
        size_t backups_cleaned = 0;
        
        Json::Value ToJson() const;
    };

    RecoveryStats GetRecoveryStats() const;
    void ResetStats();

private:
    std::filesystem::path m_dir;
    ILogger* m_logger;
    mutable std::mutex m_stats_mutex;
    mutable RecoveryStats m_stats{};

    Expected<bool> IsFileCorrupted(const std::filesystem::path& p) const;
    Expected<std::filesystem::path> FindBestBackup(const std::filesystem::path& save_file) const;
    Expected<bool> ValidateFileStructure(const std::filesystem::path& p) const;
    Expected<bool> ValidateJSONIntegrity(const std::filesystem::path& p) const;
    Expected<bool> ValidateGameHeader(const Json::Value& root) const;
};

// ============================================================================
// Chaos Engineering and Fuzz Testing Support
// ============================================================================

namespace testing {
    class ChaosManager {
    public:
        enum class ChaosType {
            NONE,
            CORRUPT_RANDOM_BYTES,    // Flip random bits in save data
            TRUNCATE_FILE,           // Cut off end of file
            INJECT_PARSE_ERRORS,     // Insert invalid JSON
            SIMULATE_DISK_FULL,      // Return disk space errors
            DELAY_OPERATIONS,        // Add artificial latency
            FAIL_ATOMIC_WRITES       // Force atomic write failures
        };
        
        static void EnableChaos(ChaosType type, double probability = 0.1);
        static void DisableChaos();
        static bool ShouldInjectFailure(ChaosType type);
        static std::vector<uint8_t> CorruptData(const std::vector<uint8_t>& data, double corruption_rate = 0.01);
    };

    // For integration with fuzzing frameworks
    struct FuzzHooks {
        static void SetCustomData(const std::vector<uint8_t>& fuzz_data);
        static std::vector<uint8_t> GetLastSaveData();
        static ValidationReport ValidateWithFuzzData(const std::string& filename);
    };
}

// ============================================================================
// Enhanced SaveManager with Production Features
// ============================================================================

class SaveManager {
public:
    using ProgressCallback   = std::function<void(const SaveProgress&)>;
    using CompletionCallback = std::function<void(const SaveOperationResult&)>;
    using ValidationCallback = std::function<ValidationReport(const Json::Value&, const std::vector<std::string>&)>;

    // Enhanced constructor with configurable options
    struct Config {
        std::unique_ptr<ILogger> logger = nullptr;
        size_t max_concurrent_saves = 2;
        size_t max_concurrent_loads = 4;
        bool enable_atomic_writes = true;
        bool enable_auto_backup = true;
        int max_backups = 10;
        std::chrono::seconds operation_timeout{300};  // 5 minutes default
        size_t json_cache_size = 100;
        bool enable_validation_caching = true;
    };

    explicit SaveManager(Config config = {});
    ~SaveManager();

    // Safe system registration with proper ownership
    void RegisterSystem(std::shared_ptr<core::ecs::ISerializable> system);
    
    template<typename T>
    Expected<bool> RegisterSystemOwned(std::unique_ptr<T> system) {
        if (!system) return std::unexpected(SaveError::UNKNOWN_ERROR);
        auto shared = std::shared_ptr<T>(system.release());
        RegisterSystem(std::static_pointer_cast<core::ecs::ISerializable>(shared));
        return true;
    }

    // Configuration with validation
    Expected<bool> SetCurrentVersion(const SaveVersion& v);
    Expected<bool> SetSaveDirectory(const std::filesystem::path& dir);
    Expected<bool> SetAutoBackup(bool enabled, int max_backups = 10);
    void SetAtomicWrites(bool enabled) { m_atomic_writes_enabled = enabled; }
    Expected<bool> SetMaxConcurrentOperations(size_t max_saves, size_t max_loads);

    // Core save operations with enhanced error handling
    Expected<SaveOperationResult> SaveGame(const std::string& filename);
    Expected<SaveOperationResult> SaveGameAtomic(const std::string& filename);

    std::future<Expected<SaveOperationResult>> SaveGameAsync(
        const std::string& filename,
        ProgressCallback progress_cb = nullptr,
        CompletionCallback completion_cb = nullptr);

    // Core load operations with enhanced error handling
    Expected<SaveOperationResult> LoadGame(const std::string& filename);

    std::future<Expected<SaveOperationResult>> LoadGameAsync(
        const std::string& filename,
        ProgressCallback progress_cb = nullptr,
        CompletionCallback completion_cb = nullptr);

    // Enhanced crash recovery
    Expected<SaveOperationResult> RecoverFromCrash();
    Expected<std::vector<std::filesystem::path>> FindRecoverableSaves() const;
    Expected<bool> ValidateSaveIntegrity(const std::string& filename) const;

    // Backup management with validation
    Expected<SaveOperationResult> CreateBackup(const std::string& filename, const std::string& backup_name = "");
    Expected<SaveOperationResult> RestoreBackup(const std::string& filename, const std::string& backup_name);
    Expected<std::vector<std::filesystem::path>> GetBackupList(const std::string& filename) const;
    Expected<bool> CleanupOldBackups(const std::string& filename);

    // Migration operations with detailed reporting
    Expected<SaveOperationResult> MigrateSave(const std::string& filename, const SaveVersion& target_version);
    Expected<std::vector<std::string>> GetMigrationPreview(const std::string& filename) const;
    Expected<bool> IsMigrationRequired(const std::string& filename) const;

    // Enhanced file management
    Expected<std::vector<std::filesystem::path>> GetSaveFileList() const;
    Expected<bool> SaveFileExists(const std::string& filename) const;
    Expected<SaveVersion> GetSaveFileVersion(const std::string& filename) const;
    Expected<bool> DeleteSaveFile(const std::string& filename);
    Expected<size_t> GetSaveFileSize(const std::string& filename) const;
    Expected<std::chrono::system_clock::time_point> GetSaveFileTimestamp(const std::string& filename) const;

    // Operation management with timeout handling
    Expected<bool> CancelOperation(const std::string& operation_id);
    std::vector<std::string> GetActiveOperations() const;
    bool IsOperationActive(const std::string& operation_id) const;
    Expected<bool> CancelAllOperations();

    // Enhanced validation with structured reporting
    Expected<ValidationReport> ValidateSave(const std::string& filename) const;
    Expected<bool> VerifyChecksum(const std::string& filename) const;

    // Custom validators with structured reporting
    void RegisterValidator(const std::string& name, ValidationCallback validator);

    // Enhanced statistics with telemetry support
    struct SaveStats {
        size_t total_saves = 0, successful_saves = 0, failed_saves = 0, cancelled_saves = 0;
        size_t total_loads = 0, successful_loads = 0, failed_loads = 0, cancelled_loads = 0;
        size_t corrupted_saves_recovered = 0, migrations_performed = 0;
        std::chrono::milliseconds average_save_time{0}, average_load_time{0};
        size_t total_bytes_saved = 0;
        
        // Performance metrics
        CanonicalJSONBuilder::CacheStats json_cache_stats;
        double validation_cache_hit_ratio = 0.0;
        size_t concurrent_operations_peak = 0;
        
        Json::Value ToJson() const;
        double GetSaveSuccessRate() const;
        double GetLoadSuccessRate() const;
    };
    
    SaveStats GetSaveStats() const;
    void ResetSaveStats();
    
    // Logger management
    void SetLogger(std::unique_ptr<ILogger> logger);
    ILogger* GetLogger() const { return m_logger.get(); }

    // CLI and tooling support
    struct VerificationOptions {
        bool check_structure = true;
        bool check_checksums = true;
        bool run_validators = true;
        bool check_migrations = true;
        bool verbose = false;
    };
    
    Expected<ValidationReport> VerifyFile(const std::string& filename, const VerificationOptions& options = {}) const;
    Json::Value GetSystemInfo() const;  // For diagnostics

private:
    // Core components with enhanced error handling
    std::vector<std::shared_ptr<core::ecs::ISerializable>> m_systems;
    std::unique_ptr<CrashRecoveryManager> m_recovery;
    std::unique_ptr<ILogger> m_logger;

    // Configuration
    SaveVersion m_current_version{1,0,0};
    std::filesystem::path m_save_dir{"saves"};
    bool m_auto_backup = true;
    int m_max_backups = 10;
    bool m_atomic_writes_enabled = true;
    std::chrono::seconds m_operation_timeout{300};

    // Enhanced concurrency management with timeout support
    struct Concurrency {
        std::mutex mtx;
        std::condition_variable cv;
        size_t max_saves = 2, max_loads = 4;
        size_t active_saves = 0, active_loads = 0;
        size_t peak_concurrent = 0;
    } m_concurrency;

    // Enhanced statistics tracking
    mutable std::shared_mutex m_stats_mtx;
    SaveStats m_stats{};
    std::chrono::milliseconds m_successful_save_time{0}, m_successful_load_time{0};

    // Operation tracking with timeout management
    struct ActiveOperation {
        std::string id;
        std::string filename;
        std::shared_ptr<SaveProgress> progress;
        std::chrono::steady_clock::time_point start_time{};
        std::chrono::steady_clock::time_point timeout_time{};
        bool is_save = true;
        std::thread::id thread_id{};
    };
    mutable std::shared_mutex m_ops_mtx;
    std::unordered_map<std::string, ActiveOperation> m_active_ops;

    // Enhanced validators with caching
    mutable std::shared_mutex m_val_mtx;
    std::unordered_map<std::string, ValidationCallback> m_validators;
    mutable std::unordered_map<std::string, ValidationReport> m_validation_cache;
    mutable size_t m_validation_cache_hits = 0;
    mutable size_t m_validation_cache_misses = 0;

    // Helper methods with enhanced error handling
    void LogDebug(const std::string& m) const;
    void LogInfo (const std::string& m) const;
    void LogWarn (const std::string& m) const;
    void LogError(const std::string& m) const;

    static std::string MakeOperationId(bool is_save);

    // Resource management with timeout guarantees
    struct SlotGuard {
        SaveManager* mgr; 
        bool save;
        std::chrono::steady_clock::time_point acquired_at;
        ~SlotGuard();
    };
    Expected<std::unique_ptr<SlotGuard>> AcquireSlot(bool save, std::chrono::seconds timeout = std::chrono::seconds{30});

    // Enhanced serialization with error reporting
    struct SerializedData { 
        std::string canonical; 
        size_t estimated_size = 0; 
        std::string sha256; 
        std::chrono::milliseconds serialization_time{0};
    };
    
    Expected<SerializedData> SerializeGameData(const SaveVersion& v, SaveProgress& prog);
    Expected<bool> DeserializeGameData(const Json::Value& save, SaveProgress& prog);

    // Header management with validation
    Json::Value CreateSaveHeader(const SaveVersion& v) const;
    Expected<bool> ValidateSaveHeader(const Json::Value& h) const;
    Expected<SaveVersion> ExtractVersionFromHeader(const Json::Value& h) const;

    // Enhanced file operations
    Expected<bool> ReadJson(Json::Value& out, const std::filesystem::path& filepath) const;

    // Migration with structured error reporting
    Expected<SaveOperationResult> PerformMigration(Json::Value& data, const SaveVersion& from, const SaveVersion& to);

    // Enhanced validation with caching
    Expected<ValidationReport> ValidateGameData(const Json::Value& data) const;
    std::vector<std::string> GetRegisteredSystemNames() const;

    // Path and backup utilities
    Expected<std::filesystem::path> GenerateBackupPath(const std::string& filename) const;
    Expected<std::filesystem::path> CanonicalSavePath(const std::string& filename) const;

    // Enhanced operation tracking with cleanup guarantees
    std::string RegisterOperation(const std::string& filename, bool is_save);
    void UnregisterOperation(const std::string& operation_id);
    void CleanupTimedOutOperations();
    
    // Statistics and performance tracking
    void RecordSaveMetrics(const SaveOperationResult& r);
    void RecordLoadMetrics(const SaveOperationResult& r);

    // Resource validation
    Expected<bool> CheckDiskSpace(const std::filesystem::path& dirpath, size_t estimated) const;

    // Cryptographic operations
    static Expected<std::string> SHA256(const uint8_t* data, size_t size);

    // Built-in validators with enhanced reporting
    void InitializeBuiltinValidators();
    
    // Cleanup with timeout guarantees
    bool WaitForOperationsToComplete(std::chrono::seconds timeout);
};

} // namespace core::save
