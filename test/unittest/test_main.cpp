/**
 * @file test_main.cpp
 * @brief Main entry point for Persistency module unit tests
 * @author AI Assistant
 * @date 2025-10-27
 */

#include <gtest/gtest.h>
#include <log/CLog.hpp>
#include "CPersistency.hpp"

int main(int argc, char **argv)
{
    ::lap::core::MemManager::getInstance();  // Initialize memory manager first

    // Initialize logging
    ::lap::log::LogManager::getInstance().initialize();
    
    // Initialize persistency manager
    ::lap::pm::CPersistencyManager::getInstance().initialize();
    
    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);
    
    // Run all tests
    int result = RUN_ALL_TESTS();
    
    return result;
}
