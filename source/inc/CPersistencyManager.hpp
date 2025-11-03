/**
 * @file CPersistencyManager.hpp
 * @author ddkv587 (ddkv587@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2024-02-02
 * 
 * 
 */
#ifndef LAP_PERSISTENCY_PERSISTENCYMANAGER_HPP
#define LAP_PERSISTENCY_PERSISTENCYMANAGER_HPP

#include <core/CInstanceSpecifier.hpp>
#include <core/CResult.hpp>
#include <core/CMemory.hpp>
#include <core/CSync.hpp>

#include "CDataType.hpp"

namespace lap
{
namespace pm
{
    class FileStorage;
    class KeyValueStorage;
    class CPersistencyManager final
    {
    public:
        IMP_OPERATOR_NEW(CPersistencyManager)
        
    private:
        using _FileStorageMap   = core::UnorderedMap< core::String, core::SharedHandle< FileStorage > >;
        using _KvsMap           = core::UnorderedMap< core::String, core::SharedHandle< KeyValueStorage > >;

        #define DEF_PM_CONFIG_INDICATE     "pmConfig"

    public:
        static CPersistencyManager& getInstance() noexcept
        {
            static CPersistencyManager instance;

            return instance;
        }

        core::Bool                              initialize() noexcept;
        void                                    uninitialize() noexcept;
        inline core::Bool                       isInitialized() noexcept             { return m_bInitialized; }

        // for FileStorage
        core::Result< core::SharedHandle< FileStorage > >       getFileStorage( const core::InstanceSpecifier&, core::Bool bCreate = false ) noexcept;
        core::Result< void >                    RecoverAllFiles( const core::InstanceSpecifier & ) noexcept;
        core::Result< void >                    ResetAllFiles( const core::InstanceSpecifier & ) noexcept;
        core::Result< core::UInt64 >            GetCurrentFileStorageSize( const core::InstanceSpecifier & ) noexcept;

        // for KeyValueStorage
        core::Result< core::SharedHandle< KeyValueStorage > >   getKvsStorage( const core::InstanceSpecifier&, core::Bool bCreate = false, KvsBackendType type = KvsBackendType::kvsFile ) noexcept;
        core::Result< void >                    RecoverKeyValueStorage( const core::InstanceSpecifier & ) noexcept;
        core::Result< void >                    ResetKeyValueStorage( const core::InstanceSpecifier & ) noexcept;
        core::Result< core::UInt64 >            GetCurrentKeyValueStorageSize( const core::InstanceSpecifier & ) noexcept;

    protected:
        CPersistencyManager() noexcept;
        ~CPersistencyManager() noexcept;

    private:
        core::Bool                              parseFromConfig( core::StringView ) noexcept;
        core::Bool                              saveToConfig( core::StringView ) noexcept;

    private:
        core::Bool                              m_bInitialized{ false };

        core::StringView                        m_strPrexPath{""};
        core::UInt32                            m_maxNumberOfFiles{32};
        LevelUpdateStrategy                     m_updateStrategy{LevelUpdateStrategy::Overwrite};
        RedundancyStrategy                      m_redundantStrategy{RedundancyStrategy::None};

        core::Mutex                             m_mtxFsMap;
        _FileStorageMap                         m_fsMap;

        core::Mutex                             m_mtxKvsMap;
        _KvsMap                                 m_kvsMap;
    };
} // namespace pm
} // namespace lap
#endif
