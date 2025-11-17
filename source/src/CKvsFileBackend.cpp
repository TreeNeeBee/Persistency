/**
 * @file CKvsFileBackend.cpp
 * @brief JSON File Backend Implementation
 * @version 2.0
 * @date 2025-11-14
 * 
 * @copyright Copyright (c) 2025
 * 
 * @note This implementation follows Core module constraints:
 * - Uses core::File::ReadAllBytes/WriteAllBytes (no std::ifstream/ofstream)
 * - Uses core::Path for path operations
 * - Uses core::Result for error handling
 * - Uses nlohmann/json for proper JSON type support
 */

#include <nlohmann/json.hpp>
#include <lap/core/CFile.hpp>
#include <lap/core/CPath.hpp>
#include "CKvsFileBackend.hpp"
#include "CStoragePathManager.hpp"

namespace lap
{
namespace per
{
    // ==================== IKvsBackend Interface Implementation ====================

    core::Result<core::Vector<core::String>> KvsFileBackend::GetAllKeys() const noexcept
    {
        using result = core::Result<core::Vector<core::String>>;

        if ( !m_bAvailable ) return result::FromError( PerErrc::kNotInitialized );

        core::ReadLockGuard lock(m_rwLock);  // Shared lock for read [SWS_PER_00309]
        
        core::Vector<core::String> keys;
        if (m_kvsRoot.is_object()) {
            for (auto it = m_kvsRoot.begin(); it != m_kvsRoot.end(); ++it) {
                keys.emplace_back(it.key().c_str());
            }
        }

        return result::FromValue( keys );
    }



    core::Result<core::UInt64> KvsFileBackend::GetSize() const noexcept
    {
        using result = core::Result<core::UInt64>;

        if ( !m_bAvailable ) return result::FromError( PerErrc::kNotInitialized );

        // Get file size using core::File
        if (!core::File::Util::exists(m_strFile.c_str())) {
            return result::FromValue(static_cast<core::UInt64>(0));
        }

        // Use core::File::Util to get file size
        core::Vector<core::UInt8> fileData;
        if (!core::File::Util::ReadBinary(m_strFile, fileData)) {
            LAP_PER_LOG_WARN << "KvsFileBackend::GetSize failed to read file: " << m_strFile;
            return result::FromError(PerErrc::kFileNotFound);
        }

        return result::FromValue(static_cast<core::UInt64>(fileData.size()));
    }

    core::Result<core::UInt32> KvsFileBackend::GetKeyCount() const noexcept
    {
        using result = core::Result<core::UInt32>;

        if ( !m_bAvailable ) return result::FromError( PerErrc::kNotInitialized );

        return result::FromValue(static_cast<core::UInt32>(m_kvsRoot.size()));
    }

    core::Result< KvsDataType > KvsFileBackend::GetValue( core::StringView key ) const noexcept
    {
        using result = core::Result< KvsDataType >;

        if ( !m_bAvailable ) return result::FromError( PerErrc::kNotInitialized );

        core::ReadLockGuard lock(m_rwLock);  // Shared lock for read [SWS_PER_00309]
        
        try {
            if (!m_kvsRoot.contains(key.data())) {
                return result::FromError( PerErrc::kKeyNotFound );
            }
            
            const auto& jsonValue = m_kvsRoot[key.data()];
            
            // Convert JSON value to KvsDataType based on stored type
            if (jsonValue.is_object() && jsonValue.contains("type") && jsonValue.contains("value")) {
                // Structured format: {"type": "d", "value": 123}
                core::String typeStr = jsonValue["type"].get<std::string>();
                char typeChar = typeStr.empty() ? 'k' : typeStr[0];
                
                // Convert based on type marker
                switch (static_cast<EKvsDataTypeIndicate>(typeChar - 'a')) {
                    case EKvsDataTypeIndicate::DataType_int8_t:
                        return result::FromValue(KvsDataType{static_cast<core::Int8>(jsonValue["value"].get<int>())});
                    case EKvsDataTypeIndicate::DataType_uint8_t:
                        return result::FromValue(KvsDataType{static_cast<core::UInt8>(jsonValue["value"].get<unsigned>())});
                    case EKvsDataTypeIndicate::DataType_int16_t:
                        return result::FromValue(KvsDataType{jsonValue["value"].get<core::Int16>()});
                    case EKvsDataTypeIndicate::DataType_uint16_t:
                        return result::FromValue(KvsDataType{jsonValue["value"].get<core::UInt16>()});
                    case EKvsDataTypeIndicate::DataType_int32_t:
                        return result::FromValue(KvsDataType{jsonValue["value"].get<core::Int32>()});
                    case EKvsDataTypeIndicate::DataType_uint32_t:
                        return result::FromValue(KvsDataType{jsonValue["value"].get<core::UInt32>()});
                    case EKvsDataTypeIndicate::DataType_int64_t:
                        return result::FromValue(KvsDataType{jsonValue["value"].get<core::Int64>()});
                    case EKvsDataTypeIndicate::DataType_uint64_t:
                        return result::FromValue(KvsDataType{jsonValue["value"].get<core::UInt64>()});
                    case EKvsDataTypeIndicate::DataType_bool:
                        return result::FromValue(KvsDataType{jsonValue["value"].get<bool>()});
                    case EKvsDataTypeIndicate::DataType_float:
                        return result::FromValue(KvsDataType{jsonValue["value"].get<core::Float>()});
                    case EKvsDataTypeIndicate::DataType_double:
                        return result::FromValue(KvsDataType{jsonValue["value"].get<core::Double>()});
                    case EKvsDataTypeIndicate::DataType_string:
                        return result::FromValue(KvsDataType{core::String(jsonValue["value"].get<std::string>().c_str())});
                    default:
                        return result::FromValue(KvsDataType{core::String(jsonValue["value"].get<std::string>().c_str())});
                }
            } else {
                // Legacy format or direct value - treat as string
                if (jsonValue.is_string()) {
                    return result::FromValue(KvsDataType{core::String(jsonValue.get<std::string>().c_str())});
                } else if (jsonValue.is_number_integer()) {
                    return result::FromValue(KvsDataType{jsonValue.get<core::Int32>()});
                } else if (jsonValue.is_number_float()) {
                    return result::FromValue(KvsDataType{jsonValue.get<core::Double>()});
                } else if (jsonValue.is_boolean()) {
                    return result::FromValue(KvsDataType{jsonValue.get<bool>()});
                }
            }
            
            return result::FromError( PerErrc::kDataTypeMismatch );
        } catch (const std::exception& e) {
            LAP_PER_LOG_WARN.logFormat( "KvsFileBackend::GetValue with key[%s] failed: %s!", key.data(), e.what() );
            return result::FromError( PerErrc::kKeyNotFound );
        }
    }

    core::Result<void> KvsFileBackend::SetValue( core::StringView key, const KvsDataType &value ) noexcept
    {
        using result = core::Result<void>;

        if ( !m_bAvailable ) return result::FromError( PerErrc::kNotInitialized );

        core::WriteLockGuard lock(m_rwLock);  // Exclusive lock for write [SWS_PER_00309]
        
        try {
            // Store with type information: {"type": "x", "value": actual_value}
            char typeMarker = static_cast<char>('a' + ::lap::core::GetVariantIndex(value));
            nlohmann::json jsonValue = nlohmann::json::object();
            jsonValue["type"] = std::string(1, typeMarker);
            
            // Store actual value with correct JSON type
            switch (static_cast<EKvsDataTypeIndicate>(::lap::core::GetVariantIndex(value))) {
                case EKvsDataTypeIndicate::DataType_int8_t:
                    jsonValue["value"] = static_cast<int>(::lap::core::get<core::Int8>(value));
                    break;
                case EKvsDataTypeIndicate::DataType_uint8_t:
                    jsonValue["value"] = static_cast<unsigned>(::lap::core::get<core::UInt8>(value));
                    break;
                case EKvsDataTypeIndicate::DataType_int16_t:
                    jsonValue["value"] = ::lap::core::get<core::Int16>(value);
                    break;
                case EKvsDataTypeIndicate::DataType_uint16_t:
                    jsonValue["value"] = ::lap::core::get<core::UInt16>(value);
                    break;
                case EKvsDataTypeIndicate::DataType_int32_t:
                    jsonValue["value"] = ::lap::core::get<core::Int32>(value);
                    break;
                case EKvsDataTypeIndicate::DataType_uint32_t:
                    jsonValue["value"] = ::lap::core::get<core::UInt32>(value);
                    break;
                case EKvsDataTypeIndicate::DataType_int64_t:
                    jsonValue["value"] = ::lap::core::get<core::Int64>(value);
                    break;
                case EKvsDataTypeIndicate::DataType_uint64_t:
                    jsonValue["value"] = ::lap::core::get<core::UInt64>(value);
                    break;
                case EKvsDataTypeIndicate::DataType_bool:
                    jsonValue["value"] = ::lap::core::get<core::Bool>(value);
                    break;
                case EKvsDataTypeIndicate::DataType_float:
                    jsonValue["value"] = ::lap::core::get<core::Float>(value);
                    break;
                case EKvsDataTypeIndicate::DataType_double:
                    jsonValue["value"] = ::lap::core::get<core::Double>(value);
                    break;
                case EKvsDataTypeIndicate::DataType_string:
                    jsonValue["value"] = ::lap::core::get<core::String>(value).c_str();
                    break;
            }
            
            m_kvsRoot[key.data()] = jsonValue;
            m_dirty = true;
        } catch (const std::exception& e) {
            LAP_PER_LOG_WARN.logFormat( "KvsFileBackend::SetValue with ( %s, %s ) failed: %s!", key.data(), kvsToStrig( value ).c_str(), e.what() );
            return result::FromError( PerErrc::kIllegalWriteAccess );
        }

        return result::FromValue();
    }

    // ==================== AUTOSAR Key-Value Storage API ====================

    core::Result<core::Bool> KvsFileBackend::KeyExists(core::StringView key) const noexcept
    {
        using result = core::Result<core::Bool>;

        if ( !m_bAvailable ) return result::FromError( PerErrc::kNotInitialized );

        core::ReadLockGuard lock(m_rwLock);  // Shared lock for read [SWS_PER_00309]
        return result::FromValue(m_kvsRoot.contains(key.data()));
    }

    core::Result<void> KvsFileBackend::RemoveKey(core::StringView key) noexcept
    {
        using result = core::Result<void>;

        if ( !m_bAvailable ) return result::FromError( PerErrc::kNotInitialized );

        core::WriteLockGuard lock(m_rwLock);  // Exclusive lock for write [SWS_PER_00309]
        
        m_kvsRoot.erase( key.data() );
        m_dirty = true;  // Mark as dirty

        return result::FromValue();
    }

    core::Result<void> KvsFileBackend::RecoverKey(core::StringView) noexcept
    {
        LAP_PER_LOG_WARN << "Not support yet";
        // TODO : KvsFileBackend::RecoverKey

        using result = core::Result<void>;
        return result::FromValue();
    }

    core::Result<void> KvsFileBackend::ResetKey( core::StringView ) noexcept
    {
        LAP_PER_LOG_WARN << "Not support yet";
        // TODO : KvsFileBackend::ResetKey

        using result = core::Result<void>;
        return result::FromValue();
    }

    core::Result<void> KvsFileBackend::RemoveAllKeys() noexcept
    {
        using result = core::Result<void>;

        if ( !m_bAvailable ) return result::FromError( PerErrc::kNotInitialized );

        core::WriteLockGuard lock(m_rwLock);  // Exclusive lock for write [SWS_PER_00309]
        
        m_kvsRoot.clear();
        m_dirty = true;  // Mark as dirty

        return result::FromValue();
    }

    core::Result<void> KvsFileBackend::SyncToStorage() noexcept
    {
        core::WriteLockGuard lock(m_rwLock);  // Exclusive lock for write [SWS_PER_00309]
        
        if (!m_dirty) {
            return core::Result<void>::FromValue();  // No changes to sync
        }

        // ==================== AUTOSAR Update Workflow [SWS_PER_00600] ====================
        // Phase 1: Save to update/ directory (not current/)
        core::String updatePath = getUpdatePath();
        LAP_PER_LOG_INFO << "AUTOSAR Workflow - Phase 1: Saving to update/ directory";
        auto saveResult = saveToFile(updatePath);
        if (!saveResult.HasValue()) {
            LAP_PER_LOG_ERROR << "Failed to save to update/ directory";
            return saveResult;
        }
        
        // Phase 2: Validate data integrity [SWS_PER_00800]
        LAP_PER_LOG_INFO << "AUTOSAR Workflow - Phase 2: Validating data integrity";
        auto validateResult = validateDataIntegrity(updatePath);
        if (!validateResult.HasValue()) {
            LAP_PER_LOG_ERROR << "Integrity validation failed, aborting commit";
            core::File::Util::remove(updatePath.data()); // Cleanup invalid update file
            return validateResult;
        }
        
        // Phase 3: Backup current/ to redundancy/ [SWS_PER_00502]
        LAP_PER_LOG_INFO << "AUTOSAR Workflow - Phase 3: Backing up to redundancy/";
        auto backupResult = backupToRedundancy();
        if (!backupResult.HasValue()) {
            LAP_PER_LOG_ERROR << "Backup to redundancy failed, aborting commit";
            core::File::Util::remove(updatePath.data()); // Cleanup update file
            return backupResult;
        }
        
        // Phase 4: Atomic replace [SWS_PER_00600]
        LAP_PER_LOG_INFO << "AUTOSAR Workflow - Phase 4: Atomic commit (update/ -> current/)";
        auto replaceResult = atomicReplaceCurrentWithUpdate();
        if (!replaceResult.HasValue()) {
            LAP_PER_LOG_ERROR << "Atomic replace failed - system state preserved";
            // Note: Rollback can be implemented using DiscardPendingChanges()
            core::File::Util::remove(updatePath.data()); // Cleanup update file
            return replaceResult;
        }
        
        // Success: Mark clean and log completion
        m_dirty = false;
        LAP_PER_LOG_INFO << "AUTOSAR Workflow - Complete: Data committed successfully";
        return core::Result<void>::FromValue();
    }

    core::Result<void> KvsFileBackend::DiscardPendingChanges() noexcept
    {
        using result = core::Result<void>;
        
        if ( !m_bAvailable ) return result::FromValue();

        core::WriteLockGuard lock(m_rwLock);  // Exclusive lock for write [SWS_PER_00309]
        
        auto loadResult = parseFromFile( m_strFile );
        if (loadResult.HasValue()) {
            m_dirty = false;  // Clear dirty flag after reload
        }
        return loadResult;
    }

    // ==================== File I/O Operations (using core::File) ====================

    core::Result<void> KvsFileBackend::parseFromFile( core::StringView strFile ) noexcept
    {
        using result = core::Result<void>;

        // Check if file exists using core::File
        if (!core::File::Util::exists(strFile.data())) {
            LAP_PER_LOG_INFO << "KvsFileBackend::parseFromFile file not found (first run): " << strFile.data();
            // Not an error for first run, just initialize empty
            m_kvsRoot.clear();
            return result::FromValue();
        }

        // ✅ Use core::File::Util::ReadBinary instead of std::ifstream
        core::Vector<core::UInt8> fileData;
        if (!core::File::Util::ReadBinary(strFile.data(), fileData)) {
            LAP_PER_LOG_WARN << "KvsFileBackend::parseFromFile failed to read file: " << strFile.data();
            return result::FromError( PerErrc::kFileNotFound );
        }

        // Convert byte vector to string for nlohmann::json
        core::String jsonContent(fileData.begin(), fileData.end());

        try {
            // Parse JSON using nlohmann::json
            m_kvsRoot = nlohmann::json::parse(jsonContent.c_str());
        } catch (const nlohmann::json::parse_error& e) {
            LAP_PER_LOG_WARN.logFormat( "KvsFileBackend::parseFromFile parse JSON %s failed with exception: %s!!!", strFile.data(), e.what() );
            return result::FromError( PerErrc::kFileNotFound );
        }
        
        return result::FromValue();
    }

    core::Result<void> KvsFileBackend::saveToFile( core::StringView strFile ) noexcept 
    {
        using result = core::Result<void>;

        try {
            // Ensure parent directory exists using core::Path
            core::String filePathStr(strFile.data());
            auto lastSlashPos = filePathStr.rfind('/');
            
            if (lastSlashPos != core::String::npos) {
                core::String dirPath = filePathStr.substr(0, lastSlashPos);
                if (!core::Path::createDirectory(dirPath)) {
                    LAP_PER_LOG_WARN << "KvsFileBackend::saveToFile failed to create directory: " << dirPath;
                    return result::FromError( PerErrc::kFileNotFound );
                }
            }
            
            // Serialize JSON using nlohmann::json with 4-space indentation
            std::string jsonContent = m_kvsRoot.dump(4);
            
            // Write using core::File
            if (!core::File::Util::WriteBinary(strFile.data(), 
                reinterpret_cast<const core::UInt8*>(jsonContent.data()), 
                jsonContent.size(), 
                true)) {  // createDirectories = true
                LAP_PER_LOG_WARN << "KvsFileBackend::saveToFile failed to write file: " << strFile.data();
                return result::FromError( PerErrc::kFileNotFound );
            }
            
            return result::FromValue();
        } catch (const std::exception& e) {
            LAP_PER_LOG_WARN.logFormat( "KvsFileBackend::saveToFile %s failed with exception: %s!!!", strFile.data(), e.what() );
            return result::FromError( PerErrc::kFileNotFound );
        }
    }
    
    // ==================== AUTOSAR Integrity & Backup Methods ====================
    
    core::Result<void> KvsFileBackend::validateDataIntegrity(core::StringView filePath) noexcept
    {
        using result = core::Result<void>;
        
        // Check 1: File existence
        if (!core::File::Util::exists(filePath.data())) {
            LAP_PER_LOG_ERROR << "Integrity check failed: File not found - " << filePath.data();
            return result::FromError(PerErrc::kFileNotFound);
        }
        
        // Check 2: File size (not empty, not too large)
        core::Vector<core::UInt8> fileData;
        if (!core::File::Util::ReadBinary(filePath.data(), fileData)) {
            LAP_PER_LOG_ERROR << "Integrity check failed: Cannot read file - " << filePath.data();
            return result::FromError(PerErrc::kIntegrityCorrupted);
        }
        
        if (fileData.empty()) {
            LAP_PER_LOG_ERROR << "Integrity check failed: File is empty - " << filePath.data();
            return result::FromError(PerErrc::kIntegrityCorrupted);
        }
        
        // Check 3: JSON format validation
        try {
            core::String jsonContent(fileData.begin(), fileData.end());
            nlohmann::json testJson = nlohmann::json::parse(jsonContent.c_str());
            
            // Successfully parsed - JSON is valid
            LAP_PER_LOG_INFO << "Integrity check passed for: " << filePath.data();
            
        } catch (const nlohmann::json::parse_error& e) {
            LAP_PER_LOG_ERROR << "Integrity check failed: Invalid JSON format - " 
                             << filePath.data() << " : " << e.what();
            return result::FromError(PerErrc::kIntegrityCorrupted);
        }
        
        // TODO: Check 4: Schema validation (if schema exists)
        // TODO: Check 5: Checksum validation (if checksum file exists)
        
        return result::FromValue();
    }
    
    core::Result<void> KvsFileBackend::backupToRedundancy() noexcept
    {
        using result = core::Result<void>;
        
        core::String currentPath = getCurrentPath();
        core::String redundancyPath = getRedundancyPath();
        
        // Check if current file exists
        if (!core::File::Util::exists(currentPath.data())) {
            // No current file to backup (might be first run)
            LAP_PER_LOG_INFO << "No current file to backup, skipping redundancy backup";
            return result::FromValue();
        }
        
        // Read current file
        core::Vector<core::UInt8> fileData;
        if (!core::File::Util::ReadBinary(currentPath.data(), fileData)) {
            LAP_PER_LOG_ERROR << "Failed to read current file for backup: " << currentPath.data();
            return result::FromError(PerErrc::kPhysicalStorageFailure);
        }
        
        // Write to redundancy
        if (!core::File::Util::WriteBinary(redundancyPath.data(), 
                                           fileData.data(), 
                                           fileData.size(), 
                                           true)) {
            LAP_PER_LOG_ERROR << "Failed to write redundancy backup: " << redundancyPath.data();
            return result::FromError(PerErrc::kPhysicalStorageFailure);
        }
        
        LAP_PER_LOG_INFO << "Backup created: " << redundancyPath.data();
        return result::FromValue();
    }
    
    core::Result<void> KvsFileBackend::atomicReplaceCurrentWithUpdate() noexcept
    {
        using result = core::Result<void>;
        
        core::String updatePath = getUpdatePath();
        core::String currentPath = getCurrentPath();
        core::String tempPath = currentPath + ".tmp";
        
        // Step 1: Check update file exists
        if (!core::File::Util::exists(updatePath.data())) {
            LAP_PER_LOG_ERROR << "Update file not found: " << updatePath.data();
            return result::FromError(PerErrc::kFileNotFound);
        }
        
        // Step 2: Copy update to temp location
        core::Vector<core::UInt8> updateData;
        if (!core::File::Util::ReadBinary(updatePath.data(), updateData)) {
            LAP_PER_LOG_ERROR << "Failed to read update file: " << updatePath.data();
            return result::FromError(PerErrc::kPhysicalStorageFailure);
        }
        
        if (!core::File::Util::WriteBinary(tempPath.data(), 
                                           updateData.data(), 
                                           updateData.size(), 
                                           true)) {
            LAP_PER_LOG_ERROR << "Failed to write temp file: " << tempPath.data();
            return result::FromError(PerErrc::kPhysicalStorageFailure);
        }
        
        // Step 3: Atomic rename (POSIX rename is atomic)
        if (rename(tempPath.c_str(), currentPath.c_str()) != 0) {
            LAP_PER_LOG_ERROR << "Atomic rename failed: " << strerror(errno);
            // Cleanup temp file on failure
            core::File::Util::remove(tempPath.data());
            return result::FromError(PerErrc::kPhysicalStorageFailure);
        }
        
        LAP_PER_LOG_INFO << "Atomic replace successful: update/ -> current/";
        return result::FromValue();
    }

    // ==================== Constructor & Destructor ====================

    KvsFileBackend::~KvsFileBackend() noexcept
    {
        if ( m_bAvailable ) {
            // Auto-sync on destruction if there are pending changes
            if (m_dirty) {
                auto syncResult = SyncToStorage();
                if (!syncResult.HasValue()) {
                    LAP_PER_LOG_WARN << "KvsFileBackend::~KvsFileBackend auto-sync failed";
                }
            }
            m_kvsRoot.clear();
            m_bAvailable = false;
        }
    }

    KvsFileBackend::KvsFileBackend( core::StringView strFile ) noexcept
        : m_strFile( strFile )
        , m_dirty(false)
    {
        // Use StoragePathManager to get standard KVS path
        core::String instancePath(strFile.data());
        
        // Get the standard KVS instance path
        m_instancePath = CStoragePathManager::getKvsInstancePath(instancePath);
        
        // Set the current data file path (AUTOSAR current/ directory)
        m_strFile = getCurrentPath();
        
        LAP_PER_LOG_INFO << "KvsFileBackend initialized with instance: " << m_instancePath;
        LAP_PER_LOG_INFO << "  current/  : " << getCurrentPath();
        LAP_PER_LOG_INFO << "  update/   : " << getUpdatePath();
        LAP_PER_LOG_INFO << "  redundancy/: " << getRedundancyPath();
        LAP_PER_LOG_INFO << "  recovery/ : " << getRecoveryPath();
        
        // Create AUTOSAR 4-layer directory structure
        auto createResult = CStoragePathManager::createStorageStructure(instancePath, "kvs");
        if (!createResult.HasValue()) {
            LAP_PER_LOG_WARN << "Failed to create KVS directory structure for: " << instancePath;
            m_bAvailable = false;
            return;
        }
        
        // Try to parse from existing file in current/ directory
        auto parseResult = parseFromFile( m_strFile );
        if (!parseResult.HasValue()) {
            // Not fatal - might be first run
            LAP_PER_LOG_INFO << "No existing KVS file found, starting with empty storage";
        }

        m_bAvailable = true;
        m_dirty = false;
    }
    
    // ==================== AUTOSAR 4-Layer Directory Path Helpers ====================
    
    core::String KvsFileBackend::getCurrentPath() const noexcept
    {
        core::String currentDir = core::Path::appendString(m_instancePath, "current");
        return core::Path::appendString(currentDir, "kvs_data.json");
    }
    
    core::String KvsFileBackend::getUpdatePath() const noexcept
    {
        core::String updateDir = core::Path::appendString(m_instancePath, "update");
        return core::Path::appendString(updateDir, "kvs_data.json");
    }
    
    core::String KvsFileBackend::getRedundancyPath() const noexcept
    {
        core::String redundancyDir = core::Path::appendString(m_instancePath, "redundancy");
        return core::Path::appendString(redundancyDir, "kvs_data.json.bak");
    }
    
    core::String KvsFileBackend::getRecoveryPath() const noexcept
    {
        core::String recoveryDir = core::Path::appendString(m_instancePath, "recovery");
        return core::Path::appendString(recoveryDir, "deleted_keys.json");
    }

} // namespace per
} // namespace lap
