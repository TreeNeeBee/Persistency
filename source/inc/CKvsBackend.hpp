/**
 * @file CKvsBackend.hpp
 * @author ddkv587 (ddkv587@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2024-02-02
 * 
 * 
 */
#ifndef LAP_PERSISTENCY_KVSBACKEND_HPP
#define LAP_PERSISTENCY_KVSBACKEND_HPP

#include <core/CResult.hpp>
#include <core/CString.hpp>
#include <core/CMemory.hpp>

#include "CDataType.hpp"

namespace lap
{
namespace pm
{
    #define DEF_KVS_MAGIC_KEY           '^'
    #define DEF_KVS_MAGIC_KEY_INDEX     0
    #define DEF_KVS_MAGIC_TYPE_INDEX    1

    class KvsBackend
    {
    public:
        IMP_OPERATOR_NEW(KvsBackend)
        
    protected:
        struct KVS_Less
        {
            core::Bool operator()( const core::String& __x, const core::String& __y ) const 
            {
                core::Int8 xOffset = ( __x.size() > 2 && __x[DEF_KVS_MAGIC_KEY_INDEX] == DEF_KVS_MAGIC_KEY ) ? 2 : 0;
                core::Int8 yOffset = ( __y.size() > 2 && __y[DEF_KVS_MAGIC_KEY_INDEX] == DEF_KVS_MAGIC_KEY ) ? 2 : 0;

                core::StringView strX( __x.c_str() + xOffset, __x.size() - xOffset );
                core::StringView strY( __y.c_str() + yOffset, __y.size() - yOffset );

                return strX < strY;
            }
        };

    public:
        virtual core::Bool                                                      available() const noexcept { return false; }
        virtual core::Result< core::Vector< core::String > >                    GetAllKeys() const noexcept;
        virtual core::Result< core::Bool >                                      KeyExists ( core::StringView key ) const noexcept;
        virtual core::Result< KvsDataType >                                     GetValue( core::StringView key ) const noexcept;
        virtual core::Result<void>                                              SetValue( core::StringView key, const KvsDataType &value ) noexcept;

        virtual core::Result<void>                                              RemoveKey( core::StringView key ) noexcept;
        virtual core::Result<void>                                              RecoveryKey( core::StringView key ) noexcept;
        virtual core::Result<void>                                              ResetKey( core::StringView key ) noexcept;
        virtual core::Result<void>                                              RemoveAllKeys() noexcept;
        virtual core::Result<void>                                              SyncToStorage() noexcept;
        virtual core::Result<void>                                              DiscardPendingChanges() noexcept;

        KvsBackend() noexcept = default;
        virtual ~KvsBackend() noexcept = default;

        static void                                                             formatKey( core::String &key, EKvsDataTypeIndicate valueType );
        static EKvsDataTypeIndicate                                             getDataType( core::StringView strKey );

    protected:
        KvsBackend( KvsBackend const& ) = delete;
        KvsBackend( KvsBackend&& ) = delete;
        KvsBackend& operator=( KvsBackend&& ) & noexcept = delete;
        KvsBackend& operator=( KvsBackend const& ) = delete;
    };
} // namespace pm
} // namespace lap

#endif
