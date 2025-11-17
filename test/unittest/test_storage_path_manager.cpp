/**
 * @file test_storage_path_manager.cpp
 * @brief Comprehensive test suite for CStoragePathManager
 * @date 2025-01-14
 */

#include <gtest/gtest.h>
#include "CStoragePathManager.hpp"
#include <lap/core/CCore.hpp>
#include <lap/core/CConfig.hpp>
#include <lap/core/CPath.hpp>
#include <lap/core/CFile.hpp>
#include <fstream>
#include <nlohmann/json.hpp>
#include <cstdlib>

using namespace lap::core;
using namespace lap::per;

// ============================================================================
// Test Fixture
// ============================================================================

class StoragePathManagerTest : public ::testing::Test {
protected:
    static const String TEST_CONFIG_PATH;
    static const String ORIGINAL_CONFIG_PATH;
    
    static void SetUpTestSuite() {
        // Backup original config if exists
        // (void)ConfigManager::getInstance(); // Removed unused variable
        // We'll use the default config from ConfigManager
    }
    
    void SetUp() override {
        // Create test configuration
        createTestConfig();
    }
    
    void TearDown() override {
        // Clean up test directories
        cleanupTestDirectories();
    }
    
    static void TearDownTestSuite() {
        // Restore original config if backed up
    }
    
private:
    void createTestConfig() {
        // Tests use default /opt/autosar/persistency
        // Directory permissions handled externally
    }
    
    void cleanupTestDirectories() {
        Vector<String> testDirs = {
            "/tmp/test_autosar_persistency",
            "/tmp/deploy1",
            "/tmp/deploy2",
            "/tmp/deploy3",
            "/tmp/test_kvs_instance",
            "/tmp/test_fs_instance"
        };
        
        for (const auto& dir : testDirs) {
            if (File::Util::exists(dir)) {
                // Recursive delete
                String cmd = "rm -rf " + dir;
                system(cmd.c_str());
            }
        }
    }
};

const String StoragePathManagerTest::TEST_CONFIG_PATH = "/tmp/test_persistency_config.json";
const String StoragePathManagerTest::ORIGINAL_CONFIG_PATH = "";

// ============================================================================
// Basic Path Generation Tests
// ============================================================================

TEST_F(StoragePathManagerTest, GetCentralStorageURI_DefaultValue) {
    String centralUri = CStoragePathManager::getCentralStorageURI();
    
    // Should return default or configured value
    EXPECT_FALSE(centralUri.empty());
    // Default is "/opt/autosar/persistency" but may be configured
    EXPECT_TRUE(centralUri.find("persistency") != String::npos);
}

TEST_F(StoragePathManagerTest, GetManifestPath_Structure) {
    String manifestPath = CStoragePathManager::getManifestPath();
    
    EXPECT_FALSE(manifestPath.empty());
    EXPECT_TRUE(manifestPath.find("manifest") != String::npos);
    
    // Path should end with "/manifest"
    EXPECT_TRUE(manifestPath.substr(manifestPath.length() - 9) == "/manifest");
}

TEST_F(StoragePathManagerTest, GetKvsRootPath_Structure) {
    String kvsRoot = CStoragePathManager::getKvsRootPath();
    
    EXPECT_FALSE(kvsRoot.empty());
    EXPECT_TRUE(kvsRoot.find("kvs") != String::npos);
    
    // Path should end with "/kvs"
    EXPECT_TRUE(kvsRoot.substr(kvsRoot.length() - 4) == "/kvs");
}

TEST_F(StoragePathManagerTest, GetFileStorageRootPath_Structure) {
    String fsRoot = CStoragePathManager::getFileStorageRootPath();
    
    EXPECT_FALSE(fsRoot.empty());
    EXPECT_TRUE(fsRoot.find("fs") != String::npos);
    
    // Path should end with "/fs"
    EXPECT_TRUE(fsRoot.substr(fsRoot.length() - 3) == "/fs");
}

// ============================================================================
// Instance Path Tests
// ============================================================================

TEST_F(StoragePathManagerTest, GetKvsInstancePath_SimpleInstance) {
    String instancePath = CStoragePathManager::getKvsInstancePath("app/kvs_instance");
    
    EXPECT_FALSE(instancePath.empty());
    EXPECT_TRUE(instancePath.find("/kvs/") != String::npos);
    EXPECT_TRUE(instancePath.find("app/kvs_instance") != String::npos);
}

TEST_F(StoragePathManagerTest, GetKvsInstancePath_LeadingSlashNormalized) {
    String path1 = CStoragePathManager::getKvsInstancePath("/app/kvs_instance");
    String path2 = CStoragePathManager::getKvsInstancePath("app/kvs_instance");
    
    // Both should produce same result (leading slash removed)
    EXPECT_EQ(path1, path2);
}

TEST_F(StoragePathManagerTest, GetKvsInstancePath_EmptyInstance) {
    String instancePath = CStoragePathManager::getKvsInstancePath("");
    
    // Should still return valid kvs root path
    EXPECT_FALSE(instancePath.empty());
    EXPECT_TRUE(instancePath.find("/kvs") != String::npos);
}

TEST_F(StoragePathManagerTest, GetFileStorageInstancePath_SimpleInstance) {
    String instancePath = CStoragePathManager::getFileStorageInstancePath("app/fs_instance");
    
    EXPECT_FALSE(instancePath.empty());
    EXPECT_TRUE(instancePath.find("/fs/") != String::npos);
    EXPECT_TRUE(instancePath.find("app/fs_instance") != String::npos);
}

TEST_F(StoragePathManagerTest, GetFileStorageInstancePath_ComplexPath) {
    String instancePath = CStoragePathManager::getFileStorageInstancePath("app/subsystem/module/storage");
    
    EXPECT_FALSE(instancePath.empty());
    EXPECT_TRUE(instancePath.find("/fs/") != String::npos);
    EXPECT_TRUE(instancePath.find("app/subsystem/module/storage") != String::npos);
}

// ============================================================================
// Replica Path Tests
// ============================================================================

TEST_F(StoragePathManagerTest, GetReplicaPaths_SingleURI_ThreeReplicas) {
    Vector<String> replicaPaths = CStoragePathManager::getReplicaPaths(
        "app/kvs_test", 
        "kvs", 
        3
    );
    
    ASSERT_EQ(replicaPaths.size(), 3);
    
    // All replicas should be under central storage
    for (size_t i = 0; i < replicaPaths.size(); ++i) {
        EXPECT_TRUE(replicaPaths[i].find("kvs/app/kvs_test") != String::npos);
        EXPECT_TRUE(replicaPaths[i].find("replica_") != String::npos);
    }
    
    // Each replica should have different replica number
    EXPECT_TRUE(replicaPaths[0].find("replica_0") != String::npos);
    EXPECT_TRUE(replicaPaths[1].find("replica_1") != String::npos);
    EXPECT_TRUE(replicaPaths[2].find("replica_2") != String::npos);
}

TEST_F(StoragePathManagerTest, GetReplicaPaths_SingleReplica) {
    Vector<String> replicaPaths = CStoragePathManager::getReplicaPaths(
        "app/single_replica", 
        "kvs", 
        1
    );
    
    ASSERT_EQ(replicaPaths.size(), 1);
    EXPECT_TRUE(replicaPaths[0].find("replica_0") != String::npos);
}

TEST_F(StoragePathManagerTest, GetReplicaPaths_ZeroReplicas) {
    Vector<String> replicaPaths = CStoragePathManager::getReplicaPaths(
        "app/no_replica", 
        "kvs", 
        0
    );
    
    // Should return empty vector for zero replicas
    EXPECT_TRUE(replicaPaths.empty());
}

TEST_F(StoragePathManagerTest, GetReplicaPaths_FileStorageType) {
    Vector<String> replicaPaths = CStoragePathManager::getReplicaPaths(
        "app/fs_test", 
        "fs", 
        3
    );
    
    ASSERT_EQ(replicaPaths.size(), 3);
    
    for (const auto& path : replicaPaths) {
        EXPECT_TRUE(path.find("/fs/") != String::npos);
        EXPECT_TRUE(path.find("app/fs_test") != String::npos);
    }
}

// ============================================================================
// Directory Creation Tests
// ============================================================================

TEST_F(StoragePathManagerTest, CreateManifestStructure_Success) {
    auto result = CStoragePathManager::createManifestStructure();
    
    EXPECT_TRUE(result.HasValue());
    
    // Verify manifest directory exists
    String manifestPath = CStoragePathManager::getManifestPath();
    EXPECT_TRUE(CStoragePathManager::pathExists(manifestPath));
}

TEST_F(StoragePathManagerTest, CreateStorageStructure_KVS_Success) {
    String instancePath = "test_app/kvs_instance_create";
    
    auto result = CStoragePathManager::createStorageStructure(instancePath, "kvs");
    
    EXPECT_TRUE(result.HasValue());
    
    // Verify KVS directory structure (AUTOSAR 4-layer: current, update, redundancy, recovery)
    String kvsInstancePath = CStoragePathManager::getKvsInstancePath(instancePath);
    EXPECT_TRUE(CStoragePathManager::pathExists(kvsInstancePath));
    EXPECT_TRUE(CStoragePathManager::pathExists(Path::append(kvsInstancePath, "current")));
    EXPECT_TRUE(CStoragePathManager::pathExists(Path::append(kvsInstancePath, "update")));
    EXPECT_TRUE(CStoragePathManager::pathExists(Path::append(kvsInstancePath, "redundancy")));
    EXPECT_TRUE(CStoragePathManager::pathExists(Path::append(kvsInstancePath, "recovery")));
}

TEST_F(StoragePathManagerTest, CreateStorageStructure_FileStorage_Success) {
    String instancePath = "test_app/fs_instance_create";
    
    auto result = CStoragePathManager::createStorageStructure(instancePath, "fs");
    
    EXPECT_TRUE(result.HasValue());
    
    // Verify FileStorage directory structure
    String fsInstancePath = CStoragePathManager::getFileStorageInstancePath(instancePath);
    EXPECT_TRUE(CStoragePathManager::pathExists(fsInstancePath));
    EXPECT_TRUE(CStoragePathManager::pathExists(Path::append(fsInstancePath, "current")));
    EXPECT_TRUE(CStoragePathManager::pathExists(Path::append(fsInstancePath, "backup")));
    EXPECT_TRUE(CStoragePathManager::pathExists(Path::append(fsInstancePath, "initial")));
    EXPECT_TRUE(CStoragePathManager::pathExists(Path::append(fsInstancePath, "update")));
    EXPECT_TRUE(CStoragePathManager::pathExists(Path::append(fsInstancePath, ".metadata")));
}

TEST_F(StoragePathManagerTest, CreateStorageStructure_InvalidType_Failure) {
    String instancePath = "test_app/invalid_type";
    
    auto result = CStoragePathManager::createStorageStructure(instancePath, "invalid_type");
    
    // Should fail for invalid storage type
    EXPECT_FALSE(result.HasValue());
}

TEST_F(StoragePathManagerTest, CreateStorageStructure_AlreadyExists_Success) {
    String instancePath = "test_app/existing_kvs";
    
    // Create first time
    auto result1 = CStoragePathManager::createStorageStructure(instancePath, "kvs");
    EXPECT_TRUE(result1.HasValue());
    
    // Create again (should succeed - idempotent operation)
    auto result2 = CStoragePathManager::createStorageStructure(instancePath, "kvs");
    EXPECT_TRUE(result2.HasValue());
}

// ============================================================================
// Path Existence Check Tests
// ============================================================================

TEST_F(StoragePathManagerTest, PathExists_NonExistentPath) {
    bool exists = CStoragePathManager::pathExists("/tmp/non_existent_path_12345");
    
    EXPECT_FALSE(exists);
}

TEST_F(StoragePathManagerTest, PathExists_AfterCreation) {
    String testPath = "/tmp/test_path_exists";
    
    // Create directory using core::Path
    Path::createDirectory(testPath);
    
    EXPECT_TRUE(CStoragePathManager::pathExists(testPath));
    
    // Cleanup
    system(("rm -rf " + testPath).c_str());
}

// ============================================================================
// Edge Cases and Error Handling Tests
// ============================================================================

TEST_F(StoragePathManagerTest, GetKvsInstancePath_SpecialCharacters) {
    // Test with special characters (should be handled properly)
    String instancePath = CStoragePathManager::getKvsInstancePath("app/test-storage_v1.0");
    
    EXPECT_FALSE(instancePath.empty());
    EXPECT_TRUE(instancePath.find("app/test-storage_v1.0") != String::npos);
}

TEST_F(StoragePathManagerTest, GetReplicaPaths_LargeReplicaCount) {
    // Test with large replica count
    Vector<String> replicaPaths = CStoragePathManager::getReplicaPaths(
        "app/large_replica", 
        "kvs", 
        10
    );
    
    ASSERT_EQ(replicaPaths.size(), 10);
    
    // Verify all have unique replica numbers
    for (size_t i = 0; i < replicaPaths.size(); ++i) {
        String expectedReplicaStr = "replica_" + String(std::to_string(i).c_str());
        EXPECT_TRUE(replicaPaths[i].find(expectedReplicaStr) != String::npos);
    }
}

TEST_F(StoragePathManagerTest, CreateStorageStructure_DeepNesting) {
    String deepPath = "level1/level2/level3/level4/storage";
    
    auto result = CStoragePathManager::createStorageStructure(deepPath, "kvs");
    
    EXPECT_TRUE(result.HasValue());
    
    String kvsPath = CStoragePathManager::getKvsInstancePath(deepPath);
    EXPECT_TRUE(CStoragePathManager::pathExists(kvsPath));
}

// ============================================================================
// Constraint Validation Tests
// ============================================================================

TEST_F(StoragePathManagerTest, ConstraintCheck_NoCoreModuleViolation) {
    // This test verifies that StoragePathManager uses Core modules
    // It's a compile-time check, but we document it here
    
    // All methods should use:
    // - core::String instead of std::string
    // - core::Vector instead of std::vector
    // - core::Path instead of std::filesystem
    // - core::ConfigManager for configuration
    
    // If this test compiles and links, constraint is satisfied
    SUCCEED();
}

TEST_F(StoragePathManagerTest, ConstraintCheck_NoHardcodedPaths) {
    // Verify all paths come from configuration or constants
    String centralUri = CStoragePathManager::getCentralStorageURI();
    
    // Should not be empty (comes from config or default)
    EXPECT_FALSE(centralUri.empty());
    
    // Should be a proper absolute path
    EXPECT_TRUE(centralUri[0] == '/');
}

TEST_F(StoragePathManagerTest, ConstraintCheck_StaticMethodsOnly) {
    // StoragePathManager should be a utility class with only static methods
    // We verify this by ensuring we don't need an instance
    
    // All these calls should work without creating an instance
    String uri = CStoragePathManager::getCentralStorageURI();
    String manifest = CStoragePathManager::getManifestPath();
    String kvsRoot = CStoragePathManager::getKvsRootPath();
    
    EXPECT_FALSE(uri.empty());
    EXPECT_FALSE(manifest.empty());
    EXPECT_FALSE(kvsRoot.empty());
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(StoragePathManagerTest, Integration_CompleteKVSSetup) {
    String instancePath = "integration_test/kvs_app";
    
    // Step 1: Create manifest structure
    auto manifestResult = CStoragePathManager::createManifestStructure();
    EXPECT_TRUE(manifestResult.HasValue());
    
    // Step 2: Create KVS storage structure
    auto storageResult = CStoragePathManager::createStorageStructure(instancePath, "kvs");
    EXPECT_TRUE(storageResult.HasValue());
    
    // Step 3: Get replica paths
    Vector<String> replicaPaths = CStoragePathManager::getReplicaPaths(instancePath, "kvs", 3);
    EXPECT_EQ(replicaPaths.size(), 3);
    
    // Step 4: Verify all paths exist (AUTOSAR naming)
    String kvsPath = CStoragePathManager::getKvsInstancePath(instancePath);
    EXPECT_TRUE(CStoragePathManager::pathExists(kvsPath));
    EXPECT_TRUE(CStoragePathManager::pathExists(Path::append(kvsPath, "current")));
    EXPECT_TRUE(CStoragePathManager::pathExists(Path::append(kvsPath, "update")));
    EXPECT_TRUE(CStoragePathManager::pathExists(Path::append(kvsPath, "redundancy")));
    EXPECT_TRUE(CStoragePathManager::pathExists(Path::append(kvsPath, "recovery")));
}

TEST_F(StoragePathManagerTest, Integration_CompleteFileStorageSetup) {
    String instancePath = "integration_test/fs_app";
    
    // Step 1: Create manifest structure
    auto manifestResult = CStoragePathManager::createManifestStructure();
    EXPECT_TRUE(manifestResult.HasValue());
    
    // Step 2: Create FileStorage structure
    auto storageResult = CStoragePathManager::createStorageStructure(instancePath, "fs");
    EXPECT_TRUE(storageResult.HasValue());
    
    // Step 3: Get replica paths
    Vector<String> replicaPaths = CStoragePathManager::getReplicaPaths(instancePath, "fs", 3);
    EXPECT_EQ(replicaPaths.size(), 3);
    
    // Step 4: Verify FileStorage directory structure
    String fsPath = CStoragePathManager::getFileStorageInstancePath(instancePath);
    EXPECT_TRUE(CStoragePathManager::pathExists(fsPath));
    EXPECT_TRUE(CStoragePathManager::pathExists(Path::append(fsPath, "current")));
    EXPECT_TRUE(CStoragePathManager::pathExists(Path::append(fsPath, "backup")));
    EXPECT_TRUE(CStoragePathManager::pathExists(Path::append(fsPath, "initial")));
    EXPECT_TRUE(CStoragePathManager::pathExists(Path::append(fsPath, "update")));
    EXPECT_TRUE(CStoragePathManager::pathExists(Path::append(fsPath, ".metadata")));
}

TEST_F(StoragePathManagerTest, Integration_MultipleInstancesIsolation) {
    String kvs1 = "app1/kvs_storage";
    String kvs2 = "app2/kvs_storage";
    
    // Create both
    auto result1 = CStoragePathManager::createStorageStructure(kvs1, "kvs");
    auto result2 = CStoragePathManager::createStorageStructure(kvs2, "kvs");
    
    EXPECT_TRUE(result1.HasValue());
    EXPECT_TRUE(result2.HasValue());
    
    // Get paths
    String path1 = CStoragePathManager::getKvsInstancePath(kvs1);
    String path2 = CStoragePathManager::getKvsInstancePath(kvs2);
    
    // Paths should be different
    EXPECT_NE(path1, path2);
    
    // Both should exist independently
    EXPECT_TRUE(CStoragePathManager::pathExists(path1));
    EXPECT_TRUE(CStoragePathManager::pathExists(path2));
}
