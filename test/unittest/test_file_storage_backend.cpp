/**
 * @file test_file_storage_backend.cpp
 * @brief Unit tests for CFileStorageBackend (basic file operations)
 * @date 2025-11-15
 * 
 * Phase 2.1 Refactoring: Tests only basic file operations.
 * Lifecycle management tests moved to test_persistency_manager.cpp (Phase 2.2)
 */

#include <gtest/gtest.h>
#include <lap/core/CCore.hpp>
#include <lap/core/CPath.hpp>
#include <lap/core/CFile.hpp>
#include "CFileStorageBackend.hpp"

using namespace lap::core;
using namespace lap::per;

class FileStorageBackendTest : public ::testing::Test {
protected:
    String testStoragePath;
    UniqueHandle<CFileStorageBackend> backend;
    
    void SetUp() override {
        testStoragePath = "/tmp/fs_backend_test";
        
        // Clean up previous test data
        if (Path::isDirectory(testStoragePath)) {
            Path::removeDirectory(testStoragePath, true);
        }
        
        // Create base directory structure
        Path::createDirectory(testStoragePath);
        Path::createDirectory(testStoragePath + "/current");
        Path::createDirectory(testStoragePath + "/backup");
        Path::createDirectory(testStoragePath + "/initial");
        Path::createDirectory(testStoragePath + "/update");
        
        backend = MakeUnique<CFileStorageBackend>(testStoragePath);
    }
    
    void TearDown() override {
        backend.reset();
        
        // Clean up test data
        if (Path::isDirectory(testStoragePath)) {
            Path::removeDirectory(testStoragePath, true);
        }
    }
};

// ========== Basic File Operations ==========

TEST_F(FileStorageBackendTest, WriteFile_CreatesFile) {
    Vector<Byte> testData = {'H', 'e', 'l', 'l', 'o'};
    
    auto result = backend->WriteFile("test.txt", testData, "current");
    ASSERT_TRUE(result.HasValue());
    
    // Verify file exists
    EXPECT_TRUE(backend->FileExists("test.txt", "current"));
}

TEST_F(FileStorageBackendTest, ReadFile_ReturnsCorrectContent) {
    Vector<Byte> originalData = {'T', 'e', 's', 't', ' ', 'D', 'a', 't', 'a'};
    
    // Write file
    auto writeResult = backend->WriteFile("test.txt", originalData, "current");
    ASSERT_TRUE(writeResult.HasValue());
    
    // Read file
    auto readResult = backend->ReadFile("test.txt", "current");
    ASSERT_TRUE(readResult.HasValue());
    
    // Verify content
    EXPECT_EQ(readResult.Value(), originalData);
}

TEST_F(FileStorageBackendTest, ReadFile_NonexistentFile_ReturnsError) {
    auto result = backend->ReadFile("nonexistent.txt", "current");
    EXPECT_FALSE(result.HasValue());
}

TEST_F(FileStorageBackendTest, DeleteFile_RemovesFile) {
    Vector<Byte> testData = {'D', 'e', 'l', 'e', 't', 'e'};
    
    // Create file
    backend->WriteFile("delete_me.txt", testData, "current");
    ASSERT_TRUE(backend->FileExists("delete_me.txt", "current"));
    
    // Delete file
    auto result = backend->DeleteFile("delete_me.txt", "current");
    ASSERT_TRUE(result.HasValue());
    
    // Verify deletion
    EXPECT_FALSE(backend->FileExists("delete_me.txt", "current"));
}

TEST_F(FileStorageBackendTest, ListFiles_ReturnsAllFiles) {
    // Create multiple files
    backend->WriteFile("file1.txt", Vector<Byte>{'1'}, "current");
    backend->WriteFile("file2.txt", Vector<Byte>{'2'}, "current");
    backend->WriteFile("file3.txt", Vector<Byte>{'3'}, "current");
    
    // List files
    auto result = backend->ListFiles("current");
    ASSERT_TRUE(result.HasValue());
    
    // Verify count
    EXPECT_EQ(result.Value().size(), 3u);
}

TEST_F(FileStorageBackendTest, GetFileSize_ReturnsCorrectSize) {
    Vector<Byte> testData(100, 'X');  // 100 bytes
    
    backend->WriteFile("sized.txt", testData, "current");
    
    auto result = backend->GetFileSize("sized.txt", "current");
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), 100u);
}

// ========== Category Operations ==========

TEST_F(FileStorageBackendTest, WriteFile_DifferentCategories) {
    Vector<Byte> currentData = {'C', 'u', 'r', 'r', 'e', 'n', 't'};
    Vector<Byte> backupData = {'B', 'a', 'c', 'k', 'u', 'p'};
    
    // Write to different categories
    backend->WriteFile("config.txt", currentData, "current");
    backend->WriteFile("config.txt", backupData, "backup");
    
    // Verify both exist
    EXPECT_TRUE(backend->FileExists("config.txt", "current"));
    EXPECT_TRUE(backend->FileExists("config.txt", "backup"));
    
    // Verify different content
    auto currentRead = backend->ReadFile("config.txt", "current");
    auto backupRead = backend->ReadFile("config.txt", "backup");
    
    ASSERT_TRUE(currentRead.HasValue());
    ASSERT_TRUE(backupRead.HasValue());
    EXPECT_EQ(currentRead.Value(), currentData);
    EXPECT_EQ(backupRead.Value(), backupData);
}

TEST_F(FileStorageBackendTest, CopyFile_BetweenCategories) {
    Vector<Byte> testData = {'C', 'o', 'p', 'y', ' ', 'T', 'e', 's', 't'};
    
    // Create file in current
    backend->WriteFile("original.txt", testData, "current");
    
    // Copy to backup
    auto result = backend->CopyFile("original.txt", "current", "backup");
    ASSERT_TRUE(result.HasValue());
    
    // Verify both exist
    EXPECT_TRUE(backend->FileExists("original.txt", "current"));
    EXPECT_TRUE(backend->FileExists("original.txt", "backup"));
    
    // Verify content is same
    auto backupRead = backend->ReadFile("original.txt", "backup");
    ASSERT_TRUE(backupRead.HasValue());
    EXPECT_EQ(backupRead.Value(), testData);
}

TEST_F(FileStorageBackendTest, MoveFile_BetweenCategories) {
    Vector<Byte> testData = {'M', 'o', 'v', 'e', ' ', 'T', 'e', 's', 't'};
    
    // Create file in current
    backend->WriteFile("tomove.txt", testData, "current");
    ASSERT_TRUE(backend->FileExists("tomove.txt", "current"));
    
    // Move to update
    auto result = backend->MoveFile("tomove.txt", "current", "update");
    ASSERT_TRUE(result.HasValue());
    
    // Verify source deleted, destination exists
    EXPECT_FALSE(backend->FileExists("tomove.txt", "current"));
    EXPECT_TRUE(backend->FileExists("tomove.txt", "update"));
    
    // Verify content preserved
    auto updateRead = backend->ReadFile("tomove.txt", "update");
    ASSERT_TRUE(updateRead.HasValue());
    EXPECT_EQ(updateRead.Value(), testData);
}

// ========== URI Operations ==========

TEST_F(FileStorageBackendTest, GetFileUri_ReturnsCorrectStructure) {
    auto uri = backend->GetFileUri("config.json", "current");
    
    EXPECT_EQ(uri.baseUri, testStoragePath);
    EXPECT_EQ(uri.category, "current");
    EXPECT_EQ(uri.fileName, "config.json");
    
    String fullPath = uri.GetFullPath();
    EXPECT_EQ(fullPath, testStoragePath + "/current/config.json");
}

// ========== Edge Cases ==========

TEST_F(FileStorageBackendTest, WriteFile_EmptyData) {
    Vector<Byte> emptyData;
    
    auto result = backend->WriteFile("empty.txt", emptyData, "current");
    ASSERT_TRUE(result.HasValue());
    
    // Verify empty file exists
    EXPECT_TRUE(backend->FileExists("empty.txt", "current"));
    
    auto sizeResult = backend->GetFileSize("empty.txt", "current");
    ASSERT_TRUE(sizeResult.HasValue());
    EXPECT_EQ(sizeResult.Value(), 0u);
}

TEST_F(FileStorageBackendTest, WriteFile_Overwrite) {
    Vector<Byte> data1 = {'F', 'i', 'r', 's', 't'};
    Vector<Byte> data2 = {'S', 'e', 'c', 'o', 'n', 'd', ' ', 'W', 'r', 'i', 't', 'e'};
    
    // First write
    backend->WriteFile("overwrite.txt", data1, "current");
    
    // Overwrite
    backend->WriteFile("overwrite.txt", data2, "current");
    
    // Verify new content
    auto result = backend->ReadFile("overwrite.txt", "current");
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), data2);
}

TEST_F(FileStorageBackendTest, ListFiles_EmptyCategory) {
    auto result = backend->ListFiles("initial");
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value().size(), 0u);
}

TEST_F(FileStorageBackendTest, DeleteFile_NonexistentFile_ReturnsError) {
    auto result = backend->DeleteFile("does_not_exist.txt", "current");
    EXPECT_FALSE(result.HasValue());
}

TEST_F(FileStorageBackendTest, CopyFile_SourceNotFound_ReturnsError) {
    auto result = backend->CopyFile("missing.txt", "current", "backup");
    EXPECT_FALSE(result.HasValue());
}

// ========== Large File Handling ==========

TEST_F(FileStorageBackendTest, WriteFile_LargeData) {
    // Create 1MB of data
    Vector<Byte> largeData(1024 * 1024, 'L');
    
    auto result = backend->WriteFile("large.bin", largeData, "current");
    ASSERT_TRUE(result.HasValue());
    
    // Verify size
    auto sizeResult = backend->GetFileSize("large.bin", "current");
    ASSERT_TRUE(sizeResult.HasValue());
    EXPECT_EQ(sizeResult.Value(), 1024u * 1024u);
}

// ========== Multiple File Operations ==========

TEST_F(FileStorageBackendTest, MultipleOperations_Sequential) {
    Vector<Byte> data1 = {'D', 'a', 't', 'a', '1'};
    Vector<Byte> data2 = {'D', 'a', 't', 'a', '2'};
    Vector<Byte> data3 = {'D', 'a', 't', 'a', '3'};
    
    // Write multiple files
    ASSERT_TRUE(backend->WriteFile("seq1.txt", data1, "current").HasValue());
    ASSERT_TRUE(backend->WriteFile("seq2.txt", data2, "current").HasValue());
    ASSERT_TRUE(backend->WriteFile("seq3.txt", data3, "current").HasValue());
    
    // Delete one
    ASSERT_TRUE(backend->DeleteFile("seq2.txt", "current").HasValue());
    
    // List remaining
    auto listResult = backend->ListFiles("current");
    ASSERT_TRUE(listResult.HasValue());
    EXPECT_EQ(listResult.Value().size(), 2u);
    
    // Verify specific files
    EXPECT_TRUE(backend->FileExists("seq1.txt", "current"));
    EXPECT_FALSE(backend->FileExists("seq2.txt", "current"));
    EXPECT_TRUE(backend->FileExists("seq3.txt", "current"));
}

// Entry point
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
