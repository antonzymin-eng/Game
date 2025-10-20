// Created: September 18, 2025 - 14:15:00 (Updated for C++17)
// Location: src/core/save/SaveManagerSerialization.cpp
// Mechanica Imperii - SaveManager Serialization Implementation (C++17 Compliant)

#include "core/save/SaveManager.h"
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <queue>
#include <unordered_set>
#include <random>
#include <ctime>
#include <filesystem>
#include <jsoncpp/json/json.h>

#ifdef _WIN32
  #define NOMINMAX
  #include <windows.h>
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
// Platform-Abstracted File Operations Implementation (C++17)
// ============================================================================

namespace platform {

Expected<bool> FileOperations::WriteAtomic(const uint8_t* data, size_t size, const std::filesystem::path& filepath) {
    try {
        std::error_code ec;
        auto dir = filepath.parent_path();
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(10000, 99999);
        std::string tmpname = filepath.filename().string() + ".tmp." + std::to_string(dis(gen));
        auto tmppath = dir / tmpname;
        
        {
#ifdef _WIN32
            int fd = _open(tmppath.string().c_str(), _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY, _S_IREAD | _S_IWRITE);
#else
            int fd = ::open(tmppath.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
#endif
            if (fd == -1) {
                return SaveError::PERMISSION_DENIED;
            }
            
#ifdef _WIN32
            if (_write(fd, data, static_cast<unsigned int>(size)) != static_cast<int>(size)) {
                _close(fd);
                std::filesystem::remove(tmppath, ec);
                return SaveError::INSUFFICIENT_SPACE;
            }
            _close(fd);
#else
            ssize_t written = 0;
            while (written < static_cast<ssize_t>(size)) {
                ssize_t result = ::write(fd, data + written, size - written);
                if (result == -1) {
                    ::close(fd);
                    std::filesystem::remove(tmppath, ec);
                    if (errno == ENOSPC) {
                        return SaveError::INSUFFICIENT_SPACE;
                    }
                    return SaveError::PERMISSION_DENIED;
                }
                written += result;
            }
            
            if (::fsync(fd) != 0) {
                ::close(fd);
                std::filesystem::remove(tmppath, ec);
                return SaveError::UNKNOWN_ERROR;
            }
            ::close(fd);
#endif
        }
        
#ifdef _WIN32
        HANDLE hFile = CreateFileW(
            tmppath.wstring().c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        
        if (hFile != INVALID_HANDLE_VALUE) {
            FlushFileBuffers(hFile);
            CloseHandle(hFile);
        }
        
        BOOL ok = MoveFileExW(
            tmppath.wstring().c_str(),
            filepath.wstring().c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
        if (!ok) {
            std::filesystem::remove(tmppath, ec);
            DWORD error = GetLastError();
            if (error == ERROR_DISK_FULL || error == ERROR_HANDLE_DISK_FULL) {
                return SaveError::INSUFFICIENT_SPACE;
            }
            return SaveError::PERMISSION_DENIED;
        }
#else
        if (::rename(tmppath.c_str(), filepath.c_str()) != 0) {
            std::filesystem::remove(tmppath, ec);
            if (errno == ENOSPC) {
                return SaveError::INSUFFICIENT_SPACE;
            }
            return SaveError::PERMISSION_DENIED;
        }
        
        int dfd = ::open(dir.c_str(), O_RDONLY);
        if (dfd != -1) {
            ::fsync(dfd);
            ::close(dfd);
        }
#endif
        
        return true;
        
    } catch (const std::exception&) {
        return SaveError::UNKNOWN_ERROR;
    }
}

Expected<bool> FileOperations::WriteDirect(const uint8_t* data, size_t size, const std::filesystem::path& filepath) {
    try {
        std::ofstream f(filepath, std::ios::binary | std::ios::trunc);
        if (!f.is_open()) {
            return SaveError::PERMISSION_DENIED;
        }
        
        f.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
        f.flush();
        
        if (!f.good()) {
            if (f.fail() && !f.bad()) {
                return SaveError::INSUFFICIENT_SPACE;
            }
            return SaveError::PERMISSION_DENIED;
        }
        
        return true;
        
    } catch (const std::exception&) {
        return SaveError::UNKNOWN_ERROR;
    }
}

Expected<std::vector<uint8_t>> FileOperations::ReadFile(const std::filesystem::path& filepath) {
    try {
        std::ifstream f(filepath, std::ios::binary);
        if (!f.is_open()) {
            return SaveError::FILE_NOT_FOUND;
        }
        
        f.seekg(0, std::ios::end);
        size_t size = static_cast<size_t>(f.tellg());
        f.seekg(0, std::ios::beg);
        
        std::vector<uint8_t> data(size);
        f.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size));
        
        if (!f.good() && !f.eof()) {
            return SaveError::CORRUPTION_DETECTED;
        }
        
        return data;
        
    } catch (const std::exception&) {
        return SaveError::UNKNOWN_ERROR;
    }
}

Expected<bool> FileOperations::SyncDirectory(const std::filesystem::path& dir_path) {
    try {
#ifdef _WIN32
        return true;
#else
        int dfd = ::open(dir_path.c_str(), O_RDONLY);
        if (dfd == -1) {
            if (errno == EACCES) {
                return SaveError::PERMISSION_DENIED;
            } else if (errno == ENOENT) {
                return SaveError::FILE_NOT_FOUND;
            }
            return SaveError::UNKNOWN_ERROR;
        }
        
        int result = ::fsync(dfd);
        ::close(dfd);
        
        if (result != 0) {
            if (errno == EACCES) {
                return SaveError::PERMISSION_DENIED;
            }
            return SaveError::UNKNOWN_ERROR;
        }
        
        return true;
#endif
    } catch (const std::exception&) {
        return SaveError::UNKNOWN_ERROR;
    }
}

Expected<uint64_t> FileOperations::GetAvailableSpace(const std::filesystem::path& path) {
    try {
        std::error_code ec;
        auto space_info = std::filesystem::space(path, ec);
        if (ec) {
            if (ec == std::errc::no_such_file_or_directory) {
                return SaveError::FILE_NOT_FOUND;
            } else if (ec == std::errc::permission_denied) {
                return SaveError::PERMISSION_DENIED;
            }
            return SaveError::UNKNOWN_ERROR;
        }
        return static_cast<uint64_t>(space_info.available);
    } catch (const std::exception&) {
        return SaveError::UNKNOWN_ERROR;
    }
}

} // namespace platform

// ============================================================================
// Canonical JSON with proper cache stats (C++17)
// ============================================================================

// Static members for LRU cache
std::unordered_map<std::string, std::unique_ptr<CanonicalJSONBuilder::CacheEntry>> CanonicalJSONBuilder::s_cache;
std::mutex CanonicalJSONBuilder::s_cache_mutex;
size_t CanonicalJSONBuilder::s_max_cache_size = 100;
CanonicalJSONBuilder::CacheStats CanonicalJSONBuilder::s_cache_stats;

Json::Value CanonicalJSONBuilder::SortKeysRecursive(const Json::Value& v) {
    if (v.isObject()) {
        Json::Value ordered(Json::objectValue);
        
        auto member_names = v.getMemberNames();
        std::sort(member_names.begin(), member_names.end());
        
        for (const auto& name : member_names) {
            ordered[name] = SortKeysRecursive(v[name]);
        }
        return ordered;
    } else if (v.isArray()) {
        Json::Value sorted_array(Json::arrayValue);
        for (const auto& element : v) {
            sorted_array.append(SortKeysRecursive(element));
        }
        return sorted_array;
    }
    return v;
}

std::string CanonicalJSONBuilder::Build(const Json::Value& root, bool exclude_checksum) {
    Json::Value working_copy = root;
    if (exclude_checksum && working_copy.isMember("checksum")) {
        working_copy.removeMember("checksum");
    }
    
    Json::Value sorted = SortKeysRecursive(working_copy);
    
    // Create better cache key from sorted structure
    Json::StreamWriterBuilder key_builder;
    key_builder["indentation"] = "";
    key_builder["commentStyle"] = "None";
    std::string cache_key = Json::writeString(key_builder, sorted);
    
    // Check cache first
    {
        std::lock_guard<std::mutex> lock(s_cache_mutex);
        auto it = s_cache.find(cache_key);
        if (it != s_cache.end()) {
            it->second->last_used = std::chrono::steady_clock::now();
            it->second->access_count++;
            s_cache_stats.hits++;
            return it->second->canonical_json;
        }
        s_cache_stats.misses++;
    }
    
    // Build canonical JSON
    Json::StreamWriterBuilder canonical_builder;
    canonical_builder["indentation"] = "";
    canonical_builder["commentStyle"] = "None";
    canonical_builder["enableYAMLCompatibility"] = false;
    canonical_builder["dropNullPlaceholders"] = false;
    canonical_builder["useSpecialFloats"] = false;
    
    std::string result = Json::writeString(canonical_builder, sorted);
    
    // Store in cache
    {
        std::lock_guard<std::mutex> lock(s_cache_mutex);
        if (s_cache.size() >= s_max_cache_size && s_max_cache_size > 0) {
            // Remove oldest entry
            auto oldest_it = std::min_element(s_cache.begin(), s_cache.end(),
                [](const auto& a, const auto& b) {
                    return a.second->last_used < b.second->last_used;
                });
            s_cache.erase(oldest_it);
            s_cache_stats.evictions++;
        }
        
        s_cache[cache_key] = std::make_unique<CacheEntry>(result);
    }
    
    return result;
}

void CanonicalJSONBuilder::SetCacheSize(size_t max_size) {
    std::lock_guard<std::mutex> lock(s_cache_mutex);
    s_max_cache_size = max_size;
    
    while (s_cache.size() > max_size && max_size > 0) {
        auto oldest_it = std::min_element(s_cache.begin(), s_cache.end(),
            [](const auto& a, const auto& b) {
                return a.second->last_used < b.second->last_used;
            });
        s_cache.erase(oldest_it);
        s_cache_stats.evictions++;
    }
}

CanonicalJSONBuilder::CacheStats CanonicalJSONBuilder::GetCacheStats() {
    std::lock_guard<std::mutex> lock(s_cache_mutex);
    s_cache_stats.size = s_cache.size();
    s_cache_stats.max_size = s_max_cache_size;
    return s_cache_stats;
}

void CanonicalJSONBuilder::ClearCache() {
    std::lock_guard<std::mutex> lock(s_cache_mutex);
    s_cache.clear();
    s_cache_stats = CacheStats{};
}

// ============================================================================
// SHA-256 as static member function (C++17)
// ============================================================================

Expected<std::string> SaveManager::SHA256(const uint8_t* data, size_t size) {
    try {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX sha256_context;
        
        if (SHA256_Init(&sha256_context) != 1) {
            return SaveError::UNKNOWN_ERROR;
        }
        
        if (SHA256_Update(&sha256_context, data, size) != 1) {
            return SaveError::UNKNOWN_ERROR;
        }
        
        if (SHA256_Final(hash, &sha256_context) != 1) {
            return SaveError::UNKNOWN_ERROR;
        }
        
        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(hash[i]);
        }
        
        return ss.str();
        
    } catch (const std::exception&) {
        return SaveError::UNKNOWN_ERROR;
    }
}

// ============================================================================
// Enhanced Migration System Implementation (C++17)
// ============================================================================

SaveMigration::SaveMigration(const SaveVersion& from, const SaveVersion& to, const std::string& desc,
                           std::function<Expected<bool>(Json::Value&, ILogger*)> func)
    : from_version(from), to_version(to), description(desc), migrate_func(std::move(func)) {}

MigrationRegistry& MigrationRegistry::Instance() {
    static MigrationRegistry instance;
    return instance;
}

void MigrationRegistry::RegisterMigration(const SaveVersion& from, const SaveVersion& to,
                                         const std::string& description,
                                         std::function<Expected<bool>(Json::Value&, ILogger*)> migration_func) {
    std::unique_lock lock(m_mutex);
    m_migrations.emplace_back(from, to, description, std::move(migration_func));
}

Expected<std::vector<SaveMigration>> MigrationRegistry::FindMigrationPath(const SaveVersion& from, const SaveVersion& to) const {
    if (from == to) {
        return std::vector<SaveMigration>{};
    }
    
    return BFS(from, to);
}

bool MigrationRegistry::IsMigrationSupported(const SaveVersion& from, const SaveVersion& to) const {
    auto path_result = FindMigrationPath(from, to);
    return path_result.has_value();
}

std::vector<std::string> MigrationRegistry::GetMigrationPreview(const SaveVersion& from, const SaveVersion& to) const {
    std::vector<std::string> preview;
    
    auto path_result = FindMigrationPath(from, to);
    if (!path_result.has_value()) {
        return preview;
    }
    
    const auto& path = *path_result;
    for (const auto& migration : path) {
        preview.push_back(migration.from_version.ToString() + " -> " + 
                         migration.to_version.ToString() + ": " + migration.description);
    }
    
    return preview;
}

Expected<std::vector<SaveMigration>> MigrationRegistry::BFS(const SaveVersion& from, const SaveVersion& to) const {
    std::shared_lock lock(m_mutex);
    
    std::queue<SaveVersion> queue;
    std::unordered_map<std::string, SaveVersion> parent;
    std::unordered_map<std::string, SaveMigration> migration_map;
    std::unordered_set<std::string> visited;
    
    queue.push(from);
    visited.insert(from.ToString());
    
    while (!queue.empty()) {
        SaveVersion current = queue.front();
        queue.pop();
        
        if (current == to) {
            // Reconstruct path
            std::vector<SaveMigration> path;
            SaveVersion step = to;
            
            while (step != from) {
                auto parent_it = parent.find(step.ToString());
                if (parent_it == parent.end()) break;
                
                SaveVersion prev = parent_it->second;
                auto migration_key = prev.ToString() + "->" + step.ToString();
                auto migration_it = migration_map.find(migration_key);
                if (migration_it != migration_map.end()) {
                    path.insert(path.begin(), migration_it->second);
                }
                step = prev;
            }
            
            return path;
        }
        
        // Find all migrations from current version
        for (const auto& migration : m_migrations) {
            if (migration.from_version == current) {
                std::string next_str = migration.to_version.ToString();
                
                if (visited.find(next_str) == visited.end()) {
                    visited.insert(next_str);
                    parent[next_str] = current;
                    migration_map.emplace(current.ToString() + "->" + next_str, migration);
                    queue.push(migration.to_version);
                }
            }
        }
    }
    
    return SaveError::MIGRATION_FAILED;
}

void MigrationRegistry::InitializeDefaultMigrations() {
    // Example default migrations - customize for your game
    RegisterMigration(
        SaveVersion{1, 0, 0}, SaveVersion{1, 1, 0},
        "Add new province taxation system",
        [](Json::Value& data, ILogger* logger) -> Expected<bool> {
            if (logger) logger->Info("Applying migration 1.0.0 -> 1.1.0");
            
            if (data.isMember("systems") && data["systems"].isMember("provinces")) {
                Json::Value& provinces = data["systems"]["provinces"];
                if (provinces.isArray()) {
                    for (auto& province : provinces) {
                        if (!province.isMember("taxation")) {
                            province["taxation"] = Json::Value(Json::objectValue);
                            province["taxation"]["base_rate"] = 0.1;
                            province["taxation"]["efficiency"] = 1.0;
                        }
                    }
                }
            }
            
            return true;
        }
    );
    
    RegisterMigration(
        SaveVersion{1, 1, 0}, SaveVersion{1, 2, 0},
        "Add diplomatic relations tracking",
        [](Json::Value& data, ILogger* logger) -> Expected<bool> {
            if (logger) logger->Info("Applying migration 1.1.0 -> 1.2.0");
            
            if (data.isMember("systems")) {
                if (!data["systems"].isMember("diplomacy")) {
                    data["systems"]["diplomacy"] = Json::Value(Json::objectValue);
                    data["systems"]["diplomacy"]["relations"] = Json::Value(Json::arrayValue);
                    data["systems"]["diplomacy"]["active_treaties"] = Json::Value(Json::arrayValue);
                }
            }
            
            return true;
        }
    );
}

// ============================================================================
// SaveManager Serialization Methods (C++17)
// ============================================================================

Expected<SaveManager::SerializedData> SaveManager::SerializeGameData(const SaveVersion& v, SaveProgress& prog) {
    SerializedData result;
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        Json::Value root;
        root["header"] = CreateSaveHeader(v);
        
        Json::Value systems_data(Json::objectValue);
        double progress_per_system = m_systems.empty() ? 60.0 : (60.0 / static_cast<double>(m_systems.size()));
        double current_progress = 10.0;
        
        for (size_t i = 0; i < m_systems.size(); ++i) {
            if (prog.IsCancelled()) {
                LogInfo("Serialization cancelled by user");
                return SaveError::OPERATION_CANCELLED;
            }
            
            auto& system = m_systems[i];
            std::string system_name = system->GetSystemName();
            
            prog.UpdateProgress(current_progress, "Serializing " + system_name);
            
            Json::Value system_data;
            if (!system->Serialize(system_data, v.ToInt())) {
                LogError("Failed to serialize system: " + system_name);
                return SaveError::SERIALIZATION_FAILED;
            }
            
            systems_data[system_name] = system_data;
            current_progress += progress_per_system;
        }
        
        root["systems"] = systems_data;
        
        // Create canonical JSON without checksum
        std::string canonical_without_checksum = CanonicalJSONBuilder::Build(root, true);
        
        // Calculate SHA-256 checksum (C++17 compatible)
        auto hash_result = SHA256(
            reinterpret_cast<const uint8_t*>(canonical_without_checksum.data()),
            canonical_without_checksum.size());
        
        if (!hash_result.has_value()) {
            return hash_result.error();
        }
        
        result.sha256 = *hash_result;
        
        // Add checksum to data and regenerate canonical form
        root["checksum"] = result.sha256;
        result.canonical = CanonicalJSONBuilder::Build(root, false);
        result.estimated_size = result.canonical.size();
        
        result.serialization_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time);
        
        prog.UpdateProgress(70.0, "Serialization complete");
        
        return result;
        
    } catch (const std::exception& e) {
        LogError("Exception during serialization: " + std::string(e.what()));
        return SaveError::SERIALIZATION_FAILED;
    }
}

Expected<bool> SaveManager::DeserializeGameData(const Json::Value& save_data, SaveProgress& prog) {
    try {
        if (!save_data.isMember("systems")) {
            LogError("Save data missing systems section");
            return SaveError::VALIDATION_FAILED;
        }
        
        const Json::Value& systems_data = save_data["systems"];
        double progress_per_system = m_systems.empty() ? 30.0 : (30.0 / static_cast<double>(m_systems.size()));
        double current_progress = 70.0;
        
        for (size_t i = 0; i < m_systems.size(); ++i) {
            if (prog.IsCancelled()) {
                LogInfo("Deserialization cancelled by user");
                return SaveError::OPERATION_CANCELLED;
            }
            
            auto& system = m_systems[i];
            std::string system_name = system->GetSystemName();
            
            prog.UpdateProgress(current_progress, "Deserializing " + system_name);
            
            if (systems_data.isMember(system_name)) {
                if (!system->Deserialize(systems_data[system_name], m_current_version.ToInt())) {
                    LogError("Failed to deserialize system: " + system_name);
                    return SaveError::SERIALIZATION_FAILED;
                }
            } else {
                LogWarn("System '" + system_name + "' not found in save data - using default state");
            }
            
            current_progress += progress_per_system;
        }
        
        prog.UpdateProgress(100.0, "Deserialization complete");
        return true;
        
    } catch (const std::exception& e) {
        LogError("Exception during deserialization: " + std::string(e.what()));
        return SaveError::SERIALIZATION_FAILED;
    }
}

Expected<bool> SaveManager::ReadJson(Json::Value& out, const std::filesystem::path& filepath) const {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return SaveError::FILE_NOT_FOUND;
        }
        
        Json::CharReaderBuilder builder;
        std::string errors;
        
        if (!Json::parseFromStream(builder, file, &out, &errors)) {
            LogError("JSON parse error: " + errors);
            return SaveError::CORRUPTION_DETECTED;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        LogError("Exception reading JSON: " + std::string(e.what()));
        return SaveError::UNKNOWN_ERROR;
    }
}

// ============================================================================
// Secure Path Resolution (C++17)
// ============================================================================

Expected<std::filesystem::path> SecurePathResolver::Resolve(const std::filesystem::path& base_dir, 
                                                            const std::string& filename,
                                                            ILogger* logger) {
    // Validate filename
    if (filename.empty()) {
        if (logger) logger->Error("Empty filename provided");
        return SaveError::INVALID_FILENAME;
    }
    
    if (filename.length() > 255) {
        if (logger) logger->Error("Filename too long: " + std::to_string(filename.length()) + " characters");
        return SaveError::INVALID_FILENAME;
    }
    
    // Check for invalid characters
    const std::string invalid_chars = "<>:\"|?*";
    if (filename.find_first_of(invalid_chars) != std::string::npos) {
        if (logger) logger->Error("Filename contains invalid characters");
        return SaveError::INVALID_FILENAME;
    }
    
    // Check for path traversal attempts
    if (filename.find("..") != std::string::npos || 
        filename.find("/") != std::string::npos || 
        filename.find("\\") != std::string::npos) {
        if (logger) logger->Error("Path traversal attempt detected in filename");
        return SaveError::PATH_TRAVERSAL;
    }
    
#ifdef _WIN32
    // Check for Windows reserved names
    if (IsWindowsReserved(filename)) {
        if (logger) logger->Error("Filename is a Windows reserved name");
        return SaveError::INVALID_FILENAME;
    }
#endif
    
    // Ensure filename has .save extension
    std::string safe_filename = filename;
    if (!string_ends_with(safe_filename, ".save")) {
        safe_filename += ".save";
    }
    
    // Construct and canonicalize path
    std::error_code ec;
    auto full_path = base_dir / safe_filename;
    auto canonical_base = std::filesystem::canonical(base_dir, ec);
    
    if (ec) {
        if (logger) logger->Error("Failed to canonicalize base directory: " + ec.message());
        return SaveError::UNKNOWN_ERROR;
    }
    
    // Verify the resulting path is within the base directory
    auto canonical_full = std::filesystem::weakly_canonical(full_path);
    if (!string_starts_with(canonical_full.string(), canonical_base.string())) {
        if (logger) logger->Error("Path escapes base directory");
        return SaveError::PATH_TRAVERSAL;
    }
    
    return canonical_full;
}

bool SecurePathResolver::IsWindowsReserved(const std::string& name) {
    static const std::vector<std::string> reserved = {
        "CON", "PRN", "AUX", "NUL",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };
    
    std::string upper_name = name;
    std::transform(upper_name.begin(), upper_name.end(), upper_name.begin(), ::toupper);
    
    // Check base name without extension
    size_t dot_pos = upper_name.find('.');
    if (dot_pos != std::string::npos) {
        upper_name = upper_name.substr(0, dot_pos);
    }
    
    return std::find(reserved.begin(), reserved.end(), upper_name) != reserved.end();
}

} // namespace core::save
