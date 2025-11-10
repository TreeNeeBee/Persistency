/**
 * @file CKvsFileBackend.hpp
 * @author ddkv587 (ddkv587@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2024-02-02
 * 
 * 
 */
#ifndef LAP_PERSISTENCY_KVSFILEBACKEND_HPP
#define LAP_PERSISTENCY_KVSFILEBACKEND_HPP

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <lap/core/CResult.hpp>
#include <lap/core/CMemory.hpp>

#include "CDataType.hpp"
#include "CKvsBackend.hpp"

namespace lap
{
namespace pm
{
    class KeyValueStorage;
    class KvsFileBackend final : public KvsBackend
    {
    public:
        IMP_OPERATOR_NEW(KvsFileBackend)
        
    private:
        typedef core::Map< core::String, KvsDataType >           _ValueMap;
        typedef ::boost::property_tree::basic_ptree< core::String, KvsDataType, KvsBackend::KVS_Less > _JsonValue;

    public:
        core::Bool                                                      available() const noexcept override { return m_bAvailable; }

        core::Result< core::Vector< core::String > >                    GetAllKeys() const noexcept override;
        core::Result< core::Bool >                                      KeyExists ( core::StringView key ) const noexcept override;
        core::Result< KvsDataType >                                     GetValue( core::StringView key ) const noexcept override;
        core::Result<void>                                              SetValue( core::StringView key, const KvsDataType &value ) noexcept override;
        core::Result<void>                                              RemoveKey( core::StringView key ) noexcept override;
        core::Result<void>                                              RecoveryKey( core::StringView key ) noexcept override;
        core::Result<void>                                              ResetKey( core::StringView key ) noexcept override;
        core::Result<void>                                              RemoveAllKeys() noexcept override;
        core::Result<void>                                              SyncToStorage() noexcept override;
        core::Result<void>                                              DiscardPendingChanges() noexcept override;

        ~KvsFileBackend() noexcept;
        explicit KvsFileBackend( core::StringView ) noexcept;

    protected:
        friend class KeyValueStorage;

        core::Result<void>                                              parseFromFile( core::StringView ) noexcept;
        core::Result<void>                                              saveToFile( core::StringView ) noexcept;

        KvsFileBackend() = delete;
        KvsFileBackend( const KvsFileBackend& ) = delete;
        KvsFileBackend( KvsFileBackend&& ) = delete;
        KvsFileBackend& operator=( const KvsFileBackend& ) = delete;

    private:
        core::Bool                                          m_bAvailable{ false };
        core::String                                        m_strFile;      // use for reset
        _JsonValue                                          m_kvsRoot;
        //_ValueMap                                         m_mapValue;
    };
} // pm
} // ara

#endif
