/**
 * @file CReadAccessor.hpp
 * @author ddkv587 (ddkv587@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2024-02-02
 * 
 * 
 */
#ifndef LAP_PERSISTENCY_READACCESSOR_HPP
#define LAP_PERSISTENCY_READACCESSOR_HPP

#include <lap/core/CTime.hpp>
#include <lap/core/CMemory.hpp>

#include "CFileStorage.hpp"
#include "CDataType.hpp"

namespace lap
{
namespace pm
{
    class ReadAccessor
    {
    public:
        IMP_OPERATOR_NEW(ReadAccessor)
        
    public:
        ReadAccessor ( core::StringView, OpenMode, FileStorage* );
        ReadAccessor ( ReadAccessor &&ra ) noexcept;
        virtual ~ReadAccessor () noexcept;

        core::Result< core::Char >                      PeekChar () const noexcept;
        core::Result< core::Byte >                      PeekByte () const noexcept;

        core::Result< core::Char >                      GetChar () noexcept;
        core::Result< core::Byte >                      GetByte () noexcept;

        core::Result< core::String >                    ReadText () noexcept;
        core::Result< core::String >                    ReadText ( core::UInt64 n ) noexcept;

        core::Result< core::Vector< core::Byte > >      ReadBinary () noexcept;
        core::Result< core::Vector< core::Byte > >      ReadBinary( core::UInt64 n ) noexcept;

        core::Result< core::String >                    ReadLine ( core::Char delimiter='\n' ) noexcept;

        core::UInt64                                    GetSize () const noexcept;
        core::UInt64                                    GetPosition () const noexcept;
        core::Result< void >                            SetPosition ( core::UInt64 position ) noexcept;
        core::Result< core::UInt64 >                    MovePosition ( Origin origin, core::Int64 offset ) noexcept;
        core::Bool                                      IsEof () const noexcept;

        ReadAccessor&                                   operator= ( ReadAccessor &&ra ) &noexcept;

        ReadAccessor ( const ReadAccessor & ) = delete;
        ReadAccessor& operator= ( const ReadAccessor & ) = delete;

        // template<typename...Args>
        // static auto Create(Args&&... args)  { return std::unique_ptr<ReadAccessor> ( std::forward<Args>(args)... ); }

    protected:
        friend class FileStorage;

        // helper function
        inline core::Bool                               checkRead() const noexcept                              { return ( m_openMode & OpenMode::kIn ); }
        inline core::Bool                               checkWrite() const noexcept                             { return ( m_openMode & OpenMode::kOut ); }
        inline core::Bool                               checkBinary() const noexcept                            { return ( m_openMode & OpenMode::kBinary ); }
        inline core::Bool                               isGood() const noexcept                                 { return m_fpStream && m_fpStream->good(); }

        // expose private member for inherit and friend class
        inline core::StringView                         file() const noexcept                                   { return m_strFile; }
        inline OpenMode                                 mode() const noexcept                                   { return m_openMode; }
        inline core::UniqueHandle< ::std::fstream >&    stream() noexcept                                       { return m_fpStream; }
        
        // fstream function
        virtual void                                    seek( ::std::streampos pos ) const noexcept;
        virtual void                                    seek( ::std::streamoff off, ::std::ios_base::seekdir way ) const noexcept;
        virtual ::std::streampos                        tell() const noexcept;

        // update for file storage
        virtual void                                    update( core::Bool isClosed = true ) noexcept;

        virtual void                                    updateCreateTime() noexcept                             { m_fileInfo.creationTime = core::Time::getCurrentTime(); }
        virtual void                                    updateAccessTime() noexcept                             { m_fileInfo.accessTime = core::Time::getCurrentTime(); }
        virtual void                                    updateModifyTime() noexcept                             { m_fileInfo.modificationTime = core::Time::getCurrentTime(); }
        virtual void                                    updateFileSize( core::Size fileSize ) noexcept          { m_fileInfo.fileSize = fileSize; }
        virtual void                                    appendFileSize( core::Size fileSize ) noexcept          { m_fileInfo.fileSize += fileSize; }

    private:
        core::StringView                                m_strFile{""};
        FileStorage*                                    m_fsParent{nullptr};

        struct FileInfo                                 m_fileInfo;
        OpenMode                                        m_openMode{OpenMode::kEnd};
        core::UniqueHandle< ::std::fstream >            m_fpStream{nullptr};
    };
}
}

#endif
