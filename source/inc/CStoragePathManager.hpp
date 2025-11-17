/**
 * @file CStoragePathManager.hpp
 * @brief AUTOSAR standard storage path management
 * 
 * This class provides centralized path management for Persistency module
 * following AUTOSAR AP R24-11 specifications.
 * 
 * @note All paths use Core::Path and Core::ConfigManager
 * @note No backward compatibility support
 */

#ifndef LAP_PER_CSTORAGEPATHMANAGER_HPP
#define LAP_PER_CSTORAGEPATHMANAGER_HPP

#include <lap/core/CCore.hpp>
#include <lap/core/CConfig.hpp>

namespace lap {
namespace per {

/**
 * @class CStoragePathManager
 * @brief Manages all storage paths according to AUTOSAR standard directory structure
 * 
 * AUTOSAR AP R24-11 Directory Structure:
 * {centralStorageURI}/
 * ├── manifest/
 * │   └── installed_storages.json
 * ├── kvs/
 * │   └── {instancePath}/
 * │       ├── current/        # [SWS_PER_00500] Active data
 * │       ├── update/         # [SWS_PER_00501] Update buffer
 * │       ├── redundancy/     # [SWS_PER_00502] Backup for rollback
 * │       └── recovery/       # [SWS_PER_00503] Deleted keys
 * └── fs/
 *     └── {instancePath}/
 *         ├── current/
 *         ├── backup/
 *         ├── initial/
 *         └── update/
 */
class CStoragePathManager {
public:
    /**
     * @brief Get the central storage root URI from configuration
     * @return Central storage URI (e.g., "/opt/autosar/persistency")
     * @note Reads from Core::ConfigManager "persistency.centralStorageURI"
     */
    static core::String getCentralStorageURI();

    /**
     * @brief Get the manifest directory path
     * @return {centralStorageURI}/manifest
     */
    static core::String getManifestPath();

    /**
     * @brief Get the KVS root directory path
     * @return {centralStorageURI}/kvs
     */
    static core::String getKvsRootPath();

    /**
     * @brief Get the FileStorage root directory path
     * @return {centralStorageURI}/fs
     */
    static core::String getFileStorageRootPath();

    /**
     * @brief Get the path for a specific KVS instance
     * @param instancePath Instance specifier (e.g., "/app/kvs_instance")
     * @return {centralStorageURI}/kvs/app/kvs_instance
     * @note Leading slash is removed from instancePath
     */
    static core::String getKvsInstancePath(core::StringView instancePath);

    /**
     * @brief Get the path for a specific FileStorage instance
     * @param instancePath Instance specifier (e.g., "/app/file_storage")
     * @return {centralStorageURI}/fs/app/file_storage
     * @note Leading slash is removed from instancePath
     */
    static core::String getFileStorageInstancePath(core::StringView instancePath);

    /**
     * @brief Get replica paths for multi-URI deployment
     * @param instancePath Instance specifier
     * @param storageType "kvs" or "fs"
     * @param replicaCount Number of replicas (N)
     * @param deploymentUris List of deployment URIs
     * @return Vector of replica paths distributed across URIs
     * 
     * Example with N=3:
     * - Replica 0: {deploymentUris[0]}/kvs/{instance}/replica_0
     * - Replica 1: {deploymentUris[1]}/kvs/{instance}/replica_1
     * - Replica 2: {deploymentUris[2]}/kvs/{instance}/replica_2
     */
    static core::Vector<core::String> getReplicaPaths(
        core::StringView instancePath,
        core::StringView storageType,
        core::UInt32 replicaCount,
        const core::Vector<core::String>& deploymentUris
    );
    
    /**
     * @brief Get replica paths (reads deployment URIs from configuration)
     * @param instancePath Instance specifier
     * @param storageType "kvs" or "fs"
     * @param replicaCount Number of replicas
     * @return Vector of replica paths
     * 
     * This overload reads deployment URIs from configuration.
     * If no deployment URIs configured, uses central storage for all replicas.
     */
    static core::Vector<core::String> getReplicaPaths(
        core::StringView instancePath,
        core::StringView storageType,
        core::UInt32 replicaCount
    );

    /**
     * @brief Create standard directory structure for a storage instance
     * @param instancePath Instance specifier
     * @param storageType "kvs" or "fs"
     * @return Success or error
     * 
     * AUTOSAR 4-Layer Structure for KVS [SWS_PER_00500]:
     * - current/     : Active data files
     * - update/      : Update buffer (all modifications happen here)
     * - redundancy/  : Backup for rollback (previous version)
     * - recovery/    : Deleted keys storage for RecoverKey()
     * 
     * For FS: Creates {root}/fs/{instance}/, current/, backup/, initial/, update/
     */
    static core::Result<void> createStorageStructure(
        core::StringView instancePath,
        core::StringView storageType
    );

    /**
     * @brief Create manifest directory structure
     * @return Success or error
     * Creates {centralStorageURI}/manifest/
     */
    static core::Result<void> createManifestStructure();

    /**
     * @brief Check if a path exists and is a directory
     * @param path Path to check
     * @return true if path exists and is a directory
     */
    static bool pathExists(core::StringView path);

#ifdef UNIT_TEST
    /**
     * @brief Reset cached configuration for testing
     * @note Only available in test builds
     */
    static void resetForTesting() {
        s_isInitialized = false;
        s_centralStorageURI.clear();
    }
#endif

private:
    /**
     * @brief Normalize instance path by removing leading slash
     * @param instancePath Original path (e.g., "/app/kvs_instance")
     * @return Normalized path (e.g., "app/kvs_instance")
     */
    static core::String normalizeInstancePath(core::StringView instancePath);

    /**
     * @brief Load configuration from Core::ConfigManager
     * @return Persistency configuration JSON
     * @throw If configuration module "persistency" not found
     */
    static nlohmann::json loadPersistencyConfig();

    // Cached central storage URI (loaded on first access)
    static core::String s_centralStorageURI;
    static bool s_isInitialized;
};

} // namespace per
} // namespace lap

#endif // LAP_PER_CSTORAGEPATHMANAGER_HPP
