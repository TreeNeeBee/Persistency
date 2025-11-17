/**
 * @file backend_comparison_example.cpp
 * @brief Simple example comparing all three KVS backends
 * @details Shows basic usage of File, SQLite, and Property backends
 */

#include "CKvsFileBackend.hpp"
#include "CKvsSqliteBackend.hpp"
#include "CKvsPropertyBackend.hpp"
#include <iostream>
#include <iomanip>

using namespace lap::per;
using namespace lap::per::util;
using namespace lap::core;

void PrintSeparator() {
    std::cout << std::string(60, '=') << std::endl;
}

void PrintTitle(const char* title) {
    PrintSeparator();
    std::cout << "  " << title << std::endl;
    PrintSeparator();
}

// ============================================================================
// File Backend Example
// ============================================================================

void DemoFileBackend() {
    PrintTitle("File Backend - Simple and Fast");
    
    // Create File backend
    KvsFileBackend fileBackend("example_file");
    
    // Store different data types
    std::cout << "Storing configuration values..." << std::endl;
    fileBackend.SetValue("app.name", String("LightAP"));
    fileBackend.SetValue("app.version", UInt32(1));
    fileBackend.SetValue("app.debug", Bool(false));
    fileBackend.SetValue("app.timeout", Float(30.0f));
    
    // Retrieve values
    std::cout << "\nRetrieving values:" << std::endl;
    
    auto nameResult = fileBackend.GetValue("app.name");
    if (nameResult.HasValue()) {
        auto name = std::get_if<String>(&nameResult.Value());
        std::cout << "  app.name = " << (name ? name->data() : "N/A") << std::endl;
    }
    
    auto versionResult = fileBackend.GetValue("app.version");
    if (versionResult.HasValue()) {
        auto version = std::get_if<UInt32>(&versionResult.Value());
        std::cout << "  app.version = " << (version ? *version : 0) << std::endl;
    }
    
    // List all keys
    auto keysResult = fileBackend.GetAllKeys();
    if (keysResult.HasValue()) {
        std::cout << "\nTotal keys: " << keysResult.Value().size() << std::endl;
    }
    
    std::cout << "\n✓ File Backend: Best for simple, fast file-based storage\n" << std::endl;
}

// ============================================================================
// SQLite Backend Example
// ============================================================================

void DemoSqliteBackend() {
    PrintTitle("SQLite Backend - Transactional and ACID");
    
    // Create SQLite backend
    KvsSqliteBackend sqliteBackend("example_sqlite");
    
    // Store database configuration
    std::cout << "Storing database configuration..." << std::endl;
    sqliteBackend.SetValue("db.host", String("localhost"));
    sqliteBackend.SetValue("db.port", UInt16(5432));
    sqliteBackend.SetValue("db.name", String("production"));
    sqliteBackend.SetValue("db.connections", Int32(100));
    sqliteBackend.SetValue("db.ssl", Bool(true));
    
    // Retrieve values
    std::cout << "\nRetrieving database config:" << std::endl;
    
    auto hostResult = sqliteBackend.GetValue("db.host");
    if (hostResult.HasValue()) {
        auto host = std::get_if<String>(&hostResult.Value());
        std::cout << "  db.host = " << (host ? host->data() : "N/A") << std::endl;
    }
    
    auto portResult = sqliteBackend.GetValue("db.port");
    if (portResult.HasValue()) {
        auto port = std::get_if<UInt16>(&portResult.Value());
        std::cout << "  db.port = " << (port ? *port : 0) << std::endl;
    }
    
    auto connResult = sqliteBackend.GetValue("db.connections");
    if (connResult.HasValue()) {
        auto conn = std::get_if<Int32>(&connResult.Value());
        std::cout << "  db.connections = " << (conn ? *conn : 0) << std::endl;
    }
    
    // Check if key exists
    auto existsResult = sqliteBackend.KeyExists("db.ssl");
    if (existsResult.HasValue()) {
        std::cout << "\ndb.ssl exists: " << (existsResult.Value() ? "yes" : "no") << std::endl;
    }
    
    std::cout << "\n✓ SQLite Backend: Best for ACID compliance and reliability\n" << std::endl;
}

// ============================================================================
// Property Backend Example
// ============================================================================

void DemoPropertyBackend() {
    PrintTitle("Property Backend - Fast Shared Memory");
    
    // Create Property backend with File persistence
    std::cout << "Creating Property backend with File persistence..." << std::endl;
    KvsPropertyBackend propBackend("example_property", KvsBackendType::kvsFile);
    
    // Store runtime configuration (in shared memory)
    std::cout << "\nStoring runtime config (in-memory)..." << std::endl;
    propBackend.SetValue("runtime.threads", UInt32(8));
    propBackend.SetValue("runtime.queue_size", UInt32(1024));
    propBackend.SetValue("runtime.log_level", String("INFO"));
    propBackend.SetValue("runtime.monitoring", Bool(true));
    
    // Fast in-memory reads
    std::cout << "\nReading from shared memory (very fast):" << std::endl;
    
    auto threadsResult = propBackend.GetValue("runtime.threads");
    if (threadsResult.HasValue()) {
        auto threads = std::get_if<UInt32>(&threadsResult.Value());
        std::cout << "  runtime.threads = " << (threads ? *threads : 0) << std::endl;
    }
    
    auto queueResult = propBackend.GetValue("runtime.queue_size");
    if (queueResult.HasValue()) {
        auto queue = std::get_if<UInt32>(&queueResult.Value());
        std::cout << "  runtime.queue_size = " << (queue ? *queue : 0) << std::endl;
    }
    
    // Sync to persistence
    std::cout << "\nSyncing to persistence..." << std::endl;
    propBackend.SyncToStorage();
    std::cout << "✓ Data saved to persistent storage" << std::endl;
    
    // Get backend info
    auto sizeResult = propBackend.GetSize();
    if (sizeResult.HasValue()) {
        std::cout << "\nTotal keys in memory: " << sizeResult.Value() << std::endl;
    }
    
    std::cout << "\n✓ Property Backend: Best for fast IPC and in-memory operations\n" << std::endl;
}

// ============================================================================
// Backend Comparison
// ============================================================================

void PrintComparison() {
    PrintTitle("Backend Feature Comparison");
    
    std::cout << std::left;
    std::cout << std::setw(25) << "Feature" 
              << std::setw(15) << "File" 
              << std::setw(15) << "SQLite" 
              << std::setw(15) << "Property" << std::endl;
    
    std::cout << std::string(70, '-') << std::endl;
    
    std::cout << std::setw(25) << "Speed" 
              << std::setw(15) << "Fast" 
              << std::setw(15) << "Slower" 
              << std::setw(15) << "Very Fast" << std::endl;
    
    std::cout << std::setw(25) << "Transactions" 
              << std::setw(15) << "No" 
              << std::setw(15) << "Yes (ACID)" 
              << std::setw(15) << "No" << std::endl;
    
    std::cout << std::setw(25) << "Shared Memory" 
              << std::setw(15) << "No" 
              << std::setw(15) << "No" 
              << std::setw(15) << "Yes" << std::endl;
    
    std::cout << std::setw(25) << "IPC Support" 
              << std::setw(15) << "No" 
              << std::setw(15) << "No" 
              << std::setw(15) << "Yes" << std::endl;
    
    std::cout << std::setw(25) << "Persistence" 
              << std::setw(15) << "File" 
              << std::setw(15) << "Database" 
              << std::setw(15) << "Configurable" << std::endl;
    
    std::cout << std::setw(25) << "Best For" 
              << std::setw(15) << "Simple apps" 
              << std::setw(15) << "Reliability" 
              << std::setw(15) << "Performance" << std::endl;
    
    PrintSeparator();
    std::cout << std::endl;
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "\n";
    PrintTitle("LightAP Persistency - Backend Examples");
    std::cout << "\nDemonstrating all three KeyValueStorage backends:\n" << std::endl;
    
    try {
        // Demo each backend
        DemoFileBackend();
        DemoSqliteBackend();
        DemoPropertyBackend();
        
        // Show comparison
        PrintComparison();
        
        // Summary
        PrintTitle("Summary");
        std::cout << "\nChoose the right backend for your use case:\n" << std::endl;
        std::cout << "• File Backend:     Simple configuration, fast reads/writes" << std::endl;
        std::cout << "• SQLite Backend:   Critical data requiring ACID compliance" << std::endl;
        std::cout << "• Property Backend: High-performance IPC and in-memory ops\n" << std::endl;
        
        std::cout << "✓ All examples completed successfully!\n" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
