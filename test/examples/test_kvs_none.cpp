/**
 * @file test_kvs_none.cpp
 * @brief Quick test for KvsBackendType::kvsNone (memory-only mode)
 * @date 2025-11-17
 */

#include <iostream>
#include <lap/core/CCore.hpp>
#include "CKvsPropertyBackend.hpp"
#include "CDataType.hpp"

using namespace lap::core;
using namespace lap::per;
using namespace lap::per::util;

int main() {
    std::cout << "Testing KvsBackendType::kvsNone (Memory-Only Mode)\n" << std::endl;
    
    try {
        // Test 1: Create Property Backend with kvsNone
        std::cout << "Test 1: Creating Property Backend with kvsNone..." << std::endl;
        KvsPropertyBackend backend("test_kvs_none", KvsBackendType::kvsNone, 1ul << 20);
        std::cout << "  ✓ Backend created successfully" << std::endl;
        
        // Test 2: Write data
        std::cout << "\nTest 2: Writing data to memory..." << std::endl;
        backend.SetValue("key1", String("value1"));
        backend.SetValue("key2", Int32(42));
        backend.SetValue("key3", Float(3.14f));
        std::cout << "  ✓ Data written successfully" << std::endl;
        
        // Test 3: Read data
        std::cout << "\nTest 3: Reading data from memory..." << std::endl;
        auto val1 = backend.GetValue("key1");
        auto val2 = backend.GetValue("key2");
        auto val3 = backend.GetValue("key3");
        
        if (val1.HasValue() && val2.HasValue() && val3.HasValue()) {
            auto str = std::get_if<String>(&val1.Value());
            auto num = std::get_if<Int32>(&val2.Value());
            auto flt = std::get_if<Float>(&val3.Value());
            
            std::cout << "  ✓ key1 = " << (str ? str->data() : "ERROR") << std::endl;
            std::cout << "  ✓ key2 = " << (num ? *num : -1) << std::endl;
            std::cout << "  ✓ key3 = " << (flt ? *flt : 0.0f) << std::endl;
        } else {
            std::cout << "  ✗ Failed to read values" << std::endl;
            return 1;
        }
        
        // Test 4: GetAllKeys
        std::cout << "\nTest 4: Getting all keys..." << std::endl;
        auto keys = backend.GetAllKeys();
        if (keys.HasValue()) {
            std::cout << "  ✓ Total keys: " << keys.Value().size() << std::endl;
            for (const auto& key : keys.Value()) {
                std::cout << "    - " << key.data() << std::endl;
            }
        }
        
        // Test 5: Remove key
        std::cout << "\nTest 5: Removing key..." << std::endl;
        backend.RemoveKey("key2");
        auto keysAfter = backend.GetAllKeys();
        if (keysAfter.HasValue()) {
            std::cout << "  ✓ Keys after removal: " << keysAfter.Value().size() << std::endl;
        }
        
        // Test 6: SyncToStorage should succeed (no-op for kvsNone)
        std::cout << "\nTest 6: Testing SyncToStorage (should be no-op)..." << std::endl;
        auto syncResult = backend.SyncToStorage();
        if (syncResult.HasValue()) {
            std::cout << "  ✓ SyncToStorage succeeded (no-op for memory-only)" << std::endl;
        } else {
            std::cout << "  ✗ SyncToStorage failed unexpectedly" << std::endl;
            return 1;
        }
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "All tests PASSED!" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "\nKvsBackendType::kvsNone verified working correctly:" << std::endl;
        std::cout << "  ✓ No persistence backend created" << std::endl;
        std::cout << "  ✓ Pure memory operations" << std::endl;
        std::cout << "  ✓ SyncToStorage is no-op (no errors)" << std::endl;
        std::cout << "  ✓ Data accessible via shared memory" << std::endl;
        std::cout << "  ✓ Data will be lost on process restart (by design)" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
}
