#include "CKeyValueStorage.hpp"
#include "IKvsBackend.hpp"
#include "CKvsFileBackend.hpp"
#include "CKvsSqliteBackend.hpp"
#include "CKvsPropertyBackend.hpp"
#include "CPersistencyManager.hpp"

namespace lap
{
namespace per
{
    KeyValueStorage::KeyValueStorage ( core::StringView strIdentifier ) noexcept
        : m_strPath( strIdentifier )
        , m_pKvsBackend( ::std::make_unique< KvsFileBackend >( strIdentifier ) )       // default
    {
        ;
    }

    KeyValueStorage::KeyValueStorage ( core::StringView strIdentifier, const KvsBackendType &type ) noexcept
        : m_strPath( strIdentifier )
    {
        try {
            if ( type & KvsBackendType::kvsFile ) {
                m_pKvsBackend = ::std::make_unique< KvsFileBackend >( strIdentifier );
            } else if ( type & KvsBackendType::kvsSqlite ) {
                // SQLite backend now fully implements IKvsBackend interface
                m_pKvsBackend = ::std::make_unique< KvsSqliteBackend >( strIdentifier );
            } else if ( type & KvsBackendType::kvsProperty ) {
                // Property backend with configurable persistence (File or SQLite)
                // Default to File backend for persistence
                m_pKvsBackend = ::std::make_unique< ::lap::per::util::KvsPropertyBackend >( 
                    strIdentifier, 
                    KvsBackendType::kvsFile  // Use File backend as persistence layer
                );
            } else {
                LAP_PER_LOG_ERROR << "Kvs backend type is not recognized, default to FileBackend";
                m_pKvsBackend = ::std::make_unique< KvsFileBackend >( strIdentifier );
            }
        } catch( const PerException& e ) {
            LAP_PER_LOG_ERROR << "Kvs backend create failed " << e.what();
        }
    }

    KeyValueStorage::KeyValueStorage ( core::StringView strIdentifier, const KvsBackendType &type, const PersistencyConfig* config ) noexcept
        : m_strPath( strIdentifier )
    {
        try {
            if ( type & KvsBackendType::kvsFile ) {
                m_pKvsBackend = ::std::make_unique< KvsFileBackend >( strIdentifier );
            } else if ( type & KvsBackendType::kvsSqlite ) {
                m_pKvsBackend = ::std::make_unique< KvsSqliteBackend >( strIdentifier );
            } else if ( type & KvsBackendType::kvsProperty ) {
                // Property backend with config support
                KvsBackendType persistenceBackend = KvsBackendType::kvsFile;
                core::Size shmSize = 1ul << 20;  // Default 1MB
                
                if (config != nullptr) {
                    // Use configured persistence backend
                    if (config->kvs.propertyBackendPersistence == "sqlite") {
                        persistenceBackend = KvsBackendType::kvsSqlite;
                    }
                    // Shared memory size will be read from config in constructor
                    shmSize = config->kvs.propertyBackendShmSize;
                }
                
                m_pKvsBackend = ::std::make_unique< ::lap::per::util::KvsPropertyBackend >( 
                    strIdentifier, 
                    persistenceBackend,
                    shmSize,
                    config  // Pass config for additional settings
                );
            } else {
                LAP_PER_LOG_ERROR << "Kvs backend type is not recognized, default to FileBackend";
                m_pKvsBackend = ::std::make_unique< KvsFileBackend >( strIdentifier );
            }
        } catch( const PerException& e ) {
            LAP_PER_LOG_ERROR << "Kvs backend create failed " << e.what();
        }
    }

    KeyValueStorage::~KeyValueStorage() noexcept
    {
        if ( m_pKvsBackend ) {
            m_pKvsBackend = nullptr;
        }
    }

    KeyValueStorage::KeyValueStorage( KeyValueStorage&& kvs ) noexcept
        : m_strPath( kvs.m_strPath )
        , m_pKvsBackend( ::std::move( kvs.m_pKvsBackend ) )
    {
        ;
    }

    KeyValueStorage& KeyValueStorage::operator=( KeyValueStorage&& kvs ) noexcept
    {
        m_strPath = kvs.m_strPath;

        m_pKvsBackend = ::std::move( kvs.m_pKvsBackend );

        return *this;
    }

    core::Result< core::Bool > KeyValueStorage::initialize( core::StringView strConfig, core::Bool bCreate ) noexcept
    {
        using result = core::Result< core::Bool >;

        UNUSED( strConfig );
        UNUSED( bCreate );

        m_bInitialized = true;

        return result::FromValue( m_bInitialized );
    }

    void KeyValueStorage::uninitialize() noexcept
    {
        ;
    }

    core::Result< core::Vector< core::String > > KeyValueStorage::GetAllKeys() const noexcept
    {
        using result = core::Result< core::Vector< core::String > >;

        if ( !m_pKvsBackend || !m_pKvsBackend->available() ) return result::FromError( PerErrc::kNotInitialized );

        return m_pKvsBackend->GetAllKeys();
    }

    core::Result< core::Bool > KeyValueStorage::KeyExists ( core::StringView key) const noexcept
    {
        if ( !m_pKvsBackend || !m_pKvsBackend->available() ) return core::Result< core::Bool >::FromError( PerErrc::kNotInitialized );

        return m_pKvsBackend->KeyExists( key );
    }

    template<class T>
    core::Result<T> KeyValueStorage::GetValue( core::StringView key ) const noexcept
    {
        using result = core::Result<T>;

        if ( !m_pKvsBackend || !m_pKvsBackend->available() ) return result::FromError( PerErrc::kNotInitialized );

        auto retValue = m_pKvsBackend->GetValue( key );

        if ( retValue.HasValue() ) {
            // Attempt to extract the requested type from the variant in a cross-compat manner
            try {
                return result::FromValue( ::lap::core::get< T >( retValue.Value() ) );
            } catch ( const std::exception& ) {
                return result::FromError( PerErrc::kDataTypeMismatch );
            }
        } else {
            return result::FromError( retValue.Error() );
        }
    }

    template core::Result< core::Int8 > KeyValueStorage::GetValue( core::StringView key ) const noexcept;
    template core::Result< core::UInt8 > KeyValueStorage::GetValue( core::StringView key ) const noexcept;
    template core::Result< core::Int16 > KeyValueStorage::GetValue( core::StringView key ) const noexcept;
    template core::Result< core::UInt16 > KeyValueStorage::GetValue( core::StringView key ) const noexcept;
    template core::Result< core::Int32 > KeyValueStorage::GetValue( core::StringView key ) const noexcept;
    template core::Result< core::UInt32 > KeyValueStorage::GetValue( core::StringView key ) const noexcept;
    template core::Result< core::Int64 > KeyValueStorage::GetValue( core::StringView key ) const noexcept;
    template core::Result< core::UInt64 > KeyValueStorage::GetValue( core::StringView key ) const noexcept;
    template core::Result< core::Bool > KeyValueStorage::GetValue( core::StringView key ) const noexcept;
    template core::Result< core::Float > KeyValueStorage::GetValue( core::StringView key ) const noexcept;
    template core::Result< core::Double > KeyValueStorage::GetValue( core::StringView key ) const noexcept;
    template core::Result< core::String > KeyValueStorage::GetValue( core::StringView key ) const noexcept;

    template<class T>
    core::Result<void> KeyValueStorage::SetValue( core::StringView key, const T &value ) noexcept
    {
        using result = core::Result<void>;
        if ( !m_pKvsBackend || !m_pKvsBackend->available() ) return result::FromError( PerErrc::kNotInitialized );

        return m_pKvsBackend->SetValue( key, KvsDataType{ value } );
    }

    template core::Result<void> KeyValueStorage::SetValue( core::StringView key, const core::Int8& ) noexcept;
    template core::Result<void> KeyValueStorage::SetValue( core::StringView key, const core::UInt8& ) noexcept;
    template core::Result<void> KeyValueStorage::SetValue( core::StringView key, const core::Int16&  ) noexcept;
    template core::Result<void> KeyValueStorage::SetValue( core::StringView key, const core::UInt16& ) noexcept;
    template core::Result<void> KeyValueStorage::SetValue( core::StringView key, const core::Int32& ) noexcept;
    template core::Result<void> KeyValueStorage::SetValue( core::StringView key, const core::UInt32&  ) noexcept;
    template core::Result<void> KeyValueStorage::SetValue( core::StringView key, const core::Int64& ) noexcept;
    template core::Result<void> KeyValueStorage::SetValue( core::StringView key, const core::UInt64& ) noexcept;
    template core::Result<void> KeyValueStorage::SetValue( core::StringView key, const core::Bool&  ) noexcept;
    template core::Result<void> KeyValueStorage::SetValue( core::StringView key, const core::Float& ) noexcept;
    template core::Result<void> KeyValueStorage::SetValue( core::StringView key, const core::Double& ) noexcept;
    template core::Result<void> KeyValueStorage::SetValue( core::StringView key, const core::String&  ) noexcept;

    core::Result<void> KeyValueStorage::RemoveKey( core::StringView key ) noexcept
    {
        if ( !m_pKvsBackend || !m_pKvsBackend->available() ) return core::Result<void>::FromError( PerErrc::kNotInitialized );

        return m_pKvsBackend->RemoveKey( key );
    }

    core::Result<void> KeyValueStorage::RecoverKey( core::StringView key ) noexcept
    {
        if ( !m_pKvsBackend || !m_pKvsBackend->available() ) return core::Result<void>::FromError( PerErrc::kNotInitialized );

        return m_pKvsBackend->RecoverKey( key );
    }

    core::Result<void> KeyValueStorage::ResetKey( core::StringView key ) noexcept
{
        if ( !m_pKvsBackend || !m_pKvsBackend->available() ) return core::Result<void>::FromError( PerErrc::kNotInitialized );

        return m_pKvsBackend->ResetKey( key );
    }

    core::Result<void> KeyValueStorage::RemoveAllKeys() noexcept
    {
        if ( !m_pKvsBackend || !m_pKvsBackend->available() ) return core::Result<void>::FromError( PerErrc::kNotInitialized );

        return m_pKvsBackend->RemoveAllKeys();
    }

    core::Result<void> KeyValueStorage::SyncToStorage() const noexcept
    {
        if ( !m_pKvsBackend || !m_pKvsBackend->available() ) return core::Result<void>::FromError( PerErrc::kNotInitialized );

        return m_pKvsBackend->SyncToStorage();
    }

    core::Result<void> KeyValueStorage::DiscardPendingChanges() noexcept
    {
        if ( !m_pKvsBackend || !m_pKvsBackend->available() ) return core::Result<void>::FromError( PerErrc::kNotInitialized );

        return m_pKvsBackend->DiscardPendingChanges();
    }

    core::Result< void > KeyValueStorage::RecoverKeyValueStorage() noexcept
    {
        using result = core::Result< void >;

        return result::FromValue();
    }

    core::Result< void > KeyValueStorage::ResetKeyValueStorage() noexcept
    {
        using result = core::Result< void >;

        return result::FromValue();
    }

    core::Result< core::UInt64 > KeyValueStorage::GetCurrentKeyValueStorageSize() noexcept
    {
        using result = core::Result< core::UInt64 >;

        return result::FromValue( static_cast<core::UInt64>(0) );
    }

    core::Result< core::SharedHandle< KeyValueStorage > > OpenKeyValueStorage( const core::InstanceSpecifier &kvs, core::Bool bCreate, KvsBackendType type ) noexcept
    {
        return CPersistencyManager::getInstance().getKvsStorage( kvs, bCreate, type );
    }

    core::Result< core::SharedHandle< KeyValueStorage > > OpenKeyValueStorage( const core::InstanceSpecifier &kvs ) noexcept
    {
        return CPersistencyManager::getInstance().getKvsStorage( kvs );
    }

    core::Result< void > RecoverKeyValueStorage( const core::InstanceSpecifier &kvs ) noexcept
    {
        return CPersistencyManager::getInstance().RecoverKeyValueStorage( kvs );
    }

    core::Result< void > ResetKeyValueStorage( const core::InstanceSpecifier &kvs ) noexcept
    {
        return CPersistencyManager::getInstance().ResetKeyValueStorage( kvs );
    }

    core::Result< core::UInt64 > GetCurrentKeyValueStorageSize( const core::InstanceSpecifier &kvs ) noexcept
    {
        return CPersistencyManager::getInstance().GetCurrentKeyValueStorageSize( kvs );
    }
} // namespace per
} // namespace lap
