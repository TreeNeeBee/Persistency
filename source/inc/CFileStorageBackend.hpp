/**
 * @file CFileStorageBackend.hpp
 * @brief File Storage Backend - Pure file operations (no lifecycle management)
 * @version 3.0
 * @date 2025-11-14
 * 
 * This class provides basic file operations for FileStorage.
 * Lifecycle management (backup, update, metadata, replicas) is handled by CPersistencyManager.
 * 
 * Architecture: Phase 2.1 Refactoring
 * - Removed: Initialize, Uninitialize, NeedsUpdate, CreateBackup, CheckReplicaHealth, LoadMetadata, etc.
 * - Kept: Basic file operations (ReadFile, WriteFile, DeleteFile, ListFiles, etc.)
 */

#ifndef LAP_PERSISTENCY_FILESTORAGEBACKEND_HPP
#define LAP_PERSISTENCY_FILESTORAGEBACKEND_HPP

#include <lap/core/CResult.hpp>
#include <lap/core/CPath.hpp>
#include <lap/core/CFile.hpp>
#include "CDataType.hpp"

namespace lap {
namespace per {

// Note: StorageUri is now defined in CDataType.hpp
// It is included via #include "CDataType.hpp" above

/**
 * @brief File Storage Backend - Pure file operations
 * 
 * This class only handles basic file operations:
 * - ReadFile / WriteFile / DeleteFile
 * - ListFiles / FileExists / GetFileSize
 * - GetFileUri / CopyFile / MoveFile
 * 
 * Lifecycle management (backup, update, metadata, replicas) is handled by CPersistencyManager.
 * 
 * Directory Structure:
 * {basePath}/
 * ├── current/     # Current active files
 * ├── backup/      # Backup files
 * ├── initial/     # Initial files from manifest
 * └── update/       # Files during update
 */
class CFileStorageBackend final {
public:
    IMP_OPERATOR_NEW(CFileStorageBackend)
    
    /**
     * @brief Constructor
     * @param basePath Base storage path (e.g., /tmp/autosar_persistency_test/fs/instance1)
     */
    explicit CFileStorageBackend(const core::String& basePath) noexcept
        : m_basePath(basePath) {}
    
    /**
     * @brief Destructor
     */
    ~CFileStorageBackend() noexcept = default;
    
    // ========================================================================
    // Basic File Operations
    // ========================================================================
    
    /**
     * @brief Read file from specific category
     * @param fileName File name
     * @param category Category (current/backup/initial/update)
     * @return File content as Vector<Byte>
     */
    core::Result<core::Vector<core::Byte>> ReadFile(
        const core::String& fileName,
        const core::String& category = LAP_PER_CATEGORY_CURRENT
    ) noexcept;
    
    /**
     * @brief Write file to specific category
     * @param fileName File name
     * @param data File content
     * @param category Category (current/backup/initial/update)
     * @return Result indicating success or error
     */
    core::Result<void> WriteFile(
        const core::String& fileName,
        const core::Vector<core::Byte>& data,
        const core::String& category = LAP_PER_CATEGORY_CURRENT
    ) noexcept;
    
    /**
     * @brief Delete file from specific category
     * @param fileName File name
     * @param category Category (current/backup/initial/update)
     * @return Result indicating success or error
     */
    core::Result<void> DeleteFile(
        const core::String& fileName,
        const core::String& category = LAP_PER_CATEGORY_CURRENT
    ) noexcept;
    
    /**
     * @brief List all files in specific category
     * @param category Category (current/backup/initial/update)
     * @return Vector of file names
     */
    core::Result<core::Vector<core::String>> ListFiles(
        const core::String& category = LAP_PER_CATEGORY_CURRENT
    ) noexcept;
    
    /**
     * @brief Check if file exists in specific category
     * @param fileName File name
     * @param category Category (current/backup/initial/update)
     * @return true if exists, false otherwise
     */
    core::Bool FileExists(
        const core::String& fileName,
        const core::String& category = LAP_PER_CATEGORY_CURRENT
    ) const noexcept;
    
    /**
     * @brief Get file size
     * @param fileName File name
     * @param category Category (current/backup/initial/update)
     * @return File size in bytes
     */
    core::Result<core::UInt64> GetFileSize(
        const core::String& fileName,
        const core::String& category = LAP_PER_CATEGORY_CURRENT
    ) const noexcept;
    
    // ========================================================================
    // Helper Operations
    // ========================================================================
    
    /**
     * @brief Get file URI structure
     * @param fileName File name
     * @param category Category (current/backup/initial/update)
     * @return StorageUri structure
     */
    StorageUri GetFileUri(
        const core::String& fileName,
        const core::String& category = LAP_PER_CATEGORY_CURRENT
    ) const noexcept;
    
    /**
     * @brief Copy file from one category to another
     * @param fileName File name
     * @param fromCategory Source category
     * @param toCategory Destination category
     * @return Result indicating success or error
     */
    core::Result<void> CopyFile(
        const core::String& fileName,
        const core::String& fromCategory,
        const core::String& toCategory
    ) noexcept;
    
    /**
     * @brief Move file from one category to another (atomic)
     * @param fileName File name
     * @param fromCategory Source category
     * @param toCategory Destination category
     * @return Result indicating success or error
     */
    core::Result<void> MoveFile(
        const core::String& fileName,
        const core::String& fromCategory,
        const core::String& toCategory
    ) noexcept;
    
private:
    core::String m_basePath;  // Base storage path
    
    // Helper methods
    core::String GetCategoryPath(const core::String& category) const noexcept;
    core::String GetFilePath(const core::String& fileName, const core::String& category) const noexcept;
};

} // namespace per
} // namespace lap

#endif // LAP_PERSISTENCY_FILESTORAGEBACKEND_HPP

