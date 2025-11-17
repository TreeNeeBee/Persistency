/**
 * @file multi_backend_usage_example.cpp
 * @brief Example demonstrating simultaneous usage of multiple KVS backends
 * @date 2025-11-17
 * 
 * This example shows how to use different KVS backends concurrently in the same application.
 * Each backend serves a different purpose:
 * - File Backend: Configuration storage (simple key-value pairs)
 * - SQLite Backend: User data (transactional, ACID-compliant)
 * - Property Backend: Runtime state/cache (fast in-memory operations with persistence)
 */

#include <iostream>
#include <string>
#include <lap/core/CCore.hpp>
#include "CKvsFileBackend.hpp"
#include "CKvsSqliteBackend.hpp"
#include "CKvsPropertyBackend.hpp"
#include "CDataType.hpp"

using namespace lap::core;
using namespace lap::per;

void PrintSeparator(const std::string& title) {
    std::cout << "\n========================================" << std::endl;
    std::cout << title << std::endl;
    std::cout << "========================================" << std::endl;
}

void DemoFileBackend(KvsFileBackend& fileBackend) {
    PrintSeparator("File Backend - Application Configuration");
    
    // Store application configuration
    fileBackend.SetValue("app.version", String("1.2.3"));
    fileBackend.SetValue("app.max_connections", Int32(100));
    fileBackend.SetValue("app.enable_logging", Bool(true));
    fileBackend.SetValue("app.timeout_ms", UInt32(5000));
    
    std::cout << "✓ Configuration stored in File Backend" << std::endl;
    
    // Read configuration
    auto version = fileBackend.GetValue("app.version");
    auto maxConn = fileBackend.GetValue("app.max_connections");
    auto logging = fileBackend.GetValue("app.enable_logging");
    
    if (version.HasValue() && maxConn.HasValue() && logging.HasValue()) {
        auto versionStr = std::get_if<String>(&version.Value());
        auto maxConnInt = std::get_if<Int32>(&maxConn.Value());
        auto loggingBool = std::get_if<Bool>(&logging.Value());
        
        std::cout << "\nConfiguration:" << std::endl;
        std::cout << "  Version: " << (versionStr ? versionStr->data() : "N/A") << std::endl;
        std::cout << "  Max Connections: " << (maxConnInt ? *maxConnInt : 0) << std::endl;
        std::cout << "  Logging: " << (loggingBool && *loggingBool ? "enabled" : "disabled") << std::endl;
    }
    
    auto countResult = fileBackend.GetKeyCount();
    std::cout << "\nTotal config keys: " << (countResult.HasValue() ? countResult.Value() : 0) << std::endl;
}

void DemoSqliteBackend(KvsSqliteBackend& sqliteBackend) {
    PrintSeparator("SQLite Backend - User Data (Transactional)");
    
    // Store user data with ACID guarantees
    std::cout << "Storing user data..." << std::endl;
    
    // User 1
    sqliteBackend.SetValue("user.1.name", String("Alice"));
    sqliteBackend.SetValue("user.1.age", Int32(28));
    sqliteBackend.SetValue("user.1.score", Float(95.5f));
    sqliteBackend.SetValue("user.1.active", Bool(true));
    
    // User 2
    sqliteBackend.SetValue("user.2.name", String("Bob"));
    sqliteBackend.SetValue("user.2.age", Int32(34));
    sqliteBackend.SetValue("user.2.score", Float(87.3f));
    sqliteBackend.SetValue("user.2.active", Bool(false));
    
    // User 3
    sqliteBackend.SetValue("user.3.name", String("Charlie"));
    sqliteBackend.SetValue("user.3.age", Int32(42));
    sqliteBackend.SetValue("user.3.score", Float(92.1f));
    sqliteBackend.SetValue("user.3.active", Bool(true));
    
    std::cout << "✓ User data stored with transactional guarantees" << std::endl;
    
    // Query user data
    std::cout << "\nUser Records:" << std::endl;
    for (int i = 1; i <= 3; ++i) {
        std::string nameKey = "user." + std::to_string(i) + ".name";
        std::string ageKey = "user." + std::to_string(i) + ".age";
        std::string scoreKey = "user." + std::to_string(i) + ".score";
        std::string activeKey = "user." + std::to_string(i) + ".active";
        
        auto name = sqliteBackend.GetValue(nameKey);
        auto age = sqliteBackend.GetValue(ageKey);
        auto score = sqliteBackend.GetValue(scoreKey);
        auto active = sqliteBackend.GetValue(activeKey);
        
        if (name.HasValue() && age.HasValue()) {
            auto nameStr = std::get_if<String>(&name.Value());
            auto ageInt = std::get_if<Int32>(&age.Value());
            auto scoreFloat = std::get_if<Float>(&score.Value());
            auto activeBool = std::get_if<Bool>(&active.Value());
            
            std::cout << "  User " << i << ": " 
                      << (nameStr ? nameStr->data() : "N/A")
                      << ", Age: " << (ageInt ? *ageInt : 0)
                      << ", Score: " << (scoreFloat ? *scoreFloat : 0.0f)
                      << ", Active: " << (activeBool && *activeBool ? "Yes" : "No")
                      << std::endl;
        }
    }
    
    auto countResult = sqliteBackend.GetKeyCount();
    std::cout << "\nTotal user data keys: " << (countResult.HasValue() ? countResult.Value() : 0) << std::endl;
}

void DemoPropertyBackend(lap::per::util::KvsPropertyBackend& propertyBackend) {
    PrintSeparator("Property Backend - Runtime State/Cache (In-Memory)");
    
    // Store runtime state in shared memory (fast access)
    std::cout << "Storing runtime state in shared memory..." << std::endl;
    
    // Session data
    propertyBackend.SetValue("session.current_user", String("Alice"));
    propertyBackend.SetValue("session.login_time", UInt64(1700178000));
    propertyBackend.SetValue("session.request_count", Int32(42));
    
    // Cache data
    propertyBackend.SetValue("cache.last_query_result", String("SUCCESS"));
    propertyBackend.SetValue("cache.hit_count", Int32(156));
    propertyBackend.SetValue("cache.miss_count", Int32(8));
    
    // Performance metrics
    propertyBackend.SetValue("metrics.avg_response_time", Float(12.5f));
    propertyBackend.SetValue("metrics.requests_per_second", Float(450.2f));
    
    std::cout << "✓ Runtime state stored in shared memory (fast access)" << std::endl;
    
    // Read runtime state
    std::cout << "\nRuntime State:" << std::endl;
    
    auto currentUser = propertyBackend.GetValue("session.current_user");
    auto requestCount = propertyBackend.GetValue("session.request_count");
    auto hitCount = propertyBackend.GetValue("cache.hit_count");
    auto missCount = propertyBackend.GetValue("cache.miss_count");
    auto avgTime = propertyBackend.GetValue("metrics.avg_response_time");
    
    if (currentUser.HasValue()) {
        auto user = std::get_if<String>(&currentUser.Value());
        std::cout << "  Current User: " << (user ? user->data() : "N/A") << std::endl;
    }
    
    if (requestCount.HasValue()) {
        auto count = std::get_if<Int32>(&requestCount.Value());
        std::cout << "  Request Count: " << (count ? *count : 0) << std::endl;
    }
    
    if (hitCount.HasValue() && missCount.HasValue()) {
        auto hits = std::get_if<Int32>(&hitCount.Value());
        auto misses = std::get_if<Int32>(&missCount.Value());
        int totalHits = hits ? *hits : 0;
        int totalMisses = misses ? *misses : 0;
        float hitRate = totalHits + totalMisses > 0 
            ? (totalHits * 100.0f) / (totalHits + totalMisses) 
            : 0.0f;
        std::cout << "  Cache Hit Rate: " << hitRate << "%" << std::endl;
    }
    
    if (avgTime.HasValue()) {
        auto time = std::get_if<Float>(&avgTime.Value());
        std::cout << "  Avg Response Time: " << (time ? *time : 0.0f) << " ms" << std::endl;
    }
    
    auto countResult = propertyBackend.GetKeyCount();
    std::cout << "\nTotal runtime state keys: " << (countResult.HasValue() ? countResult.Value() : 0) << std::endl;
    
    // Demonstrate persistence sync
    std::cout << "\n✓ Syncing runtime state to persistence..." << std::endl;
    propertyBackend.SyncToStorage();
    std::cout << "✓ Runtime state persisted (will be auto-loaded on next restart)" << std::endl;
}

void DemoCrossBackendScenario() {
    PrintSeparator("Cross-Backend Scenario: Data Processing");
    
    std::cout << "\nScenario: Processing user requests with multi-backend coordination\n" << std::endl;
    
    // Setup backends
    KvsFileBackend configBackend("demo_config");
    KvsSqliteBackend userBackend("demo_users");
    constexpr size_t shmSize = 4ul << 20; // 4MB shared memory
    lap::per::util::KvsPropertyBackend cacheBackend("demo_cache", KvsBackendType::kvsFile, shmSize);
    
    // Step 1: Load configuration from File Backend
    std::cout << "Step 1: Loading configuration..." << std::endl;
    configBackend.SetValue("processing.batch_size", Int32(100));
    configBackend.SetValue("processing.timeout_ms", Int32(3000));
    
    auto batchSize = configBackend.GetValue("processing.batch_size");
    auto batchInt = std::get_if<Int32>(batchSize.HasValue() ? &batchSize.Value() : nullptr);
    std::cout << "  Batch size: " << (batchInt ? *batchInt : 0) << std::endl;
    
    // Step 2: Query user data from SQLite Backend
    std::cout << "\nStep 2: Querying user data..." << std::endl;
    userBackend.SetValue("user.alice.id", Int32(1001));
    userBackend.SetValue("user.alice.status", String("active"));
    
    auto userId = userBackend.GetValue("user.alice.id");
    auto userIdInt = std::get_if<Int32>(userId.HasValue() ? &userId.Value() : nullptr);
    std::cout << "  User ID: " << (userIdInt ? *userIdInt : 0) << std::endl;
    
    // Step 3: Cache results in Property Backend
    std::cout << "\nStep 3: Caching results in shared memory..." << std::endl;
    cacheBackend.SetValue("cache.user.alice.last_access", UInt64(1700178000));
    cacheBackend.SetValue("cache.user.alice.access_count", Int32(1));
    
    auto accessCount = cacheBackend.GetValue("cache.user.alice.access_count");
    auto countInt = std::get_if<Int32>(accessCount.HasValue() ? &accessCount.Value() : nullptr);
    std::cout << "  Access count (cached): " << (countInt ? *countInt : 0) << std::endl;
    
    // Step 4: Update metrics across backends
    std::cout << "\nStep 4: Updating metrics..." << std::endl;
    
    // Increment cache counter
    if (countInt) {
        cacheBackend.SetValue("cache.user.alice.access_count", Int32(*countInt + 1));
    }
    
    // Log to SQLite
    userBackend.SetValue("user.alice.last_login", UInt64(1700178000));
    
    std::cout << "  ✓ Metrics updated across backends" << std::endl;
    
    std::cout << "\n✓ Cross-backend coordination completed successfully!" << std::endl;
}

int main() {
    std::cout << "============================================================" << std::endl;
    std::cout << "Multi-Backend Usage Example" << std::endl;
    std::cout << "Demonstrating concurrent use of multiple KVS backends" << std::endl;
    std::cout << "============================================================" << std::endl;
    
    try {
        // Clean up any existing test databases
        std::remove("/tmp/demo_config.db");
        std::remove("/tmp/demo_users.db");
        std::remove("/tmp/demo_cache.db");
        std::remove("/tmp/demo_file_backend.db");
        std::remove("/tmp/demo_sqlite_backend.db");
        std::remove("/tmp/demo_property_backend_file.db");
        
        // Initialize backends for different purposes
        KvsFileBackend fileBackend("demo_file_backend");
        KvsSqliteBackend sqliteBackend("demo_sqlite_backend");
        lap::per::util::KvsPropertyBackend propertyBackend("demo_property_backend", KvsBackendType::kvsFile, 2ul << 20); // 2MB
        
        // Demonstrate each backend's use case
        DemoFileBackend(fileBackend);
        DemoSqliteBackend(sqliteBackend);
        DemoPropertyBackend(propertyBackend);
        
        // Demonstrate cross-backend coordination
        DemoCrossBackendScenario();
        
        // Summary
        PrintSeparator("Summary");
        std::cout << "\nBackend Selection Guide:" << std::endl;
        std::cout << "  • File Backend    → Simple config, lightweight data" << std::endl;
        std::cout << "  • SQLite Backend  → Transactional data, ACID requirements" << std::endl;
        std::cout << "  • Property Backend→ Fast access, shared memory, runtime state" << std::endl;
        std::cout << "\nAll backends can be used simultaneously in the same application!" << std::endl;
        std::cout << "\n✓ Example completed successfully!" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
