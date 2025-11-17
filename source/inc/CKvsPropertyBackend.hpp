#ifndef LAP_PERSISTENCY_KVSPROPERTYBACKEND_HPP_
#define LAP_PERSISTENCY_KVSPROPERTYBACKEND_HPP_

#include <lap/core/CMemory.hpp>

#include "CDataType.hpp"
#include "IKvsBackend.hpp"

namespace lap
{
namespace per
{
namespace util
{
    class KeyValueStorageBase;
    
    /**
     * @brief Shared memory-based KVS backend with persistent storage support
     * 
     * Features:
     * - In-memory operations via Boost.Interprocess shared memory
     * - Configurable persistence backend (File or SQLite)
     * - Automatic load/sync with persistence backend
     * - High-performance read/write (no disk I/O per operation)
     * - Inter-process communication support
     */
    class KvsPropertyBackend final : public ::lap::per::IKvsBackend
    {
    public:
        IMP_OPERATOR_NEW(KvsPropertyBackend)

    private:
        typedef core::UnorderedMap< core::String, KvsDataType > _mapValue;

    public:
        core::Bool                                                      available() const noexcept override { return m_bAvailable; }
        KvsBackendType                                                  GetBackendType() const noexcept override { return KvsBackendType::kvsProperty; }
        core::Bool                                                      SupportsPersistence() const noexcept override { return true; }

        core::Result< core::Vector< core::String > >                    GetAllKeys() const noexcept override;
        core::Result< core::Bool >                                      KeyExists ( core::StringView key ) const noexcept override;
        core::Result< KvsDataType >                                     GetValue( core::StringView key ) const noexcept override;
        core::Result< void >                                            SetValue( core::StringView key, const KvsDataType &value ) noexcept override;
        core::Result< void >                                            RemoveKey( core::StringView key ) noexcept override;
        core::Result< void >                                            RecoverKey( core::StringView key ) noexcept override;
        core::Result< void >                                            ResetKey( core::StringView key ) noexcept override;
        core::Result< void >                                            RemoveAllKeys() noexcept override;
        core::Result< void >                                            SyncToStorage() noexcept override;
        core::Result< void >                                            DiscardPendingChanges() noexcept override;
        core::Result< core::UInt64 >                                    GetSize() const noexcept override;
        core::Result< core::UInt32 >                                    GetKeyCount() const noexcept override;

        /**
         * @brief Default shared memory size (1MB)
         */
        static constexpr core::Size DEFAULT_SHM_SIZE = 1ul << 20;

        /**
         * @brief Construct Property backend with identifier and persistence backend
         * @param identifier Unique identifier for this KVS instance
         * @param persistenceBackend Backend type for persistence (kvsFile, kvsSqlite, or kvsNone for memory-only)
         * @param shmSize Shared memory size in bytes (default: 1MB, or from config)
         * @param config Optional config to override defaults (for shared memory size, etc.)
         */
        explicit KvsPropertyBackend( core::StringView identifier, 
                                      KvsBackendType persistenceBackend = KvsBackendType::kvsFile,
                                      core::Size shmSize = DEFAULT_SHM_SIZE,
                                      const PersistencyConfig* config = nullptr );
        ~KvsPropertyBackend() noexcept;

    protected:
        KvsPropertyBackend() = delete;
        KvsPropertyBackend( const KvsPropertyBackend& ) = delete;
        KvsPropertyBackend( KvsPropertyBackend&& ) = delete;
        KvsPropertyBackend& operator=( const KvsPropertyBackend& ) = delete;

        friend class KeyValueStorageBase;

    private:
        /**
         * @brief Load data from persistence backend to shared memory
         * @return Result indicating success or error
         */
        core::Result<void> loadFromPersistence() noexcept;
        
        /**
         * @brief Save data from shared memory to persistence backend
         * @return Result indicating success or error
         */
        core::Result<void> saveToPersistence() noexcept;

    private:
        core::Bool                      m_bAvailable{ false };
        core::String                    m_strIdentifier;          // Instance identifier
        core::String                    m_strShmName;             // Shared memory name
        core::Size                      m_shmSize;                // Shared memory size
        KvsBackendType                  m_persistenceBackend;     // File or SQLite
        ::std::unique_ptr<IKvsBackend>  m_pPersistenceBackend;    // Actual persistence backend
        core::Bool                      m_bDirty{ false };        // Track if sync needed
    };
} // util
} // pm
} // lap

#endif