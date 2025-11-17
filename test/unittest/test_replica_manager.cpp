/**
 * @file test_replica_manager.cpp
 * @brief Unit tests for CReplicaManager (M-out-of-N replica redundancy)
 * @date 2025-11-14
 */

#include <gtest/gtest.h>
#include <lap/core/CCore.hpp>
#include <lap/core/CPath.hpp>
#include <lap/core/CFile.hpp>
#include "CReplicaManager.hpp"

using namespace lap::core;
using namespace lap::per;

class ReplicaManagerTest : public ::testing::Test {
protected:
    String testBasePath;
    UniqueHandle<CReplicaManager> replicaMgr;
    
    void SetUp() override {
        testBasePath = "/tmp/replica_test";
        
        // Clean up previous test data
        if (Path::isDirectory(testBasePath)) {
            Path::removeDirectory(testBasePath, true);
        }
        Path::createDirectory(testBasePath);
        
        // Create replica manager with N=3, M=2
        replicaMgr = MakeUnique<CReplicaManager>(
            testBasePath,
            3,  // N=3 replicas
            2,  // M=2 minimum valid
            ChecksumType::kCRC32
        );
    }
    
    void TearDown() override {
        replicaMgr.reset();
        
        // Clean up test data
        if (Path::isDirectory(testBasePath)) {
            Path::removeDirectory(testBasePath, true);
        }
    }
};

// Test basic write operation
TEST_F(ReplicaManagerTest, Write_CreatesAllReplicas) {
    String testData = "Hello, Replica World!";
    
    auto result = replicaMgr->Write(
        "test_file.txt",
        reinterpret_cast<const UInt8*>(testData.c_str()),
        testData.length()
    );
    
    ASSERT_TRUE(result.HasValue());
    
    // Verify all 3 replicas exist
    EXPECT_TRUE(File::Util::exists(testBasePath + "/test_file.txt.replica_0"));
    EXPECT_TRUE(File::Util::exists(testBasePath + "/test_file.txt.replica_1"));
    EXPECT_TRUE(File::Util::exists(testBasePath + "/test_file.txt.replica_2"));
}

// Test read with all replicas valid
TEST_F(ReplicaManagerTest, Read_AllReplicasValid) {
    String testData = "Test data for reading";
    
    // Write data
    auto writeResult = replicaMgr->Write(
        "read_test.txt",
        reinterpret_cast<const UInt8*>(testData.c_str()),
        testData.length()
    );
    ASSERT_TRUE(writeResult.HasValue());
    
    // Read data back
    auto readResult = replicaMgr->Read("read_test.txt");
    ASSERT_TRUE(readResult.HasValue());
    
    auto& data = readResult.Value();
    String readData(reinterpret_cast<const char*>(data.data()), data.size());
    
    EXPECT_EQ(readData, testData);
}

// Test M-out-of-N consensus (corrupt one replica)
TEST_F(ReplicaManagerTest, Read_OneReplicaCorrupted_Consensus) {
    String testData = "Consensus test data";
    
    // Write data
    auto writeResult = replicaMgr->Write(
        "consensus_test.txt",
        reinterpret_cast<const UInt8*>(testData.c_str()),
        testData.length()
    );
    ASSERT_TRUE(writeResult.HasValue());
    
    // Corrupt replica 1
    String replica1Path = testBasePath + "/consensus_test.txt.replica_1";
    std::ofstream ofs(replica1Path, std::ios::binary | std::ios::trunc);
    ofs << "CORRUPTED DATA";
    ofs.close();
    
    // Read should still succeed with M=2 valid replicas
    auto readResult = replicaMgr->Read("consensus_test.txt");
    ASSERT_TRUE(readResult.HasValue());
    
    auto& data = readResult.Value();
    String readData(reinterpret_cast<const char*>(data.data()), data.size());
    
    EXPECT_EQ(readData, testData);
}

// Test failure when too many replicas corrupted
TEST_F(ReplicaManagerTest, Read_TwoReplicasCorrupted_Failure) {
    String testData = "Data with multiple corruptions";
    
    // Write data
    auto writeResult = replicaMgr->Write(
        "multi_corrupt_test.txt",
        reinterpret_cast<const UInt8*>(testData.c_str()),
        testData.length()
    );
    ASSERT_TRUE(writeResult.HasValue());
    
    // Corrupt replicas 0 and 1 (only 1 valid replica remains)
    // With N=3, M=2, having only 1 valid replica means we cannot achieve consensus
    String replica0Path = testBasePath + "/multi_corrupt_test.txt.replica_0";
    String replica1Path = testBasePath + "/multi_corrupt_test.txt.replica_1";
    
    std::ofstream ofs0(replica0Path, std::ios::binary | std::ios::trunc);
    ofs0 << "CORRUPTED_DATA_0";
    ofs0.close();
    
    std::ofstream ofs1(replica1Path, std::ios::binary | std::ios::trunc);
    ofs1 << "CORRUPTED_DATA_1";
    ofs1.close();
    
    // Read should fail (only 1 valid replica, need M=2 for consensus)
    auto readResult = replicaMgr->Read("multi_corrupt_test.txt");
    EXPECT_FALSE(readResult.HasValue());
}

// Test CheckStatus
TEST_F(ReplicaManagerTest, CheckStatus_ReturnsMetadata) {
    String testData = "Status test data";
    
    // Write data
    auto writeResult = replicaMgr->Write(
        "status_test.txt",
        reinterpret_cast<const UInt8*>(testData.c_str()),
        testData.length()
    );
    ASSERT_TRUE(writeResult.HasValue());
    
    // Check status
    auto statusResult = replicaMgr->CheckStatus("status_test.txt");
    ASSERT_TRUE(statusResult.HasValue());
    
    auto& metadata = statusResult.Value();
    
    EXPECT_EQ(metadata.logicalFileName, "status_test.txt");
    EXPECT_EQ(metadata.totalReplicas, 3u);
    EXPECT_EQ(metadata.replicas.size(), 3u);
    
    // All replicas should be valid
    for (const auto& replica : metadata.replicas) {
        EXPECT_TRUE(replica.valid);
    }
}

// Test Repair functionality
TEST_F(ReplicaManagerTest, Repair_FixesCorruptedReplica) {
    String testData = "Repair test data with some length";
    
    // Write data
    auto writeResult = replicaMgr->Write(
        "repair_test.txt",
        reinterpret_cast<const UInt8*>(testData.c_str()),
        testData.length()
    );
    ASSERT_TRUE(writeResult.HasValue());
    
    // Delete replica 2 to simulate corruption
    String replica2Path = testBasePath + "/repair_test.txt.replica_2";
    ::std::filesystem::remove(replica2Path);
    
    // Verify corruption (replica missing counts as invalid)
    auto statusBefore = replicaMgr->CheckStatus("repair_test.txt");
    ASSERT_TRUE(statusBefore.HasValue());
    int validCountBefore = 0;
    for (const auto& replica : statusBefore.Value().replicas) {
        if (replica.valid) validCountBefore++;
    }
    // Should have 2 valid replicas (replica_0 and replica_1)
    EXPECT_EQ(validCountBefore, 2);
    
    // Repair
    auto repairResult = replicaMgr->Repair("repair_test.txt");
    ASSERT_TRUE(repairResult.HasValue());
    
    // Verify all replicas are now valid
    auto statusAfter = replicaMgr->CheckStatus("repair_test.txt");
    ASSERT_TRUE(statusAfter.HasValue());
    
    for (const auto& replica : statusAfter.Value().replicas) {
        EXPECT_TRUE(replica.valid);
    }
}

// Test ListFiles
TEST_F(ReplicaManagerTest, ListFiles_ReturnsLogicalNames) {
    // Write multiple files
    String data1 = "File 1";
    String data2 = "File 2";
    String data3 = "File 3";
    
    replicaMgr->Write("file1.txt", reinterpret_cast<const UInt8*>(data1.c_str()), data1.length());
    replicaMgr->Write("file2.txt", reinterpret_cast<const UInt8*>(data2.c_str()), data2.length());
    replicaMgr->Write("file3.txt", reinterpret_cast<const UInt8*>(data3.c_str()), data3.length());
    
    // List files
    auto listResult = replicaMgr->ListFiles();
    ASSERT_TRUE(listResult.HasValue());
    
    auto& files = listResult.Value();
    EXPECT_EQ(files.size(), 3u);
    
    // Should return logical names, not replica names
    EXPECT_TRUE(std::find(files.begin(), files.end(), "file1.txt") != files.end());
    EXPECT_TRUE(std::find(files.begin(), files.end(), "file2.txt") != files.end());
    EXPECT_TRUE(std::find(files.begin(), files.end(), "file3.txt") != files.end());
}

// Test Delete
TEST_F(ReplicaManagerTest, Delete_RemovesAllReplicas) {
    String testData = "Delete test data";
    
    // Write data
    auto writeResult = replicaMgr->Write(
        "delete_test.txt",
        reinterpret_cast<const UInt8*>(testData.c_str()),
        testData.length()
    );
    ASSERT_TRUE(writeResult.HasValue());
    
    // Verify replicas exist
    EXPECT_TRUE(File::Util::exists(testBasePath + "/delete_test.txt.replica_0"));
    EXPECT_TRUE(File::Util::exists(testBasePath + "/delete_test.txt.replica_1"));
    EXPECT_TRUE(File::Util::exists(testBasePath + "/delete_test.txt.replica_2"));
    
    // Delete
    auto deleteResult = replicaMgr->Delete("delete_test.txt");
    ASSERT_TRUE(deleteResult.HasValue());
    
    // Verify all replicas are gone
    EXPECT_FALSE(File::Util::exists(testBasePath + "/delete_test.txt.replica_0"));
    EXPECT_FALSE(File::Util::exists(testBasePath + "/delete_test.txt.replica_1"));
    EXPECT_FALSE(File::Util::exists(testBasePath + "/delete_test.txt.replica_2"));
}

// Test Reconfigure
TEST_F(ReplicaManagerTest, Reconfigure_ChangesReplicaCount) {
    // Initial configuration: N=3, M=2
    String testData = "Reconfigure test";
    
    auto writeResult = replicaMgr->Write(
        "reconfig_test.txt",
        reinterpret_cast<const UInt8*>(testData.c_str()),
        testData.length()
    );
    ASSERT_TRUE(writeResult.HasValue());
    
    // Reconfigure to N=5, M=3 (Reconfigure only takes 2 params, checksum type is fixed)
    auto reconfigResult = replicaMgr->Reconfigure(5, 3);
    EXPECT_TRUE(reconfigResult.HasValue());
    
    // Write new file with new configuration
    String newData = "New config data";
    auto newWriteResult = replicaMgr->Write(
        "new_config_test.txt",
        reinterpret_cast<const UInt8*>(newData.c_str()),
        newData.length()
    );
    ASSERT_TRUE(newWriteResult.HasValue());
    
    // Should have 5 replicas now
    EXPECT_TRUE(File::Util::exists(testBasePath + "/new_config_test.txt.replica_0"));
    EXPECT_TRUE(File::Util::exists(testBasePath + "/new_config_test.txt.replica_1"));
    EXPECT_TRUE(File::Util::exists(testBasePath + "/new_config_test.txt.replica_2"));
    EXPECT_TRUE(File::Util::exists(testBasePath + "/new_config_test.txt.replica_3"));
    EXPECT_TRUE(File::Util::exists(testBasePath + "/new_config_test.txt.replica_4"));
}

// Test with SHA256 checksum
TEST_F(ReplicaManagerTest, SHA256_Checksum) {
    // Create manager with SHA256
    auto sha256Mgr = MakeUnique<CReplicaManager>(
        testBasePath + "/sha256",
        3,
        2,
        ChecksumType::kSHA256
    );
    
    Path::createDirectory(testBasePath + "/sha256");
    
    String testData = "SHA256 test data";
    auto writeResult = sha256Mgr->Write(
        "sha256_test.txt",
        reinterpret_cast<const UInt8*>(testData.c_str()),
        testData.length()
    );
    ASSERT_TRUE(writeResult.HasValue());
    
    auto readResult = sha256Mgr->Read("sha256_test.txt");
    ASSERT_TRUE(readResult.HasValue());
    
    auto& data = readResult.Value();
    String readData(reinterpret_cast<const char*>(data.data()), data.size());
    
    EXPECT_EQ(readData, testData);
}

// Test large file
TEST_F(ReplicaManagerTest, LargeFile_MultipleReplicas) {
    // Create 1MB of data
    Vector<UInt8> largeData(1024 * 1024);
    for (size_t i = 0; i < largeData.size(); i++) {
        largeData[i] = static_cast<UInt8>(i % 256);
    }
    
    auto writeResult = replicaMgr->Write("large_file.bin", largeData.data(), largeData.size());
    ASSERT_TRUE(writeResult.HasValue());
    
    auto readResult = replicaMgr->Read("large_file.bin");
    ASSERT_TRUE(readResult.HasValue());
    
    auto& readData = readResult.Value();
    EXPECT_EQ(readData.size(), largeData.size());
    EXPECT_EQ(readData, largeData);
}
