#include "CKvsSqliteBackend.hpp"
#include <sstream>
#include <iomanip>
#include <limits>

namespace lap
{
namespace per
{
    // ==================== Constructor/Destructor ====================
    
    KvsSqliteBackend::KvsSqliteBackend( core::StringView file )
        : m_strFile( file )
    {
        {
            auto result = initializeDatabase();
            if( !result.HasValue() )
            {
                LAP_PER_LOG_ERROR << "Failed to initialize database: " << core::StringView(m_strFile);
                return;
            }
        }
        
        {
            auto result = prepareStatements();
            if( !result.HasValue() )
            {
                LAP_PER_LOG_ERROR << "Failed to prepare statements";
                return;
            }
        }
        
        m_bAvailable = true;
        LAP_PER_LOG_INFO << "SQLite backend initialized successfully: " << core::StringView(m_strFile);
    }

    KvsSqliteBackend::KvsSqliteBackend( KvsSqliteBackend&& kvs )
        : m_bAvailable( kvs.m_bAvailable )
        , m_strFile( ::std::move( kvs.m_strFile ) )
        , m_pDB( kvs.m_pDB )
        , m_pStmtInsert( kvs.m_pStmtInsert )
        , m_pStmtUpdate( kvs.m_pStmtUpdate )
        , m_pStmtSelect( kvs.m_pStmtSelect )
        , m_pStmtExists( kvs.m_pStmtExists )
        , m_pStmtDelete( kvs.m_pStmtDelete )
        , m_pStmtGetAll( kvs.m_pStmtGetAll )
        , m_bInTransaction( kvs.m_bInTransaction )
    {
        kvs.m_pDB = nullptr;
        kvs.m_pStmtInsert = nullptr;
        kvs.m_pStmtUpdate = nullptr;
        kvs.m_pStmtSelect = nullptr;
        kvs.m_pStmtExists = nullptr;
        kvs.m_pStmtDelete = nullptr;
        kvs.m_pStmtGetAll = nullptr;
        kvs.m_bAvailable = false;
        kvs.m_bInTransaction = false;
    }

    KvsSqliteBackend::~KvsSqliteBackend()
    {
        if( m_bInTransaction )
        {
            rollbackTransaction();
        }
        
        finalizeStatements();
        
        if( m_pDB )
        {
            sqlite3_close( m_pDB );
            m_pDB = nullptr;
            LAP_PER_LOG_DEBUG << "SQLite database closed: " << core::StringView(m_strFile);
        }
    }

    // ==================== Database Initialization ====================
    
    core::Result< void > KvsSqliteBackend::initializeDatabase() noexcept
    {
        core::LockGuard lock( m_mutex );
        
        // Open database with proper flags
        core::Int32 flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
        core::Int32 rc = sqlite3_open_v2( m_strFile.c_str(), &m_pDB, flags, nullptr );
        
        if( rc != SQLITE_OK )
        {
            LAP_PER_LOG_ERROR << "Failed to open SQLite database: " << sqlite3_errmsg( m_pDB );
            if( m_pDB )
            {
                sqlite3_close( m_pDB );
                m_pDB = nullptr;
            }
            return core::Result< void >::FromError( makeErrorCode( rc ) );
        }
        
        // Enable WAL mode for better concurrency and performance
        char* errMsg = nullptr;
        rc = sqlite3_exec( m_pDB, "PRAGMA journal_mode=WAL;", nullptr, nullptr, &errMsg );
        if( rc != SQLITE_OK )
        {
            LAP_PER_LOG_WARN << "Failed to enable WAL mode: " << ( errMsg ? errMsg : "unknown error" );
            if( errMsg ) sqlite3_free( errMsg );
        }
        
        // Set synchronous to NORMAL for better performance (still safe with WAL)
        rc = sqlite3_exec( m_pDB, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, &errMsg );
        if( rc != SQLITE_OK )
        {
            LAP_PER_LOG_WARN << "Failed to set synchronous mode: " << ( errMsg ? errMsg : "unknown error" );
            if( errMsg ) sqlite3_free( errMsg );
        }
        
        // Increase cache size (default is ~2MB, we use 10MB)
        rc = sqlite3_exec( m_pDB, "PRAGMA cache_size=-10000;", nullptr, nullptr, &errMsg );
        if( rc != SQLITE_OK )
        {
            LAP_PER_LOG_WARN << "Failed to set cache size: " << ( errMsg ? errMsg : "unknown error" );
            if( errMsg ) sqlite3_free( errMsg );
        }
        
        // Enable memory-mapped I/O (64MB)
        rc = sqlite3_exec( m_pDB, "PRAGMA mmap_size=67108864;", nullptr, nullptr, &errMsg );
        if( rc != SQLITE_OK )
        {
            LAP_PER_LOG_WARN << "Failed to set mmap size: " << ( errMsg ? errMsg : "unknown error" );
            if( errMsg ) sqlite3_free( errMsg );
        }
        
        // Create table with optimized schema
        const char* createTableSQL = 
            "CREATE TABLE IF NOT EXISTS kvs_data ("
            "    key TEXT PRIMARY KEY NOT NULL,"
            "    value TEXT NOT NULL,"
            "    deleted INTEGER DEFAULT 0"
            ") WITHOUT ROWID;";  // WITHOUT ROWID for better performance with TEXT primary key
        
        rc = sqlite3_exec( m_pDB, createTableSQL, nullptr, nullptr, &errMsg );
        if( rc != SQLITE_OK )
        {
            LAP_PER_LOG_ERROR << "Failed to create table: " << ( errMsg ? errMsg : "unknown error" );
            if( errMsg ) sqlite3_free( errMsg );
            return core::Result< void >::FromError( makeErrorCode( rc ) );
        }
        
        // Create index on deleted column for faster queries
        const char* createIndexSQL = "CREATE INDEX IF NOT EXISTS idx_deleted ON kvs_data(deleted);";
        rc = sqlite3_exec( m_pDB, createIndexSQL, nullptr, nullptr, &errMsg );
        if( rc != SQLITE_OK )
        {
            LAP_PER_LOG_WARN << "Failed to create index: " << ( errMsg ? errMsg : "unknown error" );
            if( errMsg ) sqlite3_free( errMsg );
        }
        
        return core::Result< void >::FromValue();
    }

    // ==================== Prepared Statements ====================
    
    core::Result< void > KvsSqliteBackend::prepareStatements() noexcept
    {
        core::LockGuard lock( m_mutex );
        
        core::Int32 rc;
        
        // INSERT OR REPLACE statement
        const char* insertSQL = "INSERT OR REPLACE INTO kvs_data (key, value, deleted) VALUES (?, ?, 0);";
        rc = sqlite3_prepare_v2( m_pDB, insertSQL, -1, &m_pStmtInsert, nullptr );
        if( rc != SQLITE_OK )
        {
            LAP_PER_LOG_ERROR << "Failed to prepare insert statement: " << sqlite3_errmsg( m_pDB );
            return core::Result< void >::FromError( makeErrorCode( rc ) );
        }
        
        // UPDATE statement
        const char* updateSQL = "UPDATE kvs_data SET value = ?, deleted = 0 WHERE key = ?;";
        rc = sqlite3_prepare_v2( m_pDB, updateSQL, -1, &m_pStmtUpdate, nullptr );
        if( rc != SQLITE_OK )
        {
            LAP_PER_LOG_ERROR << "Failed to prepare update statement: " << sqlite3_errmsg( m_pDB );
            return core::Result< void >::FromError( makeErrorCode( rc ) );
        }
        
        // SELECT statement
        const char* selectSQL = "SELECT value FROM kvs_data WHERE key = ? AND deleted = 0;";
        rc = sqlite3_prepare_v2( m_pDB, selectSQL, -1, &m_pStmtSelect, nullptr );
        if( rc != SQLITE_OK )
        {
            LAP_PER_LOG_ERROR << "Failed to prepare select statement: " << sqlite3_errmsg( m_pDB );
            return core::Result< void >::FromError( makeErrorCode( rc ) );
        }
        
        // EXISTS statement
        const char* existsSQL = "SELECT 1 FROM kvs_data WHERE key = ? AND deleted = 0 LIMIT 1;";
        rc = sqlite3_prepare_v2( m_pDB, existsSQL, -1, &m_pStmtExists, nullptr );
        if( rc != SQLITE_OK )
        {
            LAP_PER_LOG_ERROR << "Failed to prepare exists statement: " << sqlite3_errmsg( m_pDB );
            return core::Result< void >::FromError( makeErrorCode( rc ) );
        }
        
        // DELETE statement (soft delete by setting deleted flag)
        const char* deleteSQL = "UPDATE kvs_data SET deleted = 1 WHERE key = ?;";
        rc = sqlite3_prepare_v2( m_pDB, deleteSQL, -1, &m_pStmtDelete, nullptr );
        if( rc != SQLITE_OK )
        {
            LAP_PER_LOG_ERROR << "Failed to prepare delete statement: " << sqlite3_errmsg( m_pDB );
            return core::Result< void >::FromError( makeErrorCode( rc ) );
        }
        
        // GET ALL statement
        const char* getAllSQL = "SELECT key FROM kvs_data WHERE deleted = 0;";
        rc = sqlite3_prepare_v2( m_pDB, getAllSQL, -1, &m_pStmtGetAll, nullptr );
        if( rc != SQLITE_OK )
        {
            LAP_PER_LOG_ERROR << "Failed to prepare getall statement: " << sqlite3_errmsg( m_pDB );
            return core::Result< void >::FromError( makeErrorCode( rc ) );
        }
        
        return core::Result< void >::FromValue();
    }

    void KvsSqliteBackend::finalizeStatements() noexcept
    {
        if( m_pStmtInsert ) { sqlite3_finalize( m_pStmtInsert ); m_pStmtInsert = nullptr; }
        if( m_pStmtUpdate ) { sqlite3_finalize( m_pStmtUpdate ); m_pStmtUpdate = nullptr; }
        if( m_pStmtSelect ) { sqlite3_finalize( m_pStmtSelect ); m_pStmtSelect = nullptr; }
        if( m_pStmtExists ) { sqlite3_finalize( m_pStmtExists ); m_pStmtExists = nullptr; }
        if( m_pStmtDelete ) { sqlite3_finalize( m_pStmtDelete ); m_pStmtDelete = nullptr; }
        if( m_pStmtGetAll ) { sqlite3_finalize( m_pStmtGetAll ); m_pStmtGetAll = nullptr; }
    }

    // ==================== Transaction Management ====================
    
    core::Result< void > KvsSqliteBackend::beginTransaction() noexcept
    {
        if( m_bInTransaction )
        {
            return core::Result< void >::FromValue();
        }
        
        char* errMsg = nullptr;
        core::Int32 rc = sqlite3_exec( m_pDB, "BEGIN IMMEDIATE;", nullptr, nullptr, &errMsg );
        
        if( rc != SQLITE_OK )
        {
            LAP_PER_LOG_ERROR << "Failed to begin transaction: " << ( errMsg ? errMsg : "unknown error" );
            if( errMsg ) sqlite3_free( errMsg );
            return core::Result< void >::FromError( makeErrorCode( rc ) );
        }
        
        m_bInTransaction = true;
        return core::Result< void >::FromValue();
    }

    core::Result< void > KvsSqliteBackend::commitTransaction() noexcept
    {
        if( !m_bInTransaction )
        {
            return core::Result< void >::FromValue();
        }
        
        char* errMsg = nullptr;
        core::Int32 rc = sqlite3_exec( m_pDB, "COMMIT;", nullptr, nullptr, &errMsg );
        
        if( rc != SQLITE_OK )
        {
            LAP_PER_LOG_ERROR << "Failed to commit transaction: " << ( errMsg ? errMsg : "unknown error" );
            if( errMsg ) sqlite3_free( errMsg );
            return core::Result< void >::FromError( makeErrorCode( rc ) );
        }
        
        m_bInTransaction = false;
        return core::Result< void >::FromValue();
    }

    core::Result< void > KvsSqliteBackend::rollbackTransaction() noexcept
    {
        if( !m_bInTransaction )
        {
            return core::Result< void >::FromValue();
        }
        
        char* errMsg = nullptr;
        core::Int32 rc = sqlite3_exec( m_pDB, "ROLLBACK;", nullptr, nullptr, &errMsg );
        
        if( rc != SQLITE_OK )
        {
            LAP_PER_LOG_ERROR << "Failed to rollback transaction: " << ( errMsg ? errMsg : "unknown error" );
            if( errMsg ) sqlite3_free( errMsg );
            return core::Result< void >::FromError( makeErrorCode( rc ) );
        }
        
        m_bInTransaction = false;
        return core::Result< void >::FromValue();
    }

    // ==================== Type Encoding/Decoding (Solution B) ====================
    
    core::String KvsSqliteBackend::encodeValue( const KvsDataType& value ) const noexcept
    {
        core::String result;
        
        // First byte is type marker: 'a' + type_index
        char typeMarker = 'a' + static_cast<char>( ::lap::core::GetVariantIndex( value ) );
        result += typeMarker;
        
        // Convert value to string with full precision
        ::std::ostringstream oss;
        oss << ::std::setprecision( ::std::numeric_limits<core::Double>::max_digits10 );
        
        switch( ::lap::core::GetVariantIndex( value ) )
        {
            case 0:  oss << ::lap::core::get<core::Int8>( value );    break;
            case 1:  oss << ::lap::core::get<core::UInt8>( value );   break;
            case 2:  oss << ::lap::core::get<core::Int16>( value );   break;
            case 3:  oss << ::lap::core::get<core::UInt16>( value );  break;
            case 4:  oss << ::lap::core::get<core::Int32>( value );   break;
            case 5:  oss << ::lap::core::get<core::UInt32>( value );  break;
            case 6:  oss << ::lap::core::get<core::Int64>( value );   break;
            case 7:  oss << ::lap::core::get<core::UInt64>( value );  break;
            case 8:  oss << ( ::lap::core::get<core::Bool>( value ) ? "1" : "0" ); break;
            case 9:  oss << ::lap::core::get<core::Float>( value );   break;
            case 10: oss << ::lap::core::get<core::Double>( value );  break;
            case 11: oss << ::lap::core::get<core::String>( value );  break;
            default:
                LAP_PER_LOG_ERROR << "Unknown variant type: " << ::lap::core::GetVariantIndex( value );
                break;
        }
        
        result += oss.str();
        return result;
    }

    core::Result< KvsDataType > KvsSqliteBackend::decodeValue( core::StringView encodedValue ) const noexcept
    {
        if( encodedValue.empty() )
        {
            return core::Result< KvsDataType >::FromError( PerErrc::kIntegrityCorrupted );
        }
        
        // Extract type from first byte
        core::Int32 typeIndex = static_cast<core::Int32>( encodedValue[0] - 'a' );
        core::StringView dataStr = encodedValue.substr( 1 );
        
        try
        {
            switch( typeIndex )
            {
                case 0:  return core::Result< KvsDataType >::FromValue( static_cast<core::Int8>( ::std::stoi( core::String( dataStr ) ) ) );
                case 1:  return core::Result< KvsDataType >::FromValue( static_cast<core::UInt8>( ::std::stoul( core::String( dataStr ) ) ) );
                case 2:  return core::Result< KvsDataType >::FromValue( static_cast<core::Int16>( ::std::stoi( core::String( dataStr ) ) ) );
                case 3:  return core::Result< KvsDataType >::FromValue( static_cast<core::UInt16>( ::std::stoul( core::String( dataStr ) ) ) );
                case 4:  return core::Result< KvsDataType >::FromValue( static_cast<core::Int32>( ::std::stoi( core::String( dataStr ) ) ) );
                case 5:  return core::Result< KvsDataType >::FromValue( static_cast<core::UInt32>( ::std::stoul( core::String( dataStr ) ) ) );
                case 6:  return core::Result< KvsDataType >::FromValue( static_cast<core::Int64>( ::std::stoll( core::String( dataStr ) ) ) );
                case 7:  return core::Result< KvsDataType >::FromValue( static_cast<core::UInt64>( ::std::stoull( core::String( dataStr ) ) ) );
                case 8:  return core::Result< KvsDataType >::FromValue( static_cast<core::Bool>( dataStr == "1" ) );
                case 9:  return core::Result< KvsDataType >::FromValue( static_cast<core::Float>( ::std::stof( core::String( dataStr ) ) ) );
                case 10: return core::Result< KvsDataType >::FromValue( static_cast<core::Double>( ::std::stod( core::String( dataStr ) ) ) );
                case 11: return core::Result< KvsDataType >::FromValue( core::String( dataStr ) );
                default:
                    LAP_PER_LOG_ERROR << "Invalid type index: " << typeIndex;
                    return core::Result< KvsDataType >::FromError( PerErrc::kIntegrityCorrupted );
            }
        }
        catch( const ::std::exception& e )
        {
            LAP_PER_LOG_ERROR << "Failed to decode value: " << e.what();
            return core::Result< KvsDataType >::FromError( PerErrc::kIntegrityCorrupted );
        }
    }

    // ==================== Public API Implementation ====================
    
    core::Result< core::Vector< core::String > > KvsSqliteBackend::GetAllKeys() const noexcept
    {
        using result = core::Result< core::Vector< core::String > >;
        
        if( !m_bAvailable )
        {
            return result::FromError( PerErrc::kNotInitialized );
        }
        
        core::LockGuard lock( m_mutex );
        
        core::Vector< core::String > keys;
        
        sqlite3_reset( m_pStmtGetAll );
        
        core::Int32 rc;
        while( ( rc = sqlite3_step( m_pStmtGetAll ) ) == SQLITE_ROW )
        {
            const char* key = reinterpret_cast<const char*>( sqlite3_column_text( m_pStmtGetAll, 0 ) );
            if( key )
            {
                keys.push_back( key );
            }
        }
        
        if( rc != SQLITE_DONE )
        {
            LAP_PER_LOG_ERROR << "Failed to get all keys: " << sqlite3_errmsg( m_pDB );
            return result::FromError( makeErrorCode( rc ) );
        }
        
        return result::FromValue( ::std::move( keys ) );
    }

    core::Result< core::Bool > KvsSqliteBackend::KeyExists( core::StringView key ) const noexcept
    {
        using result = core::Result< core::Bool >;
        
        if( !m_bAvailable )
        {
            return result::FromError( PerErrc::kNotInitialized );
        }
        
        core::LockGuard lock( m_mutex );
        
        sqlite3_reset( m_pStmtExists );
        sqlite3_bind_text( m_pStmtExists, 1, key.data(), key.size(), SQLITE_STATIC );
        
        core::Int32 rc = sqlite3_step( m_pStmtExists );
        
        if( rc == SQLITE_ROW )
        {
            return result::FromValue( true );
        }
        else if( rc == SQLITE_DONE )
        {
            return result::FromValue( false );
        }
        else
        {
            LAP_PER_LOG_ERROR << "Failed to check key existence: " << sqlite3_errmsg( m_pDB );
            return result::FromError( makeErrorCode( rc ) );
        }
    }

    core::Result< KvsDataType > KvsSqliteBackend::GetValue( core::StringView key ) const noexcept
    {
        using result = core::Result< KvsDataType >;
        
        if( !m_bAvailable )
        {
            return result::FromError( PerErrc::kNotInitialized );
        }
        
        core::LockGuard lock( m_mutex );
        
        sqlite3_reset( m_pStmtSelect );
        sqlite3_bind_text( m_pStmtSelect, 1, key.data(), key.size(), SQLITE_STATIC );
        
        core::Int32 rc = sqlite3_step( m_pStmtSelect );
        
        if( rc == SQLITE_ROW )
        {
            const char* encodedValue = reinterpret_cast<const char*>( sqlite3_column_text( m_pStmtSelect, 0 ) );
            if( !encodedValue )
            {
                LAP_PER_LOG_ERROR << "NULL value returned for key: " << key;
                return result::FromError( PerErrc::kIntegrityCorrupted );
            }
            
            return decodeValue( encodedValue );
        }
        else if( rc == SQLITE_DONE )
        {
            return result::FromError( PerErrc::kKeyNotFound );
        }
        else
        {
            LAP_PER_LOG_ERROR << "Failed to get value for key '" << key << "': " << sqlite3_errmsg( m_pDB );
            return result::FromError( makeErrorCode( rc ) );
        }
    }

    core::Result< void > KvsSqliteBackend::SetValue( core::StringView key, const KvsDataType& value ) noexcept
    {
        using result = core::Result< void >;
        
        if( !m_bAvailable )
        {
            return result::FromError( PerErrc::kNotInitialized );
        }
        
        core::LockGuard lock( m_mutex );
        
        core::String encodedValue = encodeValue( value );
        
        sqlite3_reset( m_pStmtInsert );
        sqlite3_bind_text( m_pStmtInsert, 1, key.data(), key.size(), SQLITE_STATIC );
        sqlite3_bind_text( m_pStmtInsert, 2, encodedValue.c_str(), encodedValue.size(), SQLITE_TRANSIENT );
        
        core::Int32 rc = sqlite3_step( m_pStmtInsert );
        
        if( rc != SQLITE_DONE )
        {
            LAP_PER_LOG_ERROR << "Failed to set value for key '" << key << "': " << sqlite3_errmsg( m_pDB );
            return result::FromError( makeErrorCode( rc ) );
        }
        
        return result::FromValue();
    }

    core::Result< void > KvsSqliteBackend::RemoveKey( core::StringView key ) noexcept
    {
        using result = core::Result< void >;
        
        if( !m_bAvailable )
        {
            return result::FromError( PerErrc::kNotInitialized );
        }
        
        core::LockGuard lock( m_mutex );
        
        sqlite3_reset( m_pStmtDelete );
        sqlite3_bind_text( m_pStmtDelete, 1, key.data(), key.size(), SQLITE_STATIC );
        
        core::Int32 rc = sqlite3_step( m_pStmtDelete );
        
        if( rc != SQLITE_DONE )
        {
            LAP_PER_LOG_ERROR << "Failed to remove key '" << key << "': " << sqlite3_errmsg( m_pDB );
            return result::FromError( makeErrorCode( rc ) );
        }
        
        return result::FromValue();
    }

    core::Result<void> KvsSqliteBackend::RecoveryKey( core::StringView key ) noexcept
    {
        using result = core::Result< void >;
        
        if( !m_bAvailable )
        {
            return result::FromError( PerErrc::kNotInitialized );
        }
        
        core::LockGuard lock( m_mutex );
        
        // Recovery: set deleted flag to 0
        const char* recoverySQL = "UPDATE kvs_data SET deleted = 0 WHERE key = ?;";
        sqlite3_stmt* stmt = nullptr;
        
        core::Int32 rc = sqlite3_prepare_v2( m_pDB, recoverySQL, -1, &stmt, nullptr );
        if( rc != SQLITE_OK )
        {
            LAP_PER_LOG_ERROR << "Failed to prepare recovery statement: " << sqlite3_errmsg( m_pDB );
            return result::FromError( makeErrorCode( rc ) );
        }
        
        sqlite3_bind_text( stmt, 1, key.data(), key.size(), SQLITE_STATIC );
        rc = sqlite3_step( stmt );
        sqlite3_finalize( stmt );
        
        if( rc != SQLITE_DONE )
        {
            LAP_PER_LOG_ERROR << "Failed to recovery key '" << key << "': " << sqlite3_errmsg( m_pDB );
            return result::FromError( makeErrorCode( rc ) );
        }
        
        return result::FromValue();
    }

    core::Result<void> KvsSqliteBackend::ResetKey( core::StringView key ) noexcept
    {
        using result = core::Result< void >;
        
        if( !m_bAvailable )
        {
            return result::FromError( PerErrc::kNotInitialized );
        }
        
        // Reset: physically delete the key
        core::LockGuard lock( m_mutex );
        
        const char* resetSQL = "DELETE FROM kvs_data WHERE key = ?;";
        sqlite3_stmt* stmt = nullptr;
        
        core::Int32 rc = sqlite3_prepare_v2( m_pDB, resetSQL, -1, &stmt, nullptr );
        if( rc != SQLITE_OK )
        {
            LAP_PER_LOG_ERROR << "Failed to prepare reset statement: " << sqlite3_errmsg( m_pDB );
            return result::FromError( makeErrorCode( rc ) );
        }
        
        sqlite3_bind_text( stmt, 1, key.data(), key.size(), SQLITE_STATIC );
        rc = sqlite3_step( stmt );
        sqlite3_finalize( stmt );
        
        if( rc != SQLITE_DONE )
        {
            LAP_PER_LOG_ERROR << "Failed to reset key '" << key << "': " << sqlite3_errmsg( m_pDB );
            return result::FromError( makeErrorCode( rc ) );
        }
        
        return result::FromValue();
    }

    core::Result<void> KvsSqliteBackend::RemoveAllKeys() noexcept
    {
        using result = core::Result< void >;
        
        if( !m_bAvailable )
        {
            return result::FromError( PerErrc::kNotInitialized );
        }
        
        core::LockGuard lock( m_mutex );
        
        // Soft delete all keys
        char* errMsg = nullptr;
        core::Int32 rc = sqlite3_exec( m_pDB, "UPDATE kvs_data SET deleted = 1;", nullptr, nullptr, &errMsg );
        
        if( rc != SQLITE_OK )
        {
            LAP_PER_LOG_ERROR << "Failed to remove all keys: " << ( errMsg ? errMsg : "unknown error" );
            if( errMsg ) sqlite3_free( errMsg );
            return result::FromError( makeErrorCode( rc ) );
        }
        
        return result::FromValue();
    }

    core::Result<void> KvsSqliteBackend::SyncToStorage() noexcept
    {
        using result = core::Result< void >;
        
        if( !m_bAvailable )
        {
            return result::FromError( PerErrc::kNotInitialized );
        }
        
        core::LockGuard lock( m_mutex );
        
        // Commit any pending transaction
        if( m_bInTransaction )
        {
            auto commitResult = commitTransaction();
            if( !commitResult.HasValue() )
            {
                return commitResult;
            }
        }
        
        // Force WAL checkpoint
        core::Int32 rc = sqlite3_wal_checkpoint_v2( m_pDB, nullptr, SQLITE_CHECKPOINT_FULL, nullptr, nullptr );
        
        if( rc != SQLITE_OK )
        {
            LAP_PER_LOG_ERROR << "Failed to sync to storage: " << sqlite3_errmsg( m_pDB );
            return result::FromError( makeErrorCode( rc ) );
        }
        
        // Vacuum to reclaim space from deleted records (periodically)
        static core::Int32 syncCount = 0;
        if( ++syncCount % 100 == 0 )  // Every 100 syncs
        {
            char* errMsg = nullptr;
            rc = sqlite3_exec( m_pDB, "DELETE FROM kvs_data WHERE deleted = 1;", nullptr, nullptr, &errMsg );
            if( rc != SQLITE_OK )
            {
                LAP_PER_LOG_WARN << "Failed to cleanup deleted records: " << ( errMsg ? errMsg : "unknown error" );
                if( errMsg ) sqlite3_free( errMsg );
            }
        }
        
        return result::FromValue();
    }

    core::Result<void> KvsSqliteBackend::DiscardPendingChanges() noexcept
    {
        using result = core::Result< void >;
        
        if( !m_bAvailable )
        {
            return result::FromError( PerErrc::kNotInitialized );
        }
        
        core::LockGuard lock( m_mutex );
        
        if( m_bInTransaction )
        {
            return rollbackTransaction();
        }
        
        return result::FromValue();
    }

    // ==================== Error Handling ====================
    
    core::ErrorCode KvsSqliteBackend::makeErrorCode( core::Int32 sqliteCode ) const noexcept
    {
        // Map SQLite error codes to Persistency error codes
        switch( sqliteCode )
        {
            case SQLITE_NOTFOUND:
                return core::ErrorCode( PerErrc::kKeyNotFound );
            case SQLITE_FULL:
            case SQLITE_TOOBIG:
                return core::ErrorCode( PerErrc::kOutOfStorageSpace );
            case SQLITE_CORRUPT:
            case SQLITE_FORMAT:
                return core::ErrorCode( PerErrc::kIntegrityCorrupted );
            case SQLITE_IOERR:
            case SQLITE_CANTOPEN:
            case SQLITE_PERM:
            case SQLITE_READONLY:
                return core::ErrorCode( PerErrc::kPhysicalStorageFailure );
            default:
                return core::ErrorCode( PerErrc::kPhysicalStorageFailure );  // Use generic error
        }
    }

} // pm
} // lap