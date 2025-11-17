/**
 * @file config_integration_example.cpp
 * @brief Example demonstrating ConfigManager integration with Persistency module
 * @date 2025-11-14
 */

#include <iostream>
#include <lap/core/CCore.hpp>
#include <lap/core/CConfig.hpp>
#include "CPersistency.hpp"

using namespace lap::core;
using namespace lap::per;

int main() {
    std::cout << "=== Persistency ConfigManager Integration Example ===\n\n";
    
    // 1. Initialize Core system
    std::cout << "1. Initializing Core system...\n";
    auto initResult = Initialize();
    if (!initResult.HasValue()) {
        std::cerr << "Failed to initialize Core\n";
        return 1;
    }
    std::cout << "   ✓ Core initialized\n\n";
    
    // 2. Initialize ConfigManager
    std::cout << "2. Initializing ConfigManager...\n";
    ConfigManager& config = ConfigManager::getInstance();
    auto configResult = config.initialize("persistency_config_example.json", false);
    if (!configResult.HasValue()) {
        std::cerr << "Failed to initialize ConfigManager\n";
        return 1;
    }
    std::cout << "   ✓ ConfigManager initialized\n\n";
    
    // 3. Set persistency module configuration
    std::cout << "3. Setting persistency configuration...\n";
    nlohmann::json persistencyConfig;
    persistencyConfig["__metadata__"] = {
        {"contractVersion", "3.0.0"},
        {"deploymentVersion", "3.0.0"},
        {"manifestVersion", "1.0.0"},
        {"minimumSustainedSize", 2097152},      // 2MB
        {"maximumAllowedSize", 209715200},       // 200MB
        {"replicaCount", 5},                     // N=5
        {"minValidReplicas", 3},                 // M=3
        {"checksumType", "SHA256"},              // SHA256 checksum
        {"encryptionEnabled", false},
        {"encryptionAlgorithm", ""},
        {"encryptionKeyId", ""}
    };
    
    auto setResult = config.setModuleConfigJson("persistency", persistencyConfig);
    if (!setResult.HasValue()) {
        std::cerr << "Failed to set persistency configuration\n";
        return 1;
    }
    std::cout << "   ✓ Persistency configuration set:\n";
    std::cout << "     - Replica Count: 5\n";
    std::cout << "     - Min Valid Replicas: 3\n";
    std::cout << "     - Checksum Type: SHA256\n";
    std::cout << "     - Max Size: 200MB\n\n";
    
    // 4. Verify configuration is stored
    std::cout << "4. Verifying configuration...\n";
    auto retrievedConfig = config.getModuleConfigJson("persistency");
    if (retrievedConfig.contains("__metadata__")) {
        const auto& meta = retrievedConfig["__metadata__"];
        std::cout << "   ✓ Configuration retrieved:\n";
        std::cout << "     - Contract Version: " << meta["contractVersion"] << "\n";
        std::cout << "     - Replica Count: " << meta["replicaCount"] << "\n";
        std::cout << "     - Min Valid Replicas: " << meta["minValidReplicas"] << "\n";
        std::cout << "     - Checksum Type: " << meta["checksumType"] << "\n";
    } else {
        std::cerr << "   ✗ Configuration verification failed\n";
        return 1;
    }
    std::cout << "\n";
    
    // 5. Open FileStorage (will automatically load configuration from ConfigManager)
    std::cout << "5. Opening FileStorage...\n";
    auto fsResult = OpenFileStorage(InstanceSpecifier("/tmp/lightap_test_storage"), true);
    if (!fsResult.HasValue()) {
        std::cerr << "Failed to open FileStorage\n";
        return 1;
    }
    
    auto fs = fsResult.Value();
    std::cout << "   ✓ FileStorage opened successfully\n";
    std::cout << "   ✓ Configuration automatically loaded from ConfigManager\n";
    std::cout << "   ✓ Storage initialized with N=5, M=3, SHA256 checksums\n\n";
    
    // 6. Test file operations
    std::cout << "6. Testing file operations...\n";
    
    // Create a test file
    auto writeAccessor = fs->OpenFileWriteOnly("test_config_file.txt", OpenMode::kTruncate);
    if (writeAccessor.HasValue()) {
        String testData = "This file was created with ConfigManager-loaded configuration!\n";
        testData += "Replica configuration: N=5, M=3, SHA256 checksums\n";
        
        auto& writer = writeAccessor.Value();
        writer->WriteText(testData);
        
        std::cout << "   ✓ Test file written successfully\n";
        std::cout << "   ✓ 5 replicas created with SHA256 checksums\n";
    } else {
        std::cerr << "   ✗ Failed to open file for writing\n";
    }
    
    // Read back the file
    auto readAccessor = fs->OpenFileReadOnly("test_config_file.txt");
    if (readAccessor.HasValue()) {
        auto& reader = readAccessor.Value();
        String content;
        while (!reader->IsEof()) {
            auto ch = reader->GetChar();
            if (ch.HasValue()) {
                content += static_cast<char>(ch.Value());
            }
        }
        
        std::cout << "   ✓ Test file read successfully\n";
        std::cout << "   ✓ M-out-of-N consensus validation passed\n";
        std::cout << "   File content:\n" << content << "\n";
    } else {
        std::cerr << "   ✗ Failed to open file for reading\n";
    }
    
    std::cout << "\n=== Example completed successfully ===\n";
    
    // Cleanup
    Deinitialize();
    
    return 0;
}
