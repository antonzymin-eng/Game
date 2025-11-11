// Created: January 16, 2025 - 16:45:00
// Location: tools/save_viewer/save_viewer_tool.cpp
// Mechanica Imperii - Save File Viewer CLI Tool (C++17 Compliant)

#include "core/save/SaveManager.h"
#include "core/save/SaveCompression.h"
#include "core/save/IncrementalSaveTracker.h"
#include "utils/PlatformCompat.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include "core/logging/Logger.h"
#endif

using namespace core::save;

namespace console {

    bool EnableVirtualTerminal() {
#ifdef _WIN32
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) return false;
        
        DWORD dwMode = 0;
        if (!GetConsoleMode(hOut, &dwMode)) return false;
        
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        return SetConsoleMode(hOut, dwMode);
#else
        return true;
#endif
    }
    
    bool colors_enabled = false;
    
    void Initialize() {
        colors_enabled = EnableVirtualTerminal();
        if (!colors_enabled) {
            CORE_STREAM_ERROR("SaveViewerTool") << "Warning: Console does not support color output\n";
        }
    }
}

// Update colors namespace to use dynamic checks
namespace colors {
    const char* GetColor(const char* ansi) {
        return console::colors_enabled ? ansi : "";
    }
    
    const char* RESET() { return GetColor("\033[0m"); }
    const char* RED() { return GetColor("\033[31m"); }
    // ... etc for all colors
}

// ============================================================================
// CLI Helper Functions
// ============================================================================

void PrintHeader(const std::string& title) {
    CORE_STREAM_INFO("SaveViewerTool") << "\n" << colors::BOLD << colors::CYAN;
    CORE_STREAM_INFO("SaveViewerTool") << "═══════════════════════════════════════════════════════════\n";
    CORE_STREAM_INFO("SaveViewerTool") << "  " << title << "\n";
    CORE_STREAM_INFO("SaveViewerTool") << "═══════════════════════════════════════════════════════════\n";
    CORE_STREAM_INFO("SaveViewerTool") << colors::RESET << "\n";
}

void PrintSection(const std::string& section) {
    CORE_STREAM_INFO("SaveViewerTool") << colors::BOLD << colors::BLUE << "\n▶ " << section << colors::RESET << "\n";
    CORE_STREAM_INFO("SaveViewerTool") << "───────────────────────────────────────────────────────────\n";
}

void PrintSuccess(const std::string& message) {
    CORE_STREAM_INFO("SaveViewerTool") << colors::GREEN << "✓ " << message << colors::RESET << "\n";
}

void PrintError(const std::string& message) {
    CORE_STREAM_INFO("SaveViewerTool") << colors::RED << "✗ " << message << colors::RESET << "\n";
}

void PrintWarning(const std::string& message) {
    CORE_STREAM_INFO("SaveViewerTool") << colors::YELLOW << "⚠ " << message << colors::RESET << "\n";
}

void PrintInfo(const std::string& key, const std::string& value) {
    CORE_STREAM_INFO("SaveViewerTool") << "  " << colors::CYAN << std::left << std::setw(25) << key << ": " 
              << colors::RESET << value << "\n";
}

std::string FormatBytes(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unit_index = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit_index < 3) {
        size /= 1024.0;
        unit_index++;
    }
    
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
    return ss.str();
}

std::string FormatDuration(std::chrono::milliseconds ms) {
    if (ms.count() < 1000) {
        return std::to_string(ms.count()) + "ms";
    } else if (ms.count() < 60000) {
        return std::to_string(ms.count() / 1000) + "s";
    } else {
        return std::to_string(ms.count() / 60000) + "m " + 
               std::to_string((ms.count() % 60000) / 1000) + "s";
    }
}

// ============================================================================
// Save File Viewer Operations
// ============================================================================

class SaveFileViewer {
public:
    SaveFileViewer(const std::string& save_dir) : m_save_dir(save_dir) {
        SaveManager::Config config;
        config.logger = std::make_unique<DefaultLogger>(LogLevel::WARN);
        m_manager = std::make_unique<SaveManager>(config);
        m_manager->SetSaveDirectory(save_dir);
    }
    
    void ListSaveFiles() {
        PrintHeader("Available Save Files");
        
        auto files_result = m_manager->GetSaveFileList();
        if (!files_result.has_value()) {
            PrintError("Failed to list save files");
            return;
        }
        
        const auto& files = *files_result;
        
        if (files.empty()) {
            PrintWarning("No save files found in: " + m_save_dir);
            return;
        }
        
        CORE_STREAM_INFO("SaveViewerTool") << "Found " << colors::BOLD << files.size() << colors::RESET << " save file(s):\n\n";
        
        for (const auto& file : files) {
            std::string filename = file.filename().string();
            
            // Get file size
            auto size_result = m_manager->GetSaveFileSize(filename);
            std::string size_str = size_result.has_value() ? FormatBytes(*size_result) : "Unknown";
            
            // Get timestamp
            auto time_result = m_manager->GetSaveFileTimestamp(filename);
            std::string time_str = "Unknown";
            if (time_result.has_value()) {
                auto time = std::chrono::system_clock::to_time_t(*time_result);
                char buffer[100];
                std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&time));
                time_str = buffer;
            }
            
            // Get version
            auto version_result = m_manager->GetSaveFileVersion(filename);
            std::string version_str = version_result.has_value() ? version_result->ToString() : "Unknown";
            
            CORE_STREAM_INFO("SaveViewerTool") << colors::BOLD << "  " << filename << colors::RESET << "\n";
            PrintInfo("    Size", size_str);
            PrintInfo("    Version", version_str);
            PrintInfo("    Modified", time_str);
            CORE_STREAM_INFO("SaveViewerTool") << "\n";
        }
    }
    
    void InspectSaveFile(const std::string& filename) {
        PrintHeader("Save File Inspector: " + filename);
        
        // Check if file exists
        auto exists_result = m_manager->SaveFileExists(filename);
        if (!exists_result.has_value() || !*exists_result) {
            PrintError("Save file not found: " + filename);
            return;
        }
        
        PrintSection("File Information");
        
        // File size
        auto size_result = m_manager->GetSaveFileSize(filename);
        if (size_result.has_value()) {
            PrintInfo("File Size", FormatBytes(*size_result));
        }
        
        // Version
        auto version_result = m_manager->GetSaveFileVersion(filename);
        if (version_result.has_value()) {
            PrintInfo("Save Version", version_result->ToString());
        }
        
        // Timestamp
        auto time_result = m_manager->GetSaveFileTimestamp(filename);
        if (time_result.has_value()) {
            auto time = std::chrono::system_clock::to_time_t(*time_result);
            char buffer[100];
            std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&time));
            PrintInfo("Last Modified", buffer);
        }
        
        PrintSection("Validation Results");
        
        // Validate save file
        auto validation_result = m_manager->ValidateSave(filename);
        if (validation_result.has_value()) {
            const auto& report = *validation_result;
            
            if (report.IsValid()) {
                PrintSuccess("Save file is valid");
            } else {
                PrintError("Save file has validation errors");
            }
            
            PrintInfo("Validation Time", std::to_string(report.validation_time.count()) + "ms");
            PrintInfo("Total Issues", std::to_string(report.issues.size()));
            PrintInfo("Errors", std::to_string(report.GetErrorCount()));
            PrintInfo("Warnings", std::to_string(report.GetWarningCount()));
            PrintInfo("Critical", std::to_string(report.GetCriticalCount()));
            
            if (!report.issues.empty()) {
                CORE_STREAM_INFO("SaveViewerTool") << "\n" << colors::YELLOW << "Issues Found:" << colors::RESET << "\n";
                for (const auto& issue : report.issues) {
                    std::string severity;
                    const char* color;
                    
                    switch (issue.severity) {
                        case ValidationIssue::CRITICAL:
                            severity = "CRITICAL";
                            color = colors::RED;
                            break;
                        case ValidationIssue::ERROR:
                            severity = "ERROR";
                            color = colors::RED;
                            break;
                        case ValidationIssue::WARNING:
                            severity = "WARNING";
                            color = colors::YELLOW;
                            break;
                    }
                    
                    CORE_STREAM_INFO("SaveViewerTool") << "  " << color << "[" << severity << "]" << colors::RESET 
                              << " " << issue.validator_name;
                    if (!issue.field_path.empty()) {
                        CORE_STREAM_INFO("SaveViewerTool") << " at " << issue.field_path;
                    }
                    CORE_STREAM_INFO("SaveViewerTool") << ": " << issue.message << "\n";
                    
                    if (issue.suggested_fix) {
                        CORE_STREAM_INFO("SaveViewerTool") << "    " << colors::GREEN << "Suggested Fix: " 
                                  << *issue.suggested_fix << colors::RESET << "\n";
                    }
                }
            }
        }
        
        // Checksum verification
        auto checksum_result = m_manager->VerifyChecksum(filename);
        if (checksum_result.has_value()) {
            if (*checksum_result) {
                PrintSuccess("Checksum verification passed");
            } else {
                PrintError("Checksum verification failed - file may be corrupted");
            }
        }
        
        PrintSection("Content Preview");
        
        // Read and display header information
        auto file_path = std::filesystem::path(m_save_dir) / filename;
        std::ifstream file(file_path);
        if (file.is_open()) {
            Json::Value root;
            Json::CharReaderBuilder builder;
            std::string errors;
            
            if (Json::parseFromStream(builder, file, &root, &errors)) {
                if (root.isMember("header")) {
                    const Json::Value& header = root["header"];
                    
                    if (header.isMember("game_name")) {
                        PrintInfo("Game Name", header["game_name"].asString());
                    }
                    if (header.isMember("save_format")) {
                        PrintInfo("Save Format", header["save_format"].asString());
                    }
                    if (header.isMember("timestamp")) {
                        auto timestamp = header["timestamp"].asInt64();
                        auto time = std::chrono::system_clock::to_time_t(
                            std::chrono::system_clock::time_point(std::chrono::seconds(timestamp)));
                        char buffer[100];
                        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&time));
                        PrintInfo("Created", buffer);
                    }
                }
                
                if (root.isMember("systems")) {
                    const Json::Value& systems = root["systems"];
                    auto system_names = systems.getMemberNames();
                    
                    CORE_STREAM_INFO("SaveViewerTool") << "\n" << colors::BOLD << "Systems in Save (" 
                              << system_names.size() << "):" << colors::RESET << "\n";
                    
                    for (const auto& name : system_names) {
                        CORE_STREAM_INFO("SaveViewerTool") << "  • " << name << "\n";
                    }
                }
            } else {
                PrintError("Failed to parse JSON: " + errors);
            }
        }
    }
    
    void ValidateSaveFile(const std::string& filename, bool verbose = false) {
        PrintHeader("Validating: " + filename);
        
        SaveManager::VerificationOptions options;
        options.check_structure = true;
        options.check_checksums = true;
        options.run_validators = true;
        options.check_migrations = true;
        options.verbose = verbose;
        
        auto result = m_manager->VerifyFile(filename, options);
        
        if (!result.has_value()) {
            PrintError("Verification failed");
            return;
        }
        
        const auto& report = *result;
        
        if (report.IsValid()) {
            PrintSuccess("All validation checks passed!");
        } else {
            PrintError("Validation failed with " + std::to_string(report.issues.size()) + " issue(s)");
        }
        
        PrintInfo("Validation Time", std::to_string(report.validation_time.count()) + "ms");
        
        if (verbose) {
            CORE_STREAM_INFO("SaveViewerTool") << "\n" << report.GenerateReport();
        }
    }
    
    void ShowStatistics() {
        PrintHeader("Save System Statistics");
        
        auto stats = m_manager->GetSaveStats();
        
        PrintSection("Operation Counts");
        PrintInfo("Total Saves", std::to_string(stats.total_saves));
        PrintInfo("Successful Saves", std::to_string(stats.successful_saves));
        PrintInfo("Failed Saves", std::to_string(stats.failed_saves));
        PrintInfo("Total Loads", std::to_string(stats.total_loads));
        PrintInfo("Successful Loads", std::to_string(stats.successful_loads));
        PrintInfo("Failed Loads", std::to_string(stats.failed_loads));
        
        PrintSection("Success Rates");
        PrintInfo("Save Success Rate", 
                  std::to_string(static_cast<int>(stats.GetSaveSuccessRate() * 100)) + "%");
        PrintInfo("Load Success Rate", 
                  std::to_string(static_cast<int>(stats.GetLoadSuccessRate() * 100)) + "%");
        
        PrintSection("Performance");
        PrintInfo("Average Save Time", FormatDuration(stats.average_save_time));
        PrintInfo("Average Load Time", FormatDuration(stats.average_load_time));
        PrintInfo("Total Bytes Saved", FormatBytes(stats.total_bytes_saved));
        
        PrintSection("Advanced Metrics");
        PrintInfo("Corrupted Saves Recovered", std::to_string(stats.corrupted_saves_recovered));
        PrintInfo("Migrations Performed", std::to_string(stats.migrations_performed));
        PrintInfo("Cache Hit Ratio", 
                  std::to_string(static_cast<int>(stats.json_cache_stats.hit_ratio() * 100)) + "%");
    }
    
    void CompareCompressionAlgorithms(const std::string& filename) {
        PrintHeader("Compression Benchmark: " + filename);
        
        // Read file data
        auto file_path = std::filesystem::path(m_save_dir) / filename;
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            PrintError("Failed to open file");
            return;
        }
        
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), 
                                  std::istreambuf_iterator<char>());
        
        PrintInfo("Original Size", FormatBytes(data.size()));
        
        CompressionManager manager;
        auto results = manager.BenchmarkAlgorithms(data.data(), data.size());
        
        CORE_STREAM_INFO("SaveViewerTool") << "\n" << colors::BOLD << "Algorithm Comparison:" << colors::RESET << "\n\n";
        
        CORE_STREAM_INFO("SaveViewerTool") << std::left << std::setw(12) << "Algorithm" 
                  << std::setw(15) << "Ratio" 
                  << std::setw(18) << "Compress (ms)" 
                  << std::setw(20) << "Decompress (ms)"
                  << "Throughput\n";
        CORE_STREAM_INFO("SaveViewerTool") << std::string(80, '─') << "\n";
        
        for (const auto& result : results) {
            CORE_STREAM_INFO("SaveViewerTool") << std::left << std::setw(12) << ToString(result.algorithm);
            CORE_STREAM_INFO("SaveViewerTool") << std::setw(15) << (std::to_string(static_cast<int>(result.compression_ratio * 100)) + "%");
            CORE_STREAM_INFO("SaveViewerTool") << std::setw(18) << std::fixed << std::setprecision(2) << result.compression_time_ms;
            CORE_STREAM_INFO("SaveViewerTool") << std::setw(20) << std::fixed << std::setprecision(2) << result.decompression_time_ms;
            CORE_STREAM_INFO("SaveViewerTool") << std::fixed << std::setprecision(2) << result.throughput_mbps << " MB/s\n";
        }
    }
    
private:
    std::string m_save_dir;
    std::unique_ptr<SaveManager> m_manager;
};

// ============================================================================
// Main CLI Interface
// ============================================================================


void PrintUsage(const char* program_name) {
    CORE_STREAM_INFO("SaveViewerTool") << colors::BOLD << "Mechanica Imperii - Save File Viewer\n" << colors::RESET;
    CORE_STREAM_INFO("SaveViewerTool") << "Usage: " << program_name << " [command] [options]\n\n";
    
    CORE_STREAM_INFO("SaveViewerTool") << "Commands:\n";
    CORE_STREAM_INFO("SaveViewerTool") << "  list                      List all save files\n";
    CORE_STREAM_INFO("SaveViewerTool") << "  inspect <filename>        Inspect a save file\n";
    CORE_STREAM_INFO("SaveViewerTool") << "  validate <filename>       Validate a save file\n";
    CORE_STREAM_INFO("SaveViewerTool") << "  stats                     Show save system statistics\n";
    CORE_STREAM_INFO("SaveViewerTool") << "  benchmark <filename>      Benchmark compression algorithms\n";
    CORE_STREAM_INFO("SaveViewerTool") << "  help                      Show this help message\n\n";
    
    CORE_STREAM_INFO("SaveViewerTool") << "Options:\n";
    CORE_STREAM_INFO("SaveViewerTool") << "  --dir <path>              Set save directory (default: ./saves)\n";
    CORE_STREAM_INFO("SaveViewerTool") << "  --verbose                 Enable verbose output\n\n";
    
    CORE_STREAM_INFO("SaveViewerTool") << "Examples:\n";
    CORE_STREAM_INFO("SaveViewerTool") << "  " << program_name << " list\n";
    CORE_STREAM_INFO("SaveViewerTool") << "  " << program_name << " inspect autosave.save\n";
    CORE_STREAM_INFO("SaveViewerTool") << "  " << program_name << " validate --verbose game1.save\n";
    CORE_STREAM_INFO("SaveViewerTool") << "  " << program_name << " benchmark --dir ./saves autosave.save\n";
}

int main(int argc, char* argv[]) {
 	console::Initialize();  // ADD THIS LINE    

	if (argc < 2) {
        PrintUsage(argv[0]);
        return 1;
    }
    
    std::string command = argv[1];
    std::string save_dir = "saves";
    bool verbose = false;
    std::string filename;
    
    // Parse arguments
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--dir" && i + 1 < argc) {
            save_dir = argv[++i];
        } else if (arg == "--verbose") {
            verbose = true;
        } else if (filename.empty()) {
            filename = arg;
        }
    }
    
    try {
        SaveFileViewer viewer(save_dir);
        
        if (command == "list") {
            viewer.ListSaveFiles();
        } else if (command == "inspect" && !filename.empty()) {
            viewer.InspectSaveFile(filename);
        } else if (command == "validate" && !filename.empty()) {
            viewer.ValidateSaveFile(filename, verbose);
        } else if (command == "stats") {
            viewer.ShowStatistics();
        } else if (command == "benchmark" && !filename.empty()) {
            viewer.CompareCompressionAlgorithms(filename);
        } else if (command == "help") {
            PrintUsage(argv[0]);
        } else {
            PrintError("Unknown command or missing arguments");
            PrintUsage(argv[0]);
            return 1;
        }
        
    } catch (const std::exception& e) {
        PrintError("Exception: " + std::string(e.what()));
        return 1;
    }
    
    return 0;
}
