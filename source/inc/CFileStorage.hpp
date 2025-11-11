/**
 * @file CFileStorage.hpp
 * @author ddkv587 (ddkv587@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2024-02-02
 * 
 * 
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

namespace lap 
{
namespace per 
{
    class ReadAccessor;
    class ReadWriteAccessor;
    class FileStorage final
    {
        IMP_OPERATOR_NEW( FileStorage );

    public:
        struct tagFileContext
        {
            struct FileInfo                     fileInfo;
            FileCRCType                         crcType;
            core::UInt32                        crcKey;
            core::Bool                          isOpen;
        };

    private:
        #define DEF_FS_CURRENT_PATH_APPEND      "cur"
        #define DEF_FS_BACKUP_PATH_APPEND       "backup"
        #define DEF_FS_INITIAL_PATH_APPEND      "initial"

        #define DEF_FS_INFO_DATA                ".fsinfo"
        #define DEF_FS_INFO_DATA_KEY            "bc0eb773-c56e-400f-b96e-5d9e36f7fa78"

    public:
        ~FileStorage () noexcept;

        core::Result< core::Bool >                                  initialize( core::StringView strConfig = "", core::Bool bCreate = false ) noexcept;
        void                                                        uninitialize() noexcept;
        inline core::Bool                                           isInitialized() noexcept                            { return m_bInitialize; }

        inline core::Bool                                           isResourceBusy() noexcept                           { return m_bResourceBusy; }

        inline void                                                 SetMaxNumberOfFiles( core::UInt32 num ) noexcept    { m_maxNumberOfFiles = num; }
        inline core::UInt32                                         GetMaxNumberOfFiles() const noexcept                { return m_maxNumberOfFiles; }

        core::Result< core::Vector< core::String > >                GetAllFileNames () const noexcept;

        core::Result< void >                                        DeleteFile ( core::StringView fileName ) noexcept;
        core::Result< core::Bool >                                  FileExists ( core::StringView fileName ) const noexcept;
        core::Result< void >                                        RecoverFile ( core::StringView fileName ) noexcept;
        core::Result< void >                                        ResetFile ( core::StringView fileName ) noexcept;
        core::Result< core::UInt64 >                                GetCurrentFileSize ( core::StringView fileName ) const noexcept;
        core::Result< FileInfo >                                    GetFileInfo ( core::StringView fileName ) const noexcept;

        core::Result< core::UniqueHandle< ReadWriteAccessor > >     OpenFileReadWrite( core::StringView fileName ) noexcept;
        core::Result< core::UniqueHandle< ReadWriteAccessor > >     OpenFileReadWrite( core::StringView fileName, OpenMode mode ) noexcept;
        core::Result< core::UniqueHandle< ReadWriteAccessor > >     OpenFileReadWrite( core::StringView fileName, OpenMode mode, core::Span< core::Byte > buffer ) noexcept;

        core::Result< core::UniqueHandle< ReadAccessor > >          OpenFileReadOnly( core::StringView fileName ) noexcept;
        core::Result< core::UniqueHandle< ReadAccessor > >          OpenFileReadOnly( core::StringView fileName, OpenMode mode ) noexcept;
        core::Result< core::UniqueHandle< ReadAccessor > >          OpenFileReadOnly( core::StringView fileName, OpenMode mode, core::Span< core::Byte > buffer ) noexcept;

        core::Result< core::UniqueHandle< ReadWriteAccessor > >     OpenFileWriteOnly( core::StringView fileName ) noexcept;
        core::Result< core::UniqueHandle< ReadWriteAccessor > >     OpenFileWriteOnly( core::StringView fileName, OpenMode mode ) noexcept;
        core::Result< core::UniqueHandle< ReadWriteAccessor > >     OpenFileWriteOnly( core::StringView fileName, OpenMode mode, core::Span< core::Byte > buffer ) noexcept;

        void                                                        update( core::StringView, FileInfo&, core::Bool isClosed = true ) noexcept;

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

        core::Result< void >            RecoverAllFiles() noexcept;
        core::Result< void >            ResetAllFiles() noexcept;
        core::Result< core::UInt64 >    GetCurrentFileStorageSize() noexcept;

        inline core::StringView         path() noexcept             { return m_strPath; }

    private:
        core::Bool                      loadFileInfo() noexcept;
        core::Bool                      syncFileInfo() noexcept;

        inline core::StringView formatPath( core::StringView extra ) noexcept
        {
            return core::Path::append( m_strPath, extra );
        }

        inline core::StringView formatCurPath( core::StringView fileName ) noexcept
        {
            return core::Path::append( formatPath( DEF_FS_CURRENT_PATH_APPEND ), fileName );
        }

        inline core::StringView formatRevoveryPath( core::StringView fileName ) noexcept
        {
            return core::Path::append( formatPath( DEF_FS_BACKUP_PATH_APPEND ), fileName );
        }

        inline core::StringView formatResetPath( core::StringView fileName ) noexcept
        {
            return core::Path::append( formatPath( DEF_FS_INITIAL_PATH_APPEND ), fileName );
        }

    private:
        core::Bool                                                  m_bValid{false};
        core::Bool                                                  m_bCorrupted{false};
        core::Bool                                                  m_bDecryption{false};
        core::Bool                                                  m_bInitialize{false};
        core::Bool                                                  m_bResourceBusy{false};
        core::StringView                                            m_strPath{""};
        core::UInt32                                                m_maxNumberOfFiles{32};

        mutable core::Mutex                                         m_mtxFileStorage;
        ::std::unordered_map< core::String, tagFileContext >        m_mapFileStorage;
    };

    core::Result< core::SharedHandle< FileStorage > >               OpenFileStorage ( const core::InstanceSpecifier &fs ) noexcept;
    core::Result< core::SharedHandle< FileStorage > >               OpenFileStorage ( const core::InstanceSpecifier &fs, core::Bool bCreate ) noexcept;
    core::Result< void >                                            RecoverAllFiles ( const core::InstanceSpecifier &fs ) noexcept;
    core::Result< void >                                            ResetAllFiles ( const core::InstanceSpecifier &fs ) noexcept;
    core::Result< core::UInt64 >                                    GetCurrentFileStorageSize ( const core::InstanceSpecifier &fs ) noexcept;
} // pm
} // ara

#endif
