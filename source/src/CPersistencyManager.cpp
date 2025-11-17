#include <lap/core/CPath.hpp>
#include <lap/core/CFile.hpp>
#include <lap/core/CTime.hpp>
#include <lap/log/CLog.hpp>
#include <sstream>
#include "CFileStorage.hpp"
#include "CKeyValueStorage.hpp"
#include "CFileStorageBackend.hpp"
#include "CPersistencyManager.hpp"
#include "CPerErrorDomain.hpp"

namespace lap
{
namespace per
{
    core::Bool CPersistencyManager::initialize() noexcept
    {
        if ( m_bInitialized )     return true;

        //parseFromConfig(  );

        m_bInitialized = true;

        return true;
    }

    void CPersistencyManager::uninitialize() noexcept
    {
        if ( !m_bInitialized )    return;

        // auto finalize file storage
        {
            core::LockGuard lock( m_mtxFsMap );

            for ( auto&& it = m_fsMap.begin(); it != m_fsMap.end(); ++it ) {
                it->second->uninitialize();
            }

            m_fsMap.clear();
        }

        // auto finalize kvs storage
        {
            core::LockGuard lock( m_mtxKvsMap );

            for ( auto&& it = m_kvsMap.begin(); it != m_kvsMap.end(); ++it ) {
                it->second->uninitialize();
            }

            m_kvsMap.clear();
        }

        m_bInitialized = false;
    }

    core::Result< core::SharedHandle< FileStorage > > CPersistencyManager::getFileStorage( const core::InstanceSpecifier& indicate, core::Bool bCreate ) noexcept
    {
        using result = core::Result< core::SharedHandle< FileStorage > >;

        if ( !m_bInitialized ) return result::FromError( PerErrc::kNotInitialized );

        core::StringView instanceId = indicate.ToString();

        if ( instanceId.empty() ) {
            LAP_PER_LOG_WARN << "CPersistencyManager::getFileStorage with invalid instance specifier";
            return result::FromError( PerErrc::kStorageNotFound );
        }

        core::LockGuard lock( m_mtxFsMap );

        // Check if already exists
        auto&& it = m_fsMap.find( instanceId.data() );
        if ( it != m_fsMap.end() ) {
            // Already exists, check status
            if ( !it->second->initialize() ) {
                return result::FromError( PerErrc::kPhysicalStorageFailure );
            }

            // Check if storage is busy (in reset or update)
            if ( !it->second->isResourceBusy() ) {
                return result::FromError( PerErrc::kResourceBusy );
            }

            return result::FromValue( it->second );
        }

        // Doesn't exist, need to create
        if ( !bCreate ) {
            return result::FromError( PerErrc::kStorageNotFound );
        }

        // 1. Generate standard storage path using StoragePathManager
        core::String storagePath = generateStoragePath(indicate, StorageType::kFileStorage);
        LAP_PER_LOG_INFO << "Creating FileStorage at: " << storagePath;

        // 2. Create directory structure
        auto createResult = CStoragePathManager::createStorageStructure(instanceId, "fs");
        if (!createResult.HasValue()) {
            LAP_PER_LOG_ERROR << "Failed to create directory structure";
            return result::FromError( PerErrc::kPhysicalStorageFailure );
        }

        // 3. Load or create configuration
        if (!m_configLoaded) {
            auto configResult = loadPersistencyConfig();
            if (configResult.HasValue()) {
                m_config = configResult.Value();
                m_configLoaded = true;
            } else {
                LAP_PER_LOG_WARN << "Using default configuration";
            }
        }

        // 4. Load or create metadata
        auto metadataResult = loadMetadata(storagePath);
        FileStorageMetadata metadata;
        if (metadataResult.HasValue()) {
            metadata = metadataResult.Value();
        } else {
            // Create default metadata
            metadata.storageUri = storagePath;
            metadata.deploymentUri = storagePath;
            metadata.contractVersion = m_config.contractVersion;
            metadata.deploymentVersion = m_config.deploymentVersion;
            metadata.replicaCount = m_config.replicaCount;
            metadata.minValidReplicas = m_config.minValidReplicas;
            metadata.checksumType = (m_config.checksumType == "CRC32") ? ChecksumType::kCRC32 : ChecksumType::kSHA256;
            metadata.state = StorageState::kNormal;
            metadata.creationTime = core::Time::GetCurrentTimeISO();
            metadata.lastUpdateTime = core::Time::GetCurrentTimeISO();
            metadata.lastAccessTime = core::Time::GetCurrentTimeISO();
            metadata.encryptionEnabled = false;
            metadata.minimumSustainedSize = LAP_PER_DEFAULT_MIN_SUSTAINED_SIZE;
            metadata.maximumAllowedSize = LAP_PER_DEFAULT_MAX_ALLOWED_SIZE;

            // Save metadata
            saveMetadata(storagePath, metadata);
        }

        // 5. Create FileStorage object
        auto fs = FileStorage::create(storagePath);

        // 6. Create and inject backend
        auto backend = core::MakeUnique<CFileStorageBackend>(storagePath);
        fs->setBackend(std::move(backend));

        // 7. Initialize FileStorage (simplified - just loads file info)
        auto initResult = fs->initialize();
        if (!initResult.HasValue() || !initResult.Value()) {
            LAP_PER_LOG_ERROR << "Failed to initialize FileStorage";
            return result::FromError( PerErrc::kPhysicalStorageFailure );
        }

        // 8. Cache and return
        m_fsMap.emplace( instanceId.data(), fs );
        return result::FromValue( fs );
    }

    core::Result< void > CPersistencyManager::RecoverAllFiles( const core::InstanceSpecifier &fs ) noexcept
    {
        if ( !m_bInitialized ) return core::Result< void >::FromError( PerErrc::kNotInitialized );

        auto&& it = m_fsMap.find( fs.ToString().data() );

        if ( it != m_fsMap.end() ) {
            return it->second->RecoverAllFiles();
        } else {
            return core::Result< void >::FromError( PerErrc::kStorageNotFound );
        }
    }

    core::Result< void > CPersistencyManager::ResetAllFiles( const core::InstanceSpecifier &fs ) noexcept
    {
        if ( !m_bInitialized ) return core::Result< void >::FromError( PerErrc::kNotInitialized );

        auto&& it = m_fsMap.find( fs.ToString().data() );

        if ( it != m_fsMap.end() ) {
            return it->second->ResetAllFiles();
        } else {
            return core::Result< void >::FromError( PerErrc::kStorageNotFound );
        }
    }

    core::Result< core::UInt64 > CPersistencyManager::GetCurrentFileStorageSize( const core::InstanceSpecifier &fs ) noexcept
    {
        if ( !m_bInitialized ) return core::Result< core::UInt64 >::FromError( PerErrc::kNotInitialized );

        auto&& it = m_fsMap.find( fs.ToString().data() );

        if ( it != m_fsMap.end() ) {
            return it->second->GetCurrentFileStorageSize();
        } else {
            return core::Result< core::UInt64 >::FromError( PerErrc::kStorageNotFound );
        }
    }

    core::Result< core::SharedHandle< KeyValueStorage > > CPersistencyManager::getKvsStorage( const core::InstanceSpecifier& indicate, core::Bool bCreate, KvsBackendType type ) noexcept
    {
        using result = core::Result< core::SharedHandle< KeyValueStorage > >;

        if ( !m_bInitialized ) return result::FromError( PerErrc::kNotInitialized );

        core::StringView strFolder{ indicate.ToString() };

        if ( strFolder.empty() ) {
            LAP_PER_LOG_WARN << "CPersistencyManager::getKvsStorage with invalid path " 
                                << strFolder.data()
                                << ", try to use default";

            return result::FromError( PerErrc::kStorageNotFound );
        } else {
            auto&& it = m_kvsMap.find( strFolder.data() );

            if ( it != m_kvsMap.end() ) {
                // check kvs storage status
                core::LockGuard lock( m_mtxKvsMap );

                // check if storage can initialize
                if ( !it->second->initialize() )        return result::FromError( PerErrc::kPhysicalStorageFailure );

                // check if storage in reset or update
                if ( !it->second->isResourceBusy() )    return result::FromError( PerErrc::kResourceBusy );

                return result::FromValue( it->second );
            } else {
                if ( !bCreate ) return result::FromError( PerErrc::kStorageNotFound );

                // make sure folder is exist
                if ( core::Path::createDirectory( strFolder ) ) {
                    auto kvs = KeyValueStorage::create( strFolder.data(), type );
                    m_kvsMap.emplace( strFolder.data(), kvs );

                    return result::FromValue( kvs );
                } else {
                    LAP_PER_LOG_WARN << "CPersistencyManager::getKvsStorage can not create or access with "
                                        << strFolder.data()
                                        << ", try to use default";
                    return result::FromError( PerErrc::kStorageNotFound );
                }
            }
        }
    }

    core::Result< void > CPersistencyManager::RecoverKeyValueStorage( const core::InstanceSpecifier &kvs ) noexcept
    {
        using result = core::Result< void >;

        if ( !m_bInitialized ) return result::FromError( PerErrc::kNotInitialized );

        auto&& it = m_kvsMap.find( kvs.ToString().data() );

        if ( it != m_kvsMap.end() ) {
            return it->second->RecoverKeyValueStorage();
        } else {
            return result::FromError( PerErrc::kStorageNotFound );
        }
    }

    core::Result< void > CPersistencyManager::ResetKeyValueStorage( const core::InstanceSpecifier &kvs ) noexcept
    {
        using result = core::Result< void >;

        if ( !m_bInitialized ) return result::FromError( PerErrc::kNotInitialized );

        auto&& it = m_kvsMap.find( kvs.ToString().data() );

        if ( it != m_kvsMap.end() ) {
            return it->second->ResetKeyValueStorage();
        } else {
            return result::FromError( PerErrc::kStorageNotFound );
        }
    }

    core::Result< core::UInt64 > CPersistencyManager::GetCurrentKeyValueStorageSize( const core::InstanceSpecifier & ) noexcept
    {
        using result = core::Result< core::UInt64 >;

        return result::FromValue( static_cast< core::UInt64 >( 0 ) );
    }


    CPersistencyManager::CPersistencyManager() noexcept
    {
        ;
    }
    
    CPersistencyManager::~CPersistencyManager() noexcept
    {
        uninitialize();
    }

    core::Bool CPersistencyManager::parseFromConfig( core::StringView ) noexcept
    {
        return true;
    }

    // ========================================================================
    // Lifecycle Management Implementation (Phase 2.2)
    // ========================================================================

    core::String CPersistencyManager::generateStoragePath(
        const core::InstanceSpecifier& spec,
        StorageType type
    ) noexcept {
        core::StringView instanceId = spec.ToString();
        
        if (type == StorageType::kFileStorage) {
            return CStoragePathManager::getFileStorageInstancePath(instanceId);
        } else {
            return CStoragePathManager::getKvsInstancePath(instanceId);
        }
    }

    core::Result<PersistencyConfig> CPersistencyManager::loadPersistencyConfig() noexcept {
        using result = core::Result<PersistencyConfig>;
        
        try {
            auto& configMgr = core::ConfigManager::getInstance();
            auto moduleConfig = configMgr.getModuleConfigJson("persistency");
            
            if (moduleConfig.is_null() || moduleConfig.empty()) {
                LAP_PER_LOG_WARN << "Persistency module config not found, using defaults";
                return result::FromValue(m_config);
            }
            
            PersistencyConfig config;
            
            // Load configuration fields
            config.centralStorageURI = moduleConfig.value("centralStorageURI", "/tmp/autosar_persistency");
            config.replicaCount = moduleConfig.value("replicaCount", core::UInt32(3));
            config.minValidReplicas = moduleConfig.value("minValidReplicas", core::UInt32(2));
            config.checksumType = moduleConfig.value("checksumType", "CRC32");
            config.contractVersion = moduleConfig.value("contractVersion", "1.0.0");
            config.deploymentVersion = moduleConfig.value("deploymentVersion", "1.0.0");
            config.redundancyHandling = moduleConfig.value("redundancyHandling", "KEEP_REDUNDANCY");
            config.updateStrategy = moduleConfig.value("updateStrategy", "KEEP_LAST_VALID");
            
            // Load KVS config
            auto kvsConfigJson = moduleConfig.value("kvs", nlohmann::json::object());
            config.kvs.backendType = kvsConfigJson.value("backendType", "file");
            config.kvs.dataSourceType = kvsConfigJson.value("dataSourceType", "");
            
            // Load Property backend specific config
            config.kvs.propertyBackendShmSize = kvsConfigJson.value("propertyBackendShmSize", 1ul << 20);  // 1MB default
            config.kvs.propertyBackendPersistence = kvsConfigJson.value("propertyBackendPersistence", "file");
            
            return result::FromValue(config);
        } catch (const std::exception& e) {
            LAP_PER_LOG_ERROR << "Failed to load persistency config: " << e.what();
            return result::FromError(MakeErrorCode(PerErrc::kPhysicalStorageFailure, 0));
        }
    }

    core::Result<void> CPersistencyManager::validateConfig(const PersistencyConfig& config) noexcept {
        using result = core::Result<void>;
        
        // Validate replica configuration
        if (config.minValidReplicas > config.replicaCount) {
            LAP_PER_LOG_ERROR << "minValidReplicas (" << config.minValidReplicas 
                             << ") cannot be greater than replicaCount (" << config.replicaCount << ")";
            return result::FromError(MakeErrorCode(PerErrc::kInvalidArgument, 0));
        }
        
        if (config.replicaCount == 0) {
            LAP_PER_LOG_ERROR << "replicaCount cannot be zero";
            return result::FromError(MakeErrorCode(PerErrc::kInvalidArgument, 0));
        }
        
        // Validate checksum type
        if (config.checksumType != "CRC32" && config.checksumType != "SHA256") {
            LAP_PER_LOG_ERROR << "Invalid checksumType: " << config.checksumType;
            return result::FromError(MakeErrorCode(PerErrc::kInvalidArgument, 0));
        }
        
        return result::FromValue();
    }

    core::Result<void> CPersistencyManager::updateConfig(const PersistencyConfig& config) noexcept {
        using result = core::Result<void>;
        
        // Validate first
        auto validateResult = validateConfig(config);
        if (!validateResult.HasValue()) {
            return validateResult;
        }
        
        try {
            auto& configMgr = core::ConfigManager::getInstance();
            auto moduleConfig = configMgr.getModuleConfigJson("persistency");
            
            // Update configuration fields
            moduleConfig["centralStorageURI"] = config.centralStorageURI;
            moduleConfig["replicaCount"] = config.replicaCount;
            moduleConfig["minValidReplicas"] = config.minValidReplicas;
            moduleConfig["checksumType"] = config.checksumType;
            moduleConfig["contractVersion"] = config.contractVersion;
            moduleConfig["deploymentVersion"] = config.deploymentVersion;
            moduleConfig["redundancyHandling"] = config.redundancyHandling;
            moduleConfig["updateStrategy"] = config.updateStrategy;
            
            // Update KVS config
            nlohmann::json kvsConfig;
            kvsConfig["backendType"] = config.kvs.backendType;
            kvsConfig["dataSourceType"] = config.kvs.dataSourceType;
            kvsConfig["propertyBackendShmSize"] = config.kvs.propertyBackendShmSize;
            kvsConfig["propertyBackendPersistence"] = config.kvs.propertyBackendPersistence;
            moduleConfig["kvs"] = kvsConfig;
            
            // ConfigManager automatically handles persistence
            m_config = config;
            m_configLoaded = true;
            
            return result::FromValue();
        } catch (const std::exception& e) {
            LAP_PER_LOG_ERROR << "Failed to update persistency config: " << e.what();
            return result::FromError(MakeErrorCode(PerErrc::kPhysicalStorageFailure, 0));
        }
    }

    core::Result<FileStorageMetadata> CPersistencyManager::loadMetadata(const core::String& storagePath) noexcept {
        using result = core::Result<FileStorageMetadata>;
        
        // Check cache first
        auto cacheIt = m_metadataCache.find(storagePath);
        if (cacheIt != m_metadataCache.end()) {
            return result::FromValue(cacheIt->second);
        }
        
        // Build metadata file path
        core::String metadataDir = core::Path::appendString(storagePath, LAP_PER_METADATA_DIR);
        core::String metadataFile = core::Path::appendString(metadataDir, LAP_PER_STORAGE_INFO_FILE);
        
        if (!core::File::Util::exists(metadataFile)) {
            LAP_PER_LOG_INFO << "Metadata file does not exist: " << metadataFile;
            return result::FromError(MakeErrorCode(PerErrc::kKeyNotFound, 0));
        }
        
        // Read metadata file using Core::File
        core::Vector<core::UInt8> fileData;
        if (!core::File::Util::ReadBinary(metadataFile.data(), fileData)) {
            LAP_PER_LOG_ERROR << "Failed to read metadata file: " << metadataFile;
            return result::FromError(MakeErrorCode(PerErrc::kPhysicalStorageFailure, 0));
        }
        
        // Parse JSON (simplified - use proper JSON library in production)
        core::String jsonContent(fileData.begin(), fileData.end());
        FileStorageMetadata metadata;
        
        // Simple JSON parsing (extract key-value pairs)
        auto extractString = [&jsonContent](const core::String& key) -> core::String {
            core::String searchKey = "\"" + key + "\": \"";
            size_t pos = jsonContent.find(searchKey);
            if (pos == core::String::npos) return "";
            pos += searchKey.length();
            size_t endPos = jsonContent.find("\"", pos);
            if (endPos == core::String::npos) return "";
            return jsonContent.substr(pos, endPos - pos);
        };
        
        auto extractInt = [&jsonContent](const core::String& key) -> core::UInt32 {
            core::String searchKey = "\"" + key + "\": ";
            size_t pos = jsonContent.find(searchKey);
            if (pos == core::String::npos) return 0;
            pos += searchKey.length();
            size_t endPos = jsonContent.find_first_of(",\n}", pos);
            if (endPos == core::String::npos) return 0;
            core::String valueStr = jsonContent.substr(pos, endPos - pos);
            return static_cast<core::UInt32>(std::stoul(valueStr));
        };
        
        auto extractBool = [&jsonContent](const core::String& key) -> bool {
            core::String searchKey = "\"" + key + "\": ";
            size_t pos = jsonContent.find(searchKey);
            if (pos == core::String::npos) return false;
            pos += searchKey.length();
            return jsonContent.substr(pos, 4) == "true";
        };
        
        // Extract metadata fields
        metadata.contractVersion = extractString("contractVersion");
        metadata.deploymentVersion = extractString("deploymentVersion");
        metadata.manifestVersion = extractString("manifestVersion");
        metadata.storageUri = extractString("storageUri");
        metadata.deploymentUri = extractString("deploymentUri");
        metadata.minimumSustainedSize = extractInt("minimumSustainedSize");
        metadata.maximumAllowedSize = extractInt("maximumAllowedSize");
        metadata.state = static_cast<StorageState>(extractInt("state"));
        metadata.replicaCount = extractInt("replicaCount");
        metadata.minValidReplicas = extractInt("minValidReplicas");
        metadata.checksumType = static_cast<ChecksumType>(extractInt("checksumType"));
        metadata.encryptionEnabled = extractBool("encryptionEnabled");
        metadata.encryptionAlgorithm = extractString("encryptionAlgorithm");
        metadata.encryptionKeyId = extractString("encryptionKeyId");
        metadata.creationTime = extractString("creationTime");
        metadata.lastUpdateTime = extractString("lastUpdateTime");
        metadata.lastAccessTime = extractString("lastAccessTime");
        metadata.backupExists = extractBool("backupExists");
        metadata.backupVersion = extractString("backupVersion");
        metadata.backupCreationTime = extractString("backupCreationTime");
        
        // Set defaults if not found
        if (metadata.replicaCount == 0) metadata.replicaCount = 3;
        if (metadata.minValidReplicas == 0) metadata.minValidReplicas = 2;
        if (metadata.checksumType == static_cast<ChecksumType>(0)) {
            metadata.checksumType = ChecksumType::kCRC32;
        }
        
        // Cache metadata
        m_metadataCache[storagePath] = metadata;
        
        return result::FromValue(metadata);
    }

    core::Result<void> CPersistencyManager::saveMetadata(
        const core::String& storagePath,
        const FileStorageMetadata& meta
    ) noexcept {
        using result = core::Result<void>;
        
        // Build metadata file path
        core::String metadataDir = core::Path::appendString(storagePath, LAP_PER_METADATA_DIR);
        core::String metadataFile = core::Path::appendString(metadataDir, LAP_PER_STORAGE_INFO_FILE);
        
        // Create metadata directory if needed
        if (!core::Path::isDirectory(metadataDir)) {
            if (!core::Path::createDirectory(metadataDir)) {
                LAP_PER_LOG_ERROR << "Failed to create metadata directory: " << metadataDir;
                return result::FromError(MakeErrorCode(PerErrc::kPhysicalStorageFailure, 0));
            }
        }
        
        // Build JSON string
        std::ostringstream oss;
        oss << "{\n";
        oss << "  \"contractVersion\": \"" << meta.contractVersion << "\",\n";
        oss << "  \"deploymentVersion\": \"" << meta.deploymentVersion << "\",\n";
        oss << "  \"manifestVersion\": \"" << meta.manifestVersion << "\",\n";
        oss << "  \"storageUri\": \"" << meta.storageUri << "\",\n";
        oss << "  \"deploymentUri\": \"" << meta.deploymentUri << "\",\n";
        oss << "  \"minimumSustainedSize\": " << meta.minimumSustainedSize << ",\n";
        oss << "  \"maximumAllowedSize\": " << meta.maximumAllowedSize << ",\n";
        oss << "  \"state\": " << static_cast<int>(meta.state) << ",\n";
        oss << "  \"replicaCount\": " << meta.replicaCount << ",\n";
        oss << "  \"minValidReplicas\": " << meta.minValidReplicas << ",\n";
        oss << "  \"checksumType\": " << static_cast<int>(meta.checksumType) << ",\n";
        oss << "  \"encryptionEnabled\": " << (meta.encryptionEnabled ? "true" : "false") << ",\n";
        oss << "  \"encryptionAlgorithm\": \"" << meta.encryptionAlgorithm << "\",\n";
        oss << "  \"encryptionKeyId\": \"" << meta.encryptionKeyId << "\",\n";
        oss << "  \"creationTime\": \"" << meta.creationTime << "\",\n";
        oss << "  \"lastUpdateTime\": \"" << meta.lastUpdateTime << "\",\n";
        oss << "  \"lastAccessTime\": \"" << meta.lastAccessTime << "\",\n";
        oss << "  \"backupExists\": " << (meta.backupExists ? "true" : "false") << ",\n";
        oss << "  \"backupVersion\": \"" << meta.backupVersion << "\",\n";
        oss << "  \"backupCreationTime\": \"" << meta.backupCreationTime << "\"\n";
        oss << "}\n";
        
        // Write using Core::File
        core::String jsonContent = oss.str();
        if (!core::File::Util::WriteBinary(
            metadataFile.data(),
            reinterpret_cast<const core::UInt8*>(jsonContent.data()),
            jsonContent.size(),
            true  // overwrite
        )) {
            LAP_PER_LOG_ERROR << "Failed to write metadata file: " << metadataFile;
            return result::FromError(MakeErrorCode(PerErrc::kPhysicalStorageFailure, 0));
        }
        
        // Update cache
        m_metadataCache[storagePath] = meta;
        
        LAP_PER_LOG_INFO << "Metadata saved successfully: " << metadataFile;
        return result::FromValue();
    }

    core::Result<void> CPersistencyManager::updateVersionInfo(
        const core::String& storagePath,
        const core::String& contractVersion,
        const core::String& deploymentVersion
    ) noexcept {
        // Load existing metadata
        auto metadataResult = loadMetadata(storagePath);
        if (!metadataResult.HasValue()) {
            // Create new metadata if doesn't exist
            FileStorageMetadata metadata;
            metadata.storageUri = storagePath;
            metadata.deploymentUri = storagePath;
            metadata.contractVersion = contractVersion;
            metadata.deploymentVersion = deploymentVersion;
            metadata.creationTime = core::Time::GetCurrentTimeISO();
            metadata.lastUpdateTime = core::Time::GetCurrentTimeISO();
            metadata.state = StorageState::kNormal;
            metadata.replicaCount = 3;
            metadata.minValidReplicas = 2;
            metadata.checksumType = ChecksumType::kCRC32;
            
            return saveMetadata(storagePath, metadata);
        }
        
        FileStorageMetadata metadata = metadataResult.Value();
        metadata.contractVersion = contractVersion;
        metadata.deploymentVersion = deploymentVersion;
        metadata.lastUpdateTime = core::Time::GetCurrentTimeISO();
        
        return saveMetadata(storagePath, metadata);
    }

    // ========================================================================
    // Backup Management
    // ========================================================================

    core::Result<void> CPersistencyManager::backupFileStorage(const core::InstanceSpecifier& fs) noexcept {
        using result = core::Result<void>;
        
        if (!m_bInitialized) {
            return result::FromError(PerErrc::kNotInitialized);
        }
        
        // Get storage path
        core::String storagePath = generateStoragePath(fs, StorageType::kFileStorage);
        
        // Load metadata
        auto metadataResult = loadMetadata(storagePath);
        if (!metadataResult.HasValue()) {
            LAP_PER_LOG_ERROR << "Failed to load metadata for backup";
            return result::FromError(metadataResult.Error());
        }
        
        FileStorageMetadata metadata = metadataResult.Value();
        
        // Get FileStorage instance to access backend
        auto fsResult = getFileStorage(fs, false);
        if (!fsResult.HasValue()) {
            LAP_PER_LOG_ERROR << "FileStorage not found for backup";
            return result::FromError(PerErrc::kStorageNotFound);
        }
        
        auto fileStorage = fsResult.Value();
        auto* backend = fileStorage->GetBackend();
        if (!backend) {
            LAP_PER_LOG_ERROR << "Backend not available for backup";
            return result::FromError(PerErrc::kNotInitialized);
        }
        
        // List all files in current category
        auto filesResult = backend->ListFiles(LAP_PER_CATEGORY_CURRENT);
        if (!filesResult.HasValue()) {
            LAP_PER_LOG_ERROR << "Failed to list files for backup";
            return result::FromError(filesResult.Error());
        }
        
        auto& files = filesResult.Value();
        LAP_PER_LOG_INFO << "Creating backup of " << files.size() << " files";
        
        core::UInt32 successCount = 0;
        core::UInt32 failureCount = 0;
        
        // Copy each file from current to backup
        for (const auto& fileName : files) {
            // Read from current
            auto readResult = backend->ReadFile(fileName, LAP_PER_CATEGORY_CURRENT);
            if (!readResult.HasValue()) {
                LAP_PER_LOG_ERROR << "Failed to read file for backup: " << fileName;
                failureCount++;
                continue;
            }
            
            // Write to backup
            auto writeResult = backend->WriteFile(fileName, readResult.Value(), LAP_PER_CATEGORY_BACKUP);
            if (writeResult.HasValue()) {
                successCount++;
                LAP_PER_LOG_DEBUG << "Backed up file: " << fileName;
            } else {
                LAP_PER_LOG_ERROR << "Failed to write file to backup: " << fileName;
                failureCount++;
            }
        }
        
        // Update metadata
        metadata.backupExists = true;
        metadata.backupVersion = metadata.deploymentVersion;
        metadata.backupCreationTime = core::Time::GetCurrentTimeISO();
        
        auto saveResult = saveMetadata(storagePath, metadata);
        if (!saveResult.HasValue()) {
            LAP_PER_LOG_ERROR << "Failed to save metadata after backup";
            return saveResult;
        }
        
        LAP_PER_LOG_INFO << "Backup completed: " << successCount << " succeeded, " << failureCount << " failed";
        
        if (failureCount > 0 && successCount == 0) {
            return result::FromError(MakeErrorCode(PerErrc::kPhysicalStorageFailure, 0));
        }
        
        return result::FromValue();
    }

    core::Result<void> CPersistencyManager::restoreFileStorage(const core::InstanceSpecifier& fs) noexcept {
        using result = core::Result<void>;
        
        if (!m_bInitialized) {
            return result::FromError(PerErrc::kNotInitialized);
        }
        
        // Get storage path
        core::String storagePath = generateStoragePath(fs, StorageType::kFileStorage);
        
        // Load metadata
        auto metadataResult = loadMetadata(storagePath);
        if (!metadataResult.HasValue()) {
            LAP_PER_LOG_ERROR << "Failed to load metadata for restore";
            return result::FromError(metadataResult.Error());
        }
        
        FileStorageMetadata metadata = metadataResult.Value();
        
        if (!metadata.backupExists) {
            LAP_PER_LOG_ERROR << "No backup available for restore";
            return result::FromError(MakeErrorCode(PerErrc::kIllegalWriteAccess, 0));
        }
        
        // Get FileStorage instance
        auto fsResult = getFileStorage(fs, false);
        if (!fsResult.HasValue()) {
            LAP_PER_LOG_ERROR << "FileStorage not found for restore";
            return result::FromError(PerErrc::kStorageNotFound);
        }
        
        auto fileStorage = fsResult.Value();
        auto* backend = fileStorage->GetBackend();
        if (!backend) {
            LAP_PER_LOG_ERROR << "Backend not available for restore";
            return result::FromError(PerErrc::kNotInitialized);
        }
        
        // Update state
        metadata.state = StorageState::kRecovering;
        saveMetadata(storagePath, metadata);
        
        // List files in backup
        auto filesResult = backend->ListFiles(LAP_PER_CATEGORY_BACKUP);
        if (!filesResult.HasValue()) {
            LAP_PER_LOG_ERROR << "Failed to list backup files";
            metadata.state = StorageState::kCorrupted;
            saveMetadata(storagePath, metadata);
            return result::FromError(filesResult.Error());
        }
        
        auto& files = filesResult.Value();
        LAP_PER_LOG_INFO << "Restoring " << files.size() << " files from backup";
        
        // Delete all current files first
        auto currentFilesResult = backend->ListFiles(LAP_PER_CATEGORY_CURRENT);
        if (currentFilesResult.HasValue()) {
            for (const auto& fileName : currentFilesResult.Value()) {
                backend->DeleteFile(fileName, LAP_PER_CATEGORY_CURRENT);
            }
        }
        
        core::UInt32 successCount = 0;
        core::UInt32 failureCount = 0;
        
        // Copy each file from backup to current
        for (const auto& fileName : files) {
            // Read from backup
            auto readResult = backend->ReadFile(fileName, LAP_PER_CATEGORY_BACKUP);
            if (!readResult.HasValue()) {
                LAP_PER_LOG_ERROR << "Failed to read file from backup: " << fileName;
                failureCount++;
                continue;
            }
            
            // Write to current
            auto writeResult = backend->WriteFile(fileName, readResult.Value(), LAP_PER_CATEGORY_CURRENT);
            if (writeResult.HasValue()) {
                successCount++;
                LAP_PER_LOG_DEBUG << "Restored file: " << fileName;
            } else {
                LAP_PER_LOG_ERROR << "Failed to write file to current: " << fileName;
                failureCount++;
            }
        }
        
        if (failureCount > 0 && successCount == 0) {
            LAP_PER_LOG_ERROR << "Restore failed: no files restored";
            metadata.state = StorageState::kCorrupted;
            saveMetadata(storagePath, metadata);
            return result::FromError(MakeErrorCode(PerErrc::kPhysicalStorageFailure, 0));
        }
        
        // Restore version information
        metadata.deploymentVersion = metadata.backupVersion;
        metadata.state = StorageState::kNormal;
        metadata.lastUpdateTime = core::Time::GetCurrentTimeISO();
        
        auto saveResult = saveMetadata(storagePath, metadata);
        if (!saveResult.HasValue()) {
            LAP_PER_LOG_ERROR << "Failed to save metadata after restore";
            return saveResult;
        }
        
        LAP_PER_LOG_INFO << "Backup restored: " << successCount << " succeeded, " << failureCount << " failed";
        return result::FromValue();
    }

    core::Result<void> CPersistencyManager::removeBackup(const core::InstanceSpecifier& fs) noexcept {
        using result = core::Result<void>;
        
        if (!m_bInitialized) {
            return result::FromError(PerErrc::kNotInitialized);
        }
        
        // Get storage path
        core::String storagePath = generateStoragePath(fs, StorageType::kFileStorage);
        
        // Load metadata
        auto metadataResult = loadMetadata(storagePath);
        if (!metadataResult.HasValue()) {
            // No metadata means no backup
            return result::FromValue();
        }
        
        FileStorageMetadata metadata = metadataResult.Value();
        
        if (!metadata.backupExists) {
            LAP_PER_LOG_INFO << "No backup to remove";
            return result::FromValue();
        }
        
        // Get FileStorage instance
        auto fsResult = getFileStorage(fs, false);
        if (fsResult.HasValue()) {
            auto fileStorage = fsResult.Value();
            auto* backend = fileStorage->GetBackend();
            if (backend) {
                // List and delete all backup files
                auto filesResult = backend->ListFiles(LAP_PER_CATEGORY_BACKUP);
                if (filesResult.HasValue()) {
                    for (const auto& fileName : filesResult.Value()) {
                        backend->DeleteFile(fileName, LAP_PER_CATEGORY_BACKUP);
                    }
                }
            }
        }
        
        // Update metadata
        metadata.backupExists = false;
        metadata.backupVersion.clear();
        metadata.backupCreationTime.clear();
        
        return saveMetadata(storagePath, metadata);
    }

    core::Result<core::Bool> CPersistencyManager::backupExists(const core::InstanceSpecifier& fs) const noexcept {
        using result = core::Result<core::Bool>;
        
        if (!m_bInitialized) {
            return result::FromError(PerErrc::kNotInitialized);
        }
        
        // Get storage path
        core::String storagePath = const_cast<CPersistencyManager*>(this)->generateStoragePath(fs, StorageType::kFileStorage);
        
        // Check cache first
        auto cacheIt = m_metadataCache.find(storagePath);
        if (cacheIt != m_metadataCache.end()) {
            return result::FromValue(cacheIt->second.backupExists);
        }
        
        // Load metadata
        auto metadataResult = const_cast<CPersistencyManager*>(this)->loadMetadata(storagePath);
        if (!metadataResult.HasValue()) {
            return result::FromValue(false);
        }
        
        return result::FromValue(metadataResult.Value().backupExists);
    }

    // ========================================================================
    // Update Management
    // ========================================================================

    core::Result<core::Bool> CPersistencyManager::needsUpdate(
        const core::InstanceSpecifier& fs,
        const core::String& manifestDeploymentVersion,
        const core::String& manifestContractVersion
    ) noexcept {
        using result = core::Result<core::Bool>;
        
        if (!m_bInitialized) {
            return result::FromError(PerErrc::kNotInitialized);
        }
        
        // Get storage path
        core::String storagePath = generateStoragePath(fs, StorageType::kFileStorage);
        
        // Load metadata
        auto metadataResult = loadMetadata(storagePath);
        if (!metadataResult.HasValue()) {
            // No metadata means new storage, needs update
            return result::FromValue(true);
        }
        
        FileStorageMetadata metadata = metadataResult.Value();
        
        // Compare versions (simple string comparison - should use proper version comparison in production)
        bool needsUpdate = false;
        
        if (metadata.contractVersion != manifestContractVersion) {
            LAP_PER_LOG_INFO << "Contract version mismatch: " << metadata.contractVersion 
                            << " vs " << manifestContractVersion;
            needsUpdate = true;
        }
        
        if (metadata.deploymentVersion != manifestDeploymentVersion) {
            LAP_PER_LOG_INFO << "Deployment version mismatch: " << metadata.deploymentVersion 
                            << " vs " << manifestDeploymentVersion;
            needsUpdate = true;
        }
        
        return result::FromValue(needsUpdate);
    }

    core::Result<void> CPersistencyManager::performUpdate(
        const core::InstanceSpecifier& fs,
        const core::String& updatePath
    ) noexcept {
        using result = core::Result<void>;
        
        UNUSED(updatePath);
        
        if (!m_bInitialized) {
            return result::FromError(PerErrc::kNotInitialized);
        }
        
        // Get storage path
        core::String storagePath = generateStoragePath(fs, StorageType::kFileStorage);
        
        // Load metadata
        auto metadataResult = loadMetadata(storagePath);
        if (!metadataResult.HasValue()) {
            LAP_PER_LOG_ERROR << "Failed to load metadata for update";
            return result::FromError(metadataResult.Error());
        }
        
        FileStorageMetadata metadata = metadataResult.Value();
        
        // Check state
        if (metadata.state != StorageState::kNormal) {
            LAP_PER_LOG_ERROR << "Cannot perform update - storage state is not normal";
            return result::FromError(MakeErrorCode(PerErrc::kResourceBusy, 0));
        }
        
        // Create backup before update
        auto backupResult = backupFileStorage(fs);
        if (!backupResult.HasValue()) {
            LAP_PER_LOG_ERROR << "Failed to create backup before update";
            return backupResult;
        }
        
        // Change state to updating
        metadata.state = StorageState::kUpdating;
        saveMetadata(storagePath, metadata);
        
        // TODO: Copy files from updatePath to storage/update/ category
        // This would typically involve:
        // 1. Reading files from updatePath
        // 2. Writing them to storage/update/ using backend
        // 3. Validating checksums
        // For now, we'll just mark as updating
        
        LAP_PER_LOG_INFO << "Update initiated for: " << storagePath;
        return result::FromValue();
    }

    core::Result<void> CPersistencyManager::rollback(const core::InstanceSpecifier& fs) noexcept {
        using result = core::Result<void>;
        
        if (!m_bInitialized) {
            return result::FromError(PerErrc::kNotInitialized);
        }
        
        // Get storage path
        core::String storagePath = generateStoragePath(fs, StorageType::kFileStorage);
        
        // Load metadata
        auto metadataResult = loadMetadata(storagePath);
        if (!metadataResult.HasValue()) {
            LAP_PER_LOG_ERROR << "Failed to load metadata for rollback";
            return result::FromError(metadataResult.Error());
        }
        
        FileStorageMetadata metadata = metadataResult.Value();
        
        LAP_PER_LOG_INFO << "Rolling back update";
        
        // Change state
        metadata.state = StorageState::kRollingBack;
        saveMetadata(storagePath, metadata);
        
        // Restore from backup
        auto restoreResult = restoreFileStorage(fs);
        if (!restoreResult.HasValue()) {
            LAP_PER_LOG_ERROR << "Failed to restore backup during rollback";
            metadata.state = StorageState::kCorrupted;
            saveMetadata(storagePath, metadata);
            return restoreResult;
        }
        
        // Get FileStorage instance to clear update category
        auto fsResult = getFileStorage(fs, false);
        if (fsResult.HasValue()) {
            auto fileStorage = fsResult.Value();
            auto* backend = fileStorage->GetBackend();
            if (backend) {
                // Clear update category
                auto filesResult = backend->ListFiles(LAP_PER_CATEGORY_UPDATE);
                if (filesResult.HasValue()) {
                    for (const auto& fileName : filesResult.Value()) {
                        backend->DeleteFile(fileName, LAP_PER_CATEGORY_UPDATE);
                    }
                }
            }
        }
        
        // Restore normal state
        metadata.state = StorageState::kNormal;
        auto saveResult = saveMetadata(storagePath, metadata);
        if (!saveResult.HasValue()) {
            LAP_PER_LOG_ERROR << "Failed to save metadata after rollback";
            return saveResult;
        }
        
        LAP_PER_LOG_INFO << "Rollback completed successfully";
        return result::FromValue();
    }

    // ========================================================================
    // Replica Management
    // ========================================================================

    core::Result<core::Vector<ReplicaMetadata>> CPersistencyManager::checkReplicaHealth(
        const core::InstanceSpecifier& fs,
        const core::String& category
    ) noexcept {
        using result = core::Result<core::Vector<ReplicaMetadata>>;
        
        UNUSED(fs);
        UNUSED(category);
        
        if (!m_bInitialized) {
            return result::FromError(PerErrc::kNotInitialized);
        }
        
        // TODO: Implement replica health check
        // This requires CReplicaManager integration
        // For now, return empty vector
        LAP_PER_LOG_WARN << "Replica health check not yet implemented";
        return result::FromValue(core::Vector<ReplicaMetadata>());
    }

    core::Result<core::UInt32> CPersistencyManager::repairReplicas(
        const core::InstanceSpecifier& fs,
        const core::String& category
    ) noexcept {
        using result = core::Result<core::UInt32>;
        
        UNUSED(fs);
        UNUSED(category);
        
        if (!m_bInitialized) {
            return result::FromError(PerErrc::kNotInitialized);
        }
        
        // TODO: Implement replica repair
        // This requires CReplicaManager integration
        // For now, return 0
        LAP_PER_LOG_WARN << "Replica repair not yet implemented";
        return result::FromValue(core::UInt32(0));
    }

    // ========================================================================
    // Helper Methods
    // ========================================================================

    core::Result<CFileStorageBackend*> CPersistencyManager::getFileStorageBackend(
        const core::InstanceSpecifier& fs
    ) noexcept {
        using result = core::Result<CFileStorageBackend*>;
        
        auto fsResult = getFileStorage(fs, false);
        if (!fsResult.HasValue()) {
            return result::FromError(PerErrc::kStorageNotFound);
        }
        
        auto fileStorage = fsResult.Value();
        auto* backend = fileStorage->GetBackend();
        if (!backend) {
            return result::FromError(PerErrc::kNotInitialized);
        }
        
        return result::FromValue(backend);
    }
}
}