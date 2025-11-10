/**
 * @file CKvsSqliteBackend.hpp
 * @author ddkv587 (ddkv587@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2024-02-02
 * 
 * 
 */
#ifndef LAP_PERSISTENCY_KVSSQLITEBACKEND_HPP
#define LAP_PERSISTENCY_KVSSQLITEBACKEND_HPP

#include <sqlite3.h>
#include <lap/core/CMemory.hpp>
#include <lap/core/CSync.hpp>
#include <memory>

#include "CDataType.hpp"
#include "CKvsBackend.hpp"

namespace lap
{
namespace pm
{
    class KeyValueStorage;
    class KvsSqliteBackend final : public KvsBackend
    {
    public:
        IMP_OPERATOR_NEW(KvsSqliteBackend)
    
    public:
        core::Bool                                                      available() const noexcept override { return m_bAvailable; }

        core::Result< core::Vector< core::String > >                    GetAllKeys() const noexcept override;
        core::Result< core::Bool >                                      KeyExists ( core::StringView key ) const noexcept override;
        core::Result< KvsDataType >                                     GetValue( core::StringView key ) const noexcept override;
        core::Result< void >                                            SetValue( core::StringView key, const KvsDataType &value ) noexcept override;
        core::Result< void >                                            RemoveKey( core::StringView key ) noexcept override;
        core::Result< void >                                            RecoveryKey( core::StringView key ) noexcept override;
        core::Result< void >                                            ResetKey( core::StringView key ) noexcept override;
        core::Result< void >                                            RemoveAllKeys() noexcept override;
        core::Result< void >                                            SyncToStorage() noexcept override;
        core::Result< void >                                            DiscardPendingChanges() noexcept override;

        ~KvsSqliteBackend();
        explicit KvsSqliteBackend( core::StringView );
        KvsSqliteBackend( KvsSqliteBackend&& );

    protected:
        KvsSqliteBackend() = delete;
        KvsSqliteBackend( const KvsSqliteBackend& ) = delete;
        KvsSqliteBackend& operator=( const KvsSqliteBackend& ) = delete;

        friend class KeyValueStorage;

    private:
        // Helper functions
        core::Result< void >                initializeDatabase() noexcept;
        core::Result< void >                prepareStatements() noexcept;
        void                                finalizeStatements() noexcept;
        core::Result< void >                beginTransaction() noexcept;
        core::Result< void >                commitTransaction() noexcept;
        core::Result< void >                rollbackTransaction() noexcept;
        
        // Type encoding/decoding (consistent with PropertyBackend Solution B)
        core::String                        encodeValue( const KvsDataType& value ) const noexcept;
        core::Result< KvsDataType >         decodeValue( core::StringView encodedValue ) const noexcept;
        
        // Error handling
        core::ErrorCode                     makeErrorCode( core::Int32 sqliteCode ) const noexcept;
        
    private:
        core::Bool                          m_bAvailable{ false };
        core::String                        m_strFile;
        sqlite3*                            m_pDB{ nullptr };
        mutable core::Mutex                 m_mutex;
        
        // Prepared statements for performance
        sqlite3_stmt*                       m_pStmtInsert{ nullptr };
        sqlite3_stmt*                       m_pStmtUpdate{ nullptr };
        sqlite3_stmt*                       m_pStmtSelect{ nullptr };
        sqlite3_stmt*                       m_pStmtExists{ nullptr };
        sqlite3_stmt*                       m_pStmtDelete{ nullptr };
        sqlite3_stmt*                       m_pStmtGetAll{ nullptr };
        
        // Transaction management
        core::Bool                          m_bInTransaction{ false };
    };
} // pm
} // ara

#endif
