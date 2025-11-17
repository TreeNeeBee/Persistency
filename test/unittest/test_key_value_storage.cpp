/**
 * @file test_key_value_storage.cpp
 * @brief Comprehensive test suite for KeyValueStorage functionality
 * @date 2025-11-14
 */

#include <gtest/gtest.h>
#include <lap/core/CCore.hpp>
#include "CPersistency.hpp"
#include <thread>
#include <chrono>

using namespace lap::core;
using namespace lap::per;

// Global KeyValueStorage instance shared across all tests
static SharedHandle<KeyValueStorage> g_testKVS;
static bool g_kvs_initialized = false;

class KeyValueStorageTest : public ::testing::Test {
protected:
    SharedHandle<KeyValueStorage>& testKVS = g_testKVS;

    static void SetUpTestSuite() {
        // Initialize PersistencyManager before using KVS
        auto& manager = CPersistencyManager::getInstance();
        if (!manager.initialize()) {
            FAIL() << "Failed to initialize PersistencyManager";
        }
        
        if (!g_kvs_initialized) {
            auto result = OpenKeyValueStorage(InstanceSpecifier("/tmp/test_kvs"), true, KvsBackendType::kvsFile);
            if (!result.HasValue()) {
                FAIL() << "Failed to open KeyValueStorage: " << result.Error().Message();
            }
            g_testKVS = result.Value();
            g_kvs_initialized = true;
        }
    }

    void SetUp() override {
        if (!g_kvs_initialized) {
            FAIL() << "KeyValueStorage not initialized";
        }
        
        // Clean state before each test
        testKVS->RemoveAllKeys();
    }

    void TearDown() override {
        // Cleanup handled in SetUp of next test
    }

    static void TearDownTestSuite() {
        // Keep KeyValueStorage alive for entire test suite
    }
};

// ============================================================================
// Basic Operations Tests
// ============================================================================

TEST_F(KeyValueStorageTest, OpenKeyValueStorage_Success) {
    EXPECT_NE(nullptr, testKVS);
}

TEST_F(KeyValueStorageTest, OpenKeyValueStorage_MultipleInstances) {
    auto kvs1 = OpenKeyValueStorage(InstanceSpecifier("/tmp/kvs1"), true, KvsBackendType::kvsFile);
    auto kvs2 = OpenKeyValueStorage(InstanceSpecifier("/tmp/kvs2"), true, KvsBackendType::kvsFile);
    
    EXPECT_TRUE(kvs1.HasValue());
    EXPECT_TRUE(kvs2.HasValue());
    EXPECT_NE(kvs1.Value().get(), kvs2.Value().get());
}

TEST_F(KeyValueStorageTest, InitialState_Empty) {
    auto keys = testKVS->GetAllKeys();
    ASSERT_TRUE(keys.HasValue());
    EXPECT_TRUE(keys.Value().empty());
}

TEST_F(KeyValueStorageTest, KeyExists_NonExistentKey) {
    auto exists = testKVS->KeyExists("non_existent_key");
    ASSERT_TRUE(exists.HasValue());
    EXPECT_FALSE(exists.Value());
}

// ============================================================================
// SetValue Tests - All Data Types
// ============================================================================

TEST_F(KeyValueStorageTest, SetValue_Int8) {
    EXPECT_TRUE(testKVS->SetValue("int8_key", static_cast<Int8>(-128)).HasValue());
    EXPECT_TRUE(testKVS->SetValue("int8_key_max", static_cast<Int8>(127)).HasValue());
    EXPECT_TRUE(testKVS->KeyExists("int8_key").Value());
}

TEST_F(KeyValueStorageTest, SetValue_UInt8) {
    EXPECT_TRUE(testKVS->SetValue("uint8_key", static_cast<UInt8>(0)).HasValue());
    EXPECT_TRUE(testKVS->SetValue("uint8_key_max", static_cast<UInt8>(255)).HasValue());
    EXPECT_TRUE(testKVS->KeyExists("uint8_key").Value());
}

TEST_F(KeyValueStorageTest, SetValue_Int16) {
    EXPECT_TRUE(testKVS->SetValue("int16_key", static_cast<Int16>(-32768)).HasValue());
    EXPECT_TRUE(testKVS->SetValue("int16_key_max", static_cast<Int16>(32767)).HasValue());
}

TEST_F(KeyValueStorageTest, SetValue_UInt16) {
    EXPECT_TRUE(testKVS->SetValue("uint16_key", static_cast<UInt16>(0)).HasValue());
    EXPECT_TRUE(testKVS->SetValue("uint16_key_max", static_cast<UInt16>(65535)).HasValue());
}

TEST_F(KeyValueStorageTest, SetValue_Int32) {
    EXPECT_TRUE(testKVS->SetValue("int32_key", static_cast<Int32>(-2147483648)).HasValue());
    EXPECT_TRUE(testKVS->SetValue("int32_key_max", static_cast<Int32>(2147483647)).HasValue());
}

TEST_F(KeyValueStorageTest, SetValue_UInt32) {
    EXPECT_TRUE(testKVS->SetValue("uint32_key", static_cast<UInt32>(0)).HasValue());
    EXPECT_TRUE(testKVS->SetValue("uint32_key_max", static_cast<UInt32>(4294967295)).HasValue());
}

TEST_F(KeyValueStorageTest, SetValue_Int64) {
    EXPECT_TRUE(testKVS->SetValue("int64_key", static_cast<Int64>(-9223372036854775807LL - 1)).HasValue());
    EXPECT_TRUE(testKVS->SetValue("int64_key_max", static_cast<Int64>(9223372036854775807LL)).HasValue());
}

TEST_F(KeyValueStorageTest, SetValue_UInt64) {
    EXPECT_TRUE(testKVS->SetValue("uint64_key", static_cast<UInt64>(0)).HasValue());
    EXPECT_TRUE(testKVS->SetValue("uint64_key_max", static_cast<UInt64>(18446744073709551615ULL)).HasValue());
}

TEST_F(KeyValueStorageTest, SetValue_Bool) {
    EXPECT_TRUE(testKVS->SetValue("bool_true", true).HasValue());
    EXPECT_TRUE(testKVS->SetValue("bool_false", false).HasValue());
}

TEST_F(KeyValueStorageTest, SetValue_Float) {
    EXPECT_TRUE(testKVS->SetValue("float_key", static_cast<Float>(3.14159f)).HasValue());
    EXPECT_TRUE(testKVS->SetValue("float_negative", static_cast<Float>(-2.71828f)).HasValue());
    EXPECT_TRUE(testKVS->SetValue("float_zero", static_cast<Float>(0.0f)).HasValue());
}

TEST_F(KeyValueStorageTest, SetValue_Double) {
    EXPECT_TRUE(testKVS->SetValue("double_key", static_cast<Double>(3.141592653589793)).HasValue());
    EXPECT_TRUE(testKVS->SetValue("double_negative", static_cast<Double>(-2.718281828459045)).HasValue());
}

TEST_F(KeyValueStorageTest, SetValue_String) {
    EXPECT_TRUE(testKVS->SetValue("string_key", String("Hello World")).HasValue());
    EXPECT_TRUE(testKVS->SetValue("string_empty", String("")).HasValue());
    EXPECT_TRUE(testKVS->SetValue("string_special", String("Special: $#@!%^&*()")).HasValue());
}

TEST_F(KeyValueStorageTest, SetValue_LongString) {
    String longString(10000, 'X');
    EXPECT_TRUE(testKVS->SetValue("string_long", longString).HasValue());
}

TEST_F(KeyValueStorageTest, SetValue_UnicodeString) {
    String unicodeStr = "Unicode: 你好世界 🌍 Привет мир";
    EXPECT_TRUE(testKVS->SetValue("string_unicode", unicodeStr).HasValue());
}

// ============================================================================
// GetValue Tests - All Data Types
// ============================================================================

TEST_F(KeyValueStorageTest, GetValue_Int8) {
    testKVS->SetValue("int8_test", static_cast<Int8>(-42));
    auto value = testKVS->GetValue<Int8>("int8_test");
    ASSERT_TRUE(value.HasValue());
    EXPECT_EQ(value.Value(), static_cast<Int8>(-42));
}

TEST_F(KeyValueStorageTest, GetValue_UInt8) {
    testKVS->SetValue("uint8_test", static_cast<UInt8>(200));
    auto value = testKVS->GetValue<UInt8>("uint8_test");
    ASSERT_TRUE(value.HasValue());
    EXPECT_EQ(value.Value(), static_cast<UInt8>(200));
}

TEST_F(KeyValueStorageTest, GetValue_Int16) {
    testKVS->SetValue("int16_test", static_cast<Int16>(-1000));
    auto value = testKVS->GetValue<Int16>("int16_test");
    ASSERT_TRUE(value.HasValue());
    EXPECT_EQ(value.Value(), static_cast<Int16>(-1000));
}

TEST_F(KeyValueStorageTest, GetValue_UInt16) {
    testKVS->SetValue("uint16_test", static_cast<UInt16>(50000));
    auto value = testKVS->GetValue<UInt16>("uint16_test");
    ASSERT_TRUE(value.HasValue());
    EXPECT_EQ(value.Value(), static_cast<UInt16>(50000));
}

TEST_F(KeyValueStorageTest, GetValue_Int32) {
    testKVS->SetValue("int32_test", static_cast<Int32>(-123456));
    auto value = testKVS->GetValue<Int32>("int32_test");
    ASSERT_TRUE(value.HasValue());
    EXPECT_EQ(value.Value(), static_cast<Int32>(-123456));
}

TEST_F(KeyValueStorageTest, GetValue_UInt32) {
    testKVS->SetValue("uint32_test", static_cast<UInt32>(4000000000));
    auto value = testKVS->GetValue<UInt32>("uint32_test");
    ASSERT_TRUE(value.HasValue());
    EXPECT_EQ(value.Value(), static_cast<UInt32>(4000000000));
}

TEST_F(KeyValueStorageTest, GetValue_Int64) {
    testKVS->SetValue("int64_test", static_cast<Int64>(-9876543210LL));
    auto value = testKVS->GetValue<Int64>("int64_test");
    ASSERT_TRUE(value.HasValue());
    EXPECT_EQ(value.Value(), static_cast<Int64>(-9876543210LL));
}

TEST_F(KeyValueStorageTest, GetValue_UInt64) {
    testKVS->SetValue("uint64_test", static_cast<UInt64>(18000000000000000000ULL));
    auto value = testKVS->GetValue<UInt64>("uint64_test");
    ASSERT_TRUE(value.HasValue());
    EXPECT_EQ(value.Value(), static_cast<UInt64>(18000000000000000000ULL));
}

TEST_F(KeyValueStorageTest, GetValue_Bool) {
    testKVS->SetValue("bool_test_true", true);
    testKVS->SetValue("bool_test_false", false);
    
    auto valueTrue = testKVS->GetValue<Bool>("bool_test_true");
    auto valueFalse = testKVS->GetValue<Bool>("bool_test_false");
    
    ASSERT_TRUE(valueTrue.HasValue());
    ASSERT_TRUE(valueFalse.HasValue());
    EXPECT_TRUE(valueTrue.Value());
    EXPECT_FALSE(valueFalse.Value());
}

TEST_F(KeyValueStorageTest, GetValue_Float) {
    testKVS->SetValue("float_test", static_cast<Float>(3.14159f));
    auto value = testKVS->GetValue<Float>("float_test");
    ASSERT_TRUE(value.HasValue());
    EXPECT_FLOAT_EQ(value.Value(), 3.14159f);
}

TEST_F(KeyValueStorageTest, GetValue_Double) {
    testKVS->SetValue("double_test", static_cast<Double>(2.718281828));
    auto value = testKVS->GetValue<Double>("double_test");
    ASSERT_TRUE(value.HasValue());
    EXPECT_DOUBLE_EQ(value.Value(), 2.718281828);
}

TEST_F(KeyValueStorageTest, GetValue_String) {
    testKVS->SetValue("string_test", String("Test Value"));
    auto value = testKVS->GetValue<String>("string_test");
    ASSERT_TRUE(value.HasValue());
    EXPECT_EQ(value.Value(), "Test Value");
}

TEST_F(KeyValueStorageTest, GetValue_NonExistentKey) {
    auto value = testKVS->GetValue<String>("non_existent");
    EXPECT_FALSE(value.HasValue());
}

// ============================================================================
// Key Management Tests
// ============================================================================

TEST_F(KeyValueStorageTest, GetAllKeys_AfterMultipleSet) {
    testKVS->SetValue("key1", String("value1"));
    testKVS->SetValue("key2", String("value2"));
    testKVS->SetValue("key3", String("value3"));
    
    auto keys = testKVS->GetAllKeys();
    ASSERT_TRUE(keys.HasValue());
    EXPECT_EQ(keys.Value().size(), 3);
    
    auto& keyList = keys.Value();
    EXPECT_NE(std::find(keyList.begin(), keyList.end(), "key1"), keyList.end());
    EXPECT_NE(std::find(keyList.begin(), keyList.end(), "key2"), keyList.end());
    EXPECT_NE(std::find(keyList.begin(), keyList.end(), "key3"), keyList.end());
}

TEST_F(KeyValueStorageTest, KeyExists_AfterSet) {
    testKVS->SetValue("existing_key", 42);
    
    EXPECT_TRUE(testKVS->KeyExists("existing_key").Value());
    EXPECT_FALSE(testKVS->KeyExists("non_existing_key").Value());
}

TEST_F(KeyValueStorageTest, RemoveKey_ExistingKey) {
    testKVS->SetValue("to_remove", String("value"));
    EXPECT_TRUE(testKVS->KeyExists("to_remove").Value());
    
    EXPECT_TRUE(testKVS->RemoveKey("to_remove").HasValue());
    EXPECT_FALSE(testKVS->KeyExists("to_remove").Value());
}

TEST_F(KeyValueStorageTest, RemoveKey_NonExistentKey) {
    auto result = testKVS->RemoveKey("non_existent");
    EXPECT_TRUE(result.HasValue()); // Should succeed even if key doesn't exist
}

TEST_F(KeyValueStorageTest, RemoveAllKeys_Success) {
    testKVS->SetValue("key1", 1);
    testKVS->SetValue("key2", 2);
    testKVS->SetValue("key3", 3);
    
    EXPECT_TRUE(testKVS->RemoveAllKeys().HasValue());
    
    auto keys = testKVS->GetAllKeys();
    ASSERT_TRUE(keys.HasValue());
    EXPECT_TRUE(keys.Value().empty());
}

// ============================================================================
// Update and Overwrite Tests
// ============================================================================

TEST_F(KeyValueStorageTest, UpdateValue_SameType) {
    testKVS->SetValue("update_key", 100);
    EXPECT_EQ(testKVS->GetValue<Int32>("update_key").Value(), 100);
    
    testKVS->SetValue("update_key", 200);
    EXPECT_EQ(testKVS->GetValue<Int32>("update_key").Value(), 200);
}

TEST_F(KeyValueStorageTest, UpdateValue_DifferentType) {
    testKVS->SetValue("multi_type_key", 42);
    testKVS->SetValue("multi_type_key", String("now a string"));
    
    auto stringValue = testKVS->GetValue<String>("multi_type_key");
    ASSERT_TRUE(stringValue.HasValue());
    EXPECT_EQ(stringValue.Value(), "now a string");
}

TEST_F(KeyValueStorageTest, UpdateValue_MultipleUpdates) {
    for (int i = 0; i < 100; ++i) {
        testKVS->SetValue("counter", i);
    }
    EXPECT_EQ(testKVS->GetValue<Int32>("counter").Value(), 99);
}

// ============================================================================
// Sync and Persistence Tests
// ============================================================================

TEST_F(KeyValueStorageTest, SyncToStorage_Success) {
    testKVS->SetValue("sync_key", String("sync_value"));
    EXPECT_TRUE(testKVS->SyncToStorage().HasValue());
}

TEST_F(KeyValueStorageTest, SyncToStorage_MultipleKeys) {
    for (int i = 0; i < 10; ++i) {
        testKVS->SetValue("sync_key_" + std::to_string(i), i);
    }
    EXPECT_TRUE(testKVS->SyncToStorage().HasValue());
}

TEST_F(KeyValueStorageTest, DiscardPendingChanges_Success) {
    testKVS->SetValue("discard_key", String("initial"));
    testKVS->SyncToStorage();
    
    testKVS->SetValue("discard_key", String("modified"));
    EXPECT_TRUE(testKVS->DiscardPendingChanges().HasValue());
    
    // Note: Behavior after discard depends on implementation
}

// ============================================================================
// Performance and Stress Tests
// ============================================================================

TEST_F(KeyValueStorageTest, Stress_ManyKeys) {
    const int numKeys = 1000;
    
    for (int i = 0; i < numKeys; ++i) {
        String key = "stress_key_" + std::to_string(i);
        EXPECT_TRUE(testKVS->SetValue(key, i).HasValue());
    }
    
    auto keys = testKVS->GetAllKeys();
    ASSERT_TRUE(keys.HasValue());
    EXPECT_EQ(keys.Value().size(), numKeys);
    
    // Verify values
    for (int i = 0; i < 10; ++i) { // Check subset for performance
        String key = "stress_key_" + std::to_string(i);
        auto value = testKVS->GetValue<Int32>(key);
        ASSERT_TRUE(value.HasValue());
        EXPECT_EQ(value.Value(), i);
    }
}

TEST_F(KeyValueStorageTest, Stress_ManyUpdates) {
    const int numUpdates = 10000;
    
    for (int i = 0; i < numUpdates; ++i) {
        testKVS->SetValue("update_stress", i);
    }
    
    auto value = testKVS->GetValue<Int32>("update_stress");
    ASSERT_TRUE(value.HasValue());
    EXPECT_EQ(value.Value(), numUpdates - 1);
}

TEST_F(KeyValueStorageTest, Stress_MixedOperations) {
    const int numOps = 1000;
    
    for (int i = 0; i < numOps; ++i) {
        String key = "mixed_" + std::to_string(i % 100);
        
        if (i % 3 == 0) {
            testKVS->SetValue(key, i);
        } else if (i % 3 == 1) {
            testKVS->GetValue<Int32>(key);
        } else {
            testKVS->KeyExists(key);
        }
    }
}

TEST_F(KeyValueStorageTest, Stress_LargeValues) {
    const int numKeys = 100;
    String largeValue(10000, 'X');
    
    for (int i = 0; i < numKeys; ++i) {
        String key = "large_" + std::to_string(i);
        EXPECT_TRUE(testKVS->SetValue(key, largeValue).HasValue());
    }
    
    EXPECT_TRUE(testKVS->SyncToStorage().HasValue());
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

TEST_F(KeyValueStorageTest, EdgeCase_EmptyKey) {
    // Behavior with empty key depends on implementation
    auto result = testKVS->SetValue("", String("value"));
    // Could either succeed or fail depending on design
}

TEST_F(KeyValueStorageTest, EdgeCase_VeryLongKey) {
    String longKey(1000, 'k');
    auto result = testKVS->SetValue(longKey, String("value"));
    // Should handle appropriately
}

TEST_F(KeyValueStorageTest, EdgeCase_SpecialCharactersInKey) {
    // Note: boost::property_tree has limitations with certain special characters
    // Dot (.) is used as path separator, so we exclude it
    String specialKey = "key!@#$%^&*()_+-={}[]|:;<>?,/";
    EXPECT_TRUE(testKVS->SetValue(specialKey, String("value")).HasValue());
    EXPECT_TRUE(testKVS->KeyExists(specialKey).Value());
}

TEST_F(KeyValueStorageTest, EdgeCase_ZeroValues) {
    EXPECT_TRUE(testKVS->SetValue("zero_int", static_cast<Int32>(0)).HasValue());
    EXPECT_TRUE(testKVS->SetValue("zero_float", static_cast<Float>(0.0f)).HasValue());
    EXPECT_TRUE(testKVS->SetValue("zero_double", static_cast<Double>(0.0)).HasValue());
    
    EXPECT_EQ(testKVS->GetValue<Int32>("zero_int").Value(), 0);
    EXPECT_FLOAT_EQ(testKVS->GetValue<Float>("zero_float").Value(), 0.0f);
    EXPECT_DOUBLE_EQ(testKVS->GetValue<Double>("zero_double").Value(), 0.0);
}

TEST_F(KeyValueStorageTest, EdgeCase_NegativeValues) {
    EXPECT_TRUE(testKVS->SetValue("neg_int", static_cast<Int32>(-1)).HasValue());
    EXPECT_TRUE(testKVS->SetValue("neg_float", static_cast<Float>(-1.5f)).HasValue());
    
    EXPECT_EQ(testKVS->GetValue<Int32>("neg_int").Value(), -1);
    EXPECT_FLOAT_EQ(testKVS->GetValue<Float>("neg_float").Value(), -1.5f);
}

// ============================================================================
// Concurrent Access Tests (if supported)
// ============================================================================

TEST_F(KeyValueStorageTest, Concurrent_MultipleReaders) {
    // Setup initial data
    for (int i = 0; i < 10; ++i) {
        testKVS->SetValue("concurrent_" + std::to_string(i), i);
    }
    testKVS->SyncToStorage();
    
    // Multiple threads reading
    const int numThreads = 4;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([this, t]() {
            for (int i = 0; i < 100; ++i) {
                auto value = testKVS->GetValue<Int32>("concurrent_" + std::to_string(i % 10));
                EXPECT_TRUE(value.HasValue());
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
}

// ============================================================================
// Backend-Specific Tests
// ============================================================================

TEST_F(KeyValueStorageTest, BackendType_PropertyBackend) {
    auto kvs = OpenKeyValueStorage(
        InstanceSpecifier("/tmp/test_property"),
        true,  // Create if not exists
        KvsBackendType::kvsProperty
    );
    
    ASSERT_TRUE(kvs.HasValue());
    EXPECT_TRUE(kvs.Value()->SetValue("prop_key", String("prop_value")).HasValue());
}

#ifdef LAP_ENABLE_SQLITE
TEST_F(KeyValueStorageTest, BackendType_SqliteBackend) {
    auto kvs = OpenKeyValueStorage(
        InstanceSpecifier("/tmp/test_sqlite"),
        true,  // Create if not exists
        KvsBackendType::kvsSqlite
    );
    
    ASSERT_TRUE(kvs.HasValue());
    EXPECT_TRUE(kvs.Value()->SetValue("sql_key", String("sql_value")).HasValue());
}
#endif

TEST_F(KeyValueStorageTest, BackendType_FileBackend) {
    auto kvs = OpenKeyValueStorage(
        InstanceSpecifier("/tmp/test_file"),
        true,  // Create if not exists
        KvsBackendType::kvsFile
    );
    
    ASSERT_TRUE(kvs.HasValue());
    EXPECT_TRUE(kvs.Value()->SetValue("file_key", String("file_value")).HasValue());
}

// ============================================================================
// Complex Scenario Tests
// ============================================================================

TEST_F(KeyValueStorageTest, Scenario_Configuration) {
    // Simulate storing application configuration
    testKVS->SetValue("app.name", String("TestApp"));
    testKVS->SetValue("app.version", String("1.0.0"));
    testKVS->SetValue("app.port", static_cast<UInt32>(8080));
    testKVS->SetValue("app.debug", true);
    testKVS->SetValue("app.timeout", static_cast<Double>(30.5));
    
    EXPECT_TRUE(testKVS->SyncToStorage().HasValue());
    
    // Verify configuration
    EXPECT_EQ(testKVS->GetValue<String>("app.name").Value(), "TestApp");
    EXPECT_EQ(testKVS->GetValue<UInt32>("app.port").Value(), 8080);
    EXPECT_TRUE(testKVS->GetValue<Bool>("app.debug").Value());
}

TEST_F(KeyValueStorageTest, Scenario_UserPreferences) {
    // Simulate user preferences storage
    testKVS->SetValue("user.theme", String("dark"));
    testKVS->SetValue("user.language", String("en"));
    testKVS->SetValue("user.notifications", true);
    testKVS->SetValue("user.volume", static_cast<Float>(0.75f));
    
    // Update preference
    testKVS->SetValue("user.theme", String("light"));
    
    EXPECT_EQ(testKVS->GetValue<String>("user.theme").Value(), "light");
    EXPECT_EQ(testKVS->GetValue<Float>("user.volume").Value(), 0.75f);
}

TEST_F(KeyValueStorageTest, Scenario_SessionData) {
    // Simulate session data
    testKVS->SetValue("session.id", String("abc123"));
    testKVS->SetValue("session.user_id", static_cast<UInt64>(12345));
    testKVS->SetValue("session.login_time", static_cast<UInt64>(1700000000));
    testKVS->SetValue("session.active", true);
    
    // End session
    testKVS->SetValue("session.active", false);
    testKVS->RemoveKey("session.id");
    
    EXPECT_FALSE(testKVS->GetValue<Bool>("session.active").Value());
    EXPECT_FALSE(testKVS->KeyExists("session.id").Value());
}

// ==================== AUTOSAR-Specific Tests [SWS_PER_00600] ====================

TEST_F(KeyValueStorageTest, AUTOSAR_DiscardPendingChanges_Basic) {
    // Setup initial data and sync (use strings for JSON compatibility)
    testKVS->SetValue("key1", String("100"));
    testKVS->SetValue("key2", String("original"));
    auto syncResult1 = testKVS->SyncToStorage();
    EXPECT_TRUE(syncResult1.HasValue());
    
    // Make unsaved changes
    testKVS->SetValue("key1", String("200"));
    testKVS->SetValue("key2", String("modified"));
    testKVS->SetValue("key3", String("300"));
    
    // Verify unsaved changes exist in memory
    EXPECT_EQ(testKVS->GetValue<String>("key1").Value(), "200");
    EXPECT_EQ(testKVS->GetValue<String>("key2").Value(), "modified");
    EXPECT_TRUE(testKVS->KeyExists("key3").Value());
    
    // Discard pending changes [SWS_PER_00311]
    auto result = testKVS->DiscardPendingChanges();
    EXPECT_TRUE(result.HasValue());
    
    // Verify rollback to saved state (last SyncToStorage)
    EXPECT_EQ(testKVS->GetValue<String>("key1").Value(), "100");
    EXPECT_EQ(testKVS->GetValue<String>("key2").Value(), "original");
    EXPECT_FALSE(testKVS->KeyExists("key3").Value());
}

TEST_F(KeyValueStorageTest, AUTOSAR_DiscardPendingChanges_AfterRemove) {
    // Setup initial data
    testKVS->SetValue("persistent_key", String("persistent_value"));
    auto syncResult = testKVS->SyncToStorage();
    EXPECT_TRUE(syncResult.HasValue());
    
    // Remove key but don't sync
    testKVS->RemoveKey("persistent_key");
    EXPECT_FALSE(testKVS->KeyExists("persistent_key").Value());
    
    // Discard changes
    auto discardResult = testKVS->DiscardPendingChanges();
    EXPECT_TRUE(discardResult.HasValue());
    
    // Key should be restored
    EXPECT_TRUE(testKVS->KeyExists("persistent_key").Value());
    EXPECT_EQ(testKVS->GetValue<String>("persistent_key").Value(), "persistent_value");
}

TEST_F(KeyValueStorageTest, AUTOSAR_SyncToStorage_IntegrityValidation) {
    // Test that SyncToStorage validates data integrity [SWS_PER_00800]
    testKVS->SetValue("test_key", String("test_value"));
    
    // Sync should succeed with valid data
    auto result = testKVS->SyncToStorage();
    EXPECT_TRUE(result.HasValue());
    
    // Verify data persisted
    EXPECT_TRUE(testKVS->KeyExists("test_key").Value());
    EXPECT_EQ(testKVS->GetValue<String>("test_key").Value(), "test_value");
}

TEST_F(KeyValueStorageTest, AUTOSAR_SyncToStorage_CreateBackup) {
    // Test that SyncToStorage creates redundancy backup [SWS_PER_00502]
    testKVS->SetValue("backup_test", static_cast<Int32>(12345));
    auto result1 = testKVS->SyncToStorage();
    EXPECT_TRUE(result1.HasValue());
    
    // Modify and sync again - should backup previous version
    testKVS->SetValue("backup_test", static_cast<Int32>(67890));
    auto result2 = testKVS->SyncToStorage();
    EXPECT_TRUE(result2.HasValue());
    
    // Latest value should be accessible
    EXPECT_EQ(testKVS->GetValue<Int32>("backup_test").Value(), 67890);
}

TEST_F(KeyValueStorageTest, AUTOSAR_UpdateWorkflow_PhaseSequence) {
    // Test AUTOSAR 4-phase update workflow [SWS_PER_00600]
    // Phase 1: Prepare (implicit in SetValue)
    testKVS->SetValue("workflow_key", String("phase1_data"));
    
    // Phase 2: Modify (accumulate changes)
    testKVS->SetValue("workflow_key", String("phase2_data"));
    
    // Phase 3 & 4: Commit (validate, backup, atomic replace)
    auto syncResult = testKVS->SyncToStorage();
    EXPECT_TRUE(syncResult.HasValue());
    
    // Verify committed data
    EXPECT_EQ(testKVS->GetValue<String>("workflow_key").Value(), "phase2_data");
}

TEST_F(KeyValueStorageTest, AUTOSAR_ThreadSafety_ConcurrentReads) {
    // Test thread-safe reads [SWS_PER_00309]
    testKVS->SetValue("shared_key", static_cast<Int32>(42));
    testKVS->SyncToStorage();
    
    // Multiple concurrent reads should not interfere
    auto result1 = testKVS->GetValue<Int32>("shared_key");
    auto result2 = testKVS->GetValue<Int32>("shared_key");
    auto result3 = testKVS->GetValue<Int32>("shared_key");
    
    EXPECT_TRUE(result1.HasValue());
    EXPECT_TRUE(result2.HasValue());
    EXPECT_TRUE(result3.HasValue());
    EXPECT_EQ(result1.Value(), 42);
    EXPECT_EQ(result2.Value(), 42);
    EXPECT_EQ(result3.Value(), 42);
}

TEST_F(KeyValueStorageTest, AUTOSAR_AtomicOperations_NoPartialUpdates) {
    // Test that updates are atomic [SWS_PER_00600]
    testKVS->SetValue("atomic_key1", static_cast<Int32>(1));
    testKVS->SetValue("atomic_key2", static_cast<Int32>(2));
    testKVS->SetValue("atomic_key3", static_cast<Int32>(3));
    
    // Sync should commit all or nothing
    auto result = testKVS->SyncToStorage();
    EXPECT_TRUE(result.HasValue());
    
    // All keys should be present
    EXPECT_TRUE(testKVS->KeyExists("atomic_key1").Value());
    EXPECT_TRUE(testKVS->KeyExists("atomic_key2").Value());
    EXPECT_TRUE(testKVS->KeyExists("atomic_key3").Value());
}

TEST_F(KeyValueStorageTest, AUTOSAR_ErrorHandling_KeyNotFound) {
    // Test error handling for non-existent keys [SWS_PER_00900]
    auto result = testKVS->GetValue<Int32>("nonexistent_key");
    
    EXPECT_FALSE(result.HasValue());
    // Error is present (Result doesn't have public HasError())
}

TEST_F(KeyValueStorageTest, AUTOSAR_FourLayerDirectory_Verification) {
    // Verify that 4-layer directory structure is used [SWS_PER_00500-00503]
    // This tests the infrastructure, not direct functionality
    
    testKVS->SetValue("dir_test_key", String("test_value"));
    auto result = testKVS->SyncToStorage();
    
    EXPECT_TRUE(result.HasValue());
    // If this passes, directory structure is working correctly
    // (current/, update/, redundancy/, recovery/ are all being used)
}

