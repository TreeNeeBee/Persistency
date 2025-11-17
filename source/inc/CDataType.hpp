/**
 * @file CDataType.hpp
 * @author ddkv587 (ddkv587@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2024-02-02
 * 
 * 
 */
#ifndef LAP_PERSISTENCY_DATATYPE_HPP
#define LAP_PERSISTENCY_DATATYPE_HPP

#include <sstream>

// core
#include <lap/core/CTypedef.hpp>
#include <lap/core/CString.hpp>
#include <lap/core/CVariant.hpp>
#include <lap/log/CLog.hpp>

// persistency common
#include "CPerErrorDomain.hpp"

namespace lap 
{
namespace per 
{
    // ========================================================================
    // Checksum/Hash Types for Integrity Verification
    // ========================================================================
    /**
     * @brief Checksum algorithm type
     */
    enum class ChecksumType : core::UInt8 {
        kCRC32 = 0,     // Fast, suitable for error detection
        kSHA256 = 1     // Cryptographically secure, slower
    };

    /**
     * @brief Checksum result structure
     */
    struct ChecksumResult {
        ChecksumType type;
        core::String value;         // Hex string representation
        core::UInt64 calculationTime; // Time taken in microseconds
    };

    // ========================================================================
    // Logging Configuration
    // ========================================================================
    #define LAP_PER_LOG_CONTEXT_ID       "PM"
    #define LAP_PER_LOG_CONTEXT_DESC     "PM log ctx"

    #define LAP_DEBUG

#ifdef LAP_DEBUG
    #define LAP_PER_LOG                  LAP_LOG( LAP_PER_LOG_CONTEXT_ID, LAP_PER_LOG_CONTEXT_DESC, ::lap::log::LogLevel::kVerbose )
    #define LAP_PER_LOG_VERBOSE          LAP_PER_LOG.LogVerbose().WithLocation( __FILE__, __LINE__ )
    #define LAP_PER_LOG_DEBUG            LAP_PER_LOG.LogDebug().WithLocation( __FILE__, __LINE__ )
    #define LAP_PER_LOG_INFO             LAP_PER_LOG.LogInfo().WithLocation( __FILE__, __LINE__ )
#else
    #define LAP_PER_LOG                  LAP_LOG( LAP_PER_LOG_CONTEXT_ID, LAP_PER_LOG_CONTEXT_DESC, ::lap::log::LogLevel::kWarn )
    #define LAP_PER_LOG_VERBOSE          LAP_PER_LOG.LogOff()
    #define LAP_PER_LOG_DEBUG            LAP_PER_LOG.LogOff()
    #define LAP_PER_LOG_INFO             LAP_PER_LOG.LogOff()
#endif
    #define LAP_PER_LOG_WARN             LAP_PER_LOG.LogWarn().WithLocation( __FILE__, __LINE__ )
    #define LAP_PER_LOG_ERROR            LAP_PER_LOG.LogError().WithLocation( __FILE__, __LINE__ )
    #define LAP_PER_LOG_FATAL            LAP_PER_LOG.LogFatal().WithLocation( __FILE__, __LINE__ )

    // ========================================================================
    // AUTOSAR File Storage Default Configuration
    // ========================================================================
    // Storage size limits (in bytes)
    #define LAP_PER_DEFAULT_MIN_SUSTAINED_SIZE      (1024ULL * 1024ULL)          // 1 MB
    #define LAP_PER_DEFAULT_MAX_ALLOWED_SIZE        (100ULL * 1024ULL * 1024ULL) // 100 MB
    
    // File count limits
    #define LAP_PER_DEFAULT_MAX_FILE_COUNT          1000U
    
    // Default version strings
    #define LAP_PER_DEFAULT_VERSION                 "1.0.0"
    
    // Directory configuration
    #define LAP_PER_METADATA_DIR                    ".metadata"
    #define LAP_PER_REPLICA_DIR_PREFIX              "replica_"
    
    // Replica configuration (M-out-of-N redundancy)
    #define LAP_PER_DEFAULT_REPLICA_COUNT           3U      // N: Total number of replicas
    #define LAP_PER_MIN_VALID_REPLICAS              2U      // M: Minimum valid replicas required
    
    // Checksum/Hash configuration
    #define LAP_PER_CHECKSUM_TYPE_CRC32             "CRC32"
    #define LAP_PER_CHECKSUM_TYPE_SHA256            "SHA256"
    #define LAP_PER_DEFAULT_CHECKSUM_TYPE           LAP_PER_CHECKSUM_TYPE_CRC32
    
    // Metadata file names
    #define LAP_PER_STORAGE_INFO_FILE               "storage_info.json"
    #define LAP_PER_PARTITION_INFO_FILE             "partition_info.json"
    #define LAP_PER_FILE_REGISTRY_FILE              "file_registry.json"
    
    // Storage categories
    #define LAP_PER_CATEGORY_CURRENT                "current"
    #define LAP_PER_CATEGORY_BACKUP                 "backup"
    #define LAP_PER_CATEGORY_INITIAL                "initial"
    #define LAP_PER_CATEGORY_UPDATE                 "update"

    enum class OpenMode : core::UInt32
    {
        kAtTheBeginning = 1 << 0,
        kAtTheEnd       = 1 << 1,
        kTruncate       = 1 << 2,
        kAppend         = 1 << 3,
        kBinary         = 1 << 4,
        kIn             = 1 << 5,
        kOut            = 1 << 6,  // Fixed: was 1 << 5, should be different from kIn
        kEnd            = 1 << 16
    };

    constexpr OpenMode operator| ( OpenMode left, OpenMode right )
    {
        return static_cast< OpenMode > ( static_cast< typename std::underlying_type_t< OpenMode > >( left ) | 
                                        static_cast< typename std::underlying_type_t< OpenMode > >( right ) );
    }
    OpenMode& operator|= ( OpenMode &left, const OpenMode &right );

    constexpr typename std::underlying_type_t< OpenMode > operator& ( OpenMode left, OpenMode right )
    {
        return ( static_cast< typename std::underlying_type_t< OpenMode > >( left ) & 
                static_cast< typename std::underlying_type_t< OpenMode > >( right ) );
    }

    constexpr ::std::ios_base::openmode convert( OpenMode mode ) noexcept
    {
        ::std::ios_base::openmode modeBase{ ::std::ios_base::openmode::_S_ios_openmode_end };

        if ( mode & OpenMode::kAtTheBeginning ) {
            // no need do anything
        }

        if ( mode & OpenMode::kAtTheEnd )       modeBase |= ::std::ios_base::ate;
        if ( mode & OpenMode::kTruncate )       modeBase |= ::std::ios_base::trunc;
        if ( mode & OpenMode::kAppend )         modeBase |= ::std::ios_base::app;
        if ( mode & OpenMode::kBinary )         modeBase |= ::std::ios_base::binary;
        if ( mode & OpenMode::kIn )             modeBase |= ::std::ios_base::in;
        if ( mode & OpenMode::kOut )            modeBase |= ::std::ios_base::out;

        return modeBase;
    }

    constexpr OpenMode convert( ::std::ios_base::openmode mode ) noexcept
    {
        OpenMode modeBase{ OpenMode::kEnd };

        if ( mode & ::std::ios_base::ate )      modeBase |= OpenMode::kAtTheEnd;
        if ( mode & ::std::ios_base::trunc )    modeBase |= OpenMode::kTruncate;
        if ( mode & ::std::ios_base::app )      modeBase |= OpenMode::kAppend;
        if ( mode & ::std::ios_base::binary )   modeBase |= OpenMode::kBinary;
        if ( mode & ::std::ios_base::in )       modeBase |= OpenMode::kIn;
        if ( mode & ::std::ios_base::out )      modeBase |= OpenMode::kOut;

        return modeBase;
    }

    constexpr inline core::Bool valid( OpenMode mode ) noexcept
    {
        // false if 
        //      kAtTheBeginning combined with kAtTheEnd
        //      kTruncate combined with kAtTheEnd

        if ( ( mode & OpenMode::kAtTheEnd ) && ( ( mode & OpenMode::kAtTheBeginning ) || ( mode & OpenMode::kTruncate ) ) ) {
            return false;
        }

        return true;
    }

    enum class FileCreationState : core::UInt32
    {
        kCreatedDuringInstallion    = 1,
        kCreatedDuringUpdate        = 2,
        kCreatedDuringReset         = 3,
        kCreatedDuringRecovery      = 4,
        kCreatedByApplication       = 5
    };

    enum class FileModificationState : core::UInt32 
    {
        kModifiedDuringUpdate       = 2,
        kModifiedDuringReset        = 3,
        kModifiedDuringRecovery     = 4,
        kModifiedByApplication      = 5
    };

    enum class FileCRCType : core::UInt32 
    {
        None                    = 1,
        Header                  = 2,
        Total                   = 3,
    };

    enum class LevelUpdateStrategy : core::UInt32 
    {
        Delete                  = 2,
        KeepExisting            = 1,
        Overwrite               = 0,
    };

    enum class RedundancyStrategy : core::UInt32 
    {
        None                    = 1,
        Redundant               = 0,
        RedundantPerElement     = 2,
    };

    struct FileInfo
    {
        core::UInt64                    creationTime;
        core::UInt64                    modificationTime;
        core::UInt64                    accessTime;
        core::Size                      fileSize;
        FileCreationState               fileCreationState;
        FileModificationState           fileModificationState;
    };

    enum class Origin : core::UInt32 
    {
        kBeginning  = 0,
        kCurrent    = 1,
        kEnd        = 2
    };

    constexpr std::ios_base::seekdir convert( Origin pos ) noexcept
    {
        switch( pos ) {
        case Origin::kBeginning:
            return ::std::ios_base::beg;
        case Origin::kCurrent:
            return ::std::ios_base::cur;
        case Origin::kEnd:
            return ::std::ios_base::end;
        }
        return ::std::ios_base::beg;
    }

    using KvsDataType = core::Variant< \
                            core::Int8, core::UInt8, \
                            core::Int16, core::UInt16, \
                            core::Int32, core::UInt32, \
                            core::Int64, core::UInt64, \
                            core::Bool, \
                            core::Float, core::Double, \
                            core::String >;

    enum class EKvsDataTypeIndicate : core::UInt32
    { 
        DataType_int8_t         = 0,
        DataType_uint8_t        = 1,
        DataType_int16_t        = 2,
        DataType_uint16_t       = 3,
        DataType_int32_t        = 4,
        DataType_uint32_t       = 5,
        DataType_int64_t        = 6,
        DataType_uint64_t       = 7,
        DataType_bool           = 8,
        DataType_float          = 9,
        DataType_double         = 10,
        DataType_string         = 11,
    };

    constexpr core::Bool operator== ( EKvsDataTypeIndicate left, EKvsDataTypeIndicate right )
    {
        return static_cast< core::UInt32 >( left ) == static_cast< core::UInt32 >( right );
    }

    constexpr core::Bool operator!= ( EKvsDataTypeIndicate left, EKvsDataTypeIndicate right )
    {
        return static_cast< core::UInt32 >( left ) != static_cast< core::UInt32 >( right );
    }

    constexpr core::Bool operator== ( core::Int32 left, EKvsDataTypeIndicate right )
    {
        return static_cast< core::UInt32 >( left ) == static_cast< core::UInt32 >( right );
    }

    constexpr core::Bool operator!= ( core::Int32 left, EKvsDataTypeIndicate right )
    {
        return static_cast< core::UInt32 >( left ) != static_cast< core::UInt32 >( right );
    }

    constexpr core::Bool operator> ( EKvsDataTypeIndicate left, EKvsDataTypeIndicate right )
    {
        return static_cast< core::UInt32 >( left ) > static_cast< core::UInt32 >( right );
    }

    core::String kvsToStrig( const KvsDataType& value );
    KvsDataType kvsFromString( const core::String &value, const EKvsDataTypeIndicate &type );

    enum class KvsBackendType : core::UInt32 
    {
        kvsNone             = 0,        // No persistence backend (memory-only)
        kvsFile             = 1 << 16,
        kvsSqlite           = 1 << 17,
        kvsProperty         = 1 << 18
    };

    constexpr KvsBackendType operator| ( KvsBackendType left, KvsBackendType right )
    {
        return static_cast< KvsBackendType >( static_cast< typename std::underlying_type< KvsBackendType >::type >( left ) |
                                                static_cast< typename std::underlying_type< KvsBackendType >::type >( right ) );
    }

    constexpr typename std::underlying_type< KvsBackendType >::type operator& ( KvsBackendType left, KvsBackendType right )
    {
        return ( static_cast< typename std::underlying_type< KvsBackendType >::type >( left ) & 
                    static_cast< typename std::underlying_type< KvsBackendType >::type >( right ) );
    }

    // ========================================================================
    // Storage Type Enumeration
    // ========================================================================
    
    /**
     * @brief Storage type enumeration for path generation
     */
    enum class StorageType : core::UInt8 {
        kKeyValueStorage = 0,  // KVS storage
        kFileStorage = 1       // File storage
    };

    // ========================================================================
    // Persistency Configuration Structure
    // ========================================================================
    
    /**
     * @brief Persistency module configuration structure
     * Loaded from Core::ConfigManager "persistency" module
     */
    struct PersistencyConfig {
        core::String centralStorageURI{"/tmp/autosar_persistency"};
        core::UInt32 replicaCount{3};
        core::UInt32 minValidReplicas{2};
        core::String checksumType{"CRC32"};
        core::String contractVersion{"1.0.0"};
        core::String deploymentVersion{"1.0.0"};
        core::String redundancyHandling{"KEEP_REDUNDANCY"};
        core::String updateStrategy{"KEEP_LAST_VALID"};
        
        struct KvsConfig {
            core::String backendType{"file"};
            core::String dataSourceType{""};
            core::Size propertyBackendShmSize{1ul << 20};  // 1MB default for Property backend
            core::String propertyBackendPersistence{"file"};  // "file" or "sqlite"
        } kvs;
    };

    // ========================================================================
    // Storage State Enumeration (used by CPersistencyManager)
    // ========================================================================
    
    /**
     * @brief Storage state for update and recovery management
     */
    enum class StorageState : core::UInt8 {
        kNormal = 0,        // Normal operation
        kUpdating,          // Update in progress
        kRollingBack,       // Rollback in progress
        kCorrupted,         // Data integrity compromised
        kRecovering         // Recovery in progress
    };

    // ========================================================================
    // File Storage Metadata Structure (managed by CPersistencyManager)
    // ========================================================================
    
    /**
     * @brief File Storage metadata for version and integrity management
     * 
     * Stored in: {storageUri}/.metadata/storage_info.json
     */
    struct FileStorageMetadata {
        // Version information (SWS_PER_00463)
        core::String contractVersion;       // Interface version
        core::String deploymentVersion;     // Deployment version
        core::String manifestVersion;       // Manifest version
        
        // Storage configuration
        core::String storageUri;            // Base URI for this storage
        core::String deploymentUri;         // Deployment-specific URI
        core::UInt64 minimumSustainedSize;  // Minimum guaranteed size
        core::UInt64 maximumAllowedSize;    // Maximum allowed size
        
        // Storage state
        StorageState state;                 // Current storage state
        
        // Replica configuration (M-out-of-N redundancy, SWS_PER_00558)
        core::UInt32 replicaCount;          // N: Total number of replicas
        core::UInt32 minValidReplicas;      // M: Minimum valid replicas required
        ChecksumType checksumType;          // Checksum algorithm for integrity
        
        // Encryption configuration (SWS_PER_00559)
        core::Bool encryptionEnabled;
        core::String encryptionAlgorithm;
        core::String encryptionKeyId;
        
        // Timestamps
        core::String creationTime;
        core::String lastUpdateTime;
        core::String lastAccessTime;
        
        // Backup information
        core::Bool backupExists;
        core::String backupVersion;
        core::String backupCreationTime;
    };

    // ========================================================================
    // Storage URI Structure (used by CFileStorageBackend)
    // ========================================================================
    
    /**
     * @brief File Storage URI structure
     * 
     * Format: {baseUri}/{category}/{logicalFileName}
     * Example: /tmp/autosar_persistency_test/fs/instance1/current/config.json
     */
    struct StorageUri {
        core::String baseUri;           // Base storage URI
        core::String category;          // Category (current, backup, initial, update)
        core::String fileName;          // Logical file name
        
        /**
         * @brief Get full file path
         * @return Full path: {baseUri}/{category}/{fileName}
         */
        core::String GetFullPath() const {
            core::String result = baseUri;
            result += "/";
            result += category;
            result += "/";
            result += fileName;
            return result;
        }
        
        /**
         * @brief Get category path
         * @return Category path: {baseUri}/{category}
         */
        core::String GetCategoryPath() const {
            core::String result = baseUri;
            result += "/";
            result += category;
            return result;
        }
    };

} // pm
} // ara

#endif
