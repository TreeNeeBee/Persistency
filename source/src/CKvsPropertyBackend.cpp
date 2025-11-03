#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/unordered_map.hpp>
#include <boost/container/scoped_allocator.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/functional/hash.hpp>

#include <cmath>
#include <sstream>
#include <iomanip>
#include <unistd.h>  // for getpid()
#include <cctype>    // for std::isalnum()

#include "CKvsBackend.hpp"
#include "CKvsPropertyBackend.hpp"

namespace lap
{
namespace pm
{
namespace util
{
    namespace shm
    {
        namespace bip = ::boost::interprocess;
        
        constexpr static auto DEF_SHM_MAX_SIZE = 1ul << 20; // 1M

        using SHM_Segment = bip::managed_shared_memory;
        using SHM_Manager = SHM_Segment::segment_manager;

        template < typename T > 
        using SHM_Alloc = boost::container::scoped_allocator_adaptor< bip::allocator< T, SHM_Manager > >;

    using SHM_String = bip::basic_string< core::Char, std::char_traits< core::Char >, SHM_Alloc< core::Char > >;

        template < typename K, typename V, typename Hash = std::hash<K>, typename Cmp = ::std::equal_to<K> >
        using SHM_Map = ::boost::unordered_map< K, V, Hash, Cmp, SHM_Alloc< std::pair< K const, V > > >;

        // Solution B: Simplified hash/equal - no prefix handling needed
        struct SHM_Hash : boost::hash_detail::hash_base< SHM_String >
        {
            core::Size operator()( SHM_String const& val ) const
            {
                // Direct hash - no need to skip prefix
                return ::boost::hash_range( val.begin(), val.end() );
            }
        };

        using SHM_MapValue = SHM_Map< SHM_String, SHM_String, SHM_Hash >;

        struct SHMContext
        {
            core::Size                          size{ DEF_SHM_MAX_SIZE };
            core::String                        shmName;  // Generated from strFile
            SHM_Segment                         segment;
            SHM_MapValue*                       mapValue{ nullptr };
        } context;
        
        // Generate shared memory name from file parameter
        inline core::String generateShmName(core::StringView strFile) {
            std::ostringstream oss;
            
            // 1. Add process ID to avoid cross-process conflicts
            oss << "shm_kvs_" << ::getpid() << "_";
            
            // 2. Keep part of original name for debugging
            core::String sanitized;
            for (size_t i = 0; i < std::min(strFile.size(), size_t(16)); ++i) {
                char c = strFile[i];
                if (std::isalnum(c) || c == '_') {
                    sanitized += c;
                } else {
                    sanitized += '_';
                }
            }
            oss << sanitized << "_";
            
            // 3. Add hash to handle long names
            oss << std::hex << std::hash<std::string>{}(std::string(strFile.data(), strFile.size()));
            
            return oss.str();
        }

        // New encoding function: Store type marker in value (Solution B)
        // Format: [type_byte][data]
        // This eliminates the need for type prefix in key names
        SHM_String encodeValue( const KvsDataType &value )
        {
            std::ostringstream oss;
            
            // 1. Write type marker (1 byte: 'a' + type_index)
            char typeMarker = static_cast<char>('a' + ::lap::core::GetVariantIndex( value ));
            oss << typeMarker;
            
            // 2. Write actual data with proper precision
            switch( static_cast<EKvsDataTypeIndicate>( ::lap::core::GetVariantIndex( value ) ) ) {
            case EKvsDataTypeIndicate::DataType_int8_t: // Int8
                oss << static_cast<int>(::lap::core::get< core::Int8 >( value ));
                break;
            case EKvsDataTypeIndicate::DataType_uint8_t: // UInt8
                oss << static_cast<unsigned>(::lap::core::get< core::UInt8 >( value ));
                break;
            case EKvsDataTypeIndicate::DataType_int16_t: // Int16
                oss << ::lap::core::get< core::Int16 >( value );
                break;
            case EKvsDataTypeIndicate::DataType_uint16_t: // UInt16
                oss << ::lap::core::get< core::UInt16 >( value );
                break;
            case EKvsDataTypeIndicate::DataType_int32_t: // Int32
                oss << ::lap::core::get< core::Int32 >( value );
                break;
            case EKvsDataTypeIndicate::DataType_uint32_t: // UInt32
                oss << ::lap::core::get< core::UInt32 >( value );
                break;
            case EKvsDataTypeIndicate::DataType_int64_t: // Int64
                oss << ::lap::core::get< core::Int64 >( value );
                break;
            case EKvsDataTypeIndicate::DataType_uint64_t: // UInt64
                oss << ::lap::core::get< core::UInt64 >( value );
                break;
            case EKvsDataTypeIndicate::DataType_bool: // Bool
                oss << (::lap::core::get< core::Bool >( value ) ? "true" : "false");
                break;
            case EKvsDataTypeIndicate::DataType_float: { // Float
                oss << std::scientific << std::setprecision(8) 
                    << ::lap::core::get< core::Float >( value );
                break;
            }
            case EKvsDataTypeIndicate::DataType_double: { // Double
                oss << std::scientific << std::setprecision(16) 
                    << ::lap::core::get< core::Double >( value );
                break;
            }
            case EKvsDataTypeIndicate::DataType_string: // String
                oss << ::lap::core::get< core::String >( value );
                break;
            }
            
            return SHM_String( oss.str().c_str(), shm::context.segment.get_segment_manager() );
        }

        // New decoding function: Extract type marker and parse data
        KvsDataType decodeValue( const SHM_String &encoded )
        {
            if (encoded.empty()) {
                throw std::runtime_error("Empty encoded value");
            }
            
            // 1. Extract type marker (first byte)
            char typeMarker = encoded[0];
            EKvsDataTypeIndicate type = static_cast<EKvsDataTypeIndicate>(typeMarker - 'a');
            
            // 2. Extract data string (skip first byte)
            std::string dataStr(encoded.c_str() + 1);
            
            // 3. Parse according to type
            switch( type ) {
            case EKvsDataTypeIndicate::DataType_int8_t: // Int8
                return static_cast< core::Int8 >( ::std::stoi( dataStr ) );
            case EKvsDataTypeIndicate::DataType_uint8_t: // UInt8
                return static_cast< core::UInt8 >( ::std::stoi( dataStr ) );
            case EKvsDataTypeIndicate::DataType_int16_t: // Int16
                return static_cast< core::Int16 >( ::std::stoi( dataStr ) );
            case EKvsDataTypeIndicate::DataType_uint16_t: // UInt16
                return static_cast< core::UInt16 >( ::std::stoi( dataStr ) );
            case EKvsDataTypeIndicate::DataType_int32_t: // Int32
                return static_cast< core::Int32 >( ::std::stoi( dataStr ) );
            case EKvsDataTypeIndicate::DataType_uint32_t: // UInt32
                return static_cast< core::UInt32 >( ::std::stoul( dataStr ) );
            case EKvsDataTypeIndicate::DataType_int64_t: // Int64
                return static_cast< core::Int64 >( ::std::stoll( dataStr ) );
            case EKvsDataTypeIndicate::DataType_uint64_t: // UInt64
                return static_cast< core::UInt64 >( ::std::stoull( dataStr ) );
            case EKvsDataTypeIndicate::DataType_bool: // Bool
                return ( dataStr == "true" );
            case EKvsDataTypeIndicate::DataType_float: // Float
                return ::std::stof( dataStr );
            case EKvsDataTypeIndicate::DataType_double: // Double
                return ::std::stod( dataStr );
            case EKvsDataTypeIndicate::DataType_string: // String
                return core::String( dataStr );
            }

            return false;
        }
    }

    // Remove all commented remote namespace code
    
    core::Result< core::Vector< core::String > > KvsPropertyBackend::GetAllKeys() const noexcept
    {
        using result = core::Result< core::Vector< core::String > >;

        core::Vector< core::String > value;
        try {
            // Solution B: No need to skip prefix, key names are original
            for ( auto&& it = shm::context.mapValue->begin(); it != shm::context.mapValue->end(); ++it ) {
                value.emplace_back( it->first.c_str() );  // Direct return, no +2 offset
            }
        } catch( const std::exception& e ) {
            LAP_PM_LOG_ERROR << "Exception in KvsPropertyBackend::GetAllKeys: " << core::StringView(e.what());
            return result::FromError( PerErrc::kNotInitialized );
        }

        return result::FromValue( value );
    }

    core::Result< core::Bool > KvsPropertyBackend::KeyExists ( core::StringView key ) const noexcept
    {
        using result = core::Result< core::Bool >;
        try {
            auto&& shmKey = shm::SHM_String( key.data(), shm::context.segment.get_segment_manager() );
            auto&& it = shm::context.mapValue->find( shmKey );

            if ( it != shm::context.mapValue->end() ) {
                return result::FromValue( true );
            }
        } catch(const std::exception& e) {
            LAP_PM_LOG_ERROR << "Exception in KvsPropertyBackend::KeyExists: " << core::StringView(e.what());
            return result::FromError( PerErrc::kNotInitialized );
        }

        return result::FromValue( false );
    }

    core::Result< KvsDataType > KvsPropertyBackend::GetValue( core::StringView key ) const noexcept
    {
        using result = core::Result< KvsDataType >;

        try {
            // Use original key name (no type prefix needed)
            auto&& it = shm::context.mapValue->find( shm::SHM_String( key.data(), shm::context.segment.get_segment_manager() ) );

            if ( it == shm::context.mapValue->end() ) {
                return core::Result<KvsDataType>::FromError( PerErrc::kKeyNotFound );
            }
            
            // Decode value (type is stored in value itself)
            return result::FromValue( shm::decodeValue( it->second ) );
        } catch(const std::exception& e) {
            LAP_PM_LOG_ERROR << "Exception in KvsPropertyBackend::GetValue: " << core::StringView(e.what());
            return result::FromError( PerErrc::kNotInitialized );
        }
    }

    core::Result<void> KvsPropertyBackend::SetValue( core::StringView key, const KvsDataType &value ) noexcept
    {
        using result = core::Result<void>;
        try {
            // Solution B: Use original key name directly
            // No need to remove old type variants - key names have no type prefix
            // Type is stored in the value itself, so setting a new value automatically overwrites
            
            auto&& shmKey = shm::SHM_String( key.data(), shm::context.segment.get_segment_manager() );
            auto&& shmValue = shm::encodeValue( value );  // Encode type into value
            shm::context.mapValue->operator[]( shmKey ) = shmValue;

#ifdef LAP_DEBUG
            LAP_PM_LOG_DEBUG.logFormat( "KvsPropertyBackend::SetValue with( %s , [type:%c] )", 
                                       key.data(), static_cast<char>('a' + ::lap::core::GetVariantIndex( value )) );
#endif
            
        } catch(const std::exception& e) {
            LAP_PM_LOG_ERROR << "Exception in KvsPropertyBackend::SetValue: " << core::StringView(e.what());
            return result::FromError( PerErrc::kNotInitialized );
        }
        return result::FromValue();
    }

    core::Result<void> KvsPropertyBackend::RemoveKey( core::StringView key ) noexcept
    {
        using result = core::Result<void>;

        try {
            auto it = shm::context.mapValue->find( shm::SHM_String( key.data(), shm::context.segment.get_segment_manager() ) );

            if ( it != shm::context.mapValue->end() ) {
                shm::context.mapValue->erase( it );
            }
           
        } catch(const std::exception& e) {
            LAP_PM_LOG_ERROR << "Exception in KvsPropertyBackend::DeleteKey: " << core::StringView(e.what());
            return result::FromError( PerErrc::kNotInitialized );
        }

        return result::FromValue();
    }

    core::Result<void> KvsPropertyBackend::RecoveryKey( core::StringView ) noexcept
    {
        LAP_PM_LOG_WARN << "RecoveryKey not supported";
        return core::Result<void>::FromError( PerErrc::kUnsupported );
    }

    core::Result<void> KvsPropertyBackend::ResetKey( core::StringView ) noexcept
    {
        LAP_PM_LOG_WARN << "ResetKey not supported";
        return core::Result<void>::FromError( PerErrc::kUnsupported );
    }

    core::Result<void> KvsPropertyBackend::RemoveAllKeys() noexcept
    {
        using result = core::Result<void>;
        try {
            shm::context.mapValue->clear();
        } catch(const std::exception& e) {
            return result::FromError( PerErrc::kNotInitialized );
        }

        return result::FromValue();
    }

    core::Result<void> KvsPropertyBackend::SyncToStorage() noexcept
    {
        // Shared memory is automatically synced, no action needed
        return core::Result<void>::FromValue();
    }

    core::Result<void> KvsPropertyBackend::DiscardPendingChanges() noexcept
    {
        using result = core::Result<void>;
        try {
            // reopen mapValue
            shm::context.mapValue = shm::context.segment.find_or_construct< shm::SHM_MapValue >( m_strFile.c_str() )( shm::context.segment.get_segment_manager() );

            if ( nullptr == shm::context.mapValue ) {
                LAP_PM_LOG_ERROR << "KvsPropertyBackend::DiscardPendingChanges shm find_or_construct failed!!";

                return result::FromError( PerErrc::kNotInitialized );
            }
        } catch(const std::exception& e) {
            return result::FromError( PerErrc::kNotInitialized );
        }

        return result::FromValue();
    }

    KvsPropertyBackend::KvsPropertyBackend( core::StringView strFile )
    {
        try {
            // Generate shared memory name from strFile parameter
            shm::context.shmName = shm::generateShmName(strFile);
            
            // Initialize shared memory with open_or_create
            shm::context.segment = shm::SHM_Segment( 
                shm::bip::open_or_create, 
                shm::context.shmName.c_str(), 
                shm::context.size 
            );
            
            if ( !shm::context.segment.check_sanity() ) {
                LAP_PM_LOG_ERROR << "KvsPropertyBackend: shared memory sanity check failed";
                throw PerException( PerErrc::kInitValueNotAvailable );
            }
            
            // Open or create the map in shared memory
            shm::context.mapValue = shm::context.segment.find_or_construct< shm::SHM_MapValue >( 
                strFile.data() 
            )( shm::context.segment.get_segment_manager() );

            if ( nullptr == shm::context.mapValue ) {
                LAP_PM_LOG_ERROR << "KvsPropertyBackend: failed to find/create shared memory map";
                throw PerException( PerErrc::kInitValueNotAvailable );
            }
            
            LAP_PM_LOG_INFO << "KvsPropertyBackend initialized with SHM name: " << core::StringView(shm::context.shmName);
        } catch ( std::exception &e ) {
            LAP_PM_LOG_ERROR << "KvsPropertyBackend initialization failed: " << e.what();
            throw PerException( PerErrc::kInitValueNotAvailable );
        }

        m_bAvailable = true;
    }

    KvsPropertyBackend::~KvsPropertyBackend() noexcept
    {
        ;
    }
} // util
} // pm
} // lap