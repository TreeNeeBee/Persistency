/**
 * @file CFileStorage.hpp
 * @author ddkv587 (ddkv587@gmail.com)
 * @brief AUTOSAR-compliant File Storage with URI-based folder management
 * @version 3.0
 * @date 2024-02-02
 * 
 * Updated: 2025-11-15 - Phase 2.3: Refactored to use CFileStorageBackend
 * - Backend injection architecture (CFileStorageBackend)
 * - Lifecycle management delegated to CPersistencyManager
 * - Simplified initialization (no config/metadata management)
 * - URI-based storage area management
 * - Version-aware update strategies (via CPersistencyManager)
 * - Backup and recovery mechanisms (via CPersistencyManager)
 */
#ifndef LAP_PERSISTENCY_FILESTORAGE_HPP
#define LAP_PERSISTENCY_FILESTORAGE_HPP

#include <unordered_map>
#include <lap/core/CResult.hpp>
#include <lap/core/CPath.hpp>
#include <lap/core/CMemory.hpp>
#include <lap/core/CSync.hpp>
#include <boost/crc.hpp>
#include <boost/filesystem.hpp>

#include "CDataType.hpp"
#include "CPersistencyManager.hpp"
#include "CFileStorageBackend.hpp"

namespace lap 
{
namespace per 
{
    class ReadAccessor;
    class ReadWriteAccessor;
    
    /**
     * @brief AUTOSAR-compliant File Storage
     * 
     * This class provides AUTOSAR Adaptive Platform File Storage interface with:
     * - URI-based storage location management (SWS_PER_00386)
     * - Version management and update support (SWS_PER_00396)
     * - Backup and recovery mechanisms (SWS_PER_00446)
     * - M-out-of-N replica redundancy via CReplicaManager (SWS_PER_00558)
     * 
     * Directory Structure (Simplified):
     * {storageUri}/
     * ├── .metadata/           # Storage metadata with version info
     * ├── current/             # Current active files (with replicas)
     * │   ├── file1.replica_0  # Primary replica
     * │   ├── file1.replica_1  # Secondary replica
     * │   └── file1.replica_2  # Tertiary replica (if N=3)
     * ├── backup/              # Backup files (with replicas)
     * ├── initial/             # Initial files from manifest
     * └── update/              # Files during update transaction
     */
    class FileStorage final
    {
        IMP_OPERATOR_NEW( FileStorage );

    public:
        /**
         * @brief File context tracking structure
         * Enhanced with version information for AUTOSAR compliance
         */
        struct tagFileContext
        {
            struct FileInfo                     fileInfo;
            FileCRCType                         crcType;
            core::UInt32                        crcKey;
            core::Bool                          isOpen;
            
            // Version tracking (AUTOSAR SWS_PER_00463)
            core::String                        contractVersion;      // Interface version
            core::String                        deploymentVersion;    // Deployment version
            core::UInt64                        backupTimestamp;      // Backup creation time
            core::Bool                          hasBackup;            // Backup exists flag
        };

    private:
        // Legacy path definitions - now using macros from CDataType.hpp
        // LAP_PER_CATEGORY_CURRENT, LAP_PER_CATEGORY_BACKUP, LAP_PER_CATEGORY_INITIAL

        #define DEF_FS_INFO_DATA                ".fsinfo"
        #define DEF_FS_INFO_DATA_KEY            "bc0eb773-c56e-400f-b96e-5d9e36f7fa78"

    public:
        ~FileStorage () noexcept;

        /**
         * @brief Initialize file storage with optional metadata
         * @param strConfig Configuration string (JSON or path to config file)
         * @param bCreate Create storage if it doesn't exist
         * @return Result indicating success or error
         */
        core::Result< core::Bool >                                  initialize( core::StringView strConfig = "", core::Bool bCreate = false ) noexcept;
        
        /**
         * @brief Uninitialize and cleanup storage
         */
        void                                                        uninitialize() noexcept;
        
        inline core::Bool                                           isInitialized() noexcept                            { return m_bInitialize; }
        inline core::Bool                                           isResourceBusy() noexcept                           { return m_bResourceBusy; }

        inline void                                                 SetMaxNumberOfFiles( core::UInt32 num ) noexcept    { m_maxNumberOfFiles = num; }
        inline core::UInt32                                         GetMaxNumberOfFiles() const noexcept                { return m_maxNumberOfFiles; }

        // ========================================================================
        // AUTOSAR File Storage Interface (SWS_PER_00301 - SWS_PER_00340)
        // ========================================================================
        
        /**
         * @brief Get all file names in current storage
         * @return Vector of file names
         */
        core::Result< core::Vector< core::String > >                GetAllFileNames () const noexcept;

        /**
         * @brief Delete file from storage
         * @param fileName File name to delete
         * @return Result indicating success or error (SWS_PER_00335)
         */
        core::Result< void >                                        DeleteFile ( core::StringView fileName ) noexcept;
        
        /**
         * @brief Check if file exists in storage
         * @param fileName File name to check
         * @return true if exists, false otherwise (SWS_PER_00336)
         */
        core::Result< core::Bool >                                  FileExists ( core::StringView fileName ) const noexcept;
        
        /**
         * @brief Recover file from backup
         * @param fileName File name to recover
         * @return Result indicating success or error (SWS_PER_00386)
         */
        core::Result< void >                                        RecoverFile ( core::StringView fileName ) noexcept;
        
        /**
         * @brief Reset file to initial state (factory reset)
         * @param fileName File name to reset
         * @return Result indicating success or error (SWS_PER_00446)
         */
        core::Result< void >                                        ResetFile ( core::StringView fileName ) noexcept;
        
        /**
         * @brief Get current file size
         * @param fileName File name
         * @return File size in bytes (SWS_PER_00337)
         */
        core::Result< core::UInt64 >                                GetCurrentFileSize ( core::StringView fileName ) const noexcept;
        
        /**
         * @brief Get file information (metadata)
         * @param fileName File name
         * @return FileInfo structure (SWS_PER_00338)
         */
        core::Result< FileInfo >                                    GetFileInfo ( core::StringView fileName ) const noexcept;

        // ========================================================================
        // File Access Operations (SWS_PER_00301 - SWS_PER_00310)
        // ========================================================================
        
        /**
         * @brief Open file for read-write access
         * @param fileName File name
         * @return ReadWriteAccessor handle (SWS_PER_00301)
         */
        core::Result< core::UniqueHandle< ReadWriteAccessor > >     OpenFileReadWrite( core::StringView fileName ) noexcept;
        core::Result< core::UniqueHandle< ReadWriteAccessor > >     OpenFileReadWrite( core::StringView fileName, OpenMode mode ) noexcept;
        core::Result< core::UniqueHandle< ReadWriteAccessor > >     OpenFileReadWrite( core::StringView fileName, OpenMode mode, core::Span< core::Byte > buffer ) noexcept;

        /**
         * @brief Open file for read-only access
         * @param fileName File name
         * @return ReadAccessor handle (SWS_PER_00305)
         */
        core::Result< core::UniqueHandle< ReadAccessor > >          OpenFileReadOnly( core::StringView fileName ) noexcept;
        core::Result< core::UniqueHandle< ReadAccessor > >          OpenFileReadOnly( core::StringView fileName, OpenMode mode ) noexcept;
        core::Result< core::UniqueHandle< ReadAccessor > >          OpenFileReadOnly( core::StringView fileName, OpenMode mode, core::Span< core::Byte > buffer ) noexcept;

        /**
         * @brief Open file for write-only access
         * @param fileName File name
         * @return ReadWriteAccessor handle (SWS_PER_00310)
         */
        core::Result< core::UniqueHandle< ReadWriteAccessor > >     OpenFileWriteOnly( core::StringView fileName ) noexcept;
        core::Result< core::UniqueHandle< ReadWriteAccessor > >     OpenFileWriteOnly( core::StringView fileName, OpenMode mode ) noexcept;
        core::Result< core::UniqueHandle< ReadWriteAccessor > >     OpenFileWriteOnly( core::StringView fileName, OpenMode mode, core::Span< core::Byte > buffer ) noexcept;

        /**
         * @brief Update file context (internal use)
         * @param fileName File name
         * @param fileInfo Updated file info
         * @param isClosed File is closed flag
         */
        void                                                        update( core::StringView, FileInfo&, core::Bool isClosed = true ) noexcept;

        // ========================================================================
        // Version Management (AUTOSAR SWS_PER_00396, SWS_PER_00463)
        // ========================================================================
        
        /**
         * @brief Check if storage needs update based on version comparison
         * @param manifestDeploymentVersion Deployment version from manifest
         * @param manifestContractVersion Contract version from manifest
         * @return true if update needed, false otherwise
         */
        core::Result< core::Bool >                                  NeedsUpdate(
            const core::String& manifestDeploymentVersion,
            const core::String& manifestContractVersion
        ) noexcept;
        
        /**
         * @brief Get current storage metadata
         * @return FileStorageMetadata structure
         */
        core::Result< FileStorageMetadata >                         GetMetadata() const noexcept;
        
        /**
         * @brief Update storage version information
         * @param contractVersion New contract version
         * @param deploymentVersion New deployment version
         * @return Result indicating success or error
         */
        core::Result< void >                                        UpdateVersionInfo(
            const core::String& contractVersion,
            const core::String& deploymentVersion
        ) noexcept;

        // ========================================================================
        // Backup and Update Operations (AUTOSAR SWS_PER_00386, SWS_PER_00387)
        // ========================================================================
        
        /**
         * @brief Create backup of all files
         * @return Result indicating success or error
         */
        core::Result< void >                                        CreateBackup() noexcept;
        
        /**
         * @brief Restore all files from backup
         * @return Result indicating success or error
         */
        core::Result< void >                                        RestoreBackup() noexcept;
        
        /**
         * @brief Begin update transaction (atomic)
         * @return Result indicating success or error
         */
        core::Result< void >                                        BeginUpdate() noexcept;
        
        /**
         * @brief Commit update transaction (atomic)
         * @return Result indicating success or error
         */
        core::Result< void >                                        CommitUpdate() noexcept;
        
        /**
         * @brief Rollback update transaction
         * @return Result indicating success or error
         */
        core::Result< void >                                        RollbackUpdate() noexcept;

        // ========================================================================
        // Backend Access
        // ========================================================================
        
        /**
         * @brief Get underlying storage backend
         * @return Pointer to CFileStorageBackend
         */
        inline CFileStorageBackend* GetBackend() noexcept { return m_pBackend.get(); }
        
        /**
         * @brief Set storage backend (for lifecycle management by CPersistencyManager)
         * @param backend UniqueHandle to CFileStorageBackend
         * @note This method is called by CPersistencyManager during initialization
         */
        void setBackend(core::UniqueHandle<CFileStorageBackend> backend) noexcept {
            m_pBackend = std::move(backend);
        }

    protected:
        friend class CPersistencyManager;

        static core::SharedHandle< FileStorage > create( core::StringView path ) noexcept
        {
            struct _FileStorage : std::allocator< FileStorage > {
                void construct( void* p, core::StringView path ) noexcept   { ::new( p ) FileStorage( path ); }
                void destroy( FileStorage* p ) noexcept                     { p->~FileStorage(); }
            };

            return ::std::allocate_shared< FileStorage >( _FileStorage(), path ); 
        }

        explicit FileStorage ( core::StringView ) noexcept;
        FileStorage ( FileStorage &&fs ) noexcept;

        FileStorage& operator= ( FileStorage &&fs ) &noexcept;
        FileStorage ( const FileStorage & ) = delete;
        FileStorage& operator= ( const FileStorage &) = delete;

        /**
         * @brief Recover all files from backup (SWS_PER_00386)
         */
        core::Result< void >            RecoverAllFiles() noexcept;
        
        /**
         * @brief Reset all files to initial state (SWS_PER_00446)
         */
        core::Result< void >            ResetAllFiles() noexcept;
        
        /**
         * @brief Get total storage size
         * @return Total size in bytes (SWS_PER_00321)
         */
        core::Result< core::UInt64 >    GetCurrentFileStorageSize() noexcept;

        inline core::StringView         path() noexcept             { return m_strPath; }

    private:
        /**
         * @brief Load file info from metadata (enhanced with version tracking)
         */
        core::Bool                      loadFileInfo() noexcept;
        
        /**
         * @brief Sync file info to metadata (enhanced with version tracking)
         */
        core::Bool                      syncFileInfo() noexcept;

        /**
         * @brief Format path helper functions
         */
        inline core::StringView formatPath( core::StringView extra ) noexcept
        {
            return core::Path::append( m_strPath, extra );
        }

        inline core::StringView formatCurPath( core::StringView fileName ) noexcept
        {
            // Use backend if available
            if (m_pBackend) {
                auto uri = m_pBackend->GetFileUri(fileName.data(), LAP_PER_CATEGORY_CURRENT);
                return uri.GetFullPath();
            }
            return core::Path::append( formatPath( LAP_PER_CATEGORY_CURRENT ), fileName );
        }

        inline core::StringView formatRevoveryPath( core::StringView fileName ) noexcept
        {
            // Use backend if available
            if (m_pBackend) {
                auto uri = m_pBackend->GetFileUri(fileName.data(), LAP_PER_CATEGORY_BACKUP);
                return uri.GetFullPath();
            }
            return core::Path::append( formatPath( LAP_PER_CATEGORY_BACKUP ), fileName );
        }

        inline core::StringView formatResetPath( core::StringView fileName ) noexcept
        {
            // Use backend if available
            if (m_pBackend) {
                auto uri = m_pBackend->GetFileUri(fileName.data(), LAP_PER_CATEGORY_INITIAL);
                return uri.GetFullPath();
            }
            return core::Path::append( formatPath( LAP_PER_CATEGORY_INITIAL ), fileName );
        }

    private:
        core::Bool                                                  m_bValid{false};
        core::Bool                                                  m_bCorrupted{false};
        core::Bool                                                  m_bDecryption{false};
        core::Bool                                                  m_bInitialize{false};
        core::Bool                                                  m_bResourceBusy{false};
        core::StringView                                            m_strPath{""};
        core::UInt32                                                m_maxNumberOfFiles{LAP_PER_DEFAULT_MAX_FILE_COUNT};

        mutable core::Mutex                                         m_mtxFileStorage;
        ::std::unordered_map< core::String, tagFileContext >        m_mapFileStorage;
        
        // File storage backend (pure file operations)
        core::UniqueHandle< CFileStorageBackend >                   m_pBackend{nullptr};
    };

    /**
     * @brief Open file storage with instance specifier
     * @param fs Instance specifier
     * @return SharedHandle to FileStorage (SWS_PER_00340)
     */
    core::Result< core::SharedHandle< FileStorage > >               OpenFileStorage ( const core::InstanceSpecifier &fs ) noexcept;
    
    /**
     * @brief Open file storage with create option
     * @param fs Instance specifier
     * @param bCreate Create if doesn't exist
     * @return SharedHandle to FileStorage (SWS_PER_00340)
     */
    core::Result< core::SharedHandle< FileStorage > >               OpenFileStorage ( const core::InstanceSpecifier &fs, core::Bool bCreate ) noexcept;
    
    /**
     * @brief Recover all files in storage
     * @param fs Instance specifier
     * @return Result indicating success or error (SWS_PER_00386)
     */
    core::Result< void >                                            RecoverAllFiles ( const core::InstanceSpecifier &fs ) noexcept;
    
    /**
     * @brief Reset all files in storage to initial state
     * @param fs Instance specifier
     * @return Result indicating success or error (SWS_PER_00446)
     */
    core::Result< void >                                            ResetAllFiles ( const core::InstanceSpecifier &fs ) noexcept;
    
    /**
     * @brief Get total storage size
     * @param fs Instance specifier
     * @return Total size in bytes (SWS_PER_00321)
     */
    core::Result< core::UInt64 >                                    GetCurrentFileStorageSize ( const core::InstanceSpecifier &fs ) noexcept;
} // per
} // lap

#endif
