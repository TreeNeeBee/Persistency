#ifndef LAP_PERSISTENCY_KVSPROPERTYBACKEND_HPP_
#define LAP_PERSISTENCY_KVSPROPERTYBACKEND_HPP_

#include <core/CMemory.hpp>

#include "CDataType.hpp"
#include "CKvsBackend.hpp"

namespace lap
{
namespace pm
{
namespace util
{
    class KeyValueStorageBase;
    class KvsPropertyBackend final : public ::lap::pm::KvsBackend
    {
    public:
        IMP_OPERATOR_NEW(KvsPropertyBackend)

    private:
        typedef core::UnorderedMap< core::String, KvsDataType > _mapValue;

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

        explicit KvsPropertyBackend( core::StringView strFile );
        ~KvsPropertyBackend() noexcept;

    protected:
        KvsPropertyBackend() = delete;
        KvsPropertyBackend( const KvsPropertyBackend& ) = delete;
        KvsPropertyBackend( KvsPropertyBackend&& ) = delete;
        KvsPropertyBackend& operator=( const KvsPropertyBackend& ) = delete;

        friend class KeyValueStorageBase;

    private:
        core::Bool                   m_bAvailable{ false };
        core::String                 m_strFile;
    };
} // util
} // pm
} // lap

#endif