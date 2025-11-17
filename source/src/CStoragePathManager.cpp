/**
 * @file CStoragePathManager.cpp
 * @brief Implementation of AUTOSAR standard storage path management
 */

#include "CStoragePathManager.hpp"
#include "CPerErrorDomain.hpp"
#include <lap/core/CFile.hpp>
#include <lap/log/CLog.hpp>

namespace lap {
namespace per {

// Static member initialization
core::String CStoragePathManager::s_centralStorageURI;
bool CStoragePathManager::s_isInitialized = false;

core::String CStoragePathManager::getCentralStorageURI() {
    if (!s_isInitialized) {
        try {
            auto config = loadPersistencyConfig();
            
            // Try to get centralStorageURI from config
            if (config.contains("centralStorageURI")) {
                s_centralStorageURI = config["centralStorageURI"].get<std::string>();
            } else {
                // Default value if not configured
                s_centralStorageURI = "/opt/autosar/persistency";
                LAP_PER_LOG_WARN << "centralStorageURI not found in config, using default: " 
                                 << s_centralStorageURI.data();
            }
            
            s_isInitialized = true;
        } catch (const std::exception& e) {
            // Fallback to default on error
            s_centralStorageURI = "/opt/autosar/persistency";
            s_isInitialized = true;
            LAP_PER_LOG_ERROR << "Failed to load config: " << e.what() 
                              << ", using default: " << s_centralStorageURI.data();
        }
    }
    
    return s_centralStorageURI;
}

core::String CStoragePathManager::getManifestPath() {
    core::String centralUri = getCentralStorageURI();
    return core::Path::appendString(centralUri, "manifest");
}

core::String CStoragePathManager::getKvsRootPath() {
    core::String centralUri = getCentralStorageURI();
    return core::Path::appendString(centralUri, "kvs");
}

core::String CStoragePathManager::getFileStorageRootPath() {
    core::String centralUri = getCentralStorageURI();
    return core::Path::appendString(centralUri, "fs");
}

core::String CStoragePathManager::getKvsInstancePath(core::StringView instancePath) {
    core::String kvsRoot = getKvsRootPath();
    core::String normalizedPath = normalizeInstancePath(instancePath);
    return core::Path::appendString(kvsRoot, normalizedPath);
}

core::String CStoragePathManager::getFileStorageInstancePath(core::StringView instancePath) {
    core::String fsRoot = getFileStorageRootPath();
    core::String normalizedPath = normalizeInstancePath(instancePath);
    return core::Path::appendString(fsRoot, normalizedPath);
}

core::Vector<core::String> CStoragePathManager::getReplicaPaths(
    core::StringView instancePath,
    core::StringView storageType,
    core::UInt32 replicaCount,
    const core::Vector<core::String>& deploymentUris
) {
    core::Vector<core::String> replicaPaths;
    
    // Get base instance path
    core::String basePath;
    core::String normalizedPath = normalizeInstancePath(instancePath);
    
    if (storageType == "kvs") {
        basePath = core::Path::appendString(core::String("kvs"), normalizedPath);
    } else if (storageType == "fs") {
        basePath = core::Path::appendString(core::String("fs"), normalizedPath);
    } else {
        LAP_PER_LOG_ERROR << "Invalid storage type: " << storageType.data();
        return replicaPaths;
    }
    
    // If no deployment URIs specified, use central storage URI
    if (deploymentUris.empty()) {
        core::String centralUri = getCentralStorageURI();
        for (core::UInt32 i = 0; i < replicaCount; ++i) {
            core::String replicaName = "replica_" + core::String(std::to_string(i));
            core::String fullPath = core::Path::appendString(
                core::Path::appendString(centralUri, basePath),
                replicaName
            );
            replicaPaths.push_back(fullPath);
        }
    } else {
        // Distribute replicas across deployment URIs
        for (core::UInt32 i = 0; i < replicaCount; ++i) {
            // Round-robin distribution: replica i goes to URI[i % uriCount]
            core::UInt32 uriIndex = i % deploymentUris.size();
            core::String replicaName = "replica_" + core::String(std::to_string(i));
            core::String fullPath = core::Path::appendString(
                core::Path::appendString(deploymentUris[uriIndex], basePath),
                replicaName
            );
            replicaPaths.push_back(fullPath);
        }
    }
    
    return replicaPaths;
}

// Overload that reads deployment URIs from configuration
core::Vector<core::String> CStoragePathManager::getReplicaPaths(
    core::StringView instancePath,
    core::StringView storageType,
    core::UInt32 replicaCount
) {
    core::Vector<core::String> deploymentUris;
    
    try {
        auto& config = core::ConfigManager::getInstance();
        auto persistencyConfig = config.getModuleConfigJson("persistency");
        
        if (!persistencyConfig.empty() && persistencyConfig.contains("deploymentUris")) {
            auto& uris = persistencyConfig["deploymentUris"];
            if (uris.is_array()) {
                for (const auto& uri : uris) {
                    if (uri.is_string()) {
                        deploymentUris.push_back(core::String(uri.get<std::string>().c_str()));
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        LAP_PER_LOG_WARN << "Failed to read deploymentUris from config: " << e.what();
        // Will use empty vector, which defaults to central storage
    }
    
    // Call the main implementation with deployment URIs (or empty vector)
    return getReplicaPaths(instancePath, storageType, replicaCount, deploymentUris);
}

core::Result<void> CStoragePathManager::createStorageStructure(
    core::StringView instancePath,
    core::StringView storageType
) {
    core::String basePath;
    
    if (storageType == "kvs") {
        basePath = getKvsInstancePath(instancePath);
    } else if (storageType == "fs") {
        basePath = getFileStorageInstancePath(instancePath);
    } else {
        LAP_PER_LOG_ERROR << "Invalid storage type: " << storageType.data();
        return core::Result<void>::FromError(PerErrc::kInvalidArgument);
    }
    
    // Create base instance directory
    if (!core::Path::createDirectory(basePath)) {
        LAP_PER_LOG_ERROR << "Failed to create base directory: " << basePath.data();
        return core::Result<void>::FromError(PerErrc::kPhysicalStorageFailure);
    }
    
    // Create subdirectories based on storage type
    if (storageType == "kvs") {
        // AUTOSAR 4-layer structure: current/, update/, redundancy/, recovery/
        // [SWS_PER_00500] current/  - Active data
        // [SWS_PER_00501] update/   - Update buffer
        // [SWS_PER_00502] redundancy/ - Backup for rollback
        // [SWS_PER_00503] recovery/ - Deleted keys for RecoverKey()
        core::Vector<core::String> subdirs = {"current", "update", "redundancy", "recovery"};
        for (const auto& subdir : subdirs) {
            core::String fullPath = core::Path::appendString(basePath, subdir);
            if (!core::Path::createDirectory(fullPath)) {
                LAP_PER_LOG_ERROR << "Failed to create subdirectory: " << fullPath.data();
                return core::Result<void>::FromError(PerErrc::kPhysicalStorageFailure);
            }
        }
    } else if (storageType == "fs") {
        // FileStorage structure: current/, backup/, initial/, update/
        core::Vector<core::String> subdirs = {"current", "backup", "initial", "update"};
        for (const auto& subdir : subdirs) {
            core::String fullPath = core::Path::appendString(basePath, subdir);
            if (!core::Path::createDirectory(fullPath)) {
                LAP_PER_LOG_ERROR << "Failed to create subdirectory: " << fullPath.data();
                return core::Result<void>::FromError(PerErrc::kPhysicalStorageFailure);
            }
        }
        
        // Also create .metadata directory for FileStorage
        core::String metadataPath = core::Path::appendString(basePath, ".metadata");
        if (!core::Path::createDirectory(metadataPath)) {
            LAP_PER_LOG_ERROR << "Failed to create .metadata directory: " << metadataPath.data();
            return core::Result<void>::FromError(PerErrc::kPhysicalStorageFailure);
        }
    }
    
    LAP_PER_LOG_INFO << "Created storage structure: " << basePath.data();
    return core::Result<void>::FromValue();
}

core::Result<void> CStoragePathManager::createManifestStructure() {
    core::String manifestPath = getManifestPath();
    
    if (!core::Path::createDirectory(manifestPath)) {
        LAP_PER_LOG_ERROR << "Failed to create manifest directory: " << manifestPath.data();
        return core::Result<void>::FromError(PerErrc::kPhysicalStorageFailure);
    }
    
    LAP_PER_LOG_INFO << "Created manifest structure: " << manifestPath.data();
    return core::Result<void>::FromValue();
}

bool CStoragePathManager::pathExists(core::StringView path) {
    return core::Path::isDirectory(path);
}

core::String CStoragePathManager::normalizeInstancePath(core::StringView instancePath) {
    core::String normalized(instancePath.data());
    
    // Remove leading slash if present
    if (!normalized.empty() && normalized[0] == '/') {
        normalized = normalized.substr(1);
    }
    
    return normalized;
}

nlohmann::json CStoragePathManager::loadPersistencyConfig() {
    auto& configManager = core::ConfigManager::getInstance();
    
    try {
        // Get "persistency" module configuration
        return configManager.getModuleConfigJson("persistency");
    } catch (const std::exception& e) {
        LAP_PER_LOG_ERROR << "Failed to load persistency config: " << e.what();
        throw;
    }
}

} // namespace per
} // namespace lap
