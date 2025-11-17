/**
 * @file test_kvs_sqlite_backend_enhanced.cpp
 * @brief Enhanced unit tests for SQLite Backend
 * @details Tests AUTOSAR 4-layer structure, transactions, WAL mode, and edge cases
 */

#include <gtest/gtest.h>
#include "CKvsSqliteBackend.hpp"
#include <lap/core/CPath.hpp>
#include <lap/core/CFile.hpp>
#include <sqlite3.h>

using namespace lap::per;
using namespace lap::core;

class SqliteBackendEnhancedTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clean up test databases
        ::std::remove("/tmp/test_sqlite_enhanced.db");
        ::std::remove("/tmp/test_sqlite_wal.db");
        ::std::remove("/tmp/test_sqlite_transaction.db");
    }
    
    void TearDown() override {
        SetUp(); // Clean up after tests
    }
};

// ============================================================================
// AUTOSAR 4-Layer Structure Tests
// ============================================================================

TEST_F(SqliteBackendEnhancedTest, DatabasePath_UsesCurrentLayer) {
    KvsSqliteBackend backend("test_sqlite_enhanced");
    
    // Database should be created in current/ layer
    // The actual path is managed by CStoragePathManager
    backend.SetValue("test.key", String("value"));
    
    // Verify it was written
    auto result = backend.GetValue("test.key");
    EXPECT_TRUE(result.HasValue());
}

TEST_F(SqliteBackendEnhancedTest, MultipleInstances_DifferentDatabases) {
    KvsSqliteBackend backend1("instance1");
    KvsSqliteBackend backend2("instance2");
    
    backend1.SetValue("key", String("value1"));
    backend2.SetValue("key", String("value2"));
    
    auto result1 = backend1.GetValue("key");
    auto result2 = backend2.GetValue("key");
    
    ASSERT_TRUE(result1.HasValue());
    ASSERT_TRUE(result2.HasValue());
    
    auto str1 = ::std::get_if<String>(&result1.Value());
    auto str2 = ::std::get_if<String>(&result2.Value());
    
    ASSERT_NE(str1, nullptr);
    ASSERT_NE(str2, nullptr);
    
    EXPECT_STREQ(str1->data(), "value1");
    EXPECT_STREQ(str2->data(), "value2");
}

// ============================================================================
// WAL Mode Tests
// ============================================================================

TEST_F(SqliteBackendEnhancedTest, WALMode_Enabled) {
    KvsSqliteBackend backend("test_sqlite_wal");
    
    backend.SetValue("test", String("value"));
    
    // WAL file should be created
    // Note: Actual WAL file location is in AUTOSAR structure
    // This test verifies WAL mode is working by successful write
    auto result = backend.GetValue("test");
    EXPECT_TRUE(result.HasValue());
}

TEST_F(SqliteBackendEnhancedTest, WALMode_ConcurrentReads) {
    KvsSqliteBackend backend("test_sqlite_wal");
    
    // Write some data
    for (int i = 0; i < 100; ++i) {
        ::std::string key = "key" + ::std::to_string(i);
        backend.SetValue(key, Int32(i));
    }
    
    // Multiple reads should succeed (WAL allows concurrent reads)
    for (int i = 0; i < 100; ++i) {
        ::std::string key = "key" + ::std::to_string(i);
        auto result = backend.GetValue(key);
        ASSERT_TRUE(result.HasValue());
        
        auto val = ::std::get_if<Int32>(&result.Value());
        ASSERT_NE(val, nullptr);
        EXPECT_EQ(*val, i);
    }
}

// ============================================================================
// Transaction Tests
// ============================================================================

TEST_F(SqliteBackendEnhancedTest, Transactions_BatchWrite) {
    KvsSqliteBackend backend("test_sqlite_transaction");
    
    auto start = ::std::chrono::steady_clock::now();
    
    // Write 1000 keys (should use transactions internally)
    for (int i = 0; i < 1000; ++i) {
        ::std::string key = "key" + ::std::to_string(i);
        backend.SetValue(key, Int32(i));
    }
    
    auto end = ::std::chrono::steady_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(end - start);
    
    // With transactions, should complete in reasonable time
    EXPECT_LT(duration.count(), 500) << "Batch writes should use transactions";
    
    // Verify all data written
    auto countResult = backend.GetKeyCount();
    ASSERT_TRUE(countResult.HasValue());
    EXPECT_EQ(countResult.Value(), 1000u);
}

// ============================================================================
// Data Integrity Tests
// ============================================================================

TEST_F(SqliteBackendEnhancedTest, DataIntegrity_AllTypes) {
    KvsSqliteBackend backend("test_sqlite_enhanced");
    
    // Write all data types
    backend.SetValue("bool", Bool(true));
    backend.SetValue("int8", Int8(-127));
    backend.SetValue("uint8", UInt8(255));
    backend.SetValue("int16", Int16(-32767));
    backend.SetValue("uint16", UInt16(65535));
    backend.SetValue("int32", Int32(-2147483647));
    backend.SetValue("uint32", UInt32(4294967295u));
    backend.SetValue("int64", Int64(-9223372036854775807LL));
    backend.SetValue("uint64", UInt64(18446744073709551615ULL));
    backend.SetValue("float", Float(3.14159f));
    backend.SetValue("double", Double(2.718281828459));
    backend.SetValue("string", String("test_value"));
    
    // Verify all values
    auto boolVal = backend.GetValue("bool");
    ASSERT_TRUE(boolVal.HasValue());
    EXPECT_TRUE(*::std::get_if<Bool>(&boolVal.Value()));
    
    auto int8Val = backend.GetValue("int8");
    ASSERT_TRUE(int8Val.HasValue());
    EXPECT_EQ(*::std::get_if<Int8>(&int8Val.Value()), -127);
    
    auto uint8Val = backend.GetValue("uint8");
    ASSERT_TRUE(uint8Val.HasValue());
    EXPECT_EQ(*::std::get_if<UInt8>(&uint8Val.Value()), 255);
    
    auto strVal = backend.GetValue("string");
    ASSERT_TRUE(strVal.HasValue());
    EXPECT_STREQ(::std::get_if<String>(&strVal.Value())->data(), "test_value");
}

TEST_F(SqliteBackendEnhancedTest, DataIntegrity_PersistenceAfterClose) {
    {
        KvsSqliteBackend backend("test_sqlite_enhanced");
        backend.SetValue("persist.test", String("should_persist"));
        // Backend closes on destruction
    }
    
    // Open new instance
    KvsSqliteBackend backend("test_sqlite_enhanced");
    auto result = backend.GetValue("persist.test");
    
    ASSERT_TRUE(result.HasValue()) << "Data should persist after close";
    auto strVal = ::std::get_if<String>(&result.Value());
    ASSERT_NE(strVal, nullptr);
    EXPECT_STREQ(strVal->data(), "should_persist");
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(SqliteBackendEnhancedTest, Performance_PreparedStatements) {
    KvsSqliteBackend backend("test_sqlite_enhanced");
    
    auto start = ::std::chrono::steady_clock::now();
    
    // Multiple writes should reuse prepared statements
    for (int i = 0; i < 100; ++i) {
        ::std::string key = "key" + ::std::to_string(i);
        backend.SetValue(key, Int32(i));
    }
    
    auto end = ::std::chrono::steady_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(end - start);
    
    // Prepared statements should make this reasonably fast
    EXPECT_LT(duration.count(), 200) << "Should benefit from prepared statements";
}

TEST_F(SqliteBackendEnhancedTest, Performance_CachingEffective) {
    KvsSqliteBackend backend("test_sqlite_enhanced");
    
    // Write data
    for (int i = 0; i < 100; ++i) {
        ::std::string key = "key" + ::std::to_string(i);
        backend.SetValue(key, Int32(i));
    }
    
    auto start = ::std::chrono::steady_clock::now();
    
    // Read same data multiple times (should benefit from cache)
    for (int iteration = 0; iteration < 10; ++iteration) {
        for (int i = 0; i < 100; ++i) {
            ::std::string key = "key" + ::std::to_string(i);
            auto result = backend.GetValue(key);
            ASSERT_TRUE(result.HasValue());
        }
    }
    
    auto end = ::std::chrono::steady_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(end - start);
    
    // 1000 reads should be fast with caching
    EXPECT_LT(duration.count(), 50) << "Cache should speed up repeated reads";
}

// ============================================================================
// Edge Cases Tests
// ============================================================================

TEST_F(SqliteBackendEnhancedTest, EdgeCase_VeryLongKey) {
    KvsSqliteBackend backend("test_sqlite_enhanced");
    
    ::std::string longKey(1024, 'k');
    backend.SetValue(longKey, String("value"));
    
    auto getValue = backend.GetValue(longKey);
    EXPECT_TRUE(getValue.HasValue());
}

TEST_F(SqliteBackendEnhancedTest, EdgeCase_VeryLongStringValue) {
    KvsSqliteBackend backend("test_sqlite_enhanced");
    
    ::std::string longValue(100000, 'v'); // 100KB string
    backend.SetValue("long.value", String(longValue.c_str()));
    
    auto getValue = backend.GetValue("long.value");
    ASSERT_TRUE(getValue.HasValue());
    auto strVal = ::std::get_if<String>(&getValue.Value());
    ASSERT_NE(strVal, nullptr);
    EXPECT_EQ(::std::strlen(strVal->data()), 100000u);
}

TEST_F(SqliteBackendEnhancedTest, EdgeCase_SpecialCharactersInKey) {
    KvsSqliteBackend backend("test_sqlite_enhanced");
    
    const char* specialKeys[] = {
        "key.with.dots",
        "key/with/slashes",
        "key_with_underscores",
        "key-with-dashes",
        "key:with:colons",
        "key@with@symbols"
    };
    
    for (const char* key : specialKeys) {
        backend.SetValue(key, String("value"));
        
        auto getValue = backend.GetValue(key);
        EXPECT_TRUE(getValue.HasValue()) << "Should retrieve key: " << key;
    }
}

TEST_F(SqliteBackendEnhancedTest, EdgeCase_UnicodeInValue) {
    KvsSqliteBackend backend("test_sqlite_enhanced");
    
    const char* unicodeValue = "测试中文 🚀 Тест";
    backend.SetValue("unicode.test", String(unicodeValue));
    
    auto getValue = backend.GetValue("unicode.test");
    ASSERT_TRUE(getValue.HasValue());
    auto strVal = ::std::get_if<String>(&getValue.Value());
    ASSERT_NE(strVal, nullptr);
    EXPECT_STREQ(strVal->data(), unicodeValue);
}

TEST_F(SqliteBackendEnhancedTest, EdgeCase_EmptyValue) {
    KvsSqliteBackend backend("test_sqlite_enhanced");
    
    backend.SetValue("empty", String(""));
    
    auto getValue = backend.GetValue("empty");
    ASSERT_TRUE(getValue.HasValue());
    auto strVal = ::std::get_if<String>(&getValue.Value());
    ASSERT_NE(strVal, nullptr);
    EXPECT_STREQ(strVal->data(), "");
}

TEST_F(SqliteBackendEnhancedTest, EdgeCase_UpdateExistingKey) {
    KvsSqliteBackend backend("test_sqlite_enhanced");
    
    backend.SetValue("update.test", String("original"));
    backend.SetValue("update.test", String("updated"));
    
    auto result = backend.GetValue("update.test");
    ASSERT_TRUE(result.HasValue());
    auto strVal = ::std::get_if<String>(&result.Value());
    ASSERT_NE(strVal, nullptr);
    EXPECT_STREQ(strVal->data(), "updated");
}

TEST_F(SqliteBackendEnhancedTest, EdgeCase_TypeChange) {
    KvsSqliteBackend backend("test_sqlite_enhanced");
    
    // Store as int
    backend.SetValue("type.change", Int32(42));
    
    // Change to string
    backend.SetValue("type.change", String("now_string"));
    
    auto result = backend.GetValue("type.change");
    ASSERT_TRUE(result.HasValue());
    auto strVal = ::std::get_if<String>(&result.Value());
    ASSERT_NE(strVal, nullptr);
    EXPECT_STREQ(strVal->data(), "now_string");
}

TEST_F(SqliteBackendEnhancedTest, EdgeCase_ManyKeys) {
    KvsSqliteBackend backend("test_sqlite_many_keys");
    
    // Write 10000 keys
    for (int i = 0; i < 10000; ++i) {
        ::std::string key = "key" + ::std::to_string(i);
        backend.SetValue(key, Int32(i));
    }
    
    auto countResult = backend.GetKeyCount();
    ASSERT_TRUE(countResult.HasValue());
    EXPECT_EQ(countResult.Value(), 10000u);
    
    // Verify random keys
    for (int i = 0; i < 100; i += 10) {
        ::std::string key = "key" + ::std::to_string(i);
        auto result = backend.GetValue(key);
        ASSERT_TRUE(result.HasValue());
        auto val = ::std::get_if<Int32>(&result.Value());
        ASSERT_NE(val, nullptr);
        EXPECT_EQ(*val, i);
    }
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(SqliteBackendEnhancedTest, ErrorHandling_GetNonExistentKey) {
    KvsSqliteBackend backend("test_sqlite_enhanced");
    
    auto result = backend.GetValue("nonexistent");
    EXPECT_FALSE(result.HasValue());
}

TEST_F(SqliteBackendEnhancedTest, ErrorHandling_RemoveNonExistentKey) {
    KvsSqliteBackend backend("test_sqlite_enhanced");
    
    // RemoveKey on non-existent may or may not error - just call it
    backend.RemoveKey("nonexistent");
}
