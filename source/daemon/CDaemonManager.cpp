#include <thread>
#include <iostream>

#include "CDaemonManager.hpp"
#include "CPersistencyConnection.hpp"

namespace lap
{
namespace pm
{
namespace daemon
{
    DaemonManager::DaemonManager()
        : m_ioContext{ ::std::make_unique<bio::io_context>() }
        , m_persistencyManager{ ::std::make_shared< ::lap::pm::CPersistencyManager >() }
    {
        std::cout << "DaemonManager constructor" << std::endl;
    }

    DaemonManager::~DaemonManager()
    {
        std::cout << "DaemonManager destructor" << std::endl;
        uninitialize();
    }

    core::Bool DaemonManager::initialize()
    {
        std::cout << "DaemonManager::initialize()" << std::endl;
        
        m_bInitialized = (m_ioContext != nullptr);
        
        if (m_bInitialized) {
            std::cout << "DaemonManager initialization successful" << std::endl;
        } else {
            std::cout << "DaemonManager initialization failed" << std::endl;
        }

        return m_bInitialized;
    }

    void DaemonManager::uninitialize()
    {
        m_acceptor->close();
        while( m_ioContext->poll() );

        stop();
    }

    void DaemonManager::start()
    {
        //fprintf(stderr, "pm start 111...");
        if ( !m_acceptor ) {
            m_acceptor = ::std::make_unique<bio::ip::tcp::acceptor>( *m_ioContext, bio::ip::tcp::endpoint{ bio::ip::tcp::v4(), m_iPort } );
            //fprintf(stderr, "pm start 222...");
            if ( !m_acceptor ) {
                // TODO : cannot make accept
                throw PerException( PerErrc::kNotInitialized );
            }
            m_acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
            //fprintf(stderr, "pm start 333...");
        }
        innerDoAccept();

        //fprintf(stderr, "pm start 444...");
        if ( m_ioContext->stopped() ) {
            // reset if context is stopped
            m_ioContext->reset();
        }

        //fprintf(stderr, "pm start 555...");

        m_bRunning = true;
        m_tLooper = ::std::make_unique< ::std::thread >( &DaemonManager::innerLoop, this );
        pthread_setname_np( m_tLooper->native_handle(), "DaemonManager::innerLoop" );
        //fprintf(stderr, "pm start 666...");
    }

    void DaemonManager::stop()
    {
        m_ioContext->stop();

        m_bRunning = false;

        if ( m_tLooper && m_tLooper->joinable() ) {
            m_tLooper->join();
        }
        m_tLooper.reset();
    }

    core::Result< core::Vector< core::String > > DaemonManager::GetAllKeys() const noexcept
    {
        std::cout << "DaemonManager::GetAllKeys() called" << std::endl;
        
        // Get default KVS instance 
        auto kvsResult = m_persistencyManager->getKvsStorage(core::InstanceSpecifier("default"), true, KvsBackendType::kvsProperty);
        if (!kvsResult.HasValue()) {
            std::cout << "Failed to get KVS storage: " << kvsResult.Error().Message().data() << std::endl;
            return core::Result< core::Vector< core::String > >::FromError(kvsResult.Error());
        }
        
        auto kvs = kvsResult.Value();
        return kvs->GetAllKeys();
    }

    core::Result< core::Bool > DaemonManager::KeyExists( core::StringView key) const noexcept
    {
        std::cout << "DaemonManager::KeyExists(" << key.data() << ") called" << std::endl;
        
        auto kvsResult = m_persistencyManager->getKvsStorage(core::InstanceSpecifier("default"), true, KvsBackendType::kvsProperty);
        if (!kvsResult.HasValue()) {
            std::cout << "Failed to get KVS storage: " << kvsResult.Error().Message().data() << std::endl;
            return core::Result< core::Bool >::FromError(kvsResult.Error());
        }
        
        auto kvs = kvsResult.Value();
        return kvs->KeyExists(key);
    }

    core::Result< KvsDataType > DaemonManager::GetValue( core::StringView key ) const noexcept
    {
        std::cout << "DaemonManager::GetValue(" << key.data() << ") called" << std::endl;
        
        auto kvsResult = m_persistencyManager->getKvsStorage(core::InstanceSpecifier("default"), true, KvsBackendType::kvsProperty);
        if (!kvsResult.HasValue()) {
            std::cout << "Failed to get KVS storage: " << kvsResult.Error().Message().data() << std::endl;
            return core::Result< KvsDataType >::FromError(kvsResult.Error());
        }
        
        auto kvs = kvsResult.Value();
        return kvs->GetValue(key);
    }

    core::Result<void> DaemonManager::SetValue( core::StringView key, const KvsDataType &value ) noexcept
    {
        std::cout << "DaemonManager::SetValue(" << key.data() << ") called" << std::endl;
        
        auto kvsResult = m_persistencyManager->getKvsStorage(core::InstanceSpecifier("default"), true, KvsBackendType::kvsProperty);
        if (!kvsResult.HasValue()) {
            std::cout << "Failed to get KVS storage: " << kvsResult.Error().Message().data() << std::endl;
            return core::Result<void>::FromError(kvsResult.Error());
        }
        
        auto kvs = kvsResult.Value();
        return kvs->SetValue(key, value);
    }

    core::Result<void> DaemonManager::RemoveKey( core::StringView key ) noexcept
    {
        std::cout << "DaemonManager::RemoveKey(" << key.data() << ") called" << std::endl;
        
        auto kvsResult = m_persistencyManager->getKvsStorage(core::InstanceSpecifier("default"), true, KvsBackendType::kvsProperty);
        if (!kvsResult.HasValue()) {
            std::cout << "Failed to get KVS storage: " << kvsResult.Error().Message().data() << std::endl;
            return core::Result<void>::FromError(kvsResult.Error());
        }
        
        auto kvs = kvsResult.Value();
        return kvs->RemoveKey(key);
    }

    core::Result<void> DaemonManager::RecoveryKey( core::StringView key ) noexcept
    {
        std::cout << "DaemonManager::RecoveryKey(" << key.data() << ") called" << std::endl;
        // Recovery functionality - delegate to CPersistencyManager
        return m_persistencyManager->RecoverKeyValueStorage(core::InstanceSpecifier("default"));
    }

    core::Result<void> DaemonManager::ResetKey( core::StringView key ) noexcept
    {
        std::cout << "DaemonManager::ResetKey(" << key.data() << ") called" << std::endl;
        // Reset functionality - delegate to CPersistencyManager
        return m_persistencyManager->ResetKeyValueStorage(core::InstanceSpecifier("default"));
    }

    core::Result<void> DaemonManager::RemoveAllKeys() noexcept
    {
        std::cout << "DaemonManager::RemoveAllKeys() called" << std::endl;
        
        auto kvsResult = m_persistencyManager->getKvsStorage(core::InstanceSpecifier("default"), true, KvsBackendType::kvsProperty);
        if (!kvsResult.HasValue()) {
            std::cout << "Failed to get KVS storage: " << kvsResult.Error().Message().data() << std::endl;
            return core::Result<void>::FromError(kvsResult.Error());
        }
        
        auto kvs = kvsResult.Value();
        return kvs->RemoveAllKeys();
    }

    core::Result<void> DaemonManager::SyncToStorage() const noexcept
    {
        std::cout << "DaemonManager::SyncToStorage() called" << std::endl;
        
        auto kvsResult = m_persistencyManager->getKvsStorage(core::InstanceSpecifier("default"), true, KvsBackendType::kvsProperty);
        if (!kvsResult.HasValue()) {
            std::cout << "Failed to get KVS storage: " << kvsResult.Error().Message().data() << std::endl;
            return core::Result<void>::FromError(kvsResult.Error());
        }
        
        auto kvs = kvsResult.Value();
        return kvs->SyncToStorage();
    }

    core::Result<void> DaemonManager::DiscardPendingChanges() noexcept
    {
        std::cout << "DaemonManager::DiscardPendingChanges() called" << std::endl;
        
        auto kvsResult = m_persistencyManager->getKvsStorage(core::InstanceSpecifier("default"), true, KvsBackendType::kvsProperty);
        if (!kvsResult.HasValue()) {
            std::cout << "Failed to get KVS storage: " << kvsResult.Error().Message().data() << std::endl;
            return core::Result<void>::FromError(kvsResult.Error());
        }
        
        auto kvs = kvsResult.Value();
        return kvs->DiscardPendingChanges();
    }

    void DaemonManager::innerLoop()
    {
        while( m_bRunning && !m_ioContext->stopped() ) {
            m_ioContext->run();
        }
        // TODO : log asio run exit...
        //fprintf(stderr, "DaemonManager::innerLoop exit...");
    }

    void DaemonManager::innerDoAccept()
    {
        if ( !m_acceptor->is_open() ) {
        // check if socket is close
            return;
        }

        m_acceptor->async_accept(
            [this]( boost::system::error_code ec, bio::ip::tcp::socket socket )
            {
                if ( !m_acceptor->is_open() ) {
                    // check if socket is close
                    return;
                }

                if ( !ec ) {
                    ::std::make_shared< CPersistencyConnection >( std::move( socket ) )->start();
                }

                innerDoAccept();
            });
    }
}
}
}