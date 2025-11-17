/**
 * @file CPersistencyManager.hpp
 * @author ddkv587 (ddkv587@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2024-02-02
 * 
 * 
 */
#ifndef LAP_PERSISTENCY_PERSISTENCYMANAGER_HPP
#define LAP_PERSISTENCY_PERSISTENCYMANAGER_HPP

#include <lap/core/CInstanceSpecifier.hpp>
#include <lap/core/CResult.hpp>
#include <lap/core/CMemory.hpp>
#include <lap/core/CSync.hpp>
#include <lap/core/CConfig.hpp>
#include <lap/core/CFile.hpp>
#include <lap/core/CPath.hpp>
#include <lap/core/CTime.hpp>

#include "CDataType.hpp"  // Includes FileStorageMetadata and StorageState
#include "CFileStorageBackend.hpp"  // For CFileStorageBackend
#include "CStoragePathManager.hpp"
#include "CReplicaManager.hpp"

namespace lap
{
namespace per
{
    class FileStorage;
    class KeyValueStorage;
    class CPersistencyManager final
    {
    public:
        IMP_OPERATOR_NEW(CPersistencyManager)
        
    private:
        using _FileStorageMap   = core::UnorderedMap< core::String, core::SharedHandle< FileStorage > >;
        using _KvsMap           = core::UnorderedMap< core::String, core::SharedHandle< KeyValueStorage > >;

        #define DEF_PER_CONFIG_INDICATE     "perConfig"

    public:
        static CPersistencyManager& getInstance() noexcept
        {
            static CPersistencyManager instance;

            return instance;
        }

        core::Bool                              initialize() noexcept;
        void                                    uninitialize() noexcept;
        inline core::Bool                       isInitialized() noexcept             { return m_bInitialized; }

        // for FileStorage
        // ========================================================================
        // FileStorage Management
        // ========================================================================
        
        /**
         * @brief Get or create FileStorage instance
         * @param indicate Instance specifier
         * @param bCreate Create if doesn't exist
         * @return SharedHandle to FileStorage
         */
        core::Result< core::SharedHandle< FileStorage > >       getFileStorage( const core::InstanceSpecifier&, core::Bool bCreate = false ) noexcept;
        
        /**
         * @brief Recover all files from backup
         * @param fs Instance specifier
         * @return Result indicating success or error
         */
        core::Result< void >                    RecoverAllFiles( const core::InstanceSpecifier & ) noexcept;
        
        /**
         * @brief Reset all files to initial state
         * @param fs Instance specifier
         * @return Result indicating success or error
         */
        core::Result< void >                    ResetAllFiles( const core::InstanceSpecifier & ) noexcept;
        
        /**
         * @brief Get current FileStorage size
         * @param fs Instance specifier
         * @return Total size in bytes
         */
        core::Result< core::UInt64 >            GetCurrentFileStorageSize( const core::InstanceSpecifier & ) noexcept;
        
        // ========================================================================
        // Lifecycle Management (Phase 2.2)
        // ========================================================================
        
        /**
         * @brief Generate storage path for instance
         * @param spec Instance specifier
         * @param type Storage type (KVS or FileStorage)
         * @return Storage path
         */
        core::String generateStoragePath(
            const core::InstanceSpecifier& spec,
            StorageType type
        ) noexcept;
        
        /**
         * @brief Load persistency configuration from Core::ConfigManager
         * @return PersistencyConfig structure
         */
        core::Result<PersistencyConfig> loadPersistencyConfig() noexcept;
        
        /**
         * @brief Validate configuration
         * @param config Configuration to validate
         * @return Result indicating success or error
         */
        core::Result<void> validateConfig(const PersistencyConfig& config) noexcept;
        
        /**
         * @brief Update configuration (directly updates ModuleConfig, no manual save)
         * @param config New configuration
         * @return Result indicating success or error
         */
        core::Result<void> updateConfig(const PersistencyConfig& config) noexcept;
        
        /**
         * @brief Create backup of FileStorage
         * @param fs Instance specifier
         * @return Result indicating success or error
         */
        core::Result<void> backupFileStorage(const core::InstanceSpecifier& fs) noexcept;
        
        /**
         * @brief Restore FileStorage from backup
         * @param fs Instance specifier
         * @return Result indicating success or error
         */
        core::Result<void> restoreFileStorage(const core::InstanceSpecifier& fs) noexcept;
        
        /**
         * @brief Remove backup
         * @param fs Instance specifier
         * @return Result indicating success or error
         */
        core::Result<void> removeBackup(const core::InstanceSpecifier& fs) noexcept;
        
        /**
         * @brief Check if backup exists
         * @param fs Instance specifier
         * @return true if backup exists
         */
        core::Result<core::Bool> backupExists(const core::InstanceSpecifier& fs) const noexcept;
        
        /**
         * @brief Check if update is needed
         * @param fs Instance specifier
         * @param manifestDeploymentVersion Deployment version from manifest
         * @param manifestContractVersion Contract version from manifest
         * @return true if update needed
         */
        core::Result<core::Bool> needsUpdate(
            const core::InstanceSpecifier& fs,
            const core::String& manifestDeploymentVersion,
            const core::String& manifestContractVersion
        ) noexcept;
        
        /**
         * @brief Perform update
         * @param fs Instance specifier
         * @param updatePath Path to update package
         * @return Result indicating success or error
         */
        core::Result<void> performUpdate(
            const core::InstanceSpecifier& fs,
            const core::String& updatePath
        ) noexcept;
        
        /**
         * @brief Rollback update
         * @param fs Instance specifier
         * @return Result indicating success or error
         */
        core::Result<void> rollback(const core::InstanceSpecifier& fs) noexcept;
        
        /**
         * @brief Check replica health
         * @param fs Instance specifier
         * @param category Category to check (default: "current")
         * @return Vector of replica metadata
         */
        core::Result<core::Vector<ReplicaMetadata>> checkReplicaHealth(
            const core::InstanceSpecifier& fs,
            const core::String& category = LAP_PER_CATEGORY_CURRENT
        ) noexcept;
        
        /**
         * @brief Repair replicas
         * @param fs Instance specifier
         * @param category Category to repair (default: "current")
         * @return Number of files repaired
         */
        core::Result<core::UInt32> repairReplicas(
            const core::InstanceSpecifier& fs,
            const core::String& category = LAP_PER_CATEGORY_CURRENT
        ) noexcept;
        
        /**
         * @brief Load metadata from storage
         * @param storagePath Storage path
         * @return FileStorageMetadata structure
         */
        core::Result<FileStorageMetadata> loadMetadata(const core::String& storagePath) noexcept;
        
        /**
         * @brief Save metadata to storage
         * @param storagePath Storage path
         * @param meta Metadata to save
         * @return Result indicating success or error
         */
        core::Result<void> saveMetadata(
            const core::String& storagePath,
            const FileStorageMetadata& meta
        ) noexcept;
        
        /**
         * @brief Update version information
         * @param storagePath Storage path
         * @param contractVersion Contract version
         * @param deploymentVersion Deployment version
         * @return Result indicating success or error
         */
        core::Result<void> updateVersionInfo(
            const core::String& storagePath,
            const core::String& contractVersion,
            const core::String& deploymentVersion
        ) noexcept;

        // for KeyValueStorage
        core::Result< core::SharedHandle< KeyValueStorage > >   getKvsStorage( const core::InstanceSpecifier&, core::Bool bCreate = false, KvsBackendType type = KvsBackendType::kvsFile ) noexcept;
        core::Result< void >                    RecoverKeyValueStorage( const core::InstanceSpecifier & ) noexcept;
        core::Result< void >                    ResetKeyValueStorage( const core::InstanceSpecifier & ) noexcept;
        core::Result< core::UInt64 >            GetCurrentKeyValueStorageSize( const core::InstanceSpecifier & ) noexcept;

    protected:
        CPersistencyManager() noexcept;
        ~CPersistencyManager() noexcept;

    private:
        core::Bool                              parseFromConfig( core::StringView ) noexcept;
        core::Bool                              saveToConfig( core::StringView ) noexcept;
        
        // Helper: Get FileStorage backend for operations
        core::Result<CFileStorageBackend*> getFileStorageBackend(const core::InstanceSpecifier& fs) noexcept;

    private:
        core::Bool                              m_bInitialized{ false };

        core::StringView                        m_strPrexPath{""};
        core::UInt32                            m_maxNumberOfFiles{32};
        LevelUpdateStrategy                     m_updateStrategy{LevelUpdateStrategy::Overwrite};
        RedundancyStrategy                      m_redundantStrategy{RedundancyStrategy::None};

        core::Mutex                             m_mtxFsMap;
        _FileStorageMap                         m_fsMap;

        core::Mutex                             m_mtxKvsMap;
        _KvsMap                                 m_kvsMap;
        
        // Lifecycle management (Phase 2.2)
        PersistencyConfig                       m_config;
        core::Bool                              m_configLoaded{false};
        core::UnorderedMap<core::String, FileStorageMetadata> m_metadataCache;
    };
} // namespace per
} // namespace lap
#endif
