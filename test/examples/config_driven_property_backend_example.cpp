/**
 * @file config_driven_property_backend_example.cpp
 * @brief Example demonstrating config-driven Property Backend initialization
 * 
 * This example shows how the Property backend automatically loads its configuration
 * (shared memory size, persistence type) from the persistency module config.
 */

#include <iostream>
#include <lap/core/CConfig.hpp>
#include "CDataType.hpp"
#include "CPersistencyManager.hpp"

using namespace lap::per;
using namespace lap::core;

void print_section(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << title << std::endl;
    std::cout << std::string(60, '=') << std::endl;
}

void demo_default_config() {
    print_section("Example 1: Default Configuration (No Config File)");
    
    auto& perMgr = CPersistencyManager::getInstance();
    
    // Load config (will use defaults if no config file)
    auto configResult = perMgr.loadPersistencyConfig();
    if (configResult.HasValue()) {
        const auto& config = configResult.Value();
        std::cout << "KVS Backend Type: " << config.kvs.backendType << std::endl;
        std::cout << "Property SHM Size: " << (config.kvs.propertyBackendShmSize / 1024) << " KB" << std::endl;
        std::cout << "Property Persistence: " << config.kvs.propertyBackendPersistence << std::endl;
    }
}

void demo_json_config() {
    print_section("Example 2: Load from JSON Configuration");
    
    // Initialize ConfigManager with our example config
    auto& configMgr = ConfigManager::getInstance();
    auto initResult = configMgr.initialize("persistency_full_config.json", false);
    
    if (!initResult.HasValue()) {
        std::cout << "Note: Config file not found, using programmatic config instead" << std::endl;
        
        // Create config programmatically
        nlohmann::json persistencyConfig;
        persistencyConfig["kvs"]["backendType"] = "property";
        persistencyConfig["kvs"]["propertyBackendShmSize"] = 8ul << 20;  // 8MB
        persistencyConfig["kvs"]["propertyBackendPersistence"] = "file";
        
        configMgr.setModuleConfigJson("persistency", persistencyConfig);
    }
    
    // Load persistency config
    auto& perMgr = CPersistencyManager::getInstance();
    auto configResult = perMgr.loadPersistencyConfig();
    
    if (configResult.HasValue()) {
        const auto& config = configResult.Value();
        std::cout << "\nLoaded Configuration:" << std::endl;
        std::cout << "  Central Storage URI: " << config.centralStorageURI << std::endl;
        std::cout << "  Replica Count: " << config.replicaCount << std::endl;
        std::cout << "  Min Valid Replicas: " << config.minValidReplicas << std::endl;
        std::cout << "  Checksum Type: " << config.checksumType << std::endl;
        std::cout << "\nKVS Configuration:" << std::endl;
        std::cout << "  Backend Type: " << config.kvs.backendType << std::endl;
        std::cout << "  Property SHM Size: " << (config.kvs.propertyBackendShmSize / (1024*1024)) << " MB" << std::endl;
        std::cout << "  Property Persistence: " << config.kvs.propertyBackendPersistence << std::endl;
    }
}

void demo_update_config() {
    print_section("Example 3: Update Configuration at Runtime");
    
    auto& perMgr = CPersistencyManager::getInstance();
    
    // Load current config
    auto configResult = perMgr.loadPersistencyConfig();
    if (configResult.HasValue()) {
        auto config = configResult.Value();
        
        std::cout << "Original Config:" << std::endl;
        std::cout << "  SHM Size: " << (config.kvs.propertyBackendShmSize / (1024*1024)) << " MB" << std::endl;
        std::cout << "  Persistence: " << config.kvs.propertyBackendPersistence << std::endl;
        
        // Modify config
        config.kvs.propertyBackendShmSize = 32ul << 20;  // Change to 32MB
        config.kvs.propertyBackendPersistence = "sqlite";  // Change to SQLite
        
        // Update
        auto updateResult = perMgr.updateConfig(config);
        if (updateResult.HasValue()) {
            std::cout << "\nUpdated Config:" << std::endl;
            std::cout << "  SHM Size: " << (config.kvs.propertyBackendShmSize / (1024*1024)) << " MB" << std::endl;
            std::cout << "  Persistence: " << config.kvs.propertyBackendPersistence << std::endl;
            std::cout << "  ✓ Configuration updated successfully!" << std::endl;
        }
    }
}

void demo_size_examples() {
    print_section("Example 4: Common Configuration Patterns");
    
    std::cout << "\n┌──────────────────────┬──────────────┬─────────────────┐" << std::endl;
    std::cout << "│ Use Case             │ SHM Size     │ Persistence     │" << std::endl;
    std::cout << "├──────────────────────┼──────────────┼─────────────────┤" << std::endl;
    std::cout << "│ IoT Device           │ 512KB - 1MB  │ file            │" << std::endl;
    std::cout << "│ Edge Gateway         │ 4MB - 8MB    │ file or sqlite  │" << std::endl;
    std::cout << "│ Server Application   │ 16MB - 32MB  │ sqlite          │" << std::endl;
    std::cout << "│ High-Performance     │ 64MB+        │ sqlite (WAL)    │" << std::endl;
    std::cout << "└──────────────────────┴──────────────┴─────────────────┘" << std::endl;
    
    std::cout << "\nJSON Configuration Examples:" << std::endl;
    std::cout << "\n// IoT Device (512KB, File)" << std::endl;
    std::cout << R"({
  "kvs": {
    "backendType": "property",
    "propertyBackendShmSize": 524288,
    "propertyBackendPersistence": "file"
  }
})" << std::endl;
    
    std::cout << "\n// Server (16MB, SQLite)" << std::endl;
    std::cout << R"({
  "kvs": {
    "backendType": "property",
    "propertyBackendShmSize": 16777216,
    "propertyBackendPersistence": "sqlite"
  }
})" << std::endl;
}

void print_config_tips() {
    print_section("Configuration Tips & Best Practices");
    
    std::cout << "\n📋 Configuration File Location:" << std::endl;
    std::cout << "   - Default: config.json in working directory" << std::endl;
    std::cout << "   - Custom: Pass path to ConfigManager::initialize()" << std::endl;
    
    std::cout << "\n🔧 Shared Memory Size Guidelines:" << std::endl;
    std::cout << "   • Calculate: (expected_keys × avg_size) × 1.5 (safety margin)" << std::endl;
    std::cout << "   • Monitor: Check logs for boost::interprocess::bad_alloc" << std::endl;
    std::cout << "   • Adjust: Increase if allocation failures occur" << std::endl;
    
    std::cout << "\n💾 Persistence Backend Selection:" << std::endl;
    std::cout << "   • file: Simple, fast, single-process" << std::endl;
    std::cout << "   • sqlite: ACID, multi-process, query support" << std::endl;
    
    std::cout << "\n⚡ Performance Considerations:" << std::endl;
    std::cout << "   • In-memory ops: ~0.15ms (Property backend)" << std::endl;
    std::cout << "   • File persistence: ~2-5ms per sync" << std::endl;
    std::cout << "   • SQLite persistence: ~40-120ms per sync" << std::endl;
    
    std::cout << "\n🔄 Runtime Updates:" << std::endl;
    std::cout << "   • Config changes apply to NEW instances only" << std::endl;
    std::cout << "   • Existing backends keep their initial config" << std::endl;
    std::cout << "   • Restart required for full config reload" << std::endl;
}

int main() {
    std::cout << "============================================================" << std::endl;
    std::cout << "Property Backend - Configuration-Driven Setup" << std::endl;
    std::cout << "============================================================" << std::endl;
    
    try {
        demo_default_config();
        demo_json_config();
        demo_update_config();
        demo_size_examples();
        print_config_tips();
        
        std::cout << "\n============================================================" << std::endl;
        std::cout << "✓ All examples completed successfully!" << std::endl;
        std::cout << "============================================================" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
