/**
 * @file CKvsFileBackend.hpp
 * @brief JSON File Backend Implementation for Key-Value Storage
 * @version 2.0
 * @date 2025-11-14
 * 
 * @copyright Copyright (c) 2025
 * 
 * This file implements the file-based backend for KVS using JSON format.
 * 
 * Features:
 * - Human-readable JSON storage
 * - Easy debugging and manual editing
 * - Version control friendly
 * - Atomic write operations
 * 
 * Use Cases:
 * - Configuration data storage
 * - Development/debug environments
 * - Small datasets (<10MB)
 * 
 * @note Follows Core module constraints:
 * - Uses core::File for all file I/O (no std::fstream)
 * - Uses core::Path for path operations
 * - Thread-safe operations
 */
#ifndef LAP_PERSISTENCY_KVSFILEBACKEND_HPP
#define LAP_PERSISTENCY_KVSFILEBACKEND_HPP

#include <nlohmann/json.hpp>
#include <lap/core/CResult.hpp>
#include <lap/core/CMemory.hpp>
#include <lap/core/CSync.hpp>

#include "CDataType.hpp"
#include "IKvsBackend.hpp"

namespace lap
{
namespace per
{
    class KeyValueStorage;
    
    /**
     * @brief JSON File Backend for Key-Value Storage
     * 
     * This backend stores key-value pairs in a JSON file on disk.
     * Data is loaded into memory on construction and synchronized to disk on Sync().
     * 
     * File Format:
     * ```json
     * {
     *   "key1": "value1",
     *   "key2": "value2"
     * }
     * ```
     * 
     * Thread Safety:
     * - All operations are thread-safe
     * - Internal mutex protects concurrent access
     * 
     * Performance:
     * - In-memory cache for fast reads
     * - Writes are deferred until Sync()
     * - Suitable for small to medium datasets
     */
    class KvsFileBackend final : public IKvsBackend
    {
    public:
        IMP_OPERATOR_NEW(KvsFileBackend)
        
    private:
        typedef core::Map< core::String, KvsDataType >           _ValueMap;

    public:
        // ==================== IKvsBackend Interface Implementation ====================
        
        /**
         * @brief Get all keys in storage
         * @note AUTOSAR API: KeyValueStorage::GetAllKeys
         */
        core::Result<core::Vector<core::String>> GetAllKeys() const noexcept override;
        
        /**
         * @brief Get file size on disk
         */
        core::Result<core::UInt64> GetSize() const noexcept override;
        
        /**
         * @brief Get number of keys
         */
        core::Result<core::UInt32> GetKeyCount() const noexcept override;
        
        /**
         * @brief File backend always supports persistence
         */
        core::Bool SupportsPersistence() const noexcept override { return true; }
        
        /**
         * @brief Backend type identifier
         */
        KvsBackendType GetBackendType() const noexcept override { return KvsBackendType::kvsFile; }

        /**
         * @brief Backend availability check
         */
        core::Bool available() const noexcept override { return m_bAvailable; }

        /**
         * @brief Discard pending changes and reload from file
         */
        core::Result<void> DiscardPendingChanges() noexcept override;

        /**
         * @brief Recovery a previously deleted key (not supported for file backend)
         */
        core::Result<void> RecoverKey(core::StringView key) noexcept override;

        /**
         * @brief Reset a key to default value (not supported for file backend)
         */
        core::Result<void> ResetKey(core::StringView key) noexcept override;

        // ==================== AUTOSAR Key-Value Storage API ====================

        core::Result<core::Bool> KeyExists(core::StringView key) const noexcept override;
        core::Result<KvsDataType> GetValue(core::StringView key) const noexcept override;
        core::Result<void> SetValue(core::StringView key, const KvsDataType& value) noexcept override;
        core::Result<void> RemoveKey(core::StringView key) noexcept override;
        core::Result<void> RemoveAllKeys() noexcept override;
        core::Result<void> SyncToStorage() noexcept override;

        ~KvsFileBackend() noexcept override;
        explicit KvsFileBackend( core::StringView ) noexcept;

    protected:
        friend class KeyValueStorage;

        /**
         * @brief Parse JSON from file using core::File
         * @note Uses core::File::ReadAllBytes instead of std::ifstream
         */
        core::Result<void> parseFromFile( core::StringView ) noexcept;
        
        /**
         * @brief Save JSON to file using core::File
         * @note Uses core::File::WriteAllBytes instead of std::ofstream
         */
        core::Result<void> saveToFile( core::StringView ) noexcept;
        
        /**
         * @brief Get path to current/ directory (active data)
         * @return {instance}/current/kvs_data.json
         * @note AUTOSAR [SWS_PER_00500]
         */
        core::String getCurrentPath() const noexcept;
        
        /**
         * @brief Get path to update/ directory (modification buffer)
         * @return {instance}/update/kvs_data.json
         * @note AUTOSAR [SWS_PER_00501]
         */
        core::String getUpdatePath() const noexcept;
        
        /**
         * @brief Get path to redundancy/ directory (backup for rollback)
         * @return {instance}/redundancy/kvs_data.json.bak
         * @note AUTOSAR [SWS_PER_00502]
         */
        core::String getRedundancyPath() const noexcept;
        
        /**
         * @brief Get path to recovery/ directory (deleted keys)
         * @return {instance}/recovery/deleted_keys.json
         * @note AUTOSAR [SWS_PER_00503]
         */
        core::String getRecoveryPath() const noexcept;
        
        /**
         * @brief Validate data integrity before commit
         * @param filePath Path to data file to validate
         * @return Success if all checks pass
         * @note AUTOSAR [SWS_PER_00800] - Data integrity requirements
         */
        core::Result<void> validateDataIntegrity(core::StringView filePath) noexcept;
        
        /**
         * @brief Backup current data to redundancy directory
         * @return Success if backup created
         * @note AUTOSAR [SWS_PER_00502] - Redundancy backup
         */
        core::Result<void> backupToRedundancy() noexcept;
        
        /**
         * @brief Atomic replace: move update/ to current/
         * @return Success if replacement succeeded
         * @note Uses rename() for atomicity
         */
        core::Result<void> atomicReplaceCurrentWithUpdate() noexcept;

        KvsFileBackend() = delete;
        KvsFileBackend( const KvsFileBackend& ) = delete;
        KvsFileBackend( KvsFileBackend&& ) = delete;
        KvsFileBackend& operator=( const KvsFileBackend& ) = delete;

    private:
        core::Bool                                          m_bAvailable{ false };  ///< Backend availability flag
        core::String                                        m_strFile;              ///< JSON file path (current/ directory)
        core::String                                        m_instancePath;         ///< Instance base path
        nlohmann::json                                      m_kvsRoot;              ///< In-memory JSON object
        core::Bool                                          m_dirty{false};         ///< True if there are unsaved changes
        mutable core::RWLock                                m_rwLock;               ///< Thread-safe access protection [SWS_PER_00309]
    };
} // namespace per
} // namespace lap

#endif // LAP_PERSISTENCY_KVSFILEBACKEND_HPP
