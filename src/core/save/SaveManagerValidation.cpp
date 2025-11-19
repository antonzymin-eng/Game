// Created: September 18, 2025 - 14:30:00 (Updated for C++17)
// Location: src/core/save/SaveManagerValidation.cpp
// Mechanica Imperii - SaveManager Validation Implementation (C++17 Compliant)

#include "core/save/SaveManager.h"
#include "utils/PlatformCompat.h"
#include <algorithm>
#include <regex>
#include <cctype>
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
// Validation Report JSON Conversion (Missing Implementation)
// ============================================================================

Json::Value ValidationReport::ToJson() const {
    Json::Value root;
    root["passed"] = passed;
    root["validation_time_ms"] = static_cast<Json::Int64>(validation_time.count());
    root["error_count"] = static_cast<Json::UInt64>(GetErrorCount());
    root["warning_count"] = static_cast<Json::UInt64>(GetWarningCount());
    root["critical_count"] = static_cast<Json::UInt64>(GetCriticalCount());
    
    Json::Value issues_array(Json::arrayValue);
    for (const auto& issue : issues) {
        Json::Value issue_obj;
        switch (issue.severity) {
            case ValidationIssue::CRITICAL: 
                issue_obj["severity"] = "CRITICAL"; 
                break;
            case ValidationIssue::ERROR: 
                issue_obj["severity"] = "ERROR"; 
                break;
            case ValidationIssue::WARNING: 
                issue_obj["severity"] = "WARNING"; 
                break;
        }
        issue_obj["validator"] = issue.validator_name;
        issue_obj["field_path"] = issue.field_path;
        issue_obj["message"] = issue.message;
        if (issue.suggested_fix) {
            issue_obj["suggested_fix"] = *issue.suggested_fix;
        }
        issues_array.append(issue_obj);
    }
    root["issues"] = issues_array;
    
    return root;
}

// ============================================================================
// Secure Path Resolution Additional Methods (C++17)
// ============================================================================

std::string SecurePathResolver::ToString(PathError error) {
    switch (error) {
        case PathError::EMPTY_FILENAME: 
            return "Empty filename provided";
        case PathError::TOO_LONG: 
            return "Filename exceeds maximum length";
        case PathError::INVALID_CHARACTERS: 
            return "Filename contains invalid characters";
        case PathError::RESERVED_NAME: 
            return "Filename is a reserved system name";
        case PathError::ABSOLUTE_PATH: 
            return "Absolute path not allowed";
        case PathError::PATH_TRAVERSAL: 
            return "Path traversal attempt detected";
        case PathError::ESCAPES_BASE_DIR: 
            return "Path escapes base directory";
        case PathError::CANONICALIZATION_FAILED: 
            return "Failed to canonicalize path";
        default: 
            return "Unknown path error";
    }
}

ValidationReport SecurePathResolver::ValidateFilename(const std::string& filename) {
    ValidationReport report;
    
    if (filename.empty()) {
        report.AddError("filename", "", "Filename is empty", 
                       "Provide a valid filename");
        return report;
    }
    
    if (filename.length() > 255) {
        report.AddError("filename", "", 
                       "Filename too long (" + std::to_string(filename.length()) + " characters)",
                       "Use a filename shorter than 256 characters");
    }
    
    // Check for invalid characters
    const std::string invalid_chars = "<>:\"|?*";
    size_t pos = filename.find_first_of(invalid_chars);
    if (pos != std::string::npos) {
        report.AddError("filename", "", 
                       "Filename contains invalid character: '" + std::string(1, filename[pos]) + "'",
                       "Remove special characters from filename");
    }
    
    // Check for path traversal
    if (filename.find("..") != std::string::npos) {
        report.AddCritical("filename", "", 
                          "Path traversal attempt detected (..) in filename",
                          "Remove directory navigation from filename");
    }
    
    if (filename.find("/") != std::string::npos || filename.find("\\") != std::string::npos) {
        report.AddError("filename", "", 
                       "Filename contains path separators",
                       "Use only the filename without path");
    }
    
#ifdef _WIN32
    // Check Windows reserved names
    if (IsWindowsReserved(filename)) {
        report.AddError("filename", "", 
                       "Filename is a Windows reserved name",
                       "Choose a different filename");
    }
    
    // Check for trailing dots or spaces (Windows issue)
    if (string_ends_with(filename, ".") || string_ends_with(filename, " ")) {
        report.AddWarning("filename", "", 
                         "Filename ends with dot or space (Windows compatibility issue)",
                         "Remove trailing dots or spaces");
    }
#endif
    
    // Check for hidden file prefix
    if (string_starts_with(filename, ".")) {
        report.AddWarning("filename", "", 
                         "Filename starts with dot (hidden file)",
                         std::nullopt);
    }
    
    // Validate extension
    if (!string_ends_with(filename, ".save")) {
        report.AddWarning("filename", "", 
                         "Filename missing .save extension",
                         "Extension will be added automatically");
    }
    
    return report;
}

// ============================================================================
// SaveManager Validation Methods (C++17)
// ============================================================================

Expected<ValidationReport> SaveManager::ValidateGameData(const Json::Value& data) const {
    auto start_time = std::chrono::steady_clock::now();
    ValidationReport report;
    
    // Get list of expected system names
    std::vector<std::string> expected_systems = GetRegisteredSystemNames();
    
    // Run all registered validators
    std::shared_lock lock(m_val_mtx);
    
    for (const auto& [name, validator] : m_validators) {
        try {
            LogDebug("Running validator: " + name);
            ValidationReport validator_report = validator(data, expected_systems);
            
            // Merge validator results
            report.passed = report.passed && validator_report.passed;
            report.issues.insert(report.issues.end(), 
                               validator_report.issues.begin(), 
                               validator_report.issues.end());
                               
        } catch (const std::exception& e) {
            report.AddError(name, "", 
                          "Validator threw exception: " + std::string(e.what()),
                          "Check validator implementation");
        }
    }
    
    report.validation_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time);
    
    LogInfo("Validation completed in " + std::to_string(report.validation_time.count()) + 
            "ms with " + std::to_string(report.issues.size()) + " issues");
    
    return report;
}

std::vector<std::string> SaveManager::GetRegisteredSystemNames() const {
    std::vector<std::string> names;
    names.reserve(m_systems.size());
    
    for (const auto& system : m_systems) {
        names.push_back(system->GetSystemName());
    }
    
    return names;
}

Expected<ValidationReport> SaveManager::ValidateSave(const std::string& filename) const {
    try {
        // Check cache first
        {
            std::shared_lock lock(m_val_mtx);
            auto cache_it = m_validation_cache.find(filename);
            if (cache_it != m_validation_cache.end()) {
                m_validation_cache_hits++;
                return cache_it->second;
            }
            m_validation_cache_misses++;
        }
        
        // Resolve path
        auto path_result = CanonicalSavePath(filename);
        if (!path_result.has_value()) {
            ValidationReport report;
            report.passed = false;
            report.AddCritical("file", "", "Invalid filename", std::nullopt);
            return report;
        }
        
        // Read and parse file
        Json::Value save_data;
        auto read_result = ReadJson(save_data, *path_result);
        if (!read_result.has_value()) {
            ValidationReport report;
            report.passed = false;
            report.AddCritical("file", "", "Failed to read save file: " + ToString(read_result.error()), std::nullopt);
            return report;
        }
        
        // Validate the data
        auto validation_result = ValidateGameData(save_data);
        if (!validation_result.has_value()) {
            ValidationReport report;
            report.passed = false;
            report.AddCritical("validation", "", "Validation failed", std::nullopt);
            return report;
        }
        
        // Cache the result
        {
            std::unique_lock lock(m_val_mtx);
            m_validation_cache[filename] = *validation_result;
        }
        
        return validation_result;
        
    } catch (const std::exception& e) {
        LogError("Exception during save validation: " + std::string(e.what()));
        ValidationReport report;
        report.passed = false;
        report.AddCritical("validation", "", "Exception: " + std::string(e.what()), std::nullopt);
        return report;
    }
}

Expected<bool> SaveManager::VerifyChecksum(const std::string& filename) const {
    try {
        auto path_result = CanonicalSavePath(filename);
        if (!path_result.has_value()) {
            return path_result.error();
        }
        
        Json::Value save_data;
        auto read_result = ReadJson(save_data, *path_result);
        if (!read_result.has_value()) {
            return read_result.error();
        }
        
        if (!save_data.isMember("checksum")) {
            LogWarn("Save file missing checksum field");
            return false;
        }
        
        std::string stored_checksum = save_data["checksum"].asString();
        
        // Rebuild canonical JSON without checksum
        std::string canonical_without_checksum = CanonicalJSONBuilder::Build(save_data, true);
        
        // Calculate checksum
        auto hash_result = SHA256(
            reinterpret_cast<const uint8_t*>(canonical_without_checksum.data()),
            canonical_without_checksum.size());
        
        if (!hash_result.has_value()) {
            return hash_result.error();
        }
        
        bool matches = (stored_checksum == *hash_result);
        
        if (!matches) {
            LogError("Checksum mismatch for file: " + filename);
            LogError("Expected: " + stored_checksum);
            LogError("Calculated: " + *hash_result);
        }
        
        return matches;
        
    } catch (const std::exception& e) {
        LogError("Exception verifying checksum: " + std::string(e.what()));
        return SaveError::UNKNOWN_ERROR;
    }
}

Expected<ValidationReport> SaveManager::VerifyFile(const std::string& filename, 
                                                   const VerificationOptions& options) const {
    ValidationReport report;
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        if (options.verbose) {
            LogInfo("Starting file verification for: " + filename);
        }
        
        // Check file exists
        auto path_result = CanonicalSavePath(filename);
        if (!path_result.has_value()) {
            report.AddCritical("file", "", "Cannot resolve filename", std::nullopt);
            return report;
        }
        
        if (!std::filesystem::exists(*path_result)) {
            report.AddCritical("file", "", "File does not exist", std::nullopt);
            return report;
        }
        
        // Check structure
        if (options.check_structure) {
            Json::Value save_data;
            auto read_result = ReadJson(save_data, *path_result);
            if (!read_result.has_value()) {
                report.AddCritical("structure", "", "Cannot parse JSON", std::nullopt);
                return report;
            }
            
            if (!save_data.isMember("header") || !save_data.isMember("systems")) {
                report.AddError("structure", "", "Invalid save file structure", 
                              "File must have 'header' and 'systems' sections");
            }
        }
        
        // Check checksums
        if (options.check_checksums) {
            auto checksum_result = VerifyChecksum(filename);
            if (!checksum_result.has_value()) {
                report.AddError("checksum", "", "Failed to verify checksum", std::nullopt);
            } else if (!*checksum_result) {
                report.AddCritical("checksum", "", "Checksum mismatch - file may be corrupted", std::nullopt);
            }
        }
        
        // Run validators
        if (options.run_validators) {
            auto validation_result = ValidateSave(filename);
            if (validation_result.has_value()) {
                // Merge validation results
                report.passed = report.passed && validation_result->passed;
                report.issues.insert(report.issues.end(), 
                                   validation_result->issues.begin(), 
                                   validation_result->issues.end());
            }
        }
        
        // Check migrations
        if (options.check_migrations) {
            auto migration_check = IsMigrationRequired(filename);
            if (migration_check.has_value() && *migration_check) {
                report.AddWarning("version", "", "Save file requires migration to current version", 
                                "Use MigrateSave() to update");
            }
        }
        
        report.validation_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time);
        
        if (options.verbose) {
            LogInfo("Verification completed: " + report.GenerateReport());
        }
        
        return report;
        
    } catch (const std::exception& e) {
        report.AddCritical("verification", "", "Exception: " + std::string(e.what()), std::nullopt);
        return report;
    }
}

// ============================================================================
// SaveStats JSON Conversion Implementation
// ============================================================================

Json::Value SaveManager::SaveStats::ToJson() const {
    Json::Value root;

    // Save statistics
    root["total_saves"] = static_cast<Json::UInt64>(total_saves);
    root["successful_saves"] = static_cast<Json::UInt64>(successful_saves);
    root["failed_saves"] = static_cast<Json::UInt64>(failed_saves);
    root["cancelled_saves"] = static_cast<Json::UInt64>(cancelled_saves);
    root["save_success_rate"] = GetSaveSuccessRate();

    // Load statistics
    root["total_loads"] = static_cast<Json::UInt64>(total_loads);
    root["successful_loads"] = static_cast<Json::UInt64>(successful_loads);
    root["failed_loads"] = static_cast<Json::UInt64>(failed_loads);
    root["cancelled_loads"] = static_cast<Json::UInt64>(cancelled_loads);
    root["load_success_rate"] = GetLoadSuccessRate();

    // Recovery and migration statistics
    root["corrupted_saves_recovered"] = static_cast<Json::UInt64>(corrupted_saves_recovered);
    root["migrations_performed"] = static_cast<Json::UInt64>(migrations_performed);

    // Performance statistics
    root["average_save_time_ms"] = static_cast<Json::Int64>(average_save_time.count());
    root["average_load_time_ms"] = static_cast<Json::Int64>(average_load_time.count());
    root["total_bytes_saved"] = static_cast<Json::UInt64>(total_bytes_saved);

    // Cache statistics
    Json::Value cache_stats;
    cache_stats["size"] = static_cast<Json::UInt64>(json_cache_stats.size);
    cache_stats["max_size"] = static_cast<Json::UInt64>(json_cache_stats.max_size);
    cache_stats["hits"] = static_cast<Json::UInt64>(json_cache_stats.hits);
    cache_stats["misses"] = static_cast<Json::UInt64>(json_cache_stats.misses);
    cache_stats["evictions"] = static_cast<Json::UInt64>(json_cache_stats.evictions);
    cache_stats["hit_ratio"] = json_cache_stats.hit_ratio();
    root["json_cache_stats"] = cache_stats;

    root["validation_cache_hit_ratio"] = validation_cache_hit_ratio;
    root["concurrent_operations_peak"] = static_cast<Json::UInt64>(concurrent_operations_peak);

    return root;
}

// ============================================================================
// System Information Implementation
// ============================================================================

Json::Value SaveManager::GetSystemInfo() const {
    Json::Value root;

    // Version information
    root["current_version"] = m_current_version.ToString();
    root["save_directory"] = m_save_dir.string();

    // Configuration
    Json::Value config;
    config["auto_backup_enabled"] = m_auto_backup;
    config["max_backups"] = m_max_backups;
    config["atomic_writes_enabled"] = m_atomic_writes_enabled;
    config["operation_timeout_seconds"] = static_cast<Json::Int64>(m_operation_timeout.count());
    root["configuration"] = config;

    // Concurrency settings
    Json::Value concurrency;
    {
        std::lock_guard lock(m_concurrency.mtx);
        concurrency["max_concurrent_saves"] = static_cast<Json::UInt64>(m_concurrency.max_saves);
        concurrency["max_concurrent_loads"] = static_cast<Json::UInt64>(m_concurrency.max_loads);
        concurrency["active_saves"] = static_cast<Json::UInt64>(m_concurrency.active_saves);
        concurrency["active_loads"] = static_cast<Json::UInt64>(m_concurrency.active_loads);
        concurrency["peak_concurrent"] = static_cast<Json::UInt64>(m_concurrency.peak_concurrent);
    }
    root["concurrency"] = concurrency;

    // Registered systems
    Json::Value systems_array(Json::arrayValue);
    for (const auto& system : m_systems) {
        systems_array.append(system->GetSystemName());
    }
    root["registered_systems"] = systems_array;
    root["registered_system_count"] = static_cast<Json::UInt64>(m_systems.size());

    // Validators
    {
        std::shared_lock lock(m_val_mtx);
        Json::Value validators_array(Json::arrayValue);
        for (const auto& [name, _] : m_validators) {
            validators_array.append(name);
        }
        root["registered_validators"] = validators_array;
        root["validation_cache_size"] = static_cast<Json::UInt64>(m_validation_cache.size());
        root["validation_cache_hits"] = static_cast<Json::UInt64>(m_validation_cache_hits.load());
        root["validation_cache_misses"] = static_cast<Json::UInt64>(m_validation_cache_misses.load());

        size_t total_lookups = m_validation_cache_hits.load() + m_validation_cache_misses.load();
        double hit_ratio = total_lookups > 0 ?
            static_cast<double>(m_validation_cache_hits.load()) / total_lookups : 0.0;
        root["validation_cache_hit_ratio"] = hit_ratio;
    }

    // Active operations
    {
        std::shared_lock lock(m_ops_mtx);
        Json::Value active_ops_array(Json::arrayValue);
        for (const auto& [id, op] : m_active_ops) {
            Json::Value op_info;
            op_info["operation_id"] = id;
            op_info["filename"] = op.filename;
            op_info["is_save"] = op.is_save;
            if (op.progress) {
                op_info["progress_percentage"] = op.progress->percentage.load();
                op_info["current_operation"] = op.progress->GetCurrentOperation();
            }
            active_ops_array.append(op_info);
        }
        root["active_operations"] = active_ops_array;
        root["active_operation_count"] = static_cast<Json::UInt64>(m_active_ops.size());
    }

    // Statistics summary
    root["statistics"] = GetSaveStats().ToJson();

    // Logger information
    if (m_logger) {
        root["logger_level"] = static_cast<int>(m_logger->GetLevel());
    } else {
        root["logger_level"] = Json::Value::null;
    }

    return root;
}

// ============================================================================
// Version and Migration Implementation
// ============================================================================

Expected<SaveVersion> SaveManager::GetSaveFileVersion(const std::string& filename) const {
    try {
        // Resolve path
        auto path_result = CanonicalSavePath(filename);
        if (!path_result.has_value()) {
            return path_result.error();
        }

        // Check file exists
        if (!std::filesystem::exists(*path_result)) {
            return SaveError::FILE_NOT_FOUND;
        }

        // Read and parse file
        Json::Value save_data;
        auto read_result = ReadJson(save_data, *path_result);
        if (!read_result.has_value()) {
            return read_result.error();
        }

        // Extract version from header
        if (!save_data.isMember("header")) {
            LogError("Save file missing header section");
            return SaveError::VALIDATION_FAILED;
        }

        const Json::Value& header = save_data["header"];
        if (!header.isMember("version")) {
            LogError("Save file header missing version field");
            return SaveError::VALIDATION_FAILED;
        }

        std::string version_str = header["version"].asString();
        auto version_result = SaveVersion::FromString(version_str);
        if (!version_result.has_value()) {
            LogError("Invalid version string in save file: " + version_str);
            return version_result.error();
        }

        return *version_result;

    } catch (const std::exception& e) {
        LogError("Exception reading save file version: " + std::string(e.what()));
        return SaveError::UNKNOWN_ERROR;
    }
}

Expected<bool> SaveManager::IsMigrationRequired(const std::string& filename) const {
    try {
        // Get the save file version
        auto version_result = GetSaveFileVersion(filename);
        if (!version_result.has_value()) {
            return version_result.error();
        }

        SaveVersion file_version = *version_result;

        // Check if migration is needed
        bool needs_migration = (file_version != m_current_version);

        if (needs_migration) {
            LogInfo("Save file '" + filename + "' version " + file_version.ToString() +
                   " requires migration to current version " + m_current_version.ToString());
        }

        return needs_migration;

    } catch (const std::exception& e) {
        LogError("Exception checking migration requirement: " + std::string(e.what()));
        return SaveError::UNKNOWN_ERROR;
    }
}

} // namespace core::save
