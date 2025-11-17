/**
 * @file test_main.cpp
 * @brief Main entry point for Persistency module unit tests
 * @author AI Assistant
 * @date 2025-10-27
 * 
 * Note: This file is currently commented out because individual test files
 * (like test_file_storage_backend.cpp) have their own main() functions.
 * Use this main() when consolidating all tests into a single executable.
 */

#if 0
#include <gtest/gtest.h>
#include <lap/log/CLog.hpp>
#include "CPersistency.hpp"

int main(int argc, char **argv)
{
    ::lap::core::MemoryManager::getInstance();  // Initialize memory manager first

    // Initialize logging
    ::lap::log::LogManager::getInstance().initialize();
    
    // Initialize persistency manager
    ::lap::per::CPersistencyManager::getInstance().initialize();
    
    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);
    
    // Run all tests
    int result = RUN_ALL_TESTS();
    
    return result;
}
#endif // Comment out test_main.cpp
