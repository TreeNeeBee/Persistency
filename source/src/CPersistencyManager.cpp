#include <lap/core/CPath.hpp>
#include "CFileStorage.hpp"
#include "CKeyValueStorage.hpp"
#include "CPersistencyManager.hpp"

namespace lap
{
namespace per
{
    core::Bool CPersistencyManager::initialize() noexcept
    {
        if ( m_bInitialized )     return true;

        //parseFromConfig(  );

        m_bInitialized = true;

        return true;
    }

    void CPersistencyManager::uninitialize() noexcept
    {
        if ( !m_bInitialized )    return;

        // auto finalize file storage
        {
            core::LockGuard lock( m_mtxFsMap );

            for ( auto&& it = m_fsMap.begin(); it != m_fsMap.end(); ++it ) {
                it->second->uninitialize();
            }

            m_fsMap.clear();
        }

        // auto finalize kvs storage
        {
            core::LockGuard lock( m_mtxKvsMap );

            for ( auto&& it = m_kvsMap.begin(); it != m_kvsMap.end(); ++it ) {
                it->second->uninitialize();
            }

            m_kvsMap.clear();
        }

        m_bInitialized = false;
    }

    core::Result< core::SharedHandle< FileStorage > > CPersistencyManager::getFileStorage( const core::InstanceSpecifier& indicate, core::Bool bCreate ) noexcept
    {
        using result = core::Result< core::SharedHandle< FileStorage > >;

        if ( !m_bInitialized ) return result::FromError( PerErrc::kNotInitialized );

        core::StringView strFolder{ indicate.ToString() };

        if ( strFolder.empty() ) {
            LAP_PER_LOG_WARN << "CPersistencyManager::getFileStorage with invalid path " 
                                << strFolder.data()
                                << ", try to use default";

            return result::FromError( PerErrc::kStorageNotFound );
        } else {
            auto&& it = m_fsMap.find( strFolder.data() );

            if ( it != m_fsMap.end() ) {
                // check file storage status
                core::LockGuard lock( m_mtxFsMap );

                if ( !it->second->initialize() )        return result::FromError( PerErrc::kPhysicalStorageFailure );

                // check if storage in reset or update
                if ( !it->second->isResourceBusy() )    return result::FromError( PerErrc::kResourceBusy );

                return result::FromValue( it->second );
            } else {
                // make sure folder is exist
                if ( !bCreate ) return result::FromError( PerErrc::kStorageNotFound );

                if ( core::Path::createDirectory( strFolder ) ) {
                    auto fs = FileStorage::create( strFolder.data() );
                    m_fsMap.emplace( strFolder.data(), fs );

                    return result::FromValue( fs );
                } else {
                    LAP_PER_LOG_WARN << "CPersistencyManager::getFileStorage can not create or access with "
                                        << strFolder.data()
                                        << ", try to use default";
                    return result::FromError( PerErrc::kStorageNotFound );
                }
            }
        }
    }

    core::Result< void > CPersistencyManager::RecoverAllFiles( const core::InstanceSpecifier &fs ) noexcept
    {
        if ( !m_bInitialized ) return core::Result< void >::FromError( PerErrc::kNotInitialized );

        auto&& it = m_fsMap.find( fs.ToString().data() );

        if ( it != m_fsMap.end() ) {
            return it->second->RecoverAllFiles();
        } else {
            return core::Result< void >::FromError( PerErrc::kStorageNotFound );
        }
    }

    core::Result< void > CPersistencyManager::ResetAllFiles( const core::InstanceSpecifier &fs ) noexcept
    {
        if ( !m_bInitialized ) return core::Result< void >::FromError( PerErrc::kNotInitialized );

        auto&& it = m_fsMap.find( fs.ToString().data() );

        if ( it != m_fsMap.end() ) {
            return it->second->ResetAllFiles();
        } else {
            return core::Result< void >::FromError( PerErrc::kStorageNotFound );
        }
    }

    core::Result< core::UInt64 > CPersistencyManager::GetCurrentFileStorageSize( const core::InstanceSpecifier &fs ) noexcept
    {
        if ( !m_bInitialized ) return core::Result< core::UInt64 >::FromError( PerErrc::kNotInitialized );

        auto&& it = m_fsMap.find( fs.ToString().data() );

        if ( it != m_fsMap.end() ) {
            return it->second->GetCurrentFileStorageSize();
        } else {
            return core::Result< core::UInt64 >::FromError( PerErrc::kStorageNotFound );
        }
    }

    core::Result< core::SharedHandle< KeyValueStorage > > CPersistencyManager::getKvsStorage( const core::InstanceSpecifier& indicate, core::Bool bCreate, KvsBackendType type ) noexcept
    {
        using result = core::Result< core::SharedHandle< KeyValueStorage > >;

        if ( !m_bInitialized ) return result::FromError( PerErrc::kNotInitialized );

        core::StringView strFolder{ indicate.ToString() };

        if ( strFolder.empty() ) {
            LAP_PER_LOG_WARN << "CPersistencyManager::getKvsStorage with invalid path " 
                                << strFolder.data()
                                << ", try to use default";

            return result::FromError( PerErrc::kStorageNotFound );
        } else {
            auto&& it = m_kvsMap.find( strFolder.data() );

            if ( it != m_kvsMap.end() ) {
                // check kvs storage status
                core::LockGuard lock( m_mtxKvsMap );

                // check if storage can initialize
                if ( !it->second->initialize() )        return result::FromError( PerErrc::kPhysicalStorageFailure );

                // check if storage in reset or update
                if ( !it->second->isResourceBusy() )    return result::FromError( PerErrc::kResourceBusy );

                return result::FromValue( it->second );
            } else {
                if ( !bCreate ) return result::FromError( PerErrc::kStorageNotFound );

                // make sure folder is exist
                if ( core::Path::createDirectory( strFolder ) ) {
                    auto kvs = KeyValueStorage::create( strFolder.data(), type );
                    m_kvsMap.emplace( strFolder.data(), kvs );

                    return result::FromValue( kvs );
                } else {
                    LAP_PER_LOG_WARN << "CPersistencyManager::getKvsStorage can not create or access with "
                                        << strFolder.data()
                                        << ", try to use default";
                    return result::FromError( PerErrc::kStorageNotFound );
                }
            }
        }
    }

    core::Result< void > CPersistencyManager::RecoverKeyValueStorage( const core::InstanceSpecifier &kvs ) noexcept
    {
        using result = core::Result< void >;

        if ( !m_bInitialized ) return result::FromError( PerErrc::kNotInitialized );

        auto&& it = m_kvsMap.find( kvs.ToString().data() );

        if ( it != m_kvsMap.end() ) {
            return it->second->RecoverKeyValueStorage();
        } else {
            return result::FromError( PerErrc::kStorageNotFound );
        }
    }

    core::Result< void > CPersistencyManager::ResetKeyValueStorage( const core::InstanceSpecifier &kvs ) noexcept
    {
        using result = core::Result< void >;

        if ( !m_bInitialized ) return result::FromError( PerErrc::kNotInitialized );

        auto&& it = m_kvsMap.find( kvs.ToString().data() );

        if ( it != m_kvsMap.end() ) {
            return it->second->ResetKeyValueStorage();
        } else {
            return result::FromError( PerErrc::kStorageNotFound );
        }
    }

    core::Result< core::UInt64 > CPersistencyManager::GetCurrentKeyValueStorageSize( const core::InstanceSpecifier & ) noexcept
    {
        using result = core::Result< core::UInt64 >;

        return result::FromValue( static_cast< core::UInt64 >( 0 ) );
    }


    CPersistencyManager::CPersistencyManager() noexcept
    {
        ;
    }
    
    CPersistencyManager::~CPersistencyManager() noexcept
    {
        uninitialize();
    }

    core::Bool CPersistencyManager::parseFromConfig( core::StringView ) noexcept
    {
        return true;
    }
}
}