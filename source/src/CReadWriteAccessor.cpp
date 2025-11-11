#include "CReadWriteAccessor.hpp"

namespace lap
{
namespace per
{
    ReadWriteAccessor::ReadWriteAccessor ( core::StringView strFile, OpenMode mode, FileStorage* parent )
        : ReadAccessor( strFile, mode, parent )
    { 
        if ( !isGood() ) { throw PerException( PerErrc::kNotInitialized ); }
    }

    ReadWriteAccessor::~ReadWriteAccessor() noexcept
    {
        ;
    }

    core::Result<void> ReadWriteAccessor::SyncToFile () noexcept
    {
        using result = core::Result<void>;

        if ( !checkWrite() ) return result::FromError( PerErrc::kInvalidOpenMode );

        try {
            stream()->flush();
            stream()->sync();
        } catch(const std::exception& e) {
            return result::FromError( PerErrc::kPhysicalStorageFailure );
        }

        return result::FromValue();
    }

    core::Result<void> ReadWriteAccessor::SetFileSize ( core::UInt64 size ) noexcept
    {
        using result = core::Result<void>;

        if ( !CPersistencyManager::getInstance().isInitialized() ) return result::FromError( PerErrc::kNotInitialized );

        if ( !isGood() )    return result::FromError( PerErrc::kPhysicalStorageFailure );
        if ( size > GetSize() ) return result::FromError( PerErrc::kInvalidSize );

        if ( 0 == truncate64( file().data(), size ) ) {
            SetPosition( size - 1 );
            updateModifyTime();
            updateFileSize( size );
            return result::FromValue();
        } else {
            return result::FromError( PerErrc::kPhysicalStorageFailure );
        }
    }

    core::Result<void> ReadWriteAccessor::WriteText ( core::StringView s ) noexcept
    {
        using result = core::Result<void>;

        if ( !CPersistencyManager::getInstance().isInitialized() ) return result::FromError( PerErrc::kNotInitialized );

        if ( !isGood() )    return result::FromError( PerErrc::kPhysicalStorageFailure );
        
        if ( !checkWrite() || checkBinary() ) return result::FromError( PerErrc::kInvalidOpenMode );

        try {
            stream()->write( s.data(), s.size() );
        } catch(const std::exception& e) {
            return result::FromError( PerErrc::kResourceBusy );
        }

        updateModifyTime();
        appendFileSize( s.size() );
        return result::FromValue();
    }

    core::Result<void> ReadWriteAccessor::WriteBinary ( core::Span< const core::Byte > bytes ) noexcept
    {
        using result = core::Result<void>;

        if ( !CPersistencyManager::getInstance().isInitialized() ) return result::FromError( PerErrc::kNotInitialized );

        if ( !isGood() )    return result::FromError( PerErrc::kPhysicalStorageFailure );
        
        if ( !checkWrite() || !checkBinary() ) return result::FromError( PerErrc::kInvalidOpenMode );

        try {
            stream()->write( reinterpret_cast< const char* >( bytes.data() ), bytes.size() );
        } catch(const std::exception& e) {
            return result::FromError( PerErrc::kResourceBusy );
        }

        updateModifyTime();
        appendFileSize( bytes.size() );
        return result::FromValue();
    }

    //This operator is just a comfort feature for non-safety critical applications. If an error occurs during this operation, it is silently ignored.
    ReadWriteAccessor& ReadWriteAccessor::operator<< ( core::StringView s ) noexcept
    {
        if ( !checkWrite() || checkBinary() )  {
            LAP_PER_LOG_WARN << "ReadWriteAccessorBase::operator<< kInvalidOpenMode " << s.data();

            return *this;
        }

        updateModifyTime();
        appendFileSize( s.size() );
        stream()->write( s.data(), s.size() );

        return *this;
    }
} // pm
} // ara