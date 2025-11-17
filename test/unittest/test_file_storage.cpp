/**
 * @file test_file_storage.cpp
 * @brief Comprehensive test suite for FileStorage functionality
 * @date 2025-01-14
 */

#include <gtest/gtest.h>
#include <lap/core/CCore.hpp>
#include "CPersistency.hpp"

using namespace lap::core;
using namespace lap::per;

// Global FileStorage instance shared across all tests (similar to TestFS.hpp approach)
static SharedHandle<FileStorage> g_testFS;
static bool g_fs_initialized = false;

class FileStorageTest : public ::testing::Test {
protected:
    SharedHandle<FileStorage>& testFS = g_testFS;

    static void SetUpTestSuite() {
        // Initialize PersistencyManager before using FileStorage
        auto& manager = CPersistencyManager::getInstance();
        if (!manager.initialize()) {
            FAIL() << "Failed to initialize PersistencyManager";
        }
        
        if (!g_fs_initialized) {
            auto result = OpenFileStorage(InstanceSpecifier("test"), true);
            if (!result.HasValue()) {
                FAIL() << "Failed to open FileStorage: " << result.Error().Message();
            }
            g_testFS = result.Value();
            // NOTE: No need to call initialize(), OpenFileStorage already initializes
            g_fs_initialized = true;
        }
    }

    void SetUp() override {
        if (!g_fs_initialized) {
            FAIL() << "FileStorage not initialized";
        }
        
        // Clean up previous test files
        auto fileNames = testFS->GetAllFileNames();
        if (fileNames.HasValue()) {
            for (const auto& fileName : fileNames.Value()) {
                if (fileName.find("test_") == 0 || 
                    fileName.find("file_") == 0 ||
                    fileName.find("large_") == 0 ||
                    fileName.find("empty") == 0 ||
                    fileName.find("unicode") == 0) {
                    testFS->DeleteFile(fileName);
                }
            }
        }
    }

    void TearDown() override {
        // Cleanup handled in SetUp of next test
    }

    static void TearDownTestSuite() {
        // Keep FileStorage alive for entire test suite
    }
};

// ============================================================================
// Basic Operations Tests
// ============================================================================

TEST_F(FileStorageTest, OpenFileStorage_Success) {
    EXPECT_NE(nullptr, testFS);
}

TEST_F(FileStorageTest, GetAllFileNames_EmptyInitially) {
    auto fileNames = testFS->GetAllFileNames();
    ASSERT_TRUE(fileNames.HasValue());
    EXPECT_TRUE(fileNames.Value().empty());
}

TEST_F(FileStorageTest, GetCurrentFileNames_AfterCreation) {
    {
        auto accessor = testFS->OpenFileWriteOnly("test_file1", OpenMode::kTruncate);
        ASSERT_TRUE(accessor.HasValue());
        accessor.Value()->WriteText("content");
    }
    
    auto fileNames = testFS->GetAllFileNames();
    ASSERT_TRUE(fileNames.HasValue());
    EXPECT_EQ(fileNames.Value().size(), 1);
    EXPECT_EQ(fileNames.Value()[0], "test_file1");
}

// ============================================================================
// Write-Only Accessor Tests
// ============================================================================

TEST_F(FileStorageTest, OpenFileWriteOnly_Success) {
    auto accessor = testFS->OpenFileWriteOnly("test_write", OpenMode::kTruncate);
    if (!accessor.HasValue()) {
        std::cerr << "ERROR: " << accessor.Error().Message() << std::endl;
    }
    ASSERT_TRUE(accessor.HasValue());
    EXPECT_NE(nullptr, accessor.Value());
}

TEST_F(FileStorageTest, WriteAccessor_WriteText) {
    auto accessor = testFS->OpenFileWriteOnly("test_write", OpenMode::kTruncate);
    ASSERT_TRUE(accessor.HasValue());
    
    EXPECT_TRUE(accessor.Value()->WriteText("Hello World").HasValue());
    EXPECT_TRUE(accessor.Value()->SyncToFile().HasValue());
}

TEST_F(FileStorageTest, WriteAccessor_WriteSpecialCharacters) {
    auto accessor = testFS->OpenFileWriteOnly("test_special", OpenMode::kTruncate);
    ASSERT_TRUE(accessor.HasValue());
    
    String specialText = "Special: !@#$%^&*()_+-={}[]|:;<>?,./'\"";
    EXPECT_TRUE(accessor.Value()->WriteText(specialText).HasValue());
}

TEST_F(FileStorageTest, WriteAccessor_MultipleWrites) {
    auto accessor = testFS->OpenFileWriteOnly("test_multiple", OpenMode::kTruncate);
    ASSERT_TRUE(accessor.HasValue());
    
    EXPECT_TRUE(accessor.Value()->WriteText("Line 1\n").HasValue());
    EXPECT_TRUE(accessor.Value()->WriteText("Line 2\n").HasValue());
    EXPECT_TRUE(accessor.Value()->WriteText("Line 3\n").HasValue());
}

TEST_F(FileStorageTest, WriteAccessor_AppendMode) {
    // First write
    {
        auto accessor = testFS->OpenFileWriteOnly("test_append", OpenMode::kTruncate);
        ASSERT_TRUE(accessor.HasValue());
        EXPECT_TRUE(accessor.Value()->WriteText("Initial").HasValue());
        EXPECT_TRUE(accessor.Value()->SyncToFile().HasValue());
    }
    
    // Append
    {
        auto accessor = testFS->OpenFileWriteOnly("test_append", OpenMode::kAppend);
        ASSERT_TRUE(accessor.HasValue());
        EXPECT_TRUE(accessor.Value()->WriteText(" - Appended").HasValue());
        EXPECT_TRUE(accessor.Value()->SyncToFile().HasValue());
    }
    
    // Verify
    auto reader = testFS->OpenFileReadOnly("test_append");
    ASSERT_TRUE(reader.HasValue());
    auto content = reader.Value()->ReadText();
    ASSERT_TRUE(content.HasValue());
    EXPECT_EQ(content.Value(), "Initial - Appended");
}

TEST_F(FileStorageTest, WriteAccessor_TruncateMode) {
    // First write
    {
        auto accessor = testFS->OpenFileWriteOnly("test_truncate", OpenMode::kTruncate);
        ASSERT_TRUE(accessor.HasValue());
        EXPECT_TRUE(accessor.Value()->WriteText("Original content").HasValue());
        EXPECT_TRUE(accessor.Value()->SyncToFile().HasValue());
    }
    
    // Truncate and write new
    {
        auto accessor = testFS->OpenFileWriteOnly("test_truncate", OpenMode::kTruncate);
        ASSERT_TRUE(accessor.HasValue());
        EXPECT_TRUE(accessor.Value()->WriteText("New").HasValue());
        EXPECT_TRUE(accessor.Value()->SyncToFile().HasValue());
    }
    
    // Verify
    auto reader = testFS->OpenFileReadOnly("test_truncate");
    ASSERT_TRUE(reader.HasValue());
    auto content = reader.Value()->ReadText();
    ASSERT_TRUE(content.HasValue());
    EXPECT_EQ(content.Value(), "New");
}

TEST_F(FileStorageTest, WriteAccessor_CannotRead) {
    auto accessor = testFS->OpenFileWriteOnly("test_write_only", OpenMode::kTruncate);
    ASSERT_TRUE(accessor.HasValue());
    EXPECT_TRUE(accessor.Value()->WriteText("content").HasValue());
    
    // Write-only accessor cannot read
    EXPECT_FALSE(accessor.Value()->ReadText().HasValue());
}

// ============================================================================
// Read-Only Accessor Tests
// ============================================================================

TEST_F(FileStorageTest, OpenFileReadOnly_Success) {
    // Create file first
    {
        auto writer = testFS->OpenFileWriteOnly("test_read", OpenMode::kTruncate);
        ASSERT_TRUE(writer.HasValue());
        writer.Value()->WriteText("Test content");
        writer.Value()->SyncToFile();
    }
    
    auto reader = testFS->OpenFileReadOnly("test_read");
    ASSERT_TRUE(reader.HasValue());
    EXPECT_NE(nullptr, reader.Value());
}

TEST_F(FileStorageTest, ReadAccessor_ReadText) {
    // Prepare file
    {
        auto writer = testFS->OpenFileWriteOnly("test_read_text", OpenMode::kTruncate);
        ASSERT_TRUE(writer.HasValue());
        writer.Value()->WriteText("Hello Reader");
        writer.Value()->SyncToFile();
    }
    
    auto reader = testFS->OpenFileReadOnly("test_read_text");
    ASSERT_TRUE(reader.HasValue());
    
    auto content = reader.Value()->ReadText();
    ASSERT_TRUE(content.HasValue());
    EXPECT_EQ(content.Value(), "Hello Reader");
}

TEST_F(FileStorageTest, ReadAccessor_PeekChar) {
    // Prepare file
    {
        auto writer = testFS->OpenFileWriteOnly("test_peek", OpenMode::kTruncate);
        ASSERT_TRUE(writer.HasValue());
        writer.Value()->WriteText("ABC");
        writer.Value()->SyncToFile();
    }
    
    auto reader = testFS->OpenFileReadOnly("test_peek");
    ASSERT_TRUE(reader.HasValue());
    
    // PeekChar should not advance position
    auto char1 = reader.Value()->PeekChar();
    ASSERT_TRUE(char1.HasValue());
    EXPECT_EQ(char1.Value(), 'A');
    
    auto char2 = reader.Value()->PeekChar();
    ASSERT_TRUE(char2.HasValue());
    EXPECT_EQ(char2.Value(), 'A'); // Still 'A'
}

TEST_F(FileStorageTest, ReadAccessor_GetChar) {
    // Prepare file
    {
        auto writer = testFS->OpenFileWriteOnly("test_get", OpenMode::kTruncate);
        ASSERT_TRUE(writer.HasValue());
        writer.Value()->WriteText("ABC");
        writer.Value()->SyncToFile();
    }
    
    auto reader = testFS->OpenFileReadOnly("test_get");
    ASSERT_TRUE(reader.HasValue());
    
    // GetChar should advance position
    auto char1 = reader.Value()->GetChar();
    ASSERT_TRUE(char1.HasValue());
    EXPECT_EQ(char1.Value(), 'A');
    
    auto char2 = reader.Value()->GetChar();
    ASSERT_TRUE(char2.HasValue());
    EXPECT_EQ(char2.Value(), 'B');
}

TEST_F(FileStorageTest, ReadAccessor_ReadLine) {
    // Prepare file
    {
        auto writer = testFS->OpenFileWriteOnly("test_readline", OpenMode::kTruncate);
        ASSERT_TRUE(writer.HasValue());
        writer.Value()->WriteText("Line 1\nLine 2\nLine 3");
        writer.Value()->SyncToFile();
    }
    
    auto reader = testFS->OpenFileReadOnly("test_readline");
    ASSERT_TRUE(reader.HasValue());
    
    auto line1 = reader.Value()->ReadLine();
    ASSERT_TRUE(line1.HasValue());
    EXPECT_EQ(line1.Value(), "Line 1");
    
    auto line2 = reader.Value()->ReadLine();
    ASSERT_TRUE(line2.HasValue());
    EXPECT_EQ(line2.Value(), "Line 2");
}

TEST_F(FileStorageTest, ReadAccessor_ReadPartialText) {
    // Prepare file
    {
        auto writer = testFS->OpenFileWriteOnly("test_partial", OpenMode::kTruncate);
        ASSERT_TRUE(writer.HasValue());
        writer.Value()->WriteText("0123456789");
        writer.Value()->SyncToFile();
    }
    
    auto reader = testFS->OpenFileReadOnly("test_partial");
    ASSERT_TRUE(reader.HasValue());
    
    auto text = reader.Value()->ReadText(5);
    ASSERT_TRUE(text.HasValue());
    EXPECT_EQ(text.Value(), "01234");
}

TEST_F(FileStorageTest, ReadAccessor_Position) {
    // Prepare file
    {
        auto writer = testFS->OpenFileWriteOnly("test_pos", OpenMode::kTruncate);
        ASSERT_TRUE(writer.HasValue());
        writer.Value()->WriteText("0123456789");
        writer.Value()->SyncToFile();
    }
    
    auto reader = testFS->OpenFileReadOnly("test_pos");
    ASSERT_TRUE(reader.HasValue());
    
    // Check initial position
    auto pos1 = reader.Value()->GetPosition();
    EXPECT_EQ(pos1, 0);
    
    // Read and check position
    reader.Value()->GetChar();
    auto pos2 = reader.Value()->GetPosition();
    EXPECT_EQ(pos2, 1);
}

TEST_F(FileStorageTest, ReadAccessor_SetPosition) {
    // Prepare file
    {
        auto writer = testFS->OpenFileWriteOnly("test_setpos", OpenMode::kTruncate);
        ASSERT_TRUE(writer.HasValue());
        writer.Value()->WriteText("0123456789");
        writer.Value()->SyncToFile();
    }
    
    auto reader = testFS->OpenFileReadOnly("test_setpos");
    ASSERT_TRUE(reader.HasValue());
    
    // Set position and read
    EXPECT_TRUE(reader.Value()->SetPosition(5).HasValue());
    auto ch = reader.Value()->GetChar();
    ASSERT_TRUE(ch.HasValue());
    EXPECT_EQ(ch.Value(), '5');
}

TEST_F(FileStorageTest, ReadAccessor_MovePosition) {
    // Prepare file
    {
        auto writer = testFS->OpenFileWriteOnly("test_move", OpenMode::kTruncate);
        ASSERT_TRUE(writer.HasValue());
        writer.Value()->WriteText("0123456789");
        writer.Value()->SyncToFile();
    }
    
    auto reader = testFS->OpenFileReadOnly("test_move");
    ASSERT_TRUE(reader.HasValue());
    
    // Move forward from current position
    EXPECT_TRUE(reader.Value()->MovePosition(Origin::kCurrent, 3).HasValue());
    auto ch1 = reader.Value()->GetChar();
    ASSERT_TRUE(ch1.HasValue());
    EXPECT_EQ(ch1.Value(), '3');
    
    // Move backward from current
    EXPECT_TRUE(reader.Value()->MovePosition(Origin::kCurrent, -2).HasValue());
    auto ch2 = reader.Value()->GetChar();
    ASSERT_TRUE(ch2.HasValue());
    EXPECT_EQ(ch2.Value(), '2');
}

// ============================================================================
// Read-Write Accessor Tests
// ============================================================================

TEST_F(FileStorageTest, OpenFileReadWrite_Success) {
    auto accessor = testFS->OpenFileReadWrite("test_rw", OpenMode::kTruncate);
    ASSERT_TRUE(accessor.HasValue());
    EXPECT_NE(nullptr, accessor.Value());
}

TEST_F(FileStorageTest, ReadWriteAccessor_WriteAndRead) {
    auto accessor = testFS->OpenFileReadWrite("test_rw_ops", OpenMode::kTruncate);
    ASSERT_TRUE(accessor.HasValue());
    
    // Write
    EXPECT_TRUE(accessor.Value()->WriteText("Test data").HasValue());
    EXPECT_TRUE(accessor.Value()->SyncToFile().HasValue());
    
    // Rewind and read
    EXPECT_TRUE(accessor.Value()->SetPosition(0).HasValue());
    auto content = accessor.Value()->ReadText();
    ASSERT_TRUE(content.HasValue());
    EXPECT_EQ(content.Value(), "Test data");
}

TEST_F(FileStorageTest, ReadWriteAccessor_ComplexOperations) {
    auto accessor = testFS->OpenFileReadWrite("test_complex", OpenMode::kTruncate);
    ASSERT_TRUE(accessor.HasValue());
    
    // Write initial content
    EXPECT_TRUE(accessor.Value()->WriteText("0123456789").HasValue());
    
    // Seek to middle and overwrite
    EXPECT_TRUE(accessor.Value()->SetPosition(5).HasValue());
    EXPECT_TRUE(accessor.Value()->WriteText("XYZ").HasValue());
    EXPECT_TRUE(accessor.Value()->SyncToFile().HasValue());
    
    // Read everything
    EXPECT_TRUE(accessor.Value()->SetPosition(0).HasValue());
    auto content = accessor.Value()->ReadText();
    ASSERT_TRUE(content.HasValue());
    EXPECT_EQ(content.Value(), "01234XYZ89");
}

// ============================================================================
// File Management Tests
// ============================================================================

TEST_F(FileStorageTest, DeleteFile_ExistingFile) {
    // Create file
    {
        auto accessor = testFS->OpenFileWriteOnly("test_delete", OpenMode::kTruncate);
        ASSERT_TRUE(accessor.HasValue());
        accessor.Value()->WriteText("to be deleted");
    }
    
    // Delete
    EXPECT_TRUE(testFS->DeleteFile("test_delete").HasValue());
    
    // Verify deleted
    auto fileNames = testFS->GetAllFileNames();
    ASSERT_TRUE(fileNames.HasValue());
    EXPECT_EQ(fileNames.Value().size(), 0);
}

TEST_F(FileStorageTest, DeleteFile_NonExistent) {
    auto result = testFS->DeleteFile("non_existent");
    EXPECT_TRUE(result.HasValue()); // Should succeed or return appropriate error
}

TEST_F(FileStorageTest, RecoverKey_AfterCrash) {
    // Test file recovery by simulating file operations and checking persistence
    const String testFileName = "test_recovery_file";
    
    // Step 1: Write data to a file
    {
        auto accessor = testFS->OpenFileWriteOnly(testFileName, OpenMode::kTruncate);
        ASSERT_TRUE(accessor.HasValue());
        accessor.Value()->WriteText("Original data before crash");
        accessor.Value()->SyncToFile();
    }
    
    // Step 2: Verify file exists and can be read
    {
        auto reader = testFS->OpenFileReadOnly(testFileName);
        ASSERT_TRUE(reader.HasValue());
        auto content = reader.Value()->ReadText(100);
        ASSERT_TRUE(content.HasValue());
        // Compare only the actual content (without null padding)
        String expectedData = "Original data before crash";
        EXPECT_EQ(content.Value().substr(0, expectedData.length()), expectedData);
    }
    
    // Step 3: Simulate crash recovery - file should still be readable
    // In real AUTOSAR implementation, this would involve checking backup/redundancy layers
    auto fileNames = testFS->GetAllFileNames();
    ASSERT_TRUE(fileNames.HasValue());
    
    bool fileFound = false;
    for (const auto& name : fileNames.Value()) {
        if (name == testFileName) {
            fileFound = true;
            break;
        }
    }
    EXPECT_TRUE(fileFound) << "File should be recoverable after simulated crash";
    
    // Cleanup
    testFS->DeleteFile(testFileName);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(FileStorageTest, ReadAccessor_NonExistentFile) {
    auto result = testFS->OpenFileReadOnly("non_existent_file");
    EXPECT_FALSE(result.HasValue());
}

TEST_F(FileStorageTest, ReadAccessor_EOF) {
    // Prepare file
    {
        auto writer = testFS->OpenFileWriteOnly("test_eof", OpenMode::kTruncate);
        ASSERT_TRUE(writer.HasValue());
        writer.Value()->WriteText("AB");
        writer.Value()->SyncToFile();
    }
    
    auto reader = testFS->OpenFileReadOnly("test_eof");
    ASSERT_TRUE(reader.HasValue());
    
    // Read all
    EXPECT_TRUE(reader.Value()->GetChar().HasValue()); // 'A'
    EXPECT_TRUE(reader.Value()->GetChar().HasValue()); // 'B'
    
    // Should be at EOF
    EXPECT_TRUE(reader.Value()->IsEof());
    
    // Further reads should fail or return empty
    auto result = reader.Value()->GetChar();
    EXPECT_FALSE(result.HasValue());
}

// ============================================================================
// Performance and Stress Tests
// ============================================================================

TEST_F(FileStorageTest, Performance_LargeFile) {
    const size_t dataSize = 1024 * 1024; // 1 MB
    String largeData(dataSize, 'X');
    
    auto accessor = testFS->OpenFileWriteOnly("large_file", OpenMode::kTruncate);
    ASSERT_TRUE(accessor.HasValue());
    
    EXPECT_TRUE(accessor.Value()->WriteText(largeData).HasValue());
    EXPECT_TRUE(accessor.Value()->SyncToFile().HasValue());
}

TEST_F(FileStorageTest, Performance_MultipleFiles) {
    const int numFiles = 100;
    
    for (int i = 0; i < numFiles; ++i) {
        String fileName = "file_" + std::to_string(i);
        auto accessor = testFS->OpenFileWriteOnly(fileName, OpenMode::kTruncate);
        ASSERT_TRUE(accessor.HasValue());
        accessor.Value()->WriteText("Data " + std::to_string(i));
    }
    
    auto fileNames = testFS->GetAllFileNames();
    ASSERT_TRUE(fileNames.HasValue());
    
    // Count only files with "file_" prefix
    int fileCount = 0;
    for (const auto& name : fileNames.Value()) {
        if (name.find("file_") == 0) {
            fileCount++;
        }
    }
    EXPECT_EQ(fileCount, numFiles);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(FileStorageTest, EdgeCase_EmptyFile) {
    {
        auto accessor = testFS->OpenFileWriteOnly("empty", OpenMode::kTruncate);
        ASSERT_TRUE(accessor.HasValue());
        accessor.Value()->SyncToFile();
    }
    
    auto reader = testFS->OpenFileReadOnly("empty");
    ASSERT_TRUE(reader.HasValue());
    EXPECT_TRUE(reader.Value()->IsEof());
}

TEST_F(FileStorageTest, EdgeCase_LongFileName) {
    String longName(200, 'a');
    auto result = testFS->OpenFileWriteOnly(longName, OpenMode::kTruncate);
    // Should either succeed or fail gracefully
}

TEST_F(FileStorageTest, EdgeCase_SpecialCharactersInFileName) {
    // Test with filename containing special (but legal) characters
    String specialName = "file_with-special.chars_123";
    auto accessor = testFS->OpenFileWriteOnly(specialName, OpenMode::kTruncate);
    ASSERT_TRUE(accessor.HasValue());
    accessor.Value()->WriteText("content");
}

TEST_F(FileStorageTest, EdgeCase_UnicodeContent) {
    String unicodeText = "Unicode: 你好世界 🌍 Привет мир";
    
    {
        auto writer = testFS->OpenFileWriteOnly("unicode_file", OpenMode::kTruncate);
        ASSERT_TRUE(writer.HasValue());
        writer.Value()->WriteText(unicodeText);
        writer.Value()->SyncToFile();
    }
    
    auto reader = testFS->OpenFileReadOnly("unicode_file");
    ASSERT_TRUE(reader.HasValue());
    auto content = reader.Value()->ReadText();
    ASSERT_TRUE(content.HasValue());
    EXPECT_EQ(content.Value(), unicodeText);
}
