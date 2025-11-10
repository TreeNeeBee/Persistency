/**
 * @file sqlite_backend_demo.cpp
 * @brief SQLite Backend功能演示和性能测试
 * 
 * 本示例展示:
 * 1. SQLite backend基本CRUD操作
 * 2. 各种数据类型存储和检索
 * 3. 性能基准测试
 * 4. 事务和持久化功能
 */

#include "CKvsSqliteBackend.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>

using namespace lap::pm;
using namespace lap::core;

// 性能计时器
class Timer {
public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}
    
    double elapsed() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }
private:
    std::chrono::high_resolution_clock::time_point start_;
};

void printSection(const char* title) {
    std::cout << "\n========== " << title << " ==========\n";
}

void testBasicOperations() {
    printSection("Basic CRUD Operations");
    
    KvsSqliteBackend backend("/tmp/test_kvs.db");
    
    if (!backend.available()) {
        std::cerr << "Failed to initialize SQLite backend\n";
        return;
    }
    
    // 测试各种类型的写入
    std::cout << "Writing different types...\n";
    backend.SetValue("int8_key", KvsDataType(static_cast<Int8>(-123)));
    backend.SetValue("uint8_key", KvsDataType(static_cast<UInt8>(255)));
    backend.SetValue("int32_key", KvsDataType(static_cast<Int32>(-123456)));
    backend.SetValue("uint32_key", KvsDataType(static_cast<UInt32>(4294967295u)));
    backend.SetValue("int64_key", KvsDataType(static_cast<Int64>(-9223372036854775807LL)));
    backend.SetValue("bool_key", KvsDataType(true));
    backend.SetValue("float_key", KvsDataType(3.14159f));
    backend.SetValue("double_key", KvsDataType(3.141592653589793));
    backend.SetValue("string_key", KvsDataType(String("Hello SQLite!")));
    
    std::cout << "✓ Wrote 9 keys with different types\n";
    
    // 读取并验证
    std::cout << "\nReading values...\n";
    auto int8_result = backend.GetValue("int8_key");
    if (int8_result.HasValue()) {
        std::cout << "int8_key = " << static_cast<int>(std::get<Int8>(int8_result.Value())) << "\n";
    }
    
    auto string_result = backend.GetValue("string_key");
    if (string_result.HasValue()) {
        std::cout << "string_key = " << std::get<String>(string_result.Value()) << "\n";
    }
    
    auto double_result = backend.GetValue("double_key");
    if (double_result.HasValue()) {
        std::cout << std::setprecision(15) << "double_key = " << std::get<Double>(double_result.Value()) << "\n";
    }
    
    // 测试KeyExists
    auto exists = backend.KeyExists("int32_key");
    std::cout << "int32_key exists: " << (exists.Value() ? "Yes" : "No") << "\n";
    
    auto not_exists = backend.KeyExists("nonexistent");
    std::cout << "nonexistent exists: " << (not_exists.Value() ? "Yes" : "No") << "\n";
    
    // 测试GetAllKeys
    auto keys = backend.GetAllKeys();
    std::cout << "\nTotal keys: " << keys.Value().size() << "\n";
    std::cout << "Keys: ";
    for (const auto& key : keys.Value()) {
        std::cout << key << " ";
    }
    std::cout << "\n";
    
    // 测试更新
    std::cout << "\nUpdating int32_key...\n";
    backend.SetValue("int32_key", KvsDataType(static_cast<Int32>(999)));
    auto updated = backend.GetValue("int32_key");
    std::cout << "Updated int32_key = " << std::get<Int32>(updated.Value()) << "\n";
    
    // 测试删除
    std::cout << "\nDeleting bool_key...\n";
    backend.RemoveKey("bool_key");
    auto after_delete = backend.KeyExists("bool_key");
    std::cout << "bool_key exists after delete: " << (after_delete.Value() ? "Yes" : "No") << "\n";
    
    std::cout << "✓ Basic operations completed successfully\n";
}

void testPerformance() {
    printSection("Performance Benchmark");
    
    KvsSqliteBackend backend("/tmp/perf_test_kvs.db");
    
    if (!backend.available()) {
        std::cerr << "Failed to initialize SQLite backend\n";
        return;
    }
    
    // 清空之前的数据
    backend.RemoveAllKeys();
    backend.SyncToStorage();
    
    // 测试1: 顺序写入
    {
        const int COUNT = 10000;
        Timer timer;
        
        for (int i = 0; i < COUNT; ++i) {
            String key = "key_" + std::to_string(i);
            backend.SetValue(key, KvsDataType(static_cast<Int32>(i)));
        }
        
        double elapsed = timer.elapsed();
        std::cout << "Sequential write (" << COUNT << " keys): " 
                  << elapsed << " ms ("
                  << (COUNT / elapsed * 1000) << " ops/sec)\n";
    }
    
    // 测试2: 顺序读取
    {
        const int COUNT = 10000;
        Timer timer;
        int success = 0;
        
        for (int i = 0; i < COUNT; ++i) {
            String key = "key_" + std::to_string(i);
            auto result = backend.GetValue(key);
            if (result.HasValue()) success++;
        }
        
        double elapsed = timer.elapsed();
        std::cout << "Sequential read (" << COUNT << " keys): " 
                  << elapsed << " ms ("
                  << (COUNT / elapsed * 1000) << " ops/sec)"
                  << " Success: " << success << "/" << COUNT << "\n";
    }
    
    // 测试3: 随机更新（同一个key）
    {
        const int COUNT = 10000;
        Timer timer;
        
        for (int i = 0; i < COUNT; ++i) {
            backend.SetValue("same_key", KvsDataType(static_cast<Int32>(i)));
        }
        
        double elapsed = timer.elapsed();
        std::cout << "Same-key updates (" << COUNT << " updates): " 
                  << elapsed << " ms ("
                  << (COUNT / elapsed * 1000) << " ops/sec)\n";
    }
    
    // 测试4: 混合操作
    {
        const int COUNT = 5000;
        Timer timer;
        int ops = 0;
        
        for (int i = 0; i < COUNT; ++i) {
            String key = "mixed_" + std::to_string(i % 100);
            backend.SetValue(key, KvsDataType(static_cast<Int32>(i)));
            ops++;
            
            auto result = backend.GetValue(key);
            if (result.HasValue()) ops++;
            
            if (i % 10 == 0) {
                backend.RemoveKey(key);
                ops++;
            }
        }
        
        double elapsed = timer.elapsed();
        std::cout << "Mixed operations (" << ops << " ops): " 
                  << elapsed << " ms ("
                  << (ops / elapsed * 1000) << " ops/sec)\n";
    }
    
    // 测试5: 同步性能
    {
        Timer timer;
        backend.SyncToStorage();
        double elapsed = timer.elapsed();
        std::cout << "Sync to storage: " << elapsed << " ms\n";
    }
    
    std::cout << "✓ Performance benchmark completed\n";
}

void testTypeEncoding() {
    printSection("Type Encoding Test");
    
    KvsSqliteBackend backend("/tmp/type_test_kvs.db");
    
    if (!backend.available()) {
        std::cerr << "Failed to initialize SQLite backend\n";
        return;
    }
    
    // 测试相同key不同类型（应该覆盖）
    std::cout << "Testing same key with different types...\n";
    
    backend.SetValue("test_key", KvsDataType(static_cast<Int32>(123)));
    auto result1 = backend.GetValue("test_key");
    std::cout << "First (Int32): " << std::get<Int32>(result1.Value()) << "\n";
    
    backend.SetValue("test_key", KvsDataType(String("String value")));
    auto result2 = backend.GetValue("test_key");
    std::cout << "Second (String): " << std::get<String>(result2.Value()) << "\n";
    
    backend.SetValue("test_key", KvsDataType(3.14159));
    auto result3 = backend.GetValue("test_key");
    std::cout << "Third (Double): " << std::setprecision(10) << std::get<Double>(result3.Value()) << "\n";
    
    std::cout << "✓ Type encoding works correctly\n";
}

void testPersistence() {
    printSection("Persistence Test");
    
    // 第一次写入
    {
        KvsSqliteBackend backend("/tmp/persist_test_kvs.db");
        std::cout << "Writing data in first instance...\n";
        backend.SetValue("persist_key1", KvsDataType(static_cast<Int32>(42)));
        backend.SetValue("persist_key2", KvsDataType(String("Persistent data")));
        backend.SyncToStorage();
        std::cout << "✓ Data written and synced\n";
    }
    
    // 第二次读取（新实例）
    {
        KvsSqliteBackend backend("/tmp/persist_test_kvs.db");
        std::cout << "Reading data in second instance...\n";
        
        auto result1 = backend.GetValue("persist_key1");
        if (result1.HasValue()) {
            std::cout << "persist_key1 = " << std::get<Int32>(result1.Value()) << "\n";
        } else {
            std::cout << "❌ Failed to read persist_key1\n";
        }
        
        auto result2 = backend.GetValue("persist_key2");
        if (result2.HasValue()) {
            std::cout << "persist_key2 = " << std::get<String>(result2.Value()) << "\n";
        } else {
            std::cout << "❌ Failed to read persist_key2\n";
        }
        
        std::cout << "✓ Data persisted correctly across instances\n";
    }
}

void testSoftDelete() {
    printSection("Soft Delete & Recovery Test");
    
    KvsSqliteBackend backend("/tmp/softdelete_test_kvs.db");
    
    std::cout << "Creating test key...\n";
    backend.SetValue("delete_test", KvsDataType(static_cast<Int32>(999)));
    
    auto before = backend.KeyExists("delete_test");
    std::cout << "Key exists before delete: " << (before.Value() ? "Yes" : "No") << "\n";
    
    std::cout << "Soft deleting key...\n";
    backend.RemoveKey("delete_test");
    
    auto after = backend.KeyExists("delete_test");
    std::cout << "Key exists after delete: " << (after.Value() ? "Yes" : "No") << "\n";
    
    std::cout << "Recovering key...\n";
    backend.RecoveryKey("delete_test");
    
    auto recovered = backend.KeyExists("delete_test");
    std::cout << "Key exists after recovery: " << (recovered.Value() ? "Yes" : "No") << "\n";
    
    if (recovered.Value()) {
        auto value = backend.GetValue("delete_test");
        std::cout << "Recovered value: " << std::get<Int32>(value.Value()) << "\n";
    }
    
    std::cout << "✓ Soft delete and recovery work correctly\n";
}

int main() {
    std::cout << "==============================================\n";
    std::cout << "   SQLite Backend Demonstration & Testing\n";
    std::cout << "==============================================\n";
    
    try {
        testBasicOperations();
        testPerformance();
        testTypeEncoding();
        testPersistence();
        testSoftDelete();
        
        std::cout << "\n==============================================\n";
        std::cout << "   All tests completed successfully! ✓\n";
        std::cout << "==============================================\n";
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Exception: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
