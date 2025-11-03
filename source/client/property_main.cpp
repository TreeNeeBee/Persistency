#include <string>
#include <cstdio>
#include <vector>
#include <sstream>

#include <lap/core/CTypedef.hpp>
#include "CPropertyClient.hpp"
#include "CPerErrorDomain.hpp"

using namespace lap;
using namespace lap::pm::client;

void printUsage(const char* programName) {
    printf("Usage: %s [options] <command> [args...]\n", programName);
    printf("\n");
    printf("Commands:\n");
    printf("  get <key> [default]          Get property value, return default if not found\n");
    printf("  set <key> <value>            Set property value\n");
    printf("  has <key>                    Check if property exists\n");
    printf("  delete <key>                 Delete property\n");
    printf("  list                         List all property keys\n");
    printf("  sync                         Sync persistent properties to storage\n");
    printf("\n");
    printf("Options:\n");
    printf("  -s, --socket <path>          Socket path (default: /tmp/property_service)\n");
    printf("  -h, --help                   Show this help message\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s get ro.build.version.release\n", programName);
    printf("  %s set persist.sys.language en\n", programName);
    printf("  %s set debug.level 3\n", programName);
    printf("  %s has persist.sys.timezone\n", programName);
    printf("  %s list\n", programName);
}

bool isInteger(const std::string& str) {
    if (str.empty()) return false;
    
    size_t start = 0;
    if (str[0] == '-' || str[0] == '+') {
        start = 1;
        if (str.length() == 1) return false;
    }
    
    for (size_t i = start; i < str.length(); ++i) {
        if (!std::isdigit(str[i])) {
            return false;
        }
    }
    return true;
}

bool isBool(const std::string& str) {
    return str == "true" || str == "false" || str == "1" || str == "0";
}

int main(int argc, char* argv[]) {
    std::string socketPath = "/tmp/property_service";
    std::vector<std::string> args;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-s" || arg == "--socket") {
            if (i + 1 < argc) {
                socketPath = argv[++i];
            } else {
                fprintf(stderr, "Error: --socket requires an argument\n");
                printUsage(argv[0]);
                return 1;
            }
        } else if (arg.find("-") == 0) {
            fprintf(stderr, "Error: Unknown option %s\n", arg.c_str());
            printUsage(argv[0]);
            return 1;
        } else {
            args.push_back(arg);
        }
    }
    
    if (args.empty()) {
        fprintf(stderr, "Error: No command specified\n");
        printUsage(argv[0]);
        return 1;
    }
    
    // Initialize logging system
    ::lap::log::LogManager::getInstance().initialize();
    
    PropertyClient client(socketPath);
    std::string command = args[0];
    
    try {
        if (command == "get") {
            if (args.size() < 2) {
                fprintf(stderr, "Error: get command requires a key\n");
                return 1;
            }
            
            std::string key = args[1];
            std::string defaultValue = (args.size() > 2) ? args[2] : "";
            
            auto result = client.getProperty(key, defaultValue);
            if (result.HasValue()) {
                printf("%s\n", result.Value().c_str());
            } else {
                                fprintf(stderr, "Error: Failed to get property: %s\n", std::string(result.Error().Message()).c_str());
                return 1;
            }
            
        } else if (command == "set") {
            if (args.size() < 3) {
                fprintf(stderr, "Error: set command requires key and value\n");
                return 1;
            }
            
            std::string key = args[1];
            std::string value = args[2];
            
            bool success = false;
            std::string errorMsg = "";
            
            // Try to detect value type and set accordingly
            if (isBool(value)) {
                bool boolValue = (value == "true" || value == "1");
                auto result = client.setProperty(key, boolValue);
                success = result.HasValue();
                if (!success) errorMsg = std::string(result.Error().Message());
            } else if (isInteger(value)) {
                try {
                    int32_t intValue = std::stoi(value);
                    auto result = client.setProperty(key, intValue);
                    success = result.HasValue();
                    if (!success) errorMsg = std::string(result.Error().Message());
                } catch (const std::exception&) {
                    auto result = client.setProperty(key, value); // Fallback to string
                    success = result.HasValue();
                    if (!success) errorMsg = std::string(result.Error().Message());
                }
            } else {
                auto result = client.setProperty(key, value);
                success = result.HasValue();
                if (!success) errorMsg = std::string(result.Error().Message());
            }
            
            if (success) {
                printf("Property set successfully\n");
            } else {
                fprintf(stderr, "Error: Failed to set property: %s\n", errorMsg.c_str());
                return 1;
            }
            
        } else if (command == "has") {
            if (args.size() < 2) {
                fprintf(stderr, "Error: has command requires a key\n");
                return 1;
            }
            
            std::string key = args[1];
            auto result = client.hasProperty(key);
            if (result.HasValue()) {
                printf("%s\n", (result.Value() ? "true" : "false"));
            } else {
                fprintf(stderr, "Error: Failed to check property: %s\n", std::string(result.Error().Message()).c_str());
                return 1;
            }
            
        } else if (command == "delete" || command == "remove") {
            if (args.size() < 2) {
                fprintf(stderr, "Error: delete command requires a key\n");
                return 1;
            }
            
            std::string key = args[1];
            auto result = client.removeProperty(key);
            if (result.HasValue()) {
                printf("Property deleted successfully\n");
            } else {
                fprintf(stderr, "Error: Failed to delete property: %s\n", std::string(result.Error().Message()).c_str());
                return 1;
            }
            
        } else if (command == "list") {
            auto result = client.getAllKeys();
            if (result.HasValue()) {
                const auto& keys = result.Value();
                if (keys.empty()) {
                    printf("No properties found\n");
                } else {
                    for (const auto& key : keys) {
                        printf("%s\n", key.c_str());
                    }
                }
            } else {
                fprintf(stderr, "Error: Failed to list properties: %s\n", std::string(result.Error().Message()).c_str());
                return 1;
            }
            
        } else if (command == "sync") {
            auto result = client.syncToStorage();
            if (result.HasValue()) {
                printf("Properties synced to storage successfully\n");
            } else {
                fprintf(stderr, "Error: Failed to sync properties: %s\n", std::string(result.Error().Message()).c_str());
                return 1;
            }
            
        } else {
            fprintf(stderr, "Error: Unknown command '%s'\n", command.c_str());
            printUsage(argv[0]);
            return 1;
        }
        
    } catch (const std::exception& e) {
        fprintf(stderr, "Error: %s\n", e.what());
        return 1;
    }
    
    return 0;
}