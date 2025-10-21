// Created: September 18, 2025 - 14:45:00 (Updated for C++17)
// Location: src/core/save/SaveManagerRecovery.cpp
// Mechanica Imperii - SaveManager Recovery Implementation (C++17 Compliant)

#include "core/save/SaveManager.h"
#include "utils/PlatformCompat.h"
#include <future>
#include <algorithm>
#include <random>
#include <ctime>
#include <fstream>
#include <sstream>

namespace core::save {

// C++17 helper functions
static bool string_starts_with(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && 
           str.compare(0, prefix.size(), prefix) == 0;
}

static bool string_ends_with(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && 
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// ============================================================================
// Enhanced Crash Recovery Implementation (C++17)
// ============================================================================

CrashRecoveryManager::CrashRecoveryManager(const std::filesystem::path& save_dir, ILogger* logger)
    : m_dir(save_dir), m_logger(logger) {
    if (m_logger) {
        m_logger->Info("CrashRecoveryManager initialized for directory: " + save_dir.string());
    }
}

Expected<std::vector<std::filesystem::path>> CrashRecoveryManager::FindIncompleteOperations() const {
    std::vector<std::filesystem::path> incomplete_files;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(m_dir)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                
                // Look for temporary files that indicate incomplete operations
                if (filename.find(".tmp.") != std::string::npos ||
                    string_ends_with(filename, ".tmp") ||
                    string_ends_with(filename, ".partial") ||
                    string_ends_with(filename, ".writing")) {
                    incomplete_files.push_back(entry.path());
                }
            }
        }
        
        return incomplete_files;
        
    } catch (const std::exception& e) {
        if (m_logger) {
            m_logger->Error("Exception finding incomplete operations: " + std::string(e.what()));
        }
        return SaveError::UNKNOWN_ERROR;
    }
}

Expected<std::vector<std::filesystem::path>> CrashRecoveryManager::FindCorruptedSaves() const {
    std::vector<std::filesystem::path> corrupted_files;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(m_dir)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                
                if (string_ends_with(filename, ".save") && filename.find("_backup_") == std::string::npos) {
                    auto corruption_check = IsFileCorrupted(entry.path());
                    if (corruption_check.has_value() && *corruption_check) {
                        corrupted_files.push_back(entry.path());
                    }
                }
            }
        }
        
        return corrupted_files;
        
    } catch (const std::exception& e) {
        if (m_logger) {
            m_logger->Error("Exception finding corrupted saves: " + std::string(e.what()));
        }
        return SaveError::UNKNOWN_ERROR;
    }
}

Expected<std::vector<std::filesystem::path>> CrashRecoveryManager::FindRecoverableBackups() const {
    std::vector<std::filesystem::path> backup_files;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(m_dir)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                
                if (filename.find("_backup_") != std::string::npos && string_ends_with(filename, ".save")) {
                    // Verify backup is not corrupted
                    auto corruption_check = IsFileCorrupted(entry.path());
                    if (corruption_check.has_value() && !*corruption_check) {
                        backup_files.push_back(entry.path());
                    }
                }
            }
        }
        
        // Sort by modification time (newest first)
        std::sort(backup_files.begin(), backup_files.end(),
            [](const std::filesystem::path& a, const std::filesystem::path& b) {
                return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
            });
        
        return backup_files;
        
    } catch (const std::exception& e) {
        if (m_logger) {
            m_logger->Error("Exception finding recoverable backups: " + std::string(e.what()));
        }
        return SaveError::UNKNOWN_ERROR;
    }
}

Expected<bool> CrashRecoveryManager::AttemptRecovery(const std::filesystem::path& save_file) {
    std::lock_guard lock(m_stats_mutex);
    
    try {
        if (m_logger) {
            m_logger->Info("Attempting recovery for: " + save_file.string());
        }
        
        // First, try to find the best backup
        auto backup_result = FindBestBackup(save_file);
        if (!backup_result.has_value()) {
            m_stats.failed_recoveries++;
            return backup_result.error();
        }
        
        auto backup_path = *backup_result;
        
        // Verify the backup is intact
        auto validation_result = ValidateSaveIntegrity(backup_path);
        if (!validation_result.has_value() || !*validation_result) {
            if (m_logger) {
                m_logger->Error("Backup file is also corrupted: " + backup_path.string());
            }
            m_stats.failed_recoveries++;
            return SaveError::CORRUPTION_DETECTED;
        }
        
        // Create a backup of the corrupted file before overwriting
        std::string corrupted_backup_name = save_file.stem().string() + "_corrupted_" + 
            std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".save";
        auto corrupted_backup_path = save_file.parent_path() / corrupted_backup_name;
        
        std::error_code ec;
        std::filesystem::copy_file(save_file, corrupted_backup_path, ec);
        
        // Restore from backup
        std::filesystem::copy_file(backup_path, save_file, 
                                 std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            if (m_logger) {
                m_logger->Error("Failed to restore from backup: " + ec.message());
            }
            m_stats.failed_recoveries++;
            return SaveError::PERMISSION_DENIED;
        }
        
        if (m_logger) {
            m_logger->Info("Successfully recovered " + save_file.string() + " from backup: " + backup_path.string());
        }
        
        m_stats.successful_recoveries++;
        return true;
        
    } catch (const std::exception& e) {
        if (m_logger) {
            m_logger->Error("Exception during recovery: " + std::string(e.what()));
        }
        m_stats.failed_recoveries++;
        return SaveError::UNKNOWN_ERROR;
    }
}

Expected<bool> CrashRecoveryManager::ValidateSaveIntegrity(const std::filesystem::path& save_file) const {
    try {
        // Check file structure
        auto structure_result = ValidateFileStructure(save_file);
        if (!structure_result.has_value()) {
            return structure_result;
        }
        
        // Check JSON integrity
        auto json_result = ValidateJSONIntegrity(save_file);
        if (!json_result.has_value()) {
            return json_result;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        if (m_logger) {
            m_logger->Error("Exception during integrity validation: " + std::string(e.what()));
        }
        return SaveError::UNKNOWN_ERROR;
    }
}

Expected<bool> CrashRecoveryManager::CleanupTempFiles() {
    std::lock_guard lock(m_stats_mutex);
    size_t cleaned_count = 0;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(m_dir)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                
                if (filename.find(".tmp.") != std::string::npos ||
                    string_ends_with(filename, ".tmp") ||
                    string_ends_with(filename, ".partial") ||
                    string_ends_with(filename, ".writing")) {
                    
                    std::error_code ec;
                    std::filesystem::remove(entry.path(), ec);
                    if (!ec) {
                        cleaned_count++;
                        if (m_logger) {
                            m_logger->Debug("Removed temp file: " + filename);
                        }
                    }
                }
            }
        }
        
        m_stats.temp_files_cleaned += cleaned_count;
        
        if (m_logger) {
            m_logger->Info("Cleaned up " + std::to_string(cleaned_count) + " temporary files");
        }
        
        return true;
        
    } catch (const std::exception& e) {
        if (m_logger) {
            m_logger->Error("Exception during temp file cleanup: " + std::string(e.what()));
        }
        return SaveError::UNKNOWN_ERROR;
    }
}

Expected<bool> CrashRecoveryManager::CleanupOldBackups(const std::filesystem::path& save_file, int max_backups) {
    try {
        std::string base_name = save_file.stem().string();
        std::string backup_prefix = base_name + "_backup_";
        
        std::vector<std::filesystem::path> backup_files;
        
        for (const auto& entry : std::filesystem::directory_iterator(save_file.parent_path())) {
            if (entry.is_regular_file()) {
                std::string backup_filename = entry.path().filename().string();
                if (string_starts_with(backup_filename, backup_prefix)) {
                    backup_files.push_back(entry.path());
                }
            }
        }
        
        // Sort by modification time (newest first)
        std::sort(backup_files.begin(), backup_files.end(),
            [](const std::filesystem::path& a, const std::filesystem::path& b) {
                return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
            });
        
        // Remove excess backups
        size_t removed_count = 0;
        if (backup_files.size() > static_cast<size_t>(max_backups)) {
            for (size_t i = max_backups; i < backup_files.size(); ++i) {
                std::error_code ec;
                std::filesystem::remove(backup_files[i], ec);
                if (!ec) {
                    removed_count++;
                    if (m_logger) {
                        m_logger->Debug("Removed old backup: " + backup_files[i].string());
                    }
                }
            }
        }
        
        std::lock_guard lock(m_stats_mutex);
        m_stats.backups_cleaned += removed_count;
        
        return true;
        
    } catch (const std::exception& e) {
        if (m_logger) {
            m_logger->Error("Exception during backup cleanup: " + std::string(e.what()));
        }
        return SaveError::UNKNOWN_ERROR;
    }
}

CrashRecoveryManager::RecoveryStats CrashRecoveryManager::GetRecoveryStats() const {
    std::lock_guard lock(m_stats_mutex);
    return m_stats;
}

void CrashRecoveryManager::ResetStats() {
    std::lock_guard lock(m_stats_mutex);
    m_stats = RecoveryStats{};
}

Json::Value CrashRecoveryManager::RecoveryStats::ToJson() const {
    Json::Value root;
    root["corrupted_files_found"] = static_cast<Json::UInt64>(corrupted_files_found);
    root["successful_recoveries"] = static_cast<Json::UInt64>(successful_recoveries);
    root["failed_recoveries"] = static_cast<Json::UInt64>(failed_recoveries);
    root["temp_files_cleaned"] = static_cast<Json::UInt64>(temp_files_cleaned);
    root["backups_cleaned"] = static_cast<Json::UInt64>(backups_cleaned);
    
    if (corrupted_files_found > 0) {
        root["recovery_success_rate"] = static_cast<double>(successful_recoveries) / corrupted_files_found;
    } else {
        root["recovery_success_rate"] = 0.0;
    }
    
    return root;
}

// ============================================================================
// Recovery Manager Private Helpers (C++17)
// ============================================================================

Expected<bool> CrashRecoveryManager::IsFileCorrupted(const std::filesystem::path& p) const {
    try {
        auto structure_result = ValidateFileStructure(p);
        if (!structure_result.has_value() || !*structure_result) {
            return true; // Corrupted - either error or explicit validation failure
        }
        
        auto json_result = ValidateJSONIntegrity(p);
        if (!json_result.has_value() || !*json_result) {
            return true; // Corrupted - either error or explicit validation failure
        }
        
        return false; // Not corrupted
        
    } catch (const std::exception&) {
        return true; // Treat exceptions as corruption
    }
}

Expected<std::filesystem::path> CrashRecoveryManager::FindBestBackup(const std::filesystem::path& save_file) const {
    try {
        std::string base_name = save_file.stem().string();
        std::string backup_prefix = base_name + "_backup_";
        
        std::vector<std::filesystem::path> backup_files;
        
        for (const auto& entry : std::filesystem::directory_iterator(save_file.parent_path())) {
            if (entry.is_regular_file()) {
                std::string backup_filename = entry.path().filename().string();
                if (string_starts_with(backup_filename, backup_prefix)) {
                    backup_files.push_back(entry.path());
                }
            }
        }
        
        if (backup_files.empty()) {
            return SaveError::FILE_NOT_FOUND;
        }
        
        // Sort by modification time (newest first)
        std::sort(backup_files.begin(), backup_files.end(),
            [](const std::filesystem::path& a, const std::filesystem::path& b) {
                return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
            });
        
        // Return the newest backup that passes integrity check
        for (const auto& backup : backup_files) {
            auto integrity_result = ValidateSaveIntegrity(backup);
            if (integrity_result.has_value() && *integrity_result) {
                return backup;
            }
        }
        
        return SaveError::CORRUPTION_DETECTED;
        
    } catch (const std::exception&) {
        return SaveError::UNKNOWN_ERROR;
    }
}

Expected<bool> CrashRecoveryManager::ValidateFileStructure(const std::filesystem::path& p) const {
    try {
        if (!std::filesystem::exists(p)) {
            return SaveError::FILE_NOT_FOUND;
        }
        
        auto file_size = std::filesystem::file_size(p);
        if (file_size == 0) {
            return SaveError::CORRUPTION_DETECTED;
        }
        
        // Basic file accessibility test
        std::ifstream test_file(p);
        if (!test_file.is_open()) {
            return SaveError::PERMISSION_DENIED;
        }
        
        return true;
        
    } catch (const std::exception&) {
        return SaveError::CORRUPTION_DETECTED;
    }
}

Expected<bool> CrashRecoveryManager::ValidateJSONIntegrity(const std::filesystem::path& p) const {
    try {
        std::ifstream file(p);
        if (!file.is_open()) {
            return SaveError::FILE_NOT_FOUND;
        }
        
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errors;
        
        if (!Json::parseFromStream(builder, file, &root, &errors)) {
            return SaveError::CORRUPTION_DETECTED;
        }
        
        // Validate basic game header
        return ValidateGameHeader(root);
        
    } catch (const std::exception&) {
        return SaveError::CORRUPTION_DETECTED;
    }
}

Expected<bool> CrashRecoveryManager::ValidateGameHeader(const Json::Value& root) const {
    try {
        if (!root.isMember("header")) {
            return SaveError::VALIDATION_FAILED;
        }
        
        const Json::Value& header = root["header"];
        
        if (!header.isMember("game_name") || 
            header["game_name"].asString() != "Mechanica Imperii") {
            return SaveError::VALIDATION_FAILED;
        }
        
        if (!header.isMember("version")) {
            return SaveError::VALIDATION_FAILED;
        }
        
        return true;
        
    } catch (const std::exception&) {
        return SaveError::VALIDATION_FAILED;
    }
}

// ============================================================================
// SaveManager Additional Methods (C++17)
// ============================================================================

Expected<SaveOperationResult> SaveManager::LoadGame(const std::string& filename) {
    auto start_time = std::chrono::steady_clock::now();
    std::string operation_id = RegisterOperation(filename, false);
    
    LogInfo("Starting load operation: " + operation_id + " for file: " + filename);
    
    try {
        SaveProgress progress;
        progress.UpdateProgress(0.0, "Initializing load operation");
        
        // Acquire operation slot
        auto slot_guard = AcquireSlot(false, m_operation_timeout);
        if (!slot_guard.has_value()) {
            UnregisterOperation(operation_id);
            LogError("Failed to acquire load slot for operation: " + operation_id);
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
        
        progress.UpdateProgress(5.0, "Checking file existence");
        
        if (!std::filesystem::exists(resolved_path)) {
            UnregisterOperation(operation_id);
            LogError("Save file not found: " + resolved_path.string());
            return SaveError::FILE_NOT_FOUND;
        }
        
        // Read and parse save data
        progress.UpdateProgress(10.0, "Reading save file");
        Json::Value save_data;
        auto read_result = ReadJson(save_data, resolved_path);
        if (!read_result.has_value()) {
            UnregisterOperation(operation_id);
            LogError("Failed to read save file: " + ToString(read_result.error()));
            return read_result.error();
        }
        
        // Validate save file
        progress.UpdateProgress(30.0, "Validating save structure");
        auto validation_result = ValidateGameData(save_data);
        if (!validation_result.has_value()) {
            UnregisterOperation(operation_id);
            LogError("Save file validation failed");
            return validation_result.error();
        }
        
        // Extract version and check for migration
        if (!save_data.isMember("header") || !save_data["header"].isMember("version")) {
            UnregisterOperation(operation_id);
            LogError("Save file missing version information");
            return SaveError::VALIDATION_FAILED;
        }
        
        auto version_result = SaveVersion::FromString(save_data["header"]["version"].asString());
        if (!version_result.has_value()) {
            UnregisterOperation(operation_id);
            LogError("Invalid version format in save file");
            return SaveError::VALIDATION_FAILED;
        }
        
        SaveVersion file_version = *version_result;
        bool migration_performed = false;
        
        // Handle migration if needed
        if (file_version != m_current_version) {
            progress.UpdateProgress(40.0, "Performing migration");
            auto migration_result = PerformMigration(save_data, file_version, m_current_version);
            if (!migration_result.has_value()) {
                UnregisterOperation(operation_id);
                LogError("Migration failed");
                return migration_result.error();
            }
            migration_performed = true;
            progress.UpdateProgress(60.0, "Migration complete");
        }
        
        // Deserialize game data
        progress.UpdateProgress(70.0, "Loading game systems");
        auto deserialize_result = DeserializeGameData(save_data, progress);
        if (!deserialize_result.has_value()) {
            UnregisterOperation(operation_id);
            LogError("Deserialization failed");
            return deserialize_result.error();
        }
        
        progress.UpdateProgress(100.0, "Load complete");
        
        // Prepare result
        SaveOperationResult result;
        result.result = SaveResult::SUCCESS;
        result.operation_id = operation_id;
        result.version_loaded = file_version;
        result.migration_performed = migration_performed;
        result.bytes_written = std::filesystem::file_size(resolved_path);
        
        result.operation_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time);
        
        LogInfo("Load operation completed successfully: " + operation_id + 
                " (" + std::to_string(result.operation_time.count()) + "ms)");
        
        RecordLoadMetrics(result);
        
        // Update statistics
        std::unique_lock stats_lock(m_stats_mtx);
        m_stats.total_loads++;
        m_stats.successful_loads++;
        if (migration_performed) {
            m_stats.migrations_performed++;
        }
        
        UnregisterOperation(operation_id);
        return result;
        
    } catch (const std::exception& e) {
        UnregisterOperation(operation_id);
        LogError("Exception in load operation " + operation_id + ": " + std::string(e.what()));
        
        std::unique_lock stats_lock(m_stats_mtx);
        m_stats.total_loads++;
        m_stats.failed_loads++;
        
        return SaveError::UNKNOWN_ERROR;
    }
}

// ============================================================================
// Backup Management Implementation (C++17)
// ============================================================================

Expected<SaveOperationResult> SaveManager::CreateBackup(const std::string& filename, const std::string& backup_name) {
    SaveOperationResult result;
    result.result = SaveResult::SUCCESS;
    
    try {
        auto resolved_path_result = SecurePathResolver::Resolve(m_save_dir, filename, m_logger.get());
        if (!resolved_path_result.has_value()) {
            result.result = SaveResult::FILE_ERROR;
            result.message = "Path resolution failed";
            return result;
        }
        
        auto resolved_path = *resolved_path_result;
        
        if (!std::filesystem::exists(resolved_path)) {
            result.result = SaveResult::FILE_ERROR;
            result.message = "Source file not found";
            return result;
        }
        
        std::string actual_backup_name;
        if (backup_name.empty()) {
            // Generate timestamp-based backup name
            std::time_t now = std::time(nullptr);
            
#ifdef _WIN32
            struct tm timeinfo;
            localtime_s(&timeinfo, &now);
            std::ostringstream ss;
            ss << std::put_time(&timeinfo, "%Y%m%d_%H%M%S");
#else
            struct tm* timeinfo = std::localtime(&now);
            std::ostringstream ss;
            ss << std::put_time(timeinfo, "%Y%m%d_%H%M%S");
#endif
            
            actual_backup_name = resolved_path.stem().string() + "_backup_" + ss.str() + ".save";
        } else {
            actual_backup_name = backup_name;
            if (!string_ends_with(actual_backup_name, ".save")) {
                actual_backup_name += ".save";
            }
        }
        
        std::filesystem::path backup_path = resolved_path.parent_path() / actual_backup_name;
        
        std::filesystem::copy_file(resolved_path, backup_path);
        
        result.message = "Backup created: " + backup_path.filename().string();
        result.backup_created = true;
        LogInfo("Created backup: " + backup_path.string());
        
        return result;
        
    } catch (const std::exception& e) {
        result.result = SaveResult::BACKUP_ERROR;
        result.message = "Failed to create backup: " + std::string(e.what());
        LogError("Failed to create backup: " + std::string(e.what()));
        return result;
    }
}

Expected<bool> SaveManager::CleanupOldBackups(const std::string& filename) {
    try {
        auto resolved_path_result = SecurePathResolver::Resolve(m_save_dir, filename, m_logger.get());
        if (!resolved_path_result.has_value()) {
            return resolved_path_result.error();
        }
        
        auto resolved_path = *resolved_path_result;
        
        if (m_recovery) {
            return m_recovery->CleanupOldBackups(resolved_path, m_max_backups);
        }
        
        return true;
        
    } catch (const std::exception& e) {
        LogWarn("Failed to cleanup old backups: " + std::string(e.what()));
        return SaveError::UNKNOWN_ERROR;
    }
}

// ============================================================================
// Async Operations with Progress Callbacks (C++17)
// ============================================================================

std::future<Expected<SaveOperationResult>> SaveManager::SaveGameAsync(
    const std::string& filename,
    ProgressCallback progress_cb,
    CompletionCallback completion_cb) {
    
    return std::async(std::launch::async, [this, filename, progress_cb, completion_cb]() -> Expected<SaveOperationResult> {
        auto result = SaveGame(filename);
        
        if (completion_cb) {
            if (result.has_value()) {
                completion_cb(*result);
            } else {
                SaveOperationResult error_result;
                error_result.result = SaveResult::FILE_ERROR;
                error_result.message = ToString(result.error());
                completion_cb(error_result);
            }
        }
        
        return result;
    });
}

std::future<Expected<SaveOperationResult>> SaveManager::LoadGameAsync(
    const std::string& filename,
    ProgressCallback progress_cb,
    CompletionCallback completion_cb) {
    
    return std::async(std::launch::async, [this, filename, progress_cb, completion_cb]() -> Expected<SaveOperationResult> {
        auto result = LoadGame(filename);
        
        if (completion_cb) {
            if (result.has_value()) {
                completion_cb(*result);
            } else {
                SaveOperationResult error_result;
                error_result.result = SaveResult::FILE_ERROR;
                error_result.message = ToString(result.error());
                completion_cb(error_result);
            }
        }
        
        return result;
    });
}

// ============================================================================
// Migration Support (C++17)
// ============================================================================

Expected<SaveOperationResult> SaveManager::PerformMigration(Json::Value& data, const SaveVersion& from, const SaveVersion& to) {
    try {
        LogInfo("Performing migration from " + from.ToString() + " to " + to.ToString());
        
        auto migration_path_result = MigrationRegistry::Instance().FindMigrationPath(from, to);
        if (!migration_path_result.has_value()) {
            LogError("No migration path found from " + from.ToString() + " to " + to.ToString());
            return migration_path_result.error();
        }
        
        const auto& migrations = *migration_path_result;
        
        for (const auto& migration : migrations) {
            LogInfo("Applying migration: " + migration.description);
            
            auto migration_result = migration.migrate_func(data, m_logger.get());
            if (!migration_result.has_value()) {
                LogError("Migration step failed: " + migration.description);
                return migration_result.error();
            }
        }
        
        // Update version in header
        data["header"]["version"] = to.ToString();
        
        SaveOperationResult result;
        result.result = SaveResult::SUCCESS;
        result.migration_performed = true;
        result.version_loaded = from;
        result.version_saved = to;
        
        // Store migration steps for reporting
        for (const auto& migration : migrations) {
            result.migration_steps.push_back(migration.description);
        }
        
        LogInfo("Migration completed successfully");
        return result;
        
    } catch (const std::exception& e) {
        LogError("Exception during migration: " + std::string(e.what()));
        return SaveError::MIGRATION_FAILED;
    }
}

} // namespace core::save
