#include <cerrno>
#include "CReadAccessor.hpp"

namespace lap
{
namespace per
{
    ReadAccessor::ReadAccessor ( core::StringView strFilePath, OpenMode mode, FileStorage* parent )
        : m_strFile( strFilePath )
        , m_fsParent( parent )
        , m_openMode( mode )
    {
        m_fpStream = ::std::make_unique< ::std::fstream >();

        try {
            // Get actual file path from backend
            core::String actualPath;
            if (parent && parent->GetBackend()) {
                auto pathUri = parent->GetBackend()->GetFileUri(strFilePath.data(), LAP_PER_CATEGORY_CURRENT);
                actualPath = pathUri.GetFullPath();
            } else {
                actualPath = strFilePath.data();
            }

            ::boost::system::error_code ec;
            core::Bool fileExists = ::boost::filesystem::exists( actualPath.c_str(), ec );
            
            // For write modes, create parent directory if needed
            if (static_cast<core::UInt32>(mode & OpenMode::kOut) != 0) {
                ::boost::filesystem::path p(actualPath.c_str());
                if (p.has_parent_path()) {
                    ::boost::filesystem::create_directories(p.parent_path(), ec);
                }
            } else {
                // For read-only mode, file must exist
                if ( !fileExists || !::boost::filesystem::is_regular_file( actualPath.c_str(), ec ) ) {
                    throw PerException( PerErrc::kFileNotFound );
                }
            }

            if ( !fileExists )   updateCreateTime();

            m_fpStream->open( actualPath.c_str(), convert( mode ) );
#ifdef LAP_DEBUG
            LAP_PER_LOG_DEBUG.logFormat( "ReadAccessor open with %s, mode: 0x%x", actualPath.c_str(), static_cast< core::Int32 >( m_openMode ) );
#endif
        } catch ( const ::std::ios_base::failure& fail ) {
            LAP_PER_LOG_ERROR << "ReadAccessor open failed " << fail.what();

            m_fpStream = nullptr;

            switch( errno ) {
            case EACCES:
                throw PerException( PerErrc::KPermissionDenied );
            case ENOENT:
                throw PerException( PerErrc::kFileNotFound );
            default:
                throw PerException( PerErrc::kIntegrityCorrupted );
            }
        }
    }

    ReadAccessor::~ReadAccessor () noexcept
    {
        if ( m_fpStream ) {
            if ( m_fpStream->is_open() )    m_fpStream->close();

            m_fpStream.reset();
        }

        // update context info
        update();
    }

    ReadAccessor::ReadAccessor ( ReadAccessor &&ra ) noexcept
        : m_fsParent( ra.m_fsParent )
        , m_openMode( ra.m_openMode )
        , m_fpStream( ::std::move( ra.m_fpStream ) )
    {
        ;
    }

    ReadAccessor& ReadAccessor::operator= ( ReadAccessor &&ra ) &noexcept
    {
        m_openMode = ra.m_openMode;
        m_fpStream.swap( ra.m_fpStream );

        return *this;
    }

    core::Result< core::Char > ReadAccessor::PeekChar () const noexcept
    {
        assert( m_fpStream );

        using result = core::Result< core::Char >;
        if ( !CPersistencyManager::getInstance().isInitialized() ) return result::FromError( PerErrc::kNotInitialized );

        if ( m_fpStream->good() ) {
            //updateAccessTime();

            return result::FromValue( static_cast< core::Char >( m_fpStream->peek() ) );
        }

        return result::FromError( PerErrc::kIsEof );
    }

    core::Result< core::Byte > ReadAccessor::PeekByte () const noexcept
    {
        assert( m_fpStream );

        using result = core::Result< core::Byte >;
        if ( !CPersistencyManager::getInstance().isInitialized() ) return result::FromError( PerErrc::kNotInitialized );

        if ( m_fpStream->good() ) {
            //updateAccessTime();

            return result::FromValue( static_cast< core::Byte >( m_fpStream->peek() ) );
        }

        return result::FromError( PerErrc::kIsEof );
    }

    core::Result< core::Char > ReadAccessor::GetChar () noexcept
    {
        assert( m_fpStream );

        using result = core::Result< core::Char >;
        if ( !CPersistencyManager::getInstance().isInitialized() ) return result::FromError( PerErrc::kNotInitialized );

        if ( m_fpStream->good() ) {
            updateAccessTime();

            return result::FromValue( static_cast< core::Char >( m_fpStream->get() ) );
        }

        return result::FromError( PerErrc::kIsEof );
    }

    core::Result< core::Byte > ReadAccessor::GetByte () noexcept
    {
        assert( m_fpStream );

        using result = core::Result< core::Byte >;

        if ( m_fpStream->good() ) {
            updateAccessTime();

            return result::FromValue( static_cast< core::Byte >( m_fpStream->get() ) );
        }

        return result::FromError( PerErrc::kIsEof );
    }

    core::Result< core::String > ReadAccessor::ReadText () noexcept
    {
        assert( m_fpStream );

        using result = core::Result< core::String >;
        if ( !CPersistencyManager::getInstance().isInitialized() ) return result::FromError( PerErrc::kNotInitialized );

        if ( m_fpStream->good() ) {
            try {
                core::String buffer( ::std::istreambuf_iterator< core::Char >{ *m_fpStream }, {} );

                // try to set eof
                m_fpStream->peek();
                updateAccessTime();
                return result::FromValue( buffer );
            } catch ( const std::exception& e ) {
                return result::FromError( PerErrc::kIntegrityCorrupted );
            }
        }

        return result::FromError( PerErrc::kIsEof );
    }

    core::Result< core::String > ReadAccessor::ReadText ( core::UInt64 n ) noexcept
    {
        assert( m_fpStream );
        using result = core::Result< core::String >;
        if ( !CPersistencyManager::getInstance().isInitialized() ) return result::FromError( PerErrc::kNotInitialized );

        auto pos = tell();
        
        ::std::string str( n, '\0' );
        if ( !m_fpStream->good() || m_fpStream->readsome( &str[0], n ) <= 0 ) {
            seek( pos );
            return result::FromError( PerErrc::kIsEof );
        } else {
            // try to set eof
            m_fpStream->peek();
            updateAccessTime();
            return result::FromValue( str );
        }
    }

    core::Result< core::Vector< core::Byte > > ReadAccessor::ReadBinary () noexcept
    {
        assert( m_fpStream );
        using result = core::Result< core::Vector< core::Byte > >;
        if ( !CPersistencyManager::getInstance().isInitialized() ) return result::FromError( PerErrc::kNotInitialized );

        if ( m_fpStream->good() ) {
            core::Vector< core::Byte > data;
            while( !m_fpStream->eof() ) {
                data.push_back( m_fpStream->get() );
            }
            updateAccessTime();
            return result::FromValue( data );
        } else {
            return result::FromError( PerErrc::kIsEof );
        }
    }

    core::Result< core::Vector< core::Byte > > ReadAccessor::ReadBinary( core::UInt64 n ) noexcept
    {
        assert( m_fpStream );

        using result = core::Result< core::Vector< core::Byte > >;
        if ( !CPersistencyManager::getInstance().isInitialized() ) return result::FromError( PerErrc::kNotInitialized );

        if ( m_fpStream->good() ) {
            core::Vector< core::Byte > data;

            core::UInt64 size{0};
            while( !m_fpStream->eof() || size >= n ) {
                data.push_back( m_fpStream->get() );
                ++size;
            }
            updateAccessTime();
            return result::FromValue( data );
        } else {
            return result::FromError( PerErrc::kIsEof );
        }
    }

    core::Result< core::String > ReadAccessor::ReadLine ( core::Char delimiter ) noexcept
    {
        assert( m_fpStream );

        using result = core::Result< core::String >;
        if ( !CPersistencyManager::getInstance().isInitialized() ) return result::FromError( PerErrc::kNotInitialized );

        if ( m_fpStream->good() ) {
            ::std::string buffer;
            ::std::getline( *m_fpStream, buffer, delimiter );

            // try to set eof
            m_fpStream->peek();
            updateAccessTime();
            return result::FromValue( buffer );
        } else {
            return result::FromError( PerErrc::kIsEof );
        }
    }

    core::UInt64 ReadAccessor::GetSize () const noexcept
    {
        auto pos = tell();

        seek( 0, ::std::ios_base::end );
        auto size = tell();

        seek( pos );
        return size;
    }

    core::UInt64 ReadAccessor::GetPosition () const noexcept
    {
        return tell();
    }

    core::Result< void > ReadAccessor::SetPosition ( core::UInt64 position ) noexcept
    {
        using result = core::Result< void >;

        if ( !CPersistencyManager::getInstance().isInitialized() ) return result::FromError( PerErrc::kNotInitialized );
 
        auto pos = tell();

        seek( position );

        if ( m_fpStream->fail() ) {
            LAP_PER_LOG_WARN << "ReadAccessorBase::SetPosition seek failed";

            seek( pos );
            return result::FromError( PerErrc::kInvalidPosition );
        } else {
            return result::FromValue();
        }
    }

    core::Result< core::UInt64 > ReadAccessor::MovePosition ( Origin origin, core::Int64 offset ) noexcept
    {
        using result = core::Result< core::UInt64 >;

        if ( !CPersistencyManager::getInstance().isInitialized() ) return result::FromError( PerErrc::kNotInitialized );

        auto pos = tell();

        seek( offset, convert( origin ) );

        if ( m_fpStream->fail() ) {
            LAP_PER_LOG_WARN << "ReadAccessorBase::MovePosition seek failed";
            seek( pos );

            return result::FromError( PerErrc::kInvalidPosition );
        } else {
            return result::FromValue( GetPosition() );
        }
    }

    core::Bool ReadAccessor::IsEof () const noexcept
    {
        assert( m_fpStream );

        // Peek to update EOF flag if at end
        m_fpStream->peek();
        return m_fpStream->eof();
    }

    void ReadAccessor::update( core::Bool isClosed ) noexcept
    {
        // update fs context
        if ( m_fsParent ) {
            m_fsParent->update( m_strFile, m_fileInfo, isClosed );
        }
    }

    void ReadAccessor::seek( ::std::streampos pos ) const noexcept
    {
        assert( m_fpStream );

        m_fpStream->clear();
        if ( checkWrite() ) {
            m_fpStream->seekp( pos );   // output sequence
        } else {
            m_fpStream->seekg( pos );   // input sequence
        }
    }

    void ReadAccessor::seek( ::std::streamoff off, ::std::ios_base::seekdir way ) const noexcept
    {
        assert( m_fpStream );

        m_fpStream->clear();
        if ( checkWrite() ) {
            m_fpStream->seekp( off, way );   // output sequence
        } else {               
            m_fpStream->seekg( off, way );   // input sequence
        }
    }

    ::std::streampos ReadAccessor::tell() const noexcept
    {
        assert( m_fpStream );

        auto&& state = m_fpStream->rdstate();
        ::std::streampos ret;

        m_fpStream->clear();

        if ( checkWrite() ) {
            ret = m_fpStream->tellp();   // output sequence
        } else {
            ret = m_fpStream->tellg();   // input sequence
        }

        m_fpStream->setstate( state );

        return ret;
    }
} // pm
} // ara