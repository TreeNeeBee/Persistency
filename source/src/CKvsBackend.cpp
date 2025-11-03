#include "CKvsBackend.hpp"

namespace lap
{
namespace pm
{
    core::Result< core::Vector< core::String > > KvsBackend::GetAllKeys() const noexcept
    {
        using result = core::Result< core::Vector< core::String > >;

        return result::FromError( PerErrc::kNotInitialized );
    }

    core::Result< core::Bool > KvsBackend::KeyExists ( core::StringView ) const noexcept
    {
        //UNUSED( key );

        using result = core::Result< core::Bool >;

        return result::FromError( PerErrc::kNotInitialized );
    }

    core::Result< KvsDataType > KvsBackend::GetValue( core::StringView ) const noexcept
    {
        //UNUSED( key );

        using result = core::Result< KvsDataType >;

        return result::FromError( PerErrc::kNotInitialized );
    }

    core::Result<void> KvsBackend::SetValue( core::StringView, const KvsDataType & ) noexcept
    {
        //UNUSED( key );
        //UNUSED( value );
        using result = core::Result< void >;

        return result::FromError( PerErrc::kNotInitialized );
    }

    core::Result<void> KvsBackend::RemoveKey( core::StringView ) noexcept
    {
        //UNUSED( key );

        using result = core::Result< void >;

        return result::FromError( PerErrc::kNotInitialized );
    }

    core::Result<void> KvsBackend::RecoveryKey( core::StringView ) noexcept
    {
        //UNUSED( key );

        using result = core::Result< void >;

        return result::FromError( PerErrc::kNotInitialized );
    }

    core::Result<void> KvsBackend::ResetKey( core::StringView ) noexcept
    {
        //UNUSED( key );
        
        using result = core::Result< void >;

        return result::FromError( PerErrc::kNotInitialized );
    }

    core::Result<void> KvsBackend::RemoveAllKeys() noexcept
    {
        using result = core::Result< void >;

        return result::FromError( PerErrc::kNotInitialized );
    }

    core::Result<void> KvsBackend::SyncToStorage() noexcept
    {
        using result = core::Result< void >;

        return result::FromError( PerErrc::kNotInitialized );
    }

    core::Result<void> KvsBackend::DiscardPendingChanges() noexcept
    {
        using result = core::Result< void >;

        return result::FromError( PerErrc::kNotInitialized );
    }

    void KvsBackend::formatKey( core::String &key, EKvsDataTypeIndicate valueType )
    {
        if ( key.size() >= 2 && key[ DEF_KVS_MAGIC_KEY_INDEX ] == DEF_KVS_MAGIC_KEY ) {
#ifdef LAP_DEBUG
            LAP_PM_LOG_INFO << "Key is already format";
#endif
            return;
        }

        key.insert( 0, 2, static_cast< core::Char >( 97 /*'a'*/ + static_cast< core::UInt32 >( valueType ) ) );    
        key[ DEF_KVS_MAGIC_KEY_INDEX ] = DEF_KVS_MAGIC_KEY;
    }

    EKvsDataTypeIndicate KvsBackend::getDataType( core::StringView strKey )
    {
        if ( strKey.size() < 2 || strKey[ DEF_KVS_MAGIC_KEY_INDEX ] != DEF_KVS_MAGIC_KEY ) {
            return EKvsDataTypeIndicate::DataType_string;
        }

        core::Char cType = strKey[DEF_KVS_MAGIC_TYPE_INDEX];

        if ( cType < 'a' )  return EKvsDataTypeIndicate::DataType_string;

        return static_cast<EKvsDataTypeIndicate>( cType - 'a' );
    }
} // namespace pm
} // namespace log