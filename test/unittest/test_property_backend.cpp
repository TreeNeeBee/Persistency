/**
 * @file test_property_backend.cpp
 * @brief Comprehensive test suite for KvsPropertyBackend with shared memory
 * 
 * Tests include:
 * - Basic CRUD operations
 * - Boundary value testing
 * - Stress testing
 * - Concurrent access
 * - Memory management
 */

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <chrono>
#include <random>
#include <algorithm>
#include <iostream>
#include <atomic>

#include "CKvsPropertyBackend.hpp"
#include "CDataType.hpp"

using namespace lap::pm;
using namespace lap::pm::util;
using namespace lap::core;

class PropertyBackendTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create backend instance for testing
        testFile = "test_property_storage";
        backend = std::make_unique<KvsPropertyBackend>(testFile);
        
        // Clean up any existing data
        if (backend->available()) {
            backend->RemoveAllKeys();
        }
    }

    void TearDown() override {
        if (backend && backend->available()) {
            backend->RemoveAllKeys();
        }
        backend.reset();
    }

    String testFile;
    std::unique_ptr<KvsPropertyBackend> backend;
};

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST_F(PropertyBackendTest, Constructor_ShouldInitializeSuccessfully) {
    ASSERT_TRUE(backend != nullptr);
    EXPECT_TRUE(backend->available());
}

TEST_F(PropertyBackendTest, SetValue_Int8_ShouldSucceed) {
    Int8 value = 42;
    auto result = backend->SetValue("test_int8", KvsDataType(value));
    EXPECT_TRUE(result.HasValue());
}

TEST_F(PropertyBackendTest, GetValue_Int8_ShouldReturnCorrectValue) {
    Int8 expectedValue = 42;
    backend->SetValue("test_int8", KvsDataType(expectedValue));
    
    auto result = backend->GetValue("test_int8");
    ASSERT_TRUE(result.HasValue());
    
    Int8 actualValue = lap::core::get<Int8>(result.Value());
    EXPECT_EQ(expectedValue, actualValue);
}

TEST_F(PropertyBackendTest, SetValue_UInt8_ShouldSucceed) {
    UInt8 value = 255;
    auto result = backend->SetValue("test_uint8", KvsDataType(value));
    EXPECT_TRUE(result.HasValue());
}

TEST_F(PropertyBackendTest, GetValue_UInt8_ShouldReturnCorrectValue) {
    UInt8 expectedValue = 255;
    backend->SetValue("test_uint8", KvsDataType(expectedValue));
    
    auto result = backend->GetValue("test_uint8");
    ASSERT_TRUE(result.HasValue());
    
    UInt8 actualValue = lap::core::get<UInt8>(result.Value());
    EXPECT_EQ(expectedValue, actualValue);
}

TEST_F(PropertyBackendTest, SetValue_Int32_ShouldSucceed) {
    Int32 value = 123456;
    auto result = backend->SetValue("test_int32", KvsDataType(value));
    EXPECT_TRUE(result.HasValue());
}

TEST_F(PropertyBackendTest, GetValue_Int32_ShouldReturnCorrectValue) {
    Int32 expectedValue = 123456;
    backend->SetValue("test_int32", KvsDataType(expectedValue));
    
    auto result = backend->GetValue("test_int32");
    ASSERT_TRUE(result.HasValue());
    
    Int32 actualValue = lap::core::get<Int32>(result.Value());
    EXPECT_EQ(expectedValue, actualValue);
}

TEST_F(PropertyBackendTest, SetValue_Float_ShouldSucceed) {
    Float value = 3.14159f;
    auto result = backend->SetValue("test_float", KvsDataType(value));
    EXPECT_TRUE(result.HasValue());
}

TEST_F(PropertyBackendTest, GetValue_Float_ShouldReturnCorrectValue) {
    Float expectedValue = 3.14159f;
    backend->SetValue("test_float", KvsDataType(expectedValue));
    
    auto result = backend->GetValue("test_float");
    ASSERT_TRUE(result.HasValue());
    
    Float actualValue = lap::core::get<Float>(result.Value());
    EXPECT_FLOAT_EQ(expectedValue, actualValue);
}

TEST_F(PropertyBackendTest, SetValue_Double_ShouldSucceed) {
    Double value = 3.141592653589793;
    auto result = backend->SetValue("test_double", KvsDataType(value));
    EXPECT_TRUE(result.HasValue());
}

TEST_F(PropertyBackendTest, GetValue_Double_ShouldReturnCorrectValue) {
    Double expectedValue = 3.141592653589793;
    backend->SetValue("test_double", KvsDataType(expectedValue));
    
    auto result = backend->GetValue("test_double");
    ASSERT_TRUE(result.HasValue());
    
    Double actualValue = lap::core::get<Double>(result.Value());
    // After precision fix, we can use stricter tolerance
    EXPECT_NEAR(expectedValue, actualValue, 1e-14);
}

TEST_F(PropertyBackendTest, SetValue_Bool_ShouldSucceed) {
    Bool value = true;
    auto result = backend->SetValue("test_bool", KvsDataType(value));
    EXPECT_TRUE(result.HasValue());
}

TEST_F(PropertyBackendTest, GetValue_Bool_ShouldReturnCorrectValue) {
    Bool expectedValue = true;
    backend->SetValue("test_bool", KvsDataType(expectedValue));
    
    auto result = backend->GetValue("test_bool");
    ASSERT_TRUE(result.HasValue());
    
    Bool actualValue = lap::core::get<Bool>(result.Value());
    EXPECT_EQ(expectedValue, actualValue);
}

TEST_F(PropertyBackendTest, SetValue_String_ShouldSucceed) {
    String value = "Hello, World!";
    auto result = backend->SetValue("test_string", KvsDataType(value));
    EXPECT_TRUE(result.HasValue());
}

TEST_F(PropertyBackendTest, GetValue_String_ShouldReturnCorrectValue) {
    String expectedValue = "Hello, World!";
    backend->SetValue("test_string", KvsDataType(expectedValue));
    
    auto result = backend->GetValue("test_string");
    ASSERT_TRUE(result.HasValue());
    
    String actualValue = lap::core::get<String>(result.Value());
    EXPECT_EQ(expectedValue, actualValue);
}

TEST_F(PropertyBackendTest, KeyExists_ExistingKey_ShouldReturnTrue) {
    backend->SetValue("existing_key", KvsDataType(Int32(42)));
    
    auto result = backend->KeyExists("existing_key");
    ASSERT_TRUE(result.HasValue());
    EXPECT_TRUE(result.Value());
}

TEST_F(PropertyBackendTest, KeyExists_NonExistingKey_ShouldReturnFalse) {
    auto result = backend->KeyExists("non_existing_key");
    ASSERT_TRUE(result.HasValue());
    EXPECT_FALSE(result.Value());
}

TEST_F(PropertyBackendTest, GetValue_NonExistingKey_ShouldReturnError) {
    auto result = backend->GetValue("non_existing_key");
    EXPECT_FALSE(result.HasValue());
}

TEST_F(PropertyBackendTest, RemoveKey_ExistingKey_ShouldSucceed) {
    backend->SetValue("key_to_remove", KvsDataType(Int32(42)));
    
    auto removeResult = backend->RemoveKey("key_to_remove");
    EXPECT_TRUE(removeResult.HasValue());
    
    auto existsResult = backend->KeyExists("key_to_remove");
    ASSERT_TRUE(existsResult.HasValue());
    EXPECT_FALSE(existsResult.Value());
}

TEST_F(PropertyBackendTest, RemoveAllKeys_ShouldClearAllData) {
    backend->SetValue("key1", KvsDataType(Int32(1)));
    backend->SetValue("key2", KvsDataType(Int32(2)));
    backend->SetValue("key3", KvsDataType(Int32(3)));
    
    auto result = backend->RemoveAllKeys();
    EXPECT_TRUE(result.HasValue());
    
    auto keysResult = backend->GetAllKeys();
    ASSERT_TRUE(keysResult.HasValue());
    EXPECT_EQ(0, keysResult.Value().size());
}

TEST_F(PropertyBackendTest, GetAllKeys_ShouldReturnAllStoredKeys) {
    backend->SetValue("key1", KvsDataType(Int32(1)));
    backend->SetValue("key2", KvsDataType(Int32(2)));
    backend->SetValue("key3", KvsDataType(Int32(3)));
    
    auto result = backend->GetAllKeys();
    ASSERT_TRUE(result.HasValue());
    
    auto keys = result.Value();
    EXPECT_EQ(3, keys.size());
    
    // Check if all keys are present
    std::sort(keys.begin(), keys.end());
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "key1") != keys.end());
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "key2") != keys.end());
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "key3") != keys.end());
}

TEST_F(PropertyBackendTest, UpdateValue_ShouldOverwriteExistingValue) {
    backend->SetValue("update_key", KvsDataType(Int32(42)));
    backend->SetValue("update_key", KvsDataType(Int32(99)));
    
    auto result = backend->GetValue("update_key");
    ASSERT_TRUE(result.HasValue());
    
    Int32 actualValue = lap::core::get<Int32>(result.Value());
    EXPECT_EQ(99, actualValue);
}

// ============================================================================
// Boundary Value Tests
// ============================================================================

TEST_F(PropertyBackendTest, BoundaryValue_Int8_Min) {
    Int8 minValue = std::numeric_limits<Int8>::min();
    backend->SetValue("int8_min", KvsDataType(minValue));
    
    auto result = backend->GetValue("int8_min");
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(minValue, lap::core::get<Int8>(result.Value()));
}

TEST_F(PropertyBackendTest, BoundaryValue_Int8_Max) {
    Int8 maxValue = std::numeric_limits<Int8>::max();
    backend->SetValue("int8_max", KvsDataType(maxValue));
    
    auto result = backend->GetValue("int8_max");
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(maxValue, lap::core::get<Int8>(result.Value()));
}

TEST_F(PropertyBackendTest, BoundaryValue_UInt8_Max) {
    UInt8 maxValue = std::numeric_limits<UInt8>::max();
    backend->SetValue("uint8_max", KvsDataType(maxValue));
    
    auto result = backend->GetValue("uint8_max");
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(maxValue, lap::core::get<UInt8>(result.Value()));
}

TEST_F(PropertyBackendTest, BoundaryValue_Int32_Min) {
    Int32 minValue = std::numeric_limits<Int32>::min();
    backend->SetValue("int32_min", KvsDataType(minValue));
    
    auto result = backend->GetValue("int32_min");
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(minValue, lap::core::get<Int32>(result.Value()));
}

TEST_F(PropertyBackendTest, BoundaryValue_Int32_Max) {
    Int32 maxValue = std::numeric_limits<Int32>::max();
    backend->SetValue("int32_max", KvsDataType(maxValue));
    
    auto result = backend->GetValue("int32_max");
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(maxValue, lap::core::get<Int32>(result.Value()));
}

TEST_F(PropertyBackendTest, BoundaryValue_Int64_Min) {
    Int64 minValue = std::numeric_limits<Int64>::min();
    backend->SetValue("int64_min", KvsDataType(minValue));
    
    auto result = backend->GetValue("int64_min");
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(minValue, lap::core::get<Int64>(result.Value()));
}

TEST_F(PropertyBackendTest, BoundaryValue_Int64_Max) {
    Int64 maxValue = std::numeric_limits<Int64>::max();
    backend->SetValue("int64_max", KvsDataType(maxValue));
    
    auto result = backend->GetValue("int64_max");
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(maxValue, lap::core::get<Int64>(result.Value()));
}

TEST_F(PropertyBackendTest, BoundaryValue_Float_Min) {
    Float minValue = std::numeric_limits<Float>::lowest();
    backend->SetValue("float_min", KvsDataType(minValue));
    
    auto result = backend->GetValue("float_min");
    ASSERT_TRUE(result.HasValue());
    EXPECT_FLOAT_EQ(minValue, lap::core::get<Float>(result.Value()));
}

TEST_F(PropertyBackendTest, BoundaryValue_Float_Max) {
    Float maxValue = std::numeric_limits<Float>::max();
    backend->SetValue("float_max", KvsDataType(maxValue));
    
    auto result = backend->GetValue("float_max");
    ASSERT_TRUE(result.HasValue());
    EXPECT_FLOAT_EQ(maxValue, lap::core::get<Float>(result.Value()));
}

TEST_F(PropertyBackendTest, BoundaryValue_Double_Min) {
    Double minValue = std::numeric_limits<Double>::lowest();
    backend->SetValue("double_min", KvsDataType(minValue));
    
    auto result = backend->GetValue("double_min");
    ASSERT_TRUE(result.HasValue());
    EXPECT_DOUBLE_EQ(minValue, lap::core::get<Double>(result.Value()));
}

TEST_F(PropertyBackendTest, BoundaryValue_Double_Max) {
    Double maxValue = std::numeric_limits<Double>::max();
    backend->SetValue("double_max", KvsDataType(maxValue));
    
    auto result = backend->GetValue("double_max");
    ASSERT_TRUE(result.HasValue());
    EXPECT_DOUBLE_EQ(maxValue, lap::core::get<Double>(result.Value()));
}

TEST_F(PropertyBackendTest, BoundaryValue_EmptyString) {
    String emptyValue = "";
    backend->SetValue("empty_string", KvsDataType(emptyValue));
    
    auto result = backend->GetValue("empty_string");
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(emptyValue, lap::core::get<String>(result.Value()));
}

TEST_F(PropertyBackendTest, BoundaryValue_LongString) {
    String longValue(1000, 'A'); // 1000 character string
    backend->SetValue("long_string", KvsDataType(longValue));
    
    auto result = backend->GetValue("long_string");
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(longValue, lap::core::get<String>(result.Value()));
}

TEST_F(PropertyBackendTest, BoundaryValue_VeryLongString) {
    String veryLongValue(10000, 'B'); // 10KB string
    backend->SetValue("very_long_string", KvsDataType(veryLongValue));
    
    auto result = backend->GetValue("very_long_string");
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(veryLongValue, lap::core::get<String>(result.Value()));
}

TEST_F(PropertyBackendTest, BoundaryValue_SpecialCharactersInString) {
    String specialValue = "!@#$%^&*()_+-=[]{}|;:',.<>?/~`\n\t\r";
    backend->SetValue("special_chars", KvsDataType(specialValue));
    
    auto result = backend->GetValue("special_chars");
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(specialValue, lap::core::get<String>(result.Value()));
}

TEST_F(PropertyBackendTest, BoundaryValue_UnicodeString) {
    String unicodeValue = u8"Hello 世界 🌍 مرحبا";
    backend->SetValue("unicode_string", KvsDataType(unicodeValue));
    
    auto result = backend->GetValue("unicode_string");
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(unicodeValue, lap::core::get<String>(result.Value()));
}

TEST_F(PropertyBackendTest, BoundaryValue_KeyWithSpecialCharacters) {
    String key = "key.with-special_chars:123";
    Int32 value = 42;
    backend->SetValue(key, KvsDataType(value));
    
    auto result = backend->GetValue(key);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(value, lap::core::get<Int32>(result.Value()));
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST_F(PropertyBackendTest, StressTest_ManyKeys_Sequential) {
    const int numKeys = 1000;
    
    // Write many keys
    auto startWrite = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numKeys; ++i) {
        String key = "stress_key_" + std::to_string(i);
        backend->SetValue(key, KvsDataType(Int32(i)));
    }
    auto endWrite = std::chrono::high_resolution_clock::now();
    
    // Read all keys
    auto startRead = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numKeys; ++i) {
        String key = "stress_key_" + std::to_string(i);
        auto result = backend->GetValue(key);
        ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(i, lap::core::get<Int32>(result.Value()));
    }
    auto endRead = std::chrono::high_resolution_clock::now();
    
    auto writeDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endWrite - startWrite);
    auto readDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endRead - startRead);
    
    std::cout << "Stress test - " << numKeys << " keys:" << std::endl;
    std::cout << "  Write time: " << writeDuration.count() << " ms" << std::endl;
    std::cout << "  Read time: " << readDuration.count() << " ms" << std::endl;
    
    // Verify all keys exist
    auto keysResult = backend->GetAllKeys();
    ASSERT_TRUE(keysResult.HasValue());
    EXPECT_EQ(numKeys, keysResult.Value().size());
}

TEST_F(PropertyBackendTest, StressTest_ManyUpdates_SameKey) {
    const int numUpdates = 10000;
    String key = "stress_update_key";
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numUpdates; ++i) {
        backend->SetValue(key, KvsDataType(Int32(i)));
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Stress test - " << numUpdates << " updates on same key:" << std::endl;
    std::cout << "  Time: " << duration.count() << " ms" << std::endl;
    
    // Verify final value
    auto result = backend->GetValue(key);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(numUpdates - 1, lap::core::get<Int32>(result.Value()));
}

TEST_F(PropertyBackendTest, StressTest_RandomOperations) {
    const int numOperations = 5000;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> opDist(0, 3); // 0=set, 1=get, 2=remove, 3=exists
    std::uniform_int_distribution<> keyDist(0, 99);
    
    Vector<String> keys;
    for (int i = 0; i < 100; ++i) {
        keys.push_back("random_key_" + std::to_string(i));
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    int setCount = 0, getCount = 0, removeCount = 0, existsCount = 0;
    
    for (int i = 0; i < numOperations; ++i) {
        int op = opDist(gen);
        String key = keys[keyDist(gen)];
        
        switch (op) {
            case 0: // Set
                backend->SetValue(key, KvsDataType(Int32(i)));
                setCount++;
                break;
            case 1: // Get
                backend->GetValue(key);
                getCount++;
                break;
            case 2: // Remove
                backend->RemoveKey(key);
                removeCount++;
                break;
            case 3: // Exists
                backend->KeyExists(key);
                existsCount++;
                break;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Stress test - " << numOperations << " random operations:" << std::endl;
    std::cout << "  Set: " << setCount << ", Get: " << getCount 
              << ", Remove: " << removeCount << ", Exists: " << existsCount << std::endl;
    std::cout << "  Total time: " << duration.count() << " ms" << std::endl;
}

TEST_F(PropertyBackendTest, StressTest_LargeDataVolume) {
    const int numKeys = 100;
    const int stringSize = 10000; // 10KB per string
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numKeys; ++i) {
        String key = "large_data_" + std::to_string(i);
        String largeValue(stringSize, 'X');
        backend->SetValue(key, KvsDataType(largeValue));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Stress test - " << numKeys << " keys with " << stringSize << " bytes each:" << std::endl;
    std::cout << "  Total data: ~" << (numKeys * stringSize / 1024) << " KB" << std::endl;
    std::cout << "  Time: " << duration.count() << " ms" << std::endl;
    
    // Verify one key
    auto result = backend->GetValue("large_data_50");
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(stringSize, lap::core::get<String>(result.Value()).size());
}

TEST_F(PropertyBackendTest, StressTest_MixedDataTypes) {
    const int numKeysPerType = 100;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numKeysPerType; ++i) {
        backend->SetValue("int8_" + std::to_string(i), KvsDataType(Int8(i % 128)));
        backend->SetValue("uint8_" + std::to_string(i), KvsDataType(UInt8(i % 256)));
        backend->SetValue("int32_" + std::to_string(i), KvsDataType(Int32(i)));
        backend->SetValue("int64_" + std::to_string(i), KvsDataType(Int64(i * 1000000LL)));
        backend->SetValue("float_" + std::to_string(i), KvsDataType(Float(i * 1.5f)));
        backend->SetValue("double_" + std::to_string(i), KvsDataType(Double(i * 2.5)));
        backend->SetValue("bool_" + std::to_string(i), KvsDataType(Bool(i % 2 == 0)));
        backend->SetValue("string_" + std::to_string(i), KvsDataType(String("value_" + std::to_string(i))));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Stress test - Mixed data types (" << numKeysPerType * 8 << " total keys):" << std::endl;
    std::cout << "  Time: " << duration.count() << " ms" << std::endl;
    
    // Verify all keys
    auto keysResult = backend->GetAllKeys();
    ASSERT_TRUE(keysResult.HasValue());
    EXPECT_EQ(numKeysPerType * 8, keysResult.Value().size());
}

// ============================================================================
// Concurrent Access Tests
// ============================================================================

TEST_F(PropertyBackendTest, ConcurrentAccess_MultipleReaders) {
    // Setup data
    const int numKeys = 100;
    for (int i = 0; i < numKeys; ++i) {
        backend->SetValue("concurrent_key_" + std::to_string(i), KvsDataType(Int32(i)));
    }
    
    const int numThreads = 4;
    const int readsPerThread = 1000;
    Vector<std::thread> threads;
    std::atomic<int> successCount(0);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < readsPerThread; ++i) {
                String key = "concurrent_key_" + std::to_string(i % numKeys);
                auto result = backend->GetValue(key);
                if (result.HasValue()) {
                    successCount++;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Concurrent access - " << numThreads << " readers:" << std::endl;
    std::cout << "  Total reads: " << numThreads * readsPerThread << std::endl;
    std::cout << "  Successful: " << successCount.load() << std::endl;
    std::cout << "  Time: " << duration.count() << " ms" << std::endl;
    
    EXPECT_EQ(numThreads * readsPerThread, successCount.load());
}

TEST_F(PropertyBackendTest, ConcurrentAccess_MixedReadWrite) {
    const int numThreads = 4;
    const int opsPerThread = 500;
    Vector<std::thread> threads;
    std::atomic<int> readCount(0);
    std::atomic<int> writeCount(0);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < opsPerThread; ++i) {
                if (i % 2 == 0) {
                    // Write
                    String key = "mixed_key_" + std::to_string(t * opsPerThread + i);
                    backend->SetValue(key, KvsDataType(Int32(i)));
                    writeCount++;
                } else {
                    // Read
                    String key = "mixed_key_" + std::to_string((t * opsPerThread + i - 1));
                    backend->GetValue(key);
                    readCount++;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Concurrent access - Mixed read/write:" << std::endl;
    std::cout << "  Threads: " << numThreads << std::endl;
    std::cout << "  Writes: " << writeCount.load() << std::endl;
    std::cout << "  Reads: " << readCount.load() << std::endl;
    std::cout << "  Time: " << duration.count() << " ms" << std::endl;
}

// ============================================================================
// Memory Management Tests
// ============================================================================

TEST_F(PropertyBackendTest, MemoryTest_AddRemoveRepeatedly) {
    const int numCycles = 100;
    const int keysPerCycle = 100;
    
    for (int cycle = 0; cycle < numCycles; ++cycle) {
        // Add keys
        for (int i = 0; i < keysPerCycle; ++i) {
            String key = "memory_test_" + std::to_string(i);
            backend->SetValue(key, KvsDataType(Int32(i)));
        }
        
        // Remove all keys
        backend->RemoveAllKeys();
    }
    
    // Verify no keys remain
    auto keysResult = backend->GetAllKeys();
    ASSERT_TRUE(keysResult.HasValue());
    EXPECT_EQ(0, keysResult.Value().size());
}

TEST_F(PropertyBackendTest, MemoryTest_GrowAndShrink) {
    const int maxKeys = 500;
    
    // Grow
    for (int i = 0; i < maxKeys; ++i) {
        backend->SetValue("grow_key_" + std::to_string(i), KvsDataType(Int32(i)));
    }
    
    auto keysResult = backend->GetAllKeys();
    ASSERT_TRUE(keysResult.HasValue());
    EXPECT_EQ(maxKeys, keysResult.Value().size());
    
    // Shrink
    for (int i = 0; i < maxKeys / 2; ++i) {
        backend->RemoveKey("grow_key_" + std::to_string(i));
    }
    
    auto keysResult2 = backend->GetAllKeys();
    ASSERT_TRUE(keysResult2.HasValue());
    EXPECT_EQ(maxKeys / 2, keysResult2.Value().size());
}

TEST_F(PropertyBackendTest, PersistenceTest_DataSurvivesRecreation) {
    // Create first backend and store data
    {
        KvsPropertyBackend backend1("persistence_test");
        backend1.SetValue("persist_key", KvsDataType(Int32(12345)));
        backend1.SyncToStorage();
    }
    
    // Create second backend with same name - should see the data
    {
        KvsPropertyBackend backend2("persistence_test");
        auto result = backend2.GetValue("persist_key");
        ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(12345, lap::core::get<Int32>(result.Value()));
        
        // Cleanup
        backend2.RemoveAllKeys();
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(PropertyBackendTest, EdgeCase_ZeroValues) {
    backend->SetValue("zero_int", KvsDataType(Int32(0)));
    backend->SetValue("zero_float", KvsDataType(Float(0.0f)));
    backend->SetValue("zero_double", KvsDataType(Double(0.0)));
    
    auto intResult = backend->GetValue("zero_int");
    ASSERT_TRUE(intResult.HasValue());
    EXPECT_EQ(0, lap::core::get<Int32>(intResult.Value()));
    
    auto floatResult = backend->GetValue("zero_float");
    ASSERT_TRUE(floatResult.HasValue());
    EXPECT_FLOAT_EQ(0.0f, lap::core::get<Float>(floatResult.Value()));
}

TEST_F(PropertyBackendTest, EdgeCase_NegativeValues) {
    backend->SetValue("neg_int8", KvsDataType(Int8(-128)));
    backend->SetValue("neg_int32", KvsDataType(Int32(-2147483648)));
    backend->SetValue("neg_float", KvsDataType(Float(-3.14f)));
    
    auto int8Result = backend->GetValue("neg_int8");
    ASSERT_TRUE(int8Result.HasValue());
    EXPECT_EQ(-128, lap::core::get<Int8>(int8Result.Value()));
}

TEST_F(PropertyBackendTest, EdgeCase_BoolToggle) {
    backend->SetValue("toggle", KvsDataType(Bool(true)));
    backend->SetValue("toggle", KvsDataType(Bool(false)));
    backend->SetValue("toggle", KvsDataType(Bool(true)));
    
    auto result = backend->GetValue("toggle");
    ASSERT_TRUE(result.HasValue());
    EXPECT_TRUE(lap::core::get<Bool>(result.Value()));
}

TEST_F(PropertyBackendTest, EdgeCase_SameKeyDifferentTypes) {
    // After fix: changing a key's type should overwrite the old value
    // This verifies that removeAllTypedVariants() works correctly
    String key = "type_change_key";
    
    // Store Int32
    backend->SetValue(key, KvsDataType(Int32(42)));
    
    // Verify Int32 is stored
    auto intResult = backend->GetValue(key);
    ASSERT_TRUE(intResult.HasValue());
    EXPECT_EQ(42, lap::core::get<Int32>(intResult.Value()));
    
    // Change type to String - should overwrite the Int32 value
    backend->SetValue(key, KvsDataType(String("forty-two")));
    
    // Verify String is now stored
    auto stringResult = backend->GetValue(key);
    ASSERT_TRUE(stringResult.HasValue());
    EXPECT_EQ("forty-two", lap::core::get<String>(stringResult.Value()));
    
    // GetAllKeys should show only ONE entry now (old type removed)
    auto keysResult = backend->GetAllKeys();
    ASSERT_TRUE(keysResult.HasValue());
    
    // Count occurrences of our key
    int keyCount = 0;
    for (const auto& k : keysResult.Value()) {
        if (k == key) {
            keyCount++;
        }
    }
    
    // Should be exactly 1 (not 2 as before the fix)
    EXPECT_EQ(1, keyCount);
    
    // Change type again to Double
    backend->SetValue(key, KvsDataType(Double(42.0)));
    
    // Verify Double is now stored
    auto doubleResult = backend->GetValue(key);
    ASSERT_TRUE(doubleResult.HasValue());
    EXPECT_NEAR(42.0, lap::core::get<Double>(doubleResult.Value()), 1e-9);
    
    // Still only one key
    auto keysResult2 = backend->GetAllKeys();
    ASSERT_TRUE(keysResult2.HasValue());
    keyCount = 0;
    for (const auto& k : keysResult2.Value()) {
        if (k == key) {
            keyCount++;
        }
    }
    EXPECT_EQ(1, keyCount);
}
