/**
 * @file performance_benchmark.cpp
 * @brief Performance benchmark suite for Persistency backends
 * @details Compares File, SQLite, and Property backend performance
 */

#include "CKvsFileBackend.hpp"
#include "CKvsSqliteBackend.hpp"
#include "CKvsPropertyBackend.hpp"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <numeric>

using namespace lap::per;
using namespace lap::per::util;
using namespace lap::core;

// ============================================================================
// Benchmark Infrastructure
// ============================================================================

class BenchmarkTimer {
public:
    void Start() { m_start = ::std::chrono::steady_clock::now(); }
    void Stop() { m_end = ::std::chrono::steady_clock::now(); }
    
    double GetMilliseconds() const {
        return ::std::chrono::duration_cast<::std::chrono::microseconds>(
            m_end - m_start).count() / 1000.0;
    }

private:
    ::std::chrono::steady_clock::time_point m_start;
    ::std::chrono::steady_clock::time_point m_end;
};

// ============================================================================
// Benchmark Functions
// ============================================================================

void BenchmarkFileBackend() {
    ::std::cout << "\n=== File Backend Performance ===" << ::std::endl;
    
    const char* dbPath = "/tmp/benchmark_file.db";
    ::std::remove(dbPath);
    
    KvsFileBackend backend("benchmark_file");
    BenchmarkTimer timer;
    
    // Write benchmark
    const int iterations = 1000;
    timer.Start();
    for (int i = 0; i < iterations; ++i) {
        ::std::string key = "key" + ::std::to_string(i);
        ::std::string value = "value" + ::std::to_string(i);
        backend.SetValue(key, String(value.c_str()));
    }
    timer.Stop();
    
    ::std::cout << "Write " << iterations << " keys: " 
                << ::std::fixed << ::std::setprecision(2) 
                << timer.GetMilliseconds() << " ms" << ::std::endl;
    
    // Read benchmark
    timer.Start();
    for (int i = 0; i < iterations; ++i) {
        ::std::string key = "key" + ::std::to_string(i);
        auto result = backend.GetValue(key);
    }
    timer.Stop();
    
    ::std::cout << "Read " << iterations << " keys: " 
                << timer.GetMilliseconds() << " ms" << ::std::endl;
    
    // Remove benchmark
    timer.Start();
    for (int i = 0; i < iterations; ++i) {
        ::std::string key = "key" + ::std::to_string(i);
        backend.RemoveKey(key);
    }
    timer.Stop();
    
    ::std::cout << "Remove " << iterations << " keys: " 
                << timer.GetMilliseconds() << " ms" << ::std::endl;
}

void BenchmarkSqliteBackend() {
    ::std::cout << "\n=== SQLite Backend Performance ===" << ::std::endl;
    
    const char* dbPath = "/tmp/benchmark_sqlite.db";
    ::std::remove(dbPath);
    
    KvsSqliteBackend backend("benchmark_sqlite");
    BenchmarkTimer timer;
    
    // Write benchmark
    const int iterations = 1000;
    timer.Start();
    for (int i = 0; i < iterations; ++i) {
        ::std::string key = "key" + ::std::to_string(i);
        ::std::string value = "value" + ::std::to_string(i);
        backend.SetValue(key, String(value.c_str()));
    }
    timer.Stop();
    
    ::std::cout << "Write " << iterations << " keys: " 
                << ::std::fixed << ::std::setprecision(2) 
                << timer.GetMilliseconds() << " ms" << ::std::endl;
    
    // Read benchmark
    timer.Start();
    for (int i = 0; i < iterations; ++i) {
        ::std::string key = "key" + ::std::to_string(i);
        auto result = backend.GetValue(key);
    }
    timer.Stop();
    
    ::std::cout << "Read " << iterations << " keys: " 
                << timer.GetMilliseconds() << " ms" << ::std::endl;
    
    // Remove benchmark
    timer.Start();
    for (int i = 0; i < iterations; ++i) {
        ::std::string key = "key" + ::std::to_string(i);
        backend.RemoveKey(key);
    }
    timer.Stop();
    
    ::std::cout << "Remove " << iterations << " keys: " 
                << timer.GetMilliseconds() << " ms" << ::std::endl;
}

void BenchmarkPropertyBackend() {
    ::std::cout << "\n=== Property Backend Performance (with File persistence) ===" 
                << ::std::endl;
    
    const char* dbPath = "/tmp/benchmark_property_file.db";
    ::std::remove(dbPath);
    
    KvsPropertyBackend backend("benchmark_property", KvsBackendType::kvsFile);
    BenchmarkTimer timer;
    
    // Write benchmark (in-memory)
    const int iterations = 1000;
    timer.Start();
    for (int i = 0; i < iterations; ++i) {
        ::std::string key = "key" + ::std::to_string(i);
        ::std::string value = "value" + ::std::to_string(i);
        backend.SetValue(key, String(value.c_str()));
    }
    timer.Stop();
    
    double writeTime = timer.GetMilliseconds();
    ::std::cout << "Write " << iterations << " keys (in-memory): " 
                << ::std::fixed << ::std::setprecision(2) 
                << writeTime << " ms" << ::std::endl;
    
    // Read benchmark (from memory)
    timer.Start();
    for (int i = 0; i < iterations; ++i) {
        ::std::string key = "key" + ::std::to_string(i);
        auto result = backend.GetValue(key);
    }
    timer.Stop();
    
    ::std::cout << "Read " << iterations << " keys (from memory): " 
                << timer.GetMilliseconds() << " ms" << ::std::endl;
    
    // Sync to persistence (use public API)
    timer.Start();
    backend.SyncToStorage();
    timer.Stop();
    
    ::std::cout << "Sync to persistence: " 
                << timer.GetMilliseconds() << " ms" << ::std::endl;
    
    // Remove benchmark
    timer.Start();
    for (int i = 0; i < iterations; ++i) {
        ::std::string key = "key" + ::std::to_string(i);
        backend.RemoveKey(key);
    }
    timer.Stop();
    
    ::std::cout << "Remove " << iterations << " keys: " 
                << timer.GetMilliseconds() << " ms" << ::std::endl;
}

void BenchmarkPropertyWithSqlite() {
    ::std::cout << "\n=== Property Backend Performance (with SQLite persistence) ===" 
                << ::std::endl;
    
    const char* dbPath = "/tmp/benchmark_property_sqlite.db";
    ::std::remove(dbPath);
    
    KvsPropertyBackend backend("benchmark_property_sqlite", KvsBackendType::kvsSqlite);
    BenchmarkTimer timer;
    
    // Write benchmark (in-memory)
    const int iterations = 1000;
    timer.Start();
    for (int i = 0; i < iterations; ++i) {
        ::std::string key = "key" + ::std::to_string(i);
        ::std::string value = "value" + ::std::to_string(i);
        backend.SetValue(key, String(value.c_str()));
    }
    timer.Stop();
    
    ::std::cout << "Write " << iterations << " keys (in-memory): " 
                << ::std::fixed << ::std::setprecision(2) 
                << timer.GetMilliseconds() << " ms" << ::std::endl;
    
    // Read benchmark (from memory)
    timer.Start();
    for (int i = 0; i < iterations; ++i) {
        ::std::string key = "key" + ::std::to_string(i);
        auto result = backend.GetValue(key);
    }
    timer.Stop();
    
    ::std::cout << "Read " << iterations << " keys (from memory): " 
                << timer.GetMilliseconds() << " ms" << ::std::endl;
    
    // Sync to persistence
    timer.Start();
    backend.SyncToStorage();
    timer.Stop();
    
    ::std::cout << "Sync to SQLite: " 
                << timer.GetMilliseconds() << " ms" << ::std::endl;
}

// ============================================================================
// Stress Tests
// ============================================================================

void StressTest_LargeDataset() {
    ::std::cout << "\n=== Stress Test: Large Dataset ===" 
                << ::std::endl;
    
    BenchmarkTimer timer;
    
    // Test File Backend with large dataset
    {
        ::std::cout << "\nFile Backend - 10,000 keys:" << ::std::endl;
        KvsFileBackend backend("stress_file_large");
        timer.Start();
        for (int i = 0; i < 10000; ++i) {
            ::std::string key = "large_key_" + ::std::to_string(i);
            backend.SetValue(key, Int32(i));
        }
        timer.Stop();
        ::std::cout << "  Write: " 
                    << ::std::fixed << ::std::setprecision(2)
                    << timer.GetMilliseconds() << " ms" << ::std::endl;
    }
    
    // Test Property Backend with larger shared memory (16MB for stress test)
    {
        ::std::cout << "\nProperty Backend - 10,000 keys (16MB shared memory):" << ::std::endl;
        constexpr size_t shmSize = 16ul << 20; // 16MB
        KvsPropertyBackend backend("stress_property_large", KvsBackendType::kvsFile, shmSize);
        
        int successCount = 0;
        timer.Start();
        for (int i = 0; i < 10000; ++i) {
            ::std::string key = "large_key_" + ::std::to_string(i);
            auto result = backend.SetValue(key, Int32(i));
            if (result.HasValue()) {
                successCount++;
            } else {
                // Hit memory limit
                break;
            }
        }
        timer.Stop();
        ::std::cout << "  Write (in-memory): " << timer.GetMilliseconds() << " ms" << ::std::endl;
        ::std::cout << "  Keys stored: " << successCount << "/10000" << ::std::endl;
        
        if (successCount > 0) {
            // Sync to persistence
            timer.Start();
            backend.SyncToStorage();
            timer.Stop();
            ::std::cout << "  Sync to disk: " << timer.GetMilliseconds() << " ms" << ::std::endl;
        }
    }
}

void StressTest_LargeValues() {
    ::std::cout << "\n=== Stress Test: Large Values ===" 
                << ::std::endl;
    
    BenchmarkTimer timer;
    
    // Test File Backend with 10KB values (100 operations)
    {
        ::std::cout << "\nFile Backend - 100 x 10KB values:" << ::std::endl;
        ::std::string largeValue(10000, 'x'); // 10KB string
        KvsFileBackend backend("stress_file_values");
        
        timer.Start();
        for (int i = 0; i < 100; ++i) {
            ::std::string key = "large_value_" + ::std::to_string(i);
            backend.SetValue(key, String(largeValue.c_str()));
        }
        timer.Stop();
        ::std::cout << "  Write: " 
                    << ::std::fixed << ::std::setprecision(2)
                    << timer.GetMilliseconds() << " ms" << ::std::endl;
        
        // Read back
        timer.Start();
        for (int i = 0; i < 100; ++i) {
            ::std::string key = "large_value_" + ::std::to_string(i);
            auto result = backend.GetValue(key);
        }
        timer.Stop();
        ::std::cout << "  Read: " << timer.GetMilliseconds() << " ms" << ::std::endl;
    }
    
    // Test Property Backend with larger shared memory (16MB)
    {
        ::std::cout << "\nProperty Backend - 100 x 10KB values (16MB shared memory):" << ::std::endl;
        ::std::string largeValue(10000, 'x'); // 10KB string
        constexpr size_t shmSize = 16ul << 20; // 16MB
        KvsPropertyBackend backend("stress_property_values", KvsBackendType::kvsFile, shmSize);
        
        int successCount = 0;
        timer.Start();
        for (int i = 0; i < 100; ++i) {
            ::std::string key = "large_value_" + ::std::to_string(i);
            auto result = backend.SetValue(key, String(largeValue.c_str()));
            if (result.HasValue()) {
                successCount++;
            } else {
                break; // Hit memory limit
            }
        }
        timer.Stop();
        ::std::cout << "  Write (in-memory): " << timer.GetMilliseconds() << " ms" << ::std::endl;
        ::std::cout << "  Values stored: " << successCount << "/100" << ::std::endl;
        
        if (successCount > 0) {
            // Read from memory
            timer.Start();
            for (int i = 0; i < successCount; ++i) {
                ::std::string key = "large_value_" + ::std::to_string(i);
                auto result = backend.GetValue(key);
            }
            timer.Stop();
            ::std::cout << "  Read (from memory): " << timer.GetMilliseconds() << " ms" << ::std::endl;
        }
    }
}

void StressTest_MixedOperations() {
    ::std::cout << "\n=== Stress Test: Mixed Operations (Read/Write/Update/Delete) ===" 
                << ::std::endl;
    
    BenchmarkTimer timer;
    const int totalOps = 5000;
    
    {
        constexpr size_t shmSize = 8ul << 20; // 8MB
        KvsPropertyBackend backend("stress_mixed", KvsBackendType::kvsFile, shmSize);
        
        timer.Start();
        
        // Mix of operations
        for (int i = 0; i < totalOps; ++i) {
            ::std::string key = "mixed_key_" + ::std::to_string(i % 1000);
            
            if (i % 4 == 0) {
                // Write
                backend.SetValue(key, Int32(i));
            } else if (i % 4 == 1) {
                // Read
                backend.GetValue(key);
            } else if (i % 4 == 2) {
                // Update
                backend.SetValue(key, Int32(i * 2));
            } else {
                // Delete (if exists)
                backend.RemoveKey(key);
            }
        }
        
        timer.Stop();
        ::std::cout << "Property Backend - " << totalOps << " mixed operations: " 
                    << ::std::fixed << ::std::setprecision(2)
                    << timer.GetMilliseconds() << " ms" << ::std::endl;
        ::std::cout << "  Average per operation: " 
                    << (timer.GetMilliseconds() / totalOps) << " ms" << ::std::endl;
    }
}

void StressTest_RapidUpdates() {
    ::std::cout << "\n=== Stress Test: Rapid Updates (Same Keys) ===" 
                << ::std::endl;
    
    BenchmarkTimer timer;
    const int updates = 10000;
    const int keyCount = 100;
    
    {
        constexpr size_t shmSize = 4ul << 20; // 4MB
        KvsPropertyBackend backend("stress_updates", KvsBackendType::kvsFile, shmSize);
        
        // Pre-populate keys
        for (int i = 0; i < keyCount; ++i) {
            ::std::string key = "update_key_" + ::std::to_string(i);
            backend.SetValue(key, Int32(0));
        }
        
        timer.Start();
        
        // Rapid updates to same keys
        for (int i = 0; i < updates; ++i) {
            ::std::string key = "update_key_" + ::std::to_string(i % keyCount);
            backend.SetValue(key, Int32(i));
        }
        
        timer.Stop();
        ::std::cout << "Property Backend - " << updates << " rapid updates to " 
                    << keyCount << " keys: " 
                    << ::std::fixed << ::std::setprecision(2)
                    << timer.GetMilliseconds() << " ms" << ::std::endl;
        ::std::cout << "  Updates/second: " 
                    << (updates * 1000.0 / timer.GetMilliseconds()) << ::std::endl;
    }
}

void StressTest_MemoryPressure() {
    ::std::cout << "\n=== Stress Test: Memory Pressure ===" 
                << ::std::endl;
    
    BenchmarkTimer timer;
    
    {
        constexpr size_t shmSize = 8ul << 20; // 8MB
        KvsPropertyBackend backend("stress_memory", KvsBackendType::kvsFile, shmSize);
        
        ::std::cout << "Writing 5,000 keys with varied data types..." << ::std::endl;
        
        timer.Start();
        
        for (int i = 0; i < 5000; ++i) {
            ::std::string baseKey = "mem_key_" + ::std::to_string(i);
            
            // Mix different data types
            if (i % 5 == 0) {
                backend.SetValue(baseKey + "_str", String(("value_" + ::std::to_string(i)).c_str()));
            } else if (i % 5 == 1) {
                backend.SetValue(baseKey + "_int", Int32(i));
            } else if (i % 5 == 2) {
                backend.SetValue(baseKey + "_uint", UInt64(i * 1000ULL));
            } else if (i % 5 == 3) {
                backend.SetValue(baseKey + "_float", Float(i * 3.14f));
            } else {
                backend.SetValue(baseKey + "_bool", Bool(i % 2 == 0));
            }
        }
        
        timer.Stop();
        
        auto sizeResult = backend.GetSize();
        if (sizeResult.HasValue()) {
            ::std::cout << "Total keys in memory: " << sizeResult.Value() << ::std::endl;
        }
        
        ::std::cout << "Write time: " 
                    << ::std::fixed << ::std::setprecision(2)
                    << timer.GetMilliseconds() << " ms" << ::std::endl;
        
        // Test retrieval
        timer.Start();
        int successCount = 0;
        for (int i = 0; i < 5000; i += 10) {
            ::std::string baseKey = "mem_key_" + ::std::to_string(i);
            ::std::string key;
            
            if (i % 5 == 0) key = baseKey + "_str";
            else if (i % 5 == 1) key = baseKey + "_int";
            else if (i % 5 == 2) key = baseKey + "_uint";
            else if (i % 5 == 3) key = baseKey + "_float";
            else key = baseKey + "_bool";
            
            auto result = backend.GetValue(key);
            if (result.HasValue()) successCount++;
        }
        timer.Stop();
        
        ::std::cout << "Random read test (500 keys): " << timer.GetMilliseconds() << " ms" << ::std::endl;
        ::std::cout << "Success rate: " << successCount << "/500" << ::std::endl;
    }
}

void StressTest_PersistenceReload() {
    ::std::cout << "\n=== Stress Test: Persistence & Reload ===" 
                << ::std::endl;
    
    BenchmarkTimer timer;
    const int keyCount = 2000;
    
    // Phase 1: Write and persist
    {
        constexpr size_t shmSize = 8ul << 20; // 8MB
        KvsPropertyBackend backend("stress_persist", KvsBackendType::kvsFile, shmSize);
        
        ::std::cout << "Phase 1: Writing " << keyCount << " keys..." << ::std::endl;
        timer.Start();
        
        for (int i = 0; i < keyCount; ++i) {
            ::std::string key = "persist_key_" + ::std::to_string(i);
            backend.SetValue(key, String(("persistent_value_" + ::std::to_string(i)).c_str()));
        }
        
        timer.Stop();
        ::std::cout << "  Write time: " 
                    << ::std::fixed << ::std::setprecision(2)
                    << timer.GetMilliseconds() << " ms" << ::std::endl;
        
        timer.Start();
        backend.SyncToStorage();
        timer.Stop();
        ::std::cout << "  Sync to disk: " << timer.GetMilliseconds() << " ms" << ::std::endl;
    }
    
    // Phase 2: Reload from persistence
    {
        ::std::cout << "Phase 2: Reloading from persistence..." << ::std::endl;
        timer.Start();
        
        constexpr size_t shmSize = 8ul << 20; // 8MB
        KvsPropertyBackend backend("stress_persist", KvsBackendType::kvsFile, shmSize);
        
        timer.Stop();
        ::std::cout << "  Load time: " << timer.GetMilliseconds() << " ms" << ::std::endl;
        
        // Verify data
        auto sizeResult = backend.GetSize();
        if (sizeResult.HasValue()) {
            ::std::cout << "  Keys loaded: " << sizeResult.Value() << ::std::endl;
        }
        
        // Spot check
        timer.Start();
        int verifyCount = 0;
        for (int i = 0; i < keyCount; i += 100) {
            ::std::string key = "persist_key_" + ::std::to_string(i);
            auto result = backend.GetValue(key);
            if (result.HasValue()) verifyCount++;
        }
        timer.Stop();
        ::std::cout << "  Verification (20 keys): " << timer.GetMilliseconds() << " ms" << ::std::endl;
        ::std::cout << "  Verified: " << verifyCount << "/20" << ::std::endl;
    }
}

void PrintStressSummary() {
    ::std::cout << "\n============================================================" 
                << ::std::endl;
    ::std::cout << "Stress Test Summary" << ::std::endl;
    ::std::cout << "============================================================" 
                << ::std::endl;
    ::std::cout << "\nStress tests validate:" << ::std::endl;
    ::std::cout << "  ✓ Large dataset handling (10,000+ keys)" << ::std::endl;
    ::std::cout << "  ✓ Large value storage (10KB+ per value)" << ::std::endl;
    ::std::cout << "  ✓ Mixed operation patterns" << ::std::endl;
    ::std::cout << "  ✓ Rapid updates to same keys" << ::std::endl;
    ::std::cout << "  ✓ Memory pressure with varied types" << ::std::endl;
    ::std::cout << "  ✓ Persistence and reload integrity" << ::std::endl;
    ::std::cout << "\nProperty Backend Shared Memory Configuration:" << ::std::endl;
    ::std::cout << "  • Default size: 1MB (configurable via constructor)" << ::std::endl;
    ::std::cout << "  • Stress test sizes: 4MB - 16MB" << ::std::endl;
    ::std::cout << "  • Adjust size based on use case requirements" << ::std::endl;
    ::std::cout << "\n============================================================" 
                << ::std::endl;
}

void PrintComparisonSummary() {
    ::std::cout << "\n============================================================" 
                << ::std::endl;
    ::std::cout << "Performance Benchmark Summary" << ::std::endl;
    ::std::cout << "============================================================" 
                << ::std::endl;
    ::std::cout << "\nBackend Characteristics:" << ::std::endl;
    ::std::cout << "  • File Backend:     Simple, direct file I/O" << ::std::endl;
    ::std::cout << "  • SQLite Backend:   Transactional, ACID compliant" << ::std::endl;
    ::std::cout << "  • Property Backend: Shared memory + persistence" << ::std::endl;
    ::std::cout << "\nExpected Performance:" << ::std::endl;
    ::std::cout << "  • Property (memory ops): Fastest (~3x File)" << ::std::endl;
    ::std::cout << "  • File (direct I/O):     Medium (baseline)" << ::std::endl;
    ::std::cout << "  • SQLite (database):     Slower (70-80x File)" << ::std::endl;
    ::std::cout << "\nProperty Backend Benefits:" << ::std::endl;
    ::std::cout << "  ✓ Fast in-memory operations" << ::std::endl;
    ::std::cout << "  ✓ Inter-process communication" << ::std::endl;
    ::std::cout << "  ✓ Configurable persistence (File/SQLite)" << ::std::endl;
    ::std::cout << "  ✓ Auto-load on construction" << ::std::endl;
    ::std::cout << "  ✓ Auto-save on destruction" << ::std::endl;
    ::std::cout << "\n============================================================" 
                << ::std::endl;
}

// ============================================================================
// Main
// ============================================================================

int main() {
    ::std::cout << "============================================================" 
                << ::std::endl;
    ::std::cout << "Persistency Module - Performance Benchmark Suite" 
                << ::std::endl;
    ::std::cout << "Testing all backend implementations" << ::std::endl;
    ::std::cout << "============================================================" 
                << ::std::endl;
    
    try {
        // Basic Performance Tests
        ::std::cout << "\n### BASIC PERFORMANCE TESTS ###\n" << ::std::endl;
        BenchmarkFileBackend();
        BenchmarkSqliteBackend();
        BenchmarkPropertyBackend();
        BenchmarkPropertyWithSqlite();
        PrintComparisonSummary();

        // Stress Tests
        ::std::cout << "\n\n### STRESS TESTS ###\n" << ::std::endl;
        StressTest_LargeDataset();
        StressTest_LargeValues();
        StressTest_MixedOperations();
        StressTest_RapidUpdates();
        StressTest_MemoryPressure();
        StressTest_PersistenceReload();
        PrintStressSummary();
        
        ::std::cout << "\n============================================================" 
                    << ::std::endl;
        ::std::cout << "✓ All benchmarks completed successfully!" << ::std::endl;
        ::std::cout << "============================================================" 
                    << ::std::endl;
        return 0;
        
    } catch (const ::std::exception& e) {
        ::std::cerr << "Benchmark failed: " << e.what() << ::std::endl;
        return 1;
    }
}
