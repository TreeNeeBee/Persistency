/**
 * @file property_backend_config_example.cpp
 * @brief Example demonstrating configurable shared memory size in Property Backend
 * 
 * This example shows how to configure the shared memory size based on your
 * application's requirements.
 */

#include <iostream>
#include "CDataType.hpp"
#include "CKvsPropertyBackend.hpp"

using namespace lap::per;
using namespace lap::per::util;
using namespace lap::core;

void demo_default_size() {
    std::cout << "\n=== Example 1: Default Size (1MB) ===" << std::endl;
    
    // Use default 1MB shared memory
    KvsPropertyBackend backend("default_config", KvsBackendType::kvsFile);
    
    // Store some data
    backend.SetValue("app.name", String("LightAP"));
    backend.SetValue("app.version", Int32(1));
    backend.SetValue("app.max_connections", UInt32(1000));
    
    auto sizeResult = backend.GetKeyCount();
    if (sizeResult.HasValue()) {
        std::cout << "Stored " << sizeResult.Value() << " keys with default 1MB" << std::endl;
    }
}

void demo_small_size() {
    std::cout << "\n=== Example 2: Small Size (512KB) ===" << std::endl;
    
    // Use smaller shared memory for resource-constrained environments
    constexpr size_t smallSize = 512ul << 10; // 512KB
    KvsPropertyBackend backend("small_config", KvsBackendType::kvsFile, smallSize);
    
    // Store configuration data
    backend.SetValue("device.id", String("sensor_01"));
    backend.SetValue("device.interval", Int32(100));
    backend.SetValue("device.enabled", Bool(true));
    
    std::cout << "Using 512KB shared memory for lightweight config" << std::endl;
}

void demo_large_size() {
    std::cout << "\n=== Example 3: Large Size (10MB) ===" << std::endl;
    
    // Use larger shared memory for data-intensive applications
    constexpr size_t largeSize = 10ul << 20; // 10MB
    KvsPropertyBackend backend("large_config", KvsBackendType::kvsFile, largeSize);
    
    // Store large dataset
    int count = 0;
    for (int i = 0; i < 5000; ++i) {
        std::string key = "data.item_" + std::to_string(i);
        auto result = backend.SetValue(key, Int32(i * 100));
        if (result.HasValue()) {
            count++;
        }
    }
    
    std::cout << "Stored " << count << " items using 10MB shared memory" << std::endl;
    
    // Sync to persistence
    backend.SyncToStorage();
    std::cout << "Data persisted to disk" << std::endl;
}

void demo_custom_size() {
    std::cout << "\n=== Example 4: Custom Size Calculation ===" << std::endl;
    
    // Calculate size based on expected data
    const int expectedKeys = 1000;
    const int avgKeySize = 50;    // bytes
    const int avgValueSize = 200; // bytes
    const float overhead = 1.5f;  // 50% overhead for metadata
    
    size_t calculatedSize = static_cast<size_t>(
        expectedKeys * (avgKeySize + avgValueSize) * overhead
    );
    
    // Round up to nearest MB
    size_t sizeInMB = (calculatedSize / (1024 * 1024)) + 1;
    size_t customSize = sizeInMB << 20; // Convert MB to bytes
    
    std::cout << "Calculated size for " << expectedKeys << " keys: " 
              << sizeInMB << "MB" << std::endl;
    
    KvsPropertyBackend backend("custom_config", KvsBackendType::kvsFile, customSize);
    
    // Use the backend
    for (int i = 0; i < expectedKeys; ++i) {
        std::string key = "metric_" + std::to_string(i);
        std::string value = "value_data_" + std::to_string(i * 10);
        backend.SetValue(key, String(value.c_str()));
    }
    
    auto keyCount = backend.GetKeyCount();
    if (keyCount.HasValue()) {
        std::cout << "Successfully stored " << keyCount.Value() << " keys" << std::endl;
    }
}

void demo_size_recommendations() {
    std::cout << "\n=== Shared Memory Size Recommendations ===" << std::endl;
    std::cout << "\n┌─────────────────────┬──────────────┬────────────────────────┐" << std::endl;
    std::cout << "│ Use Case            │ Recommended  │ Example                │" << std::endl;
    std::cout << "├─────────────────────┼──────────────┼────────────────────────┤" << std::endl;
    std::cout << "│ Simple config       │ 512KB - 1MB  │ Device settings        │" << std::endl;
    std::cout << "│ Application config  │ 1MB - 4MB    │ Service parameters     │" << std::endl;
    std::cout << "│ Runtime data        │ 4MB - 16MB   │ Telemetry, metrics     │" << std::endl;
    std::cout << "│ Large datasets      │ 16MB - 64MB  │ Caching, temp storage  │" << std::endl;
    std::cout << "└─────────────────────┴──────────────┴────────────────────────┘" << std::endl;
    
    std::cout << "\nConfiguration tips:" << std::endl;
    std::cout << "  • Default (1MB) suitable for most applications" << std::endl;
    std::cout << "  • Increase size if you see boost::interprocess::bad_alloc" << std::endl;
    std::cout << "  • Consider memory constraints on embedded systems" << std::endl;
    std::cout << "  • Size is per-instance (each backend has its own)" << std::endl;
}

int main() {
    std::cout << "============================================================" << std::endl;
    std::cout << "Property Backend - Configurable Shared Memory Size" << std::endl;
    std::cout << "============================================================" << std::endl;
    
    try {
        demo_default_size();
        demo_small_size();
        demo_large_size();
        demo_custom_size();
        demo_size_recommendations();
        
        std::cout << "\n============================================================" << std::endl;
        std::cout << "✓ All examples completed successfully!" << std::endl;
        std::cout << "============================================================" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
