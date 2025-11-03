/**
 * @file CKeyValueStorage.hpp
 * @author ddkv587 (ddkv587@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2024-02-02
 * 
 * 
 */
#ifndef LAP_PERSISTENCY_KEYVALUESTORAGE_HPP
#define LAP_PERSISTENCY_KEYVALUESTORAGE_HPP

#include <core/CMemory.hpp>

#include "CDataType.hpp"

namespace lap
{
namespace pm
{
    class KvsBackend;
    class KeyValueStorage final
    {
    public:
        IMP_OPERATOR_NEW(KeyValueStorage)
        
        ~KeyValueStorage() noexcept;

        core::Result< core::Bool >                                      initialize( core::StringView strConfig = "", core::Bool bCreate = false ) noexcept;
        void                                                            uninitialize() noexcept;
        inline core::Bool                                               isInitialized() noexcept                            { return m_bInitialized; }
        inline core::Bool                                               isResourceBusy() noexcept                           { return m_bResourceBusy; }

        core::Result< core::Vector< core::String > >                    GetAllKeys() const noexcept;

        core::Result< core::Bool >                                      KeyExists ( core::StringView key ) const noexcept;

        template< class T >
        core::Result< T >                                               GetValue( core::StringView key ) const noexcept;

        template< class T >
        core::Result<void>                                              SetValue( core::StringView key, const T& value ) noexcept;

        core::Result<void>                                              RemoveKey( core::StringView key ) noexcept;
        core::Result<void>                                              RecoveryKey( core::StringView key ) noexcept;
        core::Result<void>                                              ResetKey( core::StringView key ) noexcept;
        core::Result<void>                                              RemoveAllKeys() noexcept;
        core::Result<void>                                              SyncToStorage() const noexcept;
        core::Result<void>                                              DiscardPendingChanges() noexcept;

    protected:
        friend class CPersistencyManager;

        static core::SharedHandle< KeyValueStorage > create( core::StringView path ) noexcept
        {
            struct _KeyValueStorage : std::allocator< KeyValueStorage > {
                void construct( void* p, core::StringView path ) noexcept       { ::new( p ) KeyValueStorage( path ); }
                void destroy( KeyValueStorage* p ) noexcept                     { p->~KeyValueStorage(); }
            };

            return ::std::allocate_shared< KeyValueStorage >( _KeyValueStorage(), path ); 
        }

        static core::SharedHandle< KeyValueStorage > create( core::StringView path, const KvsBackendType& type ) noexcept
        {
            struct _KeyValueStorage : std::allocator< KeyValueStorage > {
                void construct( void* p, core::StringView path, const KvsBackendType& type ) noexcept   { ::new( p ) KeyValueStorage( path, type ); }
                void destroy( KeyValueStorage* p ) noexcept                                             { p->~KeyValueStorage(); }
            };

            return ::std::allocate_shared< KeyValueStorage >( _KeyValueStorage(), path, type ); 
        }

        explicit KeyValueStorage( core::StringView ) noexcept;
        explicit KeyValueStorage( core::StringView, const KvsBackendType & ) noexcept;

        KeyValueStorage( KeyValueStorage&& ) noexcept;
        KeyValueStorage& operator=( KeyValueStorage&& ) noexcept;

        KeyValueStorage( const KeyValueStorage& ) = delete;
        KeyValueStorage& operator=( const KeyValueStorage& ) = delete;

        core::Result< void >                            RecoverKeyValueStorage() noexcept;
        core::Result< void >                            ResetKeyValueStorage() noexcept;
        core::Result< core::UInt64 >                    GetCurrentKeyValueStorageSize() noexcept;

    private:
        core::Bool                                      m_bInitialized{ false };
        core::Bool                                      m_bResourceBusy{ false };
        core::StringView                                m_strPath;
        core::UniqueHandle< KvsBackend >                m_pKvsBackend;
    };

    core::Result< core::SharedHandle< KeyValueStorage > >               OpenKeyValueStorage( const core::InstanceSpecifier &, core::Bool, KvsBackendType ) noexcept;
    core::Result< core::SharedHandle< KeyValueStorage > >               OpenKeyValueStorage( const core::InstanceSpecifier & ) noexcept;
    core::Result< void >                                                RecoverKeyValueStorage( const core::InstanceSpecifier & ) noexcept;
    core::Result< void >                                                ResetKeyValueStorage( const core::InstanceSpecifier & ) noexcept;
    core::Result< core::UInt64 >                                        GetCurrentKeyValueStorageSize( const core::InstanceSpecifier & ) noexcept;
} // namespace pm
} // namespace lap

#endif
