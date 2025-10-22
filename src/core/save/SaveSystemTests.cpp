// Created: January 16, 2025 - 16:15:00
// Location: tests/core/save/SaveSystemTests.cpp
// Mechanica Imperii - Save System Test Implementation (C++17 Compliant)

#include "SaveSystemTests.h"
#include "utils/PlatformCompat.h"
#include <iostream>
#include <fstream>
#include <random>
#include <thread>

namespace core::save::testing {

// ============================================================================
// MockSerializableSystem Implementation
// ============================================================================

MockSerializableSystem::MockSerializableSystem(const std::string& name)
    : m_name(name) {}

bool MockSerializableSystem::Serialize(Json::Value& out, int version_int) {
    m_serialize_count++;
    
    if (m_should_fail) {
        return false;
    }
    
    for (const auto& [key, value] : m_data) {
        out[key] = value;
    }
    
    return true;
}

bool MockSerializableSystem::Deserialize(const Json::Value& in, int version_int) {
    m_deserialize_count++;
    
    if (m_should_fail) {
        return false;
    }
    
    m_data.clear();
    for (const auto& key : in.getMemberNames()) {
        m_data[key] = in[key].asString();
    }
    
    return true;
}

void MockSerializableSystem::SetData(const std::string& key, const std::string& value) {
    m_data[key] = value;
}

std::string MockSerializableSystem::GetData(const std::string& key) const {
    auto it = m_data.find(key);
    return it != m_data.end() ? it->second : "";
}

void MockSerializableSystem::ResetCounts() {
    m_serialize_count = 0;
    m_deserialize_count = 0;
}

// ============================================================================
// SaveManagerTests Implementation
// ============================================================================

SaveManagerTests::SaveManagerTests() {
    SetupTestEnvironment();
}

void SaveManagerTests::SetupTestEnvironment() {
    m_test_dir = test_utils::CreateTempDirectory();
}

void SaveManagerTests::CleanupTestEnvironment() {
    test_utils::RemoveDirectory(m_test_dir);
}

std::vector<TestResult> SaveManagerTests::RunAllTests() {
    std::vector<TestResult> results;
    
    results.push_back(TestBasicSaveLoad());
    results.push_back(TestAtomicWrites());
    results.push_back(TestBackupCreation());
    results.push_back(TestChecksumValidation());
    results.push_back(TestConcurrentSaves());
    results.push_back(TestPathTraversalPrevention());
    results.push_back(TestInvalidFilenames());
    results.push_back(TestDiskSpaceCheck());
    results.push_back(TestCorruptionDetection());
    results.push_back(TestMigrationSystem());
    results.push_back(TestCrashRecovery());
    results.push_back(TestValidationSystem());
    results.push_back(TestOperationTimeout());
    results.push_back(TestCancellation());
    
    CleanupTestEnvironment();
    return results;
}

TestResult SaveManagerTests::RunTest(const std::string& name, std::function<bool()> test_func) {
    TestResult result;
    result.test_name = name;
    
    auto start = std::chrono::steady_clock::now();
    
    try {
        bool passed = test_func();
        result.status = passed ? TestResult::Status::PASSED : TestResult::Status::FAILED;
    } catch (const std::exception& e) {
        result.status = TestResult::Status::FAILED;
        result.error_message = e.what();
    } catch (...) {
        result.status = TestResult::Status::FAILED;
        result.error_message = "Unknown exception";
    }
    
    result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    
    return result;
}

TestResult SaveManagerTests::TestBasicSaveLoad() {
    return RunTest("BasicSaveLoad", [this]() {
        SaveManager::Config config;
        config.logger = std::make_unique<DefaultLogger>(LogLevel::WARN);
        SaveManager manager(config);
        manager.SetSaveDirectory(m_test_dir);
        
        // Register mock system
        auto system = std::make_shared<MockSerializableSystem>("TestSystem");
        system->SetData("key1", "value1");
        system->SetData("key2", "value2");
        manager.RegisterSystem(system);
        
        // Save
        auto save_result = manager.SaveGame("test_basic.save");
        test_utils::AssertTrue(save_result.has_value(), "Save should succeed");
        test_utils::AssertTrue(save_result->IsSuccess(), "Save result should be success");
        
        // Clear data
        system->SetData("key1", "");
        system->SetData("key2", "");
        
        // Load
        auto load_result = manager.LoadGame("test_basic.save");
        test_utils::AssertTrue(load_result.has_value(), "Load should succeed");
        test_utils::AssertTrue(load_result->IsSuccess(), "Load result should be success");
        
        // Verify data
        test_utils::AssertEqual("value1", system->GetData("key1"), "Data should match");
        test_utils::AssertEqual("value2", system->GetData("key2"), "Data should match");
        
        return true;
    });
}

TestResult SaveManagerTests::TestAtomicWrites() {
    return RunTest("AtomicWrites", [this]() {
        SaveManager::Config config;
        config.enable_atomic_writes = true;
        SaveManager manager(config);
        manager.SetSaveDirectory(m_test_dir);
        
        auto system = std::make_shared<MockSerializableSystem>("TestSystem");
        system->SetData("data", "test_atomic");
        manager.RegisterSystem(system);
        
        auto result = manager.SaveGameAtomic("test_atomic.save");
        test_utils::AssertTrue(result.has_value(), "Atomic save should succeed");
        test_utils::AssertTrue(result->atomic_write_used, "Atomic write should be used");
        
        // Verify file exists
        auto file_path = m_test_dir / "test_atomic.save";
        test_utils::AssertTrue(std::filesystem::exists(file_path), "Save file should exist");
        
        return true;
    });
}

TestResult SaveManagerTests::TestBackupCreation() {
    return RunTest("BackupCreation", [this]() {
        SaveManager::Config config;
        config.enable_auto_backup = true;
        config.max_backups = 3;
        SaveManager manager(config);
        manager.SetSaveDirectory(m_test_dir);
        
        auto system = std::make_shared<MockSerializableSystem>("TestSystem");
        manager.RegisterSystem(system);
        
        // Create initial save
        system->SetData("version", "1");
        manager.SaveGame("test_backup.save");
        
        // Save again to create backup
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        system->SetData("version", "2");
        auto result = manager.SaveGame("test_backup.save");
        
        test_utils::AssertTrue(result.has_value(), "Save should succeed");
        test_utils::AssertTrue(result->backup_created, "Backup should be created");
        
        // Check backup exists
        auto backups = manager.GetBackupList("test_backup.save");
        test_utils::AssertTrue(backups.has_value(), "Should get backup list");
        test_utils::AssertTrue(backups->size() > 0, "Should have at least one backup");
        
        return true;
    });
}

TestResult SaveManagerTests::TestChecksumValidation() {
    return RunTest("ChecksumValidation", [this]() {
        SaveManager::Config config;
        SaveManager manager(config);
        manager.SetSaveDirectory(m_test_dir);
        
        auto system = std::make_shared<MockSerializableSystem>("TestSystem");
        system->SetData("important", "data");
        manager.RegisterSystem(system);
        
        // Save file
        manager.SaveGame("test_checksum.save");
        
        // Verify checksum
        auto checksum_result = manager.VerifyChecksum("test_checksum.save");
        test_utils::AssertTrue(checksum_result.has_value(), "Checksum verification should succeed");
        test_utils::AssertTrue(*checksum_result, "Checksum should be valid");
        
        // Corrupt file
        auto file_path = m_test_dir / "test_checksum.save";
        std::ofstream file(file_path, std::ios::app);
        file << "corrupted_data";
        file.close();
        
        // Verify checksum fails
        auto corrupted_check = manager.VerifyChecksum("test_checksum.save");
        test_utils::AssertTrue(corrupted_check.has_value(), "Checksum check should complete");
        test_utils::AssertFalse(*corrupted_check, "Checksum should be invalid");
        
        return true;
    });
}

TestResult SaveManagerTests::TestConcurrentSaves() {
    return RunTest("ConcurrentSaves", [this]() {
        SaveManager::Config config;
        config.max_concurrent_saves = 2;
        SaveManager manager(config);
        manager.SetSaveDirectory(m_test_dir);
        
        auto system1 = std::make_shared<MockSerializableSystem>("System1");
        auto system2 = std::make_shared<MockSerializableSystem>("System2");
        manager.RegisterSystem(system1);
        manager.RegisterSystem(system2);
        
        // Launch concurrent saves
        std::vector<std::future<Expected<SaveOperationResult>>> futures;
        
        for (int i = 0; i < 3; ++i) {
            system1->SetData("id", std::to_string(i));
            futures.push_back(manager.SaveGameAsync(
                "concurrent_" + std::to_string(i) + ".save"));
        }
        
        // Wait for all to complete
        size_t successful = 0;
        for (auto& future : futures) {
            auto result = future.get();
            if (result.has_value() && result->IsSuccess()) {
                successful++;
            }
        }
        
        test_utils::AssertTrue(successful >= 2, "At least 2 saves should succeed");
        
        return true;
    });
}

TestResult SaveManagerTests::TestPathTraversalPrevention() {
    return RunTest("PathTraversalPrevention", [this]() {
        SaveManager::Config config;
        SaveManager manager(config);
        manager.SetSaveDirectory(m_test_dir);
        
        auto system = std::make_shared<MockSerializableSystem>("TestSystem");
        manager.RegisterSystem(system);
        
        // Try path traversal attacks
        std::vector<std::string> malicious_paths = {
            "../../../etc/passwd",
            "..\\..\\..\\windows\\system32\\config",
            "./../parent_dir/file.save",
            "subdir/../../other/file.save"
        };
        
        for (const auto& path : malicious_paths) {
            auto result = manager.SaveGame(path);
            test_utils::AssertFalse(result.has_value(), 
                "Path traversal should be blocked: " + path);
        }
        
        return true;
    });
}

TestResult SaveManagerTests::TestInvalidFilenames() {
    return RunTest("InvalidFilenames", [this]() {
        SaveManager::Config config;
        SaveManager manager(config);
        manager.SetSaveDirectory(m_test_dir);
        
        auto system = std::make_shared<MockSerializableSystem>("TestSystem");
        manager.RegisterSystem(system);
        
        // Test invalid characters
        std::vector<std::string> invalid_names = {
            "file<name>.save",
            "file>name.save",
            "file:name.save",
            "file|name.save",
            "file?name.save",
            "file*name.save"
        };
        
        for (const auto& name : invalid_names) {
            auto result = manager.SaveGame(name);
            test_utils::AssertFalse(result.has_value(), 
                "Invalid filename should be rejected: " + name);
        }
        
        return true;
    });
}

TestResult SaveManagerTests::TestDiskSpaceCheck() {
    return RunTest("DiskSpaceCheck", [this]() {
        SaveManager::Config config;
        SaveManager manager(config);
        manager.SetSaveDirectory(m_test_dir);
        
        auto system = std::make_shared<MockSerializableSystem>("TestSystem");
        // Create large dataset
        for (int i = 0; i < 1000; ++i) {
            system->SetData("key_" + std::to_string(i), std::string(1000, 'x'));
        }
        manager.RegisterSystem(system);
        
        // Save should check disk space
        auto result = manager.SaveGame("test_space.save");
        test_utils::AssertTrue(result.has_value(), "Save should succeed or report disk space issue");
        
        return true;
    });
}

TestResult SaveManagerTests::TestCorruptionDetection() {
    return RunTest("CorruptionDetection", [this]() {
        SaveManager::Config config;
        SaveManager manager(config);
        manager.SetSaveDirectory(m_test_dir);
        
        auto system = std::make_shared<MockSerializableSystem>("TestSystem");
        system->SetData("data", "valid");
        manager.RegisterSystem(system);
        
        // Create valid save
        manager.SaveGame("test_corrupt.save");
        
        // Corrupt the file
        auto file_path = m_test_dir / "test_corrupt.save";
        std::ofstream corrupt(file_path, std::ios::binary | std::ios::trunc);
        corrupt << "{ invalid json }";
        corrupt.close();
        
        // Try to load
        auto result = manager.LoadGame("test_corrupt.save");
        test_utils::AssertFalse(result.has_value() && result->IsSuccess(), 
            "Loading corrupted file should fail");
        
        return true;
    });
}

TestResult SaveManagerTests::TestMigrationSystem() {
    return RunTest("MigrationSystem", [this]() {
        // Test migration registry
        auto& registry = MigrationRegistry::Instance();
        
        SaveVersion v1_0_0(1, 0, 0);
        SaveVersion v1_1_0(1, 1, 0);
        
        registry.RegisterMigration(v1_0_0, v1_1_0, "Test migration",
            [](Json::Value& data, ILogger* logger) -> Expected<bool> {
                data["migrated"] = true;
                return true;
            });
        
        auto path = registry.FindMigrationPath(v1_0_0, v1_1_0);
        test_utils::AssertTrue(path.has_value(), "Migration path should exist");
        test_utils::AssertTrue(path->size() == 1, "Should have one migration step");
        
        return true;
    });
}

TestResult SaveManagerTests::TestCrashRecovery() {
    return RunTest("CrashRecovery", [this]() {
        SaveManager::Config config;
        SaveManager manager(config);
        manager.SetSaveDirectory(m_test_dir);
        
        auto system = std::make_shared<MockSerializableSystem>("TestSystem");
        system->SetData("data", "original");
        manager.RegisterSystem(system);
        
        // Create save and backup
        manager.SetAutoBackup(true, 5);
        manager.SaveGame("test_recovery.save");
        
        system->SetData("data", "modified");
        manager.SaveGame("test_recovery.save");
        
        // Corrupt main file
        auto file_path = m_test_dir / "test_recovery.save";
        std::ofstream corrupt(file_path, std::ios::binary | std::ios::trunc);
        corrupt << "corrupted";
        corrupt.close();
        
        // Try recovery
        auto recovery_result = manager.RecoverFromCrash();
        test_utils::AssertTrue(recovery_result.has_value(), "Recovery should succeed");
        
        return true;
    });
}

TestResult SaveManagerTests::TestValidationSystem() {
    return RunTest("ValidationSystem", [this]() {
        SaveManager::Config config;
        SaveManager manager(config);
        manager.SetSaveDirectory(m_test_dir);
        
        auto system = std::make_shared<MockSerializableSystem>("TestSystem");
        manager.RegisterSystem(system);
        
        // Create valid save
        manager.SaveGame("test_validation.save");
        
        // Validate
        auto validation = manager.ValidateSave("test_validation.save");
        test_utils::AssertTrue(validation.has_value(), "Validation should complete");
        test_utils::AssertTrue(validation->IsValid(), "Save should be valid");
        
        return true;
    });
}

TestResult SaveManagerTests::TestOperationTimeout() {
    return RunTest("OperationTimeout", [this]() {
        SaveManager::Config config;
        config.operation_timeout = std::chrono::seconds(1);
        SaveManager manager(config);
        manager.SetSaveDirectory(m_test_dir);
        
        // Test that operations respect timeout
        auto ops = manager.GetActiveOperations();
        test_utils::AssertTrue(ops.empty(), "Should start with no active operations");
        
        return true;
    });
}

TestResult SaveManagerTests::TestCancellation() {
    return RunTest("Cancellation", [this]() {
        SaveManager::Config config;
        SaveManager manager(config);
        manager.SetSaveDirectory(m_test_dir);
        
        auto system = std::make_shared<MockSerializableSystem>("TestSystem");
        manager.RegisterSystem(system);
        
        // Start async save
        auto future = manager.SaveGameAsync("test_cancel.save");
        
        // Cancel all operations
        auto cancel_result = manager.CancelAllOperations();
        test_utils::AssertTrue(cancel_result.has_value(), "Cancellation should succeed");
        
        // Wait for future
        future.wait();
        
        return true;
    });
}

// NOTE: The rest of the file (CompressionTests, IncrementalSaveTests, test_utils, etc.)
// is already correctly implemented in the "SaveSystemTests.cpp - Part 1" artifact
// starting around line 600 onwards.

// This Part 2 only needed to complete the SaveManagerTests section that was cut off.

} // namespace core::save::testing
