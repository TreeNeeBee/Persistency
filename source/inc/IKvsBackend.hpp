/**
 * @file IKvsBackend.hpp
 * @brief KVS Backend Interface - Unified abstraction for multiple backend implementations
 * @version 2.0
 * @date 2025-11-14
 * 
 * @copyright Copyright (c) 2025
 * 
 * This file defines the abstract interface for Key-Value Storage backends.
 * Supports multiple backend types: File (JSON), Database (SQLite), Property (Shared Memory)
 * 
 * @note This interface follows Core module constraints:
 * - Uses core::String, core::Vector, core::Result types
 * - No std:: standard library types
 * - Thread-safe operations
 */

#ifndef LAP_PERSISTENCY_IKVSBACKEND_HPP
#define LAP_PERSISTENCY_IKVSBACKEND_HPP

#include <lap/core/CResult.hpp>
#include <lap/core/CString.hpp>
#include <lap/core/CMemory.hpp>

#include "CDataType.hpp"

namespace lap
{
namespace per
{
    // Magic key constants for type encoding
    #define DEF_KVS_MAGIC_KEY           '^'
    #define DEF_KVS_MAGIC_KEY_INDEX     0
    #define DEF_KVS_MAGIC_TYPE_INDEX    1

    /**
     * @brief Abstract interface for KVS backend implementations
     * 
     * This interface provides a unified abstraction for different storage backends.
     * All backend implementations must inherit from this interface and implement
     * all pure virtual methods.
     * 
     * Design Philosophy:
     * - Simple and clear interface
     * - No implementation details in interface
     * - Support both persistent and non-persistent backends
     * - Metadata query capabilities
     * 
     * Thread Safety:
     * - All methods must be thread-safe
     * - Implementations should use proper locking mechanisms
     * 
     * Error Handling:
     * - All methods return core::Result<T>
     * - Errors use PerErrc error codes
     * - No exceptions thrown
     */
    class IKvsBackend
    {
    public:
        IMP_OPERATOR_NEW(IKvsBackend)

        /**
         * @brief Comparator for KVS keys that handles magic type prefix
         * 
         * Keys can have a 2-character prefix: "^X" where X is the type indicator.
         * This comparator ignores the prefix when comparing keys, ensuring that
         * keys with different types but same name are treated as equivalent.
         * 
         * Format: "^a" = bool, "^b" = int8, ..., "^i" = string (see EKvsDataTypeIndicate)
         */
        struct KVS_Less
        {
            core::Bool operator()(const core::String& __x, const core::String& __y) const
            {
                // Calculate offset based on magic key presence
                core::Int8 xOffset = (__x.size() > 2 && __x[DEF_KVS_MAGIC_KEY_INDEX] == DEF_KVS_MAGIC_KEY) ? 2 : 0;
                core::Int8 yOffset = (__y.size() > 2 && __y[DEF_KVS_MAGIC_KEY_INDEX] == DEF_KVS_MAGIC_KEY) ? 2 : 0;

                // Compare only the actual key part (skip prefix)
                core::StringView strX(__x.c_str() + xOffset, __x.size() - xOffset);
                core::StringView strY(__y.c_str() + yOffset, __y.size() - yOffset);

                return strX < strY;
            }
        };

        /**
         * @brief Virtual destructor
         */
        virtual ~IKvsBackend() noexcept = default;

        // ==================== AUTOSAR Key-Value Storage API ====================

        /**
         * @brief Get all keys in storage
         * 
         * @return core::Result<core::Vector<core::String>> Vector of all keys
         * 
         * @note AUTOSAR API: KeyValueStorage::GetAllKeys
         * @note Returns empty vector if storage is empty
         */
        virtual core::Result<core::Vector<core::String>> GetAllKeys() const noexcept = 0;

        /**
         * @brief Discard all pending changes
         * 
         * @return core::Result<void> Success or error code
         * 
         * @note AUTOSAR API: KeyValueStorage::DiscardPendingChanges
         * @note Reloads data from persistent storage, losing all uncommitted changes
         */
        virtual core::Result<void> DiscardPendingChanges() noexcept = 0;

        // ==================== Key Recovery/Reset Operations ====================

        /**
         * @brief Recover a previously deleted key
         * 
         * @param key The key to recover
         * @return core::Result<void> Success or error code
         * 
         * @retval PerErrc::kKeyNotFound if key was never deleted or cannot be recovered
         * @note AUTOSAR API: KeyValueStorage::RecoverKey [SWS_PER_00313]
         * @note Implementation-specific: may not be supported by all backends
         * @note File backend: tries to restore from backup
         */
        virtual core::Result<void> RecoverKey(core::StringView key) noexcept = 0;

        /**
         * @brief Reset a key to its default/initial value
         * 
         * @param key The key to reset
         * @return core::Result<void> Success or error code
         * 
         * @retval PerErrc::kKeyNotFound if key doesn't exist
         * @note Implementation-specific behavior
         */
        virtual core::Result<void> ResetKey(core::StringView key) noexcept = 0;

        // ==================== Metadata Query ====================

        /**
         * @brief Check if backend is available/initialized
         * 
         * @return core::Bool True if backend is ready to use
         * 
         * @note Always check this before using other methods
         * @note File backend: checks if file path is accessible
         * @note DB backend: checks if database connection is alive
         */
        virtual core::Bool available() const noexcept = 0;

        /**
         * @brief Get total storage size in bytes
         * 
         * @return core::Result<core::UInt64> Size in bytes
         * 
         * @note For file backend: file size on disk
         * @note For db backend: database size
         * @note For property backend: shared memory size
         */
        virtual core::Result<core::UInt64> GetSize() const noexcept = 0;

        /**
         * @brief Get number of keys in storage
         * 
         * @return core::Result<core::UInt32> Number of keys
         */
        virtual core::Result<core::UInt32> GetKeyCount() const noexcept = 0;

        // ==================== Backend Capabilities ====================

        /**
         * @brief Check if backend supports persistence
         * 
         * @return core::Bool True if backend persists data across restarts
         * 
         * @note File backend: true
         * @note DB backend: true
         * @note Property backend: depends on whether data source is configured
         */
        virtual core::Bool SupportsPersistence() const noexcept = 0;

        /**
         * @brief Get backend type identifier
         * 
         * @return KvsBackendType Backend type (kvsFile, kvsSqlite, or kvsProperty)
         * 
         * @note Used for backend identification and factory creation
         */
        virtual KvsBackendType GetBackendType() const noexcept = 0;

        // ==================== AUTOSAR Key-Value Storage API ====================
        
        /**
         * @brief Check if a key exists
         * 
         * @param key The key to check
         * @return core::Result<core::Bool> True if exists
         * 
         * @note AUTOSAR API: KeyValueStorage::KeyExists
         */
        virtual core::Result<core::Bool> KeyExists(core::StringView key) const noexcept = 0;

        /**
         * @brief Get value as KvsDataType
         * 
         * @param key The key to lookup
         * @return core::Result<KvsDataType> Value or error code
         * 
         * @note AUTOSAR API: KeyValueStorage::GetValue
         * @note KvsDataType is a variant type that can hold different types
         */
        virtual core::Result<KvsDataType> GetValue(core::StringView key) const noexcept = 0;

        /**
         * @brief Set value from KvsDataType
         * 
         * @param key The key to set
         * @param value The value to set
         * @return core::Result<void> Success or error code
         * 
         * @note AUTOSAR API: KeyValueStorage::SetValue
         * @note KvsDataType is a variant type that can hold different types
         */
        virtual core::Result<void> SetValue(core::StringView key, const KvsDataType& value) noexcept = 0;

        /**
         * @brief Remove a key-value pair
         * 
         * @param key The key to remove
         * @return core::Result<void> Success or error code
         * 
         * @note AUTOSAR API: KeyValueStorage::RemoveKey
         */
        virtual core::Result<void> RemoveKey(core::StringView key) noexcept = 0;

        /**
         * @brief Remove all key-value pairs
         * 
         * @return core::Result<void> Success or error code
         * 
         * @note AUTOSAR API: KeyValueStorage::RemoveAllKeys
         * @warning This operation cannot be undone!
         */
        virtual core::Result<void> RemoveAllKeys() noexcept = 0;

        /**
         * @brief Synchronize changes to persistent storage
         * 
         * @return core::Result<void> Success or error code
         * 
         * @note AUTOSAR API: KeyValueStorage::SyncToStorage
         * @note All pending changes will be written to disk/database
         */
        virtual core::Result<void> SyncToStorage() noexcept = 0;

        // ==================== Static Utility Methods ====================

        /**
         * @brief Format key with type indicator prefix
         * 
         * Prepends magic key prefix "^X" where X indicates the data type.
         * If key already has the prefix, this function does nothing.
         * 
         * @param key The key to format (modified in-place)
         * @param valueType The data type indicator
         * 
         * @note Format: "^a" = bool, "^b" = int8, ..., "^i" = string
         * 
         * Example:
         * - Input: "username", DataType_string  →  Output: "^iusername"
         * - Input: "count", DataType_int32      →  Output: "^dcount"
         */
        static void formatKey(core::String& key, EKvsDataTypeIndicate valueType);

        /**
         * @brief Extract data type from formatted key
         * 
         * Reads the type indicator from the magic key prefix.
         * If no prefix exists, assumes string type.
         * 
         * @param strKey The formatted key
         * @return EKvsDataTypeIndicate The data type
         * 
         * @note If key doesn't have magic prefix "^X", returns DataType_string
         * 
         * Example:
         * - Input: "^iusername"  →  Output: DataType_string
         * - Input: "^dcount"     →  Output: DataType_int32
         * - Input: "plain_key"   →  Output: DataType_string (default)
         */
        static EKvsDataTypeIndicate getDataType(core::StringView strKey);

    protected:
        // Protected constructor - interface cannot be instantiated directly
        IKvsBackend() noexcept = default;

        // Disable copy and move
        IKvsBackend(const IKvsBackend&) = delete;
        IKvsBackend(IKvsBackend&&) = delete;
        IKvsBackend& operator=(const IKvsBackend&) = delete;
        IKvsBackend& operator=(IKvsBackend&&) = delete;
    };

} // namespace per
} // namespace lap

#endif // LAP_PERSISTENCY_IKVSBACKEND_HPP
