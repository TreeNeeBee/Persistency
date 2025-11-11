#include <fstream>
#include <boost/property_tree/json_parser.hpp>
#include "CKvsFileBackend.hpp"

namespace lap
{
namespace per
{
    core::Result< core::Vector< core::String > > KvsFileBackend::GetAllKeys() const noexcept
    {
        using result = core::Result< core::Vector< core::String > >;

        if ( !m_bAvailable ) return result::FromError( PerErrc::kNotInitialized );

        core::Vector< core::String > keys;

        // Solution B: Direct return, no prefix skipping needed
        for ( auto&& it = m_kvsRoot.begin(); it != m_kvsRoot.end(); ++it ) {
            keys.emplace_back( it->first.c_str() );
        }

        return result::FromValue( keys );
    }

    core::Result< core::Bool > KvsFileBackend::KeyExists ( core::StringView key ) const noexcept
    {
        using result = core::Result< core::Bool >;

        if ( !m_bAvailable ) return result::FromError( PerErrc::kNotInitialized );

        return result::FromValue( m_kvsRoot.find( key.data() ) != m_kvsRoot.not_found() );
    }

    core::Result< KvsDataType > KvsFileBackend::GetValue( core::StringView key ) const noexcept
    {
        using result = core::Result< KvsDataType >;

        if ( !m_bAvailable ) return result::FromError( PerErrc::kNotInitialized );

        KvsDataType value;
        try {
            value = m_kvsRoot.get<KvsDataType>( key.data() );
        } catch ( ::boost::property_tree::ptree_error & e ) {
            LAP_PER_LOG_WARN.logFormat( "KvsFileBackend::GetValue with key[%s] failed: %s!", key.data(), e.what() );

            return result::FromError( PerErrc::kKeyNotFound );
        }

        return result::FromValue( value );
    }

    core::Result<void> KvsFileBackend::SetValue( core::StringView key, const KvsDataType &value ) noexcept
    {
        using result = core::Result<void>;

        if ( !m_bAvailable ) return result::FromError( PerErrc::kNotInitialized );

        try {
            // Solution B: Use original key directly, no type prefix needed
            // PropertyTree handles Variant serialization automatically
            m_kvsRoot.put( key.data(), value );
        } catch ( ::boost::property_tree::ptree_error & e ) {
            LAP_PER_LOG_WARN.logFormat( "KvsFileBackend::SetValue with ( %s, %s ) failed: %s!", key.data(), kvsToStrig( value ).c_str(), e.what() );

            return result::FromError( PerErrc::kIllegalWriteAccess );
        }

        return result::FromValue();
    }

    core::Result<void> KvsFileBackend::RemoveKey( core::StringView key ) noexcept
    {
        using result = core::Result<void>;

        if ( !m_bAvailable ) return result::FromError( PerErrc::kNotInitialized );

        m_kvsRoot.erase( key.data() );

        return result::FromValue();
    }

    core::Result<void> KvsFileBackend::RecoveryKey( core::StringView ) noexcept
    {
        LAP_PER_LOG_WARN << "Not support yet";
        // TODO : KvsFileBackend::RecoveryKey

        using result = core::Result<void>;
        return result::FromValue();
    }

    core::Result<void> KvsFileBackend::ResetKey( core::StringView ) noexcept
    {
        LAP_PER_LOG_WARN << "Not support yet";
        // TODO : KvsFileBackend::ResetKey

        using result = core::Result<void>;
        return result::FromValue();
    }

    core::Result<void> KvsFileBackend::RemoveAllKeys() noexcept
    {
        using result = core::Result<void>;

        if ( !m_bAvailable ) return result::FromError( PerErrc::kNotInitialized );

        m_kvsRoot.clear();

        return result::FromValue();
    }

    core::Result<void> KvsFileBackend::SyncToStorage() noexcept
    {
        return saveToFile( m_strFile );
    }

    core::Result<void> KvsFileBackend::DiscardPendingChanges() noexcept
    {
        using result = core::Result<void>;
        
        if ( !m_bAvailable ) return result::FromValue();

        return parseFromFile( m_strFile );
    }

    core::Result<void> KvsFileBackend::parseFromFile( core::StringView strFile ) noexcept
    {
        using result = core::Result<void>;

        ::boost::property_tree::ptree root;
        try {
            ::std::ifstream ifs( strFile.data(), ::std::ifstream::in );

            if ( ifs.is_open() ) {
                ::boost::property_tree::read_json( ifs, root );

                ifs.close();
            } else {
                LAP_PER_LOG_WARN << "KvsFileBackend::parseFromFile open failed";

                return result::FromError( PerErrc::kFileNotFound );
            }
        } catch ( const std::ios_base::failure& e ) {
            LAP_PER_LOG_WARN.logFormat( "KvsFileBackend::parseFromFile %s failed with exception: %s!!!", strFile.data(), e.what() );

            return result::FromError( PerErrc::kFileNotFound );
        } catch ( ::boost::property_tree::ptree_error& e) {
            LAP_PER_LOG_WARN.logFormat( "KvsFileBackend::parseFromFile read_json %s failed with exception: %s!!!", strFile.data(), e.what() );

            return result::FromError( PerErrc::kFileNotFound );
        }

        m_kvsRoot.clear();

        // Solution B: Copy from parsed JSON, values are read as strings
        // We need to convert back to KvsDataType (PropertyTree limitation)
        for ( auto&& it = root.begin(); it != root.end(); ++it ) {
            // For now, treat all values as strings (PropertyTree returns string)
            // This is a limitation of PropertyTree - it doesn't preserve type info in JSON
            m_kvsRoot.put( it->first, KvsDataType{ core::String( it->second.data() ) } );
        }
        
        return result::FromValue();
    }

    core::Result<void> KvsFileBackend::saveToFile( core::StringView strFile ) noexcept 
    {
        using result = core::Result<void>;

        try {
            ::std::ofstream outputFile( strFile.data() );

            if ( outputFile.is_open() ) {
                core::Char cIndent = '\t';
                core::Bool endChar = false;

                outputFile << "{\n";
                for ( auto&& it = m_kvsRoot.begin(); it != m_kvsRoot.end(); ++it ) {
                    if ( endChar ) { outputFile << ",\n"; }
                    outputFile << cIndent << "\"" << it->first << "\": ";
                    outputFile << kvsToStrig( it->second.data() );
                    endChar = true;
                }
                outputFile << "\n}";
                outputFile.close();

                return result::FromValue();
            } else {
                LAP_PER_LOG_WARN << "KvsFileBackend::saveToFile failed!!! " << strFile.data();
                return result::FromError( PerErrc::kFileNotFound );
            }
        } catch ( const std::ios_base::failure& e ) {
            LAP_PER_LOG_WARN.logFormat( "KvsFileBackend::saveToFile %s failed with exception: %s!!!", strFile.data(), e.what() );

            return result::FromError( PerErrc::kFileNotFound );
        }

        return result::FromValue();
    }

    KvsFileBackend::~KvsFileBackend() noexcept
    {
        if ( m_bAvailable ) {
            m_kvsRoot.clear();
            m_bAvailable = false;
        }
    }

    KvsFileBackend::KvsFileBackend( core::StringView strFile ) noexcept
        : m_strFile( strFile )
    {
        parseFromFile( m_strFile );

        m_bAvailable = true;
    }
} // pm
} // ara