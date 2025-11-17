/**
 * @file test_kvs_property_backend.cpp
 * @brief Unit tests for Property Backend (Shared Memory KVS)
 * @details Tests shared memory operations, persistence integration, and IPC
 */

#include <gtest/gtest.h>
#include "CKvsPropertyBackend.hpp"
#include "CKvsFileBackend.hpp"
#include "CKvsSqliteBackend.hpp"
#include <lap/core/CPath.hpp>
#include <lap/core/CFile.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

using namespace lap::per;
using namespace lap::per::util;
using namespace lap::core;

class PropertyBackendTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clean up any existing shared memory segments
        CleanupSharedMemory("test_property_basic");
        CleanupSharedMemory("test_property_persist_file");
        CleanupSharedMemory("test_property_persist_sqlite");
        CleanupSharedMemory("test_property_ipc");
        
        // Clean up persistence files
        ::std::remove("/tmp/test_property_persist_file.db");
        ::std::remove("/tmp/test_property_persist_sqlite.db");
    }
    
    void TearDown() override {
        SetUp(); // Clean up after tests
    }
    
    void CleanupSharedMemory(const char* name) {
        try {
            boost::interprocess::shared_memory_object::remove(name);
        } catch (...) {
            // Ignore errors if segment doesn't exist
        }
    }
};

// ============================================================================
// Basic Operations Tests
// ============================================================================

TEST_F(PropertyBackendTest, Constructor_CreatesSharedMemory) {
    KvsPropertyBackend backend("test_constructor", KvsBackendType::kvsFile);
    
    // Should be able to get key count
    auto countResult = backend.GetKeyCount();
    ASSERT_TRUE(countResult.HasValue());
    EXPECT_EQ(countResult.Value(), 0u);
}

TEST_F(PropertyBackendTest, SetValue_Success) {
    KvsPropertyBackend backend("test_setvalue", KvsBackendType::kvsFile);
    
    backend.SetValue("test.key", String("test_value"));
    
    auto countResult = backend.GetKeyCount();
    ASSERT_TRUE(countResult.HasValue());
    EXPECT_EQ(countResult.Value(), 1u);
}

TEST_F(PropertyBackendTest, GetValue_ExistingKey) {
    KvsPropertyBackend backend("test_getvalue", KvsBackendType::kvsFile);
    
    backend.SetValue("test.key", String("test_value"));
    
    auto result = backend.GetValue("test.key");
    ASSERT_TRUE(result.HasValue()) << "GetValue should return value";
    
    auto strValue = ::std::get_if<String>(&result.Value());
    ASSERT_NE(strValue, nullptr) << "Value should be String type";
    EXPECT_STREQ(strValue->data(), "test_value");
}

TEST_F(PropertyBackendTest, GetValue_NonExistentKey) {
    KvsPropertyBackend backend("test_property_basic", KvsBackendType::kvsFile);
    
    auto result = backend.GetValue("nonexistent");
    EXPECT_FALSE(result.HasValue()) << "GetValue should return error for non-existent key";
}

TEST_F(PropertyBackendTest, RemoveKey_Success) {
    KvsPropertyBackend backend("test_property_basic", KvsBackendType::kvsFile);
    
    backend.SetValue("test.key", String("test_value"));
    backend.RemoveKey("test.key");
    
    auto getValue = backend.GetValue("test.key");
    EXPECT_FALSE(getValue.HasValue()) << "Key should not exist after removal";
}

TEST_F(PropertyBackendTest, RemoveKey_NonExistentKey) {
    KvsPropertyBackend backend("test_property_basic", KvsBackendType::kvsFile);
    
    // RemoveKey on non-existent key - just call it, may or may not error
    backend.RemoveKey("nonexistent");
}

TEST_F(PropertyBackendTest, GetAllKeys_Empty) {
    KvsPropertyBackend backend("test_getallkeys_empty", KvsBackendType::kvsFile);
    
    auto result = backend.GetAllKeys();
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value().size(), 0u);
}

TEST_F(PropertyBackendTest, GetAllKeys_MultipleKeys) {
    KvsPropertyBackend backend("test_get_all_keys", KvsBackendType::kvsFile);
    
    backend.SetValue("key1", Int32(1));
    backend.SetValue("key2", Int32(2));
    backend.SetValue("key3", Int32(3));
    
    auto result = backend.GetAllKeys();
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value().size(), 3u);
}

TEST_F(PropertyBackendTest, Exists_ExistingKey) {
    KvsPropertyBackend backend("test_exists", KvsBackendType::kvsFile);
    
    backend.SetValue("test.key", String("value"));
    
    auto result = backend.KeyExists("test.key");
    ASSERT_TRUE(result.HasValue());
    EXPECT_TRUE(result.Value());
}

TEST_F(PropertyBackendTest, Exists_NonExistentKey) {
    KvsPropertyBackend backend("test_exists_nonexist", KvsBackendType::kvsFile);
    
    auto result = backend.KeyExists("nonexistent");
    ASSERT_TRUE(result.HasValue());
    EXPECT_FALSE(result.Value());
}

// ============================================================================
// Data Type Tests
// ============================================================================

TEST_F(PropertyBackendTest, DataTypes_AllSupported) {
    KvsPropertyBackend backend("test_datatypes", KvsBackendType::kvsFile);
    
    // Test all supported types
    backend.SetValue("bool", Bool(true));
    backend.SetValue("int8", Int8(-127));
    backend.SetValue("uint8", UInt8(255));
    backend.SetValue("int16", Int16(-32767));
    backend.SetValue("uint16", UInt16(65535));
    backend.SetValue("int32", Int32(-2147483647));
    backend.SetValue("uint32", UInt32(4294967295u));
    backend.SetValue("int64", Int64(-9223372036854775807LL));
    backend.SetValue("uint64", UInt64(18446744073709551615ULL));
    backend.SetValue("float", Float(3.14f));
    backend.SetValue("double", Double(2.718281828));
    backend.SetValue("string", String("test"));
    
    auto countResult = backend.GetKeyCount();
    ASSERT_TRUE(countResult.HasValue());
    EXPECT_EQ(countResult.Value(), 12u);
}

// ============================================================================
// Persistence Integration Tests
// ============================================================================

TEST_F(PropertyBackendTest, Persistence_FileBacked_SaveAndLoad) {
    // Create backend with File persistence
    {
        KvsPropertyBackend backend("test_property_persist_file", KvsBackendType::kvsFile);
        
        backend.SetValue("config.host", String("localhost"));
        backend.SetValue("config.port", UInt16(8080));
        backend.SetValue("config.enabled", Bool(true));
        
        // Sync to persistence
        backend.SyncToStorage();
    }
    
    // Recreate backend - should load from persistence
    {
        KvsPropertyBackend backend("test_property_persist_file", KvsBackendType::kvsFile);
        
        auto hostResult = backend.GetValue("config.host");
        ASSERT_TRUE(hostResult.HasValue());
        auto hostStr = ::std::get_if<String>(&hostResult.Value());
        ASSERT_NE(hostStr, nullptr);
        EXPECT_STREQ(hostStr->data(), "localhost");
        
        auto portResult = backend.GetValue("config.port");
        ASSERT_TRUE(portResult.HasValue());
        auto portVal = ::std::get_if<UInt16>(&portResult.Value());
        ASSERT_NE(portVal, nullptr);
        EXPECT_EQ(*portVal, 8080);
        
        auto enabledResult = backend.GetValue("config.enabled");
        ASSERT_TRUE(enabledResult.HasValue());
        auto enabledVal = ::std::get_if<Bool>(&enabledResult.Value());
        ASSERT_NE(enabledVal, nullptr);
        EXPECT_TRUE(*enabledVal);
    }
}

TEST_F(PropertyBackendTest, Persistence_SqliteBacked_SaveAndLoad) {
    // Create backend with SQLite persistence
    {
        KvsPropertyBackend backend("test_property_persist_sqlite", KvsBackendType::kvsSqlite);
        
        backend.SetValue("db.name", String("testdb"));
        backend.SetValue("db.connections", Int32(10));
        
        backend.SyncToStorage();
    }
    
    // Recreate backend - should load from SQLite
    {
        KvsPropertyBackend backend("test_property_persist_sqlite", KvsBackendType::kvsSqlite);
        
        auto nameResult = backend.GetValue("db.name");
        ASSERT_TRUE(nameResult.HasValue());
        auto nameStr = ::std::get_if<String>(&nameResult.Value());
        ASSERT_NE(nameStr, nullptr);
        EXPECT_STREQ(nameStr->data(), "testdb");
        
        auto connResult = backend.GetValue("db.connections");
        ASSERT_TRUE(connResult.HasValue());
        auto connVal = ::std::get_if<Int32>(&connResult.Value());
        ASSERT_NE(connVal, nullptr);
        EXPECT_EQ(*connVal, 10);
    }
}

TEST_F(PropertyBackendTest, Persistence_AutoSaveOnDestruction) {
    {
        KvsPropertyBackend backend("test_property_persist_file", KvsBackendType::kvsFile);
        backend.SetValue("auto.save.test", String("value"));
        // Backend destructor should auto-save if dirty
    }
    
    // Load in new instance
    KvsPropertyBackend backend("test_property_persist_file", KvsBackendType::kvsFile);
    auto result = backend.GetValue("auto.save.test");
    ASSERT_TRUE(result.HasValue()) << "Auto-save on destruction should persist data";
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(PropertyBackendTest, Performance_InMemoryOperationsFast) {
    KvsPropertyBackend backend("test_property_basic", KvsBackendType::kvsFile);
    
    auto start = ::std::chrono::steady_clock::now();
    
    // Write 1000 keys to memory
    for (int i = 0; i < 1000; ++i) {
        ::std::string key = "key" + ::std::to_string(i);
        backend.SetValue(key, Int32(i));
    }
    
    auto end = ::std::chrono::steady_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(end - start);
    
    // Should be very fast (< 10ms for 1000 operations)
    EXPECT_LT(duration.count(), 10) << "In-memory operations should be fast";
}

TEST_F(PropertyBackendTest, Performance_ReadFromMemoryFast) {
    KvsPropertyBackend backend("test_property_basic", KvsBackendType::kvsFile);
    
    // Prepare data
    for (int i = 0; i < 1000; ++i) {
        ::std::string key = "key" + ::std::to_string(i);
        backend.SetValue(key, Int32(i));
    }
    
    auto start = ::std::chrono::steady_clock::now();
    
    // Read 1000 keys from memory
    for (int i = 0; i < 1000; ++i) {
        ::std::string key = "key" + ::std::to_string(i);
        auto result = backend.GetValue(key);
        ASSERT_TRUE(result.HasValue());
    }
    
    auto end = ::std::chrono::steady_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(end - start);
    
    // Should be very fast (< 5ms for 1000 reads)
    EXPECT_LT(duration.count(), 5) << "Memory reads should be very fast";
}

// ============================================================================
// Edge Cases Tests
// ============================================================================

TEST_F(PropertyBackendTest, EdgeCase_EmptyStringValue) {
    KvsPropertyBackend backend("test_property_basic", KvsBackendType::kvsFile);
    
    backend.SetValue("empty", String(""));
    
    auto getValue = backend.GetValue("empty");
    ASSERT_TRUE(getValue.HasValue());
    auto strValue = ::std::get_if<String>(&getValue.Value());
    ASSERT_NE(strValue, nullptr);
    EXPECT_STREQ(strValue->data(), "");
}

TEST_F(PropertyBackendTest, EdgeCase_LongKey) {
    KvsPropertyBackend backend("test_property_basic", KvsBackendType::kvsFile);
    
    ::std::string longKey(256, 'a');
    backend.SetValue(longKey, String("value"));
    
    auto getValue = backend.GetValue(longKey);
    EXPECT_TRUE(getValue.HasValue());
}

TEST_F(PropertyBackendTest, EdgeCase_LongStringValue) {
    KvsPropertyBackend backend("test_property_basic", KvsBackendType::kvsFile);
    
    ::std::string longValue(10000, 'x');
    backend.SetValue("long.value", String(longValue.c_str()));
    
    auto getValue = backend.GetValue("long.value");
    ASSERT_TRUE(getValue.HasValue());
    auto strValue = ::std::get_if<String>(&getValue.Value());
    ASSERT_NE(strValue, nullptr);
    EXPECT_EQ(::std::strlen(strValue->data()), 10000u);
}

TEST_F(PropertyBackendTest, EdgeCase_UpdateExistingKey) {
    KvsPropertyBackend backend("test_property_basic", KvsBackendType::kvsFile);
    
    backend.SetValue("update.test", String("original"));
    backend.SetValue("update.test", String("updated"));
    
    auto getValue = backend.GetValue("update.test");
    ASSERT_TRUE(getValue.HasValue());
    auto strValue = ::std::get_if<String>(&getValue.Value());
    ASSERT_NE(strValue, nullptr);
    EXPECT_STREQ(strValue->data(), "updated");
}

TEST_F(PropertyBackendTest, EdgeCase_ClearAllKeys) {
    KvsPropertyBackend backend("test_clearall", KvsBackendType::kvsFile);
    
    // Add multiple keys
    for (int i = 0; i < 100; ++i) {
        ::std::string key = "key" + ::std::to_string(i);
        backend.SetValue(key, Int32(i));
    }
    
    auto countResult = backend.GetKeyCount();
    ASSERT_TRUE(countResult.HasValue());
    EXPECT_EQ(countResult.Value(), 100u);
    
    // Remove all keys using RemoveAllKeys
    backend.RemoveAllKeys();
    
    auto finalCount = backend.GetKeyCount();
    ASSERT_TRUE(finalCount.HasValue());
    EXPECT_EQ(finalCount.Value(), 0u);
}
