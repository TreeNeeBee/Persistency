/**
 * @file property_backend_demo.cpp
 * @brief Property Backend demonstration and performance comparison
 * 
 * This example demonstrates:
 * 1. Property backend with File persistence
 * 2. Property backend with SQLite persistence
 * 3. Performance comparison between backends
 * 4. Inter-process communication via shared memory
 */

#include <iostream>
#include <chrono>
#include "CKvsPropertyBackend.hpp"
#include "CKvsFileBackend.hpp"
#include "CKvsSqliteBackend.hpp"

using namespace lap::per;
using namespace lap::per::util;
using namespace lap::core;

// Helper to measure execution time
template<typename Func>
double measureTime(Func func, const char* description) {
    auto start = std::chrono::steady_clock::now();
    func();
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << description << ": " << elapsed.count() << " ms\n";
    return elapsed.count();
}

void demonstrateBasicUsage() {
    std::cout << "\n=== Basic Property Backend Usage ===\n";
    
    // Create Property backend with File persistence
    KvsPropertyBackend propBackend("/demo/property_kvs", KvsBackendType::kvsFile);
    
    if (!propBackend.available()) {
        std::cerr << "Failed to initialize Property backend\n";
        return;
    }
    
    // Set various types of values (in shared memory)
    propBackend.SetValue("app.name", String("LightAP"));
    propBackend.SetValue("app.version", UInt32(7));
    propBackend.SetValue("app.debug", Bool(true));
    propBackend.SetValue("app.max_threads", Int32(16));
    propBackend.SetValue("app.timeout", Float(30.5f));
    
    std::cout << "Set 5 key-value pairs in shared memory\n";
    
    // Read values (from shared memory - very fast)
    auto nameResult = propBackend.GetValue("app.name");
    if (nameResult.HasValue()) {
        std::cout << "app.name = " << get<String>(nameResult.Value()) << "\n";
    }
    
    auto versionResult = propBackend.GetValue("app.version");
    if (versionResult.HasValue()) {
        std::cout << "app.version = " << get<UInt32>(versionResult.Value()) << "\n";
    }
    
    // Get key count
    auto countResult = propBackend.GetKeyCount();
    if (countResult.HasValue()) {
        std::cout << "Total keys in shared memory: " << countResult.Value() << "\n";
    }
    
    // Sync to persistent storage (File backend)
    std::cout << "Syncing to persistent storage...\n";
    auto syncResult = propBackend.SyncToStorage();
    if (syncResult.HasValue()) {
        std::cout << "Successfully synced to File backend\n";
    }
}

void demonstrateSqlitePersistence() {
    std::cout << "\n=== Property Backend with SQLite Persistence ===\n";
    
    // Create Property backend with SQLite persistence
    KvsPropertyBackend propBackend("/demo/property_sqlite", KvsBackendType::kvsSqlite);
    
    if (!propBackend.available()) {
        std::cerr << "Failed to initialize Property backend with SQLite\n";
        return;
    }
    
    // Set values in shared memory
    propBackend.SetValue("db.host", String("localhost"));
    propBackend.SetValue("db.port", UInt16(5432));
    propBackend.SetValue("db.timeout", Int32(30));
    propBackend.SetValue("db.ssl", Bool(true));
    
    std::cout << "Set 4 key-value pairs with SQLite persistence\n";
    
    // Sync to SQLite database
    auto syncResult = propBackend.SyncToStorage();
    if (syncResult.HasValue()) {
        std::cout << "Successfully synced to SQLite backend\n";
    }
    
    // Get all keys
    auto keysResult = propBackend.GetAllKeys();
    if (keysResult.HasValue()) {
        std::cout << "All keys: ";
        for (const auto& key : keysResult.Value()) {
            std::cout << key << " ";
        }
        std::cout << "\n";
    }
}

void performanceComparison() {
    std::cout << "\n=== Performance Comparison (1000 operations) ===\n";
    
    const int NUM_OPS = 1000;
    
    // File Backend
    {
        std::cout << "\nFile Backend:\n";
        KvsFileBackend fileBackend("/demo/perf_file");
        
        double writeTime = measureTime([&]() {
            for (int i = 0; i < NUM_OPS; i++) {
                fileBackend.SetValue("key_" + std::to_string(i), Int32(i));
            }
        }, "  Write 1000 keys");
        
        double readTime = measureTime([&]() {
            for (int i = 0; i < NUM_OPS; i++) {
                fileBackend.GetValue("key_" + std::to_string(i));
            }
        }, "  Read 1000 keys");
        
        std::cout << "  Total: " << (writeTime + readTime) << " ms\n";
    }
    
    // SQLite Backend
    {
        std::cout << "\nSQLite Backend:\n";
        KvsSqliteBackend sqliteBackend("/demo/perf_sqlite");
        
        double writeTime = measureTime([&]() {
            for (int i = 0; i < NUM_OPS; i++) {
                sqliteBackend.SetValue("key_" + std::to_string(i), Int32(i));
            }
        }, "  Write 1000 keys");
        
        double readTime = measureTime([&]() {
            for (int i = 0; i < NUM_OPS; i++) {
                sqliteBackend.GetValue("key_" + std::to_string(i));
            }
        }, "  Read 1000 keys");
        
        std::cout << "  Total: " << (writeTime + readTime) << " ms\n";
    }
    
    // Property Backend (with File persistence)
    {
        std::cout << "\nProperty Backend (shared memory + File):\n";
        KvsPropertyBackend propBackend("/demo/perf_property", KvsBackendType::kvsFile);
        
        double writeTime = measureTime([&]() {
            for (int i = 0; i < NUM_OPS; i++) {
                propBackend.SetValue("key_" + std::to_string(i), Int32(i));
            }
        }, "  Write 1000 keys");
        
        double readTime = measureTime([&]() {
            for (int i = 0; i < NUM_OPS; i++) {
                propBackend.GetValue("key_" + std::to_string(i));
            }
        }, "  Read 1000 keys");
        
        double syncTime = measureTime([&]() {
            propBackend.SyncToStorage();
        }, "  Sync to persistence");
        
        std::cout << "  Total (incl. sync): " << (writeTime + readTime + syncTime) << " ms\n";
        std::cout << "  In-memory ops only: " << (writeTime + readTime) << " ms\n";
    }
}

void demonstrateLoadAndReload() {
    std::cout << "\n=== Load/Reload from Persistence ===\n";
    
    // Phase 1: Create and populate
    {
        std::cout << "Phase 1: Creating Property backend and adding data...\n";
        KvsPropertyBackend propBackend("/demo/reload_test", KvsBackendType::kvsFile);
        
        propBackend.SetValue("config.server", String("example.com"));
        propBackend.SetValue("config.port", UInt16(8080));
        propBackend.SetValue("config.retries", Int32(3));
        
        // Sync to File backend
        propBackend.SyncToStorage();
        std::cout << "Data synced to File backend\n";
    }
    // Property backend destroyed, shared memory may persist
    
    // Phase 2: Create new instance - should load from File backend
    {
        std::cout << "\nPhase 2: Creating new Property backend instance...\n";
        KvsPropertyBackend propBackend("/demo/reload_test", KvsBackendType::kvsFile);
        
        // Data should be loaded from File backend to shared memory
        auto serverResult = propBackend.GetValue("config.server");
        auto portResult = propBackend.GetValue("config.port");
        auto retriesResult = propBackend.GetValue("config.retries");
        
        if (serverResult.HasValue() && portResult.HasValue() && retriesResult.HasValue()) {
            std::cout << "Successfully reloaded from persistence:\n";
            std::cout << "  config.server = " << get<String>(serverResult.Value()) << "\n";
            std::cout << "  config.port = " << get<UInt16>(portResult.Value()) << "\n";
            std::cout << "  config.retries = " << get<Int32>(retriesResult.Value()) << "\n";
        } else {
            std::cout << "Failed to reload data\n";
        }
    }
}

int main() {
    std::cout << "=================================================\n";
    std::cout << "Property Backend Demonstration\n";
    std::cout << "=================================================\n";
    
    try {
        demonstrateBasicUsage();
        demonstrateSqlitePersistence();
        performanceComparison();
        demonstrateLoadAndReload();
        
        std::cout << "\n=== All demonstrations completed successfully ===\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
