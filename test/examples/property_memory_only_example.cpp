/**
 * @file property_memory_only_example.cpp
 * @brief Example of Property Backend without persistence (pure in-memory usage)
 * @date 2025-11-17
 * 
 * This example demonstrates using Property Backend as a pure in-memory key-value store
 * WITHOUT any persistence backend (no File, no SQLite).
 * 
 * Use cases:
 * - Temporary session data
 * - Inter-process communication via shared memory
 * - High-performance cache that doesn't need to survive restarts
 * - Volatile runtime state
 */

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <lap/core/CCore.hpp>
#include "CKvsPropertyBackend.hpp"
#include "CDataType.hpp"

using namespace lap::core;
using namespace lap::per;

void PrintSeparator(const std::string& title) {
    std::cout << "\n========================================" << std::endl;
    std::cout << title << std::endl;
    std::cout << "========================================" << std::endl;
}

void DemoBasicMemoryUsage() {
    PrintSeparator("Basic Memory-Only Usage");
    
    // Create Property Backend without persistence (KvsBackendType::kvsNone)
    constexpr size_t shmSize = 2ul << 20; // 2MB shared memory
    lap::per::util::KvsPropertyBackend backend("memory_only_demo", KvsBackendType::kvsNone, shmSize);
    
    std::cout << "✓ Property Backend created (memory-only, no persistence)" << std::endl;
    std::cout << "  Shared memory size: " << (shmSize / 1024) << " KB" << std::endl;
    
    // Store various data types in memory
    backend.SetValue("temp.string", String("Hello, Memory!"));
    backend.SetValue("temp.int32", Int32(12345));
    backend.SetValue("temp.uint64", UInt64(9876543210ULL));
    backend.SetValue("temp.float", Float(3.14159f));
    backend.SetValue("temp.bool", Bool(true));
    
    std::cout << "\n✓ Data stored in shared memory (no disk I/O)" << std::endl;
    
    // Read data back
    auto strVal = backend.GetValue("temp.string");
    auto intVal = backend.GetValue("temp.int32");
    auto floatVal = backend.GetValue("temp.float");
    
    std::cout << "\nStored values:" << std::endl;
    if (strVal.HasValue()) {
        auto str = std::get_if<String>(&strVal.Value());
        std::cout << "  String: " << (str ? str->data() : "N/A") << std::endl;
    }
    if (intVal.HasValue()) {
        auto val = std::get_if<Int32>(&intVal.Value());
        std::cout << "  Int32: " << (val ? *val : 0) << std::endl;
    }
    if (floatVal.HasValue()) {
        auto val = std::get_if<Float>(&floatVal.Value());
        std::cout << "  Float: " << (val ? *val : 0.0f) << std::endl;
    }
    
    auto countResult = backend.GetKeyCount();
    std::cout << "\nTotal keys in memory: " << (countResult.HasValue() ? countResult.Value() : 0) << std::endl;
}

void DemoSessionManagement() {
    PrintSeparator("Session Management (Temporary Data)");
    
    constexpr size_t shmSize = 4ul << 20; // 4MB
    lap::per::util::KvsPropertyBackend sessionStore("session_store", KvsBackendType::kvsNone, shmSize);
    
    std::cout << "Simulating user session management..." << std::endl;
    
    // Session 1
    sessionStore.SetValue("session.user1.id", String("alice@example.com"));
    sessionStore.SetValue("session.user1.login_time", UInt64(1700178000));
    sessionStore.SetValue("session.user1.permissions", String("read,write,admin"));
    sessionStore.SetValue("session.user1.active", Bool(true));
    
    // Session 2
    sessionStore.SetValue("session.user2.id", String("bob@example.com"));
    sessionStore.SetValue("session.user2.login_time", UInt64(1700178100));
    sessionStore.SetValue("session.user2.permissions", String("read"));
    sessionStore.SetValue("session.user2.active", Bool(true));
    
    std::cout << "✓ Sessions stored in memory (cleared on restart)" << std::endl;
    
    // Check active sessions
    auto user1Active = sessionStore.GetValue("session.user1.active");
    auto user2Active = sessionStore.GetValue("session.user2.active");
    
    int activeSessions = 0;
    if (user1Active.HasValue() && std::get_if<Bool>(&user1Active.Value()) && *std::get_if<Bool>(&user1Active.Value())) {
        activeSessions++;
    }
    if (user2Active.HasValue() && std::get_if<Bool>(&user2Active.Value()) && *std::get_if<Bool>(&user2Active.Value())) {
        activeSessions++;
    }
    
    std::cout << "\nActive sessions: " << activeSessions << std::endl;
    
    // Simulate logout
    std::cout << "\nSimulating user logout..." << std::endl;
    sessionStore.RemoveKey("session.user1.id");
    sessionStore.RemoveKey("session.user1.login_time");
    sessionStore.RemoveKey("session.user1.permissions");
    sessionStore.RemoveKey("session.user1.active");
    
    std::cout << "✓ Session data removed from memory" << std::endl;
    
    auto countResult = sessionStore.GetKeyCount();
    std::cout << "Remaining sessions: " << (countResult.HasValue() ? countResult.Value() / 4 : 0) << std::endl;
}

void DemoHighPerformanceCache() {
    PrintSeparator("High-Performance Cache (No Persistence Overhead)");
    
    constexpr size_t shmSize = 8ul << 20; // 8MB for cache
    lap::per::util::KvsPropertyBackend cache("high_perf_cache", KvsBackendType::kvsNone, shmSize);
    
    std::cout << "Performance test: Writing 1000 cache entries..." << std::endl;
    
    auto startWrite = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; ++i) {
        std::string key = "cache.entry." + std::to_string(i);
        std::string value = "cached_data_" + std::to_string(i * 100);
        cache.SetValue(key, String(value.c_str()));
    }
    
    auto endWrite = std::chrono::high_resolution_clock::now();
    auto writeTime = std::chrono::duration_cast<std::chrono::microseconds>(endWrite - startWrite).count();
    
    std::cout << "✓ Write completed in " << (writeTime / 1000.0) << " ms" << std::endl;
    std::cout << "  Average: " << (writeTime / 1000.0) << " µs per write" << std::endl;
    
    // Read performance
    std::cout << "\nPerformance test: Reading 1000 cache entries..." << std::endl;
    
    auto startRead = std::chrono::high_resolution_clock::now();
    
    int successCount = 0;
    for (int i = 0; i < 1000; ++i) {
        std::string key = "cache.entry." + std::to_string(i);
        auto result = cache.GetValue(key);
        if (result.HasValue()) {
            successCount++;
        }
    }
    
    auto endRead = std::chrono::high_resolution_clock::now();
    auto readTime = std::chrono::duration_cast<std::chrono::microseconds>(endRead - startRead).count();
    
    std::cout << "✓ Read completed in " << (readTime / 1000.0) << " ms" << std::endl;
    std::cout << "  Average: " << (readTime / 1000.0) << " µs per read" << std::endl;
    std::cout << "  Success rate: " << successCount << "/1000" << std::endl;
    
    std::cout << "\n✓ No disk I/O overhead - pure memory operations!" << std::endl;
}

void DemoInterProcessCommunication() {
    PrintSeparator("Inter-Process Communication via Shared Memory");
    
    constexpr size_t shmSize = 1ul << 20; // 1MB
    lap::per::util::KvsPropertyBackend ipcStore("ipc_demo", KvsBackendType::kvsNone, shmSize);
    
    std::cout << "Simulating IPC between processes..." << std::endl;
    std::cout << "(Using same process for demo - in real scenario, separate processes)" << std::endl;
    
    // Process 1: Producer
    std::cout << "\n[Process 1] Writing messages to shared memory..." << std::endl;
    ipcStore.SetValue("ipc.message.1", String("Hello from Process 1"));
    ipcStore.SetValue("ipc.message.2", String("Data ready for processing"));
    ipcStore.SetValue("ipc.status", String("ready"));
    ipcStore.SetValue("ipc.timestamp", UInt64(1700178000));
    
    std::cout << "✓ Messages written to shared memory" << std::endl;
    
    // Simulate small delay
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Process 2: Consumer (would be different process in real scenario)
    std::cout << "\n[Process 2] Reading messages from shared memory..." << std::endl;
    
    auto status = ipcStore.GetValue("ipc.status");
    if (status.HasValue()) {
        auto statusStr = std::get_if<String>(&status.Value());
        std::cout << "  Status: " << (statusStr ? statusStr->data() : "N/A") << std::endl;
    }
    
    auto msg1 = ipcStore.GetValue("ipc.message.1");
    if (msg1.HasValue()) {
        auto msgStr = std::get_if<String>(&msg1.Value());
        std::cout << "  Message 1: " << (msgStr ? msgStr->data() : "N/A") << std::endl;
    }
    
    auto msg2 = ipcStore.GetValue("ipc.message.2");
    if (msg2.HasValue()) {
        auto msgStr = std::get_if<String>(&msg2.Value());
        std::cout << "  Message 2: " << (msgStr ? msgStr->data() : "N/A") << std::endl;
    }
    
    std::cout << "\n✓ Inter-process communication successful!" << std::endl;
    std::cout << "  (Data accessible to all processes attached to same shared memory)" << std::endl;
}

void DemoVolatileRuntimeState() {
    PrintSeparator("Volatile Runtime State (No Persistence Needed)");
    
    constexpr size_t shmSize = 2ul << 20; // 2MB
    lap::per::util::KvsPropertyBackend runtimeState("runtime_state", KvsBackendType::kvsNone, shmSize);
    
    std::cout << "Storing volatile runtime metrics..." << std::endl;
    
    // CPU metrics
    runtimeState.SetValue("metrics.cpu.usage", Float(45.2f));
    runtimeState.SetValue("metrics.cpu.temperature", Float(65.5f));
    
    // Memory metrics
    runtimeState.SetValue("metrics.memory.used_mb", UInt64(2048));
    runtimeState.SetValue("metrics.memory.free_mb", UInt64(6144));
    
    // Network metrics
    runtimeState.SetValue("metrics.network.packets_rx", UInt64(1234567));
    runtimeState.SetValue("metrics.network.packets_tx", UInt64(987654));
    
    // Application state
    runtimeState.SetValue("state.uptime_seconds", UInt64(3600));
    runtimeState.SetValue("state.active_connections", Int32(42));
    
    std::cout << "✓ Runtime metrics stored in memory" << std::endl;
    
    // Display metrics
    std::cout << "\nCurrent Runtime Metrics:" << std::endl;
    
    auto cpuUsage = runtimeState.GetValue("metrics.cpu.usage");
    if (cpuUsage.HasValue()) {
        auto val = std::get_if<Float>(&cpuUsage.Value());
        std::cout << "  CPU Usage: " << (val ? *val : 0.0f) << "%" << std::endl;
    }
    
    auto memUsed = runtimeState.GetValue("metrics.memory.used_mb");
    auto memFree = runtimeState.GetValue("metrics.memory.free_mb");
    if (memUsed.HasValue() && memFree.HasValue()) {
        auto used = std::get_if<UInt64>(&memUsed.Value());
        auto free = std::get_if<UInt64>(&memFree.Value());
        if (used && free) {
            uint64_t total = *used + *free;
            float usagePercent = (*used * 100.0f) / total;
            std::cout << "  Memory: " << *used << " MB / " << total << " MB (" 
                      << usagePercent << "%)" << std::endl;
        }
    }
    
    auto connections = runtimeState.GetValue("state.active_connections");
    if (connections.HasValue()) {
        auto val = std::get_if<Int32>(&connections.Value());
        std::cout << "  Active Connections: " << (val ? *val : 0) << std::endl;
    }
    
    auto countResult = runtimeState.GetKeyCount();
    std::cout << "\nTotal metrics tracked: " << (countResult.HasValue() ? countResult.Value() : 0) << std::endl;
    
    std::cout << "\n✓ Data will be lost on restart (by design - no persistence)" << std::endl;
}

int main() {
    std::cout << "============================================================" << std::endl;
    std::cout << "Property Backend - Memory-Only Mode (No Persistence)" << std::endl;
    std::cout << "Using KvsBackendType::kvsNone for pure in-memory operations" << std::endl;
    std::cout << "============================================================" << std::endl;
    
    try {
        // Demonstrate various memory-only use cases
        DemoBasicMemoryUsage();
        DemoSessionManagement();
        DemoHighPerformanceCache();
        DemoInterProcessCommunication();
        DemoVolatileRuntimeState();
        
        // Summary
        PrintSeparator("Summary - Memory-Only Mode Benefits");
        std::cout << "\n✓ Zero disk I/O - maximum performance" << std::endl;
        std::cout << "✓ Shared memory IPC - process communication" << std::endl;
        std::cout << "✓ Temporary data - automatic cleanup on restart" << std::endl;
        std::cout << "✓ No persistence overhead - pure memory operations" << std::endl;
        std::cout << "\nWhen to use Memory-Only Mode:" << std::endl;
        std::cout << "  • Session management (temporary user data)" << std::endl;
        std::cout << "  • High-performance caching" << std::endl;
        std::cout << "  • Inter-process communication" << std::endl;
        std::cout << "  • Volatile runtime metrics" << std::endl;
        std::cout << "  • Any data that doesn't need to survive restarts" << std::endl;
        std::cout << "\nNote: Use KvsBackendType::kvsFile or kvsBackendType::kvsSqlite" << std::endl;
        std::cout << "      if you need data to persist across restarts." << std::endl;
        std::cout << "\n✓ Example completed successfully!" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
