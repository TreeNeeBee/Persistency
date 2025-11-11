#include <cstdio>
#include <lap/core/CPath.hpp>
#include <lap/core/CTime.hpp>
#include <lap/core/CFile.hpp>
#include "CReadAccessor.hpp"
#include "CReadWriteAccessor.hpp"
#include "CFileStorage.hpp"
#include "CPerErrorDomain.hpp"
#include "CPersistencyManager.hpp"

namespace lap
{
namespace per
{
    FileStorage::FileStorage ( core::StringView strIdentifier ) noexcept
        : m_strPath( strIdentifier )
    {
        ;
    }

    FileStorage::FileStorage ( FileStorage &&fs ) noexcept
        : m_strPath( fs.m_strPath )
        , m_mapFileStorage( ::std::move( fs.m_mapFileStorage ) )
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

        if ( m_bInitialize )    return result::FromValue(true);

        core::StringView strCurPath = core::Path::append( m_strPath, DEF_FS_CURRENT_PATH_APPEND );
        core::StringView strBackUpPath = core::Path::append( m_strPath, DEF_FS_BACKUP_PATH_APPEND );
        core::StringView strInitialPath = core::Path::append( m_strPath, DEF_FS_INITIAL_PATH_APPEND );
        core::StringView strFileInfo = core::Path::append( m_strPath, DEF_FS_INFO_DATA );

        if ( !bCreate && 
                ( !core::Path::isDirectory( strCurPath ) || 
                    !core::Path::isDirectory( strBackUpPath ) || 
                    !core::Path::isDirectory( strInitialPath ) ||
                    !core::Path::isFile( strFileInfo ) ) ) {
            LAP_PER_LOG_ERROR << "FileStorage::initialize failed!!: " << m_strPath.data();
            return result::FromError( PerErrc::kPhysicalStorageFailure );
        }

        // if ( !core::Path::createDirectory( strCurPath ) ||
        //         !core::Path::createDirectory( strBackUpPath ) || 
        //         !core::Path::createDirectory( strInitialPath ) ||
        //         !core::Path::createDirectory( strInitialPath ) ) {
        //     LAP_PER_LOG_ERROR << "FileStorage::initialize with createFolder failed!!: " << m_strPath.data();
        //     return result::FromError( PerErrc::kPhysicalStorageFailure );
        // }

        // return if decryption failed
        // result::FromError( PerErrc::kEncryptionFailed );

        // parse from file info 
        result::FromError( PerErrc::kValidationFailed );

        // parse config
        UNUSED( strConfig );

        m_bInitialize = true;

        return result::FromValue( m_bInitialize );
    }

    void FileStorage::uninitialize() noexcept
    {
        if ( !m_bInitialize )   return;

        // TODO : FileStorage::uninitialize
    }

    FileStorage& FileStorage::operator= ( FileStorage &&fs ) &noexcept
    {
        m_strPath = fs.m_strPath;

        {
            core::LockGuard lock( m_mtxFileStorage );
            m_mapFileStorage.clear();

            m_mapFileStorage = ::std::move( fs.m_mapFileStorage );
        }

        initialize();

        return *this;
    }

    core::Result< core::Vector< core::String > > FileStorage::GetAllFileNames () const noexcept
    {
        using result = core::Result< core::Vector< core::String > >;

        if ( !m_bInitialize )   return result::FromError( PerErrc::kNotInitialized );
        if ( m_bCorrupted )   return result::FromError( PerErrc::kNotInitialized );

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

            if ( !core::File::deleteFile( fileName ) ) result::FromError( PerErrc::kIntegrityCorrupted );

            m_mapFileStorage.erase( it );

            return result::FromValue();
        } else {
            // file not found, treat as success
            return result::FromValue();
        }
    }

    core::Result<core::Bool > FileStorage::FileExists ( core::StringView fileName ) const noexcept
    {
        using result = core::Result< core::Bool >;

        auto&& it = m_mapFileStorage.find( fileName.data() );

        return result::FromValue( it != m_mapFileStorage.end() );
    }

    core::Result< void > FileStorage::RecoverFile ( core::StringView ) noexcept
    {
        using result = core::Result< void >;

        // TODO : FileStorageBase::RecoverFile
        return m_bInitialize ? result::FromValue() : result::FromError( PerErrc::kNotInitialized );
    }

    core::Result< void > FileStorage::ResetFile ( core::StringView ) noexcept
    {
        using result = core::Result< void >;

        // TODO : FileStorageBase::ResetFile
        return m_bInitialize ? result::FromValue(): result::FromError( PerErrc::kNotInitialized );
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
                it->second.crcKey = core::File::crc( strFile, it->second.crcType == FileCRCType::Header );
                it->second.isOpen = false;
            }
        }
    }

    core::Result< void > FileStorage::RecoverAllFiles() noexcept
    {
        return core::Result< void >::FromValue();
    }

    core::Result< void > FileStorage::ResetAllFiles() noexcept
    {
        return core::Result< void >::FromValue();
    }

    core::Result< core::UInt64 > FileStorage::GetCurrentFileStorageSize() noexcept
    {
        return core::Result< core::UInt64 >::FromValue( core::UInt64( 0 ) );
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
} // pm
} // ara
