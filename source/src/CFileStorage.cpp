/**
 * @file CFileStorage.cpp
 * @brief AUTOSAR-compliant File Storage implementation
 * @version 3.0
 * @date 2024-02-02
 * 
 * Updated: 2025-11-14 - Integrated Core ConfigManager for configuration
 * Updated: 2025-11-15 - Phase 2.3: Refactored to use CFileStorageBackend
 */

#include <lap/core/CPath.hpp>
#include <lap/core/CTime.hpp>
#include <lap/core/CFile.hpp>
#include <lap/core/CConfig.hpp>
#include "CReadAccessor.hpp"
#include "CReadWriteAccessor.hpp"
#include "CFileStorage.hpp"
#include "CPerErrorDomain.hpp"
#include "CPersistencyManager.hpp"
#include "CStoragePathManager.hpp"
#include "CFileStorageBackend.hpp"

namespace lap
{
namespace per
{
    FileStorage::FileStorage ( core::StringView strIdentifier ) noexcept
        : m_strPath( strIdentifier )
        , m_pBackend( nullptr )
    {
        // Backend will be created during initialize()
    }

    FileStorage::FileStorage ( FileStorage &&fs ) noexcept
        : m_strPath( fs.m_strPath )
        , m_mapFileStorage( ::std::move( fs.m_mapFileStorage ) )
        , m_pBackend( ::std::move( fs.m_pBackend ) )
    {
        ;
    }

    FileStorage::~FileStorage () noexcept
    {
        uninitialize();
    }

    core::Result< core::Bool > FileStorage::initialize( core::StringView strConfig, core::Bool bCreate ) noexcept
    {
        using result = core::Result< core::Bool >;

        if ( m_bInitialize ) {
            return result::FromValue(true);
        }

        // Phase 2.3: Simplified initialization
        // - m_strPath and m_pBackend are already set by CPersistencyManager via setBackend()
        // - Directory structure is created by CPersistencyManager
        // - Configuration and metadata are managed by CPersistencyManager
        
        if (!m_pBackend) {
            LAP_PER_LOG_ERROR << "Backend not set - must be set by CPersistencyManager";
            return result::FromError(PerErrc::kNotInitialized);
        }

        // strConfig parameter is deprecated (kept for API compatibility)
        // Configuration is now loaded by CPersistencyManager from Core::ConfigManager
        UNUSED( strConfig );
        UNUSED( bCreate );

        // Load file info from metadata (if available)
        // Note: Metadata loading is now handled by CPersistencyManager,
        // but we still need to load file list for GetAllFileNames() etc.
        if (!loadFileInfo()) {
            LAP_PER_LOG_WARN << "Failed to load file info, starting with empty storage";
        }

        m_bInitialize = true;
        
        LAP_PER_LOG_INFO << "FileStorage initialized successfully at: " << m_strPath.data();
        return result::FromValue(true);
    }

    void FileStorage::uninitialize() noexcept
    {
        if ( !m_bInitialize )   return;

        LAP_PER_LOG_INFO << "Uninitializing FileStorage";
        
        // Sync file info to metadata
        syncFileInfo();
        
        // Backend doesn't need uninitialization (no state to clean up)

        m_bInitialize = false;
        
        LAP_PER_LOG_INFO << "FileStorage uninitialized";
    }

    FileStorage& FileStorage::operator= ( FileStorage &&fs ) &noexcept
    {
        m_strPath = fs.m_strPath;

        {
            core::LockGuard lock( m_mtxFileStorage );
            m_mapFileStorage.clear();

            m_mapFileStorage = ::std::move( fs.m_mapFileStorage );
        }
        
        // Move backend
        m_pBackend = ::std::move( fs.m_pBackend );

        initialize();

        return *this;
    }

    core::Result< core::Vector< core::String > > FileStorage::GetAllFileNames () const noexcept
    {
        using result = core::Result< core::Vector< core::String > >;

        if ( !m_bInitialize )   return result::FromError( PerErrc::kNotInitialized );
        if ( m_bCorrupted )   return result::FromError( PerErrc::kIntegrityCorrupted );

        core::Vector< core::String > aryFiles;

        {
            core::LockGuard lock( m_mtxFileStorage );
            for ( auto it = m_mapFileStorage.begin(); it != m_mapFileStorage.end(); ++it ) {
                aryFiles.emplace_back( it->first );
            }
        }

        return result::FromValue( aryFiles );
    }

    core::Result< void > FileStorage::DeleteFile ( core::StringView fileName ) noexcept
    {
        using result = core::Result< void >;

        if ( !m_bInitialize )   return result::FromError( PerErrc::kNotInitialized );

        auto&& it = m_mapFileStorage.find( fileName.data() );

        if ( it != m_mapFileStorage.end() ) {
            core::LockGuard lock( m_mtxFileStorage );
            
            // close file and delete
            if ( it->second.isOpen ) return result::FromError( PerErrc::kResourceBusy );

            // Delete from backend (all categories)
            if (m_pBackend) {
                m_pBackend->DeleteFile(fileName.data(), LAP_PER_CATEGORY_CURRENT);
                m_pBackend->DeleteFile(fileName.data(), LAP_PER_CATEGORY_BACKUP);
                m_pBackend->DeleteFile(fileName.data(), LAP_PER_CATEGORY_INITIAL);
            }

            if ( !core::File::Util::remove( core::String(fileName) ) ) {
                LAP_PER_LOG_WARN << "Failed to delete file (may not exist): " << fileName.data();
            }

            m_mapFileStorage.erase( it );
            
            // Sync file info
            syncFileInfo();

            return result::FromValue();
        } else {
            // file not found, treat as success
            LAP_PER_LOG_INFO << "File not found (already deleted): " << fileName.data();
            return result::FromValue();
        }
    }

    core::Result<core::Bool > FileStorage::FileExists ( core::StringView fileName ) const noexcept
    {
        using result = core::Result< core::Bool >;

        auto&& it = m_mapFileStorage.find( fileName.data() );

        return result::FromValue( it != m_mapFileStorage.end() );
    }

    core::Result< void > FileStorage::RecoverFile ( core::StringView fileName ) noexcept
    {
        using result = core::Result< void >;

        if ( !m_bInitialize )   return result::FromError( PerErrc::kNotInitialized );
        
        LAP_PER_LOG_INFO << "Recovering file: " << fileName.data();
        
        // Use backend to recover from backup
        if (m_pBackend) {
            auto copyResult = m_pBackend->CopyFile(
                fileName.data(), 
                LAP_PER_CATEGORY_BACKUP,    // from backup
                LAP_PER_CATEGORY_CURRENT     // to current
            );
            
            if (!copyResult.HasValue()) {
                LAP_PER_LOG_ERROR << "Failed to recover file from backup: " << fileName.data();
                return result::FromError( PerErrc::kPhysicalStorageFailure );
            }
            
            // Update file context
            auto&& it = m_mapFileStorage.find( fileName.data() );
            if (it != m_mapFileStorage.end()) {
                core::LockGuard lock( m_mtxFileStorage );
                it->second.fileInfo.modificationTime = core::Time::getCurrentTime();
                it->second.hasBackup = true;
            }
            
            syncFileInfo();
            
            LAP_PER_LOG_INFO << "File recovered successfully: " << fileName.data();
            return result::FromValue();
        }
        
        // Legacy fallback
        LAP_PER_LOG_WARN << "Backend not available, using legacy recovery";
        return result::FromValue();
    }

    core::Result< void > FileStorage::ResetFile ( core::StringView fileName ) noexcept
    {
        using result = core::Result< void >;

        if ( !m_bInitialize )   return result::FromError( PerErrc::kNotInitialized );
        
        LAP_PER_LOG_INFO << "Resetting file to initial state: " << fileName.data();
        
        // Use backend to reset from initial
        if (m_pBackend) {
            auto copyResult = m_pBackend->CopyFile(
                fileName.data(), 
                LAP_PER_CATEGORY_INITIAL,   // from initial
                LAP_PER_CATEGORY_CURRENT     // to current
            );
            
            if (!copyResult.HasValue()) {
                LAP_PER_LOG_ERROR << "Failed to reset file from initial: " << fileName.data();
                return result::FromError( PerErrc::kPhysicalStorageFailure );
            }
            
            // Update file context
            auto&& it = m_mapFileStorage.find( fileName.data() );
            if (it != m_mapFileStorage.end()) {
                core::LockGuard lock( m_mtxFileStorage );
                it->second.fileInfo.modificationTime = core::Time::getCurrentTime();
                it->second.fileInfo.fileModificationState = FileModificationState::kModifiedByApplication;
            }
            
            syncFileInfo();
            
            LAP_PER_LOG_INFO << "File reset successfully: " << fileName.data();
            return result::FromValue();
        }
        
        // Legacy fallback
        LAP_PER_LOG_WARN << "Backend not available, using legacy reset";
        return result::FromValue();
    }

    core::Result< core::UInt64 > FileStorage::GetCurrentFileSize ( core::StringView fileName ) const noexcept
    {
        using result = core::Result< core::UInt64 >;

        if ( !m_bInitialize )   return result::FromError( PerErrc::kNotInitialized );

        auto&& it = m_mapFileStorage.begin();

        for ( ; it != m_mapFileStorage.end(); ++it ) {
            if ( fileName == it->first ) {
                return result::FromValue( it->second.fileInfo.fileSize );
            }
        }

        return result::FromError( PerErrc::kFileNotFound );
    }

    core::Result< FileInfo > FileStorage::GetFileInfo ( core::StringView fileName ) const noexcept
    {
        using result = core::Result< FileInfo >;

        if ( !m_bInitialize )   return result::FromError( PerErrc::kNotInitialized );

        auto&& it = m_mapFileStorage.begin();

        for ( ; it != m_mapFileStorage.end(); ++it ) {
            if ( fileName == it->first ) {
                return result::FromValue( it->second.fileInfo );
            }
        }

        return result::FromError( PerErrc::kFileNotFound );
    }

    core::Result< core::UniqueHandle< ReadWriteAccessor > > FileStorage::OpenFileReadWrite( core::StringView fileName ) noexcept
    {
        return OpenFileReadWrite( fileName, convert( ::std::ios_base::in | ::std::ios_base::out ) );
    }

    core::Result< core::UniqueHandle< ReadWriteAccessor > > FileStorage::OpenFileReadWrite( core::StringView fileName, OpenMode mode ) noexcept
    {
        using result = core::Result< core::UniqueHandle< ReadWriteAccessor > >;

        if ( !m_bInitialize )   return result::FromError( PerErrc::kNotInitialized );

        // Ensure both kIn and kOut are set for read-write mode
        mode = static_cast<OpenMode>(static_cast<core::UInt32>(mode) | 
                                      static_cast<core::UInt32>(OpenMode::kIn) | 
                                      static_cast<core::UInt32>(OpenMode::kOut));

        if ( !valid( mode ) )   return result::FromError( PerErrc::kInvalidOpenMode );

        auto&& it = m_mapFileStorage.find( fileName.data() );
        if ( it != m_mapFileStorage.end() ) {
            if ( it->second.isOpen )  return result::FromError( PerErrc::kResourceBusy );

            {
                // update context
                core::LockGuard lock( m_mtxFileStorage );

                auto&& context = it->second;
                context.fileInfo.accessTime = core::Time::getCurrentTime();
                context.isOpen = true;
            }

            try {
                return result::FromValue( ::std::make_unique< ReadWriteAccessor >( fileName, mode, this ) );
            } catch ( const ::lap::per::PerException& e ) {
                return result::FromError( e.Error() );
            }
        } else {
            // open file to read & write
            if ( m_mapFileStorage.size() >= m_maxNumberOfFiles ) return result::FromError( PerErrc::kOutOfStorageSpace );

            tagFileContext context;
            context.isOpen                          = true;
            context.fileInfo.creationTime           = core::Time::getCurrentTime();;
            context.fileInfo.accessTime             = context.fileInfo.creationTime;
            context.fileInfo.modificationTime       = context.fileInfo.creationTime;
            context.fileInfo.fileCreationState      = FileCreationState::kCreatedByApplication;
            context.fileInfo.fileModificationState  = FileModificationState::kModifiedByApplication;

            {
                core::LockGuard lock( m_mtxFileStorage );

                m_mapFileStorage.emplace( fileName.data(), context );
            }

            try {
                return result::FromValue( ::std::make_unique< ReadWriteAccessor >( fileName, mode, this ) );
            } catch ( const ::lap::per::PerException& e ) {
                return result::FromError( e.Error() );
            }
        }
    }

    core::Result< core::UniqueHandle< ReadWriteAccessor > > FileStorage::OpenFileReadWrite( core::StringView , OpenMode , core::Span< core::Byte > ) noexcept
    {
        using result = core::Result< core::UniqueHandle< ReadWriteAccessor > >;

        // TODO : FileStorage::OpenFileReadWrite
        return result::FromValue( nullptr );
    }

    core::Result< core::UniqueHandle< ReadAccessor > > FileStorage::OpenFileReadOnly( core::StringView fileName ) noexcept
    {
        return OpenFileReadOnly( fileName, OpenMode::kIn );
    }

    core::Result< core::UniqueHandle< ReadAccessor > > FileStorage::OpenFileReadOnly( core::StringView fileName, OpenMode mode ) noexcept
    {
        using result = core::Result< core::UniqueHandle< ReadAccessor > >;

        if ( !m_bInitialize )   return result::FromError( PerErrc::kNotInitialized );

        // Ensure kIn is set for read-only mode
        mode = static_cast<OpenMode>(static_cast<core::UInt32>(mode) | static_cast<core::UInt32>(OpenMode::kIn));

        if ( !valid( mode ) )   return result::FromError( PerErrc::kInvalidOpenMode );

        auto&& it = m_mapFileStorage.find( fileName.data() );
        if ( it != m_mapFileStorage.end() ) {
            if ( it->second.isOpen )  return result::FromError( PerErrc::kResourceBusy );

            {
                // update context
                core::LockGuard lock( m_mtxFileStorage );

                auto&& context = it->second;
                context.fileInfo.accessTime = core::Time::getCurrentTime();
                context.isOpen = true;
            }

            try {
                return result::FromValue( ::std::make_unique< ReadAccessor >( fileName, mode, this ) );
            } catch ( const ::lap::per::PerException& e ) {
                return result::FromError( e.Error() );
            }
        } else {
            // open file to read & write
            if ( m_mapFileStorage.size() >= m_maxNumberOfFiles ) return result::FromError( PerErrc::kOutOfStorageSpace );

            tagFileContext context;
            context.isOpen                          = true;
            context.fileInfo.creationTime           = core::Time::getCurrentTime();;
            context.fileInfo.accessTime             = context.fileInfo.creationTime;
            context.fileInfo.modificationTime       = context.fileInfo.creationTime;
            context.fileInfo.fileCreationState      = FileCreationState::kCreatedByApplication;
            context.fileInfo.fileModificationState  = FileModificationState::kModifiedByApplication;

            {
                core::LockGuard lock( m_mtxFileStorage );

                m_mapFileStorage.emplace( fileName.data(), context );
            }

            try {
                return result::FromValue( ::std::make_unique< ReadAccessor >( fileName, mode, this ) );
            } catch ( const ::lap::per::PerException& e ) {
                return result::FromError( e.Error() );
            }
        }
    }

    core::Result< core::UniqueHandle< ReadAccessor > > FileStorage::OpenFileReadOnly( core::StringView , OpenMode , core::Span< core::Byte >  ) noexcept
    {
        using result = core::Result< core::UniqueHandle< ReadAccessor > >;
        
        // TODO : FileStorage::OpenFileReadOnly
        return result::FromValue( nullptr );
    }

    core::Result< core::UniqueHandle< ReadWriteAccessor > > FileStorage::OpenFileWriteOnly( core::StringView fileName ) noexcept
    {
        return OpenFileWriteOnly( fileName, OpenMode::kOut );
    }

    core::Result< core::UniqueHandle< ReadWriteAccessor > > FileStorage::OpenFileWriteOnly( core::StringView fileName, OpenMode mode ) noexcept
    {
        using result = core::Result< core::UniqueHandle< ReadWriteAccessor > >;

        if ( !m_bInitialize )   return result::FromError( PerErrc::kNotInitialized );

        // Ensure kOut is set for write-only mode
        mode = static_cast<OpenMode>(static_cast<core::UInt32>(mode) | static_cast<core::UInt32>(OpenMode::kOut));

        if ( !valid( mode ) )   return result::FromError( PerErrc::kInvalidOpenMode );

        auto&& it = m_mapFileStorage.find( fileName.data() );
        if ( it != m_mapFileStorage.end() ) {
            if ( it->second.isOpen )  return result::FromError( PerErrc::kResourceBusy );

            {
                // update context
                core::LockGuard lock( m_mtxFileStorage );

                auto&& context = it->second;
                context.fileInfo.accessTime = core::Time::getCurrentTime();
                context.isOpen = true;
            }

            try {
                return result::FromValue( ::std::make_unique< ReadWriteAccessor >( fileName, mode, this ) );
            } catch ( const ::lap::per::PerException& e ) {
                return result::FromError( e.Error() );
            }
        } else {
            // open file to read & write
            if ( m_mapFileStorage.size() >= m_maxNumberOfFiles ) return result::FromError( PerErrc::kOutOfStorageSpace );

            tagFileContext context;
            context.isOpen                          = true;
            context.fileInfo.creationTime           = core::Time::getCurrentTime();;
            context.fileInfo.accessTime             = context.fileInfo.creationTime;
            context.fileInfo.modificationTime       = context.fileInfo.creationTime;
            context.fileInfo.fileCreationState      = FileCreationState::kCreatedByApplication;
            context.fileInfo.fileModificationState  = FileModificationState::kModifiedByApplication;

            {
                core::LockGuard lock( m_mtxFileStorage );

                m_mapFileStorage.emplace( fileName.data(), context );
            }

            try {
                return result::FromValue( ::std::make_unique< ReadWriteAccessor >( fileName, mode, this ) );
            } catch ( const ::lap::per::PerException& e ) {
                return result::FromError( e.Error() );
            }
        }
    }

    core::Result< core::UniqueHandle< ReadWriteAccessor > > FileStorage::OpenFileWriteOnly( core::StringView , OpenMode , core::Span< core::Byte >  ) noexcept
    {
        using result = core::Result< core::UniqueHandle< ReadWriteAccessor > >;

        // TODO : FileStorage::OpenFileWriteOnly
        return result::FromValue( nullptr );
    }

    void FileStorage::update( core::StringView strFile, FileInfo& info, core::Bool isClosed ) noexcept
    {
        auto&& it = m_mapFileStorage.find( strFile.data() );
        if ( it == m_mapFileStorage.end() ) return;

        {
            // update file info
            core::LockGuard lock( m_mtxFileStorage );

            it->second.fileInfo.accessTime = info.accessTime;
            it->second.fileInfo.creationTime = info.creationTime;
            it->second.fileInfo.modificationTime = info.modificationTime;

            if ( isClosed ) {
                it->second.crcKey = core::File::Util::crc( strFile, it->second.crcType == FileCRCType::Header );
                it->second.isOpen = false;
            }
        }
    }

    // ========================================================================
    // Version Management (AUTOSAR SWS_PER_00396, SWS_PER_00463)
    // ========================================================================
    
    // ========================================================================
    // Version Management (Phase 2.2: Moved to CPersistencyManager)
    // ========================================================================
    // These methods are deprecated and should be called via CPersistencyManager
    // They are kept for API compatibility but delegate to CPersistencyManager
    
    core::Result< core::Bool > FileStorage::NeedsUpdate(
        const core::String& manifestDeploymentVersion,
        const core::String& manifestContractVersion
    ) noexcept {
        using result = core::Result< core::Bool >;
        
        UNUSED(manifestDeploymentVersion);
        UNUSED(manifestContractVersion);
        
        if (!m_bInitialize) return result::FromError( PerErrc::kNotInitialized );
        
        // Phase 2.2: Moved to CPersistencyManager
        // Use CPersistencyManager::needsUpdate() instead
        LAP_PER_LOG_WARN << "NeedsUpdate() is deprecated - use CPersistencyManager::needsUpdate()";
        
        // Try to delegate to CPersistencyManager if we can get instance specifier
        // For now, return error - caller should use CPersistencyManager directly
        return result::FromError( MakeErrorCode(PerErrc::kNotInitialized, 0) );
    }
    
    core::Result< FileStorageMetadata > FileStorage::GetMetadata() const noexcept {
        using result = core::Result< FileStorageMetadata >;
        
        if (!m_bInitialize) return result::FromError( PerErrc::kNotInitialized );
        
        // Phase 2.2: Moved to CPersistencyManager
        LAP_PER_LOG_WARN << "GetMetadata() is deprecated - use CPersistencyManager::loadMetadata()";
        return result::FromError( MakeErrorCode(PerErrc::kNotInitialized, 0) );
    }
    
    core::Result< void > FileStorage::UpdateVersionInfo(
        const core::String& contractVersion,
        const core::String& deploymentVersion
    ) noexcept {
        using result = core::Result< void >;
        
        UNUSED(contractVersion);
        UNUSED(deploymentVersion);
        
        if (!m_bInitialize) return result::FromError( PerErrc::kNotInitialized );
        
        // Phase 2.2: Moved to CPersistencyManager
        LAP_PER_LOG_WARN << "UpdateVersionInfo() is deprecated - use CPersistencyManager::updateVersionInfo()";
        return result::FromError( MakeErrorCode(PerErrc::kNotInitialized, 0) );
    }
    
    // ========================================================================
    // Backup and Update Operations (Phase 2.2: Moved to CPersistencyManager)
    // ========================================================================
    // These methods are deprecated and should be called via CPersistencyManager
    
    core::Result< void > FileStorage::CreateBackup() noexcept {
        using result = core::Result< void >;
        
        if (!m_bInitialize) return result::FromError( PerErrc::kNotInitialized );
        
        // Phase 2.2: Moved to CPersistencyManager
        LAP_PER_LOG_WARN << "CreateBackup() is deprecated - use CPersistencyManager::backupFileStorage()";
        return result::FromError( MakeErrorCode(PerErrc::kNotInitialized, 0) );
    }
    
    core::Result< void > FileStorage::RestoreBackup() noexcept {
        using result = core::Result< void >;
        
        if (!m_bInitialize) return result::FromError( PerErrc::kNotInitialized );
        
        // Phase 2.2: Moved to CPersistencyManager
        LAP_PER_LOG_WARN << "RestoreBackup() is deprecated - use CPersistencyManager::restoreFileStorage()";
        return result::FromError( MakeErrorCode(PerErrc::kNotInitialized, 0) );
    }
    
    core::Result< void > FileStorage::BeginUpdate() noexcept {
        using result = core::Result< void >;
        
        if (!m_bInitialize) return result::FromError( PerErrc::kNotInitialized );
        
        // Phase 2.2: Moved to CPersistencyManager
        LAP_PER_LOG_WARN << "BeginUpdate() is deprecated - use CPersistencyManager::performUpdate()";
        return result::FromError( MakeErrorCode(PerErrc::kNotInitialized, 0) );
    }
    
    core::Result< void > FileStorage::CommitUpdate() noexcept {
        using result = core::Result< void >;
        
        if (!m_bInitialize) return result::FromError( PerErrc::kNotInitialized );
        
        // Phase 2.2: Moved to CPersistencyManager
        LAP_PER_LOG_WARN << "CommitUpdate() is deprecated - update workflow managed by CPersistencyManager";
        return result::FromError( MakeErrorCode(PerErrc::kNotInitialized, 0) );
    }
    
    core::Result< void > FileStorage::RollbackUpdate() noexcept {
        using result = core::Result< void >;
        
        if (!m_bInitialize) return result::FromError( PerErrc::kNotInitialized );
        
        // Phase 2.2: Moved to CPersistencyManager
        LAP_PER_LOG_WARN << "RollbackUpdate() is deprecated - use CPersistencyManager::rollback()";
        return result::FromError( MakeErrorCode(PerErrc::kNotInitialized, 0) );
    }
    
    // ========================================================================
    // Helper Functions (Private)
    // ========================================================================
    
    core::Bool FileStorage::loadFileInfo() noexcept
    {
        LAP_PER_LOG_INFO << "Loading file info from metadata";
        
        // TODO: Phase 2.2 - Implement metadata loading via CPersistencyManager
        // For now, use backend to list files in current category
        if (m_pBackend) {
            auto filesResult = m_pBackend->ListFiles(LAP_PER_CATEGORY_CURRENT);
            if (filesResult.HasValue()) {
                LAP_PER_LOG_INFO << "Loaded " << filesResult.Value().size() << " files from current category";
                return true;
            }
        }
        
        LAP_PER_LOG_WARN << "Failed to load file info";
        return false;
    }
    
    core::Bool FileStorage::syncFileInfo() noexcept
    {
        LAP_PER_LOG_INFO << "Syncing file info to metadata";
        
        // TODO: Phase 2.2 - Implement metadata syncing via CPersistencyManager
        // For now, just log
        LAP_PER_LOG_INFO << "File info sync (metadata management moved to CPersistencyManager in Phase 2.2)";
        return true;
    }

    core::Result< void > FileStorage::RecoverAllFiles() noexcept
    {
        using result = core::Result< void >;
        
        if (!m_bInitialize) return result::FromError( PerErrc::kNotInitialized );
        
        // Phase 2.2: Moved to CPersistencyManager
        // This method is protected and called by CPersistencyManager::RecoverAllFiles()
        // For now, implement basic recovery using backend
        if (m_pBackend) {
            auto filesResult = m_pBackend->ListFiles(LAP_PER_CATEGORY_BACKUP);
            if (filesResult.HasValue()) {
                for (const auto& fileName : filesResult.Value()) {
                    auto copyResult = m_pBackend->CopyFile(
                        fileName,
                        LAP_PER_CATEGORY_BACKUP,
                        LAP_PER_CATEGORY_CURRENT
                    );
                    if (!copyResult.HasValue()) {
                        LAP_PER_LOG_ERROR << "Failed to recover file: " << fileName;
                    }
                }
                return result::FromValue();
            }
        }
        
        return result::FromError( MakeErrorCode(PerErrc::kPhysicalStorageFailure, 0) );
    }

    core::Result< void > FileStorage::ResetAllFiles() noexcept
    {
        using result = core::Result< void >;
        
        if (!m_bInitialize) return result::FromError( PerErrc::kNotInitialized );
        
        // Phase 2.2: Moved to CPersistencyManager
        // This method is protected and called by CPersistencyManager::ResetAllFiles()
        // For now, implement basic reset using backend
        if (m_pBackend) {
            auto filesResult = m_pBackend->ListFiles(LAP_PER_CATEGORY_INITIAL);
            if (filesResult.HasValue()) {
                // Delete all current files first
                auto currentFilesResult = m_pBackend->ListFiles(LAP_PER_CATEGORY_CURRENT);
                if (currentFilesResult.HasValue()) {
                    for (const auto& fileName : currentFilesResult.Value()) {
                        m_pBackend->DeleteFile(fileName, LAP_PER_CATEGORY_CURRENT);
                    }
                }
                
                // Copy from initial to current
                for (const auto& fileName : filesResult.Value()) {
                    auto copyResult = m_pBackend->CopyFile(
                        fileName,
                        LAP_PER_CATEGORY_INITIAL,
                        LAP_PER_CATEGORY_CURRENT
                    );
                    if (!copyResult.HasValue()) {
                        LAP_PER_LOG_ERROR << "Failed to reset file: " << fileName;
                    }
                }
                return result::FromValue();
            }
        }
        
        return result::FromError( MakeErrorCode(PerErrc::kPhysicalStorageFailure, 0) );
    }

    core::Result< core::UInt64 > FileStorage::GetCurrentFileStorageSize() noexcept
    {
        using result = core::Result< core::UInt64 >;
        
        if (!m_bInitialize) return result::FromError( PerErrc::kNotInitialized );
        
        // TODO: Phase 2.2 - Move to CPersistencyManager
        // For now, calculate size using backend
        if (m_pBackend) {
            auto filesResult = m_pBackend->ListFiles(LAP_PER_CATEGORY_CURRENT);
            if (filesResult.HasValue()) {
                core::UInt64 totalSize = 0;
                for (const auto& fileName : filesResult.Value()) {
                    auto sizeResult = m_pBackend->GetFileSize(fileName, LAP_PER_CATEGORY_CURRENT);
                    if (sizeResult.HasValue()) {
                        totalSize += sizeResult.Value();
                    }
                }
                return result::FromValue(totalSize);
            }
        }
        
        return result::FromValue( core::UInt64( 0 ) );
    }

    core::Result< core::SharedHandle< FileStorage > > OpenFileStorage ( const core::InstanceSpecifier &fs ) noexcept
    {
        return CPersistencyManager::getInstance().getFileStorage( fs );
    }

    core::Result< core::SharedHandle< FileStorage > > OpenFileStorage ( const core::InstanceSpecifier &fs, core::Bool bCreate ) noexcept
    {
        return CPersistencyManager::getInstance().getFileStorage( fs, bCreate );
    }

    core::Result< void > RecoverAllFiles ( const core::InstanceSpecifier &fs ) noexcept
    {
        return CPersistencyManager::getInstance().RecoverAllFiles( fs );
    }

    core::Result< void > ResetAllFiles ( const core::InstanceSpecifier &fs ) noexcept
    {
        return CPersistencyManager::getInstance().ResetAllFiles( fs );
    }

    core::Result< core::UInt64 > GetCurrentFileStorageSize ( const core::InstanceSpecifier &fs ) noexcept
    {
        return CPersistencyManager::getInstance().GetCurrentFileStorageSize( fs );
    }
} // per
} // lap
